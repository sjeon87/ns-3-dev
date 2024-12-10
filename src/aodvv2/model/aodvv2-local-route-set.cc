/*
 * Copyright (c) 2024 University of Florence
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Based on
 *      NS-3 AODV model developed by Elena Buchatskaya and Pavel Boyko of IITP RAS
 *
 * Authors: Francesco Todino <todinofrancesco97@gmail.com>
 *          Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "aodvv2-local-route-set.h"

#include "aodvv2-metric.h"
#include "aodvv2-packet.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

#include <algorithm>
#include <iomanip>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Aodvv2LocalRouteSet");

namespace aodvv2
{

/*
 The Local Route Set
 */
template <typename T>
LocalRoute<T>::LocalRoute(Ptr<NetDevice> dev,
                          T dst,
                          uint16_t seqNo,
                          IpInterfaceAddress iface,
                          uint32_t hops,
                          T nextHop,
                          Time lastUsed,
                          Time maxIdleTime,
                          Metric<T> metric,
                          uint8_t* metricValue,
                          RouteStates state)
    : m_ackTimer(Timer::CANCEL_ON_DESTROY),
      m_seqNo(seqNo),
      m_nextHopIface(iface),
      m_lastUsed(lastUsed + Simulator::Now()),
      m_metric(metric),
      m_metricValue(metricValue),
      m_metricSize(metric.GetMetricSize()),
      m_state(state),
      m_hops(hops),
      m_reqCount(0),
      m_maxIdleTime(maxIdleTime)
{
    m_ipRoute = Create<IpRoute>();
    m_ipRoute->SetDestination(dst);
    m_ipRoute->SetGateway(nextHop);
    m_ipRoute->SetSource(m_nextHopIface.GetAddress());
    m_ipRoute->SetOutputDevice(dev);
    m_prefixLength = 32; // TODO me: update if needed
}

template <typename T>
LocalRoute<T>::~LocalRoute()
{
}

template <typename T>
bool
LocalRoute<T>::InsertPrecursor(T id)
{
    NS_LOG_FUNCTION(this << id);
    if (!LookupPrecursor(id))
    {
        m_precursorList.push_back(id);
        return true;
    }
    else
    {
        return false;
    }
}

template <typename T>
bool
LocalRoute<T>::LookupPrecursor(T id)
{
    NS_LOG_FUNCTION(this << id);
    for (auto i = m_precursorList.begin(); i != m_precursorList.end(); ++i)
    {
        if (*i == id)
        {
            NS_LOG_LOGIC("Precursor " << id << " found");
            return true;
        }
    }
    NS_LOG_LOGIC("Precursor " << id << " not found");
    return false;
}

template <typename T>
bool
LocalRoute<T>::DeletePrecursor(T id)
{
    NS_LOG_FUNCTION(this << id);
    auto i = std::remove(m_precursorList.begin(), m_precursorList.end(), id);
    if (i == m_precursorList.end())
    {
        NS_LOG_LOGIC("Precursor " << id << " not found");
        return false;
    }
    else
    {
        NS_LOG_LOGIC("Precursor " << id << " found");
        m_precursorList.erase(i, m_precursorList.end());
    }
    return true;
}

template <typename T>
void
LocalRoute<T>::DeleteAllPrecursors()
{
    NS_LOG_FUNCTION(this);
    m_precursorList.clear();
}

template <typename T>
bool
LocalRoute<T>::IsPrecursorListEmpty() const
{
    return m_precursorList.empty();
}

template <typename T>
void
LocalRoute<T>::GetPrecursors(std::vector<T>& prec) const
{
    NS_LOG_FUNCTION(this);
    if (IsPrecursorListEmpty())
    {
        return;
    }
    for (auto i = m_precursorList.begin(); i != m_precursorList.end(); ++i)
    {
        bool result = true;
        for (auto j = prec.begin(); j != prec.end(); ++j)
        {
            if (*j == *i)
            {
                result = false;
                break;
            }
        }
        if (result)
        {
            prec.push_back(*i);
        }
    }
}

template <typename T>
bool
LocalRoute<T>::IsValid()
{
    NS_LOG_FUNCTION(this);
    if (m_state == IDLE)
    {
        if (m_lastUsed + m_maxIdleTime < Simulator::Now())
        {
            m_state = INVALID;
            return false;
        }
        return true;
    }
    return m_state == ACTIVE || m_state == UNCONFIRMED;
}

