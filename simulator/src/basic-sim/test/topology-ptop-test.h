/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/basic-simulation.h"
#include "ns3/test.h"
#include "ns3/topology-ptop.h"
#include "test-helpers.h"
#include "ns3/ipv4-arbiter-routing-helper.h"

using namespace ns3;

void write_config() {
    std::ofstream config_file("config_ns3.properties");
    config_file << "filename_topology=\"topology.properties.temp\"" << std::endl;
    config_file << "simulation_end_time_ns=10000000000" << std::endl;
    config_file << "simulation_seed=123456789" << std::endl;
    config_file << "link_data_rate_megabit_per_s=100.0" << std::endl;
    config_file << "link_delay_ns=10000" << std::endl;
    config_file << "link_max_queue_size_pkts=100" << std::endl;
    config_file << "disable_qdisc_endpoint_tors_xor_servers=true" << std::endl;
    config_file << "disable_qdisc_non_endpoint_switches=true" << std::endl;
    config_file.close();
}

////////////////////////////////////////////////////////////////////////////////////////

class TopologyEmptyTestCase : public TestCase
{
public:
    TopologyEmptyTestCase () : TestCase ("topology empty") {};
    void DoRun () {
        std::ofstream topology_file;
        topology_file.open ("topology.properties.temp");
        topology_file << "num_nodes=0" << std::endl;
        topology_file << "num_undirected_edges=0" << std::endl;
        topology_file << "switches=set()" << std::endl;
        topology_file << "switches_which_are_tors=set()" << std::endl;
        topology_file << "servers=set()" << std::endl;
        topology_file << "undirected_edges=set()" << std::endl;
        topology_file.close();
        write_config();
        Ptr<BasicSimulation> basicSimulation = CreateObject<BasicSimulation>("./");
        Ptr<TopologyPtop> topology = CreateObject<TopologyPtop>(basicSimulation, Ipv4ArbiterRoutingHelper());
        ASSERT_EQUAL(topology->GetNumNodes(), 0);
        ASSERT_EQUAL(topology->GetNumUndirectedEdges(), 0);
        ASSERT_EQUAL(topology->GetSwitches().size(), 0);
        ASSERT_EQUAL(topology->GetSwitchesWhichAreTors().size(), 0);
        ASSERT_EQUAL(topology->GetServers().size(), 0);
        ASSERT_EQUAL(topology->GetUndirectedEdges().size(), 0);
        ASSERT_EQUAL(topology->GetUndirectedEdgesSet().size(), 0);
        ASSERT_EQUAL(topology->GetAllAdjacencyLists().size(), 0);
        ASSERT_EQUAL(topology->GetEndpoints().size(), 0);
        remove_file_if_exists("topology.properties.temp");
        basicSimulation->Finalize();
    }
};

