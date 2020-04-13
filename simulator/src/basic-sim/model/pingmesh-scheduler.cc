#include "pingmesh-scheduler.h"

PingmeshScheduler::PingmeshScheduler(BasicSimulation* basicSimulation) {
    m_basicSimulation = basicSimulation;
    m_nodes = basicSimulation->GetNodes();
    m_simulation_end_time_ns = m_basicSimulation->GetSimulationEndTimeNs();
    m_topology = m_basicSimulation->GetTopology();
}

PingmeshScheduler::~PingmeshScheduler() {
    for (RttPingTracer* tracer : m_rttPingTracers) {
        delete tracer;
    }
    m_rttPingTracers.clear();
}

void PingmeshScheduler::Schedule() {
    std::cout << "SCHEDULING PINGMESH" << std::endl;

    // Install echo server on each node
    std::cout << "  > Setting up echo servers" << std::endl;
    for (int i = 0; i < m_topology->num_nodes; i++) {
        UdpEchoServerHelper echoServerHelper(1025);
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
                UdpEchoClientHelper source(
                        m_nodes->Get(j)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(),
                        1025
                );
                source.SetAttribute("MaxPackets", UintegerValue(1));
                source.SetAttribute("PacketSize", UintegerValue(0));

                // Install it on the node and start it right now
                ApplicationContainer app = source.Install(m_nodes->Get(i));
                RttPingTracer* tracer = new RttPingTracer(i, j);
                m_rttPingTracers.push_back(tracer);
                app.Get(0)->GetObject<UdpEchoClient>()->TraceConnectWithoutContext("Rx", MakeCallback(&RttPingTracer::ReceivePacket, tracer));
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
    fprintf(file_txt, "%-10s%-10s%s\n", "Source", "Target", "RTT");
    for (RttPingTracer* tracer : m_rttPingTracers) {

        // Check that exactly one ping has been completed
        if (tracer->GetRtts().size() != 1) {
            throw std::runtime_error("One of the pings has been lost.");
        }

        // Retrieve statistics
        int64_t from_node_id = tracer->GetFrom();
        int64_t to_node_id = tracer->GetTo();
        int64_t rtt_ns = tracer->GetRtts()[0];

        // Write plain to the csv
        fprintf(
                file_csv,
                "%" PRId64 ",%" PRId64 ",%" PRId64 "\n",
                from_node_id, to_node_id, rtt_ns
        );

        // Write nicely formatted to the text
        char str_rtt_ms[100];
        sprintf(str_rtt_ms, "%.2f ms", nanosec_to_millisec(rtt_ns));
        fprintf(
                file_txt, "%-10" PRId64 "%-10" PRId64 "%s\n",
                from_node_id, to_node_id, str_rtt_ms
        );

    }
    fclose(file_csv);
    fclose(file_txt);

    std::cout << "  > Pingmesh log files have been written" << std::endl;

    std::cout << std::endl;
    m_basicSimulation->RegisterTimestamp("Write pingmesh log files");

}
