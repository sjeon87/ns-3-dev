/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ns3/bounded-skew-scheduler.h"
#include "ns3/double.h"
#include "ns3/epoch-table.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/object-factory.h"
#include "ns3/pointer.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/test.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("BoundedSkewSchedulerTest");

static Ptr<EpochTable>
GetActiveEpochTable()
{
    return BoundedSkewScheduler::GetCurrentEpochTable();
}

static ObjectFactory
MakeSchedulerFactory(double windowSecs = 100.0,
                     double updateSecs = 10.0,
                     double minSkew = 0.5,
                     double maxSkew = 1.5,
                     double epsilonMs = 100.0)
{
    ObjectFactory f;
    f.SetTypeId("ns3::BoundedSkewScheduler");
    f.Set("WindowSize", TimeValue(Seconds(windowSecs)));
    f.Set("UpdatePeriod", TimeValue(Seconds(updateSecs)));
    f.Set("MinimumSkew", DoubleValue(minSkew));
    f.Set("MaximumSkew", DoubleValue(maxSkew));
    f.Set("Epsilon", TimeValue(MilliSeconds(epsilonMs)));
    return f;
}

/**
 * @brief Verifies node events fire at the simulator time translated from their local time.
 */
class BoundedSkewSchedulerAccuracyTestCase : public TestCase
{
  public:
    BoundedSkewSchedulerAccuracyTestCase();
    ~BoundedSkewSchedulerAccuracyTestCase() override;

  private:
    void DoRun() override;

    /**
     * @brief Check the firing event's simulator time matches the epoch table translation.
     * @param nodeId The node context the event fired on.
     * @param requestedSimTime The local node time the event was scheduled for.
     */
    void EventHandler(uint32_t nodeId, Time requestedSimTime);

    Time m_lastSimTime; //!< Simulator time of the last fired event, to check monotonicity
};

BoundedSkewSchedulerAccuracyTestCase::BoundedSkewSchedulerAccuracyTestCase()
    : TestCase("Verify that events are executed at the correctly translated simulator time")
{
}

BoundedSkewSchedulerAccuracyTestCase::~BoundedSkewSchedulerAccuracyTestCase()
{
}

void
BoundedSkewSchedulerAccuracyTestCase::EventHandler(uint32_t nodeId, Time scheduledLocalTime)
{
    Time now = Simulator::Now();

    NS_TEST_ASSERT_MSG_GT_OR_EQ(now, m_lastSimTime, "Simulator time moved backwards!");
    m_lastSimTime = now;

    Ptr<EpochTable> table = GetActiveEpochTable();
    NS_TEST_ASSERT_MSG_EQ(table != nullptr, true, "Could not retrieve EpochTable from Scheduler");

    Time expectedSimTime = table->GetSimulatorTimeFromNodeTime(nodeId, scheduledLocalTime);

    double diffSeconds = std::abs((now - expectedSimTime).GetSeconds());
    NS_TEST_ASSERT_MSG_LT(diffSeconds,
                          1e-9,
                          "Event fired at wrong simulator time for node " << nodeId);
}

