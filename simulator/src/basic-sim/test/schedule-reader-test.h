/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/basic-sim.h"
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
        schedule_file << "0,0,1,100000,1356567,a=b,test" << std::endl;
        schedule_file << "1,7,3,7488338,47327,a=b2," << std::endl;
        schedule_file.close();

        std::vector<schedule_entry_t> schedule = read_schedule(temp_dir + "/schedule.csv.temp", 8, 10000000);

        ASSERT_EQUAL(schedule.size(), 2);

        ASSERT_EQUAL(schedule[0].flow_id, 0);
        ASSERT_EQUAL(schedule[0].from_node_id, 0);
        ASSERT_EQUAL(schedule[0].to_node_id, 1);
        ASSERT_EQUAL(schedule[0].size_byte, 100000);
        ASSERT_EQUAL(schedule[0].start_time_ns, 1356567);
        ASSERT_EQUAL(schedule[0].additional_parameters, "a=b");
        ASSERT_EQUAL(schedule[0].metadata, "test");

        ASSERT_EQUAL(schedule[1].flow_id, 1);
        ASSERT_EQUAL(schedule[1].from_node_id, 7);
        ASSERT_EQUAL(schedule[1].to_node_id, 3);
        ASSERT_EQUAL(schedule[1].size_byte, 7488338);
        ASSERT_EQUAL(schedule[1].start_time_ns, 47327);
        ASSERT_EQUAL(schedule[1].additional_parameters, "a=b2");
        ASSERT_EQUAL(schedule[1].metadata, "");

        // Empty

        std::ofstream schedule_file_empty(temp_dir + "/schedule.csv.temp");
        schedule_file_empty.close();
        std::vector<schedule_entry_t> schedule_empty = read_schedule(temp_dir + "/schedule.csv.temp", 0, 10000000);
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
        bool caught;
        
        // Normal
        schedule_file = std::ofstream(temp_dir + "/schedule.csv.temp");
        schedule_file << "0,0,4,100000,1356567,a=b,test" << std::endl;
        schedule_file.close();
        schedule = read_schedule(temp_dir + "/schedule.csv.temp", 5, 10000000);

        // Source = Destination
        caught = false;
        try {
            schedule_file = std::ofstream(temp_dir + "/schedule.csv.temp");
            schedule_file << "0,3,3,100000,1356567,a=b,test" << std::endl;
            schedule_file.close();
            schedule = read_schedule(temp_dir + "/schedule.csv.temp", 5, 10000000);
        } catch (std::exception& e) {
            caught = true;
        }
        ASSERT_TRUE(caught);

        // Invalid source
        caught = false;
        try {
            schedule_file = std::ofstream(temp_dir + "/schedule.csv.temp");
            schedule_file << "0,9,0,100000,1356567,a=b,test" << std::endl;
            schedule_file.close();
            schedule = read_schedule(temp_dir + "/schedule.csv.temp", 5, 10000000);
        } catch (std::exception& e) {
            caught = true;
        }
        ASSERT_TRUE(caught);

        // Invalid destination
        caught = false;
        try {
            schedule_file = std::ofstream(temp_dir + "/schedule.csv.temp");
            schedule_file << "0,3,6,100000,1356567,a=b,test" << std::endl;
            schedule_file.close();
            schedule = read_schedule(temp_dir + "/schedule.csv.temp", 5, 10000000);
        } catch (std::exception& e) {
            caught = true;
        }
        ASSERT_TRUE(caught);

        // Not ascending
        caught = false;
        try {
            schedule_file = std::ofstream(temp_dir + "/schedule.csv.temp");
            schedule_file << "1,3,4,100000,1356567,a=b,test" << std::endl;
            schedule_file.close();
            schedule = read_schedule(temp_dir + "/schedule.csv.temp", 5, 10000000);
        } catch (std::exception& e) {
            caught = true;
        }
        ASSERT_TRUE(caught);

        // Negative flow size
        caught = false;
        try {
            schedule_file = std::ofstream(temp_dir + "/schedule.csv.temp");
            schedule_file << "1,3,4,-6,1356567,a=b,test" << std::endl;
            schedule_file.close();
            schedule = read_schedule(temp_dir + "/schedule.csv.temp", 5, 10000000);
        } catch (std::exception& e) {
            caught = true;
        }
        ASSERT_TRUE(caught);

        // Not enough values
        caught = false;
        try {
            schedule_file = std::ofstream(temp_dir + "/schedule.csv.temp");
            schedule_file << "1,3,4,7778,1356567,a=b" << std::endl;
            schedule_file.close();
            schedule = read_schedule(temp_dir + "/schedule.csv.temp", 5, 10000000);
        } catch (std::exception& e) {
            caught = true;
        }
        ASSERT_TRUE(caught);

        // Negative time
        caught = false;
        try {
            schedule_file = std::ofstream(temp_dir + "/schedule.csv.temp");
            schedule_file << "1,3,4,86959,-7,a=b,test" << std::endl;
            schedule_file.close();
            schedule = read_schedule(temp_dir + "/schedule.csv.temp", 5, 10000000);
        } catch (std::exception& e) {
            caught = true;
        }
        ASSERT_TRUE(caught);

        // Exceeding time
        caught = false;
        try {
            schedule_file = std::ofstream(temp_dir + "/schedule.csv.temp");
            schedule_file << "1,3,4,86959,10000000,a=b,test" << std::endl;
            schedule_file.close();
            schedule = read_schedule(temp_dir + "/schedule.csv.temp", 5, 10000000);
        } catch (std::exception& e) {
            caught = true;
        }
        ASSERT_TRUE(caught);

    }
};

////////////////////////////////////////////////////////////////////////////////////////
