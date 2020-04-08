/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "basic-simulation.h"

namespace ns3 {

BasicSimulation::BasicSimulation() {
    // Left intentionally empty
}

BasicSimulation::~BasicSimulation() {
    delete m_topology;
    delete m_routingArbiterEcmp;
}

int64_t BasicSimulation::now_ns_since_epoch() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void BasicSimulation::Run(std::string m_run_dir) {
    this->m_run_dir = m_run_dir;
    m_timestamps.push_back(std::make_pair("Start", now_ns_since_epoch()));
    ConfigureRunDirectory();
    WriteFinished(false);
    ReadConfig();
    ConfigureSimulation();
    SetupNodes();
    SetupLinks();
    SetupRouting();
    SetupTcpParameters();
    ScheduleApplications();
    RunSimulation();
    StoreApplicationResults();
    CleanUpSimulation();
    StoreTimingResults();
    WriteFinished(true);
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
    this->m_logs_dir = m_run_dir + "/logs_ns3";
    if (dir_exists(m_logs_dir)) {
        printf("  > Emptying existing logs directory\n");
        remove_file_if_exists(m_logs_dir + "/finished.txt");
        remove_file_if_exists(m_logs_dir + "/flows.txt");
        remove_file_if_exists(m_logs_dir + "/flows.csv");
        remove_file_if_exists(m_logs_dir + "/timing-results.txt");
    } else {
        mkdir_if_not_exists(m_logs_dir);
    }
    printf("  > Logs directory: %s\n", m_logs_dir.c_str());
    printf("\n");

    m_timestamps.push_back(std::make_pair("Configure run directory", now_ns_since_epoch()));
}

void BasicSimulation::WriteFinished(bool finished) {
    std::ofstream fileFinishedEnd(m_logs_dir + "/finished.txt");
    fileFinishedEnd << (finished ? "Yes" : "No") << std::endl;
    fileFinishedEnd.close();
}

void BasicSimulation::ReadConfig() {

    // Read the config
    std::map<std::string, std::string> config = read_config(m_run_dir + "/config_ns3.properties");

    // Print full config
    printf("CONFIGURATION\n-----\nKEY                             VALUE\n");
    std::map<std::string, std::string>::iterator it;
    for ( it = config.begin(); it != config.end(); it++ ) {
        printf("%-30s  %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");

    // End time
    m_simulation_end_time_ns = parse_positive_int64(get_param_or_fail("simulation_end_time_ns", config));

    // Seed
    m_simulation_seed = parse_positive_int64(get_param_or_fail("simulation_seed", config));

    // Link properties
    m_link_data_rate_megabit_per_s = parse_positive_double(get_param_or_fail("link_data_rate_megabit_per_s", config));
    m_link_delay_ns = parse_positive_int64(get_param_or_fail("link_delay_ns", config));
    m_link_max_queue_size_pkts = parse_positive_int64(get_param_or_fail("link_max_queue_size_pkts", config));

    // Qdisc properties
    m_disable_qdisc_endpoint_tors_xor_servers = parse_boolean(get_param_or_fail("disable_qdisc_endpoint_tors_xor_servers", config));
    m_disable_qdisc_non_endpoint_switches = parse_boolean(get_param_or_fail("disable_qdisc_non_endpoint_switches", config));

    // Read topology
    m_topology = new Topology(m_run_dir + "/" + get_param_or_fail("filename_topology", config));
    printf("TOPOLOGY\n-----\n");
    printf("%-25s  %" PRIu64 "\n", "# Nodes", m_topology->num_nodes);
    printf("%-25s  %" PRIu64 "\n", "# Undirected edges", m_topology->num_undirected_edges);
    printf("%-25s  %" PRIu64 "\n", "# Switches", m_topology->switches.size());
    printf("%-25s  %" PRIu64 "\n", "# ... of which ToRs", m_topology->switches_which_are_tors.size());
    printf("%-25s  %" PRIu64 "\n", "# Servers", m_topology->servers.size());
    std::cout << std::endl;
    m_timestamps.push_back(std::make_pair("Read topology", now_ns_since_epoch()));

    // Read schedule
    m_schedule = read_schedule(m_run_dir + "/" + get_param_or_fail("filename_schedule", config), *m_topology, m_simulation_end_time_ns);
    printf("SCHEDULE\n");
    printf("  > Read schedule (total flow start events: %lu)\n", m_schedule.size());
    m_timestamps.push_back(std::make_pair("Read schedule", now_ns_since_epoch()));

    std::cout << std::endl;
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
    m_timestamps.push_back(std::make_pair("Configure simulator", now_ns_since_epoch()));
}

void BasicSimulation::SetupNodes() {
    std::cout << "SETUP NODES" << std::endl;
    std::cout << "  > Creating nodes and installing Internet stack on each" << std::endl;

    // Install Internet on all nodes
    m_nodes.Create(m_topology->num_nodes);
    InternetStackHelper internet;
    Ipv4ArbitraryRoutingHelper arbitraryRoutingHelper;
    internet.SetRoutingHelper(arbitraryRoutingHelper);
    internet.Install(m_nodes);

    std::cout << std::endl;
    m_timestamps.push_back(std::make_pair("Create and install nodes", now_ns_since_epoch()));
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
        tch_endpoints.SetRootQueueDisc("ns3::FqCoDelQueueDisc"); // This is default
        //tch_endpoints.SetRootQueueDisc("ns3::FqCoDelQueueDisc", "Interval", StringValue("20ms"), "Target", StringValue("1ms")); // TODO: Figure out what are reasonable settings here
        std::cout << "      >> Flow-endpoints....... default fq-co-del" << std::endl;
    }

    // Qdisc for non-endpoints (i.e., if servers are defined, all switches, else the switches which are not ToRs)
    TrafficControlHelper tch_not_endpoints;
    if (m_disable_qdisc_non_endpoint_switches) {
        tch_not_endpoints.SetRootQueueDisc("ns3::FifoQueueDisc", "MaxSize", QueueSizeValue(QueueSize("1p"))); // No queueing discipline basically
        std::cout << "      >> Non-flow-endpoints... none (FIFO with 1 max. queue size)" << std::endl;
    } else {
        tch_endpoints.SetRootQueueDisc ("ns3::FqCoDelQueueDisc"); // This is default
        std::cout << "      >> Non-flow-endpoints... default fq-co-del" << std::endl;
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
        if (m_topology->is_valid_flow_endpoint(link.first)) {
            tch_endpoints.Install(container.Get(0));
        } else {
            tch_not_endpoints.Install(container.Get(0));
        }
        if (m_topology->is_valid_flow_endpoint(link.second)) {
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
    m_timestamps.push_back(std::make_pair("Create links and edge-to-interface-index mapping", now_ns_since_epoch()));
}

void BasicSimulation::SetupRouting() {
    std::cout << "SETUP ROUTING" << std::endl;

    // Calculate and instantiate the routing
    std::cout << "  > Calculating ECMP routing" << std::endl;
    m_routingArbiterEcmp = new RoutingArbiterEcmp(m_topology, m_nodes, m_interface_idxs_for_edges); // Remains alive for the entire simulation
    m_timestamps.push_back(std::make_pair("Routing arbiter base", m_routingArbiterEcmp->get_base_init_finish_ns_since_epoch()));
    m_timestamps.push_back(std::make_pair("Routing arbiter ECMP", m_routingArbiterEcmp->get_ecmp_init_finish_ns_since_epoch()));
    std::cout << "  > Setting the routing protocol on each node" << std::endl;
    for (int i = 0; i < m_topology->num_nodes; i++) {
        m_nodes.Get(i)->GetObject<Ipv4>()->GetRoutingProtocol()->GetObject<Ipv4ArbitraryRouting>()->SetRoutingArbiter(m_routingArbiterEcmp);
    }

    std::cout << std::endl;
    m_timestamps.push_back(std::make_pair("Setup routing arbiter on each node", now_ns_since_epoch()));
}

void BasicSimulation::SetupTcpParameters() {
    std::cout << "TCP PARAMETERS" << std::endl;

    // Clock granularity
    printf("  > Clock granularity: 1 ns\n");
    Config::SetDefault("ns3::TcpSocketBase::ClockGranularity", TimeValue(NanoSeconds(1)));

    // Initial congestion window
    uint32_t init_cwnd_pkts = 10;  // 1 is default, but we use 10
    printf("  > Initial CWND: %u packets\n", init_cwnd_pkts);
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(init_cwnd_pkts));

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
    double worst_case_rtt_ns =
            num_hops * (((m_link_max_queue_size_pkts + 2) * 1502) / (m_link_data_rate_megabit_per_s * 125000 / 1000000000) + m_link_delay_ns);
    // TODO: Add accounting for qdisc queues here
    printf("  > Estimated worst-case RTT: %.3f ms\n", worst_case_rtt_ns / 1e6);

    // Maximum segment lifetime
    int64_t max_seg_lifetime_ns = 5 * worst_case_rtt_ns; // 120s is default
    printf("  > Maximum segment lifetime: %.3f ms\n", max_seg_lifetime_ns / 1e6);
    Config::SetDefault("ns3::TcpSocketBase::MaxSegLifetime", DoubleValue(max_seg_lifetime_ns / 1e9));

    // Minimum retransmission timeout
    int64_t min_rto_ns = (int64_t)(1.5 * worst_case_rtt_ns);  // 1s is default, Linux uses 200ms
    printf("  > Minimum RTO: %.3f ms\n", min_rto_ns / 1e6);
    Config::SetDefault("ns3::TcpSocketBase::MinRto", TimeValue(NanoSeconds(min_rto_ns)));

    // Initial RTT estimate
    int64_t initial_rtt_estimate_ns = 2 * worst_case_rtt_ns;  // 1s is default
    printf("  > Initial RTT measurement: %.3f ms\n", initial_rtt_estimate_ns / 1e6);
    Config::SetDefault("ns3::RttEstimator::InitialEstimation", TimeValue(NanoSeconds(initial_rtt_estimate_ns)));

    // Connection timeout
    int64_t connection_timeout_ns = 2 * worst_case_rtt_ns;  // 3s is default
    printf("  > Connection timeout: %.3f ms\n", connection_timeout_ns / 1e6);
    Config::SetDefault("ns3::TcpSocket::ConnTimeout", TimeValue(NanoSeconds(connection_timeout_ns)));

    // Delayed ACK timeout
    int64_t delayed_ack_timeout_ns = 0.2 * worst_case_rtt_ns;  // 0.2s is default
    printf("  > Delayed ACK timeout: %.3f ms\n", delayed_ack_timeout_ns / 1e6);
    Config::SetDefault("ns3::TcpSocket::DelAckTimeout", TimeValue(NanoSeconds(delayed_ack_timeout_ns)));

    // Persist timeout
    int64_t persist_timeout_ns = 4 * worst_case_rtt_ns;  // 6s is default
    printf("  > Persist timeout: %.3f ms\n", persist_timeout_ns / 1e6);
    Config::SetDefault("ns3::TcpSocket::PersistTimeout", TimeValue(NanoSeconds(persist_timeout_ns)));

    // Send buffer size
    int64_t snd_buf_size_byte = 131072 * 256;  // 131072 bytes = 128 KiB is default, we set to 32 MiB
    printf("  > Send buffer size: %.3f MB\n", snd_buf_size_byte / 1e6);
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(snd_buf_size_byte));

    // Receive buffer size
    int64_t rcv_buf_size_byte = 131072 * 256;  // 131072 bytes = 128 KiB is default, we set to 32 MiB
    printf("  > Receive buffer size: %.3f MB\n", rcv_buf_size_byte / 1e6);
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
    printf("  > Segment size: %" PRId64 " byte\n", segment_size_byte);
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(segment_size_byte));