template <typename T>
void
LocalRoute<T>::Invalidate(Time badLinkLifetime)
{
    NS_LOG_FUNCTION(this << badLinkLifetime.As(Time::S));
    if (m_state == INVALID)
    {
        return;
    }
    m_state = INVALID;
    m_reqCount = 0;
    m_lastUsed = badLinkLifetime + Simulator::Now();
}

template <typename T>
void
LocalRoute<T>::Print(Ptr<OutputStreamWrapper> stream, Time::Unit unit /* = Time::S */) const
{
    std::ostream* os = stream->GetStream();
    // Copy the current ostream state
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);

    std::ostringstream dest;
    std::ostringstream gw;
    std::ostringstream iface;
    std::ostringstream expire;
    std::ostringstream metric;
    dest << m_ipRoute->GetDestination();
    gw << m_ipRoute->GetGateway();
    iface << m_nextHopIface.GetAddress();
    expire << std::setprecision(2) << (m_lastUsed - Simulator::Now()).As(unit);
    metric << static_cast<uint16_t>(m_metric.GetMetricType()) << ": ";
    for (uint8_t i = 0; i < m_metricSize; i++)
    {
        metric << static_cast<uint16_t>(m_metricValue[i]) << " ";
    }
    *os << std::setw(16) << dest.str();
    *os << std::setw(16) << gw.str();
    *os << std::setw(16) << iface.str();
    *os << std::setw(16);
    switch (m_state)
    {
    case ACTIVE: {
        *os << "ACTIVE";
        break;
    }
    case IDLE: {
        *os << "IDLE";
        break;
    }
    case INVALID: {
        *os << "INVALID";
        break;
    }
    case UNCONFIRMED: {
        *os << "UNCONFIRMED";
        break;
    }
    }

    *os << std::setw(16) << expire.str();
    *os << metric.str() << std::endl;
    // Restore the previous ostream state
    (*os).copyfmt(oldState);
}

template class LocalRoute<Ipv4Address>;
template class LocalRoute<Ipv6Address>;

/*
 The Local Route
 */
template <typename T>
LocalRouteSet<T>::LocalRouteSet(Time badlinkTime, Time unconfirmedTime)
    : m_badLinkLifetime(badlinkTime),
      m_unconfirmedTime(unconfirmedTime)
{
}

template <typename T>
bool
LocalRouteSet<T>::LookupRoute(T id, uint8_t metricType, LocalRoute<T>& route)
{
    NS_LOG_FUNCTION(this << id);
    Purge();
    for (auto& r : m_ipAddressEntry)
    {
        if (r.GetDestination() == id && r.GetMetricType() == metricType)
        {
            route = r;
            NS_LOG_LOGIC("Route to " << id << " found");
            return true;
        }
    }
    NS_LOG_LOGIC("Route to " << id << " not found");
    return false;
}

template <typename T>
bool
LocalRouteSet<T>::LookupRoutes(T dst, std::vector<LocalRoute<T>>& routes)
{
    NS_LOG_FUNCTION(this << dst);
    Purge();
    bool found = false;
    for (auto& route : m_ipAddressEntry)
    {
        if (route.GetDestination() == dst)
        {
            routes.push_back(route);
            found = true;
            NS_LOG_LOGIC("Route to " << dst << " found");
        }
    }
    if (!found)
    {
        NS_LOG_LOGIC("Route to " << dst << " not found");
    }
    return found;
}

template <typename T>
bool
LocalRouteSet<T>::LookupValidRoutes(T id, std::vector<LocalRoute<T>>& routes)
{
    NS_LOG_FUNCTION(this << id);
    std::vector<LocalRoute<T>> tmpRoutes;
    if (!LookupRoutes(id, tmpRoutes))
    {
        NS_LOG_LOGIC("Route to " << id << " not found");
        return false;
    }
    for (const auto& route : tmpRoutes)
    {
        if (route.GetNextHop().IsInitialized() &&
            (route.GetState() == ACTIVE || route.GetState() == UNCONFIRMED))
        {
            routes.push_back(route);
            NS_LOG_LOGIC("Route to " << id << " is valid");
        }
    }
    return !routes.empty();
}

