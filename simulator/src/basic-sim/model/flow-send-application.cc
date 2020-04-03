/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Georgia Institute of Technology
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
 * Author: Simon
 * Adapted from BulkSendApplication by:
 * Author: George F. Riley <riley@ece.gatech.edu>
 */

#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/tcp-tx-buffer.h"
#include "flow-send-application.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FlowSendApplication");

NS_OBJECT_ENSURE_REGISTERED (FlowSendApplication);

TypeId
FlowSendApplication::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::FlowSendApplication")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddConstructor<FlowSendApplication>()
            .AddAttribute("SendSize", "The amount of data to send each time.",
                          UintegerValue(10000),
                          MakeUintegerAccessor(&FlowSendApplication::m_sendSize),
                          MakeUintegerChecker<uint32_t>(1))
            .AddAttribute("Remote", "The address of the destination",
                          AddressValue(),
                          MakeAddressAccessor(&FlowSendApplication::m_peer),
                          MakeAddressChecker())
            .AddAttribute("MaxBytes",
                          "The total number of bytes to send. "
                          "Once these bytes are sent, "
                          "no data  is sent again. The value zero means "
                          "that there is no limit.",
                          UintegerValue(0),
                          MakeUintegerAccessor(&FlowSendApplication::m_maxBytes),
                          MakeUintegerChecker<uint64_t>())
            .AddAttribute("Protocol", "The type of protocol to use.",
                          TypeIdValue(TcpSocketFactory::GetTypeId()),
                          MakeTypeIdAccessor(&FlowSendApplication::m_tid),
                          MakeTypeIdChecker())
            .AddTraceSource("Tx", "A new packet is created and is sent",
                            MakeTraceSourceAccessor(&FlowSendApplication::m_txTrace),
                            "ns3::Packet::TracedCallback");
    return tid;
}


FlowSendApplication::FlowSendApplication()
        : m_socket(0),
          m_connected(false),
          m_totBytes(0),
          m_completionTimeNs(-1),
          m_closedNormally(false),
          m_closedByError(false),
          m_ackedBytes(0),
          m_isCompleted(false) {
    NS_LOG_FUNCTION(this);
}

FlowSendApplication::~FlowSendApplication() {
    NS_LOG_FUNCTION(this);
}

void
FlowSendApplication::DoDispose(void) {
    NS_LOG_FUNCTION(this);

    m_socket = 0;
    // chain up
    Application::DoDispose();
}

void FlowSendApplication::StartApplication(void) { // Called at time specified by Start
    NS_LOG_FUNCTION(this);

    // Create the socket if not already
    if (!m_socket) {
        m_socket = Socket::CreateSocket(GetNode(), m_tid);

        // Must be TCP basically
        if (m_socket->GetSocketType() != Socket::NS3_SOCK_STREAM &&
            m_socket->GetSocketType() != Socket::NS3_SOCK_SEQPACKET) {
            NS_FATAL_ERROR("Using FlowSend with an incompatible socket type. "
                           "FlowSend requires SOCK_STREAM or SOCK_SEQPACKET. "
                           "In other words, use TCP instead of UDP.");
        }

        // Bind socket
        if (Inet6SocketAddress::IsMatchingType(m_peer)) {
            if (m_socket->Bind6() == -1) {
                NS_FATAL_ERROR("Failed to bind socket");
            }
        } else if (InetSocketAddress::IsMatchingType(m_peer)) {
            if (m_socket->Bind() == -1) {
                NS_FATAL_ERROR("Failed to bind socket");
            }
        }

        // Connect, no receiver
        m_socket->Connect(m_peer);
        m_socket->ShutdownRecv();

        // Callbacks
        m_socket->SetConnectCallback(
                MakeCallback(&FlowSendApplication::ConnectionSucceeded, this),
                MakeCallback(&FlowSendApplication::ConnectionFailed, this)
        );
        m_socket->SetSendCallback(MakeCallback(&FlowSendApplication::DataSend, this));
        m_socket->SetCloseCallbacks(
                MakeCallback(&FlowSendApplication::SocketClosedNormal, this),
                MakeCallback(&FlowSendApplication::SocketClosedError, this)
        );
    }
    if (m_connected) {
        SendData();
    }
}

