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
        // TODO: Unit conversion
        // TODO: File system

    }
};

////////////////////////////////////////////////////////////////////////////////////////
