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

/**
 * @brief Verifies EpochTable time conversion across inserted and pruned epochs.
 */
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

/**
 * @brief Verifies node events execute and round-trip to the correct local time.
 */
class DynamicSkewSchedulerExecutionTestCase : public TestCase
{
  public:
    DynamicSkewSchedulerExecutionTestCase();
    void DoRun() override;

  private:
    /**
     * @brief Check the firing event's local time matches expectations.
     * @param context The node context the event fired on.
     * @param expectedLocalTime The expected local node time at fire.
     */
    void EventTriggered(uint32_t context, Time expectedLocalTime);

    uint32_t m_eventsFired; //!< Number of events that have fired
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

/**
 * @brief Verifies a cancelled node event does not fire while later events still do.
 */
class DynamicSkewSchedulerCancelTestCase : public TestCase
{
  public:
    DynamicSkewSchedulerCancelTestCase();
    void DoRun() override;

  private:
    /**
     * @brief Mark that the event which should have been cancelled fired.
     */
    void ShouldNotFire();

    /**
     * @brief Mark that the valid event fired.
     */
    void ShouldFire();

    /**
     * @brief Schedule the cancel-target and follow-up node events.
     */
    void ScheduleNodeEvents();

    bool m_badEventFired;  //!< True if the cancelled event incorrectly fired
    bool m_goodEventFired; //!< True once the valid event has fired
    EventId m_badEvent;    //!< The event expected to be cancelled
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

/**
 * @brief Verifies ChangeCurrentSkew updates the epoch table for a node.
 */
class DynamicSkewSchedulerChangeSkewTestCase : public TestCase
{
  public:
    DynamicSkewSchedulerChangeSkewTestCase();
    void DoRun() override;

  private:
    /**
     * @brief Change node 1's skew and check the epoch table reflects it.
     */
    void TriggerSkewChange();

    /**
     * @brief Record that the follow-up node event fired.
     */
    void FollowUpEvent();

    bool m_followUpFired; //!< True once the follow-up node event has fired
};

DynamicSkewSchedulerChangeSkewTestCase::DynamicSkewSchedulerChangeSkewTestCase()
    : TestCase("ChangeSkewCase"),
      m_followUpFired(false)
{
}

void
DynamicSkewSchedulerChangeSkewTestCase::TriggerSkewChange()
{
    DynamicSkewScheduler::ChangeCurrentSkew(1, 2.0);

    Ptr<EpochTable> table = DynamicSkewScheduler::GetCurrentEpochTable();
    NS_TEST_ASSERT_MSG_NE(table,
                          nullptr,
                          "EpochTable should be accessible after ChangeCurrentSkew");
    NS_TEST_ASSERT_MSG_EQ(table->HasNode(1),
                          true,
                          "Node 1 should have epochs after ChangeCurrentSkew");

    const EpochTable::Epoch& ep = table->GlobalTimeBinarySearch(1, Seconds(0.0));
    NS_TEST_ASSERT_MSG_EQ_TOL(ep.skew, 2.0, 1e-9, "Epoch at sim t=0 should carry skew=2.0");

    Time localAt5s = table->GetNodeTimeFromSimulatorTime(1, Seconds(5.0));
    NS_TEST_ASSERT_MSG_EQ_TOL(
        localAt5s.GetSeconds(),
        10.0,
        1e-9,
        "GetNodeTimeFromSimulatorTime should return 10s at sim 5s with skew=2.0");

    Simulator::ScheduleWithContext(1,
                                   Seconds(1.0),
                                   &DynamicSkewSchedulerChangeSkewTestCase::FollowUpEvent,
                                   this);
}

void
DynamicSkewSchedulerChangeSkewTestCase::FollowUpEvent()
{
    m_followUpFired = true;
}

void
DynamicSkewSchedulerChangeSkewTestCase::DoRun()
{
    ObjectFactory factory;
    factory.SetTypeId("ns3::DynamicSkewScheduler");
    factory.Set("MinimumSkew", DoubleValue(2.0));
    factory.Set("MaximumSkew", DoubleValue(2.0));
    factory.Set("UpdatePeriod", TimeValue(Seconds(10.0)));
    Simulator::SetScheduler(factory);

    Simulator::Schedule(Seconds(0.0),
                        &DynamicSkewSchedulerChangeSkewTestCase::TriggerSkewChange,
                        this);

    Simulator::Run();
    Simulator::Destroy();

    NS_TEST_ASSERT_MSG_EQ(m_followUpFired, true, "Follow-up node event never fired");
}

/**
 * @brief Test suite for the EpochTable and DynamicSkewScheduler.
 */
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
    AddTestCase(new DynamicSkewSchedulerChangeSkewTestCase, TestCase::Duration::QUICK);
}

static DynamicSkewSchedulerTestSuite
    g_dynamicSkewSchedulerTestSuite; //!< Static variable for test initialization

} // namespace ns3