class TopologySingleTestCase : public TestCase
{
public:
    TopologySingleTestCase () : TestCase ("topology single") {};
    void DoRun () {

        std::ofstream topology_file;
        topology_file.open ("topology.properties.temp");
        topology_file << "num_nodes=2" << std::endl;
        topology_file << "num_undirected_edges=1" << std::endl;
        topology_file << "switches=set(0,1)" << std::endl;
        topology_file << "switches_which_are_tors=set(0,1)" << std::endl;
        topology_file << "servers=set()" << std::endl;
        topology_file << "undirected_edges=set(1-0)" << std::endl;
        topology_file.close();
        write_config();
        Ptr<BasicSimulation> basicSimulation = CreateObject<BasicSimulation>("./");
        Ptr<TopologyPtop> topology = CreateObject<TopologyPtop>(basicSimulation, Ipv4ArbiterRoutingHelper());
        remove_file_if_exists("topology.properties.temp");
        basicSimulation->Finalize();

        // Basic sizes
        ASSERT_EQUAL(topology->GetNumNodes(), 2);
        ASSERT_EQUAL(topology->GetNumUndirectedEdges(), 1);
        ASSERT_EQUAL(topology->GetSwitches().size(), 2);
        ASSERT_EQUAL(topology->GetSwitchesWhichAreTors().size(), 2);
        ASSERT_EQUAL(topology->GetServers().size(), 0);
        ASSERT_EQUAL(topology->GetUndirectedEdges().size(), 1);
        ASSERT_EQUAL(topology->GetUndirectedEdgesSet().size(), 1);
        ASSERT_EQUAL(topology->GetAllAdjacencyLists().size(), 2);
        ASSERT_EQUAL(topology->GetAllAdjacencyLists()[0].size(), 1);
        ASSERT_EQUAL(topology->GetAllAdjacencyLists()[1].size(), 1);
        ASSERT_EQUAL(topology->GetEndpoints().size(), 2);

        // Check contents
        ASSERT_TRUE(topology->IsValidEndpoint(0));
        ASSERT_TRUE(topology->IsValidEndpoint(1));
        ASSERT_TRUE(set_int64_contains(topology->GetSwitches(), 0));
        ASSERT_TRUE(set_int64_contains(topology->GetSwitches(), 1));
        ASSERT_TRUE(set_int64_contains(topology->GetSwitchesWhichAreTors(), 0));
        ASSERT_TRUE(set_int64_contains(topology->GetSwitchesWhichAreTors(), 1));
        ASSERT_EQUAL(topology->GetUndirectedEdges()[0].first, 0);
        ASSERT_EQUAL(topology->GetUndirectedEdges()[0].second, 1);
        ASSERT_TRUE(set_pair_int64_contains(topology->GetUndirectedEdgesSet(), std::make_pair<int64_t, int64_t>(0, 1)));
        ASSERT_TRUE(set_int64_contains(topology->GetAllAdjacencyLists()[0], 1));
        ASSERT_TRUE(set_int64_contains(topology->GetAllAdjacencyLists()[1], 0));
        std::set<int64_t> endpoints = topology->GetEndpoints();
        ASSERT_TRUE(set_int64_contains(endpoints, 0));
        ASSERT_TRUE(set_int64_contains(endpoints, 1));

    }
};

class TopologyTorTestCase : public TestCase
{
public:
    TopologyTorTestCase () : TestCase ("topology tor") {};
    void DoRun () {

        std::ofstream topology_file;
        topology_file.open ("topology.properties.temp");
        topology_file << "num_nodes=8" << std::endl;
        topology_file << "num_undirected_edges=7" << std::endl;
        topology_file << "switches=set(4)" << std::endl;
        topology_file << "switches_which_are_tors=set(4)" << std::endl;
        topology_file << "servers=set(0,1,2,3,5,6,7)" << std::endl;
        topology_file << "undirected_edges=set(0-4,1-4,2-4,3-4,4-5,4-6,7-4)" << std::endl;
        topology_file.close();
        write_config();
        Ptr<BasicSimulation> basicSimulation = CreateObject<BasicSimulation>("./");
        Ptr<TopologyPtop> topology = CreateObject<TopologyPtop>(basicSimulation, Ipv4ArbiterRoutingHelper());
        remove_file_if_exists("topology.properties.temp");
        basicSimulation->Finalize();

        // Basic sizes
        ASSERT_EQUAL(topology->GetNumNodes(), 8);
        ASSERT_EQUAL(topology->GetNumUndirectedEdges(), 7);
        ASSERT_EQUAL(topology->GetSwitches().size(), 1);
        ASSERT_EQUAL(topology->GetSwitchesWhichAreTors().size(), 1);
        ASSERT_EQUAL(topology->GetServers().size(), 7);
        ASSERT_EQUAL(topology->GetUndirectedEdges().size(), 7);
        ASSERT_EQUAL(topology->GetUndirectedEdgesSet().size(), 7);
        ASSERT_EQUAL(topology->GetAllAdjacencyLists().size(), 8);
        std::set<int64_t> endpoints = topology->GetEndpoints();
        ASSERT_EQUAL(endpoints.size(), 7);

        // Check contents
        for (int i = 0; i < 8; i++) {
            if (i == 4) {
                ASSERT_EQUAL(topology->GetAllAdjacencyLists()[i].size(), 7);
                ASSERT_FALSE(topology->IsValidEndpoint(i));
                ASSERT_TRUE(set_int64_contains(topology->GetSwitches(), i));
                ASSERT_TRUE(set_int64_contains(topology->GetSwitchesWhichAreTors(), i));
                ASSERT_FALSE(set_int64_contains(topology->GetServers(), i));
                ASSERT_FALSE(set_int64_contains(endpoints, i));
                for (int j = 0; j < 8; j++) {
                    if (j != 4) {
                        ASSERT_TRUE(set_int64_contains(topology->GetAllAdjacencyLists()[i], j));
                    }
                }
            } else {
                ASSERT_EQUAL(topology->GetAllAdjacencyLists()[i].size(), 1);
                ASSERT_TRUE(topology->IsValidEndpoint(i));
                ASSERT_FALSE(set_int64_contains(topology->GetSwitches(), i));
                ASSERT_FALSE(set_int64_contains(topology->GetSwitchesWhichAreTors(), i));
                ASSERT_TRUE(set_int64_contains(topology->GetServers(), i));
                ASSERT_TRUE(set_int64_contains(endpoints, i));
                ASSERT_TRUE(set_int64_contains(topology->GetAllAdjacencyLists()[i], 4));
                int a = i > 4 ? 4 : i;
                int b = i > 4 ? i : 4;
                ASSERT_TRUE(set_pair_int64_contains(topology->GetUndirectedEdgesSet(), std::make_pair<int64_t, int64_t>(a, b)));
            }
        }

    }
};