    std::cout << std::endl;
    m_timestamps.push_back(std::make_pair("Setup TCP parameters", now_ns_since_epoch()));
}

void BasicSimulation::StartNextFlow(int i) {

    // Fetch the flow to start
    schedule_entry_t& entry = m_schedule[i];
    int64_t now_ns = Simulator::Now().GetNanoSeconds();
    if (now_ns != entry.start_time_ns) {
        throw std::runtime_error("Scheduling start of a flow went horribly wrong");
    }

    // Helper to install the source application
    FlowSendHelper source(
            "ns3::TcpSocketFactory",
            InetSocketAddress(m_nodes.Get(entry.to_node_id)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), 1024),
            entry.size_byte
    );

    // Install it on the node and start it right now
    ApplicationContainer app = source.Install(m_nodes.Get(entry.from_node_id));
    app.Start(NanoSeconds(0));
    m_apps.push_back(app);

    // If there is a next flow to start, schedule its start
    if (i + 1 != (int) m_schedule.size()) {
        int64_t next_flow_ns = m_schedule[i + 1].start_time_ns;
        Simulator::Schedule(NanoSeconds(next_flow_ns - now_ns), &BasicSimulation::StartNextFlow, this, i + 1);
    }

}

void BasicSimulation::ScheduleApplications() {
    std::cout << "SCHEDULING APPLICATIONS" << std::endl;

    // Install sink on each node
    std::cout << "  > Setting up sinks" << std::endl;
    for (int i = 0; i < m_topology->num_nodes; i++) {
        FlowSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 1024));
        ApplicationContainer app = sink.Install(m_nodes.Get(i));
        app.Start(Seconds(0.0));
    }
    m_timestamps.push_back(std::make_pair("Setup traffic sinks", now_ns_since_epoch()));

    // Setup all source applications
    std::cout << "  > Setting up traffic flow starter" << std::endl;
    if (m_schedule.size() > 0) {
        Simulator::Schedule(NanoSeconds(m_schedule[0].start_time_ns), &BasicSimulation::StartNextFlow, this, 0);
    }

    std::cout << std::endl;
    m_timestamps.push_back(std::make_pair("Setup traffic flow starter", now_ns_since_epoch()));
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

