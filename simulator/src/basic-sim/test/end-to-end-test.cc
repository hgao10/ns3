/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/basic-sim.h"
#include "ns3/test.h"

using namespace ns3;

////////////////////////////////////////////////////////////////////////////////////////

class MyTestCase : public TestCase
{
public:
    MyTestCase () : TestCase ("my") {};
    void DoRun () {
        NS_TEST_ASSERT_MSG_EQ (true, true, "true doesn't equal true for some reason");
    }
};

////////////////////////////////////////////////////////////////////////////////////////

class BasicSimEndToEndTestSuite : public TestSuite {
public:
    BasicSimEndToEndTestSuite() : TestSuite("basic-sim-end-to-end", UNIT) {
        AddTestCase(new MyTestCase, TestCase::QUICK);
    }
};
static BasicSimEndToEndTestSuite basicSimEndToEndTestSuite;
