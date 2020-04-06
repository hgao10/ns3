/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/basic-simulation.h"
#include "ns3/test.h"
#include "test-helpers.h"

using namespace ns3;

////////////////////////////////////////////////////////////////////////////////////////

NodeContainer create_nodes(Topology& topology) {
    NodeContainer nodes;
    nodes.Create(topology.num_nodes);
    InternetStackHelper internet;
    Ipv4ArbitraryRoutingHelper arbitraryRoutingHelper;
    internet.SetRoutingHelper (arbitraryRoutingHelper);
    internet.Install(nodes);
    return nodes;
}

std::vector<std::pair<uint32_t, uint32_t>> setup_links(NodeContainer nodes, Topology& topology) {
    Ipv4AddressHelper address;
    address.SetBase("10.0.0.0", "255.255.255.0"); // Each link is a network on its own
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue(std::to_string(100) + "Mbps"));
    p2p.SetChannelAttribute("Delay", TimeValue(NanoSeconds(1000)));
    std::vector<std::pair<uint32_t, uint32_t>> interface_idxs_for_edges;
    for (std::pair<int64_t, int64_t> link : topology.undirected_edges) {
        NetDeviceContainer container = p2p.Install(nodes.Get(link.first), nodes.Get(link.second));
        address.Assign(container);
        address.NewNetwork();
        interface_idxs_for_edges.push_back(std::make_pair(container.Get(0)->GetIfIndex(),container.Get(1)->GetIfIndex()));
    }
    return interface_idxs_for_edges;
}

////////////////////////////////////////////////////////////////////////////////////////

class RoutingArbiterIpResolutionTestCase : public TestCase
{
public:
    RoutingArbiterIpResolutionTestCase () : TestCase ("routing-arbiter basic") {};
    void DoRun () {
        std::ofstream topology_file;
        topology_file.open ("topology.properties.temp");
        topology_file << "num_nodes=4" << std::endl;
        topology_file << "num_undirected_edges=4" << std::endl;
        topology_file << "switches=set(0,1,2,3)" << std::endl;
        topology_file << "switches_which_are_tors=set(0,1,2,3)" << std::endl;
        topology_file << "servers=set()" << std::endl;
        topology_file << "undirected_edges=set(0-1,1-2,2-3,0-3)" << std::endl;
        topology_file.close();
        Topology topology = Topology("topology.properties.temp");

        // Create nodes, setup links and create arbiter
        NodeContainer nodes = create_nodes(topology);
        std::vector<std::pair<uint32_t, uint32_t>> interface_idxs_for_edges = setup_links(nodes, topology);
        RoutingArbiterEcmp routingArbiter = RoutingArbiterEcmp(&topology, nodes, interface_idxs_for_edges);

        // Test valid IPs
        ASSERT_EQUAL(routingArbiter.resolve_node_id_from_ip(Ipv4Address("10.0.0.1").Get()), 0);
        ASSERT_EQUAL(routingArbiter.resolve_node_id_from_ip(Ipv4Address("10.0.0.2").Get()), 1);
        ASSERT_EQUAL(routingArbiter.resolve_node_id_from_ip(Ipv4Address("10.0.1.1").Get()), 0);
        ASSERT_EQUAL(routingArbiter.resolve_node_id_from_ip(Ipv4Address("10.0.1.2").Get()), 3);
        ASSERT_EQUAL(routingArbiter.resolve_node_id_from_ip(Ipv4Address("10.0.2.1").Get()), 1);
        ASSERT_EQUAL(routingArbiter.resolve_node_id_from_ip(Ipv4Address("10.0.2.2").Get()), 2);
        ASSERT_EQUAL(routingArbiter.resolve_node_id_from_ip(Ipv4Address("10.0.3.1").Get()), 2);
        ASSERT_EQUAL(routingArbiter.resolve_node_id_from_ip(Ipv4Address("10.0.3.2").Get()), 3);

        // All other should be invalid, a few examples
        ASSERT_EXCEPTION(routingArbiter.resolve_node_id_from_ip(Ipv4Address("10.0.0.0").Get()));
        ASSERT_EXCEPTION(routingArbiter.resolve_node_id_from_ip(Ipv4Address("10.0.1.3").Get()));
        ASSERT_EXCEPTION(routingArbiter.resolve_node_id_from_ip(Ipv4Address("10.0.4.1").Get()));

        // Clean up topology file
        remove_file_if_exists("topology.properties.temp");

        Simulator::Destroy();

    }
};

