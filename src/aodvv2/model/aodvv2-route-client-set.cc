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

#include "aodvv2-route-client-set.h"

#include "ns3/log.h"

#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Aodvv2RouterClientSet");

namespace aodvv2
{
template <typename T>
RouterClientSet<T>::RouterClientSet()
{
}

template <typename T>
bool
RouterClientSet<T>::HasClient(T ip)
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
RouterClientSet<T>::Add(T ip, uint32_t mask, uint32_t cost)
{
    m_rc.push_back(RouteClient(ip, mask, cost));
}

template <typename T>
uint32_t
RouterClientSet<T>::GetCost(T addr)
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

template class RouterClientSet<Ipv4Address>;
template class RouterClientSet<Ipv6Address>;

} // namespace aodvv2
} // namespace ns3
