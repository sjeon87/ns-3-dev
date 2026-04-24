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

#include "contact-optimized-dijkstra-routing.h"

#include "bundle.h"

#include "ns3/log.h"
#include "ns3/uinteger.h"

#include <algorithm>
#include <limits>
#include <queue>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ContactOptimizedDijkstraRouting");
NS_OBJECT_ENSURE_REGISTERED(ContactOptimizedDijkstraRouting);

TypeId
ContactOptimizedDijkstraRouting::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ContactOptimizedDijkstraRouting")
            .SetParent<BaseRoutingEngine>()
            .SetGroupName("BundleProtocol")
            .AddConstructor<ContactOptimizedDijkstraRouting>()
            .AddAttribute("GraphSize",
                          "Total number of nodes in simulation",
                          UintegerValue(0),
                          MakeUintegerAccessor(&ContactOptimizedDijkstraRouting::m_size),
                          MakeUintegerChecker<uint32_t>());
    return tid;
}

ContactOptimizedDijkstraRouting::ContactOptimizedDijkstraRouting()
    : m_size(0),
      m_isDirty(true)
{
    NS_LOG_FUNCTION(this);
}

ContactOptimizedDijkstraRouting::~ContactOptimizedDijkstraRouting()
{
    NS_LOG_FUNCTION(this);
}

void
ContactOptimizedDijkstraRouting::InitializeMap(const std::vector<std::string>& eidList)
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
ContactOptimizedDijkstraRouting::AddContact(const std::string& fromEID,
                                            const std::string& toEID,
                                            uint32_t dataRate)
{
    NS_LOG_FUNCTION(this << fromEID << toEID << dataRate);

    auto srcIt = m_eidToIndex.find(fromEID);
    auto dstIt = m_eidToIndex.find(toEID);

    if (srcIt == m_eidToIndex.end() || dstIt == m_eidToIndex.end())
    {
        NS_LOG_ERROR("AddContact: unknown EID. from=" << fromEID << " to=" << toEID);
        return;
    }

    m_adjList[srcIt->second].push_back({dstIt->second, dataRate});
    m_isDirty = true;
}

void
ContactOptimizedDijkstraRouting::RemoveContact(const std::string& fromEID, const std::string& toEID)
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
ContactOptimizedDijkstraRouting::RecomputeRoutingTable()
{
    if (!m_isDirty || m_size == 0)
    {
        return;
    }

    NS_LOG_INFO("Topology changed — recomputing all-pairs routing table for " << m_size
                                                                              << " nodes.");

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
                double pathCap = std::min(cap, static_cast<double>(edge.dataRate));

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
ContactOptimizedDijkstraRouting::GetNextHop(Ptr<Bundle> bundle, const std::string& currEID)
{
    NS_LOG_FUNCTION(this << currEID);

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
