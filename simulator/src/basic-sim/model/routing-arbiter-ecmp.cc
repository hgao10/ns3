#include "routing-arbiter-ecmp.h"

using namespace ns3;

RoutingArbiterEcmp::RoutingArbiterEcmp(
        Topology* topology,
        NodeContainer nodes,
        std::vector<std::pair<int32_t, int32_t>> interface_idxs_for_edges
        )
        : RoutingArbiter(topology, nodes, interface_idxs_for_edges
        ) {

    int64_t n = topology->num_nodes;

    // ECMP candidate list
    for (int i = 0; i < topology->num_nodes; i++) {
        std::vector<std::vector<int64_t>> v;
        for (int j = 0; j < topology->num_nodes; j++) {
            v.push_back(std::vector<int64_t>());
        }
        candidate_list.push_back(v);
    }

    // Initialize with 0 distance to itself, and infinite distance to all others
    int n2 = n * n;
    int64_t* dist = new int64_t[n2];
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i == j) {
                dist[n * i + j] = 0;
            } else {
                dist[n * i + j] = 100000000;
            }
        }
    }

    // If there is an edge, the distance is 1
    for (std::pair<int64_t, int64_t> edge : topology->undirected_edges) {
        dist[n * edge.first + edge.second] = 1;
        dist[n * edge.second + edge.first] = 1;
    }

    // Floyd-Warshall
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (dist[n * i + j] > dist[n * i + k] + dist[n * k + j]) {
                    dist[n * i + j] = dist[n * i + k] + dist[n * k + j];
                }
            }
        }
    }

    // Now extract the choices
    for (std::pair<int64_t, int64_t> edge : topology->undirected_edges) {
        for (int j = 0; j < n; j++) {
            if (dist[edge.first * n + j] - 1 == dist[edge.second * n + j]) {
                candidate_list[edge.first][j].push_back(edge.second);
            }
            if (dist[edge.second * n + j] - 1 == dist[edge.first * n + j]) {
                candidate_list[edge.second][j].push_back(edge.first);
            }
        }
    }

}

/**
 * Calculates a hash from the 5-tuple.
 *
 * It uses separators of | between the 5 values such that a coincidental
 * concatenation does not lead to equal input between values.
 *
 * Adapted from: https://github.com/mkheirkhah/ecmp (February 20th, 2020)
 *
 * @param header IPv4 header
 * @param p      Packet
 *
 * @return Hash value
 */
uint64_t
RoutingArbiterEcmp::ComputeFiveTupleHash(const Ipv4Header &header, Ptr<const Packet> p, int32_t node_id)
{
    std::ostringstream oss;
    oss << node_id << "|";  // Each switch has a unique identifier.
    // This added to the hash input to make sure that each node make a different decision
    // with the same 5-tuple. This prevents hash polarization.

    oss << header.GetSource() << "|"       // Source IP
        << header.GetDestination() << "|"  // Destination IP
        << ((int) header.GetProtocol());   // Protocol

    switch (header.GetProtocol()) {
        case UDP_PROT_NUMBER: {
            UdpHeader udpHeader;
            p->PeekHeader(udpHeader);
            oss << "|" << udpHeader.GetSourcePort()        // UDP source port
                << "|" << udpHeader.GetDestinationPort();  // UDP destination port
            break;
        }
        case TCP_PROT_NUMBER: {
            TcpHeader tcpHeader;
            p->PeekHeader(tcpHeader);
            oss << "|" << tcpHeader.GetSourcePort()        // TCP source port
                << "|" << tcpHeader.GetDestinationPort();  // TCP destination port
            break;
        }
        default: {
            break;
        }
    }

    std::string data = oss.str();
    hasher.clear();
    uint32_t hash = hasher.GetHash32(data);
    oss.str("");
    return hash;
}

int32_t RoutingArbiterEcmp::decide_next_node_id(int32_t current_node_id, int32_t source_node_id, int32_t target_node_id, std::set<int64_t>& neighbor_node_ids, Ptr<const Packet> pkt, Ipv4Header const &ipHeader) {
    uint64_t hash = ComputeFiveTupleHash(ipHeader, pkt, current_node_id);
    int s = candidate_list[current_node_id][target_node_id].size();
    return candidate_list[current_node_id][target_node_id][hash % s];
}
