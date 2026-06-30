/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ns3/double.h"
#include "ns3/epoch-table.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/object-factory.h"
#include "ns3/pointer.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/static-skew-scheduler.h"
#include "ns3/test.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("StaticSkewSchedulerTest");

Ptr<EpochTable>
GetCurrentEpochTable()
{
    return StaticSkewScheduler::GetCurrentEpochTable();
}

static ObjectFactory
MakeSchedulerFactory(double windowSecs = 100.0,
                     double updateSecs = 10.0,
                     double minSkew = 0.5,
                     double maxSkew = 1.5)
{
    ObjectFactory f;
    f.SetTypeId("ns3::StaticSkewScheduler");
    f.Set("WindowSize", TimeValue(Seconds(windowSecs)));
    f.Set("UpdatePeriod", TimeValue(Seconds(updateSecs)));
    f.Set("MinimumSkew", DoubleValue(minSkew));
    f.Set("MaximumSkew", DoubleValue(maxSkew));
    return f;
}

/**
 * @brief Verifies node events fire at the simulator time translated from their local time.
 */
class StaticSkewSchedulerAccuracyTestCase : public TestCase
{
  public:
    StaticSkewSchedulerAccuracyTestCase();
    ~StaticSkewSchedulerAccuracyTestCase() override;

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

StaticSkewSchedulerAccuracyTestCase::StaticSkewSchedulerAccuracyTestCase()
    : TestCase("Verify that events are executed at the correctly translated simulator time")
{
}

StaticSkewSchedulerAccuracyTestCase::~StaticSkewSchedulerAccuracyTestCase()
{
}

void
StaticSkewSchedulerAccuracyTestCase::EventHandler(uint32_t nodeId, Time scheduledLocalTime)
{
    Time now = Simulator::Now();

    NS_TEST_ASSERT_MSG_GT_OR_EQ(now, m_lastSimTime, "Simulator time moved backwards!");
    m_lastSimTime = now;

    Ptr<EpochTable> table = GetCurrentEpochTable();
    NS_TEST_ASSERT_MSG_EQ(table != nullptr, true, "Could not retrieve EpochTable from Scheduler");

    Time expectedSimTime = table->GetSimulatorTimeFromNodeTime(nodeId, scheduledLocalTime);

    double diffSeconds = std::abs((now - expectedSimTime).GetSeconds());
    NS_TEST_ASSERT_MSG_LT(diffSeconds,
                          1e-9,
                          "Event fired at wrong simulator time for node " << nodeId);
}

void
StaticSkewSchedulerAccuracyTestCase::DoRun()
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
                                           &StaticSkewSchedulerAccuracyTestCase::EventHandler,
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
 * @brief Verifies scheduling a far-future event extends the epoch table and fires correctly.
 */
class StaticSkewSchedulerFutureEventTestCase : public TestCase
{
  public:
    StaticSkewSchedulerFutureEventTestCase();
    ~StaticSkewSchedulerFutureEventTestCase() override;

  private:
    void DoRun() override;

    /**
     * @brief Check the epoch table was extended and the event fired at the right sim time.
     * @param nodeId The node context the event fired on.
     * @param requestedSimTime The local node time the event was scheduled for.
     */
    void FarFutureHandler(uint32_t nodeId, Time requestedSimTime);

