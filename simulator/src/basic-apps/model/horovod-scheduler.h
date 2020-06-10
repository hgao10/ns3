/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef HOROVOD_SCHEDULER_H
#define HOROVOD_SCHEDULER_H

#include <map>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <chrono>
#include <stdexcept>

#include "ns3/flow-monitor-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/random-variable-stream.h"
#include "ns3/command-line.h"

#include "ns3/basic-simulation.h"
#include "ns3/exp-util.h"
#include "ns3/topology.h"
#include "ns3/horovod-worker-helper.h"
#include "ringallreduce-syncer.h"

using namespace ns3;

namespace ns3 {

// class GlobalRingAllReduceSyncer; 
class HorovodScheduler
{

public:
    HorovodScheduler(Ptr<BasicSimulation> basicSimulation, Ptr<Topology> topology);
    void Schedule(uint32_t port, uint8_t prio);
    void WriteResults();

protected:
    Ptr<BasicSimulation> m_basicSimulation;
    int64_t m_simulation_end_time_ns;
    Ptr<Topology> m_topology = nullptr;
    // std::vector<schedule_entry_t> m_schedule;
    NodeContainer m_nodes;
    std::vector<ApplicationContainer> m_apps;
    std::set<int64_t> m_enableFlowLoggingToFileForFlowIds;
    int64_t m_interval_ns;
    std::vector<std::vector<int64_t>> m_horovod_neighbors; // [left_nei, node, right_nei]
    std::map<int, std::shared_ptr<InetSocketAddress>> m_ipv4_addr_self; //Local addr
    std::map<int, std::shared_ptr<InetSocketAddress>> m_ipv4_addr_remote; //Remote addr

    GlobalRingAllReduceSyncer m_global_ringallreduce_syncer;
};

}

#endif /* PINGMESH_SCHEDULER_H */
