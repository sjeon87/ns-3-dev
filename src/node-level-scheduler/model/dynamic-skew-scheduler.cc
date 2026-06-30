/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "dynamic-skew-scheduler.h"

#include "ns3/assert.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

#include <limits>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DynamicSkewScheduler");
NS_OBJECT_ENSURE_REGISTERED(DynamicSkewScheduler);

static DynamicSkewScheduler* g_currentScheduler =
    nullptr; //!< The currently active scheduler instance

void
ProjectedQueue::Swap(size_t i, size_t j)
{
    std::swap(m_heapArray[i], m_heapArray[j]);
    m_indexMap[m_heapArray[i].key.m_context] = i;
    m_indexMap[m_heapArray[j].key.m_context] = j;
}

void
ProjectedQueue::BubbleUp(size_t idx)
{
    while (idx > 0)
    {
        size_t parent = (idx - 1) / 2;
        if (m_heapArray[idx].key.m_ts < m_heapArray[parent].key.m_ts)
        {
            Swap(idx, parent);
            idx = parent;
        }
        else
        {
            break;
        }
    }
}

void
ProjectedQueue::BubbleDown(size_t idx)
{
    size_t size = m_heapArray.size();
    while (true)
    {
        size_t left = 2 * idx + 1;
        size_t right = 2 * idx + 2;
        size_t smallest = idx;

        if (left < size && m_heapArray[left].key.m_ts < m_heapArray[smallest].key.m_ts)
        {
            smallest = left;
        }
        if (right < size && m_heapArray[right].key.m_ts < m_heapArray[smallest].key.m_ts)
        {
            smallest = right;
        }

        if (smallest != idx)
        {
            Swap(idx, smallest);
            idx = smallest;
        }
        else
        {
            break;
        }
    }
}

void
ProjectedQueue::Update(uint32_t nodeId, const Scheduler::Event& ev)
{
    auto it = m_indexMap.find(nodeId);
    if (it == m_indexMap.end())
    {
        size_t idx = m_heapArray.size();
        m_heapArray.push_back(ev);
        m_indexMap[nodeId] = idx;
        BubbleUp(idx);
    }
    else
    {
        size_t idx = it->second;
        uint64_t oldTs = m_heapArray[idx].key.m_ts;
        m_heapArray[idx] = ev;

        if (ev.key.m_ts < oldTs)
        {
            BubbleUp(idx);
        }
        else if (ev.key.m_ts > oldTs)
        {
            BubbleDown(idx);
        }
    }
}

Scheduler::Event
ProjectedQueue::Top() const
{
    if (m_heapArray.empty())
    {
        Scheduler::Event nullEv;
        nullEv.key.m_ts = std::numeric_limits<uint64_t>::max();
        return nullEv;
    }
    return m_heapArray[0];
}

bool
ProjectedQueue::IsEmpty() const
{
    if (m_heapArray.empty())
    {
        return true;
    }
    return m_heapArray[0].key.m_ts == std::numeric_limits<uint64_t>::max();
}

