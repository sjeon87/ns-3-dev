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
 * \ingroup aodvv2
 * \brief Route record states
 */
enum RouteStates
{
    UNCONFIRMED = 0, //!< still not bidirectional
    IDLE = 1,        //!< route is valid but not used in the last ACTIVE_INTERVAL
    ACTIVE = 2,      //!< route is valid and used in the last ACTIVE_INTERVAL
    INVALID = 3,     //!< route expired or broken
};

/**
 * \ingroup aodvv2
 * \brief Routing table entry
 */
template <typename T>
class RoutingTableEntry
    : public std::enable_if_t<std::is_same_v<Ipv4Address, T> || std::is_same_v<Ipv6Address, T>, T>
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
     * \param seqNo the sequence number
     * \param iface the interface
     * \param hops the number of hops
     * \param nextHop the IP address of the next hop
     * \param lastUsed the lastUsed time of the entry
     */
    RoutingTableEntry(Ptr<NetDevice> dev = nullptr,
                      T dst = T(),
                      uint32_t seqNo = 0,
                      IpInterfaceAddress iface = IpInterfaceAddress(),
                      uint16_t hops = 0,
                      T nextHop = T(),
                      Time lastUsed = Simulator::Now());

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
        return m_nextHopIface;
    }

    /**
     * Set the IpInterfaceAddress
     * \param iface The IpInterfaceAddress
     */
    void SetInterface(IpInterfaceAddress iface)
    {
        m_nextHopIface = iface;
    }

    /**
     * Get the valid sequence number
     * \returns the valid sequence number
     */
    bool GetValidSeqNo() const
    {
        return m_seqNo != 0;
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
     * Set the lastUsed
     * \param lu The lastUsed
     */
    void SetLastUsed(Time lu)
    {
        m_lastUsed = lu;
    }

    /**
     * Get the lastUsed
     * \returns the lastUsed
     */
    Time GetLastUsed() const
    {
        return m_lastUsed;
    }

    /**
     * Set the route state
     * \param state the route state
     */
    void SetState(RouteStates state)
    {
        m_state = state;
    }

    /**
     * Get the route flags
     * \returns the route flags
     */
    RouteStates GetState() const
    {
        return m_state;
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
    /** Ip route, include
     *   - destination address
     *   - source address
     *   - next hop address (gateway)
     *   - output device
     */
    Ptr<IpRoute> m_ipRoute;
    /// Destination address prefix length
    uint32_t m_prefixLength;
    /// Destination Sequence Number
    uint32_t m_seqNo;
    /// Output interface address
    IpInterfaceAddress m_nextHopIface;
    /// Time it was last used to forward a packet
    Time m_lastUsed;
    /// Time the seqNum was last updated
    Time m_lastSeqNumUpdate;
    /// Type of metric used for route
    uint8_t m_metricType;
    /// Cost of route expressed in units
    uint32_t m_metric;
    /// List of precursors
    std::vector<T> m_precursorList;
    /// ip address of the originator router
    T m_seqNoRtr;
    /// Routing state: unconfirmed, idle, active, invalid
    RouteStates m_state;

    /// Hop Count (number of hops needed to reach destination)
    uint16_t m_hops;
    /// Number of route requests
    uint8_t m_reqCount;
};

/**
 * \ingroup aodvv2
 * \brief The Routing table used by AODVv2 protocol
 */
template <typename T>
class RoutingTable
    : public std::enable_if_t<std::is_same_v<Ipv4Address, T> || std::is_same_v<Ipv6Address, T>, T>
{
    /// Alias for determining whether the parent is Ipv4Address or Ipv6Address
    static constexpr bool IsIpv4 = std::is_same_v<Ipv4Address, T>;

    /// Alias for Ipv4 and Ipv6 classes
    using IpInterfaceAddress =
        typename std::conditional_t<IsIpv4, Ipv4InterfaceAddress, Ipv6InterfaceAddress>;

  public:
    /**
     * constructor
     * \param t the routing table entry time
     */
    RoutingTable(Time t);

    ///\name Handle time of invalid route
    //\{
    /**
     * Get the lastUsed time of a bad link
     *
     * \return the lastUsed time of a bad link
     */
    Time GetBadLinkLifetime() const
    {
        return m_badLinkLifetime;
    }

    /**
     * Set the lastUsed time of a bad link
     *
     * \param t the lastUsed time of a bad link
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
    bool SetEntryState(T dst, RouteStates state);
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
     * 3. The lastUsed time field is updated to current time plus DELETE_PERIOD.
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

    /// Delete all outdated entries and invalidate valid entry if lastUsed time is expired
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