class TopologyLeafSpineTestCase : public TestCase
{
public:
    TopologyLeafSpineTestCase () : TestCase ("topology leaf-spine") {};
    void DoRun () {

        std::ofstream topology_file;
        topology_file.open ("topology.properties.temp");
        topology_file << "num_nodes=49" << std::endl;
        topology_file << "num_undirected_edges=72" << std::endl;
        topology_file << "switches=set(0,1,2,3,4,5,6,7,8,9,10,11,12)" << std::endl;
        topology_file << "switches_which_are_tors=set(4,5,6,7,8,9,10,11,12)" << std::endl;
        topology_file << "servers=set(13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48)" << std::endl;
        topology_file << "undirected_edges=set(4-0,4-1,4-2,4-3,5-0,5-1,5-2,5-3,6-0,6-1,6-2,6-3,7-0,7-1,7-2,7-3,8-0,8-1,8-2,8-3,9-0,9-1,9-2,9-3,10-0,10-1,10-2,10-3,11-0,11-1,11-2,11-3,12-0,12-1,12-2,12-3,13-4,14-4,15-4,16-4,17-5,18-5,19-5,20-5,21-6,22-6,23-6,24-6,25-7,26-7,27-7,28-7,29-8,30-8,31-8,32-8,33-9,34-9,35-9,36-9,37-10,38-10,39-10,40-10,41-11,42-11,43-11,44-11,45-12,46-12,47-12,48-12)" << std::endl;
        topology_file.close();
        write_config();
        Ptr<BasicSimulation> basicSimulation = CreateObject<BasicSimulation>("./");
        Ptr<TopologyPtop> topology = CreateObject<TopologyPtop>(basicSimulation, Ipv4ArbiterRoutingHelper());
        remove_file_if_exists("topology.properties.temp");
        basicSimulation->Finalize();

        // Basic values
        uint32_t num_spines = 4;
        uint32_t num_leafs = 9;
        uint32_t num_servers = num_spines * num_leafs;

        // Basic sizes
        ASSERT_EQUAL(topology->GetNumNodes(), num_spines + num_leafs + num_servers);
        ASSERT_EQUAL(topology->GetNumUndirectedEdges(), num_spines * num_leafs + num_servers);
        ASSERT_EQUAL(topology->GetSwitches().size(), num_spines + num_leafs);
        ASSERT_EQUAL(topology->GetSwitchesWhichAreTors().size(), num_leafs);
        ASSERT_EQUAL(topology->GetServers().size(), num_servers);
        ASSERT_EQUAL(topology->GetUndirectedEdges().size(), num_spines * num_leafs + num_servers);
        ASSERT_EQUAL(topology->GetUndirectedEdgesSet().size(), num_spines * num_leafs + num_servers);
        ASSERT_EQUAL(topology->GetAllAdjacencyLists().size(), num_spines + num_leafs + num_servers);

        // Spines
        for (int i = 0; i < 4; i++) {
            ASSERT_EQUAL(topology->GetAllAdjacencyLists()[i].size(), 9);
            ASSERT_FALSE(topology->IsValidEndpoint(i));
            ASSERT_TRUE(set_int64_contains(topology->GetSwitches(), i));
            ASSERT_FALSE(set_int64_contains(topology->GetSwitchesWhichAreTors(), i));
            ASSERT_FALSE(set_int64_contains(topology->GetServers(), i));
        }

        // Leafs
        for (int i = 4; i < 13; i++) {
            ASSERT_EQUAL(topology->GetAllAdjacencyLists()[i].size(), 8);
            ASSERT_FALSE(topology->IsValidEndpoint(i));
            ASSERT_TRUE(set_int64_contains(topology->GetSwitches(), i));
            ASSERT_TRUE(set_int64_contains(topology->GetSwitchesWhichAreTors(), i));
            ASSERT_FALSE(set_int64_contains(topology->GetServers(), i));
            for (int j = 0; j < 4; j++) {
                ASSERT_TRUE(set_pair_int64_contains(topology->GetUndirectedEdgesSet(), std::make_pair<int64_t, int64_t>(j, i)));
            }
            for (int j = 13 + (i - 4) * 4; j < 13 + (i - 4) * 4 + 4; j++) {
                ASSERT_TRUE(set_pair_int64_contains(topology->GetUndirectedEdgesSet(), std::make_pair<int64_t, int64_t>(i, j)));
            }
        }

        // Servers
        for (int i = 13; i < 49; i++) {
            ASSERT_EQUAL(topology->GetAllAdjacencyLists()[i].size(), 1);
            ASSERT_TRUE(topology->IsValidEndpoint(i));
            ASSERT_FALSE(set_int64_contains(topology->GetSwitches(), i));
            ASSERT_FALSE(set_int64_contains(topology->GetSwitchesWhichAreTors(), i));
            ASSERT_TRUE(set_int64_contains(topology->GetServers(), i));
            int tor = 4 + (i - 13) / 4;
            ASSERT_TRUE(set_int64_contains(topology->GetAllAdjacencyLists()[i], tor));
            ASSERT_TRUE(set_pair_int64_contains(topology->GetUndirectedEdgesSet(), std::make_pair<int64_t, int64_t>(tor, i)));
        }

    }
};

