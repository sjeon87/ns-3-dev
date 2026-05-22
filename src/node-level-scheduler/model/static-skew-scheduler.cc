/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "static-skew-scheduler.h"

#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("StaticSkewScheduler");
NS_OBJECT_ENSURE_REGISTERED(StaticSkewScheduler);

static StaticSkewScheduler* g_currentScheduler = nullptr;

TypeId
StaticSkewScheduler::GetTypeId()
{
    static TypeId tid = TypeId("ns3::StaticSkewScheduler")
                            .SetParent<MapScheduler>()
                            .SetGroupName("Core")
                            .AddConstructor<StaticSkewScheduler>()
                            .AddAttribute("MaximumSkew",
                                          "Maximum skew allowed.",
                                          DoubleValue(1.0),
                                          MakeDoubleAccessor(&StaticSkewScheduler::m_maxSkew),
                                          MakeDoubleChecker<double>())
                            .AddAttribute("MinimumSkew",
                                          "Minimum skew allowed.",
                                          DoubleValue(0.1),
                                          MakeDoubleAccessor(&StaticSkewScheduler::m_minSkew),
                                          MakeDoubleChecker<double>())
                            .AddAttribute("WindowSize",
                                          "The lookahead window.",
                                          TimeValue(Seconds(100.0)),
                                          MakeTimeAccessor(&StaticSkewScheduler::m_windowSize),
                                          MakeTimeChecker())
                            .AddAttribute("UpdatePeriod",
                                          "How frequently skew changes.",
                                          TimeValue(Seconds(10.0)),
                                          MakeTimeAccessor(&StaticSkewScheduler::m_updatePeriod),
                                          MakeTimeChecker());
    return tid;
}

StaticSkewScheduler::StaticSkewScheduler()
    : m_initialized(false)
{
    NS_LOG_FUNCTION(this);
    m_nodeTimings = CreateObject<NodeTimingGraph>();
    m_uv = CreateObject<UniformRandomVariable>();
    g_currentScheduler = this;
}

StaticSkewScheduler::~StaticSkewScheduler()
{
    NS_LOG_FUNCTION(this);
    Simulator::Cancel(m_cleanupEvent);
    m_nodeTimings = nullptr;
    if (g_currentScheduler == this)
    {
        g_currentScheduler = nullptr;
    }
}

int64_t
StaticSkewScheduler::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_uv->SetStream(stream);
    return 1;
}

Ptr<NodeTimingGraph>
StaticSkewScheduler::GetTimingGraph() const
{
    return m_nodeTimings;
}

Ptr<NodeTimingGraph>
StaticSkewScheduler::GetCurrentGraph()
{
    if (g_currentScheduler)
    {
        return g_currentScheduler->GetTimingGraph();
    }
    return nullptr;
}

void
StaticSkewScheduler::AppendWindow(uint32_t nodeId)
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

    for (uint32_t i = 0; i < numIntervals; ++i)
    {
        double skew = m_uv->GetValue(m_minSkew, m_maxSkew);
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
StaticSkewScheduler::ExtendTimingGraph(uint32_t nodeId, Time targetNodeTime)
{
    Time currentMaxNode = m_nodeTimings->GetMaxNodeTime(nodeId);
    while (currentMaxNode <= targetNodeTime + NanoSeconds(1))
    {
        AppendWindow(nodeId);
        currentMaxNode = m_nodeTimings->GetMaxNodeTime(nodeId);
    }
}

void
StaticSkewScheduler::StartCleanupTask()
{
    m_cleanupEvent = Simulator::Schedule(m_windowSize, &StaticSkewScheduler::Cleanup, this);
}

void
StaticSkewScheduler::Cleanup()
{
    Time safeMargin = Seconds(1.0);
    Time cutoff = (Simulator::Now() > safeMargin) ? Simulator::Now() - safeMargin : Seconds(0);
    m_nodeTimings->PruneIntervals(cutoff);
    m_cleanupEvent = Simulator::Schedule(m_windowSize, &StaticSkewScheduler::Cleanup, this);
}

void
StaticSkewScheduler::Insert(const Event& ev)
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
