#include "pingmesh-scheduler.h"

PingmeshScheduler::PingmeshScheduler(BasicSimulation* basicSimulation) {
    m_basicSimulation = basicSimulation;
    m_nodes = basicSimulation->GetNodes();
    m_simulation_end_time_ns = m_basicSimulation->GetSimulationEndTimeNs();
    m_topology = m_basicSimulation->GetTopology();
    m_interval_ns = parse_positive_int64(basicSimulation->GetConfigParamOrFail("pingmesh_interval_ns"));
}

void PingmeshScheduler::Schedule() {
    std::cout << "SCHEDULING PINGMESH" << std::endl;

    // Install echo server on each node
    std::cout << "  > Setting up echo servers" << std::endl;
    for (int i = 0; i < m_topology->num_nodes; i++) {
        UdpRttServerHelper echoServerHelper(1025);
        ApplicationContainer app = echoServerHelper.Install(m_nodes->Get(i));
        app.Start(Seconds(0.0));
    }
    m_basicSimulation->RegisterTimestamp("Setup pingmesh echo servers");

    // Install echo client from each node to each other node
    std::cout << "  > Setting up echo clients" << std::endl;
    for (int i = 0; i < m_topology->num_nodes; i++) {
        for (int j = 0; j < m_topology->num_nodes; j++) {
            if (m_topology->is_valid_endpoint(i) && m_topology->is_valid_endpoint(j) && i != j) {

                // Helper to install the source application
                UdpRttClientHelper source(
                        m_nodes->Get(j)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(),
                        1025,
                        i,
                        j
                );
                source.SetAttribute("Interval", TimeValue(NanoSeconds(m_interval_ns)));

                // Install it on the node and start it right now
                ApplicationContainer app = source.Install(m_nodes->Get(i));
                app.Start(NanoSeconds(0));
                m_apps.push_back(app);

            }
        }
    }

    std::cout << std::endl;
    m_basicSimulation->RegisterTimestamp("Setup pingmesh echo clients");
}

void PingmeshScheduler::WriteResults() {
    std::cout << "STORE PINGMESH RESULTS" << std::endl;

    // Write to CSV and TXT
    FILE* file_csv = fopen((m_basicSimulation->GetLogsDir() + "/pingmesh.csv").c_str(), "w+");
    FILE* file_txt = fopen((m_basicSimulation->GetLogsDir() + "/pingmesh.txt").c_str(), "w+");
    fprintf(file_txt, "%-10s%-10s%-22s%-22s%-16s%-16s%-16s%-16s%s\n",
            "Source", "Target", "Mean latency there", "Mean latency back",
            "Min. RTT", "Mean RTT", "Max. RTT", "Smp.std. RTT", "Reply arrival");
    for (uint32_t i = 0; i < m_apps.size(); i++) {
        Ptr<UdpRttClient> client = m_apps[i].Get(0)->GetObject<UdpRttClient>();

        // Data about this pair
        int64_t from_node_id = client->GetFromNodeId();
        int64_t to_node_id = client->GetToNodeId();
        uint32_t sent = client->GetSent();
        std::vector<int64_t> sendRequestTimestamps = client->GetSendRequestTimestamps();
        std::vector<int64_t> replyTimestamps = client->GetReplyTimestamps();
        std::vector<int64_t> receiveReplyTimestamps = client->GetReceiveReplyTimestamps();

        int total = 0;
        double sum_latency_to_there_ns = 0.0;
        double sum_latency_from_there_ns = 0.0;
        int64_t min_rtt_ns = 100000000000; // 100s
        int64_t max_rtt_ns = -1;
        std::vector<int64_t> rtts_ns;
        for (uint32_t j = 0; j < sent; j++) {

            // Outcome
            bool reply_arrived = replyTimestamps[j] != -1;
            std::string reply_arrived_str = reply_arrived ? "YES" : "LOST";

            // Latencies
            int64_t latency_to_there_ns = reply_arrived ? replyTimestamps[j] - sendRequestTimestamps[j] : -100000000000;
            int64_t latency_from_there_ns = reply_arrived ? receiveReplyTimestamps[j] - replyTimestamps[j] : -100000000000;
            int64_t rtt_ns = reply_arrived ? latency_to_there_ns + latency_from_there_ns : -100000000000;

            // Write plain to the csv
            fprintf(
                    file_csv,
                    "%" PRId64 ",%" PRId64 ",%u,%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%s\n",
                    from_node_id, to_node_id, j, sendRequestTimestamps[j], replyTimestamps[j], receiveReplyTimestamps[j],
                    latency_to_there_ns, latency_from_there_ns, rtt_ns, reply_arrived_str.c_str()
            );

            // Add to statistics
            if (reply_arrived) {
                total++;
                sum_latency_to_there_ns += latency_to_there_ns;
                sum_latency_from_there_ns += latency_from_there_ns;
                min_rtt_ns = std::min(min_rtt_ns, rtt_ns);
                max_rtt_ns = std::max(max_rtt_ns, rtt_ns);
                rtts_ns.push_back(rtt_ns);
            }

        }

        // Calculate the sample standard deviation
        double mean_rtt_ns = (sum_latency_to_there_ns + sum_latency_from_there_ns) / total;
        double sum_sq = 0.0;
        for (uint32_t j = 0; j < rtts_ns.size(); j++) {
            sum_sq += std::pow(rtts_ns[j] - mean_rtt_ns, 2);
        }
        double sample_std_rtt_ns = rtts_ns.size() > 1 ? std::sqrt((1.0 / (rtts_ns.size() - 1)) *  sum_sq) : 0.0;

        // Write nicely formatted to the text
        char str_latency_to_there_ms[100];
        sprintf(str_latency_to_there_ms, "%.2f ms", nanosec_to_millisec(sum_latency_to_there_ns / total));
        char str_latency_from_there_ms[100];
        sprintf(str_latency_from_there_ms, "%.2f ms", nanosec_to_millisec(sum_latency_from_there_ns / total));
        char str_min_rtt_ms[100];
        sprintf(str_min_rtt_ms, "%.2f ms", nanosec_to_millisec(min_rtt_ns));
        char str_mean_rtt_ms[100];
        sprintf(str_mean_rtt_ms, "%.2f ms", nanosec_to_millisec(mean_rtt_ns));
        char str_max_rtt_ms[100];
        sprintf(str_max_rtt_ms, "%.2f ms", nanosec_to_millisec(max_rtt_ns));
        char str_sample_std_rtt_ms[100];
        sprintf(str_sample_std_rtt_ms, "%.2f ms", nanosec_to_millisec(sample_std_rtt_ns));
        fprintf(
                file_txt, "%-10" PRId64 "%-10" PRId64 "%-22s%-22s%-16s%-16s%-16s%-16s%d/%d (%.0f%%)\n",
                from_node_id, to_node_id, str_latency_to_there_ms, str_latency_from_there_ms, str_min_rtt_ms, str_mean_rtt_ms, str_max_rtt_ms, str_sample_std_rtt_ms, total, sent, ((double) total / (double) sent) * 100.0
        );

    }
    fclose(file_csv);
    fclose(file_txt);

    std::cout << "  > Pingmesh log files have been written" << std::endl;

    std::cout << std::endl;
    m_basicSimulation->RegisterTimestamp("Write pingmesh log files");

}
