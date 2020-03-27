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
#include "ipv4-arbitrary-routing.h"

namespace ns3 {

    NS_LOG_COMPONENT_DEFINE ("Ipv4ArbitraryRouting");

    NS_OBJECT_ENSURE_REGISTERED (Ipv4ArbitraryRouting);

    TypeId
    Ipv4ArbitraryRouting::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::Ipv4ArbitraryRouting")
                .SetParent<Ipv4RoutingProtocol>()
                .SetGroupName("Internet")
                .AddConstructor<Ipv4ArbitraryRouting>();
        return tid;
    }

    Ipv4ArbitraryRouting::Ipv4ArbitraryRouting() : m_ipv4(0) {
        NS_LOG_FUNCTION(this);
    }

    /**
     * Lookup an interface to send something going to a certain destination, coming in from a certain network device.
     *
     * @param dest  Destination address
     * @param oif   Origin interface
     * @return
     */
    Ptr<Ipv4Route>
    Ipv4ArbitraryRouting::LookupStatic (const Ipv4Address& dest, const Ipv4Header &header, Ptr<const Packet> p, Ptr<NetDevice> oif) {

        // Multi-cast not supported
        if (dest.IsLocalMulticast()) {
            throw std::runtime_error("Multi-cast is not supported");
        }

        // Decide interface index
        uint32_t if_idx = 0;
        if (Ipv4Mask("255.0.0.0").IsMatch(dest, Ipv4Address("127.0.0.1"))) { // Loop-back
            if_idx = 0;

        } else { // If not loop-back, it goes to the arbiter
                 // Local delivery has already been handled if it was input
            if_idx = m_routingArbiter->decide_next_interface(m_ipv4->GetObject<Node>()->GetId(), p, header);
        }

        // Create routing entry
        Ptr<Ipv4Route> rtentry = Create<Ipv4Route>();
        rtentry->SetDestination(dest);
        rtentry->SetSource(m_ipv4->SourceAddressSelection(if_idx, dest));
        // rtentry->SetGateway(Ipv4Address("0.0.0.0")); // This also works because a point-to-point network device
                                                        // does not care about ARP resolution.
        uint32_t this_side_ip = m_ipv4->GetAddress(if_idx, 0).GetLocal().Get();
        rtentry->SetGateway(Ipv4Address(this_side_ip - 1 + 2 * (this_side_ip % 2)));
        rtentry->SetOutputDevice(m_ipv4->GetNetDevice(if_idx));
        return rtentry;

    }

    Ptr <Ipv4Route>
    Ipv4ArbitraryRouting::RouteOutput(Ptr <Packet> p, const Ipv4Header &header, Ptr <NetDevice> oif,
                                           Socket::SocketErrno &sockerr) {
        NS_LOG_FUNCTION(this << p << header << oif << sockerr);
        Ipv4Address destination = header.GetDestination();

        // Multi-cast to multiple interfaces is not supported
        if (destination.IsMulticast()) {
            throw std::runtime_error("Multi-cast not supported");
        }

        // Perform lookup
        sockerr = Socket::ERROR_NOTERROR;
        return LookupStatic(destination, header, p, oif);

    }

    bool
    Ipv4ArbitraryRouting::RouteInput(Ptr<const Packet> p, const Ipv4Header &ipHeader, Ptr<const NetDevice> idev,
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

        // Local delivery
        if (m_ipv4->IsDestinationAddress(ipHeader.GetDestination(), iif)) { // WeakESModel is set by default to true,
                                                                            // as such it works for any IP address
                                                                            // on any interface of the node
            if (lcb.IsNull()) {
                throw std::runtime_error("Local callback cannot be null");
            }
            lcb(p, ipHeader, iif);
            return true;
        }

        // Check if input device supports IP forwarding
        if (m_ipv4->IsForwarding(iif) == false) {
            throw std::runtime_error("Forwarding must be enabled for every interface");
        }

        // Uni-cast delivery
        ucb(LookupStatic(ipHeader.GetDestination(), ipHeader, p), p, ipHeader);
        return true;

    }

    Ipv4ArbitraryRouting::~Ipv4ArbitraryRouting() {
        NS_LOG_FUNCTION(this);
    }

    void
    Ipv4ArbitraryRouting::NotifyInterfaceUp(uint32_t i) {

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

        }

    }

    void
    Ipv4ArbitraryRouting::NotifyInterfaceDown(uint32_t i) {
        throw std::runtime_error("Interfaces are not permitted to go down.");
    }

    void
    Ipv4ArbitraryRouting::NotifyAddAddress(uint32_t interface, Ipv4InterfaceAddress address) {
        if (m_ipv4->IsUp(interface)) {
            throw std::runtime_error("Not permitted to add IP addresses after the interface has gone up.");
        }
    }

    void
    Ipv4ArbitraryRouting::NotifyRemoveAddress(uint32_t interface, Ipv4InterfaceAddress address) {
        if (m_ipv4->IsUp(interface)) {
            throw std::runtime_error("Not permitted to remove IP addresses after the interface has gone up.");
        }
    }

    void
    Ipv4ArbitraryRouting::SetIpv4(Ptr <Ipv4> ipv4) {
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

    void 
    Ipv4ArbitraryRouting::PrintRoutingTable(Ptr<OutputStreamWrapper> stream, Time::Unit unit) const {
        throw std::runtime_error("Not yet implemented");
    }

    void
    Ipv4ArbitraryRouting::SetRoutingArbiter (RoutingArbiter* routingArbiter) {
        m_routingArbiter = routingArbiter;
    }

} // namespace ns3
