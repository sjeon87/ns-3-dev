/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "epoch-table.h"

#include "log.h"
#include "ptr.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("EpochTable");
NS_OBJECT_ENSURE_REGISTERED(EpochTable);

TypeId
EpochTable::GetTypeId()
{
    static TypeId tid = TypeId("ns3::EpochTable")
                            .SetParent<Object>()
                            .SetGroupName("Core")
                            .AddConstructor<EpochTable>();
    return tid;
}

EpochTable::EpochTable()
{
    NS_LOG_FUNCTION(this);
}

EpochTable::~EpochTable()
{
    NS_LOG_FUNCTION(this);
}

void
EpochTable::AddEpoch(uint32_t nodeId, const Epoch& epoch)
{
    NS_LOG_FUNCTION(this << nodeId << epoch.simulatorStartTime << epoch.simulatorEndTime
                         << epoch.nodeStartTime << epoch.nodeEndTime << epoch.skew);

    if (epoch.simulatorStartTime >= epoch.simulatorEndTime ||
        epoch.nodeStartTime >= epoch.nodeEndTime)
    {
        NS_LOG_ERROR("EpochTable: Epoch not well formed!");
        return;
    }

    auto& epochs = m_epochTable[nodeId];

    if (!epochs.empty())
    {
        const auto& lastEpoch = epochs.back();
        if (lastEpoch.simulatorEndTime != epoch.simulatorStartTime ||
            lastEpoch.nodeEndTime != epoch.nodeStartTime)
        {
            NS_LOG_ERROR("EpochTable: Epoch not contiguous!");
            return;
        }
    }

    Epoch toInsert = epoch;
    toInsert.nodeId = nodeId;
    toInsert.index = static_cast<uint32_t>(epochs.size());
    epochs.emplace_back(toInsert);
}

const EpochTable::Epoch&
EpochTable::LocalTimeBinarySearch(uint32_t nodeId, Time tLocal) const
{
    NS_LOG_FUNCTION(this << nodeId << tLocal);

    const auto& epochs = m_epochTable.at(nodeId);
    NS_ASSERT_MSG(!epochs.empty(), "EpochTable: No epochs for node " << nodeId);

    uint32_t lo = 0;
    uint32_t hi = static_cast<uint32_t>(epochs.size()) - 1;
    int64_t floorIdx = -1;

    while (lo <= hi)
    {
        uint32_t mid = lo + (hi - lo) / 2;
        const Epoch& J = epochs[mid];

        if (J.nodeStartTime <= tLocal && tLocal < J.nodeEndTime)
        {
            return J;
        }
        else if (tLocal < J.nodeStartTime)
        {
            if (mid == 0)
            {
                break;
            }
            hi = mid - 1;
        }
        else
        {
            floorIdx = static_cast<int64_t>(mid);
            lo = mid + 1;
        }
    }
    if (floorIdx < 0)
    {
        return epochs.front();
    }
    return epochs[static_cast<uint32_t>(floorIdx)];
}

const EpochTable::Epoch&
EpochTable::GlobalTimeBinarySearch(uint32_t nodeId, Time tSimulator) const
{
    NS_LOG_FUNCTION(this << nodeId << tSimulator);

    const auto& epochs = m_epochTable.at(nodeId);
    NS_ASSERT_MSG(!epochs.empty(), "EpochTable: No epochs for node " << nodeId);

    uint32_t lo = 0;
    uint32_t hi = static_cast<uint32_t>(epochs.size()) - 1;
    int64_t floorIdx = -1;

    while (lo <= hi)
    {
        uint32_t mid = lo + (hi - lo) / 2;
        const Epoch& J = epochs[mid];

        if (J.simulatorStartTime <= tSimulator && tSimulator < J.simulatorEndTime)
        {
            return J;
        }
        else if (tSimulator < J.simulatorStartTime)
        {
            if (mid == 0)
            {
                break;
            }
            hi = mid - 1;
        }
        else
        {
            floorIdx = static_cast<int64_t>(mid);
            lo = mid + 1;
        }
    }

    if (floorIdx < 0)
    {
        return epochs.front();
    }
    return epochs[static_cast<uint32_t>(floorIdx)];
}

