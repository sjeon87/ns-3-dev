/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "dynamic-skew-scheduler.h"

#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DynamicSkewScheduler");
NS_OBJECT_ENSURE_REGISTERED(DynamicSkewScheduler);

static DynamicSkewScheduler* g_currentScheduler = nullptr;

TypeId
DynamicSkewScheduler::GetTypeId()
{
    static TypeId tid = TypeId("ns3::DynamicSkewScheduler")
                            .SetParent<MapScheduler>()
                            .SetGroupName("Core")
                            .AddConstructor<DynamicSkewScheduler>()
                            .AddAttribute("MaximumSkew",
                                          "Maximum skew allowed.",
                                          DoubleValue(1.0),
                                          MakeDoubleAccessor(&DynamicSkewScheduler::m_maxSkew),
                                          MakeDoubleChecker<double>())
                            .AddAttribute("MinimumSkew",
                                          "Minimum skew allowed.",
                                          DoubleValue(0.1),
                                          MakeDoubleAccessor(&DynamicSkewScheduler::m_minSkew),
                                          MakeDoubleChecker<double>())
                            .AddAttribute("WindowSize",
                                          "The lookahead window.",
                                          TimeValue(Seconds(100.0)),
                                          MakeTimeAccessor(&DynamicSkewScheduler::m_windowSize),
                                          MakeTimeChecker())
                            .AddAttribute("UpdatePeriod",
                                          "How frequently skew changes.",
                                          TimeValue(Seconds(10.0)),
                                          MakeTimeAccessor(&DynamicSkewScheduler::m_updatePeriod),
                                          MakeTimeChecker());
    return tid;
}

DynamicSkewScheduler::DynamicSkewScheduler()
    : m_initialized(false)
{
    NS_LOG_FUNCTION(this);
    m_epochTable = CreateObject<EpochTable>();
    m_uv = CreateObject<UniformRandomVariable>();
    g_currentScheduler = this;
}

DynamicSkewScheduler::~DynamicSkewScheduler()
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
DynamicSkewScheduler::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_uv->SetStream(stream);
    return 1;
}

Ptr<EpochTable>
DynamicSkewScheduler::GetEpochTable() const
{
    return m_epochTable;
}

Ptr<EpochTable>
DynamicSkewScheduler::GetCurrentEpochTable()
{
    if (g_currentScheduler)
    {
        return g_currentScheduler->GetEpochTable();
    }
    return nullptr;
}

void
DynamicSkewScheduler::ExtendEpochTable(uint32_t nodeId, Time targetNodeTime)
{
    if (m_updatePeriod.IsZero())
    {
        m_updatePeriod = Seconds(1.0);
    }

    Time currentSimTime = Seconds(0);
    Time currentNodeTime = Seconds(0);

    if (m_epochTable->HasNode(nodeId))
    {
        currentSimTime = m_epochTable->GetMaxSimulatorTime(nodeId);
        currentNodeTime = m_epochTable->GetMaxNodeTime(nodeId);
    }
    else if (Simulator::Now() > Seconds(0))
    {
        currentSimTime = Simulator::Now();
    }

    while (currentNodeTime <= targetNodeTime)
    {
        double skew = m_uv->GetValue(m_minSkew, m_maxSkew);
        Time duration = m_updatePeriod;

        EpochTable::Epoch epoch;
        epoch.simulatorStartTime = currentSimTime;
        epoch.simulatorEndTime = currentSimTime + duration;
        epoch.nodeStartTime = currentNodeTime;
        epoch.nodeEndTime =
            currentNodeTime + Time::FromDouble(duration.GetDouble() * skew, Time::NS);
        epoch.skew = skew;

        m_epochTable->AddEpoch(nodeId, epoch);

        currentSimTime = epoch.simulatorEndTime;
        currentNodeTime = epoch.nodeEndTime;
    }
}

void
DynamicSkewScheduler::StartCleanupTask()
{
    m_cleanupEvent = Simulator::Schedule(m_windowSize, &DynamicSkewScheduler::Cleanup, this);
}

void
DynamicSkewScheduler::Cleanup()
{
    Time safeMargin = Seconds(1.0);
    Time cutoff = (Simulator::Now() > safeMargin) ? Simulator::Now() - safeMargin : Seconds(0);
    m_epochTable->PruneEpochTable(cutoff);
    m_cleanupEvent = Simulator::Schedule(m_windowSize, &DynamicSkewScheduler::Cleanup, this);
}