////////////////////////////////////////////////////////////////////////////////////////

struct ecmp_fields_t {
    int32_t node_id;
    uint32_t src_ip;
    uint32_t dst_ip;
    bool is_tcp;
    bool is_udp;
    uint16_t src_port;
    uint16_t dst_port;
};

void create_headered_packet(Ptr<Packet> p, ecmp_fields_t e) {

    // Set IP header
    Ipv4Header ipHeader;
    ipHeader.SetSource(Ipv4Address(e.src_ip));
    ipHeader.SetDestination(Ipv4Address(e.dst_ip));

    if (e.is_tcp) { // Set TCP+IP header
        TcpHeader tcpHeader;
        tcpHeader.SetSourcePort(e.src_port);
        tcpHeader.SetDestinationPort(e.dst_port);
        ipHeader.SetProtocol(6);
        p->AddHeader(tcpHeader);
        p->AddHeader(ipHeader);

    } else if (e.is_udp) { // Setup UDP+IP header
        UdpHeader udpHeader;
        udpHeader.SetSourcePort(e.src_port);
        udpHeader.SetDestinationPort(e.dst_port);
        ipHeader.SetProtocol(17);
        p->AddHeader(udpHeader);
        p->AddHeader(ipHeader);

    } else { // Setup only IP header
        p->AddHeader(ipHeader);
    }

}

