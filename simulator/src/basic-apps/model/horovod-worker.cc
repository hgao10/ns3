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
#include "ns3/string.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/tcp-tx-buffer.h"
#include "ns3/exp-util.h"
#include "horovod-worker.h"
#include <fstream>

#include "ns3/address-utils.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("HorovodWorker");

NS_OBJECT_ENSURE_REGISTERED (HorovodWorker);


TypeId
HorovodWorker::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::HorovodWorker")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddConstructor<HorovodWorker>()
            // sink related parameter
            .AddAttribute("Local",
                "The Address on which to Bind the rx socket.",
                AddressValue(),
                MakeAddressAccessor(&HorovodWorker::m_local),
                MakeAddressChecker())
            // .AddAttribute("SendSize", "The amount of data to send each time.",
            //               UintegerValue(100000),
            //               MakeUintegerAccessor(&HorovodWorker::m_sendSize),
            //               MakeUintegerChecker<uint32_t>(1))
            .AddAttribute("Remote", "The address of the destination",
                          AddressValue(),
                          MakeAddressAccessor(&HorovodWorker::m_peer),
                          MakeAddressChecker())
            .AddAttribute("MaxBytes",
                          "The total number of bytes to send. "
                          "Once these bytes are sent, "
                          "no data  is sent again. The value zero means "
                          "that there is no limit.",
                          UintegerValue(0),
                          MakeUintegerAccessor(&HorovodWorker::m_maxBytes),
                          MakeUintegerChecker<uint64_t>())
            .AddAttribute("FlowId",
                          "Flow identifier",
                          UintegerValue(0),
                          MakeUintegerAccessor(&HorovodWorker::m_flowId),
                          MakeUintegerChecker<uint64_t>())
            .AddAttribute("Protocol", "The type of protocol to use.",
                          TypeIdValue(TcpSocketFactory::GetTypeId()),
                          MakeTypeIdAccessor(&HorovodWorker::m_tid),
                          MakeTypeIdChecker())
            .AddAttribute ("BaseLogsDir",
                           "Base logging directory (flow logging will be placed here, i.e. logs_dir/flow_[flow id]_{progress, cwnd, rtt}.txt",
                           StringValue (""),
                           MakeStringAccessor (&HorovodWorker::m_baseLogsDir),
                           MakeStringChecker ())
            .AddTraceSource("Tx", "A new packet is created and is sent",
                            MakeTraceSourceAccessor(&HorovodWorker::m_txTrace),
                            "ns3::Packet::TracedCallback");
    return tid;
}


HorovodWorker::HorovodWorker()
        : m_receive_socket(0),
          m_send_socket(0),
          m_connected(false),
          m_totBytes(0),
          m_completionTimeNs(-1),
          m_connFailed(false),
          m_closedNormally(false),
          m_closedByError(false),
          m_ackedBytes(0),
          m_isCompleted(false) {
    NS_LOG_FUNCTION(this);
}

HorovodWorker::~HorovodWorker() {
    NS_LOG_FUNCTION(this);
}

RingAllReduce::RingAllReduce(uint32_t size){
    r_size_bytes = size;
}

RingAllReduce::~RingAllReduce(){
    NS_LOG_FUNCTION(this);
}

uint32_t RingAllReduce::GetSize(){
    NS_LOG_FUNCTION(this);
    return r_size_bytes;
}


void
HorovodWorker::DoDispose(void) {
    NS_LOG_FUNCTION(this);
    m_send_socket = 0;
    m_receive_socket = 0;

    // clean listen socketlist
    m_socketList.clear();

    // chain up
    Application::DoDispose();
}

