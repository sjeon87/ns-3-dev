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

#ifndef AODVV2_ID_CACHE_H
#define AODVV2_ID_CACHE_H

#include "ns3/internet-module.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/simulator.h"

#include <vector>

namespace ns3
{
namespace aodvv2
{
/**
 * \ingroup aodvv2
 *
 * \brief Unique packets identification cache used for simple duplicate detection.
 */
template <typename T>
class IdCache
    : public std::enable_if_t<std::is_same_v<Ipv4Address, T> || std::is_same_v<Ipv6Address, T>, T>
{
    /// Alias for determining whether the parent is Ipv4Address or Ipv6Address
    static constexpr bool IsIpv4 = std::is_same_v<Ipv4Address, T>;
    /// Alias for Ipv4InterfaceAddress and Ipv6InterfaceAddress classes
    using IpInterfaceAddress =
        typename std::conditional_t<IsIpv4, Ipv4InterfaceAddress, Ipv6InterfaceAddress>;

  public:
    /**
     * constructor
     * \param lifetime the lifetime for added entries
     */
    IdCache(Time lifetime)
        : m_lifetime(lifetime)
    {
    }

    /**
     * Check that entry (origIp, origMask, targIp, origMetric) exists in cache.
     * Add entry, if it doesn't exist.
     * \param origIp the IP address
     * \param origMask the mask
     * \param targIp the target IP address
     * \param origMetric the metric
     * \returns true if the pair exists
     */
    bool IsDuplicate(T origIp, uint32_t origMask, T targIp, uint32_t origMetric);
    /// Remove all expired entries
    void Purge();
    /**
     * \returns number of entries in cache
     */
    uint32_t GetSize();

    /**
     * Set lifetime for future added entries.
     * \param lifetime the lifetime for entries
     */
    void SetLifetime(Time lifetime)
    {
        m_lifetime = lifetime;
    }

    /**
     * Return lifetime for existing entries in cache
     * \returns the lifetime
     */
    Time GetLifeTime() const
    {
        return m_lifetime;
    }

  private:
    /// Unique packet ID
    struct UniqueId
    {
        /// Origin Prefix or Sender IP
        T m_origIp;
        /// Origin Prefix Length or Sender Mask
        uint32_t m_origMask;
        /// Target Prefix or Receiver IP
        T m_targIp;
        /// Origin Metric
        uint32_t m_origMetric;
        /// When record will expire
        Time m_removalTime;
    };

    /**
     * \brief IsExpired structure
     */
    struct IsExpired
    {
        /**
         * \brief Check if the entry is expired
         *
         * \param u UniqueId entry
         * \return true if expired, false otherwise
         */
        bool operator()(const UniqueId& u) const
        {
            return (u.m_removalTime < Simulator::Now());
        }
    };

    /// Already seen IDs
    std::vector<UniqueId> m_idCache;
    /// Default lifetime for ID records
    Time m_lifetime;
};

} // namespace aodvv2
} // namespace ns3

#endif /* AODVV2_ID_CACHE_H */
