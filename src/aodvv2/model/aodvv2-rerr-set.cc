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

#include "aodvv2-rerr-set.h"

#include "ns3/log.h"

#include <algorithm>
#include <iomanip>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Aodvv2RouteErrorSet");

namespace aodvv2
{
template <typename T>
RouteErrorSet<T>::RouteErrorSet()
{
}

template <typename T>
bool
RouteErrorSet<T>::HasRerr(T unreachableAddr, T pktSource)
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
RouteErrorSet<T>::Add(T unreachableAddr, T pktSource, Time timeout)
{
    m_rerr.push_back(Rerr(timeout + Simulator::Now(), unreachableAddr, pktSource));
}

template <typename T>
Time
RouteErrorSet<T>::GetTimeout(T addr)
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

template <typename T>
void
RouteErrorSet<T>::Print(Ptr<OutputStreamWrapper> stream, Time::Unit unit /* = Time::S */) const
{
    std::vector<Rerr> table = m_rerr;
    std::ostream* os = stream->GetStream();
    // Copy the current ostream state
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);

    *os << "AODVv2 RERRs\n";
    *os << std::setw(16) << "Unreach Addr";
    *os << std::setw(16) << "PktSrc Addr";
    *os << std::setw(16) << "Timeout" << std::endl;
    for (auto i = table.begin(); i != table.end(); ++i)
    {
        std::ostringstream unreach;
        std::ostringstream pktSource;
        std::ostringstream timeout;
        unreach << i->m_unreachableAddr;
        pktSource << i->m_pktSource;
        timeout << std::setprecision(2) << (i->m_timeout - Simulator::Now()).As(unit);

        *os << std::setw(16) << unreach.str();
        *os << std::setw(16) << pktSource.str();
        *os << std::setw(16) << timeout.str() << std::endl;
    }
    *stream->GetStream() << "\n";
}

template class RouteErrorSet<Ipv4Address>;
template class RouteErrorSet<Ipv6Address>;

} // namespace aodvv2
} // namespace ns3
