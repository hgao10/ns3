/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/basic-simulation.h"
#include "ns3/test.h"
#include "test-helpers.h"

using namespace ns3;

////////////////////////////////////////////////////////////////////////////////////////

class ScheduleReaderNormalTestCase : public TestCase
{
public:
    ScheduleReaderNormalTestCase () : TestCase ("schedule-reader normal") {};
    const std::string temp_dir = ".testtmpdir";

    void DoRun () {
        mkdir_if_not_exists(temp_dir);

        // Normal

        std::ofstream schedule_file(temp_dir + "/schedule.csv.temp");
        schedule_file << "0,0,1,100000,47327,a=b,test" << std::endl;
        schedule_file << "1,7,3,7488338,1356567,a=b2," << std::endl;
        schedule_file.close();

        std::ofstream topology_file;
        topology_file.open (temp_dir + "/topology.properties.temp");
        topology_file << "num_nodes=8" << std::endl;
        topology_file << "num_undirected_edges=7" << std::endl;
        topology_file << "switches=set(0,1,2,3,4,5,6,7)" << std::endl;
        topology_file << "switches_which_are_tors=set(0,1,2,3,4,5,6,7)" << std::endl;
        topology_file << "servers=set()" << std::endl;
        topology_file << "undirected_edges=set(0-1,1-2,2-3,3-4,4-5,5-6,6-7)" << std::endl;
        topology_file.close();

        Topology topology = Topology(temp_dir + "/topology.properties.temp");
        std::vector<schedule_entry_t> schedule = read_schedule(temp_dir + "/schedule.csv.temp", topology, 10000000);

        ASSERT_EQUAL(schedule.size(), 2);

        ASSERT_EQUAL(schedule[0].flow_id, 0);
        ASSERT_EQUAL(schedule[0].from_node_id, 0);
        ASSERT_EQUAL(schedule[0].to_node_id, 1);
        ASSERT_EQUAL(schedule[0].size_byte, 100000);
        ASSERT_EQUAL(schedule[0].start_time_ns, 47327);
        ASSERT_EQUAL(schedule[0].additional_parameters, "a=b");
        ASSERT_EQUAL(schedule[0].metadata, "test");

        ASSERT_EQUAL(schedule[1].flow_id, 1);
        ASSERT_EQUAL(schedule[1].from_node_id, 7);
        ASSERT_EQUAL(schedule[1].to_node_id, 3);
        ASSERT_EQUAL(schedule[1].size_byte, 7488338);
        ASSERT_EQUAL(schedule[1].start_time_ns, 1356567);
        ASSERT_EQUAL(schedule[1].additional_parameters, "a=b2");
        ASSERT_EQUAL(schedule[1].metadata, "");

        // Empty

        std::ofstream schedule_file_empty(temp_dir + "/schedule.csv.temp");
        schedule_file_empty.close();
        std::vector<schedule_entry_t> schedule_empty = read_schedule(temp_dir + "/schedule.csv.temp", topology, 10000000);
        ASSERT_EQUAL(schedule_empty.size(), 0);

    }
};

class ScheduleReaderInvalidTestCase : public TestCase
{
public:
    ScheduleReaderInvalidTestCase () : TestCase ("schedule-reader invalid") {};
    const std::string temp_dir = ".testtmpdir";

