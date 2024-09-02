/*
 * Copyright (c) 2009 IITP RAS
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
 *      NS-2 AODV model developed by the CMU/MONARCH group and optimized and
 *      tuned by Samir Das and Mahesh Marina, University of Cincinnati;
 *
 *      AODV-UU implementation by Erik Nordström of Uppsala University
 *      https://web.archive.org/web/20100527072022/http://core.it.uu.se/core/index.php/AODV-UU
 *
 * Authors: Elena Buchatskaia <borovkovaes@iitp.ru>
 *          Pavel Boyko <boyko@iitp.ru>
 */
#ifndef AODVV2_RQUEUE_H
#define AODVV2_RQUEUE_H

#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv6-routing-protocol.h"
#include "ns3/simulator.h"

#include <vector>

namespace ns3
{
namespace aodvv2
{

/**
 * \ingroup aodv
 * \brief AODV Queue Entry
 */
template <typename T>
class QueueEntry
    : public std::enable_if_t<std::is_same_v<Ipv4Header, T> || std::is_same_v<Ipv6Header, T>, T>
{
    /// Alias for determining whether the parent is Ipv4Header or Ipv6Header
    static constexpr bool IsIpv4 = std::is_same_v<Ipv4Header, T>;

    /// Alias for Ipv4 and Ipv6 classes
    using IpRoutingProtocol =
        typename std::conditional_t<IsIpv4, Ipv4RoutingProtocol, Ipv6RoutingProtocol>;

  public:
    /// IP routing unicast forward callback typedef
    typedef typename IpRoutingProtocol::UnicastForwardCallback UnicastForwardCallback;
    /// IP routing error callback typedef
    typedef typename IpRoutingProtocol::ErrorCallback ErrorCallback;

    /**
     * constructor
     *
     * \param pa the packet to add to the queue
     * \param h the IpHeader
     * \param ucb the UnicastForwardCallback function
     * \param ecb the ErrorCallback function
     * \param exp the expiration time
     */
    QueueEntry(Ptr<const Packet> pa = nullptr,
               const T& h = T(),
               UnicastForwardCallback ucb = UnicastForwardCallback(),
               ErrorCallback ecb = ErrorCallback(),
               Time exp = Simulator::Now())
        : m_packet(pa),
          m_header(h),
          m_ucb(ucb),
          m_ecb(ecb),
          m_expire(exp + Simulator::Now())
    {
    }

    /**
     * \brief Compare queue entries
     * \param o QueueEntry to compare
     * \return true if equal
     */
    bool operator==(const QueueEntry& o) const
    {
        return ((m_packet == o.m_packet) &&
                (m_header.GetDestination() == o.m_header.GetDestination()) &&
                (m_expire == o.m_expire));
    }

    // Fields
    /**
     * Get unicast forward callback
     * \returns unicast callback
     */
    UnicastForwardCallback GetUnicastForwardCallback() const
    {
        return m_ucb;
    }

    /**
     * Set unicast forward callback
     * \param ucb The unicast callback
     */
    void SetUnicastForwardCallback(UnicastForwardCallback ucb)
    {
        m_ucb = ucb;
    }

    /**
     * Get error callback
     * \returns the error callback
     */
    ErrorCallback GetErrorCallback() const
    {
        return m_ecb;
    }

    /**
     * Set error callback
     * \param ecb The error callback
     */
    void SetErrorCallback(ErrorCallback ecb)
    {
        m_ecb = ecb;
    }

    /**
     * Get packet from entry
     * \returns the packet
     */
    Ptr<const Packet> GetPacket() const
    {
        return m_packet;
    }

    /**
     * Set packet in entry
     * \param p The packet
     */
    void SetPacket(Ptr<const Packet> p)
    {
        m_packet = p;
    }

    /**
     * Get IP header
     * \returns the IP header
     */
    T GetIpHeader() const
    {
        return m_header;
    }

    /**
     * Set IP header
     * \param h the IP header
     */
    void SetIpHeader(T h)
    {
        m_header = h;
    }

    /**
     * Set expire time
     * \param exp The expiration time
     */
    void SetExpireTime(Time exp)
    {
        m_expire = exp + Simulator::Now();
    }

    /**
     * Get expire time
     * \returns the expiration time
     */
    Time GetExpireTime() const
    {
        return m_expire - Simulator::Now();
    }

  private:
    /// Data packet
    Ptr<const Packet> m_packet;
    /// IP header
    T m_header;
    /// Unicast forward callback
    UnicastForwardCallback m_ucb;
    /// Error callback
    ErrorCallback m_ecb;
    /// Expire time for queue entry
    Time m_expire;
};

/**
 * \ingroup aodv
 * \brief AODV route request queue
 *
 * Since AODV is an on demand routing we queue requests while looking for route.
 */
template <typename T>
class RequestQueue
    : public std::enable_if_t<std::is_same_v<Ipv4Address, T> || std::is_same_v<Ipv6Address, T>, T>
{
    /// Alias for determining whether the parent is Ipv4Address or Ipv6Address
    static constexpr bool IsIpv4 = std::is_same_v<Ipv4Address, T>;

    /// Alias for Ipv4 and Ipv6 classes
    using IpHeader = typename std::conditional_t<IsIpv4, Ipv4Header, Ipv6Header>;

  public:
    /**
     * constructor
     *
     * \param maxLen the maximum length
     * \param routeToQueueTimeout the route to queue timeout
     */
    RequestQueue(uint32_t maxLen, Time routeToQueueTimeout)
        : m_maxLen(maxLen),
          m_queueTimeout(routeToQueueTimeout)
    {
    }

    /**
     * Push entry in queue, if there is no entry with the same packet and destination address in
     * queue.
     * \param entry the queue entry
     * \returns true if the entry is queued
     */
    bool Enqueue(QueueEntry<IpHeader>& entry);
    /**
     * Return first found (the earliest) entry for given destination
     *
     * \param dst the destination IP address
     * \param entry the queue entry
     * \returns true if the entry is dequeued
     */
    bool Dequeue(T dst, QueueEntry<IpHeader>& entry);
    /**
     * Remove all packets with destination IP address dst
     * \param dst the destination IP address
     */
    void DropPacketWithDst(T dst);
    /**
     * Finds whether a packet with destination dst exists in the queue
     *
     * \param dst the destination IP address
     * \returns true if an entry with the IP address is found
     */
    bool Find(T dst);
    /**
     * \returns the number of entries
     */
    uint32_t GetSize();

    // Fields
    /**
     * Get maximum queue length
     * \returns the maximum queue length
     */
    uint32_t GetMaxQueueLen() const
    {
        return m_maxLen;
    }

    /**
     * Set maximum queue length
     * \param len The maximum queue length
     */
    void SetMaxQueueLen(uint32_t len)
    {
        m_maxLen = len;
    }

    /**
     * Get queue timeout
     * \returns the queue timeout
     */
    Time GetQueueTimeout() const
    {
        return m_queueTimeout;
    }

    /**
     * Set queue timeout
     * \param t The queue timeout
     */
    void SetQueueTimeout(Time t)
    {
        m_queueTimeout = t;
    }

  private:
    /// The queue
    std::vector<QueueEntry<IpHeader>> m_queue;
    /// Remove all expired entries
    void Purge();
    /**
     * Notify that packet is dropped from queue by timeout
     * \param en the queue entry to drop
     * \param reason the reason to drop the entry
     */
    void Drop(QueueEntry<IpHeader> en, std::string reason);
    /// The maximum number of packets that we allow a routing protocol to buffer.
    uint32_t m_maxLen;
    /// The maximum period of time that a routing protocol is allowed to buffer a packet for,
    /// seconds.
    Time m_queueTimeout;
};

} // namespace aodvv2
} // namespace ns3

#endif /* AODVV2_RQUEUE_H */
