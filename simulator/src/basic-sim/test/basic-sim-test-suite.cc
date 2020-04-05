/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/basic-sim.h"
#include "ns3/test.h"
#include "end-to-end-test.h"
#include "schedule-reader-test.h"
#include "simon-util-test.h"
#include "topology-test.h"
#include "routing-arbiter-test.h"

using namespace ns3;

class BasicSimTestSuite : public TestSuite {
public:
    BasicSimTestSuite() : TestSuite("basic-sim", UNIT) {
        AddTestCase(new SimonUtilStringsTestCase, TestCase::QUICK);
        AddTestCase(new SimonUtilParsingTestCase, TestCase::QUICK);
        AddTestCase(new SimonUtilSetsTestCase, TestCase::QUICK);
        AddTestCase(new SimonUtilConfigurationReadingTestCase, TestCase::QUICK);
        AddTestCase(new SimonUtilUnitConversionTestCase, TestCase::QUICK);
        AddTestCase(new SimonUtilFileSystemTestCase, TestCase::QUICK);
        AddTestCase(new TopologyEmptyTestCase, TestCase::QUICK);
        AddTestCase(new TopologySingleTestCase, TestCase::QUICK);
        AddTestCase(new TopologyTorTestCase, TestCase::QUICK);
        AddTestCase(new TopologyLeafSpineTestCase, TestCase::QUICK);
        AddTestCase(new TopologyRingTestCase, TestCase::QUICK);
        AddTestCase(new TopologyInvalidTestCase, TestCase::QUICK);
        AddTestCase(new ScheduleReaderNormalTestCase, TestCase::QUICK);
        AddTestCase(new ScheduleReaderInvalidTestCase, TestCase::QUICK);
        AddTestCase(new EndToEndOneToOneEqualStartTestCase, TestCase::QUICK);
        AddTestCase(new EndToEndOneToOneApartStartTestCase, TestCase::QUICK);
        AddTestCase(new EndToEndEcmpSimpleTestCase, TestCase::QUICK);
        AddTestCase(new RoutingArbiterIpResolutionTestCase, TestCase::QUICK);
        AddTestCase(new RoutingArbiterEcmpHashTestCase, TestCase::QUICK);
        AddTestCase(new RoutingArbiterBadImplTestCase, TestCase::QUICK);
        // Disabled because it takes too long for a quick test:
        // AddTestCase(new RoutingArbiterEcmpTooBigFailTestCase, TestCase::QUICK);
    }
};
static BasicSimTestSuite basicSimTestSuite;