    bool m_eventRan; //!< True once the far-future event has fired
};

StaticSkewSchedulerFutureEventTestCase::StaticSkewSchedulerFutureEventTestCase()
    : TestCase("Verify far-future scheduling triggers table extension and correct fire time"),
      m_eventRan(false)
{
}

StaticSkewSchedulerFutureEventTestCase::~StaticSkewSchedulerFutureEventTestCase()
{
}

void
StaticSkewSchedulerFutureEventTestCase::FarFutureHandler(uint32_t nodeId, Time scheduledLocalTime)
{
    m_eventRan = true;

    Ptr<EpochTable> table = GetCurrentEpochTable();
    NS_TEST_ASSERT_MSG_EQ(table != nullptr, true, "Could not retrieve EpochTable from Scheduler");

    NS_TEST_ASSERT_MSG_GT(table->GetMaxNodeTime(nodeId),
                          Seconds(4999),
                          "EpochTable did not extend to cover the event time");

    Time now = Simulator::Now();
    Time expectedSim = table->GetSimulatorTimeFromNodeTime(nodeId, scheduledLocalTime);

    double diffSeconds = std::abs((now - expectedSim).GetSeconds());
    NS_TEST_ASSERT_MSG_LT(diffSeconds, 1e-9, "Far-future event fired at wrong simulator time");
}

void
StaticSkewSchedulerFutureEventTestCase::DoRun()
{
    Simulator::SetScheduler(MakeSchedulerFactory());

    uint32_t nodeId = 2;
    Time requestedSimTime = Seconds(5000);

    Simulator::ScheduleWithContext(nodeId,
                                   requestedSimTime,
                                   &StaticSkewSchedulerFutureEventTestCase::FarFutureHandler,
                                   this,
                                   nodeId,
                                   requestedSimTime);

    Simulator::Stop(Seconds(60000));
    Simulator::Run();

    NS_TEST_ASSERT_MSG_EQ(m_eventRan, true, "Far-future event failed to execute");

    Simulator::Destroy();
}

/**
 * @brief Verifies context-less events bypass skew translation and fire on time.
 */
class StaticSkewSchedulerInternalEventTestCase : public TestCase
{
  public:
    StaticSkewSchedulerInternalEventTestCase();
    ~StaticSkewSchedulerInternalEventTestCase() override;

  private:
    void DoRun() override;

    /**
     * @brief Check the context-less event fired at the expected simulator time.
     * @param expectedTime The simulator time the event should fire at.
     */
    void InternalHandler(Time expectedTime);

    bool m_eventRan; //!< True once the internal event has fired
};

StaticSkewSchedulerInternalEventTestCase::StaticSkewSchedulerInternalEventTestCase()
    : TestCase("Verify that context-less (0xffffffff) events bypass skew and fire on time"),
      m_eventRan(false)
{
}

StaticSkewSchedulerInternalEventTestCase::~StaticSkewSchedulerInternalEventTestCase()
{
}

void
StaticSkewSchedulerInternalEventTestCase::InternalHandler(Time expectedTime)
{
    m_eventRan = true;
    Time now = Simulator::Now();
    double diffSeconds = std::abs((now - expectedTime).GetSeconds());
    NS_TEST_ASSERT_MSG_LT(diffSeconds,
                          1e-9,
                          "Internal event fired at wrong time; skew was incorrectly applied");
}

void
StaticSkewSchedulerInternalEventTestCase::DoRun()
{
    Simulator::SetScheduler(MakeSchedulerFactory(100.0, 10.0, 0.1, 10.0));

    Time scheduleAt = Seconds(15.0);
    Simulator::Schedule(scheduleAt,
                        &StaticSkewSchedulerInternalEventTestCase::InternalHandler,
                        this,
                        scheduleAt);

    Simulator::Stop(Seconds(60.0));
    Simulator::Run();

    NS_TEST_ASSERT_MSG_EQ(m_eventRan, true, "Internal context-less event failed to execute");

    Simulator::Destroy();
}

/**
 * @brief Verifies correct translation for events scheduled on update-interval boundaries.
 */
class StaticSkewSchedulerBoundaryTestCase : public TestCase
{
  public:
    StaticSkewSchedulerBoundaryTestCase();
    ~StaticSkewSchedulerBoundaryTestCase() override;

  private:
    void DoRun() override;

    /**
     * @brief Check the boundary event fired at the simulator time translated from its local time.
     * @param nodeId The node context the event fired on.
     * @param requestedSimTime The local node time the event was scheduled for.
     */
    void BoundaryHandler(uint32_t nodeId, Time requestedSimTime);