class TopologyRingTestCase : public TestCase
{
public:
    TopologyRingTestCase () : TestCase ("topology ring") {};
    void DoRun () {

        std::ofstream topology_file;
        topology_file.open ("topology.properties.temp");
        topology_file << "num_nodes=4" << std::endl;
        topology_file << "num_undirected_edges=4" << std::endl;
        topology_file << "switches=set(0,1,2,3)" << std::endl;
        topology_file << "switches_which_are_tors=set(0,3)" << std::endl;
        topology_file << "servers=set()" << std::endl;
        topology_file << "undirected_edges=set(0-1,1-3,0-2,2-3)" << std::endl;
        topology_file.close();
        write_config();
        Ptr<BasicSimulation> basicSimulation = CreateObject<BasicSimulation>("./");
        Ptr<TopologyPtop> topology = CreateObject<TopologyPtop>(basicSimulation, Ipv4ArbiterRoutingHelper());
        remove_file_if_exists("topology.properties.temp");
        basicSimulation->Finalize();

        // Basic sizes
        ASSERT_EQUAL(topology->GetNumNodes(), 4);
        ASSERT_EQUAL(topology->GetNumUndirectedEdges(), 4);
        ASSERT_EQUAL(topology->GetSwitches().size(), 4);
        ASSERT_EQUAL(topology->GetSwitchesWhichAreTors().size(), 2);
        ASSERT_EQUAL(topology->GetServers().size(), 0);
        ASSERT_EQUAL(topology->GetUndirectedEdges().size(), 4);
        ASSERT_EQUAL(topology->GetUndirectedEdgesSet().size(), 4);
        ASSERT_EQUAL(topology->GetAllAdjacencyLists().size(), 4);
        ASSERT_EQUAL(topology->GetAllAdjacencyLists()[0].size(), 2);
        ASSERT_EQUAL(topology->GetAllAdjacencyLists()[1].size(), 2);
        ASSERT_EQUAL(topology->GetAllAdjacencyLists()[2].size(), 2);
        ASSERT_EQUAL(topology->GetAllAdjacencyLists()[3].size(), 2);

        // Check contents
        ASSERT_TRUE(topology->IsValidEndpoint(0));
        ASSERT_FALSE(topology->IsValidEndpoint(1));
        ASSERT_FALSE(topology->IsValidEndpoint(2));
        ASSERT_TRUE(topology->IsValidEndpoint(3));
        for (int i = 0; i < 4; i++) {
            ASSERT_EQUAL(topology->GetAllAdjacencyLists()[i].size(), 2);
            ASSERT_TRUE(set_int64_contains(topology->GetSwitches(), i));
            ASSERT_FALSE(set_int64_contains(topology->GetServers(), i));
            if (i == 0 || i == 3) {
                ASSERT_TRUE(set_int64_contains(topology->GetSwitchesWhichAreTors(), i));
                ASSERT_TRUE(set_int64_contains(topology->GetAllAdjacencyLists()[i], 1));
                ASSERT_TRUE(set_int64_contains(topology->GetAllAdjacencyLists()[i], 2));
            } else {
                ASSERT_FALSE(set_int64_contains(topology->GetSwitchesWhichAreTors(), i));
                ASSERT_TRUE(set_int64_contains(topology->GetAllAdjacencyLists()[i], 0));
                ASSERT_TRUE(set_int64_contains(topology->GetAllAdjacencyLists()[i], 3));
            }
        }
        ASSERT_TRUE(set_pair_int64_contains(topology->GetUndirectedEdgesSet(), std::make_pair<int64_t, int64_t>(0, 1)));
        ASSERT_TRUE(set_pair_int64_contains(topology->GetUndirectedEdgesSet(), std::make_pair<int64_t, int64_t>(0, 2)));
        ASSERT_TRUE(set_pair_int64_contains(topology->GetUndirectedEdgesSet(), std::make_pair<int64_t, int64_t>(1, 3)));
        ASSERT_TRUE(set_pair_int64_contains(topology->GetUndirectedEdgesSet(), std::make_pair<int64_t, int64_t>(2, 3)));
        ASSERT_FALSE(set_pair_int64_contains(topology->GetUndirectedEdgesSet(), std::make_pair<int64_t, int64_t>(1, 0)));
        ASSERT_FALSE(set_pair_int64_contains(topology->GetUndirectedEdgesSet(), std::make_pair<int64_t, int64_t>(2, 0)));
        ASSERT_FALSE(set_pair_int64_contains(topology->GetUndirectedEdgesSet(), std::make_pair<int64_t, int64_t>(3, 1)));
        ASSERT_FALSE(set_pair_int64_contains(topology->GetUndirectedEdgesSet(), std::make_pair<int64_t, int64_t>(3, 2)));

    }
};

