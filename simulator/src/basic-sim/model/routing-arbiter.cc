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
    this->neighbor_node_id_to_if_idx = (uint32_t*) calloc(topology->num_nodes * topology->num_nodes, sizeof(uint32_t));
    for (int i = 0; i < topology->num_undirected_edges; i++) {
        std::pair<int64_t, int64_t> edge = topology->undirected_edges[i];
        this->neighbor_node_id_to_if_idx[edge.first * topology->num_nodes + edge.second] = interface_idxs_for_edges[i].first;
        this->neighbor_node_id_to_if_idx[edge.second * topology->num_nodes + edge.first] = interface_idxs_for_edges[i].second;
    }
    base_init_finish_ns_since_epoch = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

}

RoutingArbiter::~RoutingArbiter() {
    free(this->neighbor_node_id_to_if_idx);
}

int64_t RoutingArbiter::get_base_init_finish_ns_since_epoch() {
    return base_init_finish_ns_since_epoch;
}

uint32_t RoutingArbiter::resolve_node_id_from_ip(uint32_t ip) {
    ip_to_node_id_it = ip_to_node_id.find(ip);
    if (ip_to_node_id_it != ip_to_node_id.end()) {
        return ip_to_node_id_it->second;
    } else {
        throw std::invalid_argument(format_string("IP address %u is not mapped to a node id", ip));
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

    // Retrieve the source node id
    uint32_t source_ip = ipHeader.GetSource().Get();
    uint32_t source_node_id;

    // Ipv4Address default constructor has IP 0x66666666 = 102.102.102.102 = 1717986918,
    // which is set by TcpSocketBase::SetupEndpoint to discover its actually source IP.
    // In this case, the source node id is just the current node.
    if (source_ip == 1717986918) {
        source_node_id = current_node;
    } else {
        source_node_id = resolve_node_id_from_ip(source_ip);
    }
    uint32_t target_node_id = resolve_node_id_from_ip(ipHeader.GetDestination().Get());

    // Ask the routing which node should be next
    int32_t selected_node_id = decide_next_node_id(
            current_node,
            source_node_id,
            target_node_id,
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
