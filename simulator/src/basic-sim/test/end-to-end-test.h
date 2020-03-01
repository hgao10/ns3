/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/basic-sim.h"
#include "ns3/test.h"
#include "test-helpers.h"
#include <iostream>
#include <fstream>

using namespace ns3;

////////////////////////////////////////////////////////////////////////////////////////

class EndToEndTestCase : public TestCase {
public:
    EndToEndTestCase(std::string s) : TestCase(s) {};
    const std::string temp_dir = ".testtmpdir";

    void prepare_test_dir() {
        mkdir_if_not_exists(temp_dir);
        remove_file_if_exists(temp_dir + "/config.properties");
        remove_file_if_exists(temp_dir + "/topology.properties");
        remove_file_if_exists(temp_dir + "/schedule.csv");
    }

    void write_basic_config(int64_t simulation_end_time_ns, int64_t simulation_seed, double link_data_rate_megabit_per_s, int64_t link_delay_ns) {
        std::ofstream config_file;
        config_file.open (temp_dir + "/config.properties");
        config_file << "filename_topology=\"topology.properties\"" << std::endl;
        config_file << "filename_schedule=\"schedule.csv\"" << std::endl;
        config_file << "simulation_end_time_ns=" << simulation_end_time_ns << std::endl;
        config_file << "simulation_seed=" << simulation_seed << std::endl;
        config_file << "link_data_rate_megabit_per_s=" << link_data_rate_megabit_per_s << std::endl;
        config_file << "link_delay_ns=" << link_delay_ns << std::endl;
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

    void test_run_and_simple_validate(int64_t simulation_end_time_ns, std::string temp_dir, std::vector<schedule_entry> write_schedule, std::vector<int64_t>& end_time_ns_list, std::vector<int64_t>& sent_byte_list) {

        // Make sure these are removed
        remove_file_if_exists(temp_dir + "/logs/finished.txt");
        remove_file_if_exists(temp_dir + "/logs/flows.txt");
        remove_file_if_exists(temp_dir + "/logs/flows.csv");

        // Write schedule file
        std::ofstream schedule_file;
        schedule_file.open (temp_dir + "/schedule.csv");
        for (schedule_entry entry : write_schedule) {
            schedule_file
                    << entry.flow_id << ","
                    << entry.from_node_id << ","
                    << entry.to_node_id << ","
                    << entry.size_byte << ","
                    << entry.start_time_ns << ","
                    << entry.additional_parameters << ","
                    << entry.metadata
                    << std::endl;
        }
        schedule_file.close();

        // Perform basic simulation
        basic_sim(temp_dir);

        // Check finished.txt
        std::vector<std::string> finished_lines = read_file_direct(temp_dir + "/logs/finished.txt");
        ASSERT_EQUAL(finished_lines.size(), 1);
        ASSERT_EQUAL(finished_lines[0], "Yes");

        // Check flows.csv
        std::vector<std::string> lines_csv = read_file_direct(temp_dir + "/logs/flows.csv");
        ASSERT_EQUAL(lines_csv.size(), write_schedule.size());
        int i = 0;
        for (std::string line : lines_csv) {
            std::vector<std::string> line_spl = split_string(line, ",");
            ASSERT_EQUAL(line_spl.size(), 10);
            ASSERT_EQUAL(parse_positive_int64(line_spl[0]), i);
            ASSERT_EQUAL(parse_positive_int64(line_spl[1]), write_schedule[i].from_node_id);
            ASSERT_EQUAL(parse_positive_int64(line_spl[2]), write_schedule[i].to_node_id);
            ASSERT_EQUAL(parse_positive_int64(line_spl[3]), write_schedule[i].size_byte);
            ASSERT_EQUAL(parse_positive_int64(line_spl[4]), write_schedule[i].start_time_ns);
            int64_t end_time_ns = parse_positive_int64(line_spl[5]);
            ASSERT_TRUE(end_time_ns >= 0 && end_time_ns <= simulation_end_time_ns);
            ASSERT_EQUAL(parse_positive_int64(line_spl[6]), end_time_ns - write_schedule[i].start_time_ns);
            int64_t sent_byte = parse_positive_int64(line_spl[7]);
            ASSERT_TRUE(sent_byte >= 0 && sent_byte <= simulation_end_time_ns);
            ASSERT_TRUE(line_spl[8] == "DNF" || line_spl[8] == "YES");
            ASSERT_EQUAL(line_spl[9], write_schedule[i].metadata);
            end_time_ns_list.push_back(end_time_ns);
            sent_byte_list.push_back(sent_byte);
            i++;
        }

        // Check flows.txt
        std::vector<std::string> lines_txt = read_file_direct(temp_dir + "/logs/flows.txt");
        ASSERT_EQUAL(lines_txt.size(), write_schedule.size() + 1);
        i = 0;
        for (std::string line : lines_txt) {
            if (i == 0) {
                ASSERT_EQUAL(
                        line,
                        "Flow ID     Source    Target    Size            Start time (ns)   End time (ns)     Duration        Sent            Progress     Avg. rate       Finished?   Metadata"
                        );
            } else {
                int j = i - 1;
                std::vector<std::string> line_spl;
                std::istringstream iss(line);
                for (std::string s; iss >> s;) {
                    line_spl.push_back(s);
                }
                ASSERT_TRUE(line_spl.size() == 15 || line_spl.size() == 16);
                ASSERT_EQUAL(parse_positive_int64(line_spl[0]), j);
                ASSERT_EQUAL(parse_positive_int64(line_spl[1]), write_schedule[j].from_node_id);
                ASSERT_EQUAL(parse_positive_int64(line_spl[2]), write_schedule[j].to_node_id);
                ASSERT_EQUAL(parse_positive_int64(line_spl[3]), byte_to_megabit(write_schedule[j].size_byte));
                ASSERT_EQUAL(line_spl[4], "Mbit");
                ASSERT_EQUAL(parse_positive_int64(line_spl[5]), write_schedule[j].start_time_ns);
                ASSERT_EQUAL(parse_positive_int64(line_spl[6]), end_time_ns_list[j]);
                ASSERT_EQUAL_APPROX(parse_positive_double(line_spl[7]), nanosec_to_millisec(end_time_ns_list[j] - write_schedule[j].start_time_ns), 0.01);
                ASSERT_EQUAL(line_spl[8], "ms");
                ASSERT_EQUAL_APPROX(parse_positive_double(line_spl[9]), byte_to_megabit(sent_byte_list[j]), 0.1);
                ASSERT_EQUAL(line_spl[10], "Mbit");
                ASSERT_EQUAL_APPROX(parse_positive_double(line_spl[11].substr(0, line_spl.size() - 1)), sent_byte_list[j] * 100.0 / write_schedule[j].size_byte, 0.1);
                ASSERT_EQUAL_APPROX(parse_positive_double(line_spl[12]), byte_to_megabit(sent_byte_list[j]) / nanosec_to_sec(end_time_ns_list[j] - write_schedule[j].start_time_ns), 0.1);
                ASSERT_EQUAL(line_spl[13], "Mbit/s");
                ASSERT_TRUE(line_spl[14] == "DNF" || line_spl[14] == "YES");
                if (line_spl.size() == 16) {
                    ASSERT_EQUAL(line_spl[15], write_schedule[j].metadata);
                }
            }
            i++;
        }

        // Make sure these are removed
        remove_file_if_exists(temp_dir + "/config.properties");
        remove_file_if_exists(temp_dir + "/topology.properties");
        remove_file_if_exists(temp_dir + "/schedule.csv");

    }

};

class EndToEndOneToOneEqualStartTestCase : public EndToEndTestCase
{
public:
    EndToEndOneToOneEqualStartTestCase () : EndToEndTestCase ("end-to-end 1-to-1 equal-start") {};