void
BoundedSkewSchedulerAccuracyTestCase::DoRun()
{
    Simulator::SetScheduler(MakeSchedulerFactory());

    m_lastSimTime = Seconds(0);

    const uint32_t nodeIds[] = {1, 2, 3};
    for (uint32_t nodeId : nodeIds)
    {
        for (int i = 1; i <= 10; ++i)
        {
            Time t = Seconds(i * 5.0);
            Simulator::ScheduleWithContext(nodeId,
                                           t,
                                           &BoundedSkewSchedulerAccuracyTestCase::EventHandler,
                                           this,
                                           nodeId,
                                           t);
        }
    }

    Simulator::Stop(Seconds(60.0));
    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @brief Verifies context-less events bypass skew translation and fire on time.
 */
class BoundedSkewSchedulerInternalEventTestCase : public TestCase
{
  public:
    BoundedSkewSchedulerInternalEventTestCase();
    ~BoundedSkewSchedulerInternalEventTestCase() override;

  private:
    void DoRun() override;

    /**
     * @brief Check the context-less event fired at the expected simulator time.
     * @param expectedTime The simulator time the event should fire at.
     */
    void InternalHandler(Time expectedTime);

    bool m_eventRan; //!< True once the internal event has fired
};

BoundedSkewSchedulerInternalEventTestCase::BoundedSkewSchedulerInternalEventTestCase()
    : TestCase("Verify that context-less (0xffffffff) events bypass skew and fire on time"),
      m_eventRan(false)
{
}

BoundedSkewSchedulerInternalEventTestCase::~BoundedSkewSchedulerInternalEventTestCase()
{
}

void
BoundedSkewSchedulerInternalEventTestCase::InternalHandler(Time expectedTime)
{
    m_eventRan = true;
    Time now = Simulator::Now();
    double diffSeconds = std::abs((now - expectedTime).GetSeconds());
    NS_TEST_ASSERT_MSG_LT(diffSeconds,
                          1e-9,
                          "Internal event fired at wrong time; skew was incorrectly applied");
}

void
BoundedSkewSchedulerInternalEventTestCase::DoRun()
{
    Simulator::SetScheduler(MakeSchedulerFactory(100.0, 10.0, 0.1, 10.0, 50.0));

    Time scheduleAt = Seconds(15.0);
    Simulator::Schedule(scheduleAt,
                        &BoundedSkewSchedulerInternalEventTestCase::InternalHandler,
                        this,
                        scheduleAt);

    Simulator::Stop(Seconds(60.0));
    Simulator::Run();

    NS_TEST_ASSERT_MSG_EQ(m_eventRan, true, "Internal context-less event failed to execute");

    Simulator::Destroy();
}

/**
 * @brief Verifies epoch table cleanup/pruning does not corrupt pending node events.
 */
class BoundedSkewSchedulerCleanupTestCase : public TestCase
{
  public:
    BoundedSkewSchedulerCleanupTestCase();
    ~BoundedSkewSchedulerCleanupTestCase() override;

  private:
    void DoRun() override;

    /**
     * @brief Check a late event still fires at the correct simulator time after pruning.
     * @param nodeId The node context the event fired on.
     * @param requestedSimTime The local node time the event was scheduled for.
     */
    void LateHandler(uint32_t nodeId, Time requestedSimTime);

    uint32_t m_eventCount; //!< Number of late events that have fired
};

BoundedSkewSchedulerCleanupTestCase::BoundedSkewSchedulerCleanupTestCase()
    : TestCase("Verify that cleanup or pruning does not corrupt pending events"),
      m_eventCount(0)
{
}

BoundedSkewSchedulerCleanupTestCase::~BoundedSkewSchedulerCleanupTestCase()
{
}

void
BoundedSkewSchedulerCleanupTestCase::LateHandler(uint32_t nodeId, Time scheduledLocalTime)
{
    ++m_eventCount;

    Ptr<EpochTable> table = GetActiveEpochTable();
    NS_TEST_ASSERT_MSG_EQ(table != nullptr, true, "Could not retrieve EpochTable from Scheduler");

    Time expectedSimTime = table->GetSimulatorTimeFromNodeTime(nodeId, scheduledLocalTime);
    double diffSeconds = std::abs((Simulator::Now() - expectedSimTime).GetSeconds());

    NS_TEST_ASSERT_MSG_LT(diffSeconds, 1e-9, "Event fired at wrong simulator time after pruning");
}

void
BoundedSkewSchedulerCleanupTestCase::DoRun()
{
    Simulator::SetScheduler(MakeSchedulerFactory(20.0, 5.0, 0.5, 1.5, 500.0));

    uint32_t nodeId = 7;
    uint32_t expectedCount = 5;

    for (uint32_t i = 0; i < expectedCount; ++i)
    {
        Time t = Seconds(80.0 + i * 5.0);
        Simulator::ScheduleWithContext(nodeId,
                                       t,
                                       &BoundedSkewSchedulerCleanupTestCase::LateHandler,
                                       this,
                                       nodeId,
                                       t);
    }

    Simulator::Stop(Seconds(120.0));
    Simulator::Run();

    NS_TEST_ASSERT_MSG_EQ(m_eventCount,
                          expectedCount,
                          "Cleanup pruning caused events to be lost or duplicated");

    Simulator::Destroy();
}

/**
 * @brief Verifies the node's local clock never drifts more than Epsilon away from
 * Simulator::Now(), even with an aggressive skew range and a long-running simulation.
 */
class BoundedSkewSchedulerBoundTestCase : public TestCase
{
  public:
    BoundedSkewSchedulerBoundTestCase();
    ~BoundedSkewSchedulerBoundTestCase() override;

  private:
    void DoRun() override;

    /**
     * @brief Sample the node's clock offset from Simulator::Now() and check it is within bound.
     * @param nodeId The node context to sample.
     */
    void SampleHandler(uint32_t nodeId);

    Time m_epsilon;         //!< The configured bound, for assertion comparisons
    uint32_t m_sampleCount; //!< Number of times the offset was sampled
};

BoundedSkewSchedulerBoundTestCase::BoundedSkewSchedulerBoundTestCase()
    : TestCase("Verify node clock offset from Simulator::Now() never exceeds Epsilon"),
      m_sampleCount(0)
{
}

BoundedSkewSchedulerBoundTestCase::~BoundedSkewSchedulerBoundTestCase()
{
}

void
BoundedSkewSchedulerBoundTestCase::SampleHandler(uint32_t nodeId)
{
    ++m_sampleCount;

    Ptr<EpochTable> table = GetActiveEpochTable();
    NS_TEST_ASSERT_MSG_EQ(table != nullptr, true, "Could not retrieve EpochTable from Scheduler");

    Time now = Simulator::Now();
    Time localNow = table->GetNodeTimeFromSimulatorTime(nodeId, now);
    Time offset = localNow - now;

    NS_TEST_ASSERT_MSG_LT_OR_EQ(offset.GetSeconds(),
                                m_epsilon.GetSeconds() + 1e-9,
                                "Node " << nodeId << " local time drifted above +Epsilon");
    NS_TEST_ASSERT_MSG_GT_OR_EQ(offset.GetSeconds(),
                                -m_epsilon.GetSeconds() - 1e-9,
                                "Node " << nodeId << " local time drifted below -Epsilon");
}

void
BoundedSkewSchedulerBoundTestCase::DoRun()
{
    m_epsilon = MilliSeconds(200.0);

    ObjectFactory f;
    f.SetTypeId("ns3::BoundedSkewScheduler");
    f.Set("WindowSize", TimeValue(Seconds(200.0)));
    f.Set("UpdatePeriod", TimeValue(Seconds(5.0)));
    f.Set("MinimumSkew", DoubleValue(0.5));
    f.Set("MaximumSkew", DoubleValue(2.0));
    f.Set("Epsilon", TimeValue(m_epsilon));
    Simulator::SetScheduler(f);

    const uint32_t nodeIds[] = {1, 2, 3, 4};
    for (uint32_t nodeId : nodeIds)
    {
        for (int i = 1; i <= 200; ++i)
        {
            Time t = Seconds(i * 1.0);
            Simulator::ScheduleWithContext(nodeId,
                                           t,
                                           &BoundedSkewSchedulerBoundTestCase::SampleHandler,
                                           this,
                                           nodeId);
        }
    }

    Simulator::Stop(Seconds(210.0));
    Simulator::Run();

    NS_TEST_ASSERT_MSG_EQ(m_sampleCount, 4 * 200, "Not all bound-check samples executed");

    Simulator::Destroy();
}

/**
 * @brief Test suite for the BoundedSkewScheduler class.
 */
class BoundedSkewSchedulerTestSuite : public TestSuite
{
  public:
    BoundedSkewSchedulerTestSuite();
};

BoundedSkewSchedulerTestSuite::BoundedSkewSchedulerTestSuite()
    : TestSuite("bounded-skew-scheduler", TestSuite::Type::UNIT)
{
    AddTestCase(new BoundedSkewSchedulerAccuracyTestCase, TestCase::Duration::QUICK);
    AddTestCase(new BoundedSkewSchedulerInternalEventTestCase, TestCase::Duration::QUICK);
    AddTestCase(new BoundedSkewSchedulerCleanupTestCase, TestCase::Duration::QUICK);
    AddTestCase(new BoundedSkewSchedulerBoundTestCase, TestCase::Duration::QUICK);
}

static BoundedSkewSchedulerTestSuite g_BoundedSkewSchedulerTestSuite;