void HorovodWorker::StartApplication(void) { // Called at time specified by Start
    NS_LOG_FUNCTION(this);

    // Create the socket if not already
    if (!m_send_socket) {
        m_send_socket = Socket::CreateSocket(GetNode(), m_tid);
        printf("created send socket \n");

        // Must be TCP basically
        if (m_send_socket->GetSocketType() != Socket::NS3_SOCK_STREAM &&
            m_send_socket->GetSocketType() != Socket::NS3_SOCK_SEQPACKET) {
            NS_FATAL_ERROR("Using FlowSend with an incompatible socket type. "
                           "FlowSend requires SOCK_STREAM or SOCK_SEQPACKET. "
                           "In other words, use TCP instead of UDP.");
        }

        // Bind socket
        if (Inet6SocketAddress::IsMatchingType(m_peer)) {
            if (m_send_socket->Bind6() == -1) {
                printf("failed to bind6 socket\n");
                NS_FATAL_ERROR("Failed to bind socket");
            }
        } else if (InetSocketAddress::IsMatchingType(m_peer)) {
            if (m_send_socket->Bind() == -1) {
                printf("failed to bind socket\n");
                NS_FATAL_ERROR("Failed to bind socket");
            }
        }

        // Connect, no receiver
        m_send_socket->Connect(m_peer);
        m_send_socket->ShutdownRecv();

        // Callbacks
        m_send_socket->SetConnectCallback(
                MakeCallback(&HorovodWorker::ConnectionSucceeded, this),
                MakeCallback(&HorovodWorker::ConnectionFailed, this)
        );
        
        m_send_socket->SetSendCallback(MakeCallback(&HorovodWorker::SendData, this));
        m_send_socket->SetCloseCallbacks(
                MakeCallback(&HorovodWorker::SocketClosedNormal, this),
                MakeCallback(&HorovodWorker::SocketClosedError, this)
        );

    }

    // Create a socket which is always in LISTEN state
    // As soon as it processes a SYN in ProcessListen(),
    // it forks itself into a new socket, which
    // keeps the accept and close callbacks
    if (!m_receive_socket) {
        m_receive_socket = Socket::CreateSocket(GetNode(), m_tid);
        printf("created receive socket\n");
        if (m_receive_socket->Bind(m_local) == -1) {
            NS_FATAL_ERROR("Failed to bind socket");
        }
        m_receive_socket->Listen();
        m_receive_socket->ShutdownSend();
        if (addressUtils::IsMulticast(m_local)) {
            throw std::runtime_error("No support for UDP here");
        }
    }

    // Callbacks
    m_receive_socket->SetRecvCallback(MakeCallback(&HorovodWorker::HandleRead, this));
    m_receive_socket->SetAcceptCallback(
            MakeNullCallback<bool, Ptr<Socket>,const Address &>(),
            MakeCallback(&HorovodWorker::HandleAccept, this)
    );



}

void HorovodWorker::StopApplication(void) { // Called at time specified by Stop
    NS_LOG_FUNCTION(this);
    if (m_send_socket != 0) {
        m_send_socket->Close();
        m_connected = false;
    } else {
        NS_LOG_WARN("HorovodWorker found null socket to close in StopApplication");
    }

    while (!m_socketList.empty()) {
        Ptr <Socket> socket = m_socketList.front();
        m_socketList.pop_front();
        socket->Close();
    }
    if (m_receive_socket) {
        m_receive_socket->Close();
    }
    
}

void HorovodWorker::HandleAccept(Ptr<Socket> socket, const Address &from) {
    NS_LOG_FUNCTION(this << socket << from);
    socket->SetRecvCallback (MakeCallback (&HorovodWorker::HandleRead, this));
    socket->SetCloseCallbacks(
            MakeCallback(&HorovodWorker::HandlePeerClose, this),
            MakeCallback(&HorovodWorker::HandlePeerError, this)
    );
    m_socketList.push_back(socket);
}

void HorovodWorker::HandleRead(Ptr<Socket> socket) {
    NS_LOG_FUNCTION (this << socket);

    // Immediately from the socket drain all the packets it has received
    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from))) {
        if (packet->GetSize() == 0) { // EOFs
            break;
        }
        m_totalRx += packet->GetSize ();
        std::cout<<"Received up to "<< m_totalRx<<"\n";
        std::cout<<"Curr time: "<< Simulator::Now()<<std::endl;
        // Other fields that could be useful here if actually did something:
        // Size: packet->GetSize()
        // Source IP: InetSocketAddress::ConvertFrom(from).GetIpv4 ()
        // Source port: InetSocketAddress::ConvertFrom (from).GetPort ()
        // Our own IP / port: Address localAddress; socket->GetSockName (localAddress);

    }

}

