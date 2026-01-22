/*
 * Copyright (c) 2016 IITP
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "node-level-scheduler.h"

#include "log.h"
#include "object-factory.h"
#include "simulator.h"
#include "string.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NodeLevelScheduler");

NS_OBJECT_ENSURE_REGISTERED(NodeLevelScheduler);

TypeId
NodeTimingGraph::GetTypeId()
{
    static TypeId tid = TypeId("ns3::NodeTimingGraph")
                            .SetParent<Object>()
                            .SetGroupName("Core")
                            .AddConstructor<NodeTimingGraph>();
    return tid;
}

Time
NodeTimingGraph::GetSimulatorTimeFromNodeTime(uint32_t nodeId, Time nodeTime) const
{
    auto it = m_nodeIntervals.find(nodeId);
    if (it == m_nodeIntervals.end())
    {
        return nodeTime;
    }
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
    return nodeTime;
}

void
NodeTimingGraph::PruneIntervals(Time cutoff)
{
    for (auto it = m_nodeIntervals.begin(); it != m_nodeIntervals.end();)
    {
        auto& intervals = it->second;
        auto newEnd = std::remove_if(intervals.begin(), intervals.end(), [&](const Interval& i) {
            return i.simulatorEndTime < cutoff;
        });

        intervals.erase(newEnd, intervals.end());

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
    static TypeId tid =
        TypeId("ns3::NodeLevelScheduler")
            .SetParent<Scheduler>()
            .SetGroupName("Core")
            .AddConstructor<NodeLevelScheduler>()
            .AddAttribute("Intervals",
                          "List of intervals.",
                          StringValue(""),
                          MakeStringAccessor(&NodeLevelScheduler::SetIntervalsFromString),
                          MakeStringChecker())
            .AddAttribute("WindowSize",
                          "The lookahead window for loading intervals.",
                          TimeValue(Seconds(100.0)),
                          MakeTimeAccessor(&NodeLevelScheduler::m_windowSize),
                          MakeTimeChecker())
            .AddAttribute("UpdatePeriod",
                          "How frequently to prune and load intervals.",
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
}

NodeLevelScheduler::~NodeLevelScheduler()
{
    NS_LOG_FUNCTION(this);
}

void
NodeLevelScheduler::SetIntervalsFromString(const std::string& intervalsStr)
{
    if (intervalsStr.empty())
    {
        return;
    }

    std::stringstream ss(intervalsStr);
    std::string intervalToken;

    m_pendingIntervals.clear();

    while (std::getline(ss, intervalToken, ';'))
    {
        std::stringstream interval_ss(intervalToken);
        std::string part;
        std::vector<std::string> parts;

        while (std::getline(interval_ss, part, ','))
        {
            parts.push_back(part);
        }

        if (parts.size() != 6)
        {
            NS_FATAL_ERROR("Invalid interval format: " << intervalToken);
            continue;
        }

        uint32_t nodeId = std::stoul(parts[0]);
        Time simStart = Seconds(std::stod(parts[1]));
        Time simEnd = Seconds(std::stod(parts[2]));
        Time nodeStart = Seconds(std::stod(parts[3]));
        Time nodeEnd = Seconds(std::stod(parts[4]));
        double skew = std::stod(parts[5]);

        m_pendingIntervals.push_back({nodeId, {simStart, simEnd, nodeStart, nodeEnd, skew}});
    }

    std::sort(m_pendingIntervals.begin(),
              m_pendingIntervals.end(),
              [](const PendingInterval& a, const PendingInterval& b) {
                  return a.data.simulatorStartTime < b.data.simulatorStartTime;
              });
}

void
NodeLevelScheduler::UpdateIntervalWindow()
{
    Time now = Simulator::Now();
    Time horizon = now + m_windowSize;

    NS_LOG_LOGIC("Updating Interval Table at " << now.GetSeconds()
                                               << "s. Horizon: " << horizon.GetSeconds());

    m_nodeTimings->PruneIntervals(now);

    int loadedCount = 0;
    while (!m_pendingIntervals.empty())
    {
        const auto& nextItem = m_pendingIntervals.front();

        if (nextItem.data.simulatorStartTime <= horizon)
        {
            m_nodeTimings->AddInterval(nextItem.nodeId, nextItem.data);
            m_pendingIntervals.pop_front();
            loadedCount++;
        }
        else
        {
            break;
        }
    }

    NS_LOG_LOGIC("Pruned old intervals. Loaded " << loadedCount << " new intervals.");

    Simulator::Schedule(m_updatePeriod, &NodeLevelScheduler::UpdateIntervalWindow, this);
}

void
NodeLevelScheduler::Insert(const Event& ev)
{
    if (!m_initialized)
    {
        m_initialized = true;
        UpdateIntervalWindow();
    }

    if (m_nodeTimings)
    {
        uint32_t nodeId = ev.key.m_context;
        Time nodeTime = NanoSeconds(ev.key.m_ts);

        Time simulatorTime = m_nodeTimings->GetSimulatorTimeFromNodeTime(nodeId, nodeTime);

        if (simulatorTime == nodeTime && nodeId != 0xffffffff)
        {
            NS_LOG_WARN(
                "Time translation returned identity. Interval might be missing from window.");
        }

        Event adjustedEv = ev;
        adjustedEv.key.m_ts = simulatorTime.GetNanoSeconds();
        PriorityQueueScheduler::Insert(adjustedEv);
    }
    else
    {
        PriorityQueueScheduler::Insert(ev);
    }
}

} // namespace ns3
