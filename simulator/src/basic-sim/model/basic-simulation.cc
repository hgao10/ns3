/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "basic-simulation.h"

namespace ns3 {

BasicSimulation::BasicSimulation(std::string run_dir) {
    m_run_dir = run_dir;
    RegisterTimestamp("Start");
    ConfigureRunDirectory();
    WriteFinished(false);
    ReadConfig();
    ConfigureSimulation();
    SetupNodes();
    SetupLinks();
    SetupRouting();
    SetupTcpParameters();
}

BasicSimulation::~BasicSimulation() {
    if (m_topology != nullptr) {
        delete m_topology;
    }
}

int64_t BasicSimulation::now_ns_since_epoch() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void BasicSimulation::RegisterTimestamp(std::string label) {
    m_timestamps.push_back(std::make_pair(label, now_ns_since_epoch()));
}

void BasicSimulation::RegisterTimestamp(std::string label, int64_t t) {
    m_timestamps.push_back(std::make_pair(label, t));
}

void BasicSimulation::ConfigureRunDirectory() {
    std::cout << "CONFIGURE RUN DIRECTORY" << std::endl;

    // Run directory
    if (!dir_exists(m_run_dir)) {
        throw std::runtime_error(format_string("Run directory \"%s\" does not exist.", m_run_dir.c_str()));
    } else {
        printf("  > Run directory: %s\n", m_run_dir.c_str());
    }

    // Logs in run directory
    m_logs_dir = m_run_dir + "/logs_ns3";
    if (dir_exists(m_logs_dir)) {
        printf("  > Emptying existing logs directory\n");
        remove_file_if_exists(m_logs_dir + "/finished.txt");
        remove_file_if_exists(m_logs_dir + "/flows.txt");
        remove_file_if_exists(m_logs_dir + "/flows.csv");
        remove_file_if_exists(m_logs_dir + "/timing_results.txt");
    } else {
        mkdir_if_not_exists(m_logs_dir);
    }
    printf("  > Logs directory: %s\n", m_logs_dir.c_str());
    printf("\n");

    RegisterTimestamp("Configure run directory");
}

void BasicSimulation::WriteFinished(bool finished) {
    std::ofstream fileFinishedEnd(m_logs_dir + "/finished.txt");
    fileFinishedEnd << (finished ? "Yes" : "No") << std::endl;
    fileFinishedEnd.close();
}

void BasicSimulation::ReadConfig() {

    // Read the config
    m_config = read_config(m_run_dir + "/config_ns3.properties");

    // Print full config
    printf("CONFIGURATION\n-----\nKEY                                       VALUE\n");
    std::map<std::string, std::string>::iterator it;
    for ( it = m_config.begin(); it != m_config.end(); it++ ) {
        printf("%-40s  %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");

    // End time
    m_simulation_end_time_ns = parse_positive_int64(GetConfigParamOrFail("simulation_end_time_ns"));

    // Seed
    m_simulation_seed = parse_positive_int64(GetConfigParamOrFail("simulation_seed"));

    // Link properties
    m_link_data_rate_megabit_per_s = parse_positive_double(GetConfigParamOrFail("link_data_rate_megabit_per_s"));
    m_link_delay_ns = parse_positive_int64(GetConfigParamOrFail("link_delay_ns"));
    m_link_max_queue_size_pkts = parse_positive_int64(GetConfigParamOrFail("link_max_queue_size_pkts"));

    // Qdisc properties
    m_disable_qdisc_endpoint_tors_xor_servers = parse_boolean(GetConfigParamOrFail("disable_qdisc_endpoint_tors_xor_servers"));
    m_disable_qdisc_non_endpoint_switches = parse_boolean(GetConfigParamOrFail("disable_qdisc_non_endpoint_switches"));

    // Read topology
    m_topology = new Topology(m_run_dir + "/" + GetConfigParamOrFail("filename_topology"));
    printf("TOPOLOGY\n-----\n");
    printf("%-25s  %" PRIu64 "\n", "# Nodes", m_topology->num_nodes);
    printf("%-25s  %" PRIu64 "\n", "# Undirected edges", m_topology->num_undirected_edges);
    printf("%-25s  %" PRIu64 "\n", "# Switches", m_topology->switches.size());
    printf("%-25s  %" PRIu64 "\n", "# ... of which ToRs", m_topology->switches_which_are_tors.size());
    printf("%-25s  %" PRIu64 "\n", "# Servers", m_topology->servers.size());
    std::cout << std::endl;
    RegisterTimestamp("Read topology");

    // MTU = 1500 byte, +2 with the p2p header.
    // There are n_q packets in the queue at most, e.g. n_q + 2 (incl. transmit and within mandatory 1-packet qdisc)
    // Queueing + transmission delay/hop = (n_q + 2) * 1502 byte / link data rate
    // Propagation delay/hop = link delay
    //
    // If the topology is big, lets assume 10 hops either direction worst case, so 20 hops total
    // If the topology is not big, < 10 undirected edges, we just use 2 * number of undirected edges as worst-case hop count
    //
    // num_hops * (((n_q + 2) * 1502 byte) / link data rate) + link delay)
    int num_hops = std::min((int64_t) 20, m_topology->num_undirected_edges * 2);
    m_worst_case_rtt_ns = num_hops * (((m_link_max_queue_size_pkts + 2) * 1502) / (m_link_data_rate_megabit_per_s * 125000 / 1000000000) + m_link_delay_ns);
    printf("Estimated worst-case RTT: %.3f ms\n\n", m_worst_case_rtt_ns / 1e6);

}

void BasicSimulation::ConfigureSimulation() {
    std::cout << "CONFIGURE SIMULATION" << std::endl;

    // Set primary seed
    ns3::RngSeedManager::SetSeed(m_simulation_seed);
    std::cout << "  > Seed: " << m_simulation_seed << std::endl;

    // Set end time
    Simulator::Stop(NanoSeconds(m_simulation_end_time_ns));
    printf("  > Duration: %.2f s (%" PRId64 " ns)\n", m_simulation_end_time_ns / 1e9, m_simulation_end_time_ns);

    std::cout << std::endl;
    RegisterTimestamp("Configure simulator");
}

void BasicSimulation::SetupNodes() {
    std::cout << "SETUP NODES" << std::endl;
    std::cout << "  > Creating nodes and installing Internet stack on each" << std::endl;

    // Install Internet on all nodes
    m_nodes.Create(m_topology->num_nodes);
    InternetStackHelper internet;
    Ipv4ArbiterRoutingHelper arbiterRoutingHelper;
    internet.SetRoutingHelper(arbiterRoutingHelper);
    internet.Install(m_nodes);

    std::cout << std::endl;
    RegisterTimestamp("Create and install nodes");
}

void BasicSimulation::SetupLinks() {
    std::cout << "SETUP LINKS" << std::endl;

    // Each link is a network on its own
    Ipv4AddressHelper address;
    address.SetBase("10.0.0.0", "255.255.255.0");

    // Direct network device attributes
    std::cout << "  > Point-to-point network device attributes:" << std::endl;

    // Point-to-point network device helper
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue(std::to_string(m_link_data_rate_megabit_per_s) + "Mbps"));
    p2p.SetChannelAttribute("Delay", TimeValue(NanoSeconds(m_link_delay_ns)));
    std::cout << "      >> Data rate......... " << m_link_data_rate_megabit_per_s << " Mbit/s" << std::endl;
    std::cout << "      >> Delay............. " << m_link_delay_ns << " ns" << std::endl;
    std::cout << "      >> Max. queue size... " << m_link_max_queue_size_pkts << " packets" << std::endl;
    std::string p2p_net_device_max_queue_size_pkts_str = format_string("%" PRId64 "p", m_link_max_queue_size_pkts);

    // Notify about topology state
    if (m_topology->are_tors_endpoints()) {
        std::cout << "  > Because there are no servers, ToRs are considered valid flow-endpoints" << std::endl;
    } else {
        std::cout << "  > Only servers are considered valid flow-endpoints" << std::endl;
    }

    // Queueing disciplines
    std::cout << "  > Traffic control queueing disciplines:" << std::endl;

    // Qdisc for endpoints (i.e., servers if they are defined, else the ToRs)
    TrafficControlHelper tch_endpoints;
    if (m_disable_qdisc_endpoint_tors_xor_servers) {
        tch_endpoints.SetRootQueueDisc("ns3::FifoQueueDisc", "MaxSize", QueueSizeValue(QueueSize("1p"))); // No queueing discipline basically
        std::cout << "      >> Flow-endpoints....... none (FIFO with 1 max. queue size)" << std::endl;
    } else {
        std::string interval = format_string("%" PRId64 "ns", m_worst_case_rtt_ns);
        std::string target = format_string("%" PRId64 "ns", m_worst_case_rtt_ns / 20);
        tch_endpoints.SetRootQueueDisc("ns3::FqCoDelQueueDisc", "Interval", StringValue(interval), "Target", StringValue(target));
        printf("      >> Flow-endpoints....... fq-co-del (interval = %.2f ms, target = %.2f ms)\n", m_worst_case_rtt_ns / 1e6, m_worst_case_rtt_ns / 1e6 / 20);
    }

    // Qdisc for non-endpoints (i.e., if servers are defined, all switches, else the switches which are not ToRs)
    TrafficControlHelper tch_not_endpoints;
    if (m_disable_qdisc_non_endpoint_switches) {
        tch_not_endpoints.SetRootQueueDisc("ns3::FifoQueueDisc", "MaxSize", QueueSizeValue(QueueSize("1p"))); // No queueing discipline basically
        std::cout << "      >> Non-flow-endpoints... none (FIFO with 1 max. queue size)" << std::endl;
    } else {
        std::string interval = format_string("%" PRId64 "ns", m_worst_case_rtt_ns);
        std::string target = format_string("%" PRId64 "ns", m_worst_case_rtt_ns / 20);
        tch_not_endpoints.SetRootQueueDisc("ns3::FqCoDelQueueDisc", "Interval", StringValue(interval), "Target", StringValue(target));
        printf("      >> Non-flow-endpoints... fq-co-del (interval = %.2f ms, target = %.2f ms)\n", m_worst_case_rtt_ns / 1e6, m_worst_case_rtt_ns / 1e6 / 20);
    }

    // Create Links
    std::cout << "  > Installing links" << std::endl;
    m_interface_idxs_for_edges.clear();
    for (std::pair <int64_t, int64_t> link : m_topology->undirected_edges) {

        // Install link
        NetDeviceContainer container = p2p.Install(m_nodes.Get(link.first), m_nodes.Get(link.second));
        container.Get(0)->GetObject<PointToPointNetDevice>()->GetQueue()->SetMaxSize(p2p_net_device_max_queue_size_pkts_str);
        container.Get(1)->GetObject<PointToPointNetDevice>()->GetQueue()->SetMaxSize(p2p_net_device_max_queue_size_pkts_str);

        // Install traffic control
        if (m_topology->is_valid_endpoint(link.first)) {
            tch_endpoints.Install(container.Get(0));
        } else {
            tch_not_endpoints.Install(container.Get(0));
        }
        if (m_topology->is_valid_endpoint(link.second)) {
            tch_endpoints.Install(container.Get(1));
        } else {
            tch_not_endpoints.Install(container.Get(1));
        }

        // Assign IP addresses
        address.Assign(container);
        address.NewNetwork();

        // Save to mapping
        uint32_t a = container.Get(0)->GetIfIndex();
        uint32_t b = container.Get(1)->GetIfIndex();
        m_interface_idxs_for_edges.push_back(std::make_pair(a, b));

    }

    std::cout << std::endl;
    RegisterTimestamp("Create links and edge-to-interface-index mapping");
}

void BasicSimulation::SetupRouting() {
    std::cout << "SETUP ROUTING" << std::endl;

    // Calculate and instantiate the routing
    std::cout << "  > Calculating ECMP routing" << std::endl;
    std::vector<std::vector<std::vector<uint32_t>>> global_ecmp_state = ArbiterEcmp::CalculateGlobalState(m_topology);
    RegisterTimestamp("Calculate ECMP routing state");

    std::cout << "  > Setting the routing protocol on each node" << std::endl;
    for (int i = 0; i < m_topology->num_nodes; i++) {
        Ptr<ArbiterEcmp> arbiterEcmp = CreateObject<ArbiterEcmp>(m_nodes.Get(i), m_nodes, m_topology, m_interface_idxs_for_edges, global_ecmp_state[i]);
        m_nodes.Get(i)->GetObject<Ipv4>()->GetRoutingProtocol()->GetObject<Ipv4ArbiterRouting>()->SetArbiter(arbiterEcmp);
    }
    RegisterTimestamp("Setup routing arbiter on each node");

    std::cout << std::endl;

}

void BasicSimulation::SetupTcpParameters() {
    std::cout << "TCP PARAMETERS" << std::endl;

    // Clock granularity
    printf("  > Clock granularity.......... 1 ns\n");
    Config::SetDefault("ns3::TcpSocketBase::ClockGranularity", TimeValue(NanoSeconds(1)));

    // Initial congestion window
    uint32_t init_cwnd_pkts = 10;  // 1 is default, but we use 10
    printf("  > Initial CWND............... %u packets\n", init_cwnd_pkts);
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(init_cwnd_pkts));