    uint32_t m_eventCount; //!< Number of boundary events that have fired
};

StaticSkewSchedulerBoundaryTestCase::StaticSkewSchedulerBoundaryTestCase()
    : TestCase("Verify correct translation for events scheduled on interval boundaries"),
      m_eventCount(0)
{
}

StaticSkewSchedulerBoundaryTestCase::~StaticSkewSchedulerBoundaryTestCase()
{
}

void
StaticSkewSchedulerBoundaryTestCase::BoundaryHandler(uint32_t nodeId, Time scheduledLocalTime)
{
    ++m_eventCount;

    Ptr<EpochTable> table = GetCurrentEpochTable();
    NS_TEST_ASSERT_MSG_EQ(table != nullptr, true, "Could not retrieve EpochTable from Scheduler");

    Time now = Simulator::Now();
    Time expectedSim = table->GetSimulatorTimeFromNodeTime(nodeId, scheduledLocalTime);

    double diffSeconds = std::abs((now - expectedSim).GetSeconds());
    NS_TEST_ASSERT_MSG_LT(diffSeconds,
                          1e-9,
                          "Boundary event fired at wrong simulator time for node " << nodeId);
}

void
StaticSkewSchedulerBoundaryTestCase::DoRun()
{
    Simulator::SetScheduler(MakeSchedulerFactory(100.0, 10.0, 0.5, 1.5));

    uint32_t nodeId = 5;
    uint32_t expectedCount = 0;

    for (int i = 1; i <= 5; ++i)
    {
        Time t = Seconds(i * 10.0);
        Simulator::ScheduleWithContext(nodeId,
                                       t,
                                       &StaticSkewSchedulerBoundaryTestCase::BoundaryHandler,
                                       this,
                                       nodeId,
                                       t);
        ++expectedCount;
    }

    Simulator::Stop(Seconds(60.0));
    Simulator::Run();

    NS_TEST_ASSERT_MSG_EQ(m_eventCount, expectedCount, "Not all boundary events executed");

    Simulator::Destroy();
}

/**
 * @brief Verifies epoch table cleanup/pruning does not corrupt pending node events.
 */
class StaticSkewSchedulerCleanupTestCase : public TestCase
{
  public:
    StaticSkewSchedulerCleanupTestCase();
    ~StaticSkewSchedulerCleanupTestCase() override;

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

StaticSkewSchedulerCleanupTestCase::StaticSkewSchedulerCleanupTestCase()
    : TestCase("Verify that cleanup or pruning does not corrupt pending events"),
      m_eventCount(0)
{
}

StaticSkewSchedulerCleanupTestCase::~StaticSkewSchedulerCleanupTestCase()
{
}

void
StaticSkewSchedulerCleanupTestCase::LateHandler(uint32_t nodeId, Time scheduledLocalTime)
{
    ++m_eventCount;

    Ptr<EpochTable> table = GetCurrentEpochTable();
    NS_TEST_ASSERT_MSG_EQ(table != nullptr, true, "Could not retrieve EpochTable from Scheduler");

    Time expectedSimTime = table->GetSimulatorTimeFromNodeTime(nodeId, scheduledLocalTime);
    double diffSeconds = std::abs((Simulator::Now() - expectedSimTime).GetSeconds());

    NS_TEST_ASSERT_MSG_LT(diffSeconds, 1e-9, "Event fired at wrong simulator time after pruning");
}

void
StaticSkewSchedulerCleanupTestCase::DoRun()
{
    Simulator::SetScheduler(MakeSchedulerFactory(20.0, 5.0, 0.5, 1.5));

    uint32_t nodeId = 7;
    uint32_t expectedCount = 5;

    for (uint32_t i = 0; i < expectedCount; ++i)
    {
        Time t = Seconds(80.0 + i * 5.0);
        Simulator::ScheduleWithContext(nodeId,
                                       t,
                                       &StaticSkewSchedulerCleanupTestCase::LateHandler,
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
 * @brief Stress test with many nodes and frequent events, checking simulator-time monotonicity.
 */
class StaticSkewSchedulerStressTestCase : public TestCase
{
  public:
    StaticSkewSchedulerStressTestCase();
    ~StaticSkewSchedulerStressTestCase() override;

  private:
    void DoRun() override;

    /**
     * @brief Count the fired event and check simulator time has not gone backwards.
     * @param nodeId The node context the event fired on.
     */
    void StressHandler(uint32_t nodeId);

    uint32_t m_eventCount;                    //!< Number of stress events that have fired
    std::map<uint32_t, Time> m_lastFireTime;  //!< Last simulator fire time, keyed by context
};

StaticSkewSchedulerStressTestCase::StaticSkewSchedulerStressTestCase()
    : TestCase("Stress test: multiple nodes, frequent events, monotonicity enforced"),
      m_eventCount(0)
{
}

StaticSkewSchedulerStressTestCase::~StaticSkewSchedulerStressTestCase()
{
}

void
StaticSkewSchedulerStressTestCase::StressHandler(uint32_t nodeId)
{
    ++m_eventCount;

    Time now = Simulator::Now();

    auto it = m_lastFireTime.find(0xffffffff);
    if (it != m_lastFireTime.end())
    {
        NS_TEST_ASSERT_MSG_GT_OR_EQ(now,
                                    it->second,
                                    "Global simulator time went backwards in stress test");
    }
    m_lastFireTime[0xffffffff] = now;
}

void
StaticSkewSchedulerStressTestCase::DoRun()
{
    Simulator::SetScheduler(MakeSchedulerFactory(50.0, 5.0, 0.5, 1.5));

    const uint32_t numNodes = 50;
    const uint32_t eventsPerNode = 100;

    Ptr<UniformRandomVariable> rng = CreateObject<UniformRandomVariable>();

    for (uint32_t i = 0; i < numNodes; ++i)
    {
        uint32_t nodeId = i + 10;
        for (uint32_t j = 0; j < eventsPerNode; ++j)
        {
            Time t = Seconds(rng->GetValue(1.0, 500.0));
            Simulator::ScheduleWithContext(nodeId,
                                           t,
                                           &StaticSkewSchedulerStressTestCase::StressHandler,
                                           this,
                                           nodeId);
        }
    }

    Simulator::Stop(Seconds(6000));
    Simulator::Run();

    NS_TEST_ASSERT_MSG_EQ(m_eventCount,
                          numNodes * eventsPerNode,
                          "Not all stress test events executed");

    Simulator::Destroy();
}

/**
 * @brief Test suite for the StaticSkewScheduler class.
 */
class StaticSkewSchedulerTestSuite : public TestSuite
{
  public:
    StaticSkewSchedulerTestSuite();
};

StaticSkewSchedulerTestSuite::StaticSkewSchedulerTestSuite()
    : TestSuite("static-skew-scheduler", TestSuite::Type::UNIT)
{
    AddTestCase(new StaticSkewSchedulerAccuracyTestCase, TestCase::Duration::QUICK);
    AddTestCase(new StaticSkewSchedulerFutureEventTestCase, TestCase::Duration::QUICK);
    AddTestCase(new StaticSkewSchedulerInternalEventTestCase, TestCase::Duration::QUICK);
    AddTestCase(new StaticSkewSchedulerBoundaryTestCase, TestCase::Duration::QUICK);
    AddTestCase(new StaticSkewSchedulerCleanupTestCase, TestCase::Duration::QUICK);
    AddTestCase(new StaticSkewSchedulerStressTestCase, TestCase::Duration::TAKES_FOREVER);
}

static StaticSkewSchedulerTestSuite g_StaticSkewSchedulerTestSuite;
