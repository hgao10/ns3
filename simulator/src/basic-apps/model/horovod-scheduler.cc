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


void HorovodScheduler::Schedule(uint32_t port, uint8_t prio) {
    std::cout << "SCHEDULING HOROVOD" << std::endl;

    // Install sink on each endpoint node
    std::cout << "  > Setting horovodworker" << std::endl;
    
    HorovodWorkerHelper horovodworker(
            "ns3::TcpSocketFactory",
            InetSocketAddress(Ipv4Address::GetAny(), port), 
            InetSocketAddress(m_nodes.Get(1)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), port),
            0, //rank 0 
            m_basicSimulation->GetLogsDir(),
            prio
            
            );
    
    // std::cout<<"hw 0 local: "<<InetSocketAddress(Ipv4Address::GetAny(), 1024).GetIpv4()<<std::endl; 
    // std::cout<<"hw 1 local: "<<InetSocketAddress(Ipv4Address::GetAny(), 1024).GetIpv4()<<std::endl; 
    // std::cout<<"hw 2 local: "<<InetSocketAddress(Ipv4Address::GetAny(), 1024).GetIpv4()<<std::endl; 

    // std::cout << "hw 0 peer: "<< InetSocketAddress(m_nodes.Get(1)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), 1024).GetIpv4()<<std::endl;
    // std::cout << "hw 1 peer: "<< InetSocketAddress(m_nodes.Get(2)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), 1024).GetIpv4()<<std::endl;
    // std::cout << "hw 2 peer: "<< InetSocketAddress(m_nodes.Get(0)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), 1024).GetIpv4()<<std::endl;
    ApplicationContainer app = horovodworker.Install(m_nodes.Get(0));
    app.Start(Seconds(0)); // *seconds only takes integers!
    HorovodWorkerHelper horovodworker1(
            "ns3::TcpSocketFactory",
            InetSocketAddress(Ipv4Address::GetAny(), port), 
            InetSocketAddress(m_nodes.Get(2)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), port),
            1, //rank 1
            m_basicSimulation->GetLogsDir(), 
            prio
            
            );
    ApplicationContainer app1 = horovodworker1.Install(m_nodes.Get(1));
    app1.Start(Seconds(0));
    HorovodWorkerHelper horovodworker2(
            "ns3::TcpSocketFactory",
            InetSocketAddress(Ipv4Address::GetAny(), port), 
            InetSocketAddress(m_nodes.Get(0)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), port),
            2, //rank 0 
            m_basicSimulation->GetLogsDir(),
            prio
            
            );
    ApplicationContainer app2 = horovodworker2.Install(m_nodes.Get(2));
    app2.Start(Seconds(0));  
    
    app2.Get(0)->GetObject<HorovodWorker>()->SetLeftNeighbor(app1.Get(0)->GetObject<HorovodWorker>());
    app1.Get(0)->GetObject<HorovodWorker>()->SetLeftNeighbor(app.Get(0)->GetObject<HorovodWorker>());
    app.Get(0)->GetObject<HorovodWorker>()->SetLeftNeighbor(app2.Get(0)->GetObject<HorovodWorker>());

    // key: ringallreduce priority, value: number of workers that have all partitions updated
    app.Get(0)->GetObject<HorovodWorker>()->SetGlobalRingallreduceSyncer(&m_global_ringallreduce_syncer);
    app1.Get(0)->GetObject<HorovodWorker>()->SetGlobalRingallreduceSyncer(&m_global_ringallreduce_syncer);
    app2.Get(0)->GetObject<HorovodWorker>()->SetGlobalRingallreduceSyncer(&m_global_ringallreduce_syncer);
        
    m_basicSimulation->RegisterTimestamp("Setup horovodworker");
}

}