class RoutingArbiterEcmpHashTestCase : public TestCase
{
public:
    RoutingArbiterEcmpHashTestCase () : TestCase ("routing-arbiter-ecmp hash") {};
    void DoRun () {
        std::ofstream topology_file;
        topology_file.open ("topology.properties.temp");
        topology_file << "num_nodes=4" << std::endl;
        topology_file << "num_undirected_edges=4" << std::endl;
        topology_file << "switches=set(0,1,2,3)" << std::endl;
        topology_file << "switches_which_are_tors=set(0,1,2,3)" << std::endl;
        topology_file << "servers=set()" << std::endl;
        topology_file << "undirected_edges=set(0-1,1-2,2-3,0-3)" << std::endl;
        topology_file.close();
        Topology topology = Topology("topology.properties.temp");

        // Create nodes, setup links and create arbiter
        NodeContainer nodes = create_nodes(topology);
        std::vector<std::pair<uint32_t, uint32_t>> interface_idxs_for_edges = setup_links(nodes, topology);
        RoutingArbiterEcmp routingArbiterEcmp = RoutingArbiterEcmp(&topology, nodes, interface_idxs_for_edges);

        ///////
        // All hashes should be different if any of the 5 values are different

        ecmp_fields_t cases[] = {
                {1, 2, 3, true, false, 4, 5}, // TCP
                {6, 2, 3, true, false, 4, 5}, // TCP different node id
                {1, 6, 3, true, false, 4, 5}, // TCP different source IP
                {1, 2, 6, true, false, 4, 5}, // TCP different destination IP
                {1, 2, 3, true, false, 6, 5}, // TCP different source port
                {1, 2, 3, true, false, 4, 6}, // TCP different destination port
                {1, 2, 3, false, true, 4, 5}, // UDP
                {6, 2, 3, false, true, 4, 5}, // UDP different node id
                {1, 6, 3, false, true, 4, 5}, // UDP different source IP
                {1, 2, 6, false, true, 4, 5}, // UDP different destination IP
                {1, 2, 3, false, true, 6, 5}, // UDP different source port
                {1, 2, 3, false, true, 4, 6}, // UDP different destination port
                {1, 2, 3, false, false, 1, 1}, // Not TCP/UDP
                {6, 2, 3, false, false, 1, 1}, // Not TCP/UDP different node id
                {1, 6, 3, false, false, 1, 1}, // Not TCP/UDP different source IP
                {1, 2, 6, false, false, 1, 1}, // Not TCP/UDP different destination IP
        };
        int num_cases = sizeof(cases) / sizeof(ecmp_fields_t);
        std::vector<uint32_t> hash_results;
        std::set<uint32_t> hash_results_set;
        for (ecmp_fields_t e : cases) {
            Ptr<Packet> p = Create<Packet>(5);
            create_headered_packet(p, e);
            Ipv4Header ipHeader;
            p->RemoveHeader(ipHeader);
            hash_results.push_back(routingArbiterEcmp.ComputeFiveTupleHash(ipHeader, p, e.node_id));
        }
        for (int i = 0; i < num_cases; i++) {
            for (int j = i + 1; j < num_cases; j++) {
                ASSERT_NOT_EQUAL(hash_results[i], hash_results[j]);
            }
            // std::cout << "Hash[" << i << "] = " << hash_results[i] << std::endl;
        }

        ///////
        // Test same hash

        // Same TCP for same 5-tuple
        Ptr<Packet> p1 = Create<Packet>(555);
        Ptr<Packet> p2 = Create<Packet>(20);
        create_headered_packet(p1, {6666, 4363227, 215326, true, false, 4663, 8888});
        Ipv4Header p1header;
        p1->RemoveHeader(p1header);
        create_headered_packet(p2, {6666, 4363227, 215326, true, false, 4663, 8888});
        Ipv4Header p2header;
        p2->RemoveHeader(p2header);
        ASSERT_EQUAL(
                routingArbiterEcmp.ComputeFiveTupleHash(p1header, p1, 6666),
                routingArbiterEcmp.ComputeFiveTupleHash(p2header, p2, 6666)
        );

        // Same UDP for same 5-tuple
        p1 = Create<Packet>(555);
        p2 = Create<Packet>(8888);
        create_headered_packet(p1, {123456, 32543526, 937383, false, true, 1234, 6737});
        p1->RemoveHeader(p1header);
        create_headered_packet(p2, {123456, 32543526, 937383, false, true, 1234, 6737});
        p2->RemoveHeader(p2header);
        ASSERT_EQUAL(
                routingArbiterEcmp.ComputeFiveTupleHash(p1header, p1, 123456),
                routingArbiterEcmp.ComputeFiveTupleHash(p2header, p2, 123456)
        );

        // Same not TCP/UDP for same 3-tuple
        p1 = Create<Packet>(77);
        p2 = Create<Packet>(77);
        create_headered_packet(p1, {7, 3626, 22, false, false, 55, 123});
        p1->RemoveHeader(p1header);
        create_headered_packet(p2, {7, 3626, 22, false, false, 44, 7777});
        p2->RemoveHeader(p2header);
        ASSERT_EQUAL(
                routingArbiterEcmp.ComputeFiveTupleHash(p1header, p1, 7),
                routingArbiterEcmp.ComputeFiveTupleHash(p2header, p2, 7)
        );

        // Clean up topology file
        remove_file_if_exists("topology.properties.temp");

        Simulator::Destroy();

    }
};

////////////////////////////////////////////////////////////////////////////////////////

