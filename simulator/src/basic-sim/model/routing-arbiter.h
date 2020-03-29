#ifndef ROUTING_ARBITER_H
#define ROUTING_ARBITER_H


#include <map>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <chrono>
#include <stdexcept>
#include "ns3/topology.h"
#include "ns3/node-container.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-header.h"

class RoutingArbiter
{

public:
    RoutingArbiter(Topology* topology, ns3::NodeContainer nodes, std::vector<std::pair<int32_t, int32_t>> interface_idxs_for_edges);
    int32_t decide_next_interface(int32_t current_node, ns3::Ptr<const ns3::Packet> pkt, ns3::Ipv4Header const &ipHeader);

    /**
     * From among the neighbors, decide where the packet needs to be routed to.
     *
     * @param current_node_id       Node where the packet is right now
     * @param source_node_id        Node where the packet originated from
     * @param target_node_id        Node where the packet has to go to
     * @param neighbor_node_ids     All neighboring nodes from which to choose
     * @param pkt                   Packet pointer
     * @param ipHeader              IPHeader instance
     *
     * @return Neighbor node id to which to forward to
     */
    virtual int32_t decide_next_node_id(
            int32_t current_node_id,
            int32_t source_node_id,
            int32_t target_node_id,
            std::set<int64_t>& neighbor_node_ids,
            ns3::Ptr<const ns3::Packet> pkt,
            ns3::Ipv4Header const &ipHeader
    ) = 0;

protected:
    Topology* topology;
    std::map<uint32_t, uint32_t> ip_to_node_id;

private:
    ns3::NodeContainer nodes;
    std::vector<uint32_t> neighbor_node_id_to_if_idx;

};

#endif //ROUTING_ARBITER_H
