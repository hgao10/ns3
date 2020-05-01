#include "topology-ptop.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TopologyPtop);
TypeId TopologyPtop::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::TopologyPtop")
            .SetParent<Object> ()
            .SetGroupName("BasicSim")
    ;
    return tid;
}

TopologyPtop::TopologyPtop(BasicSimulation* basicSimulation, Ipv4RoutingHelper* ipv4RoutingHelper) {
    m_basicSimulation = basicSimulation;
    m_ipv4RoutingHelper = ipv4RoutingHelper;
    ReadProperties();
    ReadGraph();
    SetupNodes();
    SetupLinks();
}

void TopologyPtop::ReadProperties() {

    // Link properties
    m_link_data_rate_megabit_per_s = parse_positive_double(m_basicSimulation->GetConfigParamOrFail("link_data_rate_megabit_per_s"));
    m_link_delay_ns = parse_positive_int64(m_basicSimulation->GetConfigParamOrFail("link_delay_ns"));
    m_link_max_queue_size_pkts = parse_positive_int64(m_basicSimulation->GetConfigParamOrFail("link_max_queue_size_pkts"));

    // Qdisc properties
    m_disable_qdisc_endpoint_tors_xor_servers = parse_boolean(m_basicSimulation->GetConfigParamOrFail("disable_qdisc_endpoint_tors_xor_servers"));
    m_disable_qdisc_non_endpoint_switches = parse_boolean(m_basicSimulation->GetConfigParamOrFail("disable_qdisc_non_endpoint_switches"));

}

void TopologyPtop::ReadGraph() {

    std::map<std::string, std::string> config = read_config(m_basicSimulation->GetRunDir() + "/" + m_basicSimulation->GetConfigParamOrFail("filename_topology"));
    this->num_nodes = parse_positive_int64(get_param_or_fail("num_nodes", config));
    this->num_undirected_edges = parse_positive_int64(get_param_or_fail("num_undirected_edges", config));

    // Node types
    std::string tmp = get_param_or_fail("switches", config);
    this->switches = parse_set_positive_int64(tmp);
    all_items_are_less_than(this->switches, num_nodes);
    tmp = get_param_or_fail("switches_which_are_tors", config);
    this->switches_which_are_tors = parse_set_positive_int64(tmp);
    all_items_are_less_than(this->switches_which_are_tors, num_nodes);
    tmp = get_param_or_fail("servers", config);
    this->servers = parse_set_positive_int64(tmp);
    all_items_are_less_than(this->servers, num_nodes);

    // Adjacency list
    for (int i = 0; i < this->num_nodes; i++) {
        adjacency_list.push_back(std::set<int64_t>());
    }

    // Edges
    tmp = get_param_or_fail("undirected_edges", config);
    std::set<std::string> string_set = parse_set_string(tmp);
    for (std::string s : string_set) {
        std::vector<std::string> spl = split_string(s, "-", 2);
        int64_t a = parse_positive_int64(spl[0]);
        int64_t b = parse_positive_int64(spl[1]);
        if (a == b) {
            throw std::invalid_argument(format_string("Cannot have edge to itself on node %" PRIu64 "", a));
        }
        if (a >= this->num_nodes) {
            throw std::invalid_argument(format_string("Left node identifier in edge does not exist: %" PRIu64 "", a));
        }
        if (b >= this->num_nodes) {
            throw std::invalid_argument(format_string("Right node identifier in edge does not exist: %" PRIu64 "", b));
        }
        undirected_edges.push_back(std::make_pair(a < b ? a : b, a < b ? b : a));
        undirected_edges_set.insert(std::make_pair(a < b ? a : b, a < b ? b : a));
        adjacency_list[a].insert(b);
        adjacency_list[b].insert(a);
    }

    // Sort them for convenience
    std::sort(undirected_edges.begin(), undirected_edges.end());

    if (undirected_edges.size() != (size_t) num_undirected_edges) {
        throw std::invalid_argument("Indicated number of undirected edges does not match edge set");
    }

    if (undirected_edges.size() != undirected_edges_set.size()) {
        throw std::invalid_argument("Duplicates in edge set");
    }

    // Node type hierarchy checks

    if (!direct_set_intersection(this->servers, this->switches).empty()) {
        throw std::invalid_argument("Server and switch identifiers are not distinct");
    }

    if (direct_set_union(this->servers, this->switches).size() != (size_t) num_nodes) {
        throw std::invalid_argument("The servers and switches do not encompass all nodes");
    }

    if (direct_set_intersection(this->switches, this->switches_which_are_tors).size() != this->switches_which_are_tors.size()) {
        throw std::invalid_argument("Servers are marked as ToRs");
    }

    // Servers must be connected to ToRs only
    for (int64_t node_id : this->servers) {
        for (int64_t neighbor_id : adjacency_list[node_id]) {
            if (this->switches_which_are_tors.find(neighbor_id) == this->switches_which_are_tors.end()) {
                throw std::invalid_argument(format_string("Server node %" PRId64 " has an edge to node %" PRId64 " which is not a ToR.", node_id, neighbor_id));
            }
        }
    }

    if (this->servers.size() > 0) {
        has_zero_servers = false;
    } else {
        has_zero_servers = true;
    }

    // Read topology
    printf("TOPOLOGY\n-----\n");
    printf("%-25s  %" PRIu64 "\n", "# Nodes", this->num_nodes);
    printf("%-25s  %" PRIu64 "\n", "# Undirected edges", this->num_undirected_edges);
    printf("%-25s  %" PRIu64 "\n", "# Switches", this->switches.size());
    printf("%-25s  %" PRIu64 "\n", "# ... of which ToRs", this->switches_which_are_tors.size());
    printf("%-25s  %" PRIu64 "\n", "# Servers", this->servers.size());
    std::cout << std::endl;
    m_basicSimulation->RegisterTimestamp("Read topology");

    // MTU = 1500 byte, +2 with the p2p header.
    // There are n_q packets in the queue at most, e.g. n_q + 2 (incl. transmit and within mandatory 1-packet qdisc)
    // Queueing + transmission delay/hop = (n_q + 2) * 1502 byte / link data rate
    // Propagation delay/hop = link delay
    //
    // If the topology is big, lets assume 10 hops either direction worst case, so 20 hops total
    // If the topology is not big, < 10 undirected edges, we just use 2 * number of undirected edges as worst-case hop count
    //
    // num_hops * (((n_q + 2) * 1502 byte) / link data rate) + link delay)
    int num_hops = std::min((int64_t) 20, this->num_undirected_edges * 2);
    m_worst_case_rtt_ns = num_hops * (((m_link_max_queue_size_pkts + 2) * 1502) / (m_link_data_rate_megabit_per_s * 125000 / 1000000000) + m_link_delay_ns);
    printf("Estimated worst-case RTT: %.3f ms\n\n", m_worst_case_rtt_ns / 1e6);

}

