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
 * Author: Hanjing
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
#include "ringallreduce-syncer.h"

#include <fstream>

#include "ns3/address-utils.h"
// #define DEBUG
#ifdef DEBUG
#define DEBUG_MSG(str) do {WORKER; std::cout << str << std::endl; } while( false )
#else
#define DEBUG_MSG(str) do { } while ( false )
#endif


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
            .AddAttribute("Remote", "The address of the destination",
                          AddressValue(),
                          MakeAddressAccessor(&HorovodWorker::m_peer),
                          MakeAddressChecker())
            .AddAttribute("Rank", "The Rank of horovodworker",
                            UintegerValue(), 
                            MakeUintegerAccessor(&HorovodWorker::m_worker_id),
                            MakeUintegerChecker<uint64_t>())
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
            .AddAttribute("Priority", "The Rank of horovodworker",
                            UintegerValue(), 
                            MakeUintegerAccessor(&HorovodWorker::m_send_priority),
                            MakeUintegerChecker<uint8_t>())
            .AddAttribute("Port", "The port of the tcp sockets",
                            UintegerValue(), 
                            MakeUintegerAccessor(&HorovodWorker::m_port),
                            MakeUintegerChecker<uint32_t>())

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

RingAllReduce::RingAllReduce()
    :   r_size_bytes(0),
        r_partition_bytes(0),
        r_priority(0){
    NS_LOG_FUNCTION(this);
}