void HorovodWorker::HandlePeerClose(Ptr<Socket> socket) {
    NS_LOG_FUNCTION(this << socket);
    CleanUp(socket);
}

void HorovodWorker::HandlePeerError(Ptr<Socket> socket) {
    NS_LOG_FUNCTION(this << socket);
    CleanUp(socket);
}

void HorovodWorker::CleanUp(Ptr<Socket> socket) {
    NS_LOG_FUNCTION(this << socket);
    bool found = false;
    std::list<Ptr<Socket>>::iterator it;
    for (it = m_socketList.begin(); it != m_socketList.end(); ++it) {
        if (*it == socket) {
            m_socketList.erase(it);
            found = true;
            break;
        }
    }
    if (!found) {
        throw std::runtime_error("When trying to clean up socket, could not find the socket.");
    }
}


// void HorovodWorker::SendData(Ptr <Socket>, uint32_t) {
    // NS_LOG_FUNCTION(this);

    // //readytosend = [layer1_p2, layer2_p1, layer2_p2];
    // //if (socket->BufferSize() > 4kb) {
    //     // check again in 1ms
    //     // wait for buffer to be sent, then schedule SendData again
    // //    return;
    // //}

    // while(m_curr_send_data_bytes[m_tx_layer_idx] < m_layer_size_bytes[m_tx_layer_idx]){
    //     uint64_t left = m_layer_size_bytes[m_tx_layer_idx] - m_curr_send_data_bytes[m_tx_layer_idx];
    //     NS_LOG_LOGIC("sending packet at " << Simulator::Now());
    //     Ptr <Packet> packet = Create<Packet>(left);
    //     int actual = m_send_socket->Send(packet);
    //     if (actual > 0) {
    //         m_curr_send_data_bytes[m_tx_layer_idx] += actual;
    //         m_txTrace(packet);
    //     }
    //     // We exit this loop when actual < left as the send side
    //     // buffer is full. The "DataSent" callback will pop when
    //     // some buffer space has freed up.
    //     if ((unsigned) actual != left) {
    //         break;
    //     }
    // }

    // if(m_curr_send_data_bytes[m_tx_layer_idx] == m_layer_size_bytes[m_tx_layer_idx]){
    //     // completed sending all data from current layer
    //     m_curr_send_data_bytes[m_tx_layer_idx] = 0;
    //     printf("Completed sending gradients for layer %d", m_tx_layer_idx);
    // }

    

// }

 void HorovodWorker::SetLeftNeighbor(Ptr<HorovodWorker> leftneighbor){
     m_leftneighbor = leftneighbor;
 }


