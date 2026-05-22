/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "node-timing-graph.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NodeTimingGraph");
NS_OBJECT_ENSURE_REGISTERED(NodeTimingGraph);

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

} // namespace ns3
