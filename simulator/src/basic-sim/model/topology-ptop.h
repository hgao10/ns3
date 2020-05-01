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

namespace ns3 {

class TopologyPtop : public Topology
{
public:

    static TypeId GetTypeId (void);
    TopologyPtop(Ptr<BasicSimulation> basicSimulation, const Ipv4RoutingHelper& ipv4RoutingHelper);
    void ReadRelevantConfig();
    void ReadTopology();
    void SetupNodes(const Ipv4RoutingHelper& ipv4RoutingHelper);
    void SetupLinks();
    NodeContainer GetNodes();
    bool IsValidEndpoint(int64_t node_id);
    std::set<int64_t> GetEndpoints();
    int64_t GetWorstCaseRttEstimateNs();
    std::vector<std::pair<uint32_t, uint32_t>>* GetInterfaceIdxsForEdges();

    // Graph properties
    int64_t num_nodes; // TODO: Move a bunch of these into private
    int64_t num_undirected_edges;
    std::set<int64_t> switches;
    std::set<int64_t> switches_which_are_tors;
    std::set<int64_t> servers;
    std::vector<std::pair<int64_t, int64_t>> undirected_edges;
    std::set<std::pair<int64_t, int64_t>> undirected_edges_set;
    std::vector<std::set<int64_t>> adjacency_list;
    bool has_zero_servers;

private:

    Ptr<BasicSimulation> m_basicSimulation;

    // Configuration properties
    double m_link_data_rate_megabit_per_s;
    int64_t m_link_delay_ns;
    int64_t m_link_max_queue_size_pkts;
    int64_t m_worst_case_rtt_ns;
    bool m_disable_qdisc_endpoint_tors_xor_servers;
    bool m_disable_qdisc_non_endpoint_switches;



    // From generating ns3 objects
    NodeContainer m_nodes;
    std::vector<std::pair<uint32_t, uint32_t>> m_interface_idxs_for_edges;

};

}

#endif //TOPOLOGY_PTOP_H