void HorovodWorker::SendData(Ptr <Socket>, uint32_t) {
    NS_LOG_FUNCTION(this);
    if (m_fifo_transmission_queue.empty()){
        std::cout<<"fifo transmission queue empty \n";
        // sendData otherwise is always called right after connection is established
        return;
    }
    

    // while (m_maxBytes == 0 || m_totBytes < m_maxBytes || m_fifo_transmission_queue.empty() == false) { // Time to send more

    //     // uint64_t to allow the comparison later.
    //     // the result is in a uint32_t range anyway, because
    //     // m_sendSize is uint32_t.
    //     uint64_t toSend = 100000;
    //     // Make sure we don't send too many
    //     if (m_maxBytes > 0) {
    //         toSend = std::min(toSend, m_maxBytes - m_totBytes);
    //     }

    //     NS_LOG_LOGIC("sending packet at " << Simulator::Now());
    //     uint64_t txbuffersize = m_send_socket->GetObject<TcpSocketBase>()->GetTxBuffer()->Size();
    //     std::cout <<"tosend: "<<toSend<<" txbuffer: "<<txbuffersize << std::endl;
    //     Ptr <Packet> packet = Create<Packet>(toSend);
    //     int actual = m_send_socket->Send(packet);
    //     if (actual > 0) {
    //         m_totBytes += actual;
    //         m_txTrace(packet);
    //     }
    //     // We exit this loop when actual < toSend as the send side
    //     // buffer is full. The "DataSent" callback will pop when
    //     // some buffer space has freed up.
    //     if ((unsigned) actual != toSend) {
    //         std::cout<<"Break from sendData, actual: "<<actual <<" toSend: "<<toSend<<std::endl;
    //         std::cout<<"curr time in nana: "<< Simulator::Now()<<std::endl;
    //         break;
    //     }
    // }
    // // Check if time to close (all sent)
    // if (m_totBytes == m_maxBytes && m_connected) {
    //     m_send_socket->Close();
    //     m_connected = false;
    // }
    m_maxBytes = m_fifo_transmission_queue.front()->GetSize();
    
    while (m_fifo_transmission_queue.empty() == false) { // Time to send more
        // uint64_t to allow the comparison later.
        // the result is in a uint32_t range anyway, because
        // m_sendSize is uint32_t.
        uint64_t toSend = 100000;
        // Make sure we don't send too many
        if (m_maxBytes > 0) {
            toSend = std::min(toSend, m_maxBytes - m_totBytes);
        }

        NS_LOG_LOGIC("sending packet at " << Simulator::Now());
        uint64_t txbuffersize = m_send_socket->GetObject<TcpSocketBase>()->GetTxBuffer()->Size();
        std::cout <<"tosend: "<<toSend<<" txbuffer: "<<txbuffersize << std::endl;
        Ptr <Packet> packet = Create<Packet>(toSend);
        int actual = m_send_socket->Send(packet);
        if (actual > 0) {
            m_totBytes += actual;
            m_txTrace(packet);
        }
        // We exit this loop when actual < toSend as the send side
        // buffer is full. The "DataSent" callback will pop when
        // some buffer space has freed up.
        if ((unsigned) actual != toSend) {
            std::cout<<"Break from sendData, actual: "<<actual <<" toSend: "<<toSend<<std::endl;
            std::cout<<"curr time in nana: "<< Simulator::Now()<<std::endl;
            break;
        }
    }
    // Check if time to close (all sent)
    if (m_totBytes == m_maxBytes && m_connected) {
        m_send_socket->Close();
        m_connected = false;
    }

}

void HorovodWorker::BackPropagationStart(uint32_t layer_idx){
    NS_LOG_FUNCTION(this);

    if (layer_idx ==0 || (layer_idx > 0 && m_backprop_layer_compute_finished[layer_idx-1]))
    {
        NS_LOG_LOGIC("HorovodWorker BackPropagation started");
        int64_t curr_timestamp = Simulator::Now().GetNanoSeconds();
        int64_t compute_time_ms = m_backprop_layer_computation_time_ms[layer_idx];
        m_timeline["Start_BP"].push_back(curr_timestamp);
        std::cout <<"Start_BP "<<m_timeline["Start_BP"].back()<<"\n";
        std::cout << "compute_time_ms: "<<compute_time_ms<<"\n";
        m_bp_compute = Simulator::Schedule(NanoSeconds(compute_time_ms * 1000000), &HorovodWorker::BackPropagationDone, this, layer_idx);        
        
    }
}