    // Maximum segment lifetime
    int64_t max_seg_lifetime_ns = 5 * m_worst_case_rtt_ns; // 120s is default
    printf("  > Maximum segment lifetime... %.3f ms\n", max_seg_lifetime_ns / 1e6);
    Config::SetDefault("ns3::TcpSocketBase::MaxSegLifetime", DoubleValue(max_seg_lifetime_ns / 1e9));

    // Minimum retransmission timeout
    int64_t min_rto_ns = (int64_t)(1.5 * m_worst_case_rtt_ns);  // 1s is default, Linux uses 200ms
    printf("  > Minimum RTO................ %.3f ms\n", min_rto_ns / 1e6);
    Config::SetDefault("ns3::TcpSocketBase::MinRto", TimeValue(NanoSeconds(min_rto_ns)));

    // Initial RTT estimate
    int64_t initial_rtt_estimate_ns = 2 * m_worst_case_rtt_ns;  // 1s is default
    printf("  > Initial RTT measurement.... %.3f ms\n", initial_rtt_estimate_ns / 1e6);
    Config::SetDefault("ns3::RttEstimator::InitialEstimation", TimeValue(NanoSeconds(initial_rtt_estimate_ns)));

    // Connection timeout
    int64_t connection_timeout_ns = 2 * m_worst_case_rtt_ns;  // 3s is default
    printf("  > Connection timeout......... %.3f ms\n", connection_timeout_ns / 1e6);
    Config::SetDefault("ns3::TcpSocket::ConnTimeout", TimeValue(NanoSeconds(connection_timeout_ns)));

