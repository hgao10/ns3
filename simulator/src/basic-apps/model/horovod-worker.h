/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
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
 * Adapted from PacketSink by:
 * Author:  Tom Henderson (tomhend@u.washington.edu)
 */

#ifndef HOROVOD_WORKER_H
#define HOROVOD_WORKER_H

#include <stdint.h>

#include <algorithm>
#include <map>
#include <queue>

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/fusion-partition.h"
#include "ns3/ptr.h"
#include "ns3/socket.h"
#include "ns3/traced-callback.h"

#define WORKER \
  std::cout << "Worker ID: " << HorovodWorker::GetWorkerID() <<" Port: "<<m_port<<std::endl
#define ITERBARRIER 1

namespace ns3 {

// class Address;
// class Socket;
// class Packet;
enum QDISC {
  FIFOQDISC,
  PERFECTPRIORITY,
};

class GlobalRingAllReduceSyncer;

class RingAllReduce {
 public:
  RingAllReduce();
  uint32_t GetSize();  // return total size, overwrite base function
  virtual ~RingAllReduce();
  void SetPriority();
  void SetPartition(uint32_t num_workers, RingAllReduce *parent);
  uint32_t GetPartitionSize() { return r_partition_bytes; };
  std::map<uint32_t, FusionPartition *> GetPartitions() {
    return r_partitions;
  };
  void AddTensor(uint32_t layer_idx, uint32_t size);
  void SortTensor() { std::sort(r_tensors.begin(), r_tensors.end()); };
  uint32_t GetPriority();
  // ToDelete, progress being tracked in partitions
  uint32_t r_communication_times = 0;  // Todo implement getter/setter instead
  std::vector<uint32_t> GetTensors() { return r_tensors; };
  bool AllPartitionsUpdated(uint32_t max_version) {
    for (auto p = r_partitions.begin(); p != r_partitions.end(); p++) {
      if (p->second->GetProgress() != max_version) {
        std::cout << "Partition " << p->second->GetIdx() << " not ready"
                  << std::endl;
        return false;
      }
    }
    return true;
  };

 private:
  uint32_t r_size_bytes = 0;
  uint32_t r_partition_bytes = 0;
  uint32_t r_priority = 0;
  std::vector<uint32_t> r_tensors;
  // uint32_t r_progress = 0;
  uint32_t r_iteration = 0;
  std::map<uint32_t, FusionPartition *> r_partitions;  // key:partition index
};



class ComparePriority {
 public:
  bool operator()(RingAllReduce *r1, RingAllReduce *r2) {
    return r1->GetPriority() > r2->GetPriority();
  };
};

class HorovodWorker : public Application {
 public:
  static TypeId GetTypeId(void);
  HorovodWorker();
  virtual ~HorovodWorker();

  int64_t GetAckedBytes();  // TODO remove later if not needed
  int64_t GetCompletionTimeNs();
  bool IsCompleted();
  bool IsConnFailed();
  bool IsClosedByError();   // TODO removed later if not needed
  bool IsClosedNormally();  // TODO removed later if not needed

  void UpdateReceivedGradients(uint32_t ringallreduce_prioritiy);
  uint32_t GetWorkerID() { return m_worker_id; };
  void SetLeftNeighbor(Ptr<HorovodWorker>);
  void SetGlobalRingallreduceSyncer(GlobalRingAllReduceSyncer * global_ringallreduce_syncer);
  void SetInflightRingallreduceStatus(bool status){
    m_ringallreduce_inflight_status = status;
  };

  std::map<uint32_t, FusionPartition *> &GetInflightFusions() {
    return m_inflight_fusion_map;
  };

 protected:
  virtual void DoDispose(void);

 private:
  virtual void StartApplication(void);  // Called at time specified by Start
  virtual void StopApplication(void);   // Called at time specified by Stop

  void HandleRead(Ptr<Socket> socket);
  void HandleAccept(Ptr<Socket> socket, const Address &from);
  void HandlePeerClose(Ptr<Socket> socket);
  void HandlePeerError(Ptr<Socket> socket);
  void CleanUp(Ptr<Socket> socket);

  Ptr<Socket> m_receive_socket;         //!< Listening socket
  std::list<Ptr<Socket>> m_socketList;  //!< the accepted sockets

  Address m_local;         //!< Local address to bind to
  uint8_t m_send_priority;
  uint32_t m_port;
  TypeId m_tid;            //!< Protocol TypeId
  uint64_t m_totalRx = 0;  //!< Total bytes received

  /**
   * Send data until the L4 transmission buffer is full.
   */
  void SendData(Ptr<Socket>, uint32_t);
  void DataSent(Ptr<Socket>, uint32_t);

  Ptr<Socket> m_send_socket;  //!< Associated socket
  Address m_peer;             //!< Peer address
  bool m_connected;           //!< True if connected
                     //   uint32_t        m_sendSize;     //!< Size of data to
                     //   send each time
  uint64_t m_maxBytes;  //!< Limit total number of bytes sent
  std::queue<std::tuple<std::vector<uint32_t>, uint32_t, uint32_t>> m_send_queue; // queues of partitions to be sent (element: {layers, size, idx} )
  uint64_t m_flowId;    //!< Flow identifier
  uint64_t m_totBytes;  //!< Total bytes sent so far

