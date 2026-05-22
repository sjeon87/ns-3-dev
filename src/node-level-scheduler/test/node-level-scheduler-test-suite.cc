/*
 * Copyright (c) 2026 Ishaan Lagwankar
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/node-level-scheduler.h"
#include "ns3/nstime.h"
#include "ns3/object-factory.h"
#include "ns3/pointer.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/test.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("NodeLevelSchedulerTestSuite");

Ptr<NodeTimingGraph>
GetCurrentTimingGraph()
{
    return NodeLevelScheduler::GetCurrentGraph();
}

class NodeLevelSchedulerAccuracyTestCase : public TestCase
{
  public:
    NodeLevelSchedulerAccuracyTestCase();
    ~NodeLevelSchedulerAccuracyTestCase() override;

  private:
    void DoRun() override;
    void EventHandler(uint32_t nodeId, Time scheduledNodeTime);

    Time m_lastSimTime;
};

NodeLevelSchedulerAccuracyTestCase::NodeLevelSchedulerAccuracyTestCase()
    : TestCase("Verify that events are executed with correct skew translation")
{
}

NodeLevelSchedulerAccuracyTestCase::~NodeLevelSchedulerAccuracyTestCase()
{
}

void
NodeLevelSchedulerAccuracyTestCase::EventHandler(uint32_t nodeId, Time scheduledNodeTime)
{
    Time now = Simulator::Now();

    NS_TEST_ASSERT_MSG_GT_OR_EQ(now, m_lastSimTime, "Simulator time moved backwards!");
    m_lastSimTime = now;

    Ptr<NodeTimingGraph> graph = GetCurrentTimingGraph();
    // Fix: Check for null pointer implicitly or explicitly against nullptr
    bool graphExists = (graph != nullptr);
    NS_TEST_ASSERT_MSG_EQ(graphExists, true, "Could not retrieve NodeTimingGraph from Scheduler");

    Time expectedSimTime = graph->GetSimulatorTimeFromNodeTime(nodeId, scheduledNodeTime);

    double diff = std::abs((now - expectedSimTime).GetSeconds());
    NS_TEST_ASSERT_MSG_LT(diff, 1e-9, "Event executed at wrong Simulator Time compared to Graph");
}

void
NodeLevelSchedulerAccuracyTestCase::DoRun()
{
    ObjectFactory schedulerFactory;
    schedulerFactory.SetTypeId("ns3::NodeLevelScheduler");
    schedulerFactory.Set("WindowSize", TimeValue(Seconds(100)));
    schedulerFactory.Set("UpdatePeriod", TimeValue(Seconds(10)));
    schedulerFactory.Set("MinimumSkew", DoubleValue(0.5));
    schedulerFactory.Set("MaximumSkew", DoubleValue(1.5));

    Simulator::SetScheduler(schedulerFactory);

    uint32_t nodeId = 1;
    m_lastSimTime = Seconds(0);

    for (int i = 1; i <= 10; ++i)
    {
        Time t = Seconds(i * 5.0);
        Simulator::ScheduleWithContext(nodeId,
                                       t,
                                       &NodeLevelSchedulerAccuracyTestCase::EventHandler,
                                       this,
                                       nodeId,
                                       t);
    }

    Simulator::Stop(Seconds(60.0));
    Simulator::Run();
    Simulator::Destroy();
}

class NodeLevelSchedulerFutureEventTestCase : public TestCase
{
  public:
    NodeLevelSchedulerFutureEventTestCase();
    ~NodeLevelSchedulerFutureEventTestCase() override;

  private:
    void DoRun() override;
    void FarFutureHandler(uint32_t nodeId);
    bool m_eventRan;
};

NodeLevelSchedulerFutureEventTestCase::NodeLevelSchedulerFutureEventTestCase()
    : TestCase("Verify scheduling far into the future triggers graph extension"),
      m_eventRan(false)
{
}

NodeLevelSchedulerFutureEventTestCase::~NodeLevelSchedulerFutureEventTestCase()
{
}

void
NodeLevelSchedulerFutureEventTestCase::FarFutureHandler(uint32_t nodeId)
{
    m_eventRan = true;

    Ptr<NodeTimingGraph> graph = GetCurrentTimingGraph();
    bool graphExists = (graph != nullptr);
    NS_TEST_ASSERT_MSG_EQ(graphExists, true, "Could not retrieve NodeTimingGraph from Scheduler");

    Time maxNodeTime = graph->GetMaxNodeTime(nodeId);

    NS_TEST_ASSERT_MSG_GT(maxNodeTime,
                          Seconds(4999),
                          "Graph did not extend to cover the event time");
}

void
NodeLevelSchedulerFutureEventTestCase::DoRun()
{
    ObjectFactory schedulerFactory;
    schedulerFactory.SetTypeId("ns3::NodeLevelScheduler");
    schedulerFactory.Set("WindowSize", TimeValue(Seconds(100)));
    schedulerFactory.Set("UpdatePeriod", TimeValue(Seconds(10)));

    Simulator::SetScheduler(schedulerFactory);

    uint32_t nodeId = 2;

    Simulator::ScheduleWithContext(nodeId,
                                   Seconds(5000),
                                   &NodeLevelSchedulerFutureEventTestCase::FarFutureHandler,
                                   this,
                                   nodeId);

    Simulator::Stop(Seconds(60000));

    Simulator::Run();

    NS_TEST_ASSERT_MSG_EQ(m_eventRan, true, "Far future event failed to execute");

    Simulator::Destroy();
}

class NodeLevelSchedulerStressTestCase : public TestCase
{
  public:
    NodeLevelSchedulerStressTestCase();
    ~NodeLevelSchedulerStressTestCase() override;

  private:
    void DoRun() override;
    void StressHandler(uint32_t nodeId);
    uint32_t m_eventCount;
};

NodeLevelSchedulerStressTestCase::NodeLevelSchedulerStressTestCase()
    : TestCase("Stress test with multiple nodes and frequent events"),
      m_eventCount(0)
{
}

NodeLevelSchedulerStressTestCase::~NodeLevelSchedulerStressTestCase()
{
}

void
NodeLevelSchedulerStressTestCase::StressHandler(uint32_t nodeId)
{
    m_eventCount++;
}

void
NodeLevelSchedulerStressTestCase::DoRun()
{
    ObjectFactory schedulerFactory;
    schedulerFactory.SetTypeId("ns3::NodeLevelScheduler");
    schedulerFactory.Set("WindowSize", TimeValue(Seconds(50)));
    schedulerFactory.Set("UpdatePeriod", TimeValue(Seconds(5)));

    Simulator::SetScheduler(schedulerFactory);

    uint32_t numNodes = 50;
    uint32_t eventsPerNode = 100;

    Ptr<UniformRandomVariable> rng = CreateObject<UniformRandomVariable>();

    for (uint32_t i = 0; i < numNodes; ++i)
    {
        uint32_t nodeId = i + 10;
        for (uint32_t j = 0; j < eventsPerNode; ++j)
        {
            Time t = Seconds(rng->GetValue(1.0, 500.0));
            Simulator::ScheduleWithContext(nodeId,
                                           t,
                                           &NodeLevelSchedulerStressTestCase::StressHandler,
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

class NodeLevelSchedulerTestSuite : public TestSuite
{
  public:
    NodeLevelSchedulerTestSuite();
};

NodeLevelSchedulerTestSuite::NodeLevelSchedulerTestSuite()
    : TestSuite("node-level-scheduler", TestSuite::Type::UNIT)
{
    AddTestCase(new NodeLevelSchedulerAccuracyTestCase, TestCase::Duration::QUICK);
    AddTestCase(new NodeLevelSchedulerFutureEventTestCase, TestCase::Duration::QUICK);
    AddTestCase(new NodeLevelSchedulerStressTestCase, TestCase::Duration::TAKES_FOREVER);
}

static NodeLevelSchedulerTestSuite g_nodeLevelSchedulerTestSuite;
