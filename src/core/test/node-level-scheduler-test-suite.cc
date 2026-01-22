/*
 * Copyright (c) 2025 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/node-level-scheduler.h"
#include "ns3/test.h"

#include <cstdio> // For std::remove
#include <fstream>
#include <map>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ComprehensiveSchedulerTest");

static std::map<std::string, Time> g_executionTimes;

/**
 * @brief Helper to record when an event actually executes in SimTime.
 */
void
RecordExecutionTimeEvent(std::string eventId)
{
    g_executionTimes[eventId] = Simulator::Now();
    NS_LOG_UNCOND("Event [" << eventId << "] executed at " << Simulator::Now().GetSeconds() << "s");
}

/**
 * @brief Helper event to schedule a future event dynamically.
 * This simulates a running application that schedules its next step.
 * * @param node The node ID to schedule on.
 * @param delay The delay from NOW.
 * @param eventName The unique ID for verification.
 */
void
ScheduleNextEvent(uint32_t node, Time delay, std::string eventName)
{
    // NodeLevelScheduler interprets the target time (Now + delay) as Node Local Time.
    Simulator::ScheduleWithContext(node, delay, &RecordExecutionTimeEvent, eventName);
}

class SchedulerTestCase : public TestCase
{
  public:
    SchedulerTestCase();
    void DoRun() override;
};

SchedulerTestCase::SchedulerTestCase()
    : TestCase("Comprehensive NodeLevelScheduler Test")
{
}

void
SchedulerTestCase::DoRun()
{
    std::string tempFileName = "scheduler-test-intervals.csv";

    {
        std::ofstream outFile(tempFileName);
        // Format: nodeId, simStart, simEnd, nodeStart, nodeEnd, skew
        // Node 1: Normal (1.0)
        outFile << "1,0,100,0,100,1.0\n";
        // Node 2 (Interval 1): Slow (0.5)
        outFile << "2,0,10,0,5,0.5\n";
        // Node 2 (Interval 2): Fast (1.5)
        outFile << "2,10,20,5,11.6666,1.5\n";
        // Node 2 (Interval 3): Very Slow (0.1) - Starts at SimTime 20.0
        outFile << "2,20,30,11.6666,12.6666,0.1\n";
        outFile.close();
    }

    LogComponentEnable("NodeLevelScheduler", LOG_LEVEL_LOGIC);

    ObjectFactory schedulerFactory;
    schedulerFactory.SetTypeId("ns3::NodeLevelScheduler");

    // Point to our temporary file
    schedulerFactory.Set("IntervalFile", StringValue(tempFileName));

    // Set WindowSize to 15s.
    // At T=0, the scheduler loads intervals up to T=15.
    // Interval 3 (starting at T=20) will NOT be loaded initially.
    schedulerFactory.Set("WindowSize", TimeValue(Seconds(15.0)));
    schedulerFactory.Set("UpdatePeriod", TimeValue(Seconds(1.0)));

    Simulator::SetScheduler(schedulerFactory);

    NodeContainer nodes;
    nodes.Create(3);

    NS_LOG_UNCOND("--- Scheduling Test Events ---");

    // Test A: Basic Events (Within initial window)
    Simulator::ScheduleWithContext(0, Seconds(2.0), &RecordExecutionTimeEvent, "A0_Normal");
    Simulator::ScheduleWithContext(1, Seconds(2.0), &RecordExecutionTimeEvent, "A1_Normal");
    Simulator::ScheduleWithContext(2, Seconds(2.0), &RecordExecutionTimeEvent, "A2_Slow");

    // Test B: Skew Change Crossing (Within initial window)
    Simulator::ScheduleWithContext(2, Seconds(4.0), &RecordExecutionTimeEvent, "B1_BeforeChange");
    Simulator::ScheduleWithContext(2, Seconds(6.0), &RecordExecutionTimeEvent, "B2_AfterChange");

    // Test C: Dynamic Window Loading (File Streaming Check)
    // We schedule a "Bridge Event" at SimTime T=10s.
    // At T=10s, the Window Horizon extends to 25s (10+15).
    // The scheduler should wake up, read the file stream, and load Interval 3 (starts at 20s).
    //
    // We want to target Node Time 12.0s.
    // Since we are at SimTime 10.0s, we schedule with delay 2.0s (10+2=12).
    NS_LOG_UNCOND("Test C: Scheduling Bridge Event at 10s to schedule Future Event");
    Simulator::Schedule(Seconds(10.0), &ScheduleNextEvent, 2, Seconds(2.0), "C1_FarFuture");

    Simulator::Stop(Seconds(25.0));
    Simulator::Run();

    NS_LOG_UNCOND("\n--- Verifying Execution Times ---");

    Time tolerance = MicroSeconds(1);

    // Verify A
    NS_TEST_ASSERT_MSG_EQ_TOL(g_executionTimes["A0_Normal"],
                              Seconds(2.0),
                              tolerance,
                              "A0 (Normal) Failed");
    NS_TEST_ASSERT_MSG_EQ_TOL(g_executionTimes["A1_Normal"],
                              Seconds(2.0),
                              tolerance,
                              "A1 (Normal) Failed");
    NS_TEST_ASSERT_MSG_EQ_TOL(g_executionTimes["A2_Slow"],
                              Seconds(4.0),
                              tolerance,
                              "A2 (Slow) Failed");

    // Verify B
    NS_TEST_ASSERT_MSG_EQ_TOL(g_executionTimes["B1_BeforeChange"],
                              Seconds(8.0),
                              tolerance,
                              "B1 (Before Skew) Failed");

    // B2 Calculation:
    // Interval 2 starts at NodeTime 5.0 (Sim 10.0). Target is 6.0.
    // Delta = 1.0. Skew = 1.5. Scaled = 0.666.
    // SimTime = 10.0 + 0.666...
    Time expectedB2 = Seconds(10.0) + Seconds(1.0 / 1.5);
    NS_TEST_ASSERT_MSG_EQ_TOL(g_executionTimes["B2_AfterChange"],
                              expectedB2,
                              tolerance,
                              "B2 (After Skew) Failed");

    // Verify C (File Streaming & Window)
    // Interval 3 starts at NodeTime 11.6666 (Sim 20.0). Target is 12.0.
    // Delta = 0.3333. Skew = 0.1. Scaled = 3.333.
    // SimTime = 20.0 + 3.333...
    double nodeStart3 = 11.6666;
    double skew3 = 0.1;
    double simStart3 = 20.0;
    Time expectedC1 = Seconds(simStart3 + ((12.0 - nodeStart3) / skew3));

    NS_TEST_ASSERT_MSG_EQ_TOL(g_executionTimes["C1_FarFuture"],
                              expectedC1,
                              tolerance,
                              "C1 (File Stream Test) Failed");

    NS_LOG_UNCOND("All tests passed!");

    Simulator::Destroy();
    g_executionTimes.clear();

    // Remove the temporary file
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
