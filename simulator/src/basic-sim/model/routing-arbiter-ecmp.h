#ifndef ROUTING_ARBITER_ECMP_H
#define ROUTING_ARBITER_ECMP_H

#include "ns3/routing-arbiter.h"
#include "ns3/topology.h"
#include "ns3/hash.h"
#include "ns3/ipv4-header.h"
#include "ns3/udp-header.h"
#include "ns3/tcp-header.h"

using namespace ns3;

const uint8_t TCP_PROT_NUMBER = 6;
const uint8_t UDP_PROT_NUMBER = 17;

class RoutingArbiterEcmp: public RoutingArbiter
{
public:
    int32_t decide_next_node_id(int32_t current_node, int32_t source_node_id, int32_t target_node_id, std::set<int64_t>& neighbor_node_ids, ns3::Ptr<const ns3::Packet> pkt, ns3::Ipv4Header const &ipHeader);
    RoutingArbiterEcmp(Topology* topology, ns3::NodeContainer nodes, std::vector<std::pair<int32_t, int32_t>> interface_idxs_for_edges);
    uint64_t ComputeFiveTupleHash(const ns3::Ipv4Header &header, ns3::Ptr<const ns3::Packet> p, int32_t node_id);
    int64_t get_init_finish_ns_since_epoch();

private:
    ns3::Hasher hasher;
    std::vector<std::vector<std::vector<uint32_t>>> candidate_list;
    char hash_input_buff[17];
    int64_t init_finish_ns_since_epoch;
};

#endif //ROUTING_ARBITER_ECMP_H
