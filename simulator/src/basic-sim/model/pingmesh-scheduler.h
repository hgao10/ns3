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
#include "ns3/simon-util.h"
#include "ns3/schedule-reader.h"
#include "ns3/topology.h"
#include "ns3/flow-send-helper.h"
#include "ns3/flow-send-application.h"
#include "ns3/flow-sink-helper.h"
#include "ns3/flow-sink.h"
#include "ns3/routing-arbiter.h"
#include "ns3/routing-arbiter-ecmp.h"
#include "ns3/ipv4-arbitrary-routing.h"
#include "ns3/ipv4-arbitrary-routing-helper.h"
#include "ns3/traffic-control-helper.h"
#include "ns3/basic-simulation.h"

namespace ns3 {

class RttPingTracer {
public:

    RttPingTracer(int64_t from, int64_t to) {
        m_from_node_id = from;
        m_to_node_id = to;
    }

    void ReceivePacket(Ptr<const Packet>) {
        m_rtts.push_back(Simulator::Now().GetNanoSeconds());
    }

    int64_t GetFrom() {
        return m_from_node_id;
    }

    int64_t GetTo() {
        return m_to_node_id;
    }

    std::vector<int64_t> GetRtts() {
        return m_rtts;
    }

private:
    int64_t m_from_node_id;
    int64_t m_to_node_id;
    std::vector<int64_t> m_rtts;

};

class PingmeshScheduler
{

public:
    PingmeshScheduler(BasicSimulation* basicSimulation);
    ~PingmeshScheduler();
    void Schedule();
    void WriteResults();

protected:
    BasicSimulation* m_basicSimulation;
    int64_t m_simulation_end_time_ns;
    Topology* m_topology = nullptr;
    std::vector<schedule_entry_t> m_schedule;
    NodeContainer* m_nodes;
    std::vector<ApplicationContainer> m_apps;
    std::vector<RttPingTracer*> m_rttPingTracers;

};

}

#endif /* PINGMESH_SCHEDULER_H */
