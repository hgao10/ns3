/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/basic-sim.h"
#include "ns3/test.h"
#include "test-helpers.h"

using namespace ns3;

////////////////////////////////////////////////////////////////////////////////////////

class TopologyTestCase : public TestCase
{
public:
    TopologyTestCase () : TestCase ("topology") {};
    void DoRun () {
        // TODO: Topology switches and ToRs
        // TODO: Topology switches, ToRs and servers

        printf("Here\n");
        // Empty topology
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

        // TODO: Topology invalid cases
    }
};

////////////////////////////////////////////////////////////////////////////////////////
