/*
 * Copyright (c) 2024 University of Florence
 *
 * SPDX-License-Identifier: GPL-2.0-only
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
RouteClientSet<T>::Add(T ip, uint32_t mask, uint32_t cost)
{
    m_rc.push_back(RouteClient(ip, mask, cost));
}

template <typename T>
uint32_t
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