    void DoRun () {
        prepare_test_dir();

        int64_t simulation_end_time_ns = 5000000000;

        // One-to-one, 5s, 10.0 Mbit/s, 10 microseconds delay
        write_basic_config(simulation_end_time_ns, 123456, 10.0, 100000);
        write_single_topology();

        // A flow each way
        std::vector<schedule_entry> schedule;
        schedule.push_back({0, 0, 1, 1000000, 0, "", "abc"});
        schedule.push_back({1, 1, 0, 1000000, 0, "", ""});

        // Perform the run
        std::vector<int64_t> end_time_ns_list;
        std::vector<int64_t> sent_byte_list;
        test_run_and_simple_validate(simulation_end_time_ns, temp_dir, schedule, end_time_ns_list, sent_byte_list);

        // As they are started at the same point and should have the same behavior, progress should be equal
        ASSERT_EQUAL(end_time_ns_list[0], end_time_ns_list[1]);
        ASSERT_EQUAL(sent_byte_list[0], sent_byte_list[1]);

    }
};

class EndToEndOneToOneApartStartTestCase : public EndToEndTestCase
{
public:
    EndToEndOneToOneApartStartTestCase () : EndToEndTestCase ("end-to-end 1-to-1 apart-start") {};

    void DoRun () {
        prepare_test_dir();

        int64_t simulation_end_time_ns = 10000000000;

        // One-to-one, 10s, 10.0 Mbit/s, 10 microseconds delay
        write_basic_config(simulation_end_time_ns, 654321, 10.0, 100000);
        write_single_topology();

        // A flow each way
        std::vector<schedule_entry> schedule;
        schedule.push_back({0, 0, 1, 1000000, 0, "", "first"});
        schedule.push_back({1, 0, 1, 1000000, 5000000000, "", "second"});

        // Perform the run
        std::vector<int64_t> end_time_ns_list;
        std::vector<int64_t> sent_byte_list;
        test_run_and_simple_validate(simulation_end_time_ns, temp_dir, schedule, end_time_ns_list, sent_byte_list);

        // As they are started without any interference, should have same completion
        ASSERT_EQUAL(end_time_ns_list[0], end_time_ns_list[1] - 5000000000);
        ASSERT_EQUAL(sent_byte_list[0], sent_byte_list[1]);

    }
};

class EndToEndEcmpSimpleTestCase : public EndToEndTestCase
{
public:
    EndToEndEcmpSimpleTestCase () : EndToEndTestCase ("end-to-end ecmp-simple") {};

