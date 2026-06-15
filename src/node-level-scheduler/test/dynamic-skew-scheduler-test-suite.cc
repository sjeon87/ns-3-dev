/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ns3/double.h"
#include "ns3/dynamic-skew-scheduler.h"
#include "ns3/epoch-table.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/test.h"

namespace ns3
{

class EpochTableTestCase : public TestCase
{
  public:
    EpochTableTestCase();
    void DoRun() override;
};

EpochTableTestCase::EpochTableTestCase()
    : TestCase("EpochTableTest")
{
}

void
EpochTableTestCase::DoRun()
{
    Ptr<EpochTable> table = CreateObject<EpochTable>();
    uint32_t nodeId = 1;

    EpochTable::Epoch e1;
    e1.simulatorStartTime = Seconds(0.0);
    e1.simulatorEndTime = Seconds(10.0);
    e1.nodeStartTime = Seconds(0.0);
    e1.nodeEndTime = Seconds(20.0);
    e1.skew = 2.0;
    table->AddEpoch(nodeId, e1);

    NS_TEST_ASSERT_MSG_EQ(table->GetNodeTimeFromSimulatorTime(nodeId, Seconds(5.0)),
                          Seconds(10.0),
                          "Sim 5.0s with 2.0x skew should be Node 10.0s");
    NS_TEST_ASSERT_MSG_EQ(table->GetSimulatorTimeFromNodeTime(nodeId, Seconds(10.0)),
                          Seconds(5.0),
                          "Node 10.0s with 2.0x skew should be Sim 5.0s");

    table->InsertEpoch(nodeId, Seconds(5.0), Seconds(15.0), Seconds(10.0), Seconds(15.0), 0.5);

    NS_TEST_ASSERT_MSG_EQ(table->GetNodeTimeFromSimulatorTime(nodeId, Seconds(9.0)),
                          Seconds(12.0),
                          "Sim 9.0s with 0.5x skew should be Node 12.0s");
    NS_TEST_ASSERT_MSG_EQ(table->GetSimulatorTimeFromNodeTime(nodeId, Seconds(14.0)),
                          Seconds(13.0),
                          "Node 14.0s with 0.5x skew should be Sim 13.0s");

    table->InsertEpoch(nodeId, Seconds(15.0), Seconds(20.0), Seconds(15.0), Seconds(35.0), 4.0);

    NS_TEST_ASSERT_MSG_EQ(table->GetNodeTimeFromSimulatorTime(nodeId, Seconds(18.0)),
                          Seconds(27.0),
                          "Sim 18.0s with 4.0x skew should be Node 27.0s");
    NS_TEST_ASSERT_MSG_EQ(table->GetSimulatorTimeFromNodeTime(nodeId, Seconds(27.0)),
                          Seconds(18.0),
                          "Node 27.0s with 4.0x skew should be Sim 18.0s");

    table->PruneEpochTable(Seconds(10.0));

    NS_TEST_ASSERT_MSG_EQ(table->GetNodeTimeFromSimulatorTime(nodeId, Seconds(12.0)),
                          Seconds(13.5),
                          "Table should correctly map unpruned epochs after cleanup");
}

class DynamicSkewSchedulerExecutionTestCase : public TestCase
{
  public:
    DynamicSkewSchedulerExecutionTestCase();
    void DoRun() override;

