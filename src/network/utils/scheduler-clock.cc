/*
 * Copyright (c) 2026 Ishaan Lagwankar
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */
#include "scheduler-clock.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SchedulerClock");
NS_OBJECT_ENSURE_REGISTERED(SchedulerClock);

TypeId
SchedulerClock::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SchedulerClock")
                            .SetParent<LocalClock>()
                            .SetGroupName("Network")
                            .AddConstructor<SchedulerClock>();
    return tid;
}

SchedulerClock::SchedulerClock()
    : m_nodeId(0)
{
    NS_LOG_FUNCTION(this);
}

SchedulerClock::~SchedulerClock()
{
    NS_LOG_FUNCTION(this);
}

void
SchedulerClock::SetNodeId(uint32_t nodeId)
{
    NS_LOG_FUNCTION(this << nodeId);
    m_nodeId = nodeId;
}

void
SchedulerClock::SetEpochTable(Ptr<EpochTable> epochTable)
{
    NS_LOG_FUNCTION(this << epochTable);
    m_epochTable = epochTable;
}

Time
SchedulerClock::Now()
{
    NS_LOG_FUNCTION(this);
    if (m_epochTable)
    {
        return m_epochTable->GetNodeTimeFromSimulatorTime(m_nodeId, Simulator::Now());
    }

    return Simulator::Now();
}

} // namespace ns3
