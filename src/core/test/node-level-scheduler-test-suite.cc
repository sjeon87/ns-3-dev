/*
 * Copyright (c) 2025 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ns3/core-module.h"
#include "ns3/multi-rate-clock.h"
#include "ns3/network-module.h"
#include "ns3/node-level-scheduler.h"
#include "ns3/test.h"

#include <cstdio>
#include <fstream>
#include <map>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ComprehensiveSchedulerTest");

static std::map<std::string, Time> g_executionTimes;

// Helper to record execution time (SimTime)
void
RecordExecutionTimeEvent(std::string eventId)
{
    g_executionTimes[eventId] = Simulator::Now();
    NS_LOG_INFO("Event [" << eventId << "] executed at SimTime " << Simulator::Now().GetSeconds()
                          << "s");
}

class SchedulerTestCase : public TestCase
{
  public:
    SchedulerTestCase();
    void DoRun() override;

  private:
    void VerifyNodeClock(Ptr<Node> node, Time expectedNodeTime, std::string label);
    void ScheduleFutureEvent(); // Helper to schedule dynamically
};

SchedulerTestCase::SchedulerTestCase()
    : TestCase("Comprehensive NodeLevelScheduler & MultiRateClock Test")
{
}

void
SchedulerTestCase::VerifyNodeClock(Ptr<Node> node, Time expectedNodeTime, std::string label)
{
    Time actualNodeTime = node->GetLocalTime();
    double tolerance = 1e-5;

    NS_TEST_EXPECT_MSG_EQ_TOL(actualNodeTime.GetSeconds(),
                              expectedNodeTime.GetSeconds(),
                              tolerance,
                              "Clock Verification Failed for " << label);

    NS_LOG_INFO("Clock Check [" << label << "]: SimTime=" << Simulator::Now().GetSeconds()
                                << "s, Node reports=" << actualNodeTime.GetSeconds()
                                << "s (Expected=" << expectedNodeTime.GetSeconds() << "s) - OK");
}

void
SchedulerTestCase::ScheduleFutureEvent()
{
    Simulator::ScheduleWithContext(2, Seconds(2.0), &RecordExecutionTimeEvent, "C_Future_Exec");
}

void
SchedulerTestCase::DoRun()
{
    std::string tempFileName = "scheduler-test-intervals.csv";

    // 1. Create the Interval File
    {
        std::ofstream outFile(tempFileName);
        // Format: nodeId, simStart, simEnd, nodeStart, nodeEnd, skew
        outFile << "1,0,100,0,100,1.0\n";
        outFile << "2,0,10,0,5,0.5\n";                  // Interval 1
        outFile << "2,10,20,5,11.666666,1.5\n";         // Interval 2
        outFile << "2,20,30,11.666666,12.666666,0.1\n"; // Interval 3 (Sim 20-30)
        outFile.close();
    }

    LogComponentEnable("NodeLevelScheduler", LOG_LEVEL_LOGIC);
    LogComponentEnable("ComprehensiveSchedulerTest", LOG_LEVEL_INFO);

    ObjectFactory schedulerFactory;
    schedulerFactory.SetTypeId("ns3::NodeLevelScheduler");
    schedulerFactory.Set("IntervalFile", StringValue(tempFileName));
    schedulerFactory.Set("WindowSize", TimeValue(Seconds(15.0)));
    schedulerFactory.Set("UpdatePeriod", TimeValue(Seconds(1.0)));
    Simulator::SetScheduler(schedulerFactory);

    NodeContainer nodes;
    nodes.Create(3);

    // Install Clocks
    {
        Ptr<MultiRateClock> clock2 = CreateObject<MultiRateClock>();
        clock2->SetNodeId(2);
        nodes.Get(2)->SetAttribute("LocalClock", PointerValue(clock2));

        Ptr<MultiRateClock> clock1 = CreateObject<MultiRateClock>();
        clock1->SetNodeId(1);
        nodes.Get(1)->SetAttribute("LocalClock", PointerValue(clock1));
    }

    // Test A: Skew 0.5 (Target 2.0s -> Sim 4.0s)
    Simulator::ScheduleWithContext(2, Seconds(2.0), &RecordExecutionTimeEvent, "A_Slow_Exec");

    Simulator::Schedule(Seconds(4.0) + MicroSeconds(1),
                        &SchedulerTestCase::VerifyNodeClock,
                        this,
                        nodes.Get(2),
                        Seconds(2.0),
                        "A_Slow_Clock");

    // Test B: Skew 1.5 (Target 6.0s -> Sim 10.666s)
    Time expectedSimB = Seconds(10.0 + (1.0 / 1.5));
    Simulator::ScheduleWithContext(2, Seconds(6.0), &RecordExecutionTimeEvent, "B_Fast_Exec");

    Simulator::Schedule(expectedSimB + MicroSeconds(1),
                        &SchedulerTestCase::VerifyNodeClock,
                        this,
                        nodes.Get(2),
                        Seconds(6.0),
                        "B_Fast_Clock");

    // Test C: Dynamic Loading (Target NodeTime 12.0s -> Sim 23.333s)
    // We schedule a helper event at T=10s to ensure the interval (Start T=20s) is loaded.
    Simulator::Schedule(Seconds(10.0), &SchedulerTestCase::ScheduleFutureEvent, this);

    Time expectedSimC = Seconds(20.0 + ((12.0 - 11.666666) / 0.1));

    Simulator::Schedule(expectedSimC + MicroSeconds(1),
                        &SchedulerTestCase::VerifyNodeClock,
                        this,
                        nodes.Get(2),
                        Seconds(12.0),
                        "C_Future_Clock");

    Simulator::Stop(Seconds(30.0));
    Simulator::Run();

    Time tolerance = MicroSeconds(10);
    NS_TEST_ASSERT_MSG_EQ_TOL(g_executionTimes["A_Slow_Exec"], Seconds(4.0), tolerance, "A Failed");
    NS_TEST_ASSERT_MSG_EQ_TOL(g_executionTimes["B_Fast_Exec"], expectedSimB, tolerance, "B Failed");
    NS_TEST_ASSERT_MSG_EQ_TOL(g_executionTimes["C_Future_Exec"],
                              expectedSimC,
                              tolerance,
                              "C Failed");

    NS_LOG_UNCOND("All tests passed!");

    Simulator::Destroy();
    g_executionTimes.clear();
    std::remove(tempFileName.c_str());
}

static class SchedulerTestSuite : public TestSuite
{
  public:
    SchedulerTestSuite()
        : TestSuite("SchedulerTestSuite", Type::UNIT)
    {
        AddTestCase(new SchedulerTestCase, Duration::QUICK);
    }
} g_schedulerTestSuite;
