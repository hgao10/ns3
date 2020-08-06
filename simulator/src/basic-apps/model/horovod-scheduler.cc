#include "horovod-scheduler.h"
#include "horovod-worker.h"
// #include "ringallreduce-syncer.h"
#include "string.h"
// #include "horovod-worker.h"
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
    int64_t num_workers = parse_positive_int64(m_basicSimulation->GetConfigParamOrDefault("num_horovod_workers", "3"));
    if(run_horovod){
        
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
                    port, 
                    num_workers    
                    );

            ApplicationContainer app = horovodworker.Install(m_nodes.Get(i));
            app.Start(Seconds(0)); // *seconds only takes integers!
            app.Get(0)->GetObject<HorovodWorker>()->SetGlobalRingallreduceSyncer(&m_global_ringallreduce_syncer);
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
        
        // std::cout<<"hw 0 local: "<<InetSocketAddress(Ipv4Address::GetAny(), 1024).GetIpv4()<<std::endl; 
        // std::cout<<"hw 1 local: "<<InetSocketAddress(Ipv4Address::GetAny(), 1024).GetIpv4()<<std::endl; 
        // std::cout<<"hw 2 local: "<<InetSocketAddress(Ipv4Address::GetAny(), 1024).GetIpv4()<<std::endl; 

        // std::cout << "hw 0 peer: "<< InetSocketAddress(m_nodes.Get(1)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), 1024).GetIpv4()<<std::endl;
        // std::cout << "hw 1 peer: "<< InetSocketAddress(m_nodes.Get(2)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), 1024).GetIpv4()<<std::endl;
        // std::cout << "hw 2 peer: "<< InetSocketAddress(m_nodes.Get(0)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), 1024).GetIpv4()<<std::endl;

        // dst_addr = InetSocketAddress(m_nodes.Get(2)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), port);
        // dst_addr.SetTos(prio);
        // HorovodWorkerHelper horovodworker1(
        //         "ns3::TcpSocketFactory",
        //         // InetSocketAddress(Ipv4Address::GetAny(), port), 
        //         src_addr,
        //         dst_addr,
        //         1, //rank 1
        //         m_basicSimulation->GetLogsDir(), 
        //         prio,
        //         port
                
        //         );
        // ApplicationContainer app1 = horovodworker1.Install(m_nodes.Get(1));
        // app1.Start(Seconds(0));
        // dst_addr = InetSocketAddress(m_nodes.Get(0)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), port);
        // dst_addr.SetTos(prio); 
        // HorovodWorkerHelper horovodworker2(
        //         "ns3::TcpSocketFactory",
        //         // InetSocketAddress(Ipv4Address::GetAny(), port), 
        //         src_addr,
        //         dst_addr,
        //         2, //rank 0 
        //         m_basicSimulation->GetLogsDir(),
        //         prio,
        //         port
                
        //         );
        // ApplicationContainer app2 = horovodworker2.Install(m_nodes.Get(2));
        // app2.Start(Seconds(0));  
        
        // app2.Get(0)->GetObject<HorovodWorker>()->SetLeftNeighbor(app1.Get(0)->GetObject<HorovodWorker>());
        // app1.Get(0)->GetObject<HorovodWorker>()->SetLeftNeighbor(app.Get(0)->GetObject<HorovodWorker>());
        // app.Get(0)->GetObject<HorovodWorker>()->SetLeftNeighbor(app2.Get(0)->GetObject<HorovodWorker>());

        // // key: ringallreduce priority, value: number of workers that have all partitions updated
        // app.Get(0)->GetObject<HorovodWorker>()->SetGlobalRingallreduceSyncer(&m_global_ringallreduce_syncer);
        // app1.Get(0)->GetObject<HorovodWorker>()->SetGlobalRingallreduceSyncer(&m_global_ringallreduce_syncer);
        // app2.Get(0)->GetObject<HorovodWorker>()->SetGlobalRingallreduceSyncer(&m_global_ringallreduce_syncer);
            
        m_basicSimulation->RegisterTimestamp("Setup horovodworker");

        }

    std::cout << std::endl;

}

}