    // Delayed ACK timeout
    int64_t delayed_ack_timeout_ns = 0.2 * m_worst_case_rtt_ns;  // 0.2s is default
    printf("  > Delayed ACK timeout........ %.3f ms\n", delayed_ack_timeout_ns / 1e6);
    Config::SetDefault("ns3::TcpSocket::DelAckTimeout", TimeValue(NanoSeconds(delayed_ack_timeout_ns)));

    // Persist timeout
    int64_t persist_timeout_ns = 4 * m_worst_case_rtt_ns;  // 6s is default
    printf("  > Persist timeout............ %.3f ms\n", persist_timeout_ns / 1e6);
    Config::SetDefault("ns3::TcpSocket::PersistTimeout", TimeValue(NanoSeconds(persist_timeout_ns)));

    // Send buffer size
    int64_t snd_buf_size_byte = 131072 * 256;  // 131072 bytes = 128 KiB is default, we set to 32 MiB
    printf("  > Send buffer size........... %.3f MB\n", snd_buf_size_byte / 1e6);
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(snd_buf_size_byte));

    // Receive buffer size
    int64_t rcv_buf_size_byte = 131072 * 256;  // 131072 bytes = 128 KiB is default, we set to 32 MiB
    printf("  > Receive buffer size........ %.3f MB\n", rcv_buf_size_byte / 1e6);
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(rcv_buf_size_byte));

    // Segment size
    int64_t segment_size_byte = 1380;   // 536 byte is default, but we know the a point-to-point network device has an MTU of 1500.
                                        // IP header size: min. 20 byte, max. 60 byte
                                        // TCP header size: min. 20 byte, max. 60 byte
                                        // So, 1500 - 60 - 60 = 1380 would be the safest bet (given we don't do tunneling)
                                        // This could be increased higher, e.g. as discussed here:
                                        // https://blog.cloudflare.com/increasing-ipv6-mtu/ (retrieved April 7th, 2020)
                                        // In past ns-3 simulations, I've observed that the IP + TCP header is generally not larger than 80 bytes.
                                        // This means it could be potentially set closer to 1400-1420.
    printf("  > Segment size............... %" PRId64 " byte\n", segment_size_byte);
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(segment_size_byte));

    // Timestamp option
    bool opt_timestamp_enabled = true;  // Default: true.
                                        // To get an RTT measurement with a resolution of less than 1ms, it needs
                                        // to be disabled because the fields in the TCP Option are in milliseconds.
                                        // When disabling it, there are two downsides:
                                        //  (1) Less protection against wrapped sequence numbers (PAWS)
                                        //  (2) Algorithm to see if it has entered loss recovery unnecessarily are not as possible (Eiffel)
                                        // See: https://tools.ietf.org/html/rfc7323#section-3
                                        //      https://tools.ietf.org/html/rfc3522
    printf("  > Timestamp option........... %s\n", opt_timestamp_enabled ? "enabled" : "disabled");
    Config::SetDefault("ns3::TcpSocketBase::Timestamp", BooleanValue(opt_timestamp_enabled));

    std::cout << std::endl;
    RegisterTimestamp("Setup TCP parameters");
}

