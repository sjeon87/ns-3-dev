/*
 * Copyright (c) 2008 INRIA
 * 2013 University of New Brunswick
 * 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Dizhi Zhou <dizhi.zhou@gmail.com>
 * Gerard Garcia <ggarcia@deic.uab.cat>
 * Ishaan Lagwankar <lagwanka@msu.edu>
 */
#include "per-packet-dijkstra-cgr.h"

#include "bundle.h"

#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/uinteger.h"

#include <algorithm>
#include <queue>
#include <string>
#include <vector>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PerPacketDijkstraCGR");
NS_OBJECT_ENSURE_REGISTERED(PerPacketDijkstraCGR);

TypeId
PerPacketDijkstraCGR::GetTypeId()
{
    static TypeId tid = TypeId("ns3::PerPacketDijkstraCGR")
                            .SetParent<BaseRoutingEngine>()
                            .SetGroupName("BundleProtocol")
                            .AddConstructor<PerPacketDijkstraCGR>()
                            .AddAttribute("GraphSize",
                                          "Total number of nodes in simulation",
                                          UintegerValue(0),
                                          MakeUintegerAccessor(&PerPacketDijkstraCGR::m_size),
                                          MakeUintegerChecker<uint32_t>());
    return tid;
}

PerPacketDijkstraCGR::PerPacketDijkstraCGR()
    : m_size(0)
{
    NS_LOG_FUNCTION(this);
}

PerPacketDijkstraCGR::~PerPacketDijkstraCGR()
{
    NS_LOG_FUNCTION(this);
}

uint32_t
PerPacketDijkstraCGR::FindIndex(const std::string& eid) const
{
    for (uint32_t i = 0; i < m_eidList.size(); i++)
    {
        if (m_eidList[i] == eid)
        {
            return i;
        }
    }
    return m_size;
}

void
PerPacketDijkstraCGR::InitializeMap(const std::vector<std::string>& eidList)
{
    NS_LOG_FUNCTION(this);

    m_size = eidList.size();

    if (m_size == 0)
    {
        NS_LOG_WARN("No size detected, exiting.");
        return;
    }

    m_eidList = eidList;
    m_adjList.assign(m_size, std::vector<ContactEdge>());
}

void
PerPacketDijkstraCGR::AddContact(const std::string& fromEID,
                         const std::string& toEID,
                         uint32_t dataRate)
{
    AddContact(fromEID, toEID, dataRate, 0);
}

void
PerPacketDijkstraCGR::AddContact(const std::string& fromEID,
                         const std::string& toEID,
                         uint32_t dataRate,
                         uint32_t totalVolume)
{
    NS_LOG_FUNCTION(this << fromEID << toEID << dataRate << totalVolume);

    uint32_t node1 = FindIndex(fromEID);
    uint32_t node2 = FindIndex(toEID);

    if (node1 == m_size || node2 == m_size)
    {
        NS_LOG_ERROR("AddContact failed: EIDs not found.");
        return;
    }

    if (node1 >= m_adjList.size() || node2 >= m_adjList.size())
    {
        return;
    }

    m_adjList[node1].push_back({node2, dataRate, 0, totalVolume});
}

void
PerPacketDijkstraCGR::RemoveContact(const std::string& fromEID, const std::string& toEID)
{
    NS_LOG_FUNCTION(this << fromEID << toEID);

    uint32_t node1 = FindIndex(fromEID);
    uint32_t node2 = FindIndex(toEID);

    if (node1 == m_size || node2 == m_size)
    {
        return;
    }

    if (node1 >= m_adjList.size() || node2 >= m_adjList.size())
    {
        return;
    }

    auto& edges = m_adjList[node1];
    edges.erase(std::remove_if(edges.begin(),
                               edges.end(),
                               [node2](const ContactEdge& e) { return e.toNode == node2; }),
                edges.end());
}

void
PerPacketDijkstraCGR::ReserveVolume(const std::string& fromEID,
                            const std::string& toEID,
                            uint32_t bytes)
{
    NS_LOG_FUNCTION(this << fromEID << toEID << bytes);

    uint32_t node1 = FindIndex(fromEID);
    uint32_t node2 = FindIndex(toEID);

    if (node1 == m_size || node2 == m_size)
    {
        NS_LOG_WARN("ReserveVolume: unknown EID.");
        return;
    }

    for (auto& edge : m_adjList[node1])
    {
        if (edge.toNode == node2)
        {
            uint32_t available = (edge.totalVolume > edge.usedVolume)
                                     ? (edge.totalVolume - edge.usedVolume)
                                     : 0;
            uint32_t reserved = std::min(bytes, available);
            edge.usedVolume += reserved;

            if (reserved < bytes)
            {
                NS_LOG_WARN("ReserveVolume: link " << fromEID << " -> " << toEID
                                                   << " only had " << available
                                                   << " bytes remaining; tried to reserve "
                                                   << bytes << " bytes.");
            }
            return;
        }
    }

    NS_LOG_WARN("ReserveVolume: no edge found from " << fromEID << " to " << toEID);
}

std::string
PerPacketDijkstraCGR::GetNextHop(Ptr<Bundle> bundle, const std::string& currEID)
{
    NS_LOG_FUNCTION(this << currEID);

    std::string destEID = bundle->GetDestinationEID();

    uint32_t startNode = FindIndex(currEID);
    uint32_t destNode = FindIndex(destEID);

    if (startNode == m_size || destNode == m_size)
    {
        NS_LOG_ERROR("GetNextHop failed: Unknown start or destination EID.");
        return "";
    }

    using PQueueItem = std::pair<double, uint32_t>;
    std::priority_queue<PQueueItem, std::vector<PQueueItem>, std::less<>> pq;

    std::vector<double> capacity(m_size, 0.0);
    std::vector<uint32_t> parent(m_size, m_size);

    capacity[startNode] = std::numeric_limits<double>::infinity();
    pq.emplace(capacity[startNode], startNode);

    while (!pq.empty())
    {
        double currentCap = pq.top().first;
        uint32_t u = pq.top().second;
        pq.pop();

        if (currentCap < capacity[u])
        {
            continue;
        }

        if (u == destNode)
        {
            break;
        }

        for (const auto& edge : m_adjList[u])
        {
            uint32_t v = edge.toNode;
            double effectiveCapacity;
            if (edge.totalVolume > 0)
            {
                effectiveCapacity = static_cast<double>(edge.totalVolume > edge.usedVolume
                                                            ? edge.totalVolume - edge.usedVolume
                                                            : 0);
            }
            else
            {
                effectiveCapacity = static_cast<double>(edge.dataRate);
            }

            double pathCapacity = std::min(capacity[u], effectiveCapacity);

            if (pathCapacity > capacity[v])
            {
                capacity[v] = pathCapacity;
                parent[v] = u;
                pq.emplace(capacity[v], v);
            }
        }
    }

    if (capacity[destNode] == 0.0)
    {
        NS_LOG_WARN("No path found to destination.");
        return "";
    }

    uint32_t currPathNode = destNode;
    while (parent[currPathNode] != startNode)
    {
        currPathNode = parent[currPathNode];
        if (currPathNode == m_size)
        {
            return "";
        }
    }

    return m_eidList[currPathNode];
}

} // namespace ns3