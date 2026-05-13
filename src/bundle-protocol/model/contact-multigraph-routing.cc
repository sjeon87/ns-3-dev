/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "contact-multigraph-routing.h"

#include "bundle.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

#include <limits>
#include <queue>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ContactMultigraphRouting");
NS_OBJECT_ENSURE_REGISTERED(ContactMultigraphRouting);

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
ContactMultigraphRouting::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ContactMultigraphRouting")
                            .SetParent<BaseRoutingEngine>()
                            .SetGroupName("BundleProtocol")
                            .AddConstructor<ContactMultigraphRouting>()
                            .AddAttribute("GraphSize",
                                          "Total number of nodes in simulation",
                                          UintegerValue(0),
                                          MakeUintegerAccessor(&ContactMultigraphRouting::m_size),
                                          MakeUintegerChecker<uint32_t>());
    return tid;
}

ContactMultigraphRouting::ContactMultigraphRouting()
    : m_size(0),
      m_isDirty(true),
      m_nextTopologyChangeTime(Time::Max())
{
}

ContactMultigraphRouting::~ContactMultigraphRouting()
{
}

uint32_t
ContactMultigraphRouting::FindIndex(const std::string& eid) const
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
ContactMultigraphRouting::InitializeMap(const std::vector<std::string>& eidList)
{
    m_size = static_cast<uint32_t>(eidList.size());
    if (m_size == 0)
    {
        return;
    }

    m_eidList = eidList;

    m_multigraph.assign(
        m_size,
        std::vector<std::vector<ContactWindow>>(m_size, std::vector<ContactWindow>()));
    m_nextHopTable.assign(m_size * m_size, m_size);
    m_isDirty = true;
}

void
ContactMultigraphRouting::AddContact(const std::string& fromEID,
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
ContactMultigraphRouting::AddTimedContact(const std::string& fromEID,
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

    uint32_t src = FindIndex(fromEID);
    uint32_t dst = FindIndex(toEID);
    if (src == m_size || dst == m_size)
    {
        return;
    }

    ContactWindow cw = {fromEID, toEID, startTime, endTime, dataRate, delay, totalVolume, 0};

    std::vector<ContactWindow>& edgeList = m_multigraph[src][dst];

    bool inserted = false;
    for (auto it = edgeList.begin(); it != edgeList.end(); ++it)
    {
        if (cw.startTime < it->startTime)
        {
            edgeList.insert(it, cw);
            inserted = true;
            break;
        }
    }
    if (!inserted)
    {
        edgeList.push_back(cw);
    }

    m_isDirty = true;
}

void
ContactMultigraphRouting::RemoveContact(const std::string& fromEID, const std::string& toEID)
{
    uint32_t src = FindIndex(fromEID);
    uint32_t dst = FindIndex(toEID);
    if (src == m_size || dst == m_size)
    {
        return;
    }

    m_multigraph[src][dst].clear();
    m_isDirty = true;
}

void
ContactMultigraphRouting::ReserveVolume(const std::string& fromEID,
                                        const std::string& toEID,
                                        uint32_t bytes)
{
    uint32_t src = FindIndex(fromEID);
    uint32_t dst = FindIndex(toEID);
    if (src == m_size || dst == m_size)
    {
        return;
    }

    Time currentTime = Simulator::Now();
    std::vector<ContactWindow>& edgeList = m_multigraph[src][dst];

    for (uint32_t i = 0; i < edgeList.size(); i++)
    {
        ContactWindow& contact = edgeList[i];

        if (contact.startTime <= currentTime && currentTime <= contact.endTime)
        {
            uint32_t available = (contact.totalVolume > contact.usedVolume)
                                     ? (contact.totalVolume - contact.usedVolume)
                                     : 0;
            uint32_t reserved = std::min(bytes, available);
            contact.usedVolume += reserved;
            return;
        }
    }
}

void
ContactMultigraphRouting::RecomputeRoutingTable()
{
    if (m_size == 0)
    {
        return;
    }

    Time currentTime = Simulator::Now();
    m_nextTopologyChangeTime = Time::Max();

    for (uint32_t u = 0; u < m_size; u++)
    {
        for (uint32_t v = 0; v < m_size; v++)
        {
            for (const auto& contact : m_multigraph[u][v])
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
    }

    std::fill(m_nextHopTable.begin(), m_nextHopTable.end(), m_size);
    uint32_t avgBundleSize = 1000;

    for (uint32_t s = 0; s < m_size; ++s)
    {
        std::vector<Time> arrivalTime(m_size, Time::Max());
        std::vector<uint32_t> parentNode(m_size, m_size);
        std::priority_queue<PqItem> pq;

        arrivalTime[s] = currentTime;
        pq.push({s, currentTime});

        while (!pq.empty())
        {
            PqItem current = pq.top();
            pq.pop();

            uint32_t u = current.nodeIndex;
            if (current.arrivalTime > arrivalTime[u])
            {
                continue;
            }

            for (uint32_t v = 0; v < m_size; ++v)
            {
                if (u == v || m_multigraph[u][v].empty())
                {
                    continue;
                }

                Time bestArrTimeAtV = Time::Max();

                for (const auto& contact : m_multigraph[u][v])
                {
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

                    Time txTime = Seconds((double)(avgBundleSize * 8) / contact.dataRate);
                    if (arrivalTime[u] + waitTime + txTime > contact.endTime)
                    {
                        continue;
                    }

                    Time arrTimeAtV = arrivalTime[u] + waitTime + txTime + contact.delay;
                    if (arrTimeAtV < bestArrTimeAtV)
                    {
                        bestArrTimeAtV = arrTimeAtV;
                    }
                }

                if (bestArrTimeAtV < arrivalTime[v])
                {
                    arrivalTime[v] = bestArrTimeAtV;
                    parentNode[v] = u;
                    pq.push({v, bestArrTimeAtV});
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
ContactMultigraphRouting::GetNextHop(Ptr<Bundle> bundle, const std::string& currEID)
{
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
        return "";
    }

    return m_eidList[nextHop];
}

} // namespace ns3
