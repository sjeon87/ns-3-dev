/*
 * Copyright (c) 2008 INRIA
 *                  2013 University of New Brunswick
 *                  2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *           Dizhi Zhou <dizhi.zhou@gmail.com>
 *           Gerard Garcia <ggarcia@deic.uab.cat>
 *           Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "per-contact-dijkstra-cgr.h"

#include "bundle.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

#include <limits>
#include <queue>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PerContactDijkstraCGR");
NS_OBJECT_ENSURE_REGISTERED(PerContactDijkstraCGR);

struct PqItem
{
    uint32_t nodeIndex;
    Time arrivalTime;

    bool operator<(const PqItem& other) const
    {
        return arrivalTime > other.arrivalTime;
    }
};

TypeId
PerContactDijkstraCGR::GetTypeId()
{
    static TypeId tid = TypeId("ns3::PerContactDijkstraCGR")
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
      m_isDirty(true),
      m_nextTopologyChangeTime(Time::Max())
{
    NS_LOG_FUNCTION(this);
}

PerContactDijkstraCGR::~PerContactDijkstraCGR()
{
    NS_LOG_FUNCTION(this);
}

uint32_t
PerContactDijkstraCGR::FindIndex(const std::string& eid) const
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
PerContactDijkstraCGR::InitializeMap(const std::vector<std::string>& eidList)
{
    NS_LOG_FUNCTION(this);

    m_size = static_cast<uint32_t>(eidList.size());

    if (m_size == 0)
    {
        NS_LOG_WARN("InitializeMap called with empty EID list.");
        return;
    }

    m_eidList = eidList;
    m_adjList.assign(m_size, std::vector<ContactWindow>());

    m_nextHopTable.assign(m_size * m_size, m_size);

    m_isDirty = true;
}

void
PerContactDijkstraCGR::AddContact(const std::string& fromEID,
                                  const std::string& toEID,
                                  uint32_t dataRate)
{
    AddTimedContact(fromEID,
                    toEID,
                    Seconds(0),
                    Time::Max(),
                    dataRate,
                    Seconds(0),
                    std::numeric_limits<uint32_t>::max());
}

void
PerContactDijkstraCGR::AddTimedContact(const std::string& fromEID,
                                       const std::string& toEID,
                                       Time startTime,
                                       Time endTime,
                                       uint32_t dataRate,
                                       Time delay,
                                       uint32_t totalVolume)
{
    BaseRoutingEngine::AddTimedContact(fromEID,
                                       toEID,
                                       startTime,
                                       endTime,
                                       dataRate,
                                       delay,
                                       totalVolume);

    uint32_t node1 = FindIndex(fromEID);
    if (node1 == m_size)
    {
        return;
    }

    ContactWindow cw;
    cw.fromEID = fromEID;
    cw.toEID = toEID;
    cw.startTime = startTime;
    cw.endTime = endTime;
    cw.dataRate = dataRate;
    cw.delay = delay;
    cw.totalVolume = totalVolume;
    cw.usedVolume = 0;

    m_adjList[node1].push_back(cw);
    m_isDirty = true;
}

void
PerContactDijkstraCGR::RemoveContact(const std::string& fromEID, const std::string& toEID)
{
    uint32_t node1 = FindIndex(fromEID);
    if (node1 == m_size)
    {
        return;
    }

    std::vector<ContactWindow> newEdges;
    for (uint32_t i = 0; i < m_adjList[node1].size(); i++)
    {
        if (m_adjList[node1][i].toEID != toEID)
        {
            newEdges.push_back(m_adjList[node1][i]);
        }
    }

    m_adjList[node1] = newEdges;
    m_isDirty = true;
}

void
PerContactDijkstraCGR::ReserveVolume(const std::string& fromEID,
                                     const std::string& toEID,
                                     uint32_t bytes)
{
    uint32_t node1 = FindIndex(fromEID);
    if (node1 == m_size)
    {
        return;
    }

    Time currentTime = Simulator::Now();

    for (uint32_t i = 0; i < m_adjList[node1].size(); i++)
    {
        ContactWindow& contact = m_adjList[node1][i];

        if (contact.toEID == toEID && contact.startTime <= currentTime &&
            currentTime <= contact.endTime)
        {
            uint32_t available = 0;
            if (contact.totalVolume > contact.usedVolume)
            {
                available = contact.totalVolume - contact.usedVolume;
            }

            uint32_t reserved = std::min(bytes, available);
            contact.usedVolume += reserved;
            return;
        }
    }
}

void
PerContactDijkstraCGR::RecomputeRoutingTable()
{
    if (m_size == 0)
    {
        return;
    }

    Time currentTime = Simulator::Now();

    m_nextTopologyChangeTime = Time::Max();

    for (uint32_t i = 0; i < m_size; i++)
    {
        for (const auto& contact : m_adjList[i])
        {
            if (contact.startTime > currentTime && contact.startTime < m_nextTopologyChangeTime)
            {
                m_nextTopologyChangeTime = contact.startTime;
            }
            if (contact.startTime <= currentTime && contact.endTime > currentTime &&
                contact.endTime < m_nextTopologyChangeTime)
            {
                m_nextTopologyChangeTime = contact.endTime;
            }
        }
    }

    NS_LOG_INFO("Recomputing all-pairs routing table. Cache valid from "
                << currentTime.GetSeconds() << "s to " << m_nextTopologyChangeTime.GetSeconds()
                << "s");

    std::fill(m_nextHopTable.begin(), m_nextHopTable.end(), m_size);

    uint32_t avgBundleSize = 1000;

    for (uint32_t s = 0; s < m_size; ++s)
    {
        std::vector<Time> arrivalTime(m_size, Time::Max());
        std::vector<uint32_t> parentNode(m_size, m_size);

        std::priority_queue<PqItem> pq;

        arrivalTime[s] = currentTime;

        PqItem startItem;
        startItem.nodeIndex = s;
        startItem.arrivalTime = currentTime;
        pq.push(startItem);

        while (!pq.empty())
        {
            PqItem current = pq.top();
            pq.pop();

            uint32_t u = current.nodeIndex;

            if (current.arrivalTime > arrivalTime[u])
            {
                continue;
            }

            for (uint32_t i = 0; i < m_adjList[u].size(); i++)
            {
                ContactWindow& contact = m_adjList[u][i];
                uint32_t v = FindIndex(contact.toEID);
                if (v == m_size)
                {
                    continue;
                }

                if (contact.endTime <= arrivalTime[u])
                {
                    continue;
                }

                if (contact.usedVolume + avgBundleSize > contact.totalVolume)
                {
                    continue;
                }

                Time waitTime = Seconds(0);
                if (contact.startTime > arrivalTime[u])
                {
                    waitTime = contact.startTime - arrivalTime[u];
                }

                double txSeconds = (double)(avgBundleSize * 8) / contact.dataRate;
                Time txTime = Seconds(txSeconds);

                if (arrivalTime[u] + waitTime + txTime > contact.endTime)
                {
                    continue;
                }

                Time arrTimeAtV = arrivalTime[u] + waitTime + txTime + contact.delay;

                if (arrTimeAtV < arrivalTime[v])
                {
                    arrivalTime[v] = arrTimeAtV;
                    parentNode[v] = u;

                    PqItem nextItem;
                    nextItem.nodeIndex = v;
                    nextItem.arrivalTime = arrTimeAtV;
                    pq.push(nextItem);
                }
            }
        }

        for (uint32_t d = 0; d < m_size; ++d)
        {
            if (d == s || arrivalTime[d] == Time::Max())
            {
                continue;
            }

            uint32_t curr = d;
            while (parentNode[curr] != s && parentNode[curr] != m_size)
            {
                curr = parentNode[curr];
            }

            if (parentNode[curr] == s)
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

    std::string destEID = bundle->GetDestinationEID();
    uint32_t s = FindIndex(currEID);
    uint32_t d = FindIndex(destEID);

    if (s == m_size || d == m_size)
    {
        return "";
    }
    if (s == d)
    {
        return destEID;
    }

    if (m_isDirty || Simulator::Now() >= m_nextTopologyChangeTime)
    {
        RecomputeRoutingTable();
    }

    uint32_t nextHop = m_nextHopTable[s * m_size + d];

    if (nextHop == m_size)
    {
        NS_LOG_WARN("No valid time-varying path found from " << currEID << " to " << destEID);
        return "";
    }

    return m_eidList[nextHop];
}

} // namespace ns3