template <typename T>
bool
LocalRouteSet<T>::LookupBestRoute(T id, LocalRoute<T>& route)
{
    NS_LOG_FUNCTION(this << id);
    std::vector<LocalRoute<T>> routes;
    if (!LookupValidRoutes(id, routes))
    {
        NS_LOG_LOGIC("Route to " << id << " not found");
        return false;
    }

    route =
        *std::max_element(routes.begin(),
                          routes.end(),
                          [](const LocalRoute<T>& a, const LocalRoute<T>& b) {
                              return a.GetMetric().LoopFree(b.GetMetricValue(), b.GetMetricValue());
                          });

    NS_LOG_LOGIC("Route to " << id << " found");
    return true;
}

template <typename T>
bool
LocalRouteSet<T>::DeleteRoute(T dst)
{
    NS_LOG_FUNCTION(this << dst);
    Purge();
    auto it =
        std::remove_if(m_ipAddressEntry.begin(),
                       m_ipAddressEntry.end(),
                       [dst](const LocalRoute<T>& route) { return route.GetDestination() == dst; });
    if (it != m_ipAddressEntry.end())
    {
        m_ipAddressEntry.erase(it, m_ipAddressEntry.end());
        NS_LOG_LOGIC("Route deletion to " << dst << " successful");
        return true;
    }
    NS_LOG_LOGIC("Route deletion to " << dst << " not successful");
    return false;
}

template <typename T>
bool
LocalRouteSet<T>::AddRoute(LocalRoute<T>& rt)
{
    NS_LOG_FUNCTION(this);
    Purge();
    if (rt.GetState() != UNCONFIRMED)
    {
        rt.SetRreqCnt(0);
    }
    m_ipAddressEntry.push_back(rt);
    return true;
}

template <typename T>
bool
LocalRouteSet<T>::Update(LocalRoute<T>& rt)
{
    NS_LOG_FUNCTION(this);
    for (auto& route : m_ipAddressEntry)
    {
        if (route.GetDestination() == rt.GetDestination() &&
            route.GetMetricType() == rt.GetMetricType())
        {
            route.SetSeqNo(std::max(rt.GetSeqNo(), route.GetSeqNo()));
            route = rt;
            if (route.GetState() != UNCONFIRMED)
            {
                NS_LOG_LOGIC("Route update to " << rt.GetDestination() << " set RreqCnt to 0");
                route.SetRreqCnt(0);
            }
            return true;
        }
    }
    NS_LOG_LOGIC("Route update to " << rt.GetDestination() << " fails; not found");
    return false;
}

template <typename T>
bool
LocalRouteSet<T>::SetEntryState(T id, RouteStates state)
{
    NS_LOG_FUNCTION(this);
    for (auto& route : m_ipAddressEntry)
    {
        if (route.GetDestination() == id)
        {
            route.SetState(state);
            route.SetRreqCnt(0);
            NS_LOG_LOGIC("Route set entry state to " << id << ": new state is " << state);
            return true;
        }
    }
    NS_LOG_LOGIC("Route set entry state to " << id << " fails; not found");
    return false;
}

template <typename T>
void
LocalRouteSet<T>::GetListOfDestinationWithNextHop(T nextHop,
                                                  std::map<T, UnreachableDst>& unreachable)
{
    NS_LOG_FUNCTION(this);
    Purge();
    unreachable.clear();
    for (const auto& route : m_ipAddressEntry)
    {
        if (route.GetNextHop() == nextHop)
        {
            NS_LOG_LOGIC("Unreachable insert " << route.GetDestination() << " " << route.GetSeqNo()
                                               << " " << route.GetMetricType());
            unreachable.insert(
                std::make_pair(route.GetDestination(),
                               UnreachableDst{route.GetSeqNo(), route.GetMetricType()}));
        }
    }
}

template <typename T>
void
LocalRouteSet<T>::ActivateRouteWithNextHop(T nextHop)
{
    NS_LOG_FUNCTION(this);
    Purge();
    for (auto& route : m_ipAddressEntry)
    {
        if (route.GetNextHop() == nextHop)
        {
            NS_LOG_LOGIC("Activate route with destination address " << route.GetDestination());
            route.SetState(ACTIVE);
        }
    }
}

template <typename T>
void
LocalRouteSet<T>::InvalidateRoutesWithDst(const std::map<T, UnreachableDst>& unreachable)
{
    NS_LOG_FUNCTION(this);
    Purge();
    for (auto& route : m_ipAddressEntry)
    {
        for (const auto& un : unreachable)
        {
            if ((route.GetDestination() == un.first) && (route.GetState() == ACTIVE))
            {
                NS_LOG_LOGIC("Invalidate route with destination address "
                             << route.GetDestination());
                route.Invalidate(m_badLinkLifetime);
            }
        }
    }
}

