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
     
    printf("HOROVOD SCHEDULE\n");
    // std::cout << std::endl;

}


void HorovodScheduler::Schedule(uint32_t port) {
    std::cout << "SCHEDULING HOROVOD" << std::endl;

    bool run_horovod = parse_boolean(m_basicSimulation->GetConfigParamOrDefault("run_horovod","false"));   
    std::string prio_config =  m_basicSimulation->GetConfigParamOrDefault("horovod_initial_priority","0x08");
    // read config
    std::map<std::string, std::string> config =
                read_config(m_basicSimulation->GetRunDir() + "/" +
                m_basicSimulation->GetConfigParamOrFail("horovod_config_file"));
    
    if(run_horovod){
        
        std::string tmp;
        tmp = get_param_or_fail("num_workers", config);
        int64_t num_workers = parse_positive_int64(tmp);

        tmp = get_param_or_fail("num_layers", config);
        int64_t num_layers = parse_positive_int64(tmp);

        tmp = get_param_or_fail("fusion_buffer_size", config);
        int64_t fusion_size_bytes = parse_positive_int64(tmp);

        std::string layer_size_file=get_param_or_fail("layer_size_file", config);
        std::string fp_compute_time_file=get_param_or_fail("fp_compute_time_file", config);
        std::string bp_compute_time_file=get_param_or_fail("bp_compute_time_file", config);
        
    
        std::vector<ApplicationContainer> apps;
        uint8_t prio = uint8_t (std::stoi(prio_config, 0, 16));
        // Install sink on each endpoint node
        std::cout << "  > Setting horovodworker" << std::endl;
        
        InetSocketAddress src_addr = InetSocketAddress(Ipv4Address::GetAny(), port);
        src_addr.SetTos(prio);

        InetSocketAddress dst_addr = InetSocketAddress(Ipv4Address::GetAny(), port);
        for(int i = 0; i < num_workers; i++){
            
            if (i==num_workers-1){
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
            app.Get(0)->GetObject<HorovodWorker>()->SetNumLayers(num_layers);

            // set layer size
            std::string filename = m_basicSimulation->GetRunDir() + "/" + layer_size_file;
            std::map<int, uint64_t> layer_size_bytes = read_layer_size(filename);
            app.Get(0)->GetObject<HorovodWorker>()->SetLayerWeight(layer_size_bytes);

            filename = m_basicSimulation->GetRunDir() + "/" + fp_compute_time_file;
            std::map<int, float> fp_layer_compute_time_ms = read_layer_computation_time(filename);
            app.Get(0)->GetObject<HorovodWorker>()->SetFPComputeTime(fp_layer_compute_time_ms);

            filename = m_basicSimulation->GetRunDir() + "/" + bp_compute_time_file;
            std::map<int, float> bp_layer_compute_time_ms = read_layer_computation_time(filename);
            app.Get(0)->GetObject<HorovodWorker>()->SetBPComputeTime(bp_layer_compute_time_ms);
            
            app.Get(0)->GetObject<HorovodWorker>()->SetNumWorkers(num_workers);

            // set fusion size bytes
            app.Get(0)->GetObject<HorovodWorker>()->SetFusionBufferSize(fusion_size_bytes);
            // set FP and BP computation time

            apps.push_back(app);
        }

        for (int i = 0; i < num_workers; i++){
            if (i==0){
                apps[i].Get(0)->GetObject<HorovodWorker>()->SetLeftNeighbor(apps[num_workers-1].Get(0)->GetObject<HorovodWorker>());
            }
            else{
                apps[i].Get(0)->GetObject<HorovodWorker>()->SetLeftNeighbor(apps[i-1].Get(0)->GetObject<HorovodWorker>());
            }

        }
        
            
        m_basicSimulation->RegisterTimestamp("Setup horovodworker");

        }

    std::cout << std::endl;

}

}
