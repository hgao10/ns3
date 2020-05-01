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

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/command-line.h"
#include "ns3/traffic-control-helper.h"

#include "ns3/exp-util.h"
#include "ns3/topology.h"
#include "ns3/routing-arbiter.h"
#include "ns3/routing-arbiter-ecmp.h"
#include "ns3/ipv4-arbiter-routing.h"
#include "ns3/ipv4-arbiter-routing-helper.h"

namespace ns3 {

class BasicSimulation
{

public:

    // Primary
    BasicSimulation(std::string run_dir);
    ~BasicSimulation();
    void Run();
    void Finalize();

    // Timestamps to track performance
    int64_t now_ns_since_epoch();
    void RegisterTimestamp(std::string label);
    void RegisterTimestamp(std::string label, int64_t t);

    // Getters
    NodeContainer* GetNodes();
    Topology* GetTopology();
    int64_t GetSimulationEndTimeNs();
    std::string GetConfigParamOrFail(std::string key);
    std::string GetConfigParamOrDefault(std::string key, std::string default_value);
    std::string GetLogsDir();
    std::string GetRunDir();

protected:

    // Internal setup
    void ConfigureRunDirectory();
    void WriteFinished(bool finished);
    void ReadConfig();
    void ConfigureSimulation();
    void SetupNodes();
    void SetupLinks();
    void SetupRouting();
    void SetupTcpParameters();
    void ShowSimulationProgress();
    void RunSimulation();
    void CleanUpSimulation();
    void ConfirmAllConfigParamKeysRequested();
    void StoreTimingResults();

    // List of all important events happening in the pipeline
    std::vector<std::pair<std::string, int64_t>> m_timestamps;

    // Run directory
    std::string m_run_dir;
    std::string m_logs_dir;

    // Config variables
    std::map<std::string, std::string> m_config;
    std::set<std::string> m_configRequestedKeys;
    Topology* m_topology = nullptr;
    int64_t m_simulation_seed;
    int64_t m_simulation_end_time_ns;
    double m_link_data_rate_megabit_per_s;
    int64_t m_link_delay_ns;
    int64_t m_link_max_queue_size_pkts;
    bool m_disable_qdisc_endpoint_tors_xor_servers;
    bool m_disable_qdisc_non_endpoint_switches;

    // Directly derived from config variables
    int64_t m_worst_case_rtt_ns;

    // Generated based on NS-3 variables
    NodeContainer m_nodes;
    std::vector<std::pair<uint32_t, uint32_t>> m_interface_idxs_for_edges;

    // Progress show variables
    int64_t m_sim_start_time_ns_since_epoch;
    int64_t m_last_log_time_ns_since_epoch;
    int m_counter_progress_updates = 0;
    double m_progress_interval_ns = 10000000000; // First one after 10s
    double m_simulation_event_interval_s = 0.00001;

};

}

#endif /* BASIC_SIMULATION_H */
