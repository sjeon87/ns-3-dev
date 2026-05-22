/*
 * Copyright (c) 2025 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "multi-rate-clock.h"

#include "node-level-scheduler.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MultiRateClock");
NS_OBJECT_ENSURE_REGISTERED(MultiRateClock);

TypeId
MultiRateClock::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MultiRateClock")
                            .SetParent<LocalClock>()
                            .SetGroupName("Network")
                            .AddConstructor<MultiRateClock>();
    return tid;
}

MultiRateClock::MultiRateClock()
    : m_nodeId(0)
{
    NS_LOG_FUNCTION(this);
}

MultiRateClock::~MultiRateClock()
{
    NS_LOG_FUNCTION(this);
}

void
MultiRateClock::SetNodeId(uint32_t nodeId)
{
    m_nodeId = nodeId;
}

Time
MultiRateClock::Now()
{
    // Use the static accessor to get the current graph linked to the active scheduler
    Ptr<NodeTimingGraph> graph = NodeLevelScheduler::GetCurrentGraph();

    if (graph)
    {
        return graph->GetNodeTimeFromSimulatorTime(m_nodeId, Simulator::Now());
    }

    return Simulator::Now();
}

} // namespace ns3
