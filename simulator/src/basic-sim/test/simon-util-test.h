/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/basic-sim.h"
#include "ns3/test.h"
#include "test-helpers.h"

using namespace ns3;

////////////////////////////////////////////////////////////////////////////////////////

class SimonUtilTestCase : public TestCase
{
public:
    SimonUtilTestCase () : TestCase ("simon-util") {};
    void DoRun () {

        // Trimming
        ASSERT_EQUAL(trim("abc"), "abc");
        ASSERT_EQUAL(trim(" abc"), "abc");
        ASSERT_EQUAL(trim("abc "), "abc");
        ASSERT_EQUAL(trim("  abc "), "abc");
        ASSERT_EQUAL(trim("\t abc"), "abc");
        ASSERT_EQUAL(left_trim("abc "), "abc ");
        ASSERT_EQUAL(right_trim(" abc"), " abc");

        // Quote removal
        ASSERT_EQUAL(remove_start_end_double_quote_if_present("abc"), "abc");
        ASSERT_EQUAL(remove_start_end_double_quote_if_present("\"abc"), "\"abc");
        ASSERT_EQUAL(remove_start_end_double_quote_if_present("abc\""), "abc\"");
        ASSERT_EQUAL(remove_start_end_double_quote_if_present("\"abc\""), "abc");
        ASSERT_EQUAL(remove_start_end_double_quote_if_present("\"abc\""), "abc");
        ASSERT_EQUAL(remove_start_end_double_quote_if_present("\" abc\""), " abc");
        ASSERT_EQUAL(remove_start_end_double_quote_if_present("\"\""), "");
        ASSERT_EQUAL(remove_start_end_double_quote_if_present("\""), "\"");

        // Starting / ending
        ASSERT_TRUE(starts_with("abc", "abc"));
        ASSERT_TRUE(starts_with("abc", "ab"));
        ASSERT_FALSE(starts_with("abc", "bc"));
        ASSERT_TRUE(starts_with("abc", ""));
        ASSERT_TRUE(ends_with("abc", "abc"));
        ASSERT_FALSE(ends_with("abc", "ab"));
        ASSERT_TRUE(ends_with("abc", "bc"));
        ASSERT_TRUE(ends_with("abc", ""));

        // TODO: Split string
        // TODO: Parsing values
        // TODO: Sets
        // TODO: Configuration reading

        // Unit conversion
        ASSERT_EQUAL_APPROX(byte_to_megabit(10000000), 80, 0.000001);
        ASSERT_EQUAL_APPROX(nanosec_to_sec(10000000), 0.01, 0.000001);
        ASSERT_EQUAL_APPROX(nanosec_to_millisec(10000000), 10.0, 0.000001);
        ASSERT_EQUAL_APPROX(nanosec_to_microsec(10000000), 10000.0, 0.000001);

        // File system: file
        remove_file_if_exists("temp.file");
        ASSERT_FALSE(file_exists("temp.file"));
        std::ofstream the_file;
        the_file.open ("temp.file");
        the_file << "Content" << std::endl;
        the_file << "Other" << std::endl;
        the_file.close();
        std::vector<std::string> lines = read_file_direct("temp.file");
        ASSERT_EQUAL(lines.size(), 2);
        ASSERT_EQUAL(trim(lines[0]), "Content");
        ASSERT_EQUAL(trim(lines[1]), "Other");
        ASSERT_TRUE(file_exists("temp.file"));
        remove_file_if_exists("temp.file");
        ASSERT_FALSE(file_exists("temp.file"));

        // File system: dir
        mkdir_if_not_exists("temp.dir");
        ASSERT_TRUE(dir_exists("temp.dir"));
        ASSERT_NOT_EQUAL(rmdir("temp.dir"), -1);
        ASSERT_FALSE(dir_exists("temp.dir"));

    }
};

////////////////////////////////////////////////////////////////////////////////////////