void HorovodWorker::BackPropagationDone(uint32_t layer_idx){
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_bp_compute.IsExpired());
    std::cout<<"Done_BP "<<Simulator::Now().GetNanoSeconds() <<"\n";
    if(layer_idx > 0){
        m_backprop_layer_compute_finished[layer_idx-1] = false; 
    }

    if(layer_idx != m_num_layers -1){
        
        BackPropagationStart(layer_idx+1);
        // Simulator::Schedule(MilliSeconds(0), &HorovodWorker::BackPropagationStart, this, layer_idx+1);
    }
    
    SendGradients(layer_idx);
    // Simulator::Schedule(MilliSeconds(0), &HorovodWorker::SendGradients, this, layer_idx);

    //ToDo ringallreduce
    // uint32_t ringallreduce_idx = m_ringallreduce_layer_map[layer_idx];
    // m_ringallreduce_populate[ringallreduce_idx][layer_idx] = true;
    // if(m_ringallreduce_ready(ringallreduce_idx)){
    //     m_send_queue.push_back(m_ringallreduce[ringallreduce_idx]);
    // }

}

void HorovodWorker::SendGradients(uint32_t layer_idx){
    m_timeline["Start_TX_Grad"].push_back(Simulator::Now().GetNanoSeconds());
    std::cout <<"Start_TX_Grad "<< m_timeline["Start_TX_Grad"].back()<<"\n";
    m_tx_layer_idx = layer_idx;
    // Todo handle multiple concurrent send requesets multiple m_tx_layer_idx
    // Ptr <Packet> packet = Create<Packet>(m_layer_size_bytes[layer_idx]);
    RingAllReduce ringallreduce(m_layer_size_bytes[layer_idx]);
    m_fifo_transmission_queue.push(ringallreduce);

    SendData(m_send_socket, 0);
}

void HorovodWorker::ConnectionSucceeded(Ptr <Socket> socket) {
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_LOGIC("HorovodWorker Connection succeeded");
    printf("HorovodWorker Connection succeeded\n");

    m_connected = true;
    // SendData(m_send_socket, 0);

    BackPropagationStart(0);
    // if(m_backprop_layer_compute_finished[0] == false)
    // {
    //     // start first layer of backprop
    //     BackPropagationStart(0);
    // }
}

void HorovodWorker::ConnectionFailed(Ptr <Socket> socket) {
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_LOGIC("HorovodWorker, Connection Failed");
    printf("HorovodWorker Connection failed\n");

    m_connFailed = true;
    m_closedByError = false;
    m_closedNormally = false;
    m_ackedBytes = 0;
    m_isCompleted = false;
    m_send_socket = 0;
}

int64_t HorovodWorker::GetAckedBytes() {
    if (m_closedNormally || m_closedByError) {
        return m_ackedBytes;
    } else {
        return m_totBytes - m_send_socket->GetObject<TcpSocketBase>()->GetTxBuffer()->Size();
    }
}

int64_t HorovodWorker::GetCompletionTimeNs() {
    return m_completionTimeNs;
}

bool HorovodWorker::IsCompleted() {
    return m_isCompleted;
}

bool HorovodWorker::IsConnFailed() {
    return m_connFailed;
}

bool HorovodWorker::IsClosedNormally() {
    return m_closedNormally;
}

bool HorovodWorker::IsClosedByError() {
    return m_closedByError;
}

void HorovodWorker::SocketClosedNormal(Ptr <Socket> socket) {
    m_completionTimeNs = Simulator::Now().GetNanoSeconds();
    m_connFailed = false;
    m_closedByError = false;
    m_closedNormally = true;
    if (m_send_socket->GetObject<TcpSocketBase>()->GetTxBuffer()->Size() != 0) {
        throw std::runtime_error("Socket closed normally but send buffer is not empty");
    }
    m_ackedBytes = m_totBytes - m_send_socket->GetObject<TcpSocketBase>()->GetTxBuffer()->Size();
    m_isCompleted = m_ackedBytes == m_maxBytes;
    m_send_socket = 0;
}

void HorovodWorker::SocketClosedError(Ptr <Socket> socket) {
    m_connFailed = false;
    m_closedByError = true;
    m_closedNormally = false;
    m_ackedBytes = m_totBytes - m_send_socket->GetObject<TcpSocketBase>()->GetTxBuffer()->Size();
    m_isCompleted = false;
    m_send_socket = 0;
}


} // Namespace ns3
