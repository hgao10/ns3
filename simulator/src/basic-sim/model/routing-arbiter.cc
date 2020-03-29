#include "ns3/routing-arbiter.h"

using namespace ns3;

RoutingArbiter::RoutingArbiter(Topology* topology, NodeContainer nodes, std::vector<std::pair<int32_t, int32_t>> interface_idxs_for_edges) {
    this->topology = topology;
    this->nodes = nodes;

    // Store IP address to node id (each interface has an IP address, so multiple IPs per node)
    for (uint32_t i = 0; i < topology->num_nodes; i++) {
        for (uint32_t j = 1; j < nodes.Get(i)->GetObject<Ipv4>()->GetNInterfaces(); j++) {
            ip_to_node_id.insert({nodes.Get(i)->GetObject<Ipv4>()->GetAddress(j,0).GetLocal().Get(), i});
        }
    }

    // Save which interface is for which neighbor node id
    int n2 = topology->num_nodes * topology->num_nodes;
    this->neighbor_node_id_to_if_idx = new uint32_t[n2]();
    for (int i = 0; i < topology->num_undirected_edges; i++) {
        std::pair<int64_t, int64_t> edge = topology->undirected_edges[i];
        this->neighbor_node_id_to_if_idx[edge.first * topology->num_nodes + edge.second] = interface_idxs_for_edges[i].first;
        this->neighbor_node_id_to_if_idx[edge.second * topology->num_nodes + edge.first] = interface_idxs_for_edges[i].second;
    }

}

/**
 * Decides what should be the next interface.
 *
 * @param current_node  Current node identifier
 * @param pkt           Packet
 * @param ipHeader      IP header of the packet
 *
 * @return Interface index
 */
int32_t RoutingArbiter::decide_next_interface(int32_t current_node, Ptr<const Packet> pkt, Ipv4Header const &ipHeader) {

    // Ask the routing which node should be next
    uint32_t source_ip = ipHeader.GetSource().Get();
    uint32_t target_ip = ipHeader.GetDestination().Get();
    int32_t selected_node_id = decide_next_node_id(
            current_node,
            ip_to_node_id[source_ip],
            ip_to_node_id[target_ip],
            topology->adjacency_list[current_node],
            pkt,
            ipHeader
    );

    // Invalid selected node id
    if (selected_node_id < 0 || selected_node_id >= topology->num_nodes) {
        throw std::runtime_error(format_string(
                "The selected next node %d is out of node id range of [0, %" PRId64 ").",
                selected_node_id,
                topology->num_nodes
        ));
    }

    // Convert the neighbor node id to the interface index of the edge which connects to it
    uint32_t selected_if_idx = neighbor_node_id_to_if_idx[current_node * topology->num_nodes + selected_node_id];
    if (selected_if_idx == 0) {
        throw std::runtime_error(format_string(
                "The selected next node %d is not a neighbor of node %d.",
                selected_node_id,
                current_node
                ));
    }

    return selected_if_idx;
}