void BasicSimulation::ShowSimulationProgress() {
    int64_t now = now_ns_since_epoch();
    if (now - m_last_log_time_ns_since_epoch > m_progress_interval_ns) {
        printf(
                "%5.2f%% - Simulation Time = %.2f s ::: Wallclock Time = %.2f s\n",
                (Simulator::Now().GetSeconds() / (m_simulation_end_time_ns / 1e9)) * 100.0,
                Simulator::Now().GetSeconds(),
                (now - m_sim_start_time_ns_since_epoch) / 1e9
        );
        if (m_counter_progress_updates < 8 || m_counter_progress_updates % 5 == 0) { // The first 8 and every 5 progress updates we show estimate
            int remaining_s = (int) ((m_simulation_end_time_ns / 1e9 - Simulator::Now().GetSeconds()) / (Simulator::Now().GetSeconds() / ((now - m_sim_start_time_ns_since_epoch) / 1e9)));
            int seconds = remaining_s % 60;
            int minutes = ((remaining_s - seconds) / 60) % 60;
            int hours = (remaining_s - seconds - minutes * 60) / 3600;
            std::string remainder;
            if (hours > 0) {
                remainder = format_string("Estimated wallclock time remaining: %d hours %d minutes", hours, minutes);
            } else if (minutes > 0) {
                remainder = format_string("Estimated wallclock time remaining: %d minutes %d seconds", minutes, seconds);
            } else {
                remainder = format_string("Estimated wallclock time remaining: %d seconds", seconds);
            }
            std::cout << remainder << std::endl;
        }
        m_last_log_time_ns_since_epoch = now;
        if (m_counter_progress_updates < 4) {
            m_progress_interval_ns = 10000000000; // The first five are every 10s
        } else if (m_counter_progress_updates < 19) {
            m_progress_interval_ns = 20000000000; // The next 15 are every 20s
        } else if (m_counter_progress_updates < 99) {
            m_progress_interval_ns = 60000000000; // The next 80 are every 60s
        } else {
            m_progress_interval_ns = 360000000000; // After that, it is every 360s = 5 minutes
        }
        m_counter_progress_updates++;
    }
    Simulator::Schedule(Seconds(m_simulation_event_interval_s), &BasicSimulation::ShowSimulationProgress, this);
}

