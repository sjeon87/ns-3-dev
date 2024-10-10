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

#include "aodvv2-neighbor-set.h"

#include "ns3/log.h"

#include <algorithm>
#include <iomanip>

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
    AddNeighbor(addr, iface);
}

template <typename T>
void
NeighborSet<T>::Print(Ptr<OutputStreamWrapper> stream, Time::Unit unit /* = Time::S */) const
{
    std::vector<Neighbor> table = m_nb;
    std::ostream* os = stream->GetStream();
    // Copy the current ostream state
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);

    *os << "AODVv2 Neighbors\n";
    *os << std::setw(16) << "Address";
    *os << std::setw(16) << "State";
    *os << std::setw(16) << "Timeout" << std::endl;
    for (auto i = table.begin(); i != table.end(); ++i)
    {
        std::ostringstream dest;
        std::ostringstream timeout;
        dest << i->m_neighborAddress;
        timeout << std::setprecision(2) << (i->m_timeout - Simulator::Now()).As(unit);

        *os << std::setw(16) << dest.str();
        *os << std::setw(16);
        switch (i->m_state)
        {
        case BLACKLISTED: {
            *os << "BLACKLISTED";
            break;
        }
        case HEARD: {
            *os << "HEARD";
            break;
        }
        case CONFIRMED: {
            *os << "CONFIRMED";
            break;
        }
        }
        *os << std::setw(16) << timeout.str() << std::endl;
    }
    *stream->GetStream() << "\n";
}

template class NeighborSet<Ipv4Address>;
template class NeighborSet<Ipv6Address>;

} // namespace aodvv2
} // namespace ns3