    void DoRun () {
        prepare_test_dir();

        int64_t simulation_end_time_ns = 2000000000;

        // One-to-one, 5s, 30.0 Mbit/s, 2 ms delay
        write_basic_config(simulation_end_time_ns, 123456, 30.0, 2000000);
        std::ofstream topology_file;
        topology_file.open (temp_dir + "/topology.properties");
        topology_file << "num_nodes=4" << std::endl;
        topology_file << "num_undirected_edges=4" << std::endl;
        topology_file << "switches=set(0,1,2,3)" << std::endl;
        topology_file << "switches_which_are_tors=set(0,3)" << std::endl;
        topology_file << "servers=set()" << std::endl;
        topology_file << "undirected_edges=set(0-1,0-2,1-3,3-2)" << std::endl;
        topology_file.close();

        // A flow each way
        std::vector<schedule_entry> schedule;
        for (int i = 0; i < 30; i++) {
            schedule.push_back({i, 0, 3, 100000000, 0, "", ""});
        }

        // Perform the run
        std::vector<int64_t> end_time_ns_list;
        std::vector<int64_t> sent_byte_list;
        test_run_and_simple_validate(simulation_end_time_ns, temp_dir, schedule, end_time_ns_list, sent_byte_list);

        // All are too large to end, and they must consume bandwidth of both links
        int64_t byte_sum = 0;
        for (int i = 0; i < 30; i++) {
            ASSERT_EQUAL(end_time_ns_list[i], simulation_end_time_ns);
            byte_sum += sent_byte_list[i];
        }
        ASSERT_TRUE(byte_to_megabit(byte_sum) / nanosec_to_sec(simulation_end_time_ns) >= 45.0); // At least 45 Mbit/s, given that probability of all of them going up is 0.5^30

    }
};

////////////////////////////////////////////////////////////////////////////////////////
