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
#ifndef AODVV2_RTABLE_H
#define AODVV2_RTABLE_H

#include "ns3/internet-module.h"
#include "ns3/ipv4-route.h"
#include "ns3/ipv4.h"
#include "ns3/net-device.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/timer.h"

#include <cassert>
#include <map>
#include <stdint.h>
#include <sys/types.h>

namespace ns3
{
namespace aodvv2
{

/**
 * \ingroup aodv
 * \brief Route record states
 */
enum RouteFlags
{
    CONFIRMED = 0,   //!< CONFIRMED
    BLACKLISTED = 1, //!< BLACKLISTED
    HEARD = 2,       //!< HEARD
};

/**
 * \ingroup aodv
 * \brief Routing table entry
 */
template <typename T>
class RoutingTableEntry
{
    /// Alias for determining whether the parent is Ipv4Address or Ipv6Address
    static constexpr bool IsIpv4 = std::is_same_v<Ipv4Address, T>;

    /// Alias for Ipv4InterfaceAddress and Ipv6InterfaceAddress classes
    using IpInterfaceAddress =
        typename std::conditional_t<IsIpv4, Ipv4InterfaceAddress, Ipv6InterfaceAddress>;
    /// Alias for Ipv4Route and Ipv6Route classes
    using IpRoute = typename std::conditional_t<IsIpv4, Ipv4Route, Ipv6Route>;

  public:
    /**
     * constructor
     *
     * \param dev the device
     * \param dst the destination IP address
     * \param vSeqNo verify sequence number flag
     * \param seqNo the sequence number
     * \param iface the interface
     * \param hops the number of hops
     * \param nextHop the IP address of the next hop
     * \param lifetime the lifetime of the entry
     */
    RoutingTableEntry(Ptr<NetDevice> dev = nullptr,
                      T dst = T(),
                      bool vSeqNo = false,
                      uint32_t seqNo = 0,
                      IpInterfaceAddress iface = IpInterfaceAddress(),
                      uint16_t hops = 0,
                      T nextHop = T(),
                      Time lifetime = Simulator::Now());

    ~RoutingTableEntry();

    ///\name Precursors management
    //\{
    /**
     * Insert precursor in precursor list if it doesn't yet exist in the list
     * \param id precursor address
     * \return true on success
     */
    bool InsertPrecursor(T id);
    /**
     * Lookup precursor by address
     * \param id precursor address
     * \return true on success
     */
    bool LookupPrecursor(T id);
    /**
     * \brief Delete precursor
     * \param id precursor address
     * \return true on success
     */
    bool DeletePrecursor(T id);
    /// Delete all precursors
    void DeleteAllPrecursors();
    /**
     * Check that precursor list is empty
     * \return true if precursor list is empty
     */
    bool IsPrecursorListEmpty() const;
    /**
     * Inserts precursors in output parameter prec if they do not yet exist in vector
     * \param prec vector of precursor addresses
     */
    void GetPrecursors(std::vector<T>& prec) const;
    //\}

    /**
     * Mark entry as "down" (i.e. disable it)
     * \param badLinkLifetime duration to keep entry marked as invalid
     */
    void Invalidate(Time badLinkLifetime);

    // Fields
    /**
     * Get destination address function
     * \returns the IP destination address
     */
    T GetDestination() const
    {
        return m_ipRoute->GetDestination();
    }

    /**
     * Get route function
     * \returns The IP route
     */
    Ptr<IpRoute> GetRoute() const
    {
        return m_ipRoute;
    }

    /**
     * Set route function
     * \param r the IP route
     */
    void SetRoute(Ptr<IpRoute> r)
    {
        m_ipRoute = r;
    }

    /**
     * Set next hop address
     * \param nextHop the next hop IP address
     */
    void SetNextHop(T nextHop)
    {
        m_ipRoute->SetGateway(nextHop);
    }

    /**
     * Get next hop address
     * \returns the next hop address
     */
    T GetNextHop() const
    {
        return m_ipRoute->GetGateway();
    }

    /**
     * Set output device
     * \param dev The output device
     */
    void SetOutputDevice(Ptr<NetDevice> dev)
    {
        m_ipRoute->SetOutputDevice(dev);
    }

    /**
     * Get output device
     * \returns the output device
     */
    Ptr<NetDevice> GetOutputDevice() const
    {
        return m_ipRoute->GetOutputDevice();
    }