void
DynamicSkewScheduler::Insert(const Event& ev)
{
    if (!m_initialized)
    {
        m_initialized = true;
        StartCleanupTask();
    }

    uint32_t context = ev.key.m_context;

    if (context != 0xffffffff && m_epochTable)
    {
        if (!m_epochTable->HasNode(context))
        {
            AppendWindow(context);
        }

        Time requestedSimTs = Time::FromInteger(ev.key.m_ts, Time::GetResolution());
        Time nodeLocalTs = m_epochTable->GetNodeTimeFromSimulatorTime(context, requestedSimTs);

        ExtendEpochTable(context, nodeLocalTs);

        Event localEv = ev;
        localEv.key.m_ts = nodeLocalTs.GetTimeStep();
        m_nodeQueues[context].push(localEv);

        RebalanceNode(context);
    }
    else
    {
        MapScheduler::Insert(ev);
    }
}

Scheduler::Event
DynamicSkewScheduler::RemoveNext()
{
    Event globalEv = MapScheduler::RemoveNext();
    uint32_t context = globalEv.key.m_context;

    if (context != 0xffffffff && m_epochTable)
    {
        auto& queue = m_nodeQueues[context];

        while (!queue.empty() &&
               (queue.top().impl->IsCancelled() || m_cancelled.count(queue.top().key.m_uid)))
        {
            m_cancelled.erase(queue.top().key.m_uid);
            queue.pop();
        }

        if (!queue.empty() && queue.top().key.m_uid == globalEv.key.m_uid)
        {
            queue.pop();
        }
        else
        {
            NS_LOG_WARN("RemoveNext: local heap top UID does not match global event UID "
                        << globalEv.key.m_uid << " for node " << context);
        }

        m_activeEvents.erase(context);

        RebalanceNode(context);

        return globalEv;
    }

    return globalEv;
}

void
DynamicSkewScheduler::Remove(const Event& ev)
{
    uint32_t context = ev.key.m_context;

    if (context != 0xffffffff && m_epochTable)
    {
        Time requestedSimTs = Time::FromInteger(ev.key.m_ts, Time::GetResolution());

        if (!m_epochTable->HasNode(context))
        {
            ExtendEpochTable(context, Seconds(0) + m_windowSize);
        }

        Time nodeLocalTs = m_epochTable->GetNodeTimeFromSimulatorTime(context, requestedSimTs);

        ExtendEpochTable(context, nodeLocalTs);

        Event localEv = ev;
        localEv.key.m_ts = nodeLocalTs.GetTimeStep();
        m_nodeQueues[context].push(localEv);

        RebalanceNode(context);
    }
    else
    {
        MapScheduler::Remove(ev);
    }
}

void
DynamicSkewScheduler::RebalanceNode(uint32_t context)
{
    auto& queue = m_nodeQueues[context];

    while (!queue.empty() &&
           (queue.top().impl->IsCancelled() || m_cancelled.count(queue.top().key.m_uid)))
    {
        m_cancelled.erase(queue.top().key.m_uid);
        queue.pop();
    }

    if (queue.empty())
    {
        auto it = m_activeEvents.find(context);
        if (it != m_activeEvents.end())
        {
            MapScheduler::Remove(it->second);
            m_activeEvents.erase(context);
        }
        return;
    }

    Event topLocalEv = queue.top();
    Time localTime = Time::FromInteger(topLocalEv.key.m_ts, Time::GetResolution());
    Time simTime = m_epochTable->GetSimulatorTimeFromNodeTime(context, localTime);

    if (simTime < Simulator::Now())
    {
        simTime = Simulator::Now();
    }

    Event globalEv = topLocalEv;
    globalEv.key.m_ts = simTime.GetTimeStep();

    auto it = m_activeEvents.find(context);
    if (it != m_activeEvents.end())
    {
        if (it->second.key.m_uid == globalEv.key.m_uid && it->second.key.m_ts == globalEv.key.m_ts)
        {
            return;
        }
        MapScheduler::Remove(it->second);
        MapScheduler::Insert(globalEv);
        m_activeEvents[context] = globalEv;
    }
    else
    {
        MapScheduler::Insert(globalEv);
        m_activeEvents[context] = globalEv;
    }
}

void
DynamicSkewScheduler::ChangeSkew(uint32_t nodeId, double skew)
{
    if (!m_initialized)
    {
        m_initialized = true;
        StartCleanupTask();
    }

    if (!m_epochTable)
    {
        NS_LOG_WARN("ChangeSkew: EpochTable unavailable.");
        return;
    }

    Time simNow = Simulator::Now();
    Time localNow = m_epochTable->GetNodeTimeFromSimulatorTime(nodeId, simNow);

    Time newSimEnd = simNow + m_updatePeriod;
    Time newNodeEnd = localNow + Time::FromDouble(m_updatePeriod.GetDouble() * skew, Time::NS);

    m_epochTable->InsertEpoch(nodeId, simNow, newSimEnd, localNow, newNodeEnd, skew);

    ExtendEpochTable(nodeId, localNow + m_windowSize);

    RebalanceNode(nodeId);
}

} // namespace ns3