class TopologyInvalidTestCase : public TestCase
{
public:
    TopologyInvalidTestCase () : TestCase ("topology invalid") {};
    void DoRun () {
        
        write_config();
        Ptr<BasicSimulation> basicSimulation = CreateObject<BasicSimulation>("./");
        
        std::ofstream topology_file;

        // Incorrect number of nodes
        topology_file.open ("topology.properties.temp");
        topology_file << "num_nodes=1" << std::endl;
        topology_file << "num_undirected_edges=1" << std::endl;
        topology_file << "switches=set(0,1)" << std::endl;
        topology_file << "switches_which_are_tors=set(0,1)" << std::endl;
        topology_file << "servers=set()" << std::endl;
        topology_file << "undirected_edges=set(1-0)" << std::endl;
        topology_file.close();
        ASSERT_EXCEPTION(CreateObject<TopologyPtop>(basicSimulation, Ipv4ArbiterRoutingHelper()));
        remove_file_if_exists("topology.properties.temp");

        // Incorrect number of edges
        topology_file.open ("topology.properties.temp");
        topology_file << "num_nodes=3" << std::endl;
        topology_file << "num_undirected_edges=1" << std::endl;
        topology_file << "switches=set(0,1,2)" << std::endl;
        topology_file << "switches_which_are_tors=set(0,1)" << std::endl;
        topology_file << "servers=set()" << std::endl;
        topology_file << "undirected_edges=set(1-0,1-2)" << std::endl;
        topology_file.close();
        ASSERT_EXCEPTION(CreateObject<TopologyPtop>(basicSimulation, Ipv4ArbiterRoutingHelper()));
        remove_file_if_exists("topology.properties.temp");

        // Non-existent node id
        topology_file.open ("topology.properties.temp");
        topology_file << "num_nodes=2" << std::endl;
        topology_file << "num_undirected_edges=1" << std::endl;
        topology_file << "switches=set(0,1)" << std::endl;
        topology_file << "switches_which_are_tors=set(0,1,2)" << std::endl;
        topology_file << "servers=set()" << std::endl;
        topology_file << "undirected_edges=set(1-0)" << std::endl;
        topology_file.close();
        ASSERT_EXCEPTION(CreateObject<TopologyPtop>(basicSimulation, Ipv4ArbiterRoutingHelper()));
        remove_file_if_exists("topology.properties.temp");

        // Not all node ids are covered
        topology_file.open ("topology.properties.temp");
        topology_file << "num_nodes=3" << std::endl;
        topology_file << "num_undirected_edges=1" << std::endl;
        topology_file << "switches=set(0,1)" << std::endl;
        topology_file << "switches_which_are_tors=set(0,1)" << std::endl;
        topology_file << "servers=set()" << std::endl;
        topology_file << "undirected_edges=set(1-0)" << std::endl;
        topology_file.close();
        ASSERT_EXCEPTION(CreateObject<TopologyPtop>(basicSimulation, Ipv4ArbiterRoutingHelper()));
        remove_file_if_exists("topology.properties.temp");

        // Duplicate edge
        topology_file.open ("topology.properties.temp");
        topology_file << "num_nodes=2" << std::endl;
        topology_file << "num_undirected_edges=1" << std::endl;
        topology_file << "switches=set(0,1)" << std::endl;
        topology_file << "switches_which_are_tors=set(0,1)" << std::endl;
        topology_file << "servers=set()" << std::endl;
        topology_file << "undirected_edges=set(1-0,0-1)" << std::endl;
        topology_file.close();
        ASSERT_EXCEPTION(CreateObject<TopologyPtop>(basicSimulation, Ipv4ArbiterRoutingHelper()));
        remove_file_if_exists("topology.properties.temp");

        // Duplicate node id
        topology_file.open ("topology.properties.temp");
        topology_file << "num_nodes=2" << std::endl;
        topology_file << "num_undirected_edges=1" << std::endl;
        topology_file << "switches=set(0,1,1)" << std::endl;
        topology_file << "switches_which_are_tors=set(0,1)" << std::endl;
        topology_file << "servers=set()" << std::endl;
        topology_file << "undirected_edges=set(1-0)" << std::endl;
        topology_file.close();
        ASSERT_EXCEPTION(CreateObject<TopologyPtop>(basicSimulation, Ipv4ArbiterRoutingHelper()));
        remove_file_if_exists("topology.properties.temp");

        // Edge between server and non-ToR switch
        topology_file.open ("topology.properties.temp");
        topology_file << "num_nodes=4" << std::endl;
        topology_file << "num_undirected_edges=3" << std::endl;
        topology_file << "switches=set(0,1)" << std::endl;
        topology_file << "switches_which_are_tors=set(1)" << std::endl;
        topology_file << "servers=set(2,3)" << std::endl;
        topology_file << "undirected_edges=set(1-0,2-1,3-0)" << std::endl;
        topology_file.close();
        ASSERT_EXCEPTION(CreateObject<TopologyPtop>(basicSimulation, Ipv4ArbiterRoutingHelper()));
        remove_file_if_exists("topology.properties.temp");

        // Edge between server and server
        topology_file.open ("topology.properties.temp");
        topology_file << "num_nodes=4" << std::endl;
        topology_file << "num_undirected_edges=3" << std::endl;
        topology_file << "switches=set(0,1)" << std::endl;
        topology_file << "switches_which_are_tors=set(1)" << std::endl;
        topology_file << "servers=set(2,3)" << std::endl;
        topology_file << "undirected_edges=set(1-0,2-1,2-3)" << std::endl;
        topology_file.close();
        ASSERT_EXCEPTION(CreateObject<TopologyPtop>(basicSimulation, Ipv4ArbiterRoutingHelper()));
        remove_file_if_exists("topology.properties.temp");

        // Server marked as ToR
        topology_file.open ("topology.properties.temp");
        topology_file << "num_nodes=4" << std::endl;
        topology_file << "num_undirected_edges=3" << std::endl;
        topology_file << "switches=set(1,2)" << std::endl;
        topology_file << "switches_which_are_tors=set(1,2,3)" << std::endl;
        topology_file << "servers=set(0,3)" << std::endl;
        topology_file << "undirected_edges=set(0-1,1-2,2-3)" << std::endl;
        topology_file.close();
        ASSERT_EXCEPTION(CreateObject<TopologyPtop>(basicSimulation, Ipv4ArbiterRoutingHelper()));
        remove_file_if_exists("topology.properties.temp");

        // Servers and switches not distinct
        topology_file.open ("topology.properties.temp");
        topology_file << "num_nodes=4" << std::endl;
        topology_file << "num_undirected_edges=3" << std::endl;
        topology_file << "switches=set(1,2)" << std::endl;
        topology_file << "switches_which_are_tors=set(1,2)" << std::endl;
        topology_file << "servers=set(0,3,1)" << std::endl;
        topology_file << "undirected_edges=set(0-1,1-2,2-3)" << std::endl;
        topology_file.close();
        ASSERT_EXCEPTION(CreateObject<TopologyPtop>(basicSimulation, Ipv4ArbiterRoutingHelper()));
        remove_file_if_exists("topology.properties.temp");

        // Edge to itself
        topology_file.open ("topology.properties.temp");
        topology_file << "num_nodes=4" << std::endl;
        topology_file << "num_undirected_edges=3" << std::endl;
        topology_file << "switches=set(1,2)" << std::endl;
        topology_file << "switches_which_are_tors=set(1,2)" << std::endl;
        topology_file << "servers=set(0,3)" << std::endl;
        topology_file << "undirected_edges=set(0-1,1-2,2-2)" << std::endl;
        topology_file.close();
        ASSERT_EXCEPTION(CreateObject<TopologyPtop>(basicSimulation, Ipv4ArbiterRoutingHelper()));
        remove_file_if_exists("topology.properties.temp");

        // Edge left out of bound
        topology_file.open ("topology.properties.temp");
        topology_file << "num_nodes=4" << std::endl;
        topology_file << "num_undirected_edges=3" << std::endl;
        topology_file << "switches=set(1,2)" << std::endl;
        topology_file << "switches_which_are_tors=set(1,2)" << std::endl;
        topology_file << "servers=set(0,3)" << std::endl;
        topology_file << "undirected_edges=set(0-1,1-2,2-4)" << std::endl;
        topology_file.close();
        ASSERT_EXCEPTION(CreateObject<TopologyPtop>(basicSimulation, Ipv4ArbiterRoutingHelper()));
        remove_file_if_exists("topology.properties.temp");

        // Edge right out of bound
        topology_file.open ("topology.properties.temp");
        topology_file << "num_nodes=4" << std::endl;
        topology_file << "num_undirected_edges=3" << std::endl;
        topology_file << "switches=set(1,2)" << std::endl;
        topology_file << "switches_which_are_tors=set(1,2)" << std::endl;
        topology_file << "servers=set(0,3)" << std::endl;
        topology_file << "undirected_edges=set(0-1,1-2,4-3)" << std::endl;
        topology_file.close();
        ASSERT_EXCEPTION(CreateObject<TopologyPtop>(basicSimulation, Ipv4ArbiterRoutingHelper()));
        remove_file_if_exists("topology.properties.temp");

        // Duplicate edges
        topology_file.open ("topology.properties.temp");
        topology_file << "num_nodes=4" << std::endl;
        topology_file << "num_undirected_edges=4" << std::endl;
        topology_file << "switches=set(1,2)" << std::endl;
        topology_file << "switches_which_are_tors=set(1,2)" << std::endl;
        topology_file << "servers=set(0,3)" << std::endl;
        topology_file << "undirected_edges=set(0-1,1-2,2-3,2 -3)" << std::endl;
        topology_file.close();
        ASSERT_EXCEPTION(CreateObject<TopologyPtop>(basicSimulation, Ipv4ArbiterRoutingHelper()));
        remove_file_if_exists("topology.properties.temp");

        basicSimulation->Finalize();

    }
};

////////////////////////////////////////////////////////////////////////////////////////
