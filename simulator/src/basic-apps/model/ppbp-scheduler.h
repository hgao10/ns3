/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef PPBP_SCHEDULER_H
#define PPBP_SCHEDULER_H

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

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/command-line.h"

#include "ns3/basic-simulation.h"
#include "ns3/exp-util.h"
#include "ns3/topology.h"
#include "ns3/PPBP-helper.h"


namespace ns3 {

class PPBPScheduler
{

public:
    PPBPScheduler(Ptr<BasicSimulation> basicSimulation, Ptr<Topology> topology);
    void Schedule(uint32_t port, uint8_t prio, DoubleValue mburst_arr, DoubleValue mburst_timelt);

protected:
    Ptr<BasicSimulation> m_basicSimulation;
    int64_t m_simulation_end_time_ns;
    Ptr<Topology> m_topology = nullptr;
    NodeContainer m_nodes;
    std::vector<ApplicationContainer> m_apps;
    // std::set<int64_t> m_enableFlowLoggingToFileForFlowIds;

};

}

#endif /* PPBP_SCHEDULER_H */
