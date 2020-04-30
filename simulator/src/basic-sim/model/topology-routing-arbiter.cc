#include "ns3/topology-routing-arbiter.h"

using namespace ns3;

TopologyRoutingArbiter::TopologyRoutingArbiter(
        Ptr<Node> this_node,
        NodeContainer nodes,
        Topology* topology,
        const std::vector<std::pair<uint32_t, uint32_t>>& interface_idxs_for_edges
) : RoutingArbiter(this_node, nodes) {

    // Topology
    m_topology = topology;

    // Save which interface is for which neighbor node id
    this->neighbor_node_id_to_if_idx = (uint32_t*) calloc(m_topology->num_nodes * m_topology->num_nodes, sizeof(uint32_t));
    for (int i = 0; i < m_topology->num_undirected_edges; i++) {
        std::pair<int64_t, int64_t> edge = m_topology->undirected_edges[i];
        this->neighbor_node_id_to_if_idx[edge.first * m_topology->num_nodes + edge.second] = interface_idxs_for_edges[i].first;
        this->neighbor_node_id_to_if_idx[edge.second * m_topology->num_nodes + edge.first] = interface_idxs_for_edges[i].second;
    }

}

TopologyRoutingArbiter::~TopologyRoutingArbiter() {
    free(this->neighbor_node_id_to_if_idx);
}

RoutingArbiterResult Decide(
        int32_t source_node_id,
        int32_t target_node_id,
        ns3::Ptr<const ns3::Packet> pkt,
        ns3::Ipv4Header const &ipHeader,
        bool is_socket_request_for_source_ip
) {

    // Decide the next node
    int32_t selected_node_id = TopologyDecide(
                source_node_id,
                target_node_id,
                m_topology->adjacency_list[current_node],
                pkt,
                ipHeader,
                is_request_for_source_ip
    );

    // Invalid selected node id
    if (selected_node_id != -1) {

        // Must be a valid node identifier
        if (selected_node_id < 0 || selected_node_id >= m_topology->num_nodes) {
            throw std::runtime_error(format_string(
                    "The selected next node %d is out of node id range of [0, %"  PRId64 ").", selected_node_id, m_topology->num_nodes
            ));
        }

        // Convert the neighbor node id to the interface index of the edge which connects to it
        uint32_t selected_if_idx = neighbor_node_id_to_if_idx[current_node * m_topology->num_nodes + selected_node_id];
        if (selected_if_idx == 0) {
            throw std::runtime_error(format_string(
                    "The selected next node %d is not a neighbor of node %d.",
                    selected_node_id,
                    current_node
            ));
        }

        // We succeeded in finding the interface to the next hop
        return RoutingArbiterResult(false, selected_if_idx, 0); // Gateway is 0.0.0.0

    } else {
        return RoutingArbiterResult(true, 0, 0); // Failed = no route (means either drop, or socket fails)
    }

}
