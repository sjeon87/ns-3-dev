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
                                          "How frequently skew changes (upsilon).",
                                          TimeValue(Seconds(10.0)),
                                          MakeTimeAccessor(&StaticSkewScheduler::m_updatePeriod),
                                          MakeTimeChecker());
    return tid;
}

StaticSkewScheduler::StaticSkewScheduler()
    : m_initialized(false)
{
    NS_LOG_FUNCTION(this);
    m_epochTable = CreateObject<EpochTable>();
    m_uv = CreateObject<UniformRandomVariable>();
    g_currentScheduler = this;
}

StaticSkewScheduler::~StaticSkewScheduler()
{
    NS_LOG_FUNCTION(this);
    Simulator::Cancel(m_cleanupEvent);
    m_epochTable = nullptr;
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

Ptr<EpochTable>
StaticSkewScheduler::GetEpochTable() const
{
    return m_epochTable;
}

Ptr<EpochTable>
StaticSkewScheduler::GetCurrentEpochTable()
{
    if (g_currentScheduler)
    {
        return g_currentScheduler->GetEpochTable();
    }
    return nullptr;
}

void
StaticSkewScheduler::ExtendEpochTable(uint32_t nodeId, Time targetNodeTime)
{
    NS_LOG_FUNCTION(this << nodeId << targetNodeTime);

    Time nodeMax = Seconds(0);
    Time simMax = Seconds(0);

    if (m_epochTable->HasNode(nodeId))
    {
        nodeMax = m_epochTable->GetMaxNodeTime(nodeId);
        simMax = m_epochTable->GetMaxSimulatorTime(nodeId);
    }

    if (nodeMax >= targetNodeTime)
    {
        return;
    }

    while (nodeMax < targetNodeTime)
    {
        double skew = m_uv->GetValue(m_minSkew, m_maxSkew);

        Time newSimStart = simMax;
        Time newSimEnd = newSimStart + m_updatePeriod;

        Time newNodeStart = nodeMax;
        Time deltaNode = Time::FromDouble(m_updatePeriod.GetDouble() * skew, Time::NS);
        Time newNodeEnd = newNodeStart + deltaNode;

        EpochTable::Epoch epoch;
        epoch.simulatorStartTime = newSimStart;
        epoch.simulatorEndTime = newSimEnd;
        epoch.nodeStartTime = newNodeStart;
        epoch.nodeEndTime = newNodeEnd;
        epoch.skew = skew;

        m_epochTable->AddEpoch(nodeId, epoch);

        simMax = newSimEnd;
        nodeMax = newNodeEnd;
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
    m_epochTable->PruneEpochTable(cutoff);
    m_cleanupEvent = Simulator::Schedule(m_windowSize, &StaticSkewScheduler::Cleanup, this);
}

void
StaticSkewScheduler::Insert(const Event& ev)
{
    NS_LOG_FUNCTION(this << ev.key.m_ts << ev.key.m_context);

    if (!m_initialized)
    {
        m_initialized = true;
        StartCleanupTask();
    }

    uint32_t context = ev.key.m_context;

    if (context != 0xffffffff && m_epochTable)
    {
        Time requestedTs = Time::FromInteger(ev.key.m_ts, Time::GetResolution());

        ExtendEpochTable(context, requestedTs);

        Time targetSimTime = m_epochTable->GetSimulatorTimeFromNodeTime(context, requestedTs);

        if (targetSimTime < Simulator::Now())
        {
            targetSimTime = Simulator::Now();
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
