/*
 * Copyright (c) 2024 University of Florence
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Francesco Todino <todinofrancesco97@gmail.com>
 *          Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#ifndef AODVV2_DPD_H
#define AODVV2_DPD_H

#include "aodvv2-id-cache.h"

#include "ns3/ipv4-address.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv6-address.h"
#include "ns3/ipv6-header.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"

namespace ns3
{
namespace aodvv2
{
/**
 * \ingroup aodvv2
 *
 * \brief Helper class used to remember already seen packets and detect duplicates.
 *
 * Currently duplicate detection is based on unique packet ID given by Packet::GetUid ()
 * This approach is known to be weak (ns3::Packet UID is an internal identifier and not intended for
 * logical uniqueness in models) and should be changed.
 */
template <typename T>
class DuplicatePacketDetection
    : public std::enable_if_t<std::is_same_v<Ipv4Header, T> || std::is_same_v<Ipv6Header, T>, T>
{
    /// Alias for determining whether the parent is Ipv4RoutingProtocol or Ipv6RoutingProtocol
    static constexpr bool IsIpv4 = std::is_same_v<Ipv4Header, T>;

    /// Alias for Ipv4 and Ipv6 classes
    using IdCacheAlias =
        typename std::conditional_t<IsIpv4, IdCache<Ipv4Address>, IdCache<Ipv6Address>>;

  public:
    /**
     * Constructor
     * \param lifetime the lifetime for added entries
     */
    DuplicatePacketDetection(Time lifetime)
        : m_idCache(lifetime)
    {
    }

    /**
     * Check if the packet is a duplicate. If not, save information about this packet.
     * \param p the packet to check
     * \param header the IP header to check
     * \param metricType the metric type
     * \returns true if duplicate
     */
    bool IsDuplicate(Ptr<const Packet> p, const T& header, const uint8_t metricType);
    /**
     * Set duplicate record lifetime
     * \param lifetime the lifetime for duplicate records
     */
    void SetLifetime(Time lifetime);
    /**
     * Get duplicate record lifetime
     * \returns the duplicate record lifetime
     */
    Time GetLifetime() const;

  private:
    /// Impl
    IdCacheAlias m_idCache;
};

} // namespace aodvv2
} // namespace ns3

#endif /* AODVV2_DPD_H */
