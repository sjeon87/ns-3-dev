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

#include "aodvv2-rerr-set.h"

#include "ns3/log.h"

#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Aodvv2RerrSet");

namespace aodvv2
{
template <typename T>
RerrSet<T>::RerrSet()
{
}

template <typename T>
bool
RerrSet<T>::HasRerr(T unreachableAddr, T pktSource)
{
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
void
RerrSet<T>::Add(T unreachableAddr, T pktSource, Time timeout)
{
    m_rerr.push_back(Rerr(timeout + Simulator::Now(), unreachableAddr, pktSource));
}

template <typename T>
Time
RerrSet<T>::GetTimeout(T addr)
{
    for (auto i = m_rerr.begin(); i != m_rerr.end(); ++i)
    {
        if (i->m_unreachableAddr == addr)
        {
            return (i->m_timeout - Simulator::Now());
        }
    }
    return Seconds(0);
}

template class RerrSet<Ipv4Address>;
template class RerrSet<Ipv6Address>;

} // namespace aodvv2
} // namespace ns3
