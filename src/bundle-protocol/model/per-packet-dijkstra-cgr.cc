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
#include "per-packet-dijkstra-cgr.h"

#include "bundle.h"

#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

#include <limits>
#include <queue>
#include <string>
#include <vector>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PerPacketDijkstraCGR");
NS_OBJECT_ENSURE_REGISTERED(PerPacketDijkstraCGR);

/**
 * @brief Priority queue item for Dijkstra's algorithm.
 */
struct PqItem
{
    uint32_t nodeIndex; ///< Index of the node in the multigraph.
    Time arrivalTime;   ///< The earliest calculated arrival time at this node.

    /**
     * @brief Less-than operator for priority queue ordering.
     *
     * @param other The other PqItem to compare against.
     * @return true if this item has a later arrival time than the other item.
     */
    bool operator<(const PqItem& other) const
    {
        return arrivalTime > other.arrivalTime;
    }
};

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
    m_adjList.assign(m_size, std::vector<ContactWindow>());
}

void
PerPacketDijkstraCGR::AddContact(const std::string& fromEID,
                                 const std::string& toEID,
                                 uint32_t dataRate)
{
    NS_LOG_FUNCTION(this << fromEID << toEID << dataRate);

    AddTimedContact(fromEID,
                    toEID,
                    Seconds(0),
                    Time::Max(),
                    dataRate,
                    Seconds(0),
                    std::numeric_limits<uint32_t>::max());
}

void
PerPacketDijkstraCGR::AddTimedContact(const std::string& fromEID,
                                      const std::string& toEID,
                                      Time startTime,
                                      Time endTime,
                                      uint32_t dataRate,
                                      Time delay,
                                      uint32_t totalVolume)
{
    NS_LOG_FUNCTION(this << fromEID << toEID << startTime.GetSeconds() << endTime.GetSeconds());

    BaseRoutingEngine::AddTimedContact(fromEID,
                                       toEID,
                                       startTime,
                                       endTime,
                                       dataRate,
                                       delay,
                                       totalVolume);

    uint32_t node1 = FindIndex(fromEID);
    uint32_t node2 = FindIndex(toEID);

    if (node1 == m_size || node2 == m_size)
    {
        NS_LOG_ERROR("AddTimedContact failed: EIDs not found.");
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
}

void
PerPacketDijkstraCGR::RemoveContact(const std::string& fromEID, const std::string& toEID)
{
    NS_LOG_FUNCTION(this << fromEID << toEID);

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
}

void
PerPacketDijkstraCGR::ReserveVolume(const std::string& fromEID,
                                    const std::string& toEID,
                                    uint32_t bytes)
{
    NS_LOG_FUNCTION(this << fromEID << toEID << bytes);

    uint32_t node1 = FindIndex(fromEID);
    if (node1 == m_size)
    {
        NS_LOG_WARN("ReserveVolume: unknown fromEID.");
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

            if (reserved < bytes)
            {
                NS_LOG_WARN("ReserveVolume: link "
                            << fromEID << " -> " << toEID << " only had " << available
                            << " bytes remaining in the current window; tried to reserve " << bytes
                            << " bytes.");
            }
            return;
        }
    }

    NS_LOG_WARN("ReserveVolume: No currently active contact window found from "
                << fromEID << " to " << toEID << " at time " << currentTime.GetSeconds() << "s");
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

    if (startNode == destNode)
    {
        return destEID;
    }

    uint32_t bundleSize = bundle->GetTotalSize();
    Time currentTime = Simulator::Now();

    std::vector<Time> arrivalTime(m_size, Time::Max());
    std::vector<uint32_t> parentNode(m_size, m_size);

    std::priority_queue<PqItem> pq;

    arrivalTime[startNode] = currentTime;

    PqItem startItem;
    startItem.nodeIndex = startNode;
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

        if (u == destNode)
        {
            break;
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
            if (contact.usedVolume + bundleSize > contact.totalVolume)
            {
                continue;
            }

            Time waitTime = Seconds(0);
            if (contact.startTime > arrivalTime[u])
            {
                waitTime = contact.startTime - arrivalTime[u];
            }

            double txSeconds = (double)(bundleSize * 8) / contact.dataRate;
            Time txTime = Seconds(txSeconds);

            Time arrTimeAtV = arrivalTime[u] + waitTime + txTime + contact.delay;

            if (arrivalTime[u] + waitTime + txTime > contact.endTime)
            {
                continue;
            }

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

    if (arrivalTime[destNode] == Time::Max())
    {
        NS_LOG_WARN("No time-valid path found to destination.");
        return "";
    }

    uint32_t currPathNode = destNode;
    while (parentNode[currPathNode] != startNode)
    {
        currPathNode = parentNode[currPathNode];
        if (currPathNode == m_size)
        {
            return "";
        }
    }

    return m_eidList[currPathNode];
}

} // namespace ns3
