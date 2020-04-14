/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/basic-simulation.h"
#include "ns3/test.h"
#include "end-to-end-test.h"
#include "schedule-reader-test.h"
#include "pingmesh-test.h"

using namespace ns3;

class BasicAppsTestSuite : public TestSuite {
public:
    BasicAppsTestSuite() : TestSuite("basic-apps", UNIT) {
        AddTestCase(new ScheduleReaderNormalTestCase, TestCase::QUICK);
        AddTestCase(new ScheduleReaderInvalidTestCase, TestCase::QUICK);
        AddTestCase(new EndToEndOneToOneEqualStartTestCase, TestCase::QUICK);
        AddTestCase(new EndToEndOneToOneSimpleStartTestCase, TestCase::QUICK);
        AddTestCase(new EndToEndOneToOneApartStartTestCase, TestCase::QUICK);
        AddTestCase(new EndToEndEcmpSimpleTestCase, TestCase::QUICK);
        AddTestCase(new EndToEndEcmpRemainTestCase, TestCase::QUICK);
        AddTestCase(new EndToEndNonExistentRunDirTestCase, TestCase::QUICK);
        AddTestCase(new PingmeshOneToOneTestCase, TestCase::QUICK);
    }
};
static BasicAppsTestSuite basicAppsTestSuite;