void BasicSimulation::ConfirmAllConfigParamKeysRequested() {
    for (const std::pair<std::string, std::string>& key_val : m_config) {
        if (m_configRequestedKeys.find(key_val.first) == m_configRequestedKeys.end()) {
            throw std::runtime_error(format_string("Config key \'%s\' has not been requested (unused config keys are not allowed)", key_val.first.c_str()));
        }
    }
}

void BasicSimulation::Run() {
    std::cout << "SIMULATION" << std::endl;

    // Before it starts to run, we need to have processed all the config
    ConfirmAllConfigParamKeysRequested();

    // Schedule progress printing
    m_sim_start_time_ns_since_epoch = now_ns_since_epoch();
    m_last_log_time_ns_since_epoch = m_sim_start_time_ns_since_epoch;
    Simulator::Schedule(Seconds(m_simulation_event_interval_s), &BasicSimulation::ShowSimulationProgress, this);

    // Run
    printf("Running the simulation for %.2f simulation seconds...\n", (m_simulation_end_time_ns / 1e9));
    Simulator::Run();
    printf("Finished simulation.\n");

    // Print final duration
    printf(
            "Simulation of %.1f seconds took in wallclock time %.1f seconds.\n\n",
            m_simulation_end_time_ns / 1e9,
            (now_ns_since_epoch() - m_sim_start_time_ns_since_epoch) / 1e9
    );

    RegisterTimestamp("Run simulation");
}

