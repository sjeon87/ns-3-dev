/*
 * Copyright (c) 2009 IITP RAS
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
 *      NS-2 AODV model developed by the CMU/MONARCH group and optimized and
 *      tuned by Samir Das and Mahesh Marina, University of Cincinnati;
 *
 *      AODV-UU implementation by Erik Nordström of Uppsala University
 *      https://web.archive.org/web/20100527072022/http://core.it.uu.se/core/index.php/AODV-UU
 *
 * Authors: Elena Buchatskaia <borovkovaes@iitp.ru>
 *          Pavel Boyko <boyko@iitp.ru>
 */

#include "aodvv2-rtable.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

#include <algorithm>
#include <iomanip>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Aodvv2RoutingTable");

namespace aodvv2
{

/*
 The Routing Table
 */
template <typename T>
RoutingTableEntry<T>::RoutingTableEntry(Ptr<NetDevice> dev,
                                        T dst,
                                        bool vSeqNo,
                                        uint32_t seqNo,
                                        IpInterfaceAddress iface,
                                        uint16_t hops,
                                        T nextHop,
                                        Time lifetime)
    : m_ackTimer(Timer::CANCEL_ON_DESTROY),
      m_validSeqNo(vSeqNo),
      m_seqNo(seqNo),
      m_hops(hops),
      m_lifeTime(lifetime + Simulator::Now()),
      m_iface(iface),
      m_flag(CONFIRMED),
      m_reqCount(0),
      m_blackListState(false),
      m_blackListTimeout(Simulator::Now())
{
    m_ipRoute = Create<IpRoute>();
    m_ipRoute->SetDestination(dst);
    m_ipRoute->SetGateway(nextHop);
    if constexpr (std::is_same_v<T, Ipv4Address>)
    {
        m_ipRoute->SetSource(m_iface.GetLocal());
    }
    else if constexpr (std::is_same_v<T, Ipv6Address>)
    {
        m_ipRoute->SetSource(m_iface.GetAddress());
    }
    m_ipRoute->SetOutputDevice(dev);
}

template <typename T>
RoutingTableEntry<T>::~RoutingTableEntry()
{
}

template <typename T>
bool
RoutingTableEntry<T>::InsertPrecursor(T id)
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
RoutingTableEntry<T>::LookupPrecursor(T id)
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
RoutingTableEntry<T>::DeletePrecursor(T id)
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
RoutingTableEntry<T>::DeleteAllPrecursors()
{
    NS_LOG_FUNCTION(this);
    m_precursorList.clear();
}

template <typename T>
bool
RoutingTableEntry<T>::IsPrecursorListEmpty() const
{
    return m_precursorList.empty();
}

template <typename T>
void
RoutingTableEntry<T>::GetPrecursors(std::vector<T>& prec) const
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
void
RoutingTableEntry<T>::Invalidate(Time badLinkLifetime)
{
    NS_LOG_FUNCTION(this << badLinkLifetime.As(Time::S));
    if (m_flag == BLACKLISTED)
    {
        return;
    }
    m_flag = BLACKLISTED;
    m_reqCount = 0;
    m_lifeTime = badLinkLifetime + Simulator::Now();
}

template <typename T>
void
RoutingTableEntry<T>::Print(Ptr<OutputStreamWrapper> stream, Time::Unit unit /* = Time::S */) const
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
    if constexpr (std::is_same_v<T, Ipv4Address>)
    {
        iface << m_iface.GetLocal();
    }
    else if constexpr (std::is_same_v<T, Ipv6Address>)
    {
        iface << m_iface.GetAddress();
    }
    expire << std::setprecision(2) << (m_lifeTime - Simulator::Now()).As(unit);
    *os << std::setw(16) << dest.str();
    *os << std::setw(16) << gw.str();
    *os << std::setw(16) << iface.str();
    *os << std::setw(16);
    switch (m_flag)
    {
    case CONFIRMED: {
        *os << "UP";
        break;
    }
    case BLACKLISTED: {
        *os << "DOWN";
        break;
    }
    case HEARD: {
        *os << "HEARD";
        break;
    }
    }

    *os << std::setw(16) << expire.str();
    *os << m_hops << std::endl;
    // Restore the previous ostream state
    (*os).copyfmt(oldState);
}

template class RoutingTableEntry<Ipv4Address>;
template class RoutingTableEntry<Ipv6Address>;

/*
 The Routing Table
 */
template <typename T>
RoutingTable<T>::RoutingTable(Time t)
    : m_badLinkLifetime(t)
{
}

template <typename T>
bool
RoutingTable<T>::LookupRoute(T id, RoutingTableEntry<T>& rt)
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
RoutingTable<T>::LookupValidRoute(T id, RoutingTableEntry<T>& rt)
{
    NS_LOG_FUNCTION(this << id);
    if (!LookupRoute(id, rt))
    {
        NS_LOG_LOGIC("Route to " << id << " not found");
        return false;
    }
    NS_LOG_LOGIC("Route to " << id << " flag is "
                             << ((rt.GetFlag() == CONFIRMED) ? "valid" : "not valid"));
    return (rt.GetFlag() == CONFIRMED);
}

