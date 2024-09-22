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

#include "aodvv2-neighbor-set.h"

#include "ns3/log.h"

#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Aodvv2NeighborSet");

namespace aodvv2
{
template <typename T>
NeighborSet<T>::NeighborSet(Time delay)
    : m_ntimer(Timer::CANCEL_ON_DESTROY)
{
    m_ntimer.SetDelay(delay);
    m_ntimer.SetFunction(&NeighborSet::Purge, this);
}

template <typename T>
bool
NeighborSet<T>::IsNeighbor(T addr)
{
    Purge();
    for (auto i = m_nb.begin(); i != m_nb.end(); ++i)
    {
        if (i->m_neighborAddress == addr)
        {
            return true;
        }
    }
    return false;
}

template <typename T>
Time
NeighborSet<T>::GetTimeout(T addr)
{
    Purge();
    for (auto i = m_nb.begin(); i != m_nb.end(); ++i)
    {
        if (i->m_neighborAddress == addr)
        {
            return (i->m_timeout - Simulator::Now());
        }
    }
    return Seconds(0);
}

template <typename T>
void
NeighborSet<T>::Update(T addr, IpInterfaceAddress iface, Time expire)
{
    for (auto i = m_nb.begin(); i != m_nb.end(); ++i)
    {
        if (i->m_neighborAddress == addr)
        {
            i->m_timeout = std::max(expire + Simulator::Now(), i->m_timeout);
            return;
        }
    }

    NS_LOG_LOGIC("Open link to " << addr);
    Neighbor neighbor(addr, iface, expire + Simulator::Now());
    m_nb.push_back(neighbor);
    Purge();
}

/**
 * \brief CloseNeighbor structure
 */
struct CloseNeighbor
{
    /**
     * Check if the entry is expired
     *
     * \param nb NeighborSet::Neighbor entry
     * \return true if expired, false otherwise
     */
    bool operator()(const NeighborSet<Ipv4Address>::Neighbor& nb) const
    {
        return nb.m_timeout < Simulator::Now();
    }

    /**
     * Check if the entry is expired
     *
     * \param nb NeighborSet::Neighbor entry
     * \return true if expired, false otherwise
     */
    bool operator()(const NeighborSet<Ipv6Address>::Neighbor& nb) const
    {
        return nb.m_timeout < Simulator::Now();
    }
};

template <typename T>
void
NeighborSet<T>::Purge()
{
    if (m_nb.empty())
    {
        return;
    }

    CloseNeighbor pred;
    if (!m_handleLinkFailure.IsNull())
    {
        for (auto j = m_nb.begin(); j != m_nb.end(); ++j)
        {
            if (pred(*j))
            {
                NS_LOG_LOGIC("Close link to " << j->m_neighborAddress);
                m_handleLinkFailure(j->m_neighborAddress);
            }
        }
    }
    m_nb.erase(std::remove_if(m_nb.begin(), m_nb.end(), pred), m_nb.end());
    m_ntimer.Cancel();
    m_ntimer.Schedule();
}

template <typename T>
void
NeighborSet<T>::ScheduleTimer()
{
    m_ntimer.Cancel();
    m_ntimer.Schedule();
}

template <typename T>
void
NeighborSet<T>::AddArpCache(Ptr<ArpCache> a)
{
    m_arp.push_back(a);
}

template <typename T>
void
NeighborSet<T>::DelArpCache(Ptr<ArpCache> a)
{
    m_arp.erase(std::remove(m_arp.begin(), m_arp.end(), a), m_arp.end());
}

template class NeighborSet<Ipv4Address>;
template class NeighborSet<Ipv6Address>;

} // namespace aodvv2
} // namespace ns3
