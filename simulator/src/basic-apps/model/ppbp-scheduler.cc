#include "ppbp-scheduler.h"

namespace ns3 {

PPBPScheduler::PPBPScheduler(Ptr<BasicSimulation> basicSimulation,
                             Ptr<Topology> topology) {
  m_basicSimulation = basicSimulation;
  m_topology = topology;

  // Properties we will use often
  m_nodes = m_topology->GetNodes();
  m_simulation_end_time_ns = m_basicSimulation->GetSimulationEndTimeNs();
//   m_enableFlowLoggingToFileForFlowIds =
//       parse_set_positive_int64(m_basicSimulation->GetConfigParamOrDefault(
//           "enable_flow_logging_to_file_for_flow_ids", "set()"));


  std::cout << std::endl;
}

void PPBPScheduler::Schedule(uint32_t port, uint8_t prio, DoubleValue mburst_arr, DoubleValue mburst_timelt){
    std::cout << "SCHEDULING PPBP" << std::endl;

    // Install sink on each endpoint node
    std::cout << "  > Setting horovodworker" << std::endl;
    
    InetSocketAddress dst_addr = InetSocketAddress(m_nodes.Get(1)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), port);
    dst_addr.SetTos(prio);
    InetSocketAddress src_addr = InetSocketAddress(Ipv4Address::GetAny(), port);
    src_addr.SetTos(prio);

    PPBPHelper ppbp0 = PPBPHelper("ns3::TcpSocketFactory", 
                                    dst_addr, 
                                    src_addr,
                                    m_basicSimulation->GetLogsDir(),
                                    0,
                                    mburst_arr,
                                    mburst_timelt
                                    );

    ApplicationContainer app = ppbp0.Install(m_nodes.Get(0));
    app.Start(Seconds(0)); // *seconds only takes integers!

    dst_addr = InetSocketAddress(m_nodes.Get(2)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), port);
    dst_addr.SetTos(prio);

    PPBPHelper ppbp1 = PPBPHelper("ns3::TcpSocketFactory", 
                                    dst_addr, 
                                    src_addr,
                                    m_basicSimulation->GetLogsDir(),
                                    1,
                                    mburst_arr,
                                    mburst_timelt);

    ApplicationContainer app1 = ppbp1.Install(m_nodes.Get(1));
    app1.Start(Seconds(0));

    dst_addr = InetSocketAddress(m_nodes.Get(0)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), port);
    dst_addr.SetTos(prio); 

    PPBPHelper ppbp2 = PPBPHelper("ns3::TcpSocketFactory", 
                                    dst_addr, 
                                    src_addr,
                                    m_basicSimulation->GetLogsDir(),
                                    2,
                                    mburst_arr,
                                    mburst_timelt);
    ApplicationContainer app2 = ppbp2.Install(m_nodes.Get(2));
    app2.Start(Seconds(0));  
            
    m_basicSimulation->RegisterTimestamp("Setup ppbp");
}




}  // namespace ns3