void TopologyPtop::SetupNodes() {
    std::cout << "SETUP NODES" << std::endl;
    std::cout << "  > Creating nodes and installing Internet stack on each" << std::endl;

    // Install Internet on all nodes
    m_nodes.Create(this->num_nodes);
    InternetStackHelper internet;
    internet.SetRoutingHelper(*m_ipv4RoutingHelper);
    internet.Install(m_nodes);

    std::cout << std::endl;
    m_basicSimulation->RegisterTimestamp("Create and install nodes");
}

void TopologyPtop::SetupLinks() {
    std::cout << "SETUP LINKS" << std::endl;

    // Each link is a network on its own
    Ipv4AddressHelper address;
    address.SetBase("10.0.0.0", "255.255.255.0");

    // Direct network device attributes
    std::cout << "  > Point-to-point network device attributes:" << std::endl;

    // Point-to-point network device helper
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue(std::to_string(m_link_data_rate_megabit_per_s) + "Mbps"));
    p2p.SetChannelAttribute("Delay", TimeValue(NanoSeconds(m_link_delay_ns)));
    std::cout << "      >> Data rate......... " << m_link_data_rate_megabit_per_s << " Mbit/s" << std::endl;
    std::cout << "      >> Delay............. " << m_link_delay_ns << " ns" << std::endl;
    std::cout << "      >> Max. queue size... " << m_link_max_queue_size_pkts << " packets" << std::endl;
    std::string p2p_net_device_max_queue_size_pkts_str = format_string("%" PRId64 "p", m_link_max_queue_size_pkts);

    // Notify about topology state
    if (this->has_zero_servers) {
        std::cout << "  > Because there are no servers, ToRs are considered valid flow-endpoints" << std::endl;
    } else {
        std::cout << "  > Only servers are considered valid flow-endpoints" << std::endl;
    }

    // Queueing disciplines
    std::cout << "  > Traffic control queueing disciplines:" << std::endl;

    // Qdisc for endpoints (i.e., servers if they are defined, else the ToRs)
    TrafficControlHelper tch_endpoints;
    if (m_disable_qdisc_endpoint_tors_xor_servers) {
        tch_endpoints.SetRootQueueDisc("ns3::FifoQueueDisc", "MaxSize", QueueSizeValue(QueueSize("1p"))); // No queueing discipline basically
        std::cout << "      >> Flow-endpoints....... none (FIFO with 1 max. queue size)" << std::endl;
    } else {
        std::string interval = format_string("%" PRId64 "ns", m_worst_case_rtt_ns);
        std::string target = format_string("%" PRId64 "ns", m_worst_case_rtt_ns / 20);
        tch_endpoints.SetRootQueueDisc("ns3::FqCoDelQueueDisc", "Interval", StringValue(interval), "Target", StringValue(target));
        printf("      >> Flow-endpoints....... fq-co-del (interval = %.2f ms, target = %.2f ms)\n", m_worst_case_rtt_ns / 1e6, m_worst_case_rtt_ns / 1e6 / 20);
    }

    // Qdisc for non-endpoints (i.e., if servers are defined, all switches, else the switches which are not ToRs)
    TrafficControlHelper tch_not_endpoints;
    if (m_disable_qdisc_non_endpoint_switches) {
        tch_not_endpoints.SetRootQueueDisc("ns3::FifoQueueDisc", "MaxSize", QueueSizeValue(QueueSize("1p"))); // No queueing discipline basically
        std::cout << "      >> Non-flow-endpoints... none (FIFO with 1 max. queue size)" << std::endl;
    } else {
        std::string interval = format_string("%" PRId64 "ns", m_worst_case_rtt_ns);
        std::string target = format_string("%" PRId64 "ns", m_worst_case_rtt_ns / 20);
        tch_not_endpoints.SetRootQueueDisc("ns3::FqCoDelQueueDisc", "Interval", StringValue(interval), "Target", StringValue(target));
        printf("      >> Non-flow-endpoints... fq-co-del (interval = %.2f ms, target = %.2f ms)\n", m_worst_case_rtt_ns / 1e6, m_worst_case_rtt_ns / 1e6 / 20);
    }

    // Create Links
    std::cout << "  > Installing links" << std::endl;
    m_interface_idxs_for_edges.clear();
    for (std::pair <int64_t, int64_t> link : this->undirected_edges) {

        // Install link
        NetDeviceContainer container = p2p.Install(m_nodes.Get(link.first), m_nodes.Get(link.second));
        container.Get(0)->GetObject<PointToPointNetDevice>()->GetQueue()->SetMaxSize(p2p_net_device_max_queue_size_pkts_str);
        container.Get(1)->GetObject<PointToPointNetDevice>()->GetQueue()->SetMaxSize(p2p_net_device_max_queue_size_pkts_str);

        // Install traffic control
        if (this->IsValidEndpoint(link.first)) {
            tch_endpoints.Install(container.Get(0));
        } else {
            tch_not_endpoints.Install(container.Get(0));
        }
        if (this->IsValidEndpoint(link.second)) {
            tch_endpoints.Install(container.Get(1));
        } else {
            tch_not_endpoints.Install(container.Get(1));
        }

        // Assign IP addresses
        address.Assign(container);
        address.NewNetwork();

        // Save to mapping
        uint32_t a = container.Get(0)->GetIfIndex();
        uint32_t b = container.Get(1)->GetIfIndex();
        m_interface_idxs_for_edges.push_back(std::make_pair(a, b));

    }

    std::cout << std::endl;
    m_basicSimulation->RegisterTimestamp("Create links and edge-to-interface-index mapping");
}

NodeContainer TopologyPtop::GetNodes() {
    return m_nodes;
}

bool TopologyPtop::IsValidEndpoint(int64_t node_id) {
    if (has_zero_servers) {
        return this->switches_which_are_tors.find(node_id) != this->switches_which_are_tors.end();
    } else {
        return this->servers.find(node_id) != this->servers.end();
    }
}

std::set<int64_t> TopologyPtop::GetEndpoints() {
    if (has_zero_servers) {
        return this->switches_which_are_tors;
    } else {
        return this->servers;
    }
}

int64_t TopologyPtop::GetWorstCaseRttEstimateNs() {
    return m_worst_case_rtt_ns;
}

std::vector<std::pair<uint32_t, uint32_t>>* TopologyPtop::GetInterfaceIdxsForEdges() {
    return &m_interface_idxs_for_edges;
}

}
