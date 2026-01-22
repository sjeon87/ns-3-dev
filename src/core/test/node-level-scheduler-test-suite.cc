#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/node-level-scheduler.h"
#include "ns3/test.h"

#include <map>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ComprehensiveSchedulerTest");

static std::map<std::string, Time> g_executionTimes;

void
RecordExecutionTimeEvent(std::string eventId)
{
    g_executionTimes[eventId] = Simulator::Now();
    NS_LOG_UNCOND("Event [" << eventId << "] executed at " << Simulator::Now().GetSeconds() << "s");
}

/**
 * @brief Helper event to schedule the far future event dynamically.
 */
void
ScheduleNextEvent(uint32_t node, Time delay, std::string eventName)
{
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
    LogComponentEnable("NodeLevelScheduler", LOG_LEVEL_LOGIC);

    // Interval Config:
    // Node 2 (Int 2): Sim [10, 20), Node [5, 11.666) -> Skew 1.5
    // Node 2 (Int 3): Sim [20, 30), Node [11.666, 12.666) -> Skew 0.1
    std::string intervalConfig = "1,0,100,0,100,1.0;"
                                 "2,0,10,0,5,0.5;"
                                 "2,10,20,5,11.6666,1.5;"
                                 "2,20,30,11.6666,12.6666,0.1;";

    ObjectFactory schedulerFactory;
    schedulerFactory.SetTypeId("ns3::NodeLevelScheduler");
    schedulerFactory.Set("Intervals", StringValue(intervalConfig));

    // Window Size is 15s. At T=0, we only know about intervals up to T=15.
    schedulerFactory.Set("WindowSize", TimeValue(Seconds(15.0)));
    schedulerFactory.Set("UpdatePeriod", TimeValue(Seconds(1.0)));

    Simulator::SetScheduler(schedulerFactory);

    NodeContainer nodes;
    nodes.Create(3);

    NS_LOG_UNCOND("--- Scheduling Test Events ---");

    // Test A & B (Within initial window)
    Simulator::ScheduleWithContext(0, Seconds(2.0), &RecordExecutionTimeEvent, "A0_Normal");
    Simulator::ScheduleWithContext(1, Seconds(2.0), &RecordExecutionTimeEvent, "A1_Normal");
    Simulator::ScheduleWithContext(2, Seconds(2.0), &RecordExecutionTimeEvent, "A2_Slow");
    Simulator::ScheduleWithContext(2, Seconds(4.0), &RecordExecutionTimeEvent, "B1_BeforeChange");
    Simulator::ScheduleWithContext(2, Seconds(6.0), &RecordExecutionTimeEvent, "B2_AfterChange");

    // Test C: Dynamic Window Loading
    // "Bridge Event" at SimTime T=10s.
    // We want to schedule the next event for Node Time 12.0s.
    // Since we are currently at SimTime 10.0s, the delay must be 2.0s.
    // Logic: TargetNodeTime (12) = Now (10) + Delay (2).
    NS_LOG_UNCOND("Test C: Scheduling Bridge Event at 10s to schedule Future Event");

    Simulator::Schedule(Seconds(10.0), &ScheduleNextEvent, 2, Seconds(2.0), "C1_FarFuture");

    Simulator::Stop(Seconds(25.0));
    Simulator::Run();

    NS_LOG_UNCOND("\n--- Verifying Execution Times ---");

    Time tolerance = MicroSeconds(1);

    // Verify A
    NS_TEST_ASSERT_MSG_EQ_TOL(g_executionTimes["A0_Normal"], Seconds(2.0), tolerance, "A0");
    NS_TEST_ASSERT_MSG_EQ_TOL(g_executionTimes["A1_Normal"], Seconds(2.0), tolerance, "A1");
    NS_TEST_ASSERT_MSG_EQ_TOL(g_executionTimes["A2_Slow"], Seconds(4.0), tolerance, "A2");

    // Verify B
    NS_TEST_ASSERT_MSG_EQ_TOL(g_executionTimes["B1_BeforeChange"], Seconds(8.0), tolerance, "B1");
    Time expectedB2 = Seconds(10.0) + Seconds(1.0 / 1.5);
    NS_TEST_ASSERT_MSG_EQ_TOL(g_executionTimes["B2_AfterChange"], expectedB2, tolerance, "B2");

    // Verify C
    // Interval 3 Start: Node 11.6666, Sim 20.0, Skew 0.1
    // Target Node Time: 12.0
    // Delta Node: 12.0 - 11.6666 = 0.3333
    // Scaled Delta: 0.3333 / 0.1 = 3.333
    // Sim Time: 20.0 + 3.333 = 23.333
    double nodeStart3 = 11.6666;
    double skew3 = 0.1;
    double simStart3 = 20.0;
    Time expectedC1 = Seconds(simStart3 + ((12.0 - nodeStart3) / skew3));

    NS_TEST_ASSERT_MSG_EQ_TOL(g_executionTimes["C1_FarFuture"],
                              expectedC1,
                              tolerance,
                              "C1 (Window Test)");

    NS_LOG_UNCOND("All tests passed!");
    Simulator::Destroy();
    g_executionTimes.clear();
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