    void DoRun () {
        mkdir_if_not_exists(temp_dir);

        std::ofstream schedule_file;
        std::vector<schedule_entry_t> schedule;

        std::ofstream topology_file;
        topology_file.open (temp_dir + "/topology.properties.temp");
        topology_file << "num_nodes=5" << std::endl;
        topology_file << "num_undirected_edges=4" << std::endl;
        topology_file << "switches=set(0,1,2,3,4)" << std::endl;
        topology_file << "switches_which_are_tors=set(0,1,3,4)" << std::endl; // Only 2 cannot be endpoint
        topology_file << "servers=set()" << std::endl;
        topology_file << "undirected_edges=set(0-1,1-2,2-3,3-4)" << std::endl;
        topology_file.close();

        Topology topology = Topology(temp_dir + "/topology.properties.temp");

        // Non-existent file
        ASSERT_EXCEPTION(read_schedule("does-not-exist-temp.file", topology, 10000000));
        
        // Normal
        schedule_file = std::ofstream(temp_dir + "/schedule.csv.temp");
        schedule_file << "0,0,4,100000,1356567,a=b,test" << std::endl;
        schedule_file.close();
        schedule = read_schedule(temp_dir + "/schedule.csv.temp", topology, 10000000);

        // Source = Destination
        schedule_file = std::ofstream(temp_dir + "/schedule.csv.temp");
        schedule_file << "0,3,3,100000,1356567,a=b,test" << std::endl;
        schedule_file.close();
        ASSERT_EXCEPTION(read_schedule(temp_dir + "/schedule.csv.temp", topology, 10000000));

        // Invalid source (out of range)
        schedule_file = std::ofstream(temp_dir + "/schedule.csv.temp");
        schedule_file << "0,9,0,100000,1356567,a=b,test" << std::endl;
        schedule_file.close();
        ASSERT_EXCEPTION(read_schedule(temp_dir + "/schedule.csv.temp", topology, 10000000));

        // Invalid destination (out of range)
        schedule_file = std::ofstream(temp_dir + "/schedule.csv.temp");
        schedule_file << "0,3,6,100000,1356567,a=b,test" << std::endl;
        schedule_file.close();
        ASSERT_EXCEPTION(read_schedule(temp_dir + "/schedule.csv.temp", topology, 10000000));

        // Invalid source (not a ToR)
        schedule_file = std::ofstream(temp_dir + "/schedule.csv.temp");
        schedule_file << "0,2,4,100000,1356567,a=b,test" << std::endl;
        schedule_file.close();
        ASSERT_EXCEPTION(read_schedule(temp_dir + "/schedule.csv.temp", topology, 10000000));

        // Invalid destination (not a ToR)
        schedule_file = std::ofstream(temp_dir + "/schedule.csv.temp");
        schedule_file << "0,4,2,100000,1356567,a=b,test" << std::endl;
        schedule_file.close();
        ASSERT_EXCEPTION(read_schedule(temp_dir + "/schedule.csv.temp", topology, 10000000));

        // Not ascending flow ID
        schedule_file = std::ofstream(temp_dir + "/schedule.csv.temp");
        schedule_file << "1,3,4,100000,1356567,a=b,test" << std::endl;
        schedule_file.close();
        ASSERT_EXCEPTION(read_schedule(temp_dir + "/schedule.csv.temp", topology, 10000000));

        // Negative flow size
        schedule_file = std::ofstream(temp_dir + "/schedule.csv.temp");
        schedule_file << "0,3,4,-6,1356567,a=b,test" << std::endl;
        schedule_file.close();
        ASSERT_EXCEPTION(read_schedule(temp_dir + "/schedule.csv.temp", topology, 10000000));

        // Not enough values
        schedule_file = std::ofstream(temp_dir + "/schedule.csv.temp");
        schedule_file << "0,3,4,7778,1356567,a=b" << std::endl;
        schedule_file.close();
        ASSERT_EXCEPTION(read_schedule(temp_dir + "/schedule.csv.temp", topology, 10000000));

        // Negative time
        schedule_file = std::ofstream(temp_dir + "/schedule.csv.temp");
        schedule_file << "0,3,4,86959,-7,a=b,test" << std::endl;
        schedule_file.close();
        ASSERT_EXCEPTION(schedule = read_schedule(temp_dir + "/schedule.csv.temp", topology, 10000000));

        // Just normal ordered with equal start time
        schedule_file = std::ofstream(temp_dir + "/schedule.csv.temp");
        schedule_file << "0,3,4,86959,9999,a=b,test" << std::endl;
        schedule_file << "1,3,4,86959,9999,a=b,test" << std::endl;
        schedule_file.close();
        schedule = read_schedule(temp_dir + "/schedule.csv.temp", topology, 10000000);

        // Not ordered time
        schedule_file = std::ofstream(temp_dir + "/schedule.csv.temp");
        schedule_file << "0,3,4,86959,10000,a=b,test" << std::endl;
        schedule_file << "1,3,4,86959,9999,a=b,test" << std::endl;
        schedule_file.close();
        ASSERT_EXCEPTION(read_schedule(temp_dir + "/schedule.csv.temp", topology, 10000000));

        // Exceeding time
        schedule_file = std::ofstream(temp_dir + "/schedule.csv.temp");
        schedule_file << "0,3,4,86959,10000000,a=b,test" << std::endl;
        schedule_file.close();
        ASSERT_EXCEPTION(read_schedule(temp_dir + "/schedule.csv.temp", topology, 10000000));

    }
};

////////////////////////////////////////////////////////////////////////////////////////