    /**
     * Get the IpInterfaceAddress
     * \returns the IpInterfaceAddress
     */
    IpInterfaceAddress GetInterface() const
    {
        return m_iface;
    }

    /**
     * Set the IpInterfaceAddress
     * \param iface The IpInterfaceAddress
     */
    void SetInterface(IpInterfaceAddress iface)
    {
        m_iface = iface;
    }

    /**
     * Set the valid sequence number
     * \param s the sequence number
     */
    void SetValidSeqNo(bool s)
    {
        m_validSeqNo = s;
    }

    /**
     * Get the valid sequence number
     * \returns the valid sequence number
     */
    bool GetValidSeqNo() const
    {
        return m_validSeqNo;
    }

    /**
     * Set the sequence number
     * \param sn the sequence number
     */
    void SetSeqNo(uint32_t sn)
    {
        m_seqNo = sn;
    }

    /**
     * Get the sequence number
     * \returns the sequence number
     */
    uint32_t GetSeqNo() const
    {
        return m_seqNo;
    }

    /**
     * Set the number of hops
     * \param hop the number of hops
     */
    void SetHop(uint16_t hop)
    {
        m_hops = hop;
    }

    /**
     * Get the number of hops
     * \returns the number of hops
     */
    uint16_t GetHop() const
    {
        return m_hops;
    }

    /**
     * Set the lifetime
     * \param lt The lifetime
     */
    void SetLifeTime(Time lt)
    {
        m_lifeTime = lt + Simulator::Now();
    }

    /**
     * Get the lifetime
     * \returns the lifetime
     */
    Time GetLifeTime() const
    {
        return m_lifeTime - Simulator::Now();
    }

    /**
     * Set the route flags
     * \param flag the route flags
     */
    void SetFlag(RouteFlags flag)
    {
        m_flag = flag;
    }

    /**
     * Get the route flags
     * \returns the route flags
     */
    RouteFlags GetFlag() const
    {
        return m_flag;
    }

    /**
     * Set the RREQ count
     * \param n the RREQ count
     */
    void SetRreqCnt(uint8_t n)
    {
        m_reqCount = n;
    }

    /**
     * Get the RREQ count
     * \returns the RREQ count
     */
    uint8_t GetRreqCnt() const
    {
        return m_reqCount;
    }

    /**
     * Increment the RREQ count
     */
    void IncrementRreqCnt()
    {
        m_reqCount++;
    }

    /**
     * Set the unidirectional flag
     * \param u the uni directional flag
     */
    void SetUnidirectional(bool u)
    {
        m_blackListState = u;
    }

    /**
     * Get the unidirectional flag
     * \returns the unidirectional flag
     */
    bool IsUnidirectional() const
    {
        return m_blackListState;
    }

    /**
     * Set the blacklist timeout
     * \param t the blacklist timeout value
     */
    void SetBlacklistTimeout(Time t)
    {
        m_blackListTimeout = t;
    }

    /**
     * Get the blacklist timeout value
     * \returns the blacklist timeout value
     */
    Time GetBlacklistTimeout() const
    {
        return m_blackListTimeout;
    }

    /// RREP_ACK timer
    Timer m_ackTimer;

    /**
     * \brief Compare destination address
     * \param dst IP address to compare
     * \return true if equal
     */
    bool operator==(const T dst) const
    {
        return (m_ipRoute->GetDestination() == dst);
    }

    /**
     * Print packet to trace file
     * \param stream The output stream
     * \param unit The time unit to use (default Time::S)
     */
    void Print(Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const;

  private:
    /// Valid Destination Sequence Number flag
    bool m_validSeqNo;
    /// Destination Sequence Number, if m_validSeqNo = true
    uint32_t m_seqNo;
    /// Hop Count (number of hops needed to reach destination)
    uint16_t m_hops;
    /**
     * \brief Expiration or deletion time of the route
     * Lifetime field in the routing table plays dual role:
     * for an active route it is the expiration time, and for an invalid route
     * it is the deletion time.
     */
    Time m_lifeTime;
    /** Ip route, include
     *   - destination address
     *   - source address
     *   - next hop address (gateway)
     *   - output device
     */
    Ptr<IpRoute> m_ipRoute;
    /// Output interface address
    IpInterfaceAddress m_iface;
    /// Routing flags: valid, invalid or in search
    RouteFlags m_flag;

    /// List of precursors
    std::vector<T> m_precursorList;
    /// When I can send another request
    Time m_routeRequestTimeout;
    /// Number of route requests
    uint8_t m_reqCount;
    /// Indicate if this entry is in "blacklist"
    bool m_blackListState;
    /// Time for which the node is put into the blacklist
    Time m_blackListTimeout;
};

/**
 * \ingroup aodv
 * \brief The Routing table used by AODV protocol
 */
template <typename T>
class RoutingTable
{
    /// Alias for determining whether the parent is Ipv4Address or Ipv6Address
    static constexpr bool IsIpv4 = std::is_same_v<Ipv4Address, T>;