RingAllReduce::~RingAllReduce(){
    NS_LOG_FUNCTION(this);
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
    InitializeLayerWeight();
    InitializeRingAllReduceMap();
    InitializeComputeTime();
    
    std::cout<<"Start Horovod application "<<std::endl;
    // Create the socket if not already
    if (!m_send_socket) {
        m_send_socket = Socket::CreateSocket(GetNode(), m_tid);
        WORKER;
        printf("  > created send socket \n");

        // Must be TCP basically
        if (m_send_socket->GetSocketType() != Socket::NS3_SOCK_STREAM &&
            m_send_socket->GetSocketType() != Socket::NS3_SOCK_SEQPACKET) {
            NS_FATAL_ERROR("Using FlowSend with an incompatible socket type. "
                           "Horovod requires SOCK_STREAM or SOCK_SEQPACKET. "
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
        // m_send_socket->SetIpTos(m_send_priority);
        WORKER;
        std::cout<<"m_send_socket priority after connect peer "
            <<unsigned(m_send_socket->GetPriority())
            <<" tos: "<<unsigned(m_send_priority)<<std::endl;    
        m_send_socket->ShutdownRecv();
        
        // Callbacks
        m_send_socket->SetConnectCallback(
                MakeCallback(&HorovodWorker::ConnectionSucceeded, this),
                MakeCallback(&HorovodWorker::ConnectionFailed, this)
        );
        // std::cout<<"m_send_socket priority after set connectcallback "<<unsigned(m_send_socket->GetPriority())<<std::endl;

        m_send_socket->SetSendCallback(MakeCallback(&HorovodWorker::SendData, this));
        m_send_socket->SetCloseCallbacks(
                MakeCallback(&HorovodWorker::SocketClosedNormal, this),
                MakeCallback(&HorovodWorker::SocketClosedError, this)
        );

        m_send_socket->SetDataSentCallback(MakeCallback(&HorovodWorker::NotifyDataSent, this));

    }

    // Create a socket which is always in LISTEN state
    // As soon as it processes a SYN in ProcessListen(),
    // it forks itself into a new socket, which
    // keeps the accept and close callbacks
    if (!m_receive_socket) {
        m_receive_socket = Socket::CreateSocket(GetNode(), m_tid);
        printf("  > created receive socket\n");
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

    //create log file
    std::ofstream ofs;
    
    std::cout<<"file priority: "<<unsigned(m_send_socket->GetPriority()) <<std::endl;
    
    // ofs.open(m_baseLogsDir + "/" + format_string("HorovodWorker_%" PRIu32 "_layer_%" PRIu32 "_prio_%u_progress.txt", m_worker_id, m_num_layers, (unsigned)(m_send_socket->GetPriority())));
    ofs.open(m_baseLogsDir + "/" + format_string("HorovodWorker_%" PRIu32 "_layer_%" PRIu32 "_port_%u_progress.txt", m_worker_id, m_num_layers, m_port));
    ofs << "Iteration_idx" << "," << "Layer_idx" << "," <<  "Event" << ","<<"Time"<< std::endl;

    ofs.close();
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
    // WORKER;
    // Immediately from the socket drain all the packets it has received
    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from))) {
        if (packet->GetSize() == 0) { // EOFs
            DEBUG_MSG("Received 0 length packets");
            break;
        }
        
        m_totalRx += packet->GetSize ();
        // std::cout<<"  > Received up to "<< m_totalRx<<"\n";
        DEBUG_MSG("  > Received up to "<< m_totalRx);
        // std::cout<<"  > Curr time: "<< Simulator::Now()<<std::endl;
        std::map<uint32_t, FusionPartition*> map = m_leftneighbor->GetInflightFusions();
        DEBUG_MSG("leftneighbor bytes sent vector lastest: "<<m_leftneighbor->GetBytesSentVector().back());
        for( std::vector<uint32_t>::reverse_iterator it = m_leftneighbor->GetBytesSentVector().rbegin(); it != m_leftneighbor->GetBytesSentVector().rend(); ++it ){
            // Debug(format_string("left neighbor byte sent vector: %u", *it));

            if (m_totalRx >= *it) {
                // Debug(format_string("m_totalRx: %u >= *it", m_totalRx));
                DEBUG_MSG("m_totalRx:"<<m_totalRx<<" >= *it "<<*it);
                // Debug(format_string("received vector empty %u", m_bytes_received_vector.empty()));
                // std::cout<<"received vector empty"<< m_bytes_received_vector.empty()<<std::endl;
                if((m_bytes_received_vector.empty()) || (*it != m_bytes_received_vector.back())){
                    // found fusion, push it back to received vector
                    m_bytes_received_vector.push_back(*it);
                    // matches whats being sent by the neighbor

                    // Debug("Found fusion");
                    // std::cout<<"    > FOUND FUSION : "<<map[*it]<<std::endl;
                    // std::cout<<"    > Received FUSION"<<std::endl;
                    // std::cout<<"    > Received partition from R["<<map[*it]->GetParent()->GetPriority()
                                                            // <<"] : "<<map[*it]->GetIdx() <<std::endl;
                    // FusionPartition* fusionpartition= map[m_totalRx];
                    DEBUG_MSG("    > FOUND FUSION : "<<map[*it]);
                    uint32_t partition_idx = map[*it]->GetIdx();

                    for(auto layer: map[*it]->GetParent()->GetTensors()){
                        RecordEvent(layer, format_string("Received_Partition_%" PRIu32 "_Priority_%" PRIu64, partition_idx, uint64_t(m_send_socket->GetPriority())));
                    }

                    // std::cout<<"    > Received partition progress "<<map[*it]->GetProgress()<<std::endl; 
                    if (map[*it]->GetProgress() < (2 * (m_num_workers-1) - 1)) // not yet fully synced
                    {
                        // Add next fusion to send to queue
                        // increment progress by one
                        // std::cout<<"    > Not fully synced, send Partition "<<partition_idx<<" to neighbor"<< std::endl;
                        DEBUG_MSG("    > Not fully synced, send Partition "<<partition_idx<<" to neighbor");
                        uint32_t new_progress = map[*it]->GetProgress() +1;
                        m_inflight_allreduce->GetPartitions()[partition_idx]->UpdateProgress(new_progress);
                        // send to right neighbor, add to transmission queue
                        m_maxBytes += map[*it]->GetSize();
                        m_bytes_sent += map[*it]->GetSize();
                        m_inflight_fusion_map[m_bytes_sent] = m_inflight_allreduce->GetPartitions()[partition_idx];
                        m_bytes_sent_vector.push_back(m_bytes_sent);   

                        // for(auto layer: m_inflight_allreduce->GetTensors()){
                        //     // RecordEvent(layer, format_string("Start_Sending_Partition_%" PRIu32 "_Priority_%" PRIu64, partition_idx, uint64_t(m_send_socket->GetPriority())));
                        //     DEBUG_MSG("Start_Sending_Partition_"<<partition_idx<<"_Priority_"<< uint64_t(m_send_socket->GetPriority()) << " layer: "<< layer);
                        // }

                    }
                    //current partition has been circulated among workers twice, no need to send it to neighbors
                    else{

                        // clear send buffer
                        // Check if all paritions have truly been synced and if other workers have done, otherwise                         
                        if(CheckAllPartitionSynced(partition_idx)){
                            // std::cout<<"    > all local partitions are synced "<<std::endl;
                            DEBUG_MSG("    > all local partitions are synced for Piro"<< m_inflight_allreduce->GetPriority());
                            DEBUG_MSG("    > Update Global ");
                            UpdateGlobalRingallreduceSyncer();
 
                            if(CheckAllWorkersSynced()){
                                // std::cout<<"    > RingAllreduce done for: "<<m_inflight_allreduce->GetPriority()<<"\n";
                                // std::cout<<"    > All workers are synced notify other workers" <<std::endl;
                                DEBUG_MSG("    > RingAllreduce done for: "<<m_inflight_allreduce->GetPriority());
                                DEBUG_MSG("    > All workers are synced notify other workers");
                                NotifyAllOtherWorkers();
                            }
                            else{
                                //  Not all workers have finished syncing
                                // std::cout <<"     > Not all workers have finished syncing" <<std::endl;
                                DEBUG_MSG("     > Not all workers have finished syncing");
                            }
                        }
                        else{
                            // std::cout <<"      > Not all partitions are synced"<<std::endl;
                            DEBUG_MSG("      > Not all partitions are synced");
                            // std::cout<<"      > Not ready to remove inflight ringallreduce yet"<<std::endl;
                        }
                    }
                    SendData(m_send_socket, 0);
                }
                break;
            }
        }         
    }
}

