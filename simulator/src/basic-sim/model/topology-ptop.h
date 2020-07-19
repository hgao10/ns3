#ifndef TOPOLOGY_PTOP_H
#define TOPOLOGY_PTOP_H

#include <utility>
#include "ns3/core-module.h"
#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/topology.h"
#include "ns3/exp-util.h"
#include "ns3/basic-simulation.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/command-line.h"
#include "ns3/traffic-control-helper.h"
#include "ns3/callback.h"
#include "ns3/packet.h"
#include "ns3/utilization-tracker.h"


namespace ns3 {
void TcBytessInQueueTrace (uint32_t oldValue, uint32_t newValue);

class MyCallback2: public CallbackImpl<void, ns3::Ptr<ns3::Packet const>, empty,empty,empty,empty,empty,empty,empty,empty>
{
  public:
      MyCallback2(UtilizationTracker * ut, bool next_state_is_on){
        m_ut =  ut;
        m_state = next_state_is_on;
      };
      
      void operator() (ns3::Ptr<ns3::Packet const>) {
        m_ut->TrackUtilization(m_state);
      }      

      bool IsEqual (Ptr<const CallbackImplBase> other) const {
        return false;
      }

  private:
      UtilizationTracker * m_ut;
      bool m_state;
};

class MyCallback: public CallbackImpl<void, uint32_t, uint32_t,empty,empty,empty,empty,empty,empty,empty>
{
  public:
      MyCallback(std::string baselogdir, int64_t node, uint16_t band){
        m_base_logdir = baselogdir;
        queue_node = node;
        queue_band = band; };
      
      void operator() (uint32_t old_value, uint32_t new_value) {
        std::ofstream ofs;
        ofs.open(m_base_logdir + "/" + format_string("Node_%d_queue_band_%u.txt", queue_node, queue_band),
        std::ofstream::out | std::ofstream::app);
        ofs << "TcBytesInQueue " << old_value << " to " << new_value 
                << " "<< Simulator::Now().GetNanoSeconds()<<std::endl;
        
        ofs.close(); 
    
      }      
        bool IsEqual (Ptr<const CallbackImplBase> other) const {
            return false;
        }

  private:
      std::string m_base_logdir;
      int64_t queue_node;
      uint16_t queue_band; 
};

class TopologyPtop : public Topology
{
public:

    // Constructors
    static TypeId GetTypeId (void);
    TopologyPtop(Ptr<BasicSimulation> basicSimulation, const Ipv4RoutingHelper& ipv4RoutingHelper);

    // Accessors
    const NodeContainer& GetNodes();
    int64_t GetNumNodes();
    int64_t GetNumUndirectedEdges();
    const std::set<int64_t>& GetSwitches();
    const std::set<int64_t>& GetSwitchesWhichAreTors();
    const std::set<int64_t>& GetServers();
    bool IsValidEndpoint(int64_t node_id);
    const std::set<int64_t>& GetEndpoints();
    const std::vector<std::pair<int64_t, int64_t>>& GetUndirectedEdges();
    const std::set<std::pair<int64_t, int64_t>>& GetUndirectedEdgesSet();
    const std::vector<std::set<int64_t>>& GetAllAdjacencyLists();
    const std::set<int64_t>& GetAdjacencyList(int64_t node_id);
    int64_t GetWorstCaseRttEstimateNs();
    const std::vector<std::pair<uint32_t, uint32_t>>& GetInterfaceIdxsForEdges();
    void RecordInternalQueues(QueueDiscContainer qdiscs_ptr, int64_t node);
    double GetNumberOfActiveBursts();


private:

    Ptr<BasicSimulation> m_basicSimulation;

    // Construction
    void ReadRelevantConfig();
    void ReadTopology();
    void SetupNodes(const Ipv4RoutingHelper& ipv4RoutingHelper);
    void SetupLinks();

    // Configuration properties
    double m_link_data_rate_megabit_per_s;
    int64_t m_link_delay_ns;
    int64_t m_link_max_queue_size_pkts;
    int64_t m_worst_case_rtt_ns;
    bool m_disable_qdisc_endpoint_tors_xor_servers;
    bool m_disable_qdisc_non_endpoint_switches;
    double m_num_active_bursts;
    // Graph properties
    int64_t m_num_nodes;
    int64_t m_num_undirected_edges;
    std::set<int64_t> m_switches;
    std::set<int64_t> m_switches_which_are_tors;
    std::set<int64_t> m_servers;
    std::vector<std::pair<int64_t, int64_t>> m_undirected_edges;
    std::set<std::pair<int64_t, int64_t>> m_undirected_edges_set;
    std::vector<std::set<int64_t>> m_adjacency_list;
    bool m_has_zero_servers;

    // From generating ns3 objects
    NodeContainer m_nodes;
    std::vector<std::pair<uint32_t, uint32_t>> m_interface_idxs_for_edges;
};

}

#endif //TOPOLOGY_PTOP_H
