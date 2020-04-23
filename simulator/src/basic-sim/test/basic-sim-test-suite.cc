/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/basic-simulation.h"
#include "ns3/test.h"
#include "exp-util-test.h"
#include "topology-test.h"
#include "routing-arbiter-test.h"

using namespace ns3;

class BasicSimTestSuite : public TestSuite {
public:
    BasicSimTestSuite() : TestSuite("basic-sim", UNIT) {
        AddTestCase(new ExpUtilStringsTestCase, TestCase::QUICK);
        AddTestCase(new ExpUtilParsingTestCase, TestCase::QUICK);
        AddTestCase(new ExpUtilSetsTestCase, TestCase::QUICK);
        AddTestCase(new ExpUtilConfigurationReadingTestCase, TestCase::QUICK);
        AddTestCase(new ExpUtilUnitConversionTestCase, TestCase::QUICK);
        AddTestCase(new ExpUtilFileSystemTestCase, TestCase::QUICK);
        AddTestCase(new TopologyEmptyTestCase, TestCase::QUICK);
        AddTestCase(new TopologySingleTestCase, TestCase::QUICK);
        AddTestCase(new TopologyTorTestCase, TestCase::QUICK);
        AddTestCase(new TopologyLeafSpineTestCase, TestCase::QUICK);
        AddTestCase(new TopologyRingTestCase, TestCase::QUICK);
        AddTestCase(new TopologyInvalidTestCase, TestCase::QUICK);
        AddTestCase(new RoutingArbiterIpResolutionTestCase, TestCase::QUICK);
        AddTestCase(new RoutingArbiterEcmpHashTestCase, TestCase::QUICK);
        AddTestCase(new RoutingArbiterEcmpStringReprTestCase, TestCase::QUICK);
        AddTestCase(new RoutingArbiterBadImplTestCase, TestCase::QUICK);
        // Disabled because it takes too long for a quick test:
        // AddTestCase(new RoutingArbiterEcmpTooBigFailTestCase, TestCase::QUICK);
    }
};
static BasicSimTestSuite basicSimTestSuite;