  int64_t m_completionTimeNs;  //!< Completion time in nanoseconds
  bool m_connFailed;           //!< Whether the connection failed
  bool m_closedNormally;       //!< Whether the connection closed normally
  bool m_closedByError;        //!< Whether the connection closed by error

  // TODO remove if not needed
  uint64_t m_ackedBytes;  //!< Amount of acknowledged bytes cached after close
                          //!< of the socket
  bool m_isCompleted;     //!< True iff the flow is completed fully AND closed
                          //!< normally
  std::string
      m_baseLogsDir;  //!< Where the flow logs will be written to:
                      //!<   logs_dir/flow-[id]-{progress, cwnd, rtt}.txt
  TracedCallback<Ptr<const Packet>> m_txTrace;

  // horovod ML specific
  std::map<int, uint64_t> m_layer_size_bytes{
      {0, 1000000}, {1, 2000000}};  // later initialized in a function
  std::map<uint32_t, float> m_bp_layer_compute_time_ms{{0, 5}, {1, 10}};
  std::map<uint32_t, float> m_fp_layer_compute_time_ms{{0, 6}, {1, 4}};
  uint32_t m_layer_idx;
  uint32_t m_num_layers = 50;
  uint32_t m_num_workers = 3;
  EventId m_bp_compute;
  uint32_t m_tx_layer_idx;
  uint32_t m_fused_tensor_size_bytes = 2000000;
  uint32_t m_worker_id;
  std::queue<RingAllReduce *> m_fifo_transmission_queue;
  std::priority_queue<RingAllReduce *, std::vector<RingAllReduce *>,
                      ComparePriority>
      m_perfectpriority_queue;
  QDISC m_qdisc{PERFECTPRIORITY};

  // TODO <int, vector<event class>> with layer and size information
  std::map<std::string, std::vector<int64_t>>
      m_timeline;  // timeline to record invents and their stamps
  std::map<uint32_t, RingAllReduce *> m_ringallreduce_map;  // key: priority
  bool m_ringallreduce_inflight_status = false;
  RingAllReduce *m_inflight_allreduce;
  // std::deque<uint32_t>
  //     m_inflight_partition_idx;  // Use deque instead of queue to use clear()
  Ptr<HorovodWorker> m_leftneighbor;
  GlobalRingAllReduceSyncer * m_global_ringallreduce_syncer;
  std::map<uint32_t, bool> m_gradients_received{
      {0, false}, {1, false}};  // Records status for each layer on whether the
                                // tensors have been received
  std::map<uint32_t, bool> m_fp_finished_status{
      {0, false}, {1, false}};  // Records fp computation status per layer
  uint32_t m_iteration_idx = 0;
  uint32_t m_max_iteration = 1;
  std::map<uint32_t, FusionPartition *> m_inflight_fusion_map;
  uint32_t m_bytes_sent = 0;

 private:
  void ConnectionSucceeded(Ptr<Socket> socket);
  void ConnectionFailed(Ptr<Socket> socket);
  void DataSend(Ptr<Socket>, uint32_t tosend);
  void SocketClosedNormal(Ptr<Socket> socket);
  void SocketClosedError(Ptr<Socket> socket);

  // horovod ml specific
  void BackPropagationStart(uint32_t layer_idx);
  void BackPropagationDone(uint32_t layer_idx);
  void StartRingAllReduce(uint32_t layer_idx);
  void InitializeRingAllReduceMap();
  void ForwardPropagationStart(uint32_t layer_idx);
  void ForwardPropagationDone(uint32_t layer_idx);
  bool ReceivedAllGradients();
  void DequeTransmission();
  void EnqueTransmission(RingAllReduce *ringallreduce);
  RingAllReduce *QueuePeek();
  void RecordEvent(uint32_t layer_idx, std::string event);
  void Debug(std::string event);
  void DebugAll(std::string event);
  void InitializeLayerWeight();
  void InitializeComputeTime();

  void SetFusionBufferSize(uint32_t size);

  bool CheckAllPartitionSynced(uint32_t excluded_partition_idx){
    for (uint32_t i=0; i< m_num_workers; ++i){
      uint32_t progress = m_inflight_allreduce->GetPartitions()[i]->GetProgress();
      if(i == excluded_partition_idx){
        continue;
      }
      else if(i == m_worker_id){
        if(progress != m_num_workers){
          std::cout<<" Partition: "<<i
                                  <<" not fully synced, at progress "
                                  <<progress
                                  <<" , expecting progress "
                                  << m_num_workers
                                  <<std::endl;
          return false;
        }
      }
      else{
        if(progress != m_num_workers -1) {
          std::cout<<" Partition: "<<i
                                  <<" not fully synced, at progress "
                                  <<progress
                                  <<" , expecting progress "
                                  <<m_num_workers-1
                                  <<std::endl;
          return false;
        }
      }
      
    }
    return true;
  };

  void UpdateGlobalRingallreduceSyncer();
  bool CheckAllWorkersSynced();

  void NotifyAllOtherWorkers();

};  // namespace ns3
}
#endif /* FLOW_SINK_H */
