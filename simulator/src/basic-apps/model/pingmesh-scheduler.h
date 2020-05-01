/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef PINGMESH_SCHEDULER_H
#define PINGMESH_SCHEDULER_H

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
#include "ns3/udp-rtt-helper.h"
#include "ns3/udp-rtt-client.h"
#include "ns3/udp-rtt-server.h"

using namespace ns3;

namespace ns3 {

class PingmeshScheduler
{

public:
    PingmeshScheduler(BasicSimulation* basicSimulation, Topology* topology);
    void Schedule();
    void WriteResults();

protected:
    BasicSimulation* m_basicSimulation;
    int64_t m_simulation_end_time_ns;
    Topology* m_topology = nullptr;
    NodeContainer m_nodes;
    std::vector<ApplicationContainer> m_apps;
    int64_t m_interval_ns;
    std::vector<std::pair<int64_t, int64_t>> m_pingmesh_endpoint_pairs;

};

}

#endif /* PINGMESH_SCHEDULER_H */