void HorovodWorker::DequeTransmission(){
    if(m_qdisc == FIFOQDISC){
        m_fifo_transmission_queue.pop();
    }
    if(m_qdisc == PERFECTPRIORITY){
        // std::cout<<"Goint to pop from transmission queue: "<<m_perfectpriority_queue.top()->GetPriority()<<std::endl;
        std::priority_queue<RingAllReduce*, std::vector<RingAllReduce*>, ComparePriority> temp = m_perfectpriority_queue;
        while(!temp.empty()){
            // std::cout<<" temp priority queue: priority "<< temp.top()->GetPriority() << std::endl;
            temp.pop();
        }
        m_perfectpriority_queue.pop();
    }
}

bool HorovodWorker::ReceivedAllGradients(){
    for(auto i = m_gradients_received.begin(); i != m_gradients_received.end(); ++i){
        if(i->second == false){
            // std::cout<<"    > ReceivedAllGradients falsed at layer "<< i->first <<std::endl;    
            return false;
        }
    }
    return true;
}

void HorovodWorker::UpdateReceivedGradients(uint32_t ringallreduce_prioritiy){
    
    for(auto layer_idx: m_ringallreduce_map[ringallreduce_prioritiy]->GetTensors()){
        m_gradients_received[layer_idx] = true;
        // std::cout <<"  > udpate grad received "<<layer_idx << std::endl;
        // std::cout <<"  > update grad received "<<layer_idx << std::endl;
        if(ITERBARRIER && (m_qdisc != PERFECTPRIORITY)){
            if(ReceivedAllGradients())
            {
                // std::cout <<"   > Iteration Barrier Enabled, all gradients have been received"<<std::endl;
                ForwardPropagationStart(0);
            }
            // else{
            //     std::cout <<"   > Iteration Barrier Enabled, haven't received all gradients"<<std::endl;
            // }
        }
        else{
           ForwardPropagationStart(layer_idx);
        }
    }
    return;
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

void HorovodWorker::SetLeftNeighbor(Ptr<HorovodWorker> leftneighbor){
    m_leftneighbor = leftneighbor;
}

void HorovodWorker::SetGlobalRingallreduceSyncer(GlobalRingAllReduceSyncer * global_ringallreduce_syncer){
    m_global_ringallreduce_syncer = global_ringallreduce_syncer;
}

void HorovodWorker::SendData(Ptr <Socket>, uint32_t) {
    NS_LOG_FUNCTION(this);
    if ( ((m_qdisc == FIFOQDISC) && m_fifo_transmission_queue.empty() && (m_ringallreduce_inflight_status != true) ) || ((m_qdisc == PERFECTPRIORITY) && m_perfectpriority_queue.empty() && (m_ringallreduce_inflight_status != true))){
        // std::cout<<"  > fifo or priority transmission queue empty \n";
        DEBUG_MSG("  > fifo or priority transmission queue empty");
        // sendData otherwise is always called right after connection is established
        return;
    }

    if(m_ringallreduce_inflight_status == false){
        m_inflight_allreduce = QueuePeek();
        // std::cout<<"  > Update inflight allreduce to "<<m_inflight_allreduce->GetPriority()<<std::endl;
        uint32_t prio = m_inflight_allreduce->GetPriority();
        // std::cout <<"size of global status vector "<< m_global_ringallreduce_status->size()<<std::endl;
        if(m_global_ringallreduce_syncer->Empty()){
            m_global_ringallreduce_syncer->AddRingallreduce(prio);
            // std::cout<<" Add to global ringallreduce: "<<prio<<" progress: "<< m_global_ringallreduce_syncer->GetProgress() <<std::endl;
            DEBUG_MSG(" Add to global ringallreduce: "<<prio<<" progress: "<< m_global_ringallreduce_syncer->GetProgress() );
        }
        else if( m_global_ringallreduce_syncer->GetPriority() != prio){
            // Not allowed, all workers need to agree on one ringallreduce to work with
            std::cout<<"Not allowed, all workers need to agree on one ringallreduce to work with"<<std::endl;
            std::cout<<"Expecting global ringallreduce: "<< m_global_ringallreduce_syncer->GetPriority() <<" instead of "<< prio<< std::endl;
            // Todo! check the queue to see if desired ringallreduce exist, if it does, deque it first
        }
        else{
            std::cout<<" >Working on the same global ringallreduce: "<< prio <<std::endl;
            DEBUG_MSG(" >Working on the same global ringallreduce: ");
        }

        DequeTransmission();
        m_ringallreduce_inflight_status = true;
        
        // Push to send queue { [layer1, layer2, ], size_in_bytes, partition_id } std::vector<uint32_t>, uint32_t size, uint32_t partition id
        std::vector<uint32_t> p_layers =  m_inflight_allreduce->GetTensors();
        uint32_t p_size = m_inflight_allreduce->GetPartitionSize();
        uint32_t p_idx = m_worker_id;
        std::tuple<std::vector<uint32_t>, uint32_t, uint32_t> partition_to_send = make_tuple( p_layers, p_size, p_idx);
        m_send_queue.push(partition_to_send);
        m_maxBytes += m_inflight_allreduce->GetPartitionSize();

        m_bytes_sent += m_inflight_allreduce->GetPartitionSize();
        // m_bytes_sent += m_maxBytes;
        // std::cout<<"  > Add to fusion map key: " <<m_bytes_sent<< " Partition: " <<p_idx <<std::endl;
        DEBUG_MSG("  > Add m_bytes_sent_vector: " <<m_bytes_sent<< " Partition: " <<p_idx);
        m_bytes_sent_vector.push_back(m_bytes_sent);
        m_inflight_fusion_map[m_bytes_sent] = m_inflight_allreduce->GetPartitions()[m_worker_id];

        // ***** Scheduling scheme 1:
        // *************************** allreduce priority <= 14 : P0
        // *************************** allreduce priority > 14 : P2
        // if(m_inflight_allreduce->GetPriority() <= 14){
        //     // set priority to high, band 0
        //     m_send_socket->SetIpTos(0x10);
        //     std::cout<<"Change priority to 0x10, expect priority to be 6 "<<unsigned(m_send_socket->GetPriority())<<std::endl;
        // }
        // else{
        //     m_send_socket->SetIpTos(0x08);
        //     std::cout<<"Change priority to 0x08, expect priority to be 2 "<<unsigned(m_send_socket->GetPriority())<<std::endl;

        // }

        for(auto layer: m_inflight_allreduce->GetTensors()){
            RecordEvent(layer, format_string("Start_Sending_Partition_%" PRIu32 "_Priority_%" PRIu64, m_worker_id, uint64_t(m_send_socket->GetPriority())));
            DEBUG_MSG("Start_Sending_Partition_"<<m_worker_id<<"_Priority_"<< uint64_t(m_send_socket->GetPriority()) << " layer: "<< layer);
        }
    }

    // if(m_maxBytes == 0 && m_ringallreduce_inflight_status && (m_send_socket->GetObject<TcpSocketBase>()->GetTxBuffer()->Size() == 0)){
    //     if(!m_send_queue.empty()){
    //         std::tuple<std::vector<uint32_t>, uint32_t, uint32_t> p_to_send =  m_send_queue.front();
    //         m_maxBytes = std::get<1>(p_to_send);
    //         uint32_t idx = std::get<2>(p_to_send);
    //         m_send_queue.pop();
    //         m_bytes_sent += m_maxBytes;
    //         m_inflight_fusion_map[m_bytes_sent] = m_inflight_allreduce->GetPartitions()[idx];
    //         std::cout<<"  > Add to fusion map key: " <<m_bytes_sent<< " Partition: " <<idx <<std::endl;
    //         for(auto layer: m_inflight_allreduce->GetTensors()){
    //             RecordEvent(layer, format_string("Start_Sending_Partition_%" PRIu32, idx));
    //         }
    //     }
    // }
    

    while (m_maxBytes) { // Time to send more
        // Make sure we don't send too many
        uint64_t toSend = 100000;
        toSend = std::min(m_maxBytes, toSend);

        NS_LOG_LOGIC("  > sending packet at " << Simulator::Now());
        // uint64_t txbuffersize = m_send_socket->GetObject<TcpSocketBase>()->GetTxBuffer()->Size();
        // std::cout <<"  > tosend: "<<toSend<<" txbuffer: "<<txbuffersize << std::endl;
        Ptr <Packet> packet = Create<Packet>(toSend);
        int actual = m_send_socket->Send(packet);
        if (actual > 0) {
            // m_totBytes += actual;
            // std::cout<<" Actual bytes sent: "<<actual<<std::endl;
            m_maxBytes -= actual;
            m_txTrace(packet);
        }
        // We exit this loop when actual == -1 as the send side
        // buffer is full. The "DataSent" callback will pop when
        // some buffer space has freed up.
        if ( actual == -1) {
            std::cout<<"  > Break from sendData, actual: "<<actual <<" toSend: "<<toSend<<std::endl;
            std::cout<<"  > curr time in nana: "<< Simulator::Now()<<std::endl;
            break;
        }
    }
    if (m_maxBytes == 0){
        DEBUG_MSG("Sent everything, m_maxBytes is zero, total bytes sent "<<m_bytes_sent);
        
    }
}

void HorovodWorker::BackPropagationStart(uint32_t layer_idx){
    NS_LOG_FUNCTION(this);
    NS_LOG_LOGIC("HorovodWorker BackPropagation started");
    int64_t curr_timestamp = Simulator::Now().GetNanoSeconds();
    float compute_time_ms = m_bp_layer_compute_time_ms[layer_idx];
    m_timeline["Start_BP"].push_back(curr_timestamp);
    RecordEvent(layer_idx, "BP_Start");
    // std::cout <<"  > Start_BP "<<m_timeline["Start_BP"].back()<<"\n";
    // std::cout <<"    > compute_time_ms: "<<compute_time_ms<<"\n";
    uint32_t compute_time_ns = uint32_t(compute_time_ms * 1000000.0);
    // std::cout<<"  > Schedule BP["<<layer_idx<<"] to finish in "<< compute_time_ns<<" ns"<<std::endl;

    // m_bp_compute = Simulator::Schedule(NanoSeconds(compute_time_ms * 1000000), &HorovodWorker::BackPropagationDone, this, layer_idx);        
    Simulator::Schedule(NanoSeconds(compute_time_ns), &HorovodWorker::BackPropagationDone, this, layer_idx);        
}

void HorovodWorker::ForwardPropagationStart(uint32_t layer_idx){
    // std::cout<<"  > Trying to start FP["<<layer_idx<<"]"<<std::endl;
    if(m_gradients_received[layer_idx] == false){
        // std::cout<<" > Have not received gradients for FP["<<layer_idx<<"]"<<std::endl;
        DEBUG_MSG(" > Have not received gradients for FP["<<layer_idx<<"]");
        return;
    }

    if( (layer_idx == 0) || ( (layer_idx > 0) && (m_fp_finished_status[layer_idx-1] == true)))
    {
        float compute_time_ms = m_fp_layer_compute_time_ms[layer_idx];        
        // std::cout<<"  > Schedule FP["<<layer_idx<<"] to finish in "<< compute_time_ms<<" ms"<<std::endl;
        RecordEvent(layer_idx, "FP_Start");
        uint32_t compute_time_ns = uint32_t(compute_time_ms * 1000000.0);
        // std::cout<<"  > Schedule FP["<<layer_idx<<"] to finish in "<< compute_time_ns<<" ns"<<std::endl;
        DEBUG_MSG("  > Schedule FP["<<layer_idx<<"] to finish in ");
        Simulator::Schedule(NanoSeconds(compute_time_ns), &HorovodWorker::ForwardPropagationDone, this, layer_idx);
    }

    // else{
    //     std::cout<<" > Previous FP["<<layer_idx-1<<"] not done yet"<<std::endl;
    // }

    return;
}

void HorovodWorker::ForwardPropagationDone(uint32_t layer_idx){
    // Reset status for future iterations
    m_fp_finished_status[layer_idx - 1] = false;
    m_fp_finished_status[layer_idx] = true;
    m_gradients_received[layer_idx] = false;
    RecordEvent(layer_idx, "FP_Done");
    // std::cout<<"  > ForwardProp Done "<<layer_idx<<std::endl;
    if(layer_idx == m_num_layers-1){
        /********** Enforce a max iteration limit
            if(m_iteration_idx + 1 == m_max_iteration){
                std::cout<<"    > Reached max iteration"<<std::endl;
                std::cout<<"    > time: "<< Simulator::Now() <<std::endl;
                return;
            }
            else
            {
                m_iteration_idx += 1;

                Debug(format_string("FP Done, start BP for iter: " PRIu32, m_iteration_idx));
                std::cout<<"    > FP Done, start BP for iter: "<< m_iteration_idx<<std::endl;
                BackPropagationStart(m_num_layers-1);
            }
        ****/
        m_iteration_idx += 1;

        // Debug(format_string("FP Done, start BP for iter: " PRIu32, m_iteration_idx));
        // std::cout<<"    > FP Done, start BP for iter: "<< m_iteration_idx<<std::endl;
        BackPropagationStart(m_num_layers-1);


    }
    else{
            ForwardPropagationStart(layer_idx+1);
    }
}

void RingAllReduce::AddTensor(uint32_t layer_idx, uint32_t size){
    r_tensors.push_back(layer_idx);
    r_size_bytes += size;
}

uint32_t RingAllReduce::GetSize(){
    return r_size_bytes;
}

void RingAllReduce::SetPriority(){
    if(r_tensors.size() > 0){
        r_priority = r_tensors.back();
    }
    else{
        NS_LOG_ERROR("Ringallreduce contains no tensors");
    }
    return;
}
uint32_t RingAllReduce::GetPriority(){
    return r_priority;
}

void RingAllReduce::SetPartition(uint32_t num_workers, RingAllReduce* parent){
    r_partition_bytes = uint32_t(r_size_bytes/num_workers);
    for(uint32_t i =0; i < num_workers; ++i){
        r_partitions[i] = new FusionPartition();
        r_partitions[i]->SetSize(r_partition_bytes);
        r_partitions[i]->SetIdx(i);
        r_partitions[i]->SetParent(parent);
        std::cout<<"    > Initialize Parition for R["<<r_partitions[i]->GetParent()->GetPriority()<<"]"<<" : "
                                                <<","<<" idx: "<<r_partitions[i]->GetIdx()<<std::endl;
    }
    std::cout <<"   > Partition size: "<<r_partition_bytes<<"\n";
}

void HorovodWorker::SetFusionBufferSize(uint32_t size){
    m_fused_tensor_size_bytes =size;
}

void HorovodWorker::InitializeLayerWeight(){
    uint32_t model_size_bytes = 100 * 1000000;
    uint32_t min_layer_size_bytes = uint32_t(2 * model_size_bytes / (9 * m_num_layers));
    std::cout << "Min layer size " << min_layer_size_bytes<<std::endl;
    for(uint32_t i=0; i< m_num_layers; ++i){
        if(i < uint32_t(m_num_layers/2)){
            std::cout <<"i < layer/2 "<< uint32_t(m_num_layers/2) << std::endl;
            m_layer_size_bytes[i] = min_layer_size_bytes;
            std::cout<<"layer "<<i <<" size: "<<m_layer_size_bytes[i]<<std::endl;
        }
        else if( (i >= uint32_t(m_num_layers/2)) && (i < uint32_t(3* m_num_layers/4))){
            m_layer_size_bytes[i] = 4* min_layer_size_bytes;
            std::cout<<"layer "<<i <<" size: "<<m_layer_size_bytes[i]<<std::endl;
        }
        else {
            m_layer_size_bytes[i] = 12 * min_layer_size_bytes;
            std::cout<<"layer "<<i <<" size: "<<m_layer_size_bytes[i]<<std::endl;
        }
    }
    // set fusionbuffer size to be slightly larger than the maximum layer size
    SetFusionBufferSize(m_layer_size_bytes[m_num_layers-1]+1);
}

void HorovodWorker::InitializeComputeTime(){
    float compute_time_per_iteration_ms = 900;
    float fp_total_time_ms = (float(1)/float(3)) * compute_time_per_iteration_ms;
    std::cout<<"fp_total_time_ms "<<fp_total_time_ms<<std::endl;
    float bp_total_time_ms = (2.0/3.0) * compute_time_per_iteration_ms;
    std::cout<<"bp_total_time_ms "<<bp_total_time_ms<<std::endl;
    //  To simplify computation time in FP: assum each layer takes less then d ms to compute than previous layer and the last layer takes 0 ms
    float fp_diff_per_layer_ms = 2.0 * fp_total_time_ms / (float(m_num_layers) * (float(m_num_layers)-1.0)); // (self.config.num_layers * (self.config.num_layers-1))
    std::cout<<"fp_diff_per_layer_ms "<<fp_diff_per_layer_ms<<std::endl;
    float fp_first_layer_ms = 2.0 * fp_total_time_ms / float(m_num_layers); // self.config.num_layers
    // Same simplification applies to BP except its in ascending order
    float bp_diff_per_layer_ms = 2.0 * bp_total_time_ms / (float(m_num_layers) * (float(m_num_layers)-1.0)); // (self.config.num_layers * (self.config.num_layers-1))
    std::cout<<"bp_diff_per_layer_ms "<<bp_diff_per_layer_ms<<std::endl;
    for(uint32_t i =0; i< m_num_layers; ++i){
    m_fp_layer_compute_time_ms[i] = fp_first_layer_ms - i * fp_diff_per_layer_ms;
    m_fp_layer_compute_time_ms[m_num_layers-1] = fp_diff_per_layer_ms;
    std::cout <<"fp layer compute time "<< i<<" : " <<m_fp_layer_compute_time_ms[i]<<std::endl;
    m_bp_layer_compute_time_ms[i] = i * bp_diff_per_layer_ms;
    m_bp_layer_compute_time_ms[0] = bp_diff_per_layer_ms;
    std::cout <<"bp layer compute time "<< i<<" : " <<m_bp_layer_compute_time_ms[i]<<std::endl;
    }

}


void HorovodWorker::InitializeRingAllReduceMap(){
    WORKER;
    RingAllReduce *ringallreduce = new RingAllReduce();
    // Todo Optimize 
    for(int i= m_layer_size_bytes.size()-1; i > -1 ; --i){
        if(ringallreduce->GetSize() + m_layer_size_bytes[i] <= m_fused_tensor_size_bytes){
            std::cout<<"  > add tensor: "<<i <<" size: "<<m_layer_size_bytes[i]<<"\n";
            ringallreduce->AddTensor(i, m_layer_size_bytes[i]);
        }

        else if(ringallreduce->GetSize() + m_layer_size_bytes[i] > m_fused_tensor_size_bytes  ){
            // Edge case: a single tensor is too large to fit in user defined fusion buffer
            if(ringallreduce->GetSize() > 0 ){
                ringallreduce->SetPriority();
                ringallreduce->SortTensor();            
                uint32_t pri = ringallreduce->GetPriority();
                ringallreduce->SetPartition(m_num_workers, ringallreduce);
                
                std::cout<<"  > set priority: "<<pri<<"\n";
                m_ringallreduce_map[pri] = ringallreduce;
            }
            ringallreduce = new RingAllReduce();
            std::cout<<"  > add tensor: "<<i <<" size: "<<m_layer_size_bytes[i]<<"\n";
            ringallreduce->AddTensor(i, m_layer_size_bytes[i]);  
        }
    }

    ringallreduce->SetPriority();
    ringallreduce->SortTensor();
    uint32_t pri = ringallreduce->GetPriority();
    ringallreduce->SetPartition(m_num_workers, ringallreduce);
    std::cout<<"  > set priority: "<<pri<<"\n";
    m_ringallreduce_map[pri] = ringallreduce;

 }

void HorovodWorker::EnqueTransmission(RingAllReduce* ringallreduce){
    if(m_qdisc == FIFOQDISC){
        m_fifo_transmission_queue.push(ringallreduce);
    }
    if (m_qdisc == PERFECTPRIORITY){
        m_perfectpriority_queue.push(ringallreduce);
    }
}

RingAllReduce* HorovodWorker::QueuePeek(){
    if(m_qdisc == FIFOQDISC){
        return m_fifo_transmission_queue.front();
    }
    if(m_qdisc == PERFECTPRIORITY){
        return m_perfectpriority_queue.top();
    }
    else{
        NS_LOG_ERROR("QDISC NOT SET CORRECTLY");
        return m_perfectpriority_queue.top();
    }
}

void HorovodWorker::StartRingAllReduce(uint32_t layer_idx){
    if(m_ringallreduce_map.find(layer_idx) != m_ringallreduce_map.end())
    {
        DEBUG_MSG("  > add to fifo queue a new ringallreduce of priority "<< layer_idx);
        //Todo: send updates to worker 0 and worker 0 will broadcast to all workers when its ready to start ringallreduce
        EnqueTransmission(m_ringallreduce_map[layer_idx]);
        SendData(m_send_socket, 0);
    }
    
    // otherwise ringallreduce that contains that layer has unfinished computation
    return;
}

void HorovodWorker::BackPropagationDone(uint32_t layer_idx){
    NS_LOG_FUNCTION(this);
    // NS_ASSERT(m_bp_compute.IsExpired());
    // std::cout<<"  > Done_BP["<< layer_idx<<"]: "<<Simulator::Now().GetNanoSeconds() <<"\n";
    DEBUG_MSG("  > Done_BP["<< layer_idx<<"]: "<<Simulator::Now().GetNanoSeconds() );
    RecordEvent(layer_idx, "BP_Done");
    StartRingAllReduce(layer_idx);

    if(layer_idx != 0){
        BackPropagationStart(layer_idx-1);
    }
}

  void HorovodWorker::UpdateGlobalRingallreduceSyncer(){
    m_global_ringallreduce_syncer->AddSyncedWorker(m_worker_id, [&]() {
        uint32_t prio =this->m_inflight_allreduce->GetPriority(); 
        // std::cout<< "Worker " <<this->GetWorkerID()
        //         <<": RingAllreduce done for: "
        //         <<prio<<"\n";
        this->m_inflight_allreduce->ResetPartitionsProgress();
        this->SetInflightRingallreduceStatus(false);
        this->UpdateReceivedGradients(prio);
        this->SendData(this->m_send_socket, 0);
        
        return;
    });
    // m_global_ringallreduce_syncer->AddSyncedWorker(m_worker_id);
  }

  
  bool HorovodWorker::CheckAllWorkersSynced(){
    return m_global_ringallreduce_syncer->AllWorkersSynced(m_num_workers);
  }

  void HorovodWorker::NotifyAllOtherWorkers(){
    m_global_ringallreduce_syncer->NotifyAllWorkersRingallreduceDone();
  }


void HorovodWorker::ConnectionSucceeded(Ptr <Socket> socket) {
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_LOGIC("HorovodWorker Connection succeeded");
    WORKER;
    printf("  > HorovodWorker Connection succeeded\n");
    m_connected = true;

    BackPropagationStart(m_num_layers-1);

}

void HorovodWorker::ConnectionFailed(Ptr <Socket> socket) {
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_LOGIC("HorovodWorker, Connection Failed");
    WORKER;
    printf("HorovodWorker Connection failed\n");

    m_connFailed = true;
    m_closedByError = false;
    m_closedNormally = false;
    m_ackedBytes = 0;
    m_isCompleted = false;
    m_send_socket = 0;
}

void HorovodWorker::RecordEvent(uint32_t layer_idx, std::string event){
    std::ofstream ofs;
    // ofs.open(m_baseLogsDir + "/" + format_string("HorovodWorker_%" PRIu32 "_layer_%" PRIu32 "_prio_%" PRIu32 "_progress.txt", m_worker_id, m_num_layers, unsigned(m_send_socket->GetPriority())),
    ofs.open(m_baseLogsDir + "/" + format_string("HorovodWorker_%" PRIu32 "_layer_%" PRIu32 "_port_%u_progress.txt", m_worker_id, m_num_layers, m_port),
    std::ofstream::out | std::ofstream::app);
    ofs << m_iteration_idx << "," << layer_idx << "," <<  event << ","<<Simulator::Now().GetNanoSeconds()<< std::endl;
    ofs.close(); 
}

void HorovodWorker::Debug(std::string event){
    std::ofstream ofs;
    // ofs.open(m_baseLogsDir + "/" + format_string("HorovodWorker_%" PRIu32 "_layer_%" PRIu32 "_prio_%" PRIu32 "_debug.txt", m_worker_id, m_num_layers, unsigned(m_send_socket->GetPriority())),
    // std::ofstream::out | std::ofstream::app);
    ofs.open(m_baseLogsDir + "/" + format_string("HorovodWorker_%" PRIu32 "_layer_%" PRIu32 "_port_%u_debug.txt", m_worker_id, m_num_layers, m_port),
    std::ofstream::out | std::ofstream::app);

    ofs << event << ","<<Simulator::Now().GetNanoSeconds()<< std::endl;
    ofs.close(); 
}

void HorovodWorker::DebugAll(std::string event){
    std::ofstream ofs;
    // ofs.open (m_baseLogsDir + "/" + format_string("HorovodWorker_%" PRIu32 "_layer_%" PRIu32 "_progress.txt", m_worker_id, m_num_layers), std::ofstream::out | std::ofstream::app);
    ofs.open(m_baseLogsDir + "/" + format_string("HorovodWorker_layer_%" PRIu32 "_debug.txt", m_num_layers),
    std::ofstream::out | std::ofstream::app);
    ofs << event << ","<<Simulator::Now().GetNanoSeconds()<< std::endl;
    ofs.close(); 
}

void HorovodWorker::NotifyDataSent(Ptr<Socket>, uint32_t datasent){
    // Notify the applications that bytes being flushed from transport layer buffer 
    m_notify_datasent += datasent;
    DEBUG_MSG("m_notify_datasent: "<<m_notify_datasent <<"m_bytes_sent: "<<m_bytes_sent);
    return;
}

//******************************************** Functions below currently unused *****************************************************//
// int64_t HorovodWorker::GetAckedBytes() {
//     if (m_closedNormally || m_closedByError) {
//         return m_ackedBytes;
//     } else {
//         return m_totBytes - m_send_socket->GetObject<TcpSocketBase>()->GetTxBuffer()->Size();
//     }
// }

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
    DEBUG_MSG("SOCKET CLOSED NORMAL");
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
    DEBUG_MSG("SocketClosedError");
    std::cout<<"Worker ID: " << HorovodWorker::GetWorkerID()<<"SocketClosedError"<<std::endl;

    m_connFailed = false;
    m_closedByError = true;
    m_closedNormally = false;
    m_ackedBytes = m_totBytes - m_send_socket->GetObject<TcpSocketBase>()->GetTxBuffer()->Size();
    m_isCompleted = false;
    m_send_socket = 0;
}

} // Namespace ns3
