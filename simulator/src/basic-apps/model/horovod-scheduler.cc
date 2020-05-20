#include "horovod-scheduler.h"
#include "string.h"
#include "horovod-worker.h"
namespace ns3 {

HorovodScheduler::HorovodScheduler(Ptr<BasicSimulation> basicSimulation, Ptr<Topology> topology) {
    m_basicSimulation = basicSimulation;
    m_topology = topology;

    // Properties we will use often
    m_nodes = m_topology->GetNodes();
//     for (int64_t endpoint : m_topology->GetEndpoints()) {
//         std::string ipv4 = "10.0.0." + std::to_string(endpoint);
//         const char* ipv4_add = ipv4.c_str();
//         m_ipv4_addr_self[endpoint] = std::shared_ptr<InetSocketAddress>(new InetSocketAddress(Ipv4Address(ipv4_add), 1024));
//         m_ipv4_addr_self[endpoint] =  InetSocketAddress(m_nodes.Get(entry.to_node_id)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), 1024),
// ;
//     }

//     for (int64_t endpoint : m_topology->GetEndpoints()) {
//         m_ipv4_addr_remote[endpoint] = std::shared_ptr<InetSocketAddress>((endpoint != int64_t(m_topology->GetEndpoints().size()-1))? m_ipv4_addr_self[endpoint+1]: m_ipv4_addr_self[0]);
//     }
    

    // m_simulation_end_time_ns = m_basicSimulation->GetSimulationEndTimeNs();

    // TODO remove or modify? 
    // m_enableFlowLoggingToFileForFlowIds = parse_set_positive_int64(m_basicSimulation->GetConfigParamOrDefault("enable_flow_logging_to_file_for_flow_ids", "set()"));

    printf("HOROVOD SCHEDULE\n");
    // std::cout << std::endl;

}


void HorovodScheduler::Schedule() {
    std::cout << "SCHEDULING HOROVOD" << std::endl;

    // Install sink on each endpoint node
    std::cout << "  > Setting horovodworker" << std::endl;
    HorovodWorkerHelper horovodworker(
            "ns3::TcpSocketFactory",
            InetSocketAddress(Ipv4Address::GetAny(), 1024), 
            InetSocketAddress(m_nodes.Get(1)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), 1024),
            0, //rank 0 
            m_basicSimulation->GetLogsDir()
            
            );
    ApplicationContainer app = horovodworker.Install(m_nodes.Get(0));
    app.Start(Seconds(0)); // *seconds only takes integers!
    HorovodWorkerHelper horovodworker1(
            "ns3::TcpSocketFactory",
            InetSocketAddress(Ipv4Address::GetAny(), 1024), 
            InetSocketAddress(m_nodes.Get(2)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), 1024),
            1, //rank 1
            m_basicSimulation->GetLogsDir()
            
            );
    ApplicationContainer app1 = horovodworker1.Install(m_nodes.Get(1));
    app1.Start(Seconds(0));
    HorovodWorkerHelper horovodworker2(
            "ns3::TcpSocketFactory",
            InetSocketAddress(Ipv4Address::GetAny(), 1024), 
            InetSocketAddress(m_nodes.Get(0)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), 1024),
            2, //rank 0 
            m_basicSimulation->GetLogsDir()
            
            );
    ApplicationContainer app2 = horovodworker2.Install(m_nodes.Get(2));
    app2.Get(0)->GetObject<HorovodWorker>()->SetLeftNeighbor(app1.Get(0)->GetObject<HorovodWorker>());
    app1.Get(0)->GetObject<HorovodWorker>()->SetLeftNeighbor(app.Get(0)->GetObject<HorovodWorker>());
    app.Get(0)->GetObject<HorovodWorker>()->SetLeftNeighbor(app2.Get(0)->GetObject<HorovodWorker>());

    app2.Start(Seconds(0));  

    m_basicSimulation->RegisterTimestamp("Setup horovodworker");
}

/*
void HorovodScheduler::WriteResults() {
    std::cout << "STORE FLOW RESULTS" << std::endl;

    FILE* file_csv = fopen((m_basicSimulation->GetLogsDir() + "/flows.csv").c_str(), "w+");
    FILE* file_txt = fopen((m_basicSimulation->GetLogsDir() + "/flows.txt").c_str(), "w+");
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

    std::cout << "  > horovodworker log files have been written" << std::endl;
    std::cout << std::endl;

    m_basicSimulation->RegisterTimestamp("Write flow log files");
} */

}
