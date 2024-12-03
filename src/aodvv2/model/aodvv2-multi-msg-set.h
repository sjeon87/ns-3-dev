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

#ifndef AODVV2_MULTI_MSG_SET_H
#define AODVV2_MULTI_MSG_SET_H

#include "aodvv2-metric.h"

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
 * @ingroup aodvv2
 *
 * @brief Multicast message set used by AODVv2 protocol to avoid duplicate RREQ messages
 */
template <typename T>
class MultiMsgSet
    : public std::enable_if_t<std::is_same_v<Ipv4Address, T> || std::is_same_v<Ipv6Address, T>, T>
{
    /// Alias for determining whether the parent is Ipv4Address or Ipv6Address
    static constexpr bool IsIpv4 = std::is_same_v<Ipv4Address, T>;
    /// Alias for Ipv4InterfaceAddress and Ipv6InterfaceAddress classes
    using IpInterfaceAddress =
        typename std::conditional_t<IsIpv4, Ipv4InterfaceAddress, Ipv6InterfaceAddress>;
    /// Alias for Ipv4Address and Ipv6Address classes
    using IpAddress = typename std::conditional_t<IsIpv4, Ipv4Address, Ipv6Address>;

  public:
    /**
     * constructor
     * @param lifetime the lifetime for added entries
     */
    MultiMsgSet(Time lifetime)
        : m_maxSeqnoLifetime(lifetime)
    {
    }

    /**
     * Check if that entry exists in cache.
     * Add entry, if it doesn't exist.
     * @param origIp the IP address
     * @param origMask the mask
     * @param targIp the target IP address
     * @param seqNoRtr the sequence number of the router
     * @param origSeqNum the sequence number
     * @param interface the interface
     * @param metric the metric object
     * @param metricValue the metric value
     * @returns true if the pair exists
     */
    bool IsDuplicate(T origIp,
                     uint16_t origMask,
                     T targIp,
                     T seqNoRtr,
                     uint16_t origSeqNum,
                     IpInterfaceAddress interface,
                     Metric<IpAddress> metric,
                     uint8_t* metricValue);
    /// Remove all expired entries
    void Purge();
    /**
     * @returns number of entries in cache
     */
    uint32_t GetSize();

    /**
     * Set lifetime for future added entries.
     * @param lifetime the lifetime for entries
     */
    void SetLifetime(Time lifetime)
    {
        m_maxSeqnoLifetime = lifetime;
    }

    /**
     * Return lifetime for existing entries in cache
     * @returns the lifetime
     */
    Time GetLifeTime() const
    {
        return m_maxSeqnoLifetime;
    }

    /**
     * Print multicast msg
     * @param stream the output stream
     * @param unit The time unit to use (default Time::S)
     */
    void Print(Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const;

  private:
    /// Multicast message entry
    struct MultiMsgEntry
    {
        /// Origin Prefix or Sender IP
        T m_origIp;
        /// Origin Prefix Length or Sender Mask
        uint16_t m_origMask;
        /// Target Prefix or Receiver IP
        T m_targIp;
        /// ip address of the originator router
        T m_seqNoRtr;
        /// Origin Sequence Number
        uint16_t m_origSeqNum;
        /// Interface on which the RREQ was received
        IpInterfaceAddress m_interface;
        /// Origin Metric object
        Metric<IpAddress> m_metric;
        /// Origin Metric Value
        uint8_t* m_metricValue;
        /// Timestamp when record was updated
        Time m_timestamp;
        /// When record will expire
        Time m_removalTime;
    };

    /**
     * @brief IsExpired structure
     */
    struct IsExpired
    {
        /**
         * @brief Check if the entry is expired
         *
         * @param m Multicast message entry
         * @return true if expired, false otherwise
         */
        bool operator()(const MultiMsgEntry& m) const
        {
            return (m.m_removalTime < Simulator::Now());
        }
    };

    /// Already seen messages
    std::vector<MultiMsgEntry> m_msgCache;
    /// Default lifetime for message records
    Time m_maxSeqnoLifetime;
};

} // namespace aodvv2
} // namespace ns3

#endif /* AODVV2_MULTI_MSG_SET_H */
