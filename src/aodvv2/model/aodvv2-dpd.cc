/*
 * Copyright (c) 2024 University of Florence
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Francesco Todino <francesco.todino@edu.unifi.it>
 *          Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "aodvv2-dpd.h"

namespace ns3
{
namespace aodvv2
{

template <typename T>
bool
DuplicatePacketDetection<T>::IsDuplicate(Ptr<const Packet> p, const T& header)
{
    // TODO me: update if needed the mask
    // TODO me: update the metric
    return m_idCache.IsDuplicate(header.GetSource(), 32, header.GetDestination(), 1);
}

template <typename T>
void
DuplicatePacketDetection<T>::SetLifetime(Time lifetime)
{
    m_idCache.SetLifetime(lifetime);
}

template <typename T>
Time
DuplicatePacketDetection<T>::GetLifetime() const
{
    return m_idCache.GetLifeTime();
}

template class DuplicatePacketDetection<Ipv4Header>;
template class DuplicatePacketDetection<Ipv6Header>;

} // namespace aodvv2
} // namespace ns3
