#include "routing-arbiter-ecmp.h"

using namespace ns3;

RoutingArbiterEcmp::RoutingArbiterEcmp(
        Topology* topology,
        NodeContainer nodes,
        const std::vector<std::pair<uint32_t, uint32_t>>& interface_idxs_for_edges
        )
        : RoutingArbiter(topology, nodes, interface_idxs_for_edges
        ) {

    ///////////////////////////
    // Floyd-Warshall

    int64_t n = topology->num_nodes;

    // Enforce that more than 40000 nodes is not permitted (sqrt(2^31) ~= 46340, so let's call it an even 40000)
    if (n > 40000) {
        throw std::runtime_error("Cannot handle more than 40000 nodes");
    }

    // Initialize with 0 distance to itself, and infinite distance to all others
    int32_t* dist = new int32_t[n * n];
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

    // Floyd-Warshall core
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (dist[n * i + j] > dist[n * i + k] + dist[n * k + j]) {
                    dist[n * i + j] = dist[n * i + k] + dist[n * k + j];
                }
            }
        }
    }

    ///////////////////////////
    // Determine from the shortest path distances
    // the possible next hops

    // ECMP candidate list: candidate_list[current][destination] = [ list of next hops ]
    candidate_list.reserve(topology->num_nodes);
    for (int i = 0; i < topology->num_nodes; i++) {
        std::vector<std::vector<uint32_t>> v;
        v.reserve(topology->num_nodes);
        for (int j = 0; j < topology->num_nodes; j++) {
            v.push_back(std::vector<uint32_t>());
        }
        candidate_list.push_back(v);
    }

    // Candidate next hops are determined in the following way:
    // For each edge a -> b, for a destination t:
    // If the shortest_path_distance(b, t) == shortest_path_distance(a, t) - 1
    // then a -> b must be part of a shortest path from a towards t.
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

    // Free up the distance matrix
    delete[] dist;

    ecmp_init_finish_ns_since_epoch = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

}

/**
 * Calculates a hash from the 5-tuple.
 *
 * Inspired by: https://github.com/mkheirkhah/ecmp (February 20th, 2020)
 *
 * @param header IPv4 header
 * @param p      Packet
 *
 * @return Hash value
 */
uint64_t
RoutingArbiterEcmp::ComputeFiveTupleHash(const Ipv4Header &header, Ptr<const Packet> p, int32_t node_id)
{
    std::memcpy(&hash_input_buff[0], &node_id, 4);
    
    // Source IP address
    uint32_t src_ip = header.GetSource().Get();
    std::memcpy(&hash_input_buff[4], &src_ip, 4);
    
    // Destination IP address
    uint32_t dst_ip = header.GetDestination().Get();
    std::memcpy(&hash_input_buff[8], &dst_ip, 4);
    
    // Protocol
    uint8_t protocol = header.GetProtocol();
    std::memcpy(&hash_input_buff[12], &protocol, 1);
    
    // If there are ports, add them to the input
    switch (protocol) {
        case UDP_PROT_NUMBER: {
            UdpHeader udpHeader;
            p->PeekHeader(udpHeader);
            uint16_t src_port = udpHeader.GetSourcePort();
            uint16_t dst_port = udpHeader.GetDestinationPort();
            std::memcpy(&hash_input_buff[13], &src_port, 2);
            std::memcpy(&hash_input_buff[15], &dst_port, 2);
            break;
        }
        case TCP_PROT_NUMBER: {
            TcpHeader tcpHeader;
            p->PeekHeader(tcpHeader);
            uint16_t src_port = tcpHeader.GetSourcePort();
            uint16_t dst_port = tcpHeader.GetDestinationPort();
            std::memcpy(&hash_input_buff[13], &src_port, 2);
            std::memcpy(&hash_input_buff[15], &dst_port, 2);
            break;
        }
        default: {
            hash_input_buff[13] = 0;
            hash_input_buff[14] = 0;
            hash_input_buff[15] = 0;
            hash_input_buff[16] = 0;
            break;
        }
    }
    hasher.clear();
    return hasher.GetHash32(hash_input_buff, 17);
}

int32_t RoutingArbiterEcmp::decide_next_node_id(int32_t current_node_id, int32_t source_node_id, int32_t target_node_id, std::set<int64_t>& neighbor_node_ids, Ptr<const Packet> pkt, Ipv4Header const &ipHeader) {
    uint32_t hash = ComputeFiveTupleHash(ipHeader, pkt, current_node_id);
    int s = candidate_list[current_node_id][target_node_id].size();
    return candidate_list[current_node_id][target_node_id][hash % s];
}

int64_t RoutingArbiterEcmp::get_ecmp_init_finish_ns_since_epoch() {
    return ecmp_init_finish_ns_since_epoch;
}
