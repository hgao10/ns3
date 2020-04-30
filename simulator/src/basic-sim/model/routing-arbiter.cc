#include "ns3/routing-arbiter.h"

using namespace ns3;

// Routing arbiter result

RoutingArbiterResult::RoutingArbiterResult(bool failed, uint32_t out_if_idx, uint32_t gateway_ip_address) {
    m_failed = failed;
    m_out_if_idx = out_if_idx;
    m_gateway_ip_address = gateway_ip_address;
}

bool RoutingArbiterResult::Failed() {
    return m_failed;
}

uint32_t RoutingArbiterResult::GetOutIfIdx() {
    if (m_failed) {
        throw std::runtime_error("Cannot retrieve out interface index if the arbiter did not succeed in finding a next hop");
    }
    return m_out_if_idx;
}

uint32_t RoutingArbiterResult::GetGatewayIpAddress() {
    if (m_failed) {
        throw std::runtime_error("Cannot retrieve gateway IP address if the arbiter did not succeed in finding a next hop");
    }
    return m_gateway_ip_address;
}

// Routing arbiter

RoutingArbiter::RoutingArbiter(Ptr<Node> this_node, NodeContainer nodes) {
    this->m_node_id = this_node->GetId();
    this->m_nodes = nodes;

    // Store IP address to node id (each interface has an IP address, so multiple IPs per node)
    for (uint32_t i = 0; i < m_nodes.GetN(); i++) {
        for (uint32_t j = 1; j < m_nodes.Get(i)->GetObject<Ipv4>()->GetNInterfaces(); j++) {
            m_ip_to_node_id.insert({m_nodes.Get(i)->GetObject<Ipv4>()->GetAddress(j,0).GetLocal().Get(), i});
        }
    }

}

uint32_t RoutingArbiter::ResolveNodeIdFromIp(uint32_t ip) {
    m_ip_to_node_id_it = m_ip_to_node_id.find(ip);
    if (m_ip_to_node_id_it != m_ip_to_node_id.end()) {
        return m_ip_to_node_id_it->second;
    } else {
        throw std::invalid_argument(format_string("IP address %u is not mapped to a node id", ip));
    }
}

RoutingArbiterResult RoutingArbiter::BaseDecide(Ptr<const Packet> pkt, Ipv4Header const &ipHeader) {

    // Retrieve the source node id
    uint32_t source_ip = ipHeader.GetSource().Get();
    uint32_t source_node_id;

    // Ipv4Address default constructor has IP 0x66666666 = 102.102.102.102 = 1717986918,
    // which is set by TcpSocketBase::SetupEndpoint to discover its actually source IP.
    bool is_request_for_source_ip = source_ip == 1717986918;

    // If it is a request for source IP, the source node id is just the current node.
    if (is_request_for_source_ip) {
        source_node_id = m_node_id;
    } else {
        source_node_id = ResolveNodeIdFromIp(source_ip);
    }
    uint32_t target_node_id = ResolveNodeIdFromIp(ipHeader.GetDestination().Get());

    // Decide the next node
    return Decide(
                source_node_id,
                target_node_id,
                pkt,
                ipHeader,
                is_request_for_source_ip
    );

}