class RoutingArbiterEcmpStringReprTestCase : public TestCase
{
public:
    RoutingArbiterEcmpStringReprTestCase () : TestCase ("routing-arbiter-ecmp string-repr") {};
    void DoRun () {
        std::ofstream topology_file;
        topology_file.open ("topology.properties.temp");
        topology_file << "num_nodes=4" << std::endl;
        topology_file << "num_undirected_edges=4" << std::endl;
        topology_file << "switches=set(0,1,2,3)" << std::endl;
        topology_file << "switches_which_are_tors=set(0,1,2,3)" << std::endl;
        topology_file << "servers=set()" << std::endl;
        topology_file << "undirected_edges=set(0-1,1-2,2-3,0-3)" << std::endl;
        topology_file.close();
        Topology topology = Topology("topology.properties.temp");

        // Create nodes, setup links and create arbiter
        NodeContainer nodes = create_nodes(topology);
        std::vector<std::pair<uint32_t, uint32_t>> interface_idxs_for_edges = setup_links(nodes, topology);
        RoutingArbiterEcmp routingArbiterEcmp = RoutingArbiterEcmp(&topology, nodes, interface_idxs_for_edges);

        for (int i = 0; i < topology.num_nodes; i++) {
            nodes.Get(i)->GetObject<Ipv4>()->GetRoutingProtocol()->GetObject<Ipv4ArbitraryRouting>()->SetRoutingArbiter(&routingArbiterEcmp);
            std::ostringstream res;
            OutputStreamWrapper out_stream = OutputStreamWrapper(&res);
            nodes.Get(i)->GetObject<Ipv4>()->GetRoutingProtocol()->GetObject<Ipv4ArbitraryRouting>()->PrintRoutingTable(&out_stream);
            std::ostringstream expected;
            if (i == 0) {
                expected << "ECMP state of node 0" << std::endl;
                expected << "  -> 0: {}" << std::endl;
                expected << "  -> 1: {1}" << std::endl;
                expected << "  -> 2: {1,3}" << std::endl;
                expected << "  -> 3: {3}" << std::endl;
                ASSERT_EQUAL(res.str(), expected.str());
            } else if (i == 1) {
                expected << "ECMP state of node 1" << std::endl;
                expected << "  -> 0: {0}" << std::endl;
                expected << "  -> 1: {}" << std::endl;
                expected << "  -> 2: {2}" << std::endl;
                expected << "  -> 3: {0,2}" << std::endl;
                ASSERT_EQUAL(res.str(), expected.str());
            } else if (i == 2) {
                expected << "ECMP state of node 2" << std::endl;
                expected << "  -> 0: {1,3}" << std::endl;
                expected << "  -> 1: {1}" << std::endl;
                expected << "  -> 2: {}" << std::endl;
                expected << "  -> 3: {3}" << std::endl;
                ASSERT_EQUAL(res.str(), expected.str());
            } else if (i == 3) {
                expected << "ECMP state of node 3" << std::endl;
                expected << "  -> 0: {0}" << std::endl;
                expected << "  -> 1: {0,2}" << std::endl;
                expected << "  -> 2: {2}" << std::endl;
                expected << "  -> 3: {}" << std::endl;
                ASSERT_EQUAL(res.str(), expected.str());
            } else {
                ASSERT_TRUE(false);
            }
        }

        // Clean up topology file
        remove_file_if_exists("topology.properties.temp");

        Simulator::Destroy();

    }
};

////////////////////////////////////////////////////////////////////////////////////////

class RoutingArbiterBad: public RoutingArbiter
{
public:

    RoutingArbiterBad(Topology* topology, NodeContainer nodes, const std::vector<std::pair<uint32_t, uint32_t>>& interface_idxs_for_edges) : RoutingArbiter(topology, nodes, interface_idxs_for_edges) {
        // Left empty intentionally
    }

    void set_decision(int32_t val) {
        this->next_decision = val;
    }

    int32_t decide_next_node_id(int32_t current_node, int32_t source_node_id, int32_t target_node_id, std::set<int64_t>& neighbor_node_ids, Ptr<const Packet> pkt, Ipv4Header const &ipHeader) {
        return this->next_decision;
    }

    std::string string_repr_of_routing_table(int32_t node_id) {
        return "";
    }

private:
    int32_t next_decision = -1;

};