template <typename T>
bool
RoutingTable<T>::DeleteRoute(T dst)
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
RoutingTable<T>::AddRoute(RoutingTableEntry<T>& rt)
{
    NS_LOG_FUNCTION(this);
    Purge();
    if (rt.GetFlag() != HEARD)
    {
        rt.SetRreqCnt(0);
    }
    auto result = m_ipAddressEntry.insert(std::make_pair(rt.GetDestination(), rt));
    return result.second;
}

template <typename T>
bool
RoutingTable<T>::Update(RoutingTableEntry<T>& rt)
{
    NS_LOG_FUNCTION(this);
    auto i = m_ipAddressEntry.find(rt.GetDestination());
    if (i == m_ipAddressEntry.end())
    {
        NS_LOG_LOGIC("Route update to " << rt.GetDestination() << " fails; not found");
        return false;
    }
    i->second = rt;
    if (i->second.GetFlag() != HEARD)
    {
        NS_LOG_LOGIC("Route update to " << rt.GetDestination() << " set RreqCnt to 0");
        i->second.SetRreqCnt(0);
    }
    return true;
}

template <typename T>
bool
RoutingTable<T>::SetEntryState(T id, RouteFlags state)
{
    NS_LOG_FUNCTION(this);
    auto i = m_ipAddressEntry.find(id);
    if (i == m_ipAddressEntry.end())
    {
        NS_LOG_LOGIC("Route set entry state to " << id << " fails; not found");
        return false;
    }
    i->second.SetFlag(state);
    i->second.SetRreqCnt(0);
    NS_LOG_LOGIC("Route set entry state to " << id << ": new state is " << state);
    return true;
}

template <typename T>
void
RoutingTable<T>::GetListOfDestinationWithNextHop(T nextHop, std::map<T, uint32_t>& unreachable)
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
RoutingTable<T>::InvalidateRoutesWithDst(const std::map<T, uint32_t>& unreachable)
{
    NS_LOG_FUNCTION(this);
    Purge();
    for (auto i = m_ipAddressEntry.begin(); i != m_ipAddressEntry.end(); ++i)
    {
        for (auto j = unreachable.begin(); j != unreachable.end(); ++j)
        {
            if ((i->first == j->first) && (i->second.GetFlag() == CONFIRMED))
            {
                NS_LOG_LOGIC("Invalidate route with destination address " << i->first);
                i->second.Invalidate(m_badLinkLifetime);
            }
        }
    }
}

template <typename T>
void
RoutingTable<T>::DeleteAllRoutesFromInterface(IpInterfaceAddress iface)
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
RoutingTable<T>::Purge()
{
    NS_LOG_FUNCTION(this);
    if (m_ipAddressEntry.empty())
    {
        return;
    }
    for (auto i = m_ipAddressEntry.begin(); i != m_ipAddressEntry.end();)
    {
        if (i->second.GetLifeTime() < Seconds(0))
        {
            if (i->second.GetFlag() == BLACKLISTED)
            {
                auto tmp = i;
                ++i;
                m_ipAddressEntry.erase(tmp);
            }
            else if (i->second.GetFlag() == CONFIRMED)
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
            ++i;
        }
    }
}

template <typename T>
void
RoutingTable<T>::Purge(std::map<T, RoutingTableEntry<T>>& table) const
{
    NS_LOG_FUNCTION(this);
    if (table.empty())
    {
        return;
    }
    for (auto i = table.begin(); i != table.end();)
    {
        if (i->second.GetLifeTime() < Seconds(0))
        {
            if (i->second.GetFlag() == BLACKLISTED)
            {
                auto tmp = i;
                ++i;
                table.erase(tmp);
            }
            else if (i->second.GetFlag() == CONFIRMED)
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
            ++i;
        }
    }
}

template <typename T>
bool
RoutingTable<T>::MarkLinkAsUnidirectional(T neighbor, Time blacklistTimeout)
{
    NS_LOG_FUNCTION(this << neighbor << blacklistTimeout.As(Time::S));
    auto i = m_ipAddressEntry.find(neighbor);
    if (i == m_ipAddressEntry.end())
    {
        NS_LOG_LOGIC("Mark link unidirectional to  " << neighbor << " fails; not found");
        return false;
    }
    i->second.SetUnidirectional(true);
    i->second.SetBlacklistTimeout(blacklistTimeout);
    i->second.SetRreqCnt(0);
    NS_LOG_LOGIC("Set link to " << neighbor << " to unidirectional");
    return true;
}

template <typename T>
void
RoutingTable<T>::Print(Ptr<OutputStreamWrapper> stream, Time::Unit unit /* = Time::S */) const
{
    std::map<T, RoutingTableEntry<T>> table = m_ipAddressEntry;
    Purge(table);
    std::ostream* os = stream->GetStream();
    // Copy the current ostream state
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);
    *os << "\nAODV Routing table\n";
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

template class RoutingTable<Ipv4Address>;
template class RoutingTable<Ipv6Address>;

} // namespace aodvv2
} // namespace ns3
