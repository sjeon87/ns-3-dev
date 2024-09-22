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

#include "aodvv2-route-client-set.h"

#include "ns3/log.h"

#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Aodvv2RouteClientSet");

namespace aodvv2
{
template <typename T>
RouteClientSet<T>::RouteClientSet()
{
}

template <typename T>
bool
RouteClientSet<T>::HasClient(T ip)
{
    for (auto i = m_rc.begin(); i != m_rc.end(); ++i)
    {
        if (i->m_ip == ip)
        {
            return true;
        }
    }
    return false;
}

template <typename T>
void
RouteClientSet<T>::Add(T ip, uint16_t mask, uint16_t cost)
{
    m_rc.push_back(RouteClient(ip, mask, cost));
}

template <typename T>
uint16_t
RouteClientSet<T>::GetCost(T addr)
{
    for (auto i = m_rc.begin(); i != m_rc.end(); ++i)
    {
        if (i->m_ip == addr)
        {
            return i->m_cost;
        }
    }
    return 0;
}

template class RouteClientSet<Ipv4Address>;
template class RouteClientSet<Ipv6Address>;

} // namespace aodvv2
} // namespace ns3
