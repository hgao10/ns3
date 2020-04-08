/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef BASIC_SIMULATION_H
#define BASIC_SIMULATION_H

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

namespace ns3 {

class BasicSimulation
{

public:
    BasicSimulation();
    ~BasicSimulation();
    void Run(std::string run_dir);

protected:
    int64_t now_ns_since_epoch();
    void ConfigureRunDirectory();
    void WriteFinished(bool finished);
    void ReadConfig();
    void ConfigureSimulation();
    void SetupNodes();
    void SetupLinks();
    void SetupRouting();
    void SetupTcpParameters();
    void StartNextFlow(int i);
    void ScheduleApplications();
    void ShowSimulationProgress();
    void RunSimulation();
    void StoreApplicationResults();
    void CleanUpSimulation();
    void StoreTimingResults();

    // List of all important events happening in the pipeline
    std::vector<std::pair<std::string, int64_t>> m_timestamps;

    // Run directory
    std::string m_run_dir;
    std::string m_logs_dir;

    // Config variables
    std::vector<schedule_entry_t> m_schedule;
    Topology* m_topology;
    int64_t m_simulation_seed;
    int64_t m_simulation_end_time_ns;
    double m_link_data_rate_megabit_per_s;
    int64_t m_link_delay_ns;
    int64_t m_link_max_queue_size_pkts;
    bool m_disable_qdisc_endpoint_tors_xor_servers;
    bool m_disable_qdisc_non_endpoint_switches;

    // Generated based on NS-3 variables
    NodeContainer m_nodes;
    std::vector<std::pair<uint32_t, uint32_t>> m_interface_idxs_for_edges;
    std::vector<ApplicationContainer> m_apps;
    RoutingArbiterEcmp* m_routingArbiterEcmp;

    // Progress show variables
    int64_t m_sim_start_time_ns_since_epoch;
    int64_t m_last_log_time_ns_since_epoch;
    int m_counter_progress_updates = 0;
    double m_progress_interval_ns = 10000000000; // First one after 10s
    double m_simulation_event_interval_s = 0.00001;

};

}

#endif /* BASIC_SIMULATION_H */
