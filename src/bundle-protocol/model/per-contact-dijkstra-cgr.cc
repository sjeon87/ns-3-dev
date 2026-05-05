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

#include "per-contact-dijkstra-cgr.h"

#include "bundle.h"

#include "ns3/log.h"
#include "ns3/uinteger.h"

#include <algorithm>
#include <limits>
#include <queue>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PerContactDijkstraCGR");
NS_OBJECT_ENSURE_REGISTERED(PerContactDijkstraCGR);

TypeId
PerContactDijkstraCGR::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::PerContactDijkstraCGR")
            .SetParent<BaseRoutingEngine>()
            .SetGroupName("BundleProtocol")
            .AddConstructor<PerContactDijkstraCGR>()
            .AddAttribute("GraphSize",
                          "Total number of nodes in simulation",
                          UintegerValue(0),
                          MakeUintegerAccessor(&PerContactDijkstraCGR::m_size),
                          MakeUintegerChecker<uint32_t>());
    return tid;
}

PerContactDijkstraCGR::PerContactDijkstraCGR()
    : m_size(0),
      m_isDirty(true)
{
    NS_LOG_FUNCTION(this);
}

PerContactDijkstraCGR::~PerContactDijkstraCGR()
{
    NS_LOG_FUNCTION(this);
}

void
PerContactDijkstraCGR::InitializeMap(const std::vector<std::string>& eidList)
{
    NS_LOG_FUNCTION(this);

    m_size = static_cast<uint32_t>(eidList.size());

    if (m_size == 0)
    {
        NS_LOG_WARN("InitializeMap called with empty EID list.");
        return;
    }

    m_indexToEid = eidList;
    m_eidToIndex.reserve(m_size);
    for (uint32_t i = 0; i < m_size; ++i)
    {
        m_eidToIndex[eidList[i]] = i;
    }

    m_adjList.assign(m_size, std::vector<ContactEdge>());
    for (auto& row : m_adjList)
    {
        row.reserve(8);
    }
    m_nextHopTable.assign(m_size * m_size, m_size);
    m_capacity.resize(m_size);
    m_parent.resize(m_size);

    m_isDirty = true;
}

void
PerContactDijkstraCGR::AddContact(const std::string& fromEID,
                                            const std::string& toEID,
                                            uint32_t dataRate)
{
    AddContact(fromEID, toEID, dataRate, 0);
}

void
PerContactDijkstraCGR::AddContact(const std::string& fromEID,
                                            const std::string& toEID,
                                            uint32_t dataRate,
                                            uint32_t totalVolume)
{
    NS_LOG_FUNCTION(this << fromEID << toEID << dataRate << totalVolume);

    auto srcIt = m_eidToIndex.find(fromEID);
    auto dstIt = m_eidToIndex.find(toEID);

    if (srcIt == m_eidToIndex.end() || dstIt == m_eidToIndex.end())
    {
        NS_LOG_ERROR("AddContact: unknown EID. from=" << fromEID << " to=" << toEID);
        return;
    }

    m_adjList[srcIt->second].push_back({dstIt->second, dataRate, 0, totalVolume});
    m_isDirty = true;
    NS_LOG_INFO("AddContact succeeded: " << fromEID << " -> " << toEID 
            << " idx " << srcIt->second << " -> " << dstIt->second);
}

void
PerContactDijkstraCGR::RemoveContact(const std::string& fromEID,
                                               const std::string& toEID)
{
    NS_LOG_FUNCTION(this << fromEID << toEID);

    auto srcIt = m_eidToIndex.find(fromEID);
    auto dstIt = m_eidToIndex.find(toEID);

    if (srcIt == m_eidToIndex.end() || dstIt == m_eidToIndex.end())
    {
        return;
    }

    uint32_t node1 = srcIt->second;
    uint32_t node2 = dstIt->second;

    auto& edges = m_adjList[node1];
    edges.erase(std::remove_if(edges.begin(),
                               edges.end(),
                               [node2](const ContactEdge& e) { return e.toNode == node2; }),
                edges.end());

    m_isDirty = true;
}

