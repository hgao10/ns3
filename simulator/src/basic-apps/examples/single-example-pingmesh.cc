/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <iostream>
#include <fstream>

#include "ns3/basic-simulation.h"
#include "ns3/pingmesh-scheduler.h"
#include "ns3/topology-ptop.h"
#include "ns3/tcp-optimizer.h"
#include "ns3/arbiter-ecmp-helper.h"
#include "ns3/ipv4-arbiter-routing-helper.h"

using namespace ns3;

int main(int argc, char *argv[]) {

    // Prepare run directory
    const std::string example_dir = "example";
    mkdir_if_not_exists(example_dir);
    remove_file_if_exists(example_dir + "/config_ns3.properties");
    remove_file_if_exists(example_dir + "/topology.properties");
    remove_file_if_exists(example_dir + "/schedule.csv");

    // Write config file
    std::ofstream config_file;
    config_file.open (example_dir + "/config_ns3.properties");
    config_file << "filename_topology=\"topology.properties\"" << std::endl;
    config_file << "pingmesh_interval_ns=100000000" << std::endl;
    config_file << "simulation_end_time_ns=1000000000" << std::endl;
    config_file << "simulation_seed=123456789" << std::endl;
    config_file << "link_data_rate_megabit_per_s=100" << std::endl;
    config_file << "link_delay_ns=10000"<< std::endl;
    config_file << "link_max_queue_size_pkts=100" << std::endl;
    config_file << "disable_qdisc_endpoint_tors_xor_servers=false" << std::endl;
    config_file << "disable_qdisc_non_endpoint_switches=false" << std::endl;
    config_file.close();

    // Write topology file (0 - 1)
    std::ofstream topology_file;
    topology_file.open (example_dir + "/topology.properties");
    topology_file << "num_nodes=2" << std::endl;
    topology_file << "num_undirected_edges=1" << std::endl;
    topology_file << "switches=set(0,1)" << std::endl;
    topology_file << "switches_which_are_tors=set(0,1)" << std::endl;
    topology_file << "servers=set()" << std::endl;
    topology_file << "undirected_edges=set(0-1)" << std::endl;
    topology_file.close();

    // Load basic simulation environment
    Ptr<BasicSimulation> basicSimulation = CreateObject<BasicSimulation>(example_dir);

    // Read point-to-point topology, and install routing arbiters
    Ptr<TopologyPtop> topology = CreateObject<TopologyPtop>(basicSimulation, Ipv4ArbiterRoutingHelper());
    ArbiterEcmpHelper::InstallArbiters(basicSimulation, topology);

    // Schedule pings
    PingmeshScheduler pingmeshScheduler(basicSimulation, topology); // Requires pingmesh_interval_ns to be present in the configuration
    pingmeshScheduler.Schedule();

    // Run simulation
    basicSimulation->Run();

    // Write results
    pingmeshScheduler.WriteResults();

    // Finalize the simulation
    basicSimulation->Finalize();

    return 0;
}
