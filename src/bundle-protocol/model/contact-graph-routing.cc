/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */
#include "contact-graph-routing.h"

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

NS_LOG_COMPONENT_DEFINE("ContactGraph");

TypeId
ContactGraph::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ContactGraph")
                            .SetParent<Object>()
                            .SetGroupName("BundleProtocol")
                            .AddConstructor<ContactGraph>()
                            .AddAttribute("GraphSize",
                                          "Total number of nodes in simulation",
                                          UintegerValue(0),
                                          MakeUintegerAccessor(&ContactGraph::m_size),
                                          MakeUintegerChecker<uint32_t>());
    return tid;
}

ContactGraph::ContactGraph()
    : m_size(0)
{
    NS_LOG_FUNCTION(this);
}

ContactGraph::~ContactGraph()
{
    NS_LOG_FUNCTION(this);
}

uint32_t
ContactGraph::FindIndex(const std::string& eid) const
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
ContactGraph::InitializeMap(const std::vector<std::string>& eidList)
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
ContactGraph::AddContact(const std::string& fromEID, const std::string& toEID, uint32_t dataRate)
{
    NS_LOG_FUNCTION(this << fromEID << toEID << dataRate);

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

    m_adjList[node1].push_back({node2, dataRate});
}

void
ContactGraph::RemoveContact(const std::string& fromEID, const std::string& toEID)
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
ContactGraph::AddTimedContact(const std::string& fromEID,
                              const std::string& toEID,
                              Time startTime,
                              Time endTime,
                              uint32_t dataRate,
                              Time delay) 
{
    NS_LOG_FUNCTION(this << fromEID << toEID << startTime << endTime << dataRate << delay);
    m_contactWindows.push_back({fromEID, toEID, startTime, endTime, dataRate, delay}); 
}

const std::vector<ContactWindow>&
ContactGraph::GetContactWindows() const
{
    return m_contactWindows;
}

std::string
ContactGraph::GetNextHop(Ptr<Bundle> bundle, const std::string& currEID)
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
            double pathCapacity = std::min(capacity[u], static_cast<double>(edge.dataRate));

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
