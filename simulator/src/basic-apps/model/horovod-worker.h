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
 * Author: Simon
 * Adapted from PacketSink by:
 * Author:  Tom Henderson (tomhend@u.washington.edu)
 */

#ifndef HOROVOD_WORKER_H
#define HOROVOD_WORKER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/socket.h"
#include <map>

namespace ns3 {

//class Address;
//class Socket;
//class Packet;

class HorovodWorker : public Application 
{
public:
  static TypeId GetTypeId (void);
  HorovodWorker ();
  virtual ~HorovodWorker ();

  int64_t GetAckedBytes(); // TODO remove later if not needed
  int64_t GetCompletionTimeNs();
  bool IsCompleted();
  bool IsConnFailed();
  bool IsClosedByError();  // TODO removed later if not needed
  bool IsClosedNormally(); // TODO removed later if not needed
 
protected:
  virtual void DoDispose (void);

private:
    
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop
  
  void HandleRead (Ptr<Socket> socket);
  void HandleAccept (Ptr<Socket> socket, const Address& from);
  void HandlePeerClose (Ptr<Socket> socket);
  void HandlePeerError (Ptr<Socket> socket);
  void CleanUp (Ptr<Socket> socket);

  Ptr<Socket> m_receive_socket;                 //!< Listening socket
  std::list<Ptr<Socket> > m_socketList; //!< the accepted sockets

  Address m_local;        //!< Local address to bind to
  TypeId  m_tid;          //!< Protocol TypeId
  uint64_t m_totalRx;     //!< Total bytes received


    /**
   * Send data until the L4 transmission buffer is full.
   */
  void SendData (Ptr <Socket>, uint32_t);

  Ptr<Socket>     m_send_socket;       //!< Associated socket
  Address         m_peer;         //!< Peer address
  bool            m_connected;    //!< True if connected
//   uint32_t        m_sendSize;     //!< Size of data to send each time
  uint64_t        m_maxBytes;     //!< Limit total number of bytes sent
  uint64_t        m_flowId;       //!< Flow identifier
  uint64_t        m_totBytes;     //!< Total bytes sent so far
//   TypeId          m_tid;          //!< The type of protocol to use.
  int64_t         m_completionTimeNs; //!< Completion time in nanoseconds
  bool            m_connFailed;       //!< Whether the connection failed
  bool            m_closedNormally;   //!< Whether the connection closed normally
  bool            m_closedByError;    //!< Whether the connection closed by error

  //TODO remove if not needed
  uint64_t        m_ackedBytes;       //!< Amount of acknowledged bytes cached after close of the socket 
  bool            m_isCompleted;      //!< True iff the flow is completed fully AND closed normally

  // Flow logging
  // bool m_enableFlowLoggingToFile;          //!< True iff you want to write flow logs
  std::string m_baseLogsDir;               //!< Where the flow logs will be written to:
                                           //!<   logs_dir/flow-[id]-{progress, cwnd, rtt}.txt
  TracedCallback<Ptr<const Packet> > m_txTrace;

  // horovod ML specific
  std::map<int, uint64_t>          m_layer_size_bytes{{0,1000}, {1,2000}};
  std::map<int, float>        m_backprop_layer_computation_time_ms{{0, 5}, {1, 10}};
  std::map<int, bool>         m_backprop_layer_compute_finished{{0, false}, {1, false}};
  uint32_t                    m_iteration_idx;
  uint32_t                    m_layer_idx;
  uint32_t                    m_num_layers = 2;
  EventId                     m_bp_compute;
  uint32_t                    m_tx_layer_idx;
  std::vector<uint64_t>                    m_curr_send_data_bytes {0, 0};

  // TODO <int, vector<event class>> with layer and size information
  std::map<std::string, std::vector<int64_t>>     m_timeline; //timeline to record invents and their stamps


private:
  void ConnectionSucceeded (Ptr<Socket> socket);
  void ConnectionFailed (Ptr<Socket> socket);
  void DataSend (Ptr<Socket>, uint32_t tosend);
  void SocketClosedNormal(Ptr<Socket> socket);
  void SocketClosedError(Ptr<Socket> socket);


//   void RttChange(Time oldRtt, Time newRtt);
//   void CwndChange(uint32_t oldCwnd, uint32_t newCwnd);
//   void HighestRxAckChange(SequenceNumber<unsigned int, int> oldHighestRxAck, SequenceNumber<unsigned int, int> newHighestRxAck);

// horovod ml specific
 void BackPropagationStart(uint32_t layer_idx);
 void BackPropagationDone(uint32_t layer_idx);
 void SendGradients(uint32_t layer_idx);
};

} // namespace ns3

#endif /* FLOW_SINK_H */
