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

#include "aodvv2-rerr-set.h"

#include "ns3/log.h"

#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Aodvv2RerrSet");

namespace aodvv2
{
template <typename T>
RerrSet<T>::RerrSet(Time delay)
    : m_ntimer(Timer::CANCEL_ON_DESTROY)
{
    m_ntimer.SetDelay(delay);
    m_ntimer.SetFunction(&RerrSet::Purge, this);
}

template <typename T>
bool
RerrSet<T>::HasRerr(T unreachableAddr, T pktSource)
{
    Purge();
    for (auto i = m_rerr.begin(); i != m_rerr.end(); ++i)
    {
        if (i->m_unreachableAddr == unreachableAddr && i->m_pktSource == pktSource)
        {
            return true;
        }
    }
    return false;
}

template <typename T>
Time
RerrSet<T>::GetTimeout(T addr)
{
    Purge();
    for (auto i = m_rerr.begin(); i != m_rerr.end(); ++i)
    {
        if (i->m_unreachableAddr == addr)
        {
            return (i->m_timeout - Simulator::Now());
        }
    }
    return Seconds(0);
}

template <typename T>
void
RerrSet<T>::Purge()
{
    if (m_rerr.empty())
    {
        return;
    }

    auto pred = [](const Rerr& rerr) { return rerr.m_timeout < Simulator::Now(); };

    m_rerr.erase(std::remove_if(m_rerr.begin(), m_rerr.end(), pred), m_rerr.end());

    m_ntimer.Cancel();
    m_ntimer.Schedule();
}

template <typename T>
void
RerrSet<T>::ScheduleTimer()
{
    m_ntimer.Cancel();
    m_ntimer.Schedule();
}

template class RerrSet<Ipv4Address>;
template class RerrSet<Ipv6Address>;

} // namespace aodvv2
} // namespace ns3
