/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "bounded-skew-scheduler.h"

#include "double.h"
#include "log.h"
#include "simulator.h"

#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("BoundedSkewScheduler");
NS_OBJECT_ENSURE_REGISTERED(BoundedSkewScheduler);

TypeId
BoundedSkewScheduler::GetTypeId()
{
    static TypeId tid = TypeId("ns3::BoundedSkewScheduler")
                            .SetParent<StaticSkewScheduler>()
                            .SetGroupName("Core")
                            .AddConstructor<BoundedSkewScheduler>()
                            .AddAttribute("Epsilon",
                                          "Maximum allowed drift between a node's local time and "
                                          "Simulator::Now().",
                                          TimeValue(MilliSeconds(100.0)),
                                          MakeTimeAccessor(&BoundedSkewScheduler::m_epsilon),
                                          MakeTimeChecker());
    return tid;
}

BoundedSkewScheduler::BoundedSkewScheduler()
{
    NS_LOG_FUNCTION(this);
}

BoundedSkewScheduler::~BoundedSkewScheduler()
{
    NS_LOG_FUNCTION(this);
}

Ptr<EpochTable>
BoundedSkewScheduler::GetCurrentEpochTable()
{
    return StaticSkewScheduler::GetCurrentEpochTable();
}

void
BoundedSkewScheduler::ExtendEpochTable(uint32_t nodeId, Time targetNodeTime)
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

    double epsilonNs = m_epsilon.GetDouble();

    while (nodeMax < targetNodeTime)
    {
        double offsetNs = (nodeMax - simMax).GetDouble();

        double lo = m_minSkew;
        double hi = m_maxSkew;
        if (offsetNs >= epsilonNs)
        {
            hi = std::min(hi, 1.0);
        }
        if (offsetNs <= -epsilonNs)
        {
            lo = std::max(lo, 1.0);
        }

        double skew;
        if (lo > hi)
        {
            NS_LOG_WARN("BoundedSkewScheduler: skew range ["
                        << m_minSkew << ", " << m_maxSkew
                        << "] cannot honor Epsilon bound for node " << nodeId
                        << "; holding skew at 1.0");
            skew = 1.0;
        }
        else
        {
            skew = m_uv->GetValue(lo, hi);
        }

        Time epochDuration = m_updatePeriod;
        if (skew > 1.0)
        {
            double durationNs = (epsilonNs - offsetNs) / (skew - 1.0);
            epochDuration = std::min(epochDuration, Time::FromDouble(durationNs, Time::NS));
        }
        else if (skew < 1.0)
        {
            double durationNs = (-epsilonNs - offsetNs) / (skew - 1.0);
            epochDuration = std::min(epochDuration, Time::FromDouble(durationNs, Time::NS));
        }

        if (epochDuration <= Time(0))
        {
            epochDuration = TimeStep(1);
        }

        Time newSimStart = simMax;
        Time newSimEnd = newSimStart + epochDuration;

        Time newNodeStart = nodeMax;
        Time deltaNode = Time::FromDouble(epochDuration.GetDouble() * skew, Time::NS);
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

} // namespace ns3