void
PerContactDijkstraCGR::ReserveVolume(const std::string& fromEID,
                                               const std::string& toEID,
                                               uint32_t bytes)
{
    NS_LOG_FUNCTION(this << fromEID << toEID << bytes);

    auto srcIt = m_eidToIndex.find(fromEID);
    auto dstIt = m_eidToIndex.find(toEID);

    if (srcIt == m_eidToIndex.end() || dstIt == m_eidToIndex.end())
    {
        NS_LOG_WARN("ReserveVolume: unknown EID.");
        return;
    }

    uint32_t node2 = dstIt->second;

    for (auto& edge : m_adjList[srcIt->second])
    {
        if (edge.toNode == node2)
        {
            if (edge.totalVolume > 0)
            {
                uint32_t available = (edge.totalVolume > edge.usedVolume)
                                         ? (edge.totalVolume - edge.usedVolume)
                                         : 0;
                uint32_t reserved = std::min(bytes, available);
                edge.usedVolume += reserved;

                if (reserved < bytes)
                {
                    NS_LOG_WARN("ReserveVolume: link "
                                << fromEID << " -> " << toEID << " only had " << available
                                << " bytes remaining; tried to reserve " << bytes << " bytes.");
                }
            }
            m_isDirty = true;
            return;
        }
    }

    NS_LOG_WARN("ReserveVolume: no edge found from " << fromEID << " to " << toEID);
}

void
PerContactDijkstraCGR::RecomputeRoutingTable()
{

    NS_LOG_INFO("Recomputing: m_size=" << m_size << " adjList[0].size()=" 
            << (m_size > 0 ? m_adjList[0].size() : 0));
            
    if (!m_isDirty || m_size == 0)
    {
        return;
    }

    NS_LOG_INFO("Topology or volume changed — recomputing all-pairs routing table for "
                << m_size << " nodes.");

    std::fill(m_nextHopTable.begin(), m_nextHopTable.end(), m_size);

    for (uint32_t s = 0; s < m_size; ++s)
    {
        std::fill(m_capacity.begin(), m_capacity.end(), 0.0);
        std::fill(m_parent.begin(), m_parent.end(), m_size);

        m_capacity[s] = std::numeric_limits<double>::infinity();

        std::priority_queue<std::pair<double, uint32_t>> pq;
        pq.emplace(m_capacity[s], s);

        while (!pq.empty())
        {
            auto [cap, u] = pq.top();
            pq.pop();

            if (cap < m_capacity[u])
            {
                continue;
            }

            for (const auto& edge : m_adjList[u])
            {
                uint32_t v = edge.toNode;
                double effectiveCapacity;
                if (edge.totalVolume > 0)
                {
                    effectiveCapacity =
                        static_cast<double>(edge.totalVolume > edge.usedVolume
                                                ? edge.totalVolume - edge.usedVolume
                                                : 0);
                }
                else
                {
                    effectiveCapacity = static_cast<double>(edge.dataRate);
                }

                double pathCap = std::min(cap, effectiveCapacity);

                if (pathCap > m_capacity[v])
                {
                    m_capacity[v] = pathCap;
                    m_parent[v] = u;
                    pq.emplace(m_capacity[v], v);
                }
            }
        }

        for (uint32_t d = 0; d < m_size; ++d)
        {
            if (d == s || m_capacity[d] == 0.0)
            {
                continue;
            }

            uint32_t curr = d;
            while (m_parent[curr] != s && m_parent[curr] != m_size)
            {
                curr = m_parent[curr];
            }

            if (m_parent[curr] == s)
            {
                m_nextHopTable[s * m_size + d] = curr;
            }
        }
    }

    m_isDirty = false;
}

std::string
PerContactDijkstraCGR::GetNextHop(Ptr<Bundle> bundle, const std::string& currEID)
{
    NS_LOG_FUNCTION(this << currEID);

    NS_LOG_INFO("GetNextHop from " << currEID << " to " << bundle->GetDestinationEID());
    for (uint32_t i = 0; i < m_size; ++i)
    {
        for (const auto& e : m_adjList[i])
        {
            NS_LOG_INFO("  edge: " << m_indexToEid[i] << " -> " 
                        << m_indexToEid[e.toNode] << " cap=" << e.dataRate);
        }
    }

    if (m_isDirty)
    {
        RecomputeRoutingTable();
    }

    auto srcIt = m_eidToIndex.find(currEID);
    auto dstIt = m_eidToIndex.find(bundle->GetDestinationEID());

    if (srcIt == m_eidToIndex.end() || dstIt == m_eidToIndex.end())
    {
        NS_LOG_ERROR("GetNextHop: unknown EID. src=" << currEID
                                                     << " dst=" << bundle->GetDestinationEID());
        return "";
    }

    uint32_t s = srcIt->second;
    uint32_t d = dstIt->second;

    if (s == d)
    {
        return bundle->GetDestinationEID();
    }

    uint32_t nextHop = m_nextHopTable[s * m_size + d];

    if (nextHop == m_size)
    {
        NS_LOG_WARN("No path from " << currEID << " to " << bundle->GetDestinationEID());
        return "";
    }

    return m_indexToEid[nextHop];
}

} // namespace ns3