class RoutingArbiterBadImplTestCase : public TestCase
{
public:
    RoutingArbiterBadImplTestCase () : TestCase ("routing-arbiter bad-impl") {};
    void DoRun () {
        std::ofstream topology_file;
        topology_file.open ("topology.properties.temp");
        topology_file << "num_nodes=4" << std::endl;
        topology_file << "num_undirected_edges=4" << std::endl;
        topology_file << "switches=set(0,1,2,3)" << std::endl;
        topology_file << "switches_which_are_tors=set(0,1,2,3)" << std::endl;
        topology_file << "servers=set()" << std::endl;
        topology_file << "undirected_edges=set(0-1,1-2,2-3,0-3)" << std::endl;
        topology_file.close();
        Topology topology = Topology("topology.properties.temp");

        // Create nodes, setup links and create arbiter
        NodeContainer nodes = create_nodes(topology); // Only nodes to create
        std::vector<std::pair<uint32_t, uint32_t>> interface_idxs_for_edges = setup_links(nodes, topology);
        RoutingArbiterBad arbiterBad = RoutingArbiterBad(&topology, nodes, interface_idxs_for_edges);

        // This should be fine
        arbiterBad.set_decision(2);
        Ptr<Packet> p1 = Create<Packet>(555);
        create_headered_packet(p1, {1, Ipv4Address("10.0.2.1").Get(), Ipv4Address("10.0.3.2").Get(), true, false, 4663, 8888});
        Ipv4Header p1header;
        p1->RemoveHeader(p1header);
        ASSERT_EQUAL(2, arbiterBad.decide_next_interface(1, p1, p1header));

        // Not a neighbor
        arbiterBad.set_decision(3);
        p1 = Create<Packet>(555);
        create_headered_packet(p1, {1, Ipv4Address("10.0.2.1").Get(), Ipv4Address("10.0.3.2").Get(), true, false, 4663, 8888});
        p1->RemoveHeader(p1header);
        ASSERT_EXCEPTION(arbiterBad.decide_next_interface(1, p1, p1header));

        // Out of range <
        arbiterBad.set_decision(-1);
        p1 = Create<Packet>(555);
        create_headered_packet(p1, {1, Ipv4Address("10.0.2.1").Get(), Ipv4Address("10.0.3.2").Get(), true, false, 4663, 8888});
        p1->RemoveHeader(p1header);
        ASSERT_EXCEPTION(arbiterBad.decide_next_interface(1, p1, p1header));

        // Out of range >
        arbiterBad.set_decision(4);
        p1 = Create<Packet>(555);
        create_headered_packet(p1, {1, Ipv4Address("10.0.2.1").Get(), Ipv4Address("10.0.3.2").Get(), true, false, 4663, 8888});
        p1->RemoveHeader(p1header);
        ASSERT_EXCEPTION(arbiterBad.decide_next_interface(1, p1, p1header));

        // Clean up
        remove_file_if_exists("topology.properties.temp");
        Simulator::Destroy();

    }
};

////////////////////////////////////////////////////////////////////////////////////////

// Currently disabled because it takes too long for a quick test
class RoutingArbiterEcmpTooBigFailTestCase : public TestCase
{
public:
    RoutingArbiterEcmpTooBigFailTestCase () : TestCase ("routing-arbiter-ecmp too-big-fail") {};
    void DoRun () {
        std::ofstream topology_file;
        topology_file.open ("topology.properties.temp");
        topology_file << "num_nodes=40001" << std::endl;
        topology_file << "num_undirected_edges=0" << std::endl;
        topology_file << "switches=set(";
        for (int i = 0; i < 40001; i++) {
            if (i == 0) {
                topology_file << i;
            } else {
                topology_file << "," << i;
            }
        }
        topology_file << ")" << std::endl;
        topology_file << "switches_which_are_tors=set()" << std::endl;
        topology_file << "servers=set()" << std::endl;
        topology_file << "undirected_edges=set()" << std::endl;
        topology_file.close();
        Topology topology = Topology("topology.properties.temp");

        // Create nodes, setup links and create arbiter
        NodeContainer nodes = create_nodes(topology); // Only nodes to create
        std::vector<std::pair<uint32_t, uint32_t>> interface_idxs_for_edges = setup_links(nodes, topology);
        ASSERT_EXCEPTION(RoutingArbiterEcmp(&topology, nodes, interface_idxs_for_edges)); // > 40000 nodes is not allowed

        remove_file_if_exists("topology.properties.temp");

        Simulator::Destroy();

    }
};
