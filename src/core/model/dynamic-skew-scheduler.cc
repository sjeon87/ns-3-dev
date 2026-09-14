/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "dynamic-skew-scheduler.h"

#include "assert.h"
#include "double.h"
#include "log.h"
#include "simulator.h"

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
                            .AddAttribute("UpdatePeriod",
                                          "How long a skew holds before it is redrawn.",
                                          TimeValue(Seconds(10.0)),
                                          MakeTimeAccessor(&DynamicSkewScheduler::m_updatePeriod),
                                          MakeTimeChecker());
    return tid;
}

DynamicSkewScheduler::DynamicSkewScheduler()
{
    NS_LOG_FUNCTION(this);
    m_epochTable = CreateObject<EpochTable>();
    m_uv = CreateObject<UniformRandomVariable>();
    g_currentScheduler = this;
}

DynamicSkewScheduler::~DynamicSkewScheduler()
{
    NS_LOG_FUNCTION(this);
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

Time
DynamicSkewScheduler::GetEffectiveUpdatePeriod() const
{
    return m_updatePeriod.IsZero() ? Seconds(1.0) : m_updatePeriod;
}

void
DynamicSkewScheduler::ApplySkew(uint32_t nodeId, double skew)
{
    NS_LOG_FUNCTION(this << nodeId << skew);

    Time simNow = Simulator::Now();
    Time localNow = m_epochTable->HasNode(nodeId)
                        ? m_epochTable->GetNodeTimeFromSimulatorTime(nodeId, simNow)
                        : simNow;

    // The anchor nominally spans one update period, which is how long this skew holds before
    // it is redrawn. Times past its end are resolved by extrapolating at the same skew, so the
    // anchor also stays valid for events scheduled further ahead than that.
    Time span = GetEffectiveUpdatePeriod();
    Time nodeSpan = Time::FromDouble(span.GetDouble() * skew, Time::NS);
    if (nodeSpan <= Time(0))
    {
        // A very small skew can round the node-side span down to nothing, which would leave
        // the anchor ill-formed and the node without a clock at all.
        nodeSpan = TimeStep(1);
    }

    EpochTable::Epoch anchor;
    anchor.simulatorStartTime = simNow;
    anchor.simulatorEndTime = simNow + span;
    anchor.nodeStartTime = localNow;
    anchor.nodeEndTime = localNow + nodeSpan;
    anchor.skew = skew;

    m_epochTable->SetSingleEpoch(nodeId, anchor);
}

void
DynamicSkewScheduler::EnsureAnchor(uint32_t nodeId)
{
    if (m_epochTable->HasNode(nodeId))
    {
        // The node already has a clock, supplied from outside the scheduler. Leave it alone:
        // callers pin a node's epoch to make it a fixed time reference, and redrawing its skew
        // would defeat that.
        return;
    }
    m_selfAnchored.insert(nodeId);
    ApplySkew(nodeId, m_uv->GetValue(m_minSkew, m_maxSkew));
}

void
DynamicSkewScheduler::ReprojectTop(uint32_t nodeId)
{
    auto it = m_nodeQueues.find(nodeId);
    if (it == m_nodeQueues.end() || it->second.empty())
    {
        return;
    }

    Event top = it->second.top();
    Time simTs = m_epochTable->GetSimulatorTimeFromNodeTime(
        nodeId,
        Time::FromInteger(top.key.m_ts, Time::GetResolution()));

    Time now = Simulator::Now();
    if (simTs < now)
    {
        simTs = now;
    }

    top.key.m_ts = simTs.GetTimeStep();
    m_projectedQueue.Update(nodeId, top);
}

void
DynamicSkewScheduler::ScheduleSkewRedraw(uint32_t nodeId)
{
    // Mark the redraw outstanding before scheduling it. Simulator::Schedule re-enters Insert,
    // which consults this flag to decide whether the chain needs starting; were the flag set
    // afterwards, that nested call would schedule a further redraw and recurse without bound.
    m_redrawPending.insert(nodeId);
    uint64_t generation = ++m_redrawGen[nodeId];

    // A redraw is scheduler housekeeping rather than a node's own event. Scheduling it with
    // whatever context happens to be executing, as Simulator::Schedule would, files it in that
    // node's local queue and reinterprets its delay as that node's local time.
    Simulator::ScheduleWithContext(Simulator::NO_CONTEXT,
                                   GetEffectiveUpdatePeriod(),
                                   &DynamicSkewScheduler::RedrawSkew,
                                   this,
                                   nodeId,
                                   generation);
}

void
DynamicSkewScheduler::RedrawSkew(uint32_t nodeId, uint64_t generation)
{
    if (generation != m_redrawGen[nodeId])
    {
        // A correction reset this node's timer after the redraw was scheduled, so a newer one
        // is outstanding and this generation is stale.
        return;
    }
    m_redrawPending.erase(nodeId);

    ApplySkew(nodeId, m_uv->GetValue(m_minSkew, m_maxSkew));
    ReprojectTop(nodeId);

    // Only continue the redraw chain while the node still has work pending, so a quiescent
    // node cannot hold Simulator::Run() open forever.
    auto it = m_nodeQueues.find(nodeId);
    if (m_selfAnchored.count(nodeId) && it != m_nodeQueues.end() && !it->second.empty())
    {
        ScheduleSkewRedraw(nodeId);
    }
}

void
DynamicSkewScheduler::Insert(const Event& ev)
{
    uint32_t context = ev.key.m_context;

    if (context == ns3::Simulator::NO_CONTEXT)
    {
        m_globalQueue.push(ev);
        return;
    }

    if (!m_epochTable)
    {
        return;
    }

    EnsureAnchor(context);

    Time simNow = Simulator::Now();
    Time scheduledGlobalTs = Time::FromInteger(ev.key.m_ts, Time::GetResolution());
    Time delay = scheduledGlobalTs - simNow;
    Time nodeLocalTs = m_epochTable->GetNodeTimeFromSimulatorTime(context, simNow) + delay;

    Event localEv = ev;
    localEv.key.m_ts = nodeLocalTs.GetTimeStep();

    auto& queue = m_nodeQueues[context];
    bool wasEmpty = queue.empty();
    uint32_t oldTopUid = wasEmpty ? 0 : queue.top().key.m_uid;

    queue.push(localEv);

    if (wasEmpty || queue.top().key.m_uid != oldTopUid)
    {
        ReprojectTop(context);
    }

    // Restart the free-running drift chain if it has lapsed, which it has both for a node
    // seen for the first time and for one whose queue previously drained. Nodes whose clock
    // was pinned from outside the scheduler never drift, so they are skipped.
    if (m_selfAnchored.count(context) && !m_redrawPending.count(context))
    {
        ScheduleSkewRedraw(context);
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
        ReprojectTop(context);
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
    if (!m_epochTable)
    {
        NS_LOG_WARN("ChangeSkew: EpochTable unavailable.");
        return;
    }

    ApplySkew(nodeId, skew);
    ReprojectTop(nodeId);

    // The correction holds for one update period, after which free-running drift resumes.
    // A pinned node stays exactly where the caller put it.
    auto it = m_nodeQueues.find(nodeId);
    if (m_selfAnchored.count(nodeId) && it != m_nodeQueues.end() && !it->second.empty())
    {
        ScheduleSkewRedraw(nodeId);
    }
}

} // namespace ns3