void FlowSendApplication::StopApplication(void) { // Called at time specified by Stop
    NS_LOG_FUNCTION(this);
    if (m_socket != 0) {
        m_socket->Close();
        m_connected = false;
    } else {
        NS_LOG_WARN("FlowSendApplication found null socket to close in StopApplication");
    }
}

void FlowSendApplication::SendData(void) {
    NS_LOG_FUNCTION(this);
    while (m_maxBytes == 0 || m_totBytes < m_maxBytes) { // Time to send more

        // uint64_t to allow the comparison later.
        // the result is in a uint32_t range anyway, because
        // m_sendSize is uint32_t.
        uint64_t toSend = m_sendSize;
        // Make sure we don't send too many
        if (m_maxBytes > 0) {
            toSend = std::min(toSend, m_maxBytes - m_totBytes);
        }

        NS_LOG_LOGIC("sending packet at " << Simulator::Now());
        Ptr <Packet> packet = Create<Packet>(toSend);
        int actual = m_socket->Send(packet);
        if (actual > 0) {
            m_totBytes += actual;
            m_txTrace(packet);
        }
        // We exit this loop when actual < toSend as the send side
        // buffer is full. The "DataSent" callback will pop when
        // some buffer space has freed up.
        if ((unsigned) actual != toSend) {
            break;
        }
    }
    // Check if time to close (all sent)
    if (m_totBytes == m_maxBytes && m_connected) {
        m_socket->Close();
        m_connected = false;
    }
}

void FlowSendApplication::ConnectionSucceeded(Ptr <Socket> socket) {
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_LOGIC("FlowSendApplication Connection succeeded");
    m_connected = true;
    SendData();
}

void FlowSendApplication::ConnectionFailed(Ptr <Socket> socket) {
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_LOGIC("FlowSendApplication, Connection Failed");
}

void FlowSendApplication::DataSend(Ptr <Socket>, uint32_t) {
    NS_LOG_FUNCTION(this);
    if (m_connected) { // Only send new data if the connection has completed
        SendData();
    }
}

int64_t FlowSendApplication::GetAckedBytes() {
    if (m_closedNormally || m_closedByError) {
        return m_ackedBytes;
    } else {
        return m_totBytes - m_socket->GetObject<TcpSocketBase>()->GetTxBuffer()->Size();
    }
}

int64_t FlowSendApplication::GetCompletionTimeNs() {
    return m_completionTimeNs;
}

bool FlowSendApplication::IsCompleted() {
    return m_isCompleted;
}

bool FlowSendApplication::IsClosedNormally() {
    return m_closedNormally;
}

bool FlowSendApplication::IsClosedByError() {
    return m_closedByError;
}

void FlowSendApplication::SocketClosedNormal(Ptr <Socket> socket) {
    m_completionTimeNs = Simulator::Now().GetNanoSeconds();
    m_closedByError = false;
    m_closedNormally = true;
    if (m_socket->GetObject<TcpSocketBase>()->GetTxBuffer()->Size() != 0) {
        throw std::runtime_error("Socket closed normally but send buffer is not empty");
    }
    m_ackedBytes = m_totBytes - m_socket->GetObject<TcpSocketBase>()->GetTxBuffer()->Size();
    m_isCompleted = m_ackedBytes == m_maxBytes;
    m_socket = 0;
}

void FlowSendApplication::SocketClosedError(Ptr <Socket> socket) {
    m_closedByError = true;
    m_closedNormally = false;
    m_ackedBytes = m_totBytes - m_socket->GetObject<TcpSocketBase>()->GetTxBuffer()->Size();
    m_isCompleted = false;
    m_socket = 0;
}

} // Namespace ns3
