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
#include "aodvv2-id-cache.h"

#include <algorithm>
#include <iomanip>

namespace ns3
{
namespace aodvv2
{
template <typename T>
bool
IdCache<T>::IsDuplicate(T origIp, uint32_t origMask, T targIp, uint32_t metricType)
{
    Purge();
    for (auto i = m_idCache.begin(); i != m_idCache.end(); ++i)
    {
        if (i->m_origIp == origIp && i->m_origMask == origMask && i->m_targIp == targIp &&
            i->m_metricType == metricType)
        {
            return true;
        }
    }
    UniqueId uniqueId = {origIp, origMask, targIp, metricType, m_lifetime + Simulator::Now()};
    m_idCache.push_back(uniqueId);
    return false;
}

template <typename T>
void
IdCache<T>::Purge()
{
    m_idCache.erase(remove_if(m_idCache.begin(), m_idCache.end(), IsExpired()), m_idCache.end());
}

template <typename T>
uint32_t
IdCache<T>::GetSize()
{
    Purge();
    return m_idCache.size();
}

template <typename T>
void
IdCache<T>::Print(Ptr<OutputStreamWrapper> stream, Time::Unit unit /* = Time::S */) const
{
    std::vector<UniqueId> table = m_idCache;
    std::ostream* os = stream->GetStream();
    // Copy the current ostream state
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);

    *os << "AODVv2 Multicast Msg\n";
    *os << std::setw(16) << "Orig Addr";
    *os << std::setw(16) << "Targ Addr";
    *os << std::setw(16) << "Orig Metric" << std::endl;
    for (auto i = table.begin(); i != table.end(); ++i)
    {
        std::ostringstream orig;
        std::ostringstream targ;
        orig << i->m_origIp << "/" << i->m_origMask;
        targ << i->m_targIp << "/" << i->m_origMask;

        *os << std::setw(16) << orig.str();
        *os << std::setw(16) << targ.str();
        *os << std::setw(16) << i->m_metricType << std::endl;
    }
    *stream->GetStream() << "\n";
}

template class IdCache<Ipv4Address>;
template class IdCache<Ipv6Address>;

} // namespace aodvv2
} // namespace ns3