TypeId
DynamicSkewScheduler::GetTypeId()
{
    static TypeId tid = TypeId("ns3::DynamicSkewScheduler")
                            .SetParent<Scheduler>()
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
DynamicSkewScheduler::ChangeCurrentSkew(uint32_t nodeId, double skew)
{
    if (g_currentScheduler)
    {
        g_currentScheduler->ChangeSkew(nodeId, skew);
    }
    else
    {
        NS_LOG_WARN("ChangeCurrentSkew: no DynamicSkewScheduler is currently active.");
    }
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

    bool anyPending = false;
    for (const auto& [nodeId, queue] : m_nodeQueues)
    {
        if (!queue.empty())
        {
            anyPending = true;
            break;
        }
    }

    if (anyPending)
    {
        m_cleanupEvent = Simulator::Schedule(m_windowSize, &DynamicSkewScheduler::Cleanup, this);
    }
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

    if (context == ns3::Simulator::NO_CONTEXT)
    {
        m_globalQueue.push(ev);
        return;
    }

    if (m_epochTable)
    {
        Time simNow = Simulator::Now();
        Time scheduledGlobalTs = Time::FromInteger(ev.key.m_ts, Time::GetResolution());

        if (!m_epochTable->HasNode(context))
        {
            ExtendEpochTable(context, simNow + m_windowSize);
        }

        Time delay = scheduledGlobalTs - simNow;
        Time localNow = m_epochTable->GetNodeTimeFromSimulatorTime(context, simNow);
        Time nodeLocalTs = localNow + delay;

        ExtendEpochTable(context, nodeLocalTs);

        Event localEv = ev;
        localEv.key.m_ts = nodeLocalTs.GetTimeStep();

        auto& queue = m_nodeQueues[context];
        bool wasEmpty = queue.empty();

        uint32_t oldTopUid;
        if (wasEmpty)
        {
            oldTopUid = 0;
        }
        else
        {
            oldTopUid = queue.top().key.m_uid;
        }

        queue.push(localEv);

        if (wasEmpty || queue.top().key.m_uid != oldTopUid)
        {
            Event newTop = queue.top();
            Time simTs = m_epochTable->GetSimulatorTimeFromNodeTime(
                context,
                Time::FromInteger(newTop.key.m_ts, Time::GetResolution()));

            Time now = Simulator::Now();
            if (simTs < now)
            {
                simTs = now;
            }

            newTop.key.m_ts = simTs.GetTimeStep();
            m_projectedQueue.Update(context, newTop);
        }
    }
}

Scheduler::Event
DynamicSkewScheduler::PeekNext() const
{
    Event globalTop;
    globalTop.key.m_ts = std::numeric_limits<uint64_t>::max();
    if (!m_globalQueue.empty())
    {
        globalTop = m_globalQueue.top();
    }

    Event projTop = m_projectedQueue.Top();

    if (globalTop.key.m_ts <= projTop.key.m_ts)
    {
        return globalTop;
    }
    return projTop;
}

bool
DynamicSkewScheduler::IsEmpty() const
{
    return m_globalQueue.empty() && m_projectedQueue.IsEmpty();
}

Scheduler::Event
DynamicSkewScheduler::RemoveNext()
{
    Event globalTop;
    globalTop.key.m_ts = std::numeric_limits<uint64_t>::max();

    while (!m_globalQueue.empty() && (m_globalQueue.top().impl->IsCancelled() ||
                                      m_cancelled.count(m_globalQueue.top().key.m_uid)))
    {
        m_cancelled.erase(m_globalQueue.top().key.m_uid);
        m_globalQueue.pop();
    }

    if (!m_globalQueue.empty())
    {
        globalTop = m_globalQueue.top();
    }

    Event projTop = m_projectedQueue.Top();

    if (globalTop.key.m_ts < projTop.key.m_ts)
    {
        m_globalQueue.pop();
        return globalTop;
    }

    uint32_t context = projTop.key.m_context;
    auto& queue = m_nodeQueues[context];

    Event localTarget = queue.top();
    queue.pop();

    while (!queue.empty() &&
           (queue.top().impl->IsCancelled() || m_cancelled.count(queue.top().key.m_uid)))
    {
        m_cancelled.erase(queue.top().key.m_uid);
        queue.pop();
    }

    if (!queue.empty())
    {
        Event newTop = queue.top();
        Time simTs = m_epochTable->GetSimulatorTimeFromNodeTime(
            context,
            Time::FromInteger(newTop.key.m_ts, Time::GetResolution()));
        Time now = Simulator::Now();
        if (simTs < now)
        {
            simTs = now;
        }
        newTop.key.m_ts = simTs.GetTimeStep();
        m_projectedQueue.Update(context, newTop);
    }
    else
    {
        Event nullEv;
        nullEv.key.m_ts = std::numeric_limits<uint64_t>::max();
        nullEv.key.m_context = context;
        m_projectedQueue.Update(context, nullEv);
    }

    localTarget.key.m_ts = projTop.key.m_ts;
    return localTarget;
}

void
DynamicSkewScheduler::Remove(const Event& ev)
{
    m_cancelled.insert(ev.key.m_uid);
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
    Time localNow;

    if (m_epochTable->HasNode(nodeId))
    {
        localNow = m_epochTable->GetNodeTimeFromSimulatorTime(nodeId, simNow);
    }
    else
    {
        localNow = simNow;
    }

    Time newSimEnd = simNow + m_updatePeriod;

    auto scaledSteps = static_cast<int64_t>(m_updatePeriod.GetTimeStep() * skew);
    Time newNodeEnd = localNow + Time::FromInteger(scaledSteps, Time::GetResolution());

    m_epochTable->InsertEpoch(nodeId, simNow, newSimEnd, localNow, newNodeEnd, skew);

    ExtendEpochTable(nodeId, localNow + m_windowSize);

    auto& queue = m_nodeQueues[nodeId];
    if (!queue.empty())
    {
        Event top = queue.top();
        Time newSimTs = m_epochTable->GetSimulatorTimeFromNodeTime(
            nodeId,
            Time::FromInteger(top.key.m_ts, Time::GetResolution()));
        Time now = Simulator::Now();
        if (newSimTs < now)
        {
            newSimTs = now;
        }
        top.key.m_ts = newSimTs.GetTimeStep();
        m_projectedQueue.Update(nodeId, top);
    }
}

} // namespace ns3