void BasicSimulation::RunSimulation() {
    std::cout << "SIMULATION" << std::endl;

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

    m_timestamps.push_back(std::make_pair("Run simulation", now_ns_since_epoch()));
}

void BasicSimulation::StoreApplicationResults() {
    std::cout << "STORE TRAFFIC RESULTS" << std::endl;

    FILE* file_csv = fopen((m_logs_dir + "/flows.csv").c_str(), "w+");
    FILE* file_txt = fopen((m_logs_dir + "/flows.txt").c_str(), "w+");
    fprintf(
            file_txt, "%-12s%-10s%-10s%-16s%-18s%-18s%-16s%-16s%-13s%-16s%-14s%s\n",
            "Flow ID", "Source", "Target", "Size", "Start time (ns)",
            "End time (ns)", "Duration", "Sent", "Progress", "Avg. rate", "Finished?", "Metadata"
    );
    std::vector<ApplicationContainer>::iterator it = m_apps.begin();
    for (schedule_entry_t& entry : m_schedule) {

        // Retrieve statistics
        ApplicationContainer app = *it;
        Ptr<FlowSendApplication> flowSendApp = ((it->Get(0))->GetObject<FlowSendApplication>());
        bool is_completed = flowSendApp->IsCompleted();
        bool is_conn_failed = flowSendApp->IsConnFailed();
        bool is_closed_err = flowSendApp->IsClosedByError();
        bool is_closed_normal = flowSendApp->IsClosedNormally();
        int64_t sent_byte = flowSendApp->GetAckedBytes();
        int64_t fct_ns;
        if (is_completed) {
            fct_ns = flowSendApp->GetCompletionTimeNs() - entry.start_time_ns;
        } else {
            fct_ns = m_simulation_end_time_ns - entry.start_time_ns;
        }
        std::string finished_state;
        if (is_completed) {
            finished_state = "YES";
        } else if (is_conn_failed) {
            finished_state = "NO_CONN_FAIL";
        } else if (is_closed_normal) {
            finished_state = "NO_BAD_CLOSE";
        } else if (is_closed_err) {
            finished_state = "NO_ERR_CLOSE";
        } else {
            finished_state = "NO_ONGOING";
        }

        // Write plain to the csv
        fprintf(
                file_csv, "%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%s,%s\n",
                entry.flow_id, entry.from_node_id, entry.to_node_id, entry.size_byte, entry.start_time_ns,
                entry.start_time_ns + fct_ns, fct_ns, sent_byte, finished_state.c_str(), entry.metadata.c_str()
        );

        // Write nicely formatted to the text
        char str_size_megabit[100];
        sprintf(str_size_megabit, "%.2f Mbit", byte_to_megabit(entry.size_byte));
        char str_duration_ms[100];
        sprintf(str_duration_ms, "%.2f ms", nanosec_to_millisec(fct_ns));
        char str_sent_megabit[100];
        sprintf(str_sent_megabit, "%.2f Mbit", byte_to_megabit(sent_byte));
        char str_progress_perc[100];
        sprintf(str_progress_perc, "%.1f%%", ((double) sent_byte) / ((double) entry.size_byte) * 100.0);
        char str_avg_rate_megabit_per_s[100];
        sprintf(str_avg_rate_megabit_per_s, "%.1f Mbit/s", byte_to_megabit(sent_byte) / nanosec_to_sec(fct_ns));
        fprintf(
                file_txt, "%-12" PRId64 "%-10" PRId64 "%-10" PRId64 "%-16s%-18" PRId64 "%-18" PRId64 "%-16s%-16s%-13s%-16s%-14s%s\n",
                entry.flow_id, entry.from_node_id, entry.to_node_id, str_size_megabit, entry.start_time_ns,
                entry.start_time_ns + fct_ns, str_duration_ms, str_sent_megabit, str_progress_perc, str_avg_rate_megabit_per_s,
                finished_state.c_str(), entry.metadata.c_str()
        );

        // Move on iterator
        it++;

    }
    fclose(file_csv);
    fclose(file_txt);

    std::cout << "  > Flow log files have been written" << std::endl;

    std::cout << std::endl;
    m_timestamps.push_back(std::make_pair("Write flow log files", now_ns_since_epoch()));
}

void BasicSimulation::CleanUpSimulation() {
    std::cout << "CLEAN-UP" << std::endl;
    Simulator::Destroy();
    std::cout << "  > Simulator is destroyed" << std::endl;
    std::cout << std::endl;
    m_timestamps.push_back(std::make_pair("Destroy simulator", now_ns_since_epoch()));
}

void BasicSimulation::StoreTimingResults() {
    std::cout << "TIMING RESULTS" << std::endl;
    std::cout << "------" << std::endl;

    // Write to both file and out
    std::ofstream fileTimingResults(m_logs_dir + "/timing-results.txt");
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

}