  private:
    void EventTriggered(uint32_t context, Time expectedLocalTime);
    uint32_t m_eventsFired;
};

DynamicSkewSchedulerExecutionTestCase::DynamicSkewSchedulerExecutionTestCase()
    : TestCase("ExecutionTest"),
      m_eventsFired(0)
{
}

void
DynamicSkewSchedulerExecutionTestCase::EventTriggered(uint32_t context, Time expectedLocalTime)
{
    m_eventsFired++;

    Ptr<EpochTable> table = DynamicSkewScheduler::GetCurrentEpochTable();
    Time localTimeAtFire = table->GetNodeTimeFromSimulatorTime(context, Simulator::Now());

    NS_TEST_ASSERT_MSG_EQ_TOL(
        localTimeAtFire.GetSeconds(),
        expectedLocalTime.GetSeconds(),
        1e-9,
        "Event fired at incorrect local node time due to skew translation error");
}

void
DynamicSkewSchedulerExecutionTestCase::DoRun()
{
    ObjectFactory factory;
    factory.SetTypeId("ns3::DynamicSkewScheduler");
    factory.Set("MinimumSkew", DoubleValue(0.5));
    factory.Set("MaximumSkew", DoubleValue(3.5));
    Simulator::SetScheduler(factory);

    Simulator::ScheduleWithContext(1,
                                   Seconds(10.0),
                                   &DynamicSkewSchedulerExecutionTestCase::EventTriggered,
                                   this,
                                   1,
                                   Seconds(10.0));

    Simulator::ScheduleWithContext(2,
                                   Seconds(40.0),
                                   &DynamicSkewSchedulerExecutionTestCase::EventTriggered,
                                   this,
                                   2,
                                   Seconds(40.0));

    Simulator::Run();
    Simulator::Destroy();

    NS_TEST_ASSERT_MSG_EQ(m_eventsFired, 2, "Not all scheduled events fired");
}

class DynamicSkewSchedulerCancelTestCase : public TestCase
{
  public:
    DynamicSkewSchedulerCancelTestCase();
    void DoRun() override;

  private:
    void ShouldNotFire();
    void ShouldFire();
    void ScheduleNodeEvents();

    bool m_badEventFired;
    bool m_goodEventFired;
    EventId m_badEvent;
};

DynamicSkewSchedulerCancelTestCase::DynamicSkewSchedulerCancelTestCase()
    : TestCase("CancelCase"),
      m_badEventFired(false),
      m_goodEventFired(false)
{
}

void
DynamicSkewSchedulerCancelTestCase::ShouldNotFire()
{
    m_badEventFired = true;
}

void
DynamicSkewSchedulerCancelTestCase::ShouldFire()
{
    m_goodEventFired = true;
}

void
DynamicSkewSchedulerCancelTestCase::ScheduleNodeEvents()
{
    m_badEvent = Simulator::Schedule(Seconds(10.0),
                                     &DynamicSkewSchedulerCancelTestCase::ShouldNotFire,
                                     this);

    Simulator::Schedule(Seconds(15.0), &DynamicSkewSchedulerCancelTestCase::ShouldFire, this);
}

void
DynamicSkewSchedulerCancelTestCase::DoRun()
{
    ObjectFactory factory;
    factory.SetTypeId("ns3::DynamicSkewScheduler");
    factory.Set("MinimumSkew", DoubleValue(0.1));
    factory.Set("MaximumSkew", DoubleValue(5.0));
    Simulator::SetScheduler(factory);

    Simulator::ScheduleWithContext(1,
                                   Seconds(0.0),
                                   &DynamicSkewSchedulerCancelTestCase::ScheduleNodeEvents,
                                   this);

    Simulator::Schedule(Seconds(2.0), &EventId::Cancel, &m_badEvent);

    Simulator::Run();
    Simulator::Destroy();

    NS_TEST_ASSERT_MSG_EQ(m_badEventFired, false, "Cancelled event was incorrectly executed!");
    NS_TEST_ASSERT_MSG_EQ(m_goodEventFired,
                          true,
                          "Valid event failed to execute after cancellation occurred.");
}

class DynamicSkewSchedulerTestSuite : public TestSuite
{
  public:
    DynamicSkewSchedulerTestSuite();
};

DynamicSkewSchedulerTestSuite::DynamicSkewSchedulerTestSuite()
    : TestSuite("dynamic-skew-scheduler", Type::UNIT)
{
    AddTestCase(new EpochTableTestCase, TestCase::Duration::QUICK);
    AddTestCase(new DynamicSkewSchedulerExecutionTestCase, TestCase::Duration::QUICK);
    AddTestCase(new DynamicSkewSchedulerCancelTestCase, TestCase::Duration::QUICK);
}

static DynamicSkewSchedulerTestSuite g_dynamicSkewSchedulerTestSuite;

} // namespace ns3