void
EpochTable::InsertEpoch(uint32_t nodeId,
                        Time simStart,
                        Time simEnd,
                        Time nodeStart,
                        Time nodeEnd,
                        double skew)
{
    NS_LOG_FUNCTION(this << nodeId << simStart << simEnd << nodeStart << nodeEnd << skew);

    auto& epochs = m_epochTable[nodeId];

    if (!epochs.empty())
    {
        int64_t splitIdx = -1;
        for (size_t i = 0; i < epochs.size(); ++i)
        {
            if (epochs[i].simulatorStartTime <= simStart && simStart <= epochs[i].simulatorEndTime)
            {
                splitIdx = static_cast<int64_t>(i);
                break;
            }
        }

        if (splitIdx != -1)
        {
            if (epochs[splitIdx].simulatorStartTime == simStart)
            {
                epochs.erase(epochs.begin() + splitIdx, epochs.end());
            }
            else
            {
                epochs[splitIdx].simulatorEndTime = simStart;
                epochs[splitIdx].nodeEndTime = nodeStart;
                epochs.erase(epochs.begin() + splitIdx + 1, epochs.end());
            }
        }
    }

    Epoch newEpoch;
    newEpoch.simulatorStartTime = simStart;
    newEpoch.simulatorEndTime = simEnd;
    newEpoch.nodeStartTime = nodeStart;
    newEpoch.nodeEndTime = nodeEnd;
    newEpoch.skew = skew;
    newEpoch.index = static_cast<uint32_t>(epochs.size());

    epochs.push_back(newEpoch);
}

bool
EpochTable::HasNode(uint32_t nodeId) const
{
    NS_LOG_FUNCTION(this << nodeId);
    return m_epochTable.find(nodeId) != m_epochTable.end();
}

Time
EpochTable::GetSimulatorTimeFromNodeTime(uint32_t nodeId, Time nodeTime) const
{
    NS_LOG_FUNCTION(this << nodeId << nodeTime);

    if (!HasNode(nodeId) || m_epochTable.at(nodeId).empty())
    {
        return nodeTime;
    }

    const Epoch& J = LocalTimeBinarySearch(nodeId, nodeTime);
    double deltaNs = (nodeTime - J.nodeStartTime).GetDouble();
    return J.simulatorStartTime + Time::FromDouble(deltaNs / J.skew, Time::NS);
}

Time
EpochTable::GetNodeTimeFromSimulatorTime(uint32_t nodeId, Time simulatorTime) const
{
    NS_LOG_FUNCTION(this << nodeId << simulatorTime);

    if (!HasNode(nodeId) || m_epochTable.at(nodeId).empty())
    {
        return simulatorTime;
    }

    const Epoch& J = GlobalTimeBinarySearch(nodeId, simulatorTime);
    double deltaNs = (simulatorTime - J.simulatorStartTime).GetDouble();
    return J.nodeStartTime + Time::FromDouble(deltaNs * J.skew, Time::NS);
}

Time
EpochTable::GetMaxNodeTime(uint32_t nodeId) const
{
    NS_LOG_FUNCTION(this << nodeId);
    NS_ASSERT_MSG(HasNode(nodeId), "EpochTable: No epochs for node " << nodeId);
    return m_epochTable.at(nodeId).back().nodeEndTime;
}

Time
EpochTable::GetMaxSimulatorTime(uint32_t nodeId) const
{
    NS_LOG_FUNCTION(this << nodeId);
    NS_ASSERT_MSG(HasNode(nodeId), "EpochTable: No epochs for node " << nodeId);
    return m_epochTable.at(nodeId).back().simulatorEndTime;
}

void
EpochTable::PruneEpochTable(Time cutoff)
{
    NS_LOG_FUNCTION(this << cutoff);

    for (auto it = m_epochTable.begin(); it != m_epochTable.end();)
    {
        uint32_t nodeId = it->first;
        auto& epochs = it->second;

        const Epoch& J = GlobalTimeBinarySearch(nodeId, cutoff);
        uint32_t idx = J.index;

        epochs.erase(epochs.begin(), epochs.begin() + idx);

        for (uint32_t i = 0; i < epochs.size(); ++i)
        {
            epochs[i].index = i;
        }

        if (epochs.empty())
        {
            it = m_epochTable.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

} // namespace ns3
