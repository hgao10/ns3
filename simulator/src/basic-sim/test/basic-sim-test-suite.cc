/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/basic-sim.h"
#include "ns3/test.h"
#include "end-to-end-test.h"
#include "reading-helper-test.h"
#include "simon-util-test.h"
#include "topology-test.h"

using namespace ns3;

class BasicSimTestSuite : public TestSuite {
public:
    BasicSimTestSuite() : TestSuite("basic-sim", UNIT) {
        AddTestCase(new SimonUtilTestCase, TestCase::QUICK);
        AddTestCase(new TopologyTestCase, TestCase::QUICK);
        AddTestCase(new ReadingHelperTestCase, TestCase::QUICK);
        AddTestCase(new EndToEndTestCase, TestCase::QUICK);
    }
};
static BasicSimTestSuite basicSimTestSuite;
