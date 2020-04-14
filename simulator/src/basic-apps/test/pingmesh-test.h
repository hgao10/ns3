/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/basic-simulation.h"
#include "ns3/flow-scheduler.h"
#include "ns3/test.h"
#include "test-helpers.h"
#include <iostream>
#include <fstream>
#include "ns3/pingmesh-scheduler.h"

using namespace ns3;

////////////////////////////////////////////////////////////////////////////////////////

class PingmeshTestCase : public TestCase {
public:
    PingmeshTestCase(std::string s) : TestCase(s) {};
    const std::string temp_dir = ".testtmpdir";

    void prepare_test_dir() {
        mkdir_if_not_exists(temp_dir);
        remove_file_if_exists(temp_dir + "/config_ns3.properties");
        remove_file_if_exists(temp_dir + "/topology.properties");
    }

    void write_basic_config(int64_t simulation_end_time_ns, int64_t simulation_seed, double link_data_rate_megabit_per_s, int64_t link_delay_ns, int64_t pingmesh_interval_ns) {
        std::ofstream config_file;
        config_file.open (temp_dir + "/config_ns3.properties");
        config_file << "filename_topology=\"topology.properties\"" << std::endl;
        config_file << "pingmesh_interval_ns=" << pingmesh_interval_ns << std::endl;
        config_file << "simulation_end_time_ns=" << simulation_end_time_ns << std::endl;
        config_file << "simulation_seed=" << simulation_seed << std::endl;
        config_file << "link_data_rate_megabit_per_s=" << link_data_rate_megabit_per_s << std::endl;
        config_file << "link_delay_ns=" << link_delay_ns << std::endl;
        config_file << "link_max_queue_size_pkts=100" << std::endl;
        config_file << "disable_qdisc_endpoint_tors_xor_servers=false" << std::endl;
        config_file << "disable_qdisc_non_endpoint_switches=false" << std::endl;
        config_file.close();
    }

    void write_single_topology() {
        std::ofstream topology_file;
        topology_file.open (temp_dir + "/topology.properties");
        topology_file << "num_nodes=2" << std::endl;
        topology_file << "num_undirected_edges=1" << std::endl;
        topology_file << "switches=set(0,1)" << std::endl;
        topology_file << "switches_which_are_tors=set(0,1)" << std::endl;
        topology_file << "servers=set()" << std::endl;
        topology_file << "undirected_edges=set(0-1)" << std::endl;
        topology_file.close();
    }

    void test_run_and_simple_validate(int64_t simulation_end_time_ns, std::string temp_dir, uint32_t expected_num_pings) {

        // Make sure these are removed
        remove_file_if_exists(temp_dir + "/logs_ns3/finished.txt");
        remove_file_if_exists(temp_dir + "/logs_ns3/pingmesh.txt");
        remove_file_if_exists(temp_dir + "/logs_ns3/pingmesh.csv");

        // Perform basic simulation
        BasicSimulation simulation(temp_dir);
        PingmeshScheduler pingmeshScheduler(&simulation); // Requires filename_schedule to be present in the configuration
        pingmeshScheduler.Schedule();
        simulation.Run();
        pingmeshScheduler.WriteResults();
        simulation.Finalize();

        // Check finished.txt
        std::vector<std::string> finished_lines = read_file_direct(temp_dir + "/logs_ns3/finished.txt");
        ASSERT_EQUAL(finished_lines.size(), 1);
        ASSERT_EQUAL(finished_lines[0], "Yes");

        // Check pingmesh.csv
        std::vector<std::string> lines_csv = read_file_direct(temp_dir + "/logs_ns3/pingmesh.csv");
        ASSERT_EQUAL(lines_csv.size(), expected_num_pings);
        // TODO: more in-depth checking
        // TODO: check pingmesh.txt

        // Make sure these are removed
        remove_file_if_exists(temp_dir + "/config_ns3.properties");
        remove_file_if_exists(temp_dir + "/topology.properties");

    }

};

class PingmeshOneToOneTestCase : public PingmeshTestCase
{
public:
    PingmeshOneToOneTestCase () : PingmeshTestCase ("pingmesh 1-to-1") {};

    void DoRun () {
        prepare_test_dir();

        int64_t simulation_end_time_ns = 5000000000;

        // One-to-one, 5s, 10.0 Mbit/s, 100 microseconds delay, 1 ping per 100ms
        write_basic_config(simulation_end_time_ns, 123456, 10.0, 100000, 100000000);
        write_single_topology();

        // Perform the run
        test_run_and_simple_validate(simulation_end_time_ns, temp_dir, 100);

    }
};

////////////////////////////////////////////////////////////////////////////////////////
