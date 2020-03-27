/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 ETH Zurich
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Simon Kassing <kassings@ethz.ch>
 */

#define NS_LOG_APPEND_CONTEXT                                   \
  if (m_ipv4 && m_ipv4->GetObject<Node> ()) { \
      std::clog << Simulator::Now ().GetSeconds () \
                << " [node " << m_ipv4->GetObject<Node> ()->GetId () << "] "; }

#include <iomanip>
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-route.h"
#include "ns3/output-stream-wrapper.h"
#include "ipv4-fast-static-host-routing.h"

namespace ns3 {

    NS_LOG_COMPONENT_DEFINE ("Ipv4FastStaticHostRouting");

    NS_OBJECT_ENSURE_REGISTERED (Ipv4FastStaticHostRouting);

    TypeId
    Ipv4FastStaticHostRouting::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::Ipv4FastStaticHostRouting")
                .SetParent<Ipv4RoutingProtocol>()
                .SetGroupName("Internet")
                .AddConstructor<Ipv4FastStaticHostRouting>();
        return tid;
    }

    Ipv4FastStaticHostRouting::Ipv4FastStaticHostRouting() : m_ipv4(0) {
        NS_LOG_FUNCTION(this);
    }

    /**
     * Add a host route to the interface list.
     *
     * TODO: Check for duplicates
     *
     * @param dest          Destination IP address (no mask)
     * @param interface     Interface index
     */
    void
    Ipv4FastStaticHostRouting::AddHostRouteTo(Ipv4Address dest, uint32_t interface) {
        if (this->host_ip_to_if_list.find(dest.Get()) == this->host_ip_to_if_list.end()) {
            this->host_ip_to_if_list.insert({dest.Get(), std::vector<uint32_t>()});
        }
        std::vector<uint32_t> *if_list = &this->host_ip_to_if_list[dest.Get()];
        if_list->push_back(interface);
    }

    /**
     * Lookup an interface to send something going to a certain destination, coming in from a certain network device.
     *
     * @param dest  Destination address
     * @param oif   Origin interface
     * @return
     */
    Ptr <Ipv4Route>
    Ipv4FastStaticHostRouting::LookupStatic(Ipv4Address dest, Ptr <NetDevice> oif) {

        // Multi-cast not supported
        if (dest.IsLocalMulticast()) {
            throw std::runtime_error("Multi-cast is not supported");
        }

        uint32_t if_idx = 0;

        // Loop-back (not added to the routing table explicitly)
        if (Ipv4Mask("255.0.0.0").IsMatch(dest, Ipv4Address("127.0.0.1"))) {
            if_idx = 0;

        } else { // If not loop-back, it must be delivered to another interface

            // Perform look-up
            std::map< uint32_t, std::vector<uint32_t>>::iterator it = this->host_ip_to_if_list.find(dest.Get());
            if (it == this->host_ip_to_if_list.end()) {
                throw std::runtime_error("No route found: not permitted");
            } else {
                std::vector <uint32_t> *if_list = &(it->second);
                if_idx = (*if_list)[0];
            }

        }

        // Create routing entry
        Ptr<Ipv4Route> rtentry = Create<Ipv4Route>();
        rtentry->SetDestination(dest);
        rtentry->SetSource(m_ipv4->SourceAddressSelection(if_idx, dest));
        rtentry->SetGateway(m_ipv4->GetAddress(if_idx, 0).GetLocal());  // m_ipv4->GetNetDevice(if_idx));
        rtentry->SetOutputDevice(m_ipv4->GetNetDevice(if_idx));
        return rtentry;

    }

    Ptr <Ipv4Route>
    Ipv4FastStaticHostRouting::RouteOutput(Ptr <Packet> p, const Ipv4Header &header, Ptr <NetDevice> oif,
                                           Socket::SocketErrno &sockerr) {
        NS_LOG_FUNCTION(this << p << header << oif << sockerr);
        Ipv4Address destination = header.GetDestination();

        // Multi-cast to multiple interfaces is not supported
        if (destination.IsMulticast()) {
            throw std::runtime_error("Multi-cast not supported");
        }

        // Perform lookup
        sockerr = Socket::ERROR_NOTERROR;
        return LookupStatic(destination, oif);

    }

    bool
    Ipv4FastStaticHostRouting::RouteInput(Ptr<const Packet> p, const Ipv4Header &ipHeader, Ptr<const NetDevice> idev,
                                          UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                                          LocalDeliverCallback lcb, ErrorCallback ecb) {
        NS_ASSERT(m_ipv4 != 0);

        // Check if input device supports IP
        NS_ASSERT(m_ipv4->GetInterfaceForDevice(idev) >= 0);
        uint32_t iif = m_ipv4->GetInterfaceForDevice(idev);

        // Multi-cast
        if (ipHeader.GetDestination().IsMulticast()) {
            throw std::runtime_error("Multi-cast not supported.");
        }

        // Local delivery at the input interface
        if (m_ipv4->IsDestinationAddress(ipHeader.GetDestination(), iif)) {
            if (lcb.IsNull()) {
                throw std::runtime_error("Local callback cannot be null");
            }
            lcb(p, ipHeader, iif);
            return true;
        }

        // Check if input device supports IP forwarding
        if (m_ipv4->IsForwarding(iif) == false) {
            NS_LOG_LOGIC("Forwarding disabled for this interface");
            ecb(p, ipHeader, Socket::ERROR_NOROUTETOHOST);
            return true;
        }

        // Uni-cast delivery
        ucb(LookupStatic(ipHeader.GetDestination()), p, ipHeader);
        return true;

    }

    Ipv4FastStaticHostRouting::~Ipv4FastStaticHostRouting() {
        NS_LOG_FUNCTION(this);
    }

    void
    Ipv4FastStaticHostRouting::NotifyInterfaceUp(uint32_t i) {

        // One IP address per interface
        if (m_ipv4->GetNAddresses(i) != 1) {
            throw std::runtime_error("Each interface is permitted exactly one IP address.");
        }

        // Get interface single IP's address and mask
        Ipv4Address if_addr = m_ipv4->GetAddress(i, 0).GetLocal();
        Ipv4Mask if_mask = m_ipv4->GetAddress(i, 0).GetMask();

        // Loopback interface must be 0
        if (i == 0) {
            if (if_addr != Ipv4Address("127.0.0.1") || if_mask != Ipv4Mask("255.0.0.0")) {
                throw std::runtime_error("Loopback interface 0 must have IP 127.0.0.1 and mask 255.0.0.0");
            }

        } else { // Towards another interface

            // Check that the subnet mask is maintained
            if (if_mask.Get() != Ipv4Mask("255.255.255.0").Get()) {
                throw std::runtime_error("Each interface must have a subnet mask of 255.255.255.0");
            }

            // Add the route to the local interface
            this->AddHostRouteTo(m_ipv4->GetAddress(i, 0).GetLocal(), i);

        }

    }

    void
    Ipv4FastStaticHostRouting::NotifyInterfaceDown(uint32_t i) {
        throw std::runtime_error("Interfaces are not permitted to go down.");
    }

    void
    Ipv4FastStaticHostRouting::NotifyAddAddress(uint32_t interface, Ipv4InterfaceAddress address) {
        if (m_ipv4->IsUp(interface)) {
            throw std::runtime_error("Not permitted to add IP addresses after the interface has gone up.");
        }
    }

    void
    Ipv4FastStaticHostRouting::NotifyRemoveAddress(uint32_t interface, Ipv4InterfaceAddress address) {
        if (m_ipv4->IsUp(interface)) {
            throw std::runtime_error("Not permitted to remove IP addresses after the interface has gone up.");
        }
    }

    void
    Ipv4FastStaticHostRouting::SetIpv4(Ptr <Ipv4> ipv4) {
        NS_LOG_FUNCTION(this << ipv4);
        NS_ASSERT(m_ipv4 == 0 && ipv4 != 0);
        m_ipv4 = ipv4;
        for (uint32_t i = 0; i < m_ipv4->GetNInterfaces(); i++) {
            if (m_ipv4->IsUp(i)) {
                NotifyInterfaceUp(i);
            } else {
                NotifyInterfaceDown(i);
            }
        }
    }

    void Ipv4FastStaticHostRouting::PrintRoutingTable(Ptr<OutputStreamWrapper> stream, Time::Unit unit) const {
        throw std::runtime_error("Not yet implemented");
    }

} // namespace ns3