void BasicSimulation::CleanUpSimulation() {
    std::cout << "CLEAN-UP" << std::endl;
    Simulator::Destroy();
    std::cout << "  > Simulator is destroyed" << std::endl;
    std::cout << std::endl;
    RegisterTimestamp("Destroy simulator");
}

void BasicSimulation::StoreTimingResults() {
    std::cout << "TIMING RESULTS" << std::endl;
    std::cout << "------" << std::endl;

    // Write to both file and out
    std::ofstream fileTimingResults(m_logs_dir + "/timing_results.txt");
    int64_t t_prev = -1;
    for (std::pair <std::string, int64_t> &ts : m_timestamps) {
        if (t_prev == -1) {
            t_prev = ts.second;
        } else {
            std::string line = format_string(
                    "[%7.1f - %7.1f] (%.1f s) :: %s",
                    (t_prev - m_timestamps[0].second) / 1e9,
                    (ts.second - m_timestamps[0].second) / 1e9,
                    (ts.second - t_prev) / 1e9,
                    ts.first.c_str()
            );
            std::cout << line << std::endl;
            fileTimingResults << line << std::endl;
            t_prev = ts.second;
        }
    }
    fileTimingResults.close();

    std::cout << std::endl;
}

void BasicSimulation::Finalize() {
    CleanUpSimulation();
    StoreTimingResults();
    WriteFinished(true);
}

NodeContainer* BasicSimulation::GetNodes() {
    return &m_nodes;
}

Topology* BasicSimulation::GetTopology() {
    return m_topology;
}

int64_t BasicSimulation::GetSimulationEndTimeNs() {
    return m_simulation_end_time_ns;
}

std::string BasicSimulation::GetConfigParamOrFail(std::string key) {
    m_configRequestedKeys.insert(key);
    return get_param_or_fail(key, m_config);
}

std::string BasicSimulation::GetConfigParamOrDefault(std::string key, std::string default_value) {
    m_configRequestedKeys.insert(key);
    return get_param_or_default(key, default_value, m_config);
}

std::string BasicSimulation::GetLogsDir() {
    return m_logs_dir;
}

std::string BasicSimulation::GetRunDir() {
    return m_run_dir;
}

}
