#include "horovod-scheduler.h"
#include "horovod-worker.h"
#include "string.h"
#include "horovod-worker-config-reader.h"

namespace ns3 {

HorovodScheduler::HorovodScheduler(Ptr<BasicSimulation> basicSimulation, Ptr<Topology> topology) {
    m_basicSimulation = basicSimulation;
    m_topology = topology;

    // Properties we will use often
    m_nodes = m_topology->GetNodes();
     
    m_run_horovod = parse_boolean(m_basicSimulation->GetConfigParamOrDefault("run_horovod","false"));   
    m_prio_config =  m_basicSimulation->GetConfigParamOrDefault("horovod_initial_priority","0x08");
    // read config
    m_config = read_config(m_basicSimulation->GetRunDir() + "/" +
                m_basicSimulation->GetConfigParamOrFail("horovod_config_file"));

    std::cout<<"Run horovod: "<<m_run_horovod<<std::endl;

    if(m_run_horovod) {
        std::string tmp;
        tmp = get_param_or_fail("num_workers", m_config);
        m_num_workers = parse_positive_int64(tmp);
        std::cout<<" > m_num_workers "<<m_num_workers<<std::endl;
        tmp = get_param_or_fail("num_layers", m_config);
        m_num_layers = parse_positive_int64(tmp);
        std::cout<<" > m_num_workers "<<m_num_layers<<std::endl;

        tmp = get_param_or_fail("fusion_buffer_size", m_config);
        m_fusion_size_bytes = parse_positive_int64(tmp);

        m_layer_size_file=get_param_or_fail("layer_size_file", m_config);
        m_fp_compute_time_file=get_param_or_fail("fp_compute_time_file", m_config);
        m_bp_compute_time_file=get_param_or_fail("bp_compute_time_file", m_config);

    }
    printf("HOROVOD SCHEDULE\n");
    // std::cout << std::endl;

}


void HorovodScheduler::Schedule(uint32_t port) {
    std::cout << "SCHEDULING HOROVOD" << std::endl;

    m_port = port;
    if(m_run_horovod){
        
        // std::vector<ApplicationContainer> apps;
        uint8_t prio = uint8_t (std::stoi(m_prio_config, 0, 16));
        // Install sink on each endpoint node
        std::cout << "  > Setting horovodworker" << std::endl;
        
        InetSocketAddress src_addr = InetSocketAddress(Ipv4Address::GetAny(), port);
        src_addr.SetTos(prio);

        InetSocketAddress dst_addr = InetSocketAddress(Ipv4Address::GetAny(), port);
        for(int i = 0; i < m_num_workers; i++){
            
            if (i==m_num_workers-1){
                // last worker in the ring is connected with the worker with rank 0
                dst_addr = InetSocketAddress(m_nodes.Get(0)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), port);
            }
            else{
                // destination is the worker to the right with index i+1
                dst_addr = InetSocketAddress(m_nodes.Get(i+1)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), port);
            }

            dst_addr.SetTos(prio);
            HorovodWorkerHelper horovodworker(
                    "ns3::TcpSocketFactory",
                    src_addr,
                    dst_addr,
                    i, //rank 0 
                    m_basicSimulation->GetLogsDir(),
                    prio,
                    port 
                    );

            ApplicationContainer app = horovodworker.Install(m_nodes.Get(i));
            app.Start(Seconds(0)); // *seconds only takes integers!
            app.Get(0)->GetObject<HorovodWorker>()->SetGlobalRingallreduceSyncer(&m_global_ringallreduce_syncer);

            // set num_layers
            app.Get(0)->GetObject<HorovodWorker>()->SetNumLayers(m_num_layers);

            // set layer size
            std::string filename = m_basicSimulation->GetRunDir() + "/" + m_layer_size_file;
            std::map<int, uint64_t> layer_size_bytes = read_layer_size(filename);
            app.Get(0)->GetObject<HorovodWorker>()->SetLayerWeight(layer_size_bytes);

            filename = m_basicSimulation->GetRunDir() + "/" + m_fp_compute_time_file;
            std::map<int, float> fp_layer_compute_time_ms = read_layer_computation_time(filename);
            app.Get(0)->GetObject<HorovodWorker>()->SetFPComputeTime(fp_layer_compute_time_ms);

            filename = m_basicSimulation->GetRunDir() + "/" + m_bp_compute_time_file;
            std::map<int, float> bp_layer_compute_time_ms = read_layer_computation_time(filename);
            app.Get(0)->GetObject<HorovodWorker>()->SetBPComputeTime(bp_layer_compute_time_ms);
            
            app.Get(0)->GetObject<HorovodWorker>()->SetNumWorkers(m_num_workers);

            // set fusion size bytes
            app.Get(0)->GetObject<HorovodWorker>()->SetFusionBufferSize(m_fusion_size_bytes);
            // set FP and BP computation time

            m_apps.push_back(app);
        }

        for (int i = 0; i < m_num_workers; i++){
            if (i==0){
                m_apps[i].Get(0)->GetObject<HorovodWorker>()->SetLeftNeighbor(m_apps[m_num_workers-1].Get(0)->GetObject<HorovodWorker>());
            }
            else{
                m_apps[i].Get(0)->GetObject<HorovodWorker>()->SetLeftNeighbor(m_apps[i-1].Get(0)->GetObject<HorovodWorker>());
            }

        }
            
        m_basicSimulation->RegisterTimestamp("Setup horovodworker");

        }
    std::cout << std::endl;

}


void HorovodScheduler::WriteResults() {
    std::cout<< "STORE HOROVOD RESULTS" << std::endl;
    std::cout<<"m_run_horovod: "<<m_run_horovod<<std::endl;
    if(m_run_horovod){
        std::cout<<"    > m_num_workers "<<m_num_workers<<std::endl;
        for (int i = 0; i < m_num_workers; i++){
            std::vector<std::string> events = m_apps[i].Get(0)->GetObject<HorovodWorker>()->GetEventString();
            // todo: compare performance vs FILE and fprintf
            std::ofstream ofs;
            std::string progress_file = format_string("HorovodWorker_%" PRIu32 "_layer_%" PRIu32 "_port_%u_progress.txt", i, m_num_layers, m_port);
            std::cout<<"write to logfile: "<<m_basicSimulation->GetLogsDir()<<"/"<<progress_file<<std::endl;
            ofs.open(m_basicSimulation->GetLogsDir() + "/" + format_string("HorovodWorker_%" PRIu32 "_layer_%" PRIu32 "_port_%u_progress.txt", i, m_num_layers, m_port),
            std::ofstream::out | std::ofstream::app);
            for (std::vector<std::string>::iterator it = events.begin(); it != events.end(); it++) {
                
                ofs << *it;
            }
            ofs.close(); 
        }
    }
}
}
