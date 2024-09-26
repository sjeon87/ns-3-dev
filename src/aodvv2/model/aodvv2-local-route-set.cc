/*
 * Copyright (c) 2024 University of Florence
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Based on
 *      NS-3 AODV model developed by Elena Buchatskaya and Pavel Boyko of IITP RAS
 *
 * Authors: Francesco Todino <francesco.todino@edu.unifi.it>
 *          Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "aodvv2-local-route-set.h"

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
                          uint32_t seqNo,
                          IpInterfaceAddress iface,
                          uint32_t hops,
                          T nextHop,
                          Time lastUsed,
                          Time maxIdleTime,
                          RouteStates state)
    : m_ackTimer(Timer::CANCEL_ON_DESTROY),
      m_seqNo(seqNo),
      m_nextHopIface(iface),
      m_lastUsed(lastUsed + Simulator::Now()),
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
    return m_state == ACTIVE;
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
    dest << m_ipRoute->GetDestination();
    gw << m_ipRoute->GetGateway();
    iface << m_nextHopIface.GetAddress();
    expire << std::setprecision(2) << (m_lastUsed - Simulator::Now()).As(unit);
    *os << std::setw(16) << dest.str();
    *os << std::setw(16) << gw.str();
    *os << std::setw(16) << iface.str();
    *os << std::setw(16);
    switch (m_state)
    {
    // TODO me: understand how to stream the IDLE state
    case ACTIVE: {
        *os << "UP";
        break;
    }
    case IDLE: {
        *os << "IDLE";
        break;
    }
    case INVALID: {
        *os << "DOWN";
        break;
    }
    case UNCONFIRMED: {
        *os << "UNCONFIRMED";
        break;
    }
    }

    *os << std::setw(16) << expire.str();
    *os << m_hops << std::endl;
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
LocalRouteSet<T>::LookupRoute(T id, LocalRoute<T>& rt)
{
    NS_LOG_FUNCTION(this << id);
    Purge();
    if (m_ipAddressEntry.empty())
    {
        NS_LOG_LOGIC("Route to " << id << " not found; m_ipAddressEntry is empty");
        return false;
    }
    auto i = m_ipAddressEntry.find(id);
    if (i == m_ipAddressEntry.end())
    {
        NS_LOG_LOGIC("Route to " << id << " not found");
        return false;
    }
    rt = i->second;
    NS_LOG_LOGIC("Route to " << id << " found");
    return true;
}

template <typename T>
bool
LocalRouteSet<T>::LookupValidRoute(T id, LocalRoute<T>& rt)
{
    NS_LOG_FUNCTION(this << id);
    if (!LookupRoute(id, rt))
    {
        NS_LOG_LOGIC("Route to " << id << " not found");
        return false;
    }
    NS_LOG_LOGIC("Route to " << id << " flag is "
                             << ((rt.GetState() == ACTIVE) ? "valid" : "not valid"));
    return (rt.GetState() == ACTIVE);
}

template <typename T>
bool
LocalRouteSet<T>::DeleteRoute(T dst)
{
    NS_LOG_FUNCTION(this << dst);
    Purge();
    if (m_ipAddressEntry.erase(dst) != 0)
    {
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
    auto result = m_ipAddressEntry.insert(std::make_pair(rt.GetDestination(), rt));
    return result.second;
}

template <typename T>
bool
LocalRouteSet<T>::Update(LocalRoute<T>& rt)
{
    NS_LOG_FUNCTION(this);
    auto i = m_ipAddressEntry.find(rt.GetDestination());
    if (i == m_ipAddressEntry.end())
    {
        NS_LOG_LOGIC("Route update to " << rt.GetDestination() << " fails; not found");
        return false;
    }
    rt.SetSeqNo(std::max(rt.GetSeqNo(), i->second.GetSeqNo()));
    i->second = rt;
    if (i->second.GetState() != UNCONFIRMED)
    {
        NS_LOG_LOGIC("Route update to " << rt.GetDestination() << " set RreqCnt to 0");
        i->second.SetRreqCnt(0);
    }
    return true;
}

template <typename T>
bool
LocalRouteSet<T>::SetEntryState(T id, RouteStates state)
{
    NS_LOG_FUNCTION(this);
    auto i = m_ipAddressEntry.find(id);
    if (i == m_ipAddressEntry.end())
    {
        NS_LOG_LOGIC("Route set entry state to " << id << " fails; not found");
        return false;
    }
    i->second.SetState(state);
    i->second.SetRreqCnt(0);
    NS_LOG_LOGIC("Route set entry state to " << id << ": new state is " << state);
    return true;
}

template <typename T>
void
LocalRouteSet<T>::GetListOfDestinationWithNextHop(T nextHop, std::map<T, uint32_t>& unreachable)
{
    NS_LOG_FUNCTION(this);
    Purge();
    unreachable.clear();
    for (auto i = m_ipAddressEntry.begin(); i != m_ipAddressEntry.end(); ++i)
    {
        if (i->second.GetNextHop() == nextHop)
        {
            NS_LOG_LOGIC("Unreachable insert " << i->first << " " << i->second.GetSeqNo());
            unreachable.insert(std::make_pair(i->first, i->second.GetSeqNo()));
        }
    }
}

template <typename T>
void
LocalRouteSet<T>::InvalidateRoutesWithDst(const std::map<T, uint32_t>& unreachable)
{
    NS_LOG_FUNCTION(this);
    Purge();
    for (auto i = m_ipAddressEntry.begin(); i != m_ipAddressEntry.end(); ++i)
    {
        for (auto j = unreachable.begin(); j != unreachable.end(); ++j)
        {
            if ((i->first == j->first) && (i->second.GetState() == ACTIVE))
            {
                NS_LOG_LOGIC("Invalidate route with destination address " << i->first);
                i->second.Invalidate(m_badLinkLifetime);
            }
        }
    }
}

template <typename T>
void
LocalRouteSet<T>::DeleteAllRoutesFromInterface(IpInterfaceAddress iface)
{
    NS_LOG_FUNCTION(this);
    if (m_ipAddressEntry.empty())
    {
        return;
    }
    for (auto i = m_ipAddressEntry.begin(); i != m_ipAddressEntry.end();)
    {
        if (i->second.GetInterface() == iface)
        {
            auto tmp = i;
            ++i;
            m_ipAddressEntry.erase(tmp);
        }
        else
        {
            ++i;
        }
    }
}

template <typename T>
void
LocalRouteSet<T>::Purge()
{
    NS_LOG_FUNCTION(this);
    if (m_ipAddressEntry.empty())
    {
        return;
    }
    for (auto i = m_ipAddressEntry.begin(); i != m_ipAddressEntry.end();)
    {
        if (i->second.GetLastSeqNumUpdate() + m_unconfirmedTime < Simulator::Now())
        {
            i->second.SetSeqNo(0);

            if (i->second.GetState() == UNCONFIRMED)
            {
                auto tmp = i;
                ++i;
                m_ipAddressEntry.erase(tmp);
            }
            else if (i->second.GetState() == ACTIVE)
            {
                NS_LOG_LOGIC("Invalidate route with destination address " << i->first);
                i->second.Invalidate(m_badLinkLifetime);
                ++i;
            }
            else
            {
                ++i;
            }
        }
        else
        {
            if (i->second.GetState() == IDLE &&
                i->second.GetLastSeqNumUpdate() < Simulator::Now() + i->second.GetMaxIdleTime())
            {
                i->second.SetState(INVALID);
            }
            ++i;
        }
    }
}

template <typename T>
void
LocalRouteSet<T>::Purge(std::map<T, LocalRoute<T>>& table) const
{
    NS_LOG_FUNCTION(this);
    if (table.empty())
    {
        return;
    }
    for (auto i = table.begin(); i != table.end();)
    {
        if (i->second.GetLastSeqNumUpdate() + m_unconfirmedTime < Simulator::Now())
        {
            if (i->second.GetState() == UNCONFIRMED)
            {
                auto tmp = i;
                ++i;
                table.erase(tmp);
            }
            else if (i->second.GetState() == ACTIVE)
            {
                NS_LOG_LOGIC("Invalidate route with destination address " << i->first);
                i->second.Invalidate(m_badLinkLifetime);
                ++i;
            }
            else
            {
                ++i;
            }
        }
        else
        {
            if (i->second.GetState() == IDLE &&
                i->second.GetLastSeqNumUpdate() < Simulator::Now() + i->second.GetMaxIdleTime())
            {
                i->second.SetState(INVALID);
            }
            ++i;
        }
    }
}

template <typename T>
bool
LocalRouteSet<T>::MarkLinkAsUnidirectional(T neighbor, Time blacklistTimeout)
{
    NS_LOG_FUNCTION(this << neighbor << blacklistTimeout.As(Time::S));
    auto i = m_ipAddressEntry.find(neighbor);
    if (i == m_ipAddressEntry.end())
    {
        NS_LOG_LOGIC("Mark link unidirectional to  " << neighbor << " fails; not found");
        return false;
    }
    i->second.SetState(INVALID);
    i->second.SetLastUsed(blacklistTimeout);
    i->second.SetRreqCnt(0);
    NS_LOG_LOGIC("Set link to " << neighbor << " to unidirectional");
    return true;
}

template <typename T>
void
LocalRouteSet<T>::Print(Ptr<OutputStreamWrapper> stream, Time::Unit unit /* = Time::S */) const
{
    std::map<T, LocalRoute<T>> table = m_ipAddressEntry;
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
    *os << std::setw(16) << "Flag";
    *os << std::setw(16) << "Expire";
    *os << "Hops" << std::endl;
    for (auto i = table.begin(); i != table.end(); ++i)
    {
        i->second.Print(stream, unit);
    }
    *stream->GetStream() << "\n";
}

template class LocalRouteSet<Ipv4Address>;
template class LocalRouteSet<Ipv6Address>;

} // namespace aodvv2
} // namespace ns3
