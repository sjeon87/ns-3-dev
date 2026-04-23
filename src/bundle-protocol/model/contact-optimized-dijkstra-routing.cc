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

uint32_t
ContactOptimizedDijkstraRouting::FindIndex(const std::string& eid) const
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
ContactOptimizedDijkstraRouting::InitializeMap(const std::vector<std::string>& eidList)
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
    m_nextHopTable.assign(m_size, std::vector<uint32_t>(m_size, m_size));
    m_isDirty = true;
}

void
ContactOptimizedDijkstraRouting::AddContact(const std::string& fromEID,
                                            const std::string& toEID,
                                            uint32_t dataRate)
{
    NS_LOG_FUNCTION(this << fromEID << toEID << dataRate);

    uint32_t node1 = FindIndex(fromEID);
    uint32_t node2 = FindIndex(toEID);

    if (node1 == m_size || node2 == m_size || node1 >= m_adjList.size() ||
        node2 >= m_adjList.size())
    {
        return;
    }

    m_adjList[node1].push_back({node2, dataRate});
    m_isDirty = true;
}

void
ContactOptimizedDijkstraRouting::RemoveContact(const std::string& fromEID, const std::string& toEID)
{
    NS_LOG_FUNCTION(this << fromEID << toEID);

    uint32_t node1 = FindIndex(fromEID);
    uint32_t node2 = FindIndex(toEID);

    if (node1 == m_size || node2 == m_size || node1 >= m_adjList.size() ||
        node2 >= m_adjList.size())
    {
        return;
    }

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

    NS_LOG_INFO("Topology changed. Recomputing routing table for " << m_size << " nodes.");

    m_nextHopTable.assign(m_size, std::vector<uint32_t>(m_size, m_size));

    for (uint32_t startNode = 0; startNode < m_size; ++startNode)
    {
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

            for (const auto& edge : m_adjList[u])
            {
                uint32_t v = edge.toNode;
                double pathCapacity = std::min(capacity[u], static_cast<double>(edge.dataRate));

                if (pathCapacity > capacity[v])
                {
                    capacity[v] = pathCapacity;
                    parent[v] = u;
                    pq.emplace(capacity[v], v);
                }
            }
        }

        for (uint32_t destNode = 0; destNode < m_size; ++destNode)
        {
            if (startNode == destNode || capacity[destNode] == 0.0)
            {
                continue;
            }

            uint32_t currPathNode = destNode;

            while (parent[currPathNode] != startNode && parent[currPathNode] != m_size)
            {
                currPathNode = parent[currPathNode];
            }

            if (parent[currPathNode] == startNode)
            {
                m_nextHopTable[startNode][destNode] = currPathNode;
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

    std::string destEID = bundle->GetDestinationEID();
    uint32_t startNode = FindIndex(currEID);
    uint32_t destNode = FindIndex(destEID);

    if (startNode == m_size || destNode == m_size)
    {
        NS_LOG_ERROR("GetNextHop failed: Unknown start or destination EID.");
        return "";
    }

    if (startNode == destNode)
    {
        return destEID;
    }

    uint32_t nextHopNode = m_nextHopTable[startNode][destNode];

    if (nextHopNode == m_size)
    {
        NS_LOG_WARN("No path found from " << currEID << " to " << destEID);
        return "";
    }

    return m_eidList[nextHopNode];
}

} // namespace ns3