    /// Alias for Ipv4 and Ipv6 classes
    using IpInterfaceAddress =
        typename std::conditional_t<IsIpv4, Ipv4InterfaceAddress, Ipv6InterfaceAddress>;

  public:
    /**
     * constructor
     * \param t the routing table entry lifetime
     */
    RoutingTable(Time t);

    ///\name Handle lifetime of invalid route
    //\{
    /**
     * Get the lifetime of a bad link
     *
     * \return the lifetime of a bad link
     */
    Time GetBadLinkLifetime() const
    {
        return m_badLinkLifetime;
    }

    /**
     * Set the lifetime of a bad link
     *
     * \param t the lifetime of a bad link
     */
    void SetBadLinkLifetime(Time t)
    {
        m_badLinkLifetime = t;
    }

    //\}
    /**
     * Add routing table entry if it doesn't yet exist in routing table
     * \param r routing table entry
     * \return true in success
     */
    bool AddRoute(RoutingTableEntry<T>& r);
    /**
     * Delete routing table entry with destination address dst, if it exists.
     * \param dst destination address
     * \return true on success
     */
    bool DeleteRoute(T dst);
    /**
     * Lookup routing table entry with destination address dst
     * \param dst destination address
     * \param rt entry with destination address dst, if exists
     * \return true on success
     */
    bool LookupRoute(T dst, RoutingTableEntry<T>& rt);
    /**
     * Lookup route in VALID state
     * \param dst destination address
     * \param rt entry with destination address dst, if exists
     * \return true on success
     */
    bool LookupValidRoute(T dst, RoutingTableEntry<T>& rt);
    /**
     * Update routing table
     * \param rt entry with destination address dst, if exists
     * \return true on success
     */
    bool Update(RoutingTableEntry<T>& rt);
    /**
     * Set routing table entry flags
     * \param dst destination address
     * \param state the routing flags
     * \return true on success
     */
    bool SetEntryState(T dst, RouteFlags state);
    /**
     * Lookup routing entries with next hop Address dst and not empty list of precursors.
     *
     * \param nextHop the next hop IP address
     * \param unreachable
     */
    void GetListOfDestinationWithNextHop(T nextHop, std::map<T, uint32_t>& unreachable);
    /**
     * Update routing entries with this destination as follows:
     * 1. The destination sequence number of this routing entry, if it
     *    exists and is valid, is incremented.
     * 2. The entry is invalidated by marking the route entry as invalid
     * 3. The Lifetime field is updated to current time plus DELETE_PERIOD.
     * \param unreachable routes to invalidate
     */
    void InvalidateRoutesWithDst(const std::map<T, uint32_t>& unreachable);
    /**
     * Delete all route from interface with address iface
     * \param iface the interface IP address
     */
    void DeleteAllRoutesFromInterface(IpInterfaceAddress iface);

    /// Delete all entries from routing table
    void Clear()
    {
        m_ipAddressEntry.clear();
    }

    /// Delete all outdated entries and invalidate valid entry if Lifetime is expired
    void Purge();
    /** Mark entry as unidirectional (e.g. add this neighbor to "blacklist" for blacklistTimeout
     * period)
     * \param neighbor neighbor address link to which assumed to be unidirectional
     * \param blacklistTimeout time for which the neighboring node is put into the blacklist
     * \return true on success
     */
    bool MarkLinkAsUnidirectional(T neighbor, Time blacklistTimeout);
    /**
     * Print routing table
     * \param stream the output stream
     * \param unit The time unit to use (default Time::S)
     */
    void Print(Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const;

  private:
    /// The routing table
    std::map<T, RoutingTableEntry<T>> m_ipAddressEntry;
    /// Deletion time for invalid routes
    Time m_badLinkLifetime;
    /**
     * const version of Purge, for use by Print() method
     * \param table the routing table entry to purge
     */
    void Purge(std::map<T, RoutingTableEntry<T>>& table) const;
};

} // namespace aodvv2
} // namespace ns3

#endif /* AODVV2_RTABLE_H */
