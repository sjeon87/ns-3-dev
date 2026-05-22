/*
 * Copyright (c) 2016 IITP
 * Copyright (c) 2025 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "node-level-scheduler.h"

#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NodeLevelScheduler");

NS_OBJECT_ENSURE_REGISTERED(NodeTimingGraph);
NS_OBJECT_ENSURE_REGISTERED(NodeLevelScheduler);

static NodeLevelScheduler* g_currentScheduler = nullptr;

TypeId
NodeTimingGraph::GetTypeId()
{
    static TypeId tid = TypeId("ns3::NodeTimingGraph")
                            .SetParent<Object>()
                            .SetGroupName("Core")
                            .AddConstructor<NodeTimingGraph>();
    return tid;
}

NodeTimingGraph::NodeTimingGraph()
{
    NS_LOG_FUNCTION(this);
}

NodeTimingGraph::~NodeTimingGraph()
{
    NS_LOG_FUNCTION(this);
}

void
NodeTimingGraph::AddInterval(uint32_t nodeId, const Interval& interval)
{
    m_nodeIntervals[nodeId].push_back(interval);
    m_lastSimEndTime[nodeId] = interval.simulatorEndTime;
    m_lastNodeEndTime[nodeId] = interval.nodeEndTime;
    m_lastSkew[nodeId] = interval.skew;
}

bool
NodeTimingGraph::HasNode(uint32_t nodeId) const
{
    return m_nodeIntervals.find(nodeId) != m_nodeIntervals.end() ||
           m_lastSimEndTime.find(nodeId) != m_lastSimEndTime.end();
}

Time
NodeTimingGraph::GetSimulatorTimeFromNodeTime(uint32_t nodeId, Time nodeTime) const
{
    auto it = m_nodeIntervals.find(nodeId);
    if (it != m_nodeIntervals.end())
    {
        const auto& intervals = it->second;
        for (const auto& interval : intervals)
        {
            if (nodeTime >= interval.nodeStartTime && nodeTime < interval.nodeEndTime)
            {
                Time deltaNodeTime = (nodeTime - interval.nodeStartTime);
                Time scaledDelta = Seconds(deltaNodeTime.GetSeconds() / interval.skew);
                return interval.simulatorStartTime + scaledDelta;
            }
        }
    }

    auto itEnd = m_lastNodeEndTime.find(nodeId);
    if (itEnd != m_lastNodeEndTime.end())
    {
        Time lastNodeEnd = itEnd->second;
        if (nodeTime >= lastNodeEnd)
        {
            Time lastSimEnd = m_lastSimEndTime.at(nodeId);
            double skew = m_lastSkew.at(nodeId);
            Time deltaNodeTime = (nodeTime - lastNodeEnd);
            Time scaledDelta = Seconds(deltaNodeTime.GetSeconds() / skew);
            return lastSimEnd + scaledDelta;
        }
    }

    return nodeTime;
}

Time
NodeTimingGraph::GetNodeTimeFromSimulatorTime(uint32_t nodeId, Time simulatorTime) const
{
    auto it = m_nodeIntervals.find(nodeId);
    if (it != m_nodeIntervals.end())
    {
        const auto& intervals = it->second;
        for (const auto& interval : intervals)
        {
            if (simulatorTime >= interval.simulatorStartTime &&
                simulatorTime < interval.simulatorEndTime)
            {
                Time deltaSimTime = (simulatorTime - interval.simulatorStartTime);
                Time scaledDelta = Seconds(deltaSimTime.GetSeconds() * interval.skew);
                return interval.nodeStartTime + scaledDelta;
            }
        }
    }

    auto itEnd = m_lastSimEndTime.find(nodeId);
    if (itEnd != m_lastSimEndTime.end())
    {
        Time lastSimEnd = itEnd->second;
        if (simulatorTime >= lastSimEnd)
        {
            Time lastNodeEnd = m_lastNodeEndTime.at(nodeId);
            double skew = m_lastSkew.at(nodeId);
            Time deltaSimTime = (simulatorTime - lastSimEnd);
            Time scaledDelta = Seconds(deltaSimTime.GetSeconds() * skew);
            return lastNodeEnd + scaledDelta;
        }
    }
    return simulatorTime;
}

Time
NodeTimingGraph::GetMaxNodeTime(uint32_t nodeId) const
{
    auto it = m_lastNodeEndTime.find(nodeId);
    return (it != m_lastNodeEndTime.end()) ? it->second : Seconds(0.0);
}

Time
NodeTimingGraph::GetMaxSimulatorTime(uint32_t nodeId) const
{
    auto it = m_lastSimEndTime.find(nodeId);
    return (it != m_lastSimEndTime.end()) ? it->second : Seconds(0.0);
}

void
NodeTimingGraph::PruneIntervals(Time cutoff)
{
    for (auto it = m_nodeIntervals.begin(); it != m_nodeIntervals.end();)
    {
        auto& intervals = it->second;
        auto eraseIt = intervals.begin();
        while (eraseIt != intervals.end() && eraseIt->simulatorEndTime < cutoff)
        {
            eraseIt++;
        }
        intervals.erase(intervals.begin(), eraseIt);

        if (intervals.empty())
        {
            it = m_nodeIntervals.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

TypeId
NodeLevelScheduler::GetTypeId()
{
    static TypeId tid = TypeId("ns3::NodeLevelScheduler")
                            .SetParent<MapScheduler>()
                            .SetGroupName("Core")
                            .AddConstructor<NodeLevelScheduler>()
                            .AddAttribute("MaximumSkew",
                                          "Maximum skew allowed.",
                                          DoubleValue(1.0),
                                          MakeDoubleAccessor(&NodeLevelScheduler::m_maxSkew),
                                          MakeDoubleChecker<double>())
                            .AddAttribute("MinimumSkew",
                                          "Minimum skew allowed.",
                                          DoubleValue(0.1),
                                          MakeDoubleAccessor(&NodeLevelScheduler::m_minSkew),
                                          MakeDoubleChecker<double>())
                            .AddAttribute("WindowSize",
                                          "The lookahead window.",
                                          TimeValue(Seconds(100.0)),
                                          MakeTimeAccessor(&NodeLevelScheduler::m_windowSize),
                                          MakeTimeChecker())
                            .AddAttribute("UpdatePeriod",
                                          "How frequently skew changes.",
                                          TimeValue(Seconds(10.0)),
                                          MakeTimeAccessor(&NodeLevelScheduler::m_updatePeriod),
                                          MakeTimeChecker());
    return tid;
}

NodeLevelScheduler::NodeLevelScheduler()
    : m_initialized(false)
{
    NS_LOG_FUNCTION(this);
    m_nodeTimings = CreateObject<NodeTimingGraph>();
    g_currentScheduler = this;
}

NodeLevelScheduler::~NodeLevelScheduler()
{
    NS_LOG_FUNCTION(this);
    Simulator::Cancel(m_cleanupEvent);
    m_nodeTimings = nullptr;
    if (g_currentScheduler == this)
    {
        g_currentScheduler = nullptr;
    }
}

Ptr<NodeTimingGraph>
NodeLevelScheduler::GetTimingGraph() const
{
    return m_nodeTimings;
}

Ptr<NodeTimingGraph>
NodeLevelScheduler::GetCurrentGraph()
{
    if (g_currentScheduler)
    {
        return g_currentScheduler->GetTimingGraph();
    }
    return nullptr;
}

void
NodeLevelScheduler::AppendWindow(uint32_t nodeId)
{
    if (m_updatePeriod.IsZero())
    {
        m_updatePeriod = Seconds(1.0);
    }
    auto numIntervals =
        static_cast<uint32_t>(m_windowSize.GetSeconds() / m_updatePeriod.GetSeconds());
    if (numIntervals == 0)
    {
        numIntervals = 1;
    }

    Time currentSimTime = m_nodeTimings->GetMaxSimulatorTime(nodeId);
    Time currentNodeTime = m_nodeTimings->GetMaxNodeTime(nodeId);

    if (currentSimTime.IsZero() && Simulator::Now() > Seconds(0))
    {
        currentSimTime = Simulator::Now();
    }

    Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();

    for (uint32_t i = 0; i < numIntervals; ++i)
    {
        double skew = uv->GetValue(m_minSkew, m_maxSkew);
        Time duration = m_updatePeriod;

        NodeTimingGraph::Interval interval;
        interval.simulatorStartTime = currentSimTime;
        interval.simulatorEndTime = currentSimTime + duration;
        interval.nodeStartTime = currentNodeTime;
        interval.nodeEndTime = currentNodeTime + Seconds(duration.GetSeconds() * skew);
        interval.skew = skew;

        m_nodeTimings->AddInterval(nodeId, interval);

        currentSimTime = interval.simulatorEndTime;
        currentNodeTime = interval.nodeEndTime;
    }
}

void
NodeLevelScheduler::ExtendTimingGraph(uint32_t nodeId, Time targetNodeTime)
{
    Time currentMaxNode = m_nodeTimings->GetMaxNodeTime(nodeId);
    while (currentMaxNode <= targetNodeTime + NanoSeconds(1))
    {
        AppendWindow(nodeId);
        currentMaxNode = m_nodeTimings->GetMaxNodeTime(nodeId);
    }
}

void
NodeLevelScheduler::StartCleanupTask()
{
    m_cleanupEvent = Simulator::Schedule(m_windowSize, &NodeLevelScheduler::Cleanup, this);
}

void
NodeLevelScheduler::Cleanup()
{
    Time safeMargin = Seconds(1.0);
    Time cutoff = (Simulator::Now() > safeMargin) ? Simulator::Now() - safeMargin : Seconds(0);
    m_nodeTimings->PruneIntervals(cutoff);
    m_cleanupEvent = Simulator::Schedule(m_windowSize, &NodeLevelScheduler::Cleanup, this);
}

void
NodeLevelScheduler::Insert(const Event& ev)
{
    if (!m_initialized)
    {
        m_initialized = true;
        StartCleanupTask();
    }

    uint32_t context = ev.key.m_context;

    if (context != 0xffffffff && m_nodeTimings)
    {
        if (!m_nodeTimings->HasNode(context))
        {
            AppendWindow(context);
        }

        Time requestedTs = Time::FromInteger(ev.key.m_ts, Time::GetResolution());
        Time simNow = Simulator::Now();

        Time delay = requestedTs - simNow;
        if (delay.IsNegative())
        {
            delay = Seconds(0);
        }

        Time currentNodeTime = m_nodeTimings->GetNodeTimeFromSimulatorTime(context, simNow);

        Time targetNodeTime = currentNodeTime + delay;

        ExtendTimingGraph(context, targetNodeTime);

        Time targetSimTime = m_nodeTimings->GetSimulatorTimeFromNodeTime(context, targetNodeTime);

        if (targetSimTime < simNow)
        {
            targetSimTime = simNow;
        }

        Event adjustedEv = ev;
        adjustedEv.key.m_ts = targetSimTime.GetTimeStep();
        MapScheduler::Insert(adjustedEv);
    }
    else
    {
        MapScheduler::Insert(ev);
    }
}

} // namespace ns3