template <typename T>
void
LocalRouteSet<T>::DeleteAllRoutesFromInterface(IpInterfaceAddress iface)
{
    NS_LOG_FUNCTION(this);
    auto it = std::remove_if(
        m_ipAddressEntry.begin(),
        m_ipAddressEntry.end(),
        [iface](const LocalRoute<T>& route) { return route.GetInterface() == iface; });
    if (it != m_ipAddressEntry.end())
    {
        m_ipAddressEntry.erase(it, m_ipAddressEntry.end());
    }
}

template <typename T>
void
LocalRouteSet<T>::Purge()
{
    NS_LOG_FUNCTION(this);
    auto it = std::remove_if(
        m_ipAddressEntry.begin(),
        m_ipAddressEntry.end(),
        [this](LocalRoute<T>& route) {
            if (route.GetLastSeqNumUpdate() + m_unconfirmedTime < Simulator::Now())
            {
                route.SetSeqNo(0);
                if (route.GetState() == UNCONFIRMED)
                {
                    return true;
                }
                else if (route.GetState() == ACTIVE)
                {
                    NS_LOG_LOGIC("Invalidate route with destination address "
                                 << route.GetDestination());
                    route.Invalidate(m_badLinkLifetime);
                }
            }
            else if (route.GetState() == IDLE &&
                     route.GetLastSeqNumUpdate() < Simulator::Now() + route.GetMaxIdleTime())
            {
                route.SetState(INVALID);
            }
            return false;
        });
    m_ipAddressEntry.erase(it, m_ipAddressEntry.end());
}

template <typename T>
void
LocalRouteSet<T>::Purge(std::vector<LocalRoute<T>>& table) const
{
    NS_LOG_FUNCTION(this);
    auto it = std::remove_if(table.begin(), table.end(), [this](LocalRoute<T>& route) {
        if (route.GetLastSeqNumUpdate() + m_unconfirmedTime < Simulator::Now())
        {
            if (route.GetState() == UNCONFIRMED)
            {
                return true;
            }
            else if (route.GetState() == ACTIVE)
            {
                NS_LOG_LOGIC("Invalidate route with destination address "
                             << route.GetDestination());
                route.Invalidate(m_badLinkLifetime);
            }
        }
        else if (route.GetState() == IDLE &&
                 route.GetLastSeqNumUpdate() < Simulator::Now() + route.GetMaxIdleTime())
        {
            route.SetState(INVALID);
        }
        return false;
    });
    table.erase(it, table.end());
}

template <typename T>
bool
LocalRouteSet<T>::MarkLinkAsUnidirectional(T neighbor, Time blacklistTimeout)
{
    NS_LOG_FUNCTION(this << neighbor << blacklistTimeout.As(Time::S));
    for (auto& route : m_ipAddressEntry)
    {
        if (route.GetDestination() == neighbor)
        {
            route.SetState(INVALID);
            route.SetLastUsed(blacklistTimeout);
            route.SetRreqCnt(0);
            NS_LOG_LOGIC("Set link to " << neighbor << " to unidirectional");
            return true;
        }
    }
    NS_LOG_LOGIC("Mark link unidirectional to  " << neighbor << " fails; not found");
    return false;
}

template <typename T>
void
LocalRouteSet<T>::Print(Ptr<OutputStreamWrapper> stream, Time::Unit unit /* = Time::S */) const
{
    std::vector<LocalRoute<T>> table = m_ipAddressEntry;
    Purge(table);
    std::ostream* os = stream->GetStream();
    // Copy the current ostream state
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);
    *os << "\nAODVv2 Local Route\n";
    *os << std::setw(16) << "Destination";
    *os << std::setw(16) << "Gateway";
    *os << std::setw(16) << "Interface";
    *os << std::setw(16) << "State";
    *os << std::setw(16) << "Expire";
    *os << "Metrics" << std::endl;
    for (const auto& route : table)
    {
        route.Print(stream, unit);
    }
    *stream->GetStream() << "\n";
}

template class LocalRouteSet<Ipv4Address>;
template class LocalRouteSet<Ipv6Address>;

} // namespace aodvv2
} // namespace ns3
