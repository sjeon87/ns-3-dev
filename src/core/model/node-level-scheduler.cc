/*
 * Copyright (c) 2016 IITP
 * Copyright (c) 2025 Michigan State University
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
#include <fstream>
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
        
        auto newEnd = std::remove_if(intervals.begin(), intervals.end(),
                                     [&](const Interval& i) {
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
            .AddAttribute("IntervalFile",
                          "Path to the CSV file containing sorted intervals. "
                          "Format: nodeId,simStart,simEnd,nodeStart,nodeEnd,skew",
                          StringValue(""),
                          MakeStringAccessor(&NodeLevelScheduler::SetIntervalFile),
                          MakeStringChecker())
            .AddAttribute("WindowSize",
                          "The lookahead window for loading intervals from disk to RAM.",
                          TimeValue(Seconds(100.0)),
                          MakeTimeAccessor(&NodeLevelScheduler::m_windowSize),
                          MakeTimeChecker())
            .AddAttribute("UpdatePeriod",
                          "How frequently to run the maintenance task (prune old / load new).",
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
    if (m_intervalStream.is_open())
    {
        m_intervalStream.close();
    }
}

void
NodeLevelScheduler::SetIntervalFile(const std::string& filepath)
{
    m_intervalsFilePath = filepath;
}

bool
NodeLevelScheduler::ParseNextLine()
{
    if (!m_intervalStream.is_open())
    {
        if (m_intervalsFilePath.empty())
        {
            return false;
        }
        
        m_intervalStream.open(m_intervalsFilePath);
        if (!m_intervalStream.is_open())
        {
            NS_FATAL_ERROR("NodeLevelScheduler: Could not open interval file: " << m_intervalsFilePath);
            return false;
        }
    }

    std::string line;
    while (std::getline(m_intervalStream, line))
    {
        if (line.empty() || line[0] == '#') 
        {
            continue;
        }

        std::stringstream ss(line);
        std::string part;
        std::vector<std::string> parts;

        while (std::getline(ss, part, ','))
        {
            parts.push_back(part);
        }

        if (parts.size() != 6)
        {
            NS_LOG_WARN("Skipping invalid line format in interval file: " << line);
            continue;
        }

        try 
        {
            PendingInterval p;
            p.nodeId = std::stoul(parts[0]);
            p.data.simulatorStartTime = Seconds(std::stod(parts[1]));
            p.data.simulatorEndTime = Seconds(std::stod(parts[2]));
            p.data.nodeStartTime = Seconds(std::stod(parts[3]));
            p.data.nodeEndTime = Seconds(std::stod(parts[4]));
            p.data.skew = std::stod(parts[5]);
            
            m_nextBufferedInterval = p;
            return true;
        } 
        catch (const std::exception& e) 
        {
            NS_LOG_WARN("Error parsing interval values: " << line << " (" << e.what() << ")");
            continue;
        }
    }

    return false;
}

void
NodeLevelScheduler::UpdateIntervalWindow()
{
    Time now = Simulator::Now();
    Time horizon = now + m_windowSize;

    NS_LOG_LOGIC("Update Window: Now=" << now.GetSeconds() << "s, Horizon=" << horizon.GetSeconds() << "s");

    m_nodeTimings->PruneIntervals(now);

    while (true)
    {
        if (!m_nextBufferedInterval.has_value())
        {
            bool success = ParseNextLine();
            if (!success) 
            {
                break; 
            }
        }

        if (m_nextBufferedInterval->data.simulatorStartTime <= horizon)
        {
            NS_LOG_LOGIC("Loading interval for Node " << m_nextBufferedInterval->nodeId 
                         << " starting at SimTime " << m_nextBufferedInterval->data.simulatorStartTime.GetSeconds() << "s");
            
            m_nodeTimings->AddInterval(m_nextBufferedInterval->nodeId, m_nextBufferedInterval->data);
            
            m_nextBufferedInterval.reset(); 
        }
        else
        {
            break;
        }
    }

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