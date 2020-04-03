/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/basic-sim.h"
#include "ns3/test.h"
#include "test-helpers.h"

using namespace ns3;

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
        Topology topology = Topology("topology.properties.temp");
        ASSERT_EQUAL(topology.num_nodes, 0);
        ASSERT_EQUAL(topology.num_undirected_edges, 0);
        ASSERT_EQUAL(topology.switches.size(), 0);
        ASSERT_EQUAL(topology.switches_which_are_tors.size(), 0);
        ASSERT_EQUAL(topology.servers.size(), 0);
        ASSERT_EQUAL(topology.undirected_edges.size(), 0);
        ASSERT_EQUAL(topology.undirected_edges_set.size(), 0);
        ASSERT_EQUAL(topology.adjacency_list.size(), 0);
        remove_file_if_exists("topology.properties.temp");
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
        Topology topology = Topology("topology.properties.temp");
        remove_file_if_exists("topology.properties.temp");

        // Basic sizes
        ASSERT_EQUAL(topology.num_nodes, 2);
        ASSERT_EQUAL(topology.num_undirected_edges, 1);
        ASSERT_EQUAL(topology.switches.size(), 2);
        ASSERT_EQUAL(topology.switches_which_are_tors.size(), 2);
        ASSERT_EQUAL(topology.servers.size(), 0);
        ASSERT_EQUAL(topology.undirected_edges.size(), 1);
        ASSERT_EQUAL(topology.undirected_edges_set.size(), 1);
        ASSERT_EQUAL(topology.adjacency_list.size(), 2);
        ASSERT_EQUAL(topology.adjacency_list[0].size(), 1);
        ASSERT_EQUAL(topology.adjacency_list[1].size(), 1);

        // Check contents
        ASSERT_TRUE(topology.is_valid_flow_endpoint(0));
        ASSERT_TRUE(topology.is_valid_flow_endpoint(1));
        ASSERT_TRUE(set_int64_contains(topology.switches, 0));
        ASSERT_TRUE(set_int64_contains(topology.switches, 1));
        ASSERT_TRUE(set_int64_contains(topology.switches_which_are_tors, 0));
        ASSERT_TRUE(set_int64_contains(topology.switches_which_are_tors, 1));
        ASSERT_EQUAL(topology.undirected_edges[0].first, 0);
        ASSERT_EQUAL(topology.undirected_edges[0].second, 1);
        ASSERT_TRUE(set_pair_int64_contains(topology.undirected_edges_set, std::make_pair<int64_t, int64_t>(0, 1)));
        ASSERT_TRUE(set_int64_contains(topology.adjacency_list[0], 1));
        ASSERT_TRUE(set_int64_contains(topology.adjacency_list[1], 0));

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
        Topology topology = Topology("topology.properties.temp");
        remove_file_if_exists("topology.properties.temp");

        // Basic sizes
        ASSERT_EQUAL(topology.num_nodes, 8);
        ASSERT_EQUAL(topology.num_undirected_edges, 7);
        ASSERT_EQUAL(topology.switches.size(), 1);
        ASSERT_EQUAL(topology.switches_which_are_tors.size(), 1);
        ASSERT_EQUAL(topology.servers.size(), 7);
        ASSERT_EQUAL(topology.undirected_edges.size(), 7);
        ASSERT_EQUAL(topology.undirected_edges_set.size(), 7);
        ASSERT_EQUAL(topology.adjacency_list.size(), 8);

        // Check contents
        for (int i = 0; i < 8; i++) {
            if (i == 4) {
                ASSERT_EQUAL(topology.adjacency_list[i].size(), 7);
                ASSERT_FALSE(topology.is_valid_flow_endpoint(i));
                ASSERT_TRUE(set_int64_contains(topology.switches, i));
                ASSERT_TRUE(set_int64_contains(topology.switches_which_are_tors, i));
                ASSERT_FALSE(set_int64_contains(topology.servers, i));
                for (int j = 0; j < 8; j++) {
                    if (j != 4) {
                        ASSERT_TRUE(set_int64_contains(topology.adjacency_list[i], j));
                    }
                }
            } else {
                ASSERT_EQUAL(topology.adjacency_list[i].size(), 1);
                ASSERT_TRUE(topology.is_valid_flow_endpoint(i));
                ASSERT_FALSE(set_int64_contains(topology.switches, i));
                ASSERT_FALSE(set_int64_contains(topology.switches_which_are_tors, i));
                ASSERT_TRUE(set_int64_contains(topology.servers, i));
                ASSERT_TRUE(set_int64_contains(topology.adjacency_list[i], 4));
                int a = i > 4 ? 4 : i;
                int b = i > 4 ? i : 4;
                ASSERT_TRUE(set_pair_int64_contains(topology.undirected_edges_set, std::make_pair<int64_t, int64_t>(a, b)));
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
        Topology topology = Topology("topology.properties.temp");
        remove_file_if_exists("topology.properties.temp");

        // Basic values
        uint32_t num_spines = 4;
        uint32_t num_leafs = 9;
        uint32_t num_servers = num_spines * num_leafs;

        // Basic sizes
        ASSERT_EQUAL(topology.num_nodes, num_spines + num_leafs + num_servers);
        ASSERT_EQUAL(topology.num_undirected_edges, num_spines * num_leafs + num_servers);
        ASSERT_EQUAL(topology.switches.size(), num_spines + num_leafs);
        ASSERT_EQUAL(topology.switches_which_are_tors.size(), num_leafs);
        ASSERT_EQUAL(topology.servers.size(), num_servers);
        ASSERT_EQUAL(topology.undirected_edges.size(), num_spines * num_leafs + num_servers);
        ASSERT_EQUAL(topology.undirected_edges_set.size(), num_spines * num_leafs + num_servers);
        ASSERT_EQUAL(topology.adjacency_list.size(), num_spines + num_leafs + num_servers);

        // Spines
        for (int i = 0; i < 4; i++) {
            ASSERT_EQUAL(topology.adjacency_list[i].size(), 9);
            ASSERT_FALSE(topology.is_valid_flow_endpoint(i));
            ASSERT_TRUE(set_int64_contains(topology.switches, i));
            ASSERT_FALSE(set_int64_contains(topology.switches_which_are_tors, i));
            ASSERT_FALSE(set_int64_contains(topology.servers, i));
        }

        // Leafs
        for (int i = 4; i < 13; i++) {
            ASSERT_EQUAL(topology.adjacency_list[i].size(), 8);
            ASSERT_FALSE(topology.is_valid_flow_endpoint(i));
            ASSERT_TRUE(set_int64_contains(topology.switches, i));
            ASSERT_TRUE(set_int64_contains(topology.switches_which_are_tors, i));
            ASSERT_FALSE(set_int64_contains(topology.servers, i));
            for (int j = 0; j < 4; j++) {
                ASSERT_TRUE(set_pair_int64_contains(topology.undirected_edges_set, std::make_pair<int64_t, int64_t>(j, i)));
            }
            for (int j = 13 + (i - 4) * 4; j < 13 + (i - 4) * 4 + 4; j++) {
                ASSERT_TRUE(set_pair_int64_contains(topology.undirected_edges_set, std::make_pair<int64_t, int64_t>(i, j)));
            }
        }

        // Servers
        for (int i = 13; i < 49; i++) {
            ASSERT_EQUAL(topology.adjacency_list[i].size(), 1);
            ASSERT_TRUE(topology.is_valid_flow_endpoint(i));
            ASSERT_FALSE(set_int64_contains(topology.switches, i));
            ASSERT_FALSE(set_int64_contains(topology.switches_which_are_tors, i));
            ASSERT_TRUE(set_int64_contains(topology.servers, i));
            int tor = 4 + (i - 13) / 4;
            ASSERT_TRUE(set_int64_contains(topology.adjacency_list[i], tor));
            ASSERT_TRUE(set_pair_int64_contains(topology.undirected_edges_set, std::make_pair<int64_t, int64_t>(tor, i)));
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
        Topology topology = Topology("topology.properties.temp");
        remove_file_if_exists("topology.properties.temp");

        // Basic sizes
        ASSERT_EQUAL(topology.num_nodes, 4);
        ASSERT_EQUAL(topology.num_undirected_edges, 4);
        ASSERT_EQUAL(topology.switches.size(), 4);
        ASSERT_EQUAL(topology.switches_which_are_tors.size(), 2);
        ASSERT_EQUAL(topology.servers.size(), 0);
        ASSERT_EQUAL(topology.undirected_edges.size(), 4);
        ASSERT_EQUAL(topology.undirected_edges_set.size(), 4);
        ASSERT_EQUAL(topology.adjacency_list.size(), 4);
        ASSERT_EQUAL(topology.adjacency_list[0].size(), 2);
        ASSERT_EQUAL(topology.adjacency_list[1].size(), 2);
        ASSERT_EQUAL(topology.adjacency_list[2].size(), 2);
        ASSERT_EQUAL(topology.adjacency_list[3].size(), 2);

        // Check contents
        ASSERT_TRUE(topology.is_valid_flow_endpoint(0));
        ASSERT_FALSE(topology.is_valid_flow_endpoint(1));
        ASSERT_FALSE(topology.is_valid_flow_endpoint(2));
        ASSERT_TRUE(topology.is_valid_flow_endpoint(3));
        for (int i = 0; i < 4; i++) {
            ASSERT_EQUAL(topology.adjacency_list[i].size(), 2);
            ASSERT_TRUE(set_int64_contains(topology.switches, i));
            ASSERT_FALSE(set_int64_contains(topology.servers, i));
            if (i == 0 || i == 3) {
                ASSERT_TRUE(set_int64_contains(topology.switches_which_are_tors, i));
                ASSERT_TRUE(set_int64_contains(topology.adjacency_list[i], 1));
                ASSERT_TRUE(set_int64_contains(topology.adjacency_list[i], 2));
            } else {
                ASSERT_FALSE(set_int64_contains(topology.switches_which_are_tors, i));
                ASSERT_TRUE(set_int64_contains(topology.adjacency_list[i], 0));
                ASSERT_TRUE(set_int64_contains(topology.adjacency_list[i], 3));
            }
        }
        ASSERT_TRUE(set_pair_int64_contains(topology.undirected_edges_set, std::make_pair<int64_t, int64_t>(0, 1)));
        ASSERT_TRUE(set_pair_int64_contains(topology.undirected_edges_set, std::make_pair<int64_t, int64_t>(0, 2)));
        ASSERT_TRUE(set_pair_int64_contains(topology.undirected_edges_set, std::make_pair<int64_t, int64_t>(1, 3)));
        ASSERT_TRUE(set_pair_int64_contains(topology.undirected_edges_set, std::make_pair<int64_t, int64_t>(2, 3)));
        ASSERT_FALSE(set_pair_int64_contains(topology.undirected_edges_set, std::make_pair<int64_t, int64_t>(1, 0)));
        ASSERT_FALSE(set_pair_int64_contains(topology.undirected_edges_set, std::make_pair<int64_t, int64_t>(2, 0)));
        ASSERT_FALSE(set_pair_int64_contains(topology.undirected_edges_set, std::make_pair<int64_t, int64_t>(3, 1)));
        ASSERT_FALSE(set_pair_int64_contains(topology.undirected_edges_set, std::make_pair<int64_t, int64_t>(3, 2)));

    }
};

class TopologyInvalidTestCase : public TestCase
{
public:
    TopologyInvalidTestCase () : TestCase ("topology invalid") {};
    void DoRun () {

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
        ASSERT_EXCEPTION(Topology("topology.properties.temp"));
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
        ASSERT_EXCEPTION(Topology("topology.properties.temp"));
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
        ASSERT_EXCEPTION(Topology("topology.properties.temp"));
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
        ASSERT_EXCEPTION(Topology("topology.properties.temp"));
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
        ASSERT_EXCEPTION(Topology("topology.properties.temp"));
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
        ASSERT_EXCEPTION(Topology("topology.properties.temp"));
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
        ASSERT_EXCEPTION(Topology("topology.properties.temp"));
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
        ASSERT_EXCEPTION(Topology("topology.properties.temp"));
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
        ASSERT_EXCEPTION(Topology("topology.properties.temp"));
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
        ASSERT_EXCEPTION(Topology("topology.properties.temp"));
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
        ASSERT_EXCEPTION(Topology("topology.properties.temp"));
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
        ASSERT_EXCEPTION(Topology("topology.properties.temp"));
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
        ASSERT_EXCEPTION(Topology("topology.properties.temp"));
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
        ASSERT_EXCEPTION(Topology("topology.properties.temp"));
        remove_file_if_exists("topology.properties.temp");

    }
};

////////////////////////////////////////////////////////////////////////////////////////
