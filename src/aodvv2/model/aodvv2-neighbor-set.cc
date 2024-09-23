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
NeighborSet<T>::NeighborSet(Time maxBlacklistTime)
    : m_maxBlacklistTime(maxBlacklistTime)
{
}

template <typename T>
bool
NeighborSet<T>::IsNeighbor(T addr)
{
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
NeighborStates
NeighborSet<T>::GetState(T addr)
{
    for (auto i = m_nb.begin(); i != m_nb.end(); ++i)
    {
        if (i->m_neighborAddress == addr)
        {
            return i->m_state;
        }
    }
    return BLACKLISTED;
}

template <typename T>
void
NeighborSet<T>::AddNeighbor(T addr, IpInterfaceAddress iface)
{
    for (auto i = m_nb.begin(); i != m_nb.end(); ++i)
    {
        if (i->m_neighborAddress == addr)
        {
            return;
        }
    }

    NS_LOG_LOGIC("Open link to " << addr);
    Neighbor neighbor(addr, iface);
    m_nb.push_back(neighbor);
}

template <typename T>
void
NeighborSet<T>::UpdateState(T addr, IpInterfaceAddress iface, Time timeout)
{
    for (auto i = m_nb.begin(); i != m_nb.end(); ++i)
    {
        if (i->m_neighborAddress == addr)
        {
            switch (i->m_state)
            {
            case CONFIRMED:
                break;
            case HEARD:
                if (Simulator::Now() < i->m_timeout + timeout)
                {
                    i->m_state = CONFIRMED;
                    i->m_timeout = Simulator::Now() + INFINITY_TIME;
                }
                else
                {
                    i->m_state = BLACKLISTED;
                    i->m_timeout = Simulator::Now() + m_maxBlacklistTime;
                }
                break;
            case BLACKLISTED:
                if (Simulator::Now() > i->m_timeout)
                {
                    i->m_state = HEARD;
                    i->m_timeout = Simulator::Now() + INFINITY_TIME;
                }
                break;
            }
            return;
        }
    }
}

template class NeighborSet<Ipv4Address>;
template class NeighborSet<Ipv6Address>;

} // namespace aodvv2
} // namespace ns3
