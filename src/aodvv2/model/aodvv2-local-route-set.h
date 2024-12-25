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
#ifndef AODVV2_LOCAL_ROUTE_SET_H
#define AODVV2_LOCAL_ROUTE_SET_H

#include "aodvv2-metric.h"
#include "aodvv2-packet.h"

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
#include <vector>

namespace ns3
{
namespace aodvv2
{

/**
 * @ingroup aodvv2
 * @brief Route record states
 */
enum RouteStates
{
    UNCONFIRMED = 0, //!< still not bidirectional
    IDLE = 1,        //!< route is valid but not used in the last ACTIVE_INTERVAL
    ACTIVE = 2,      //!< route is valid and used in the last ACTIVE_INTERVAL
    INVALID = 3,     //!< route expired or broken
};

/**
 * @ingroup aodvv2
 * @brief Local Route entry
 */
template <typename T>
class LocalRoute
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
     * @param dev the device
     * @param dst the destination IP address
     * @param seqNo the sequence number
     * @param iface the interface
     * @param hops the number of hops
     * @param nextHop the IP address of the next hop
     * @param lastUsed the lastUsed time of the entry
     * @param metric the metric
     * @param metricValue the metric value
     * @param state the route state
     */
    LocalRoute(Ptr<NetDevice> dev = nullptr,
               T dst = T(),
               uint16_t seqNo = 0,
               IpInterfaceAddress iface = IpInterfaceAddress(),
               uint32_t hops = 0,
               T nextHop = T(),
               Time lastUsed = Simulator::Now(),
               Time maxIdleTime = Seconds(200),
               Metric<T> metric = Metric<T>(),
               uint8_t* metricValue = new uint8_t[1]{1},
               RouteStates state = UNCONFIRMED);

    ~LocalRoute();

    ///@name Precursors management
    //\{
    /**
     * Insert precursor in precursor list if it doesn't yet exist in the list
     * @param id precursor address
     * @return true on success
     */
    bool InsertPrecursor(T id);
    /**
     * Lookup precursor by address
     * @param id precursor address
     * @return true on success
     */
    bool LookupPrecursor(T id);
    /**
     * @brief Delete precursor
     * @param id precursor address
     * @return true on success
     */
    bool DeletePrecursor(T id);
    /// Delete all precursors
    void DeleteAllPrecursors();
    /**
     * Check that precursor list is empty
     * @return true if precursor list is empty
     */
    bool IsPrecursorListEmpty() const;
    /**
     * Inserts precursors in output parameter prec if they do not yet exist in vector
     * @param prec vector of precursor addresses
     */
    void GetPrecursors(std::vector<T>& prec) const;
    //\}

    /**
     * Check if entry is valid
     */
    bool IsValid();
    /**
     * Mark entry as "down" (i.e. disable it)
     * @param badLinkLifetime duration to keep entry marked as invalid
     */
    void Invalidate(Time badLinkLifetime);

    // Fields
    /**
     * Get the max idle time
     * @returns the max idle time
     */
    Time GetMaxIdleTime() const
    {
        return m_maxIdleTime;
    }

    /**
     * Get source address function
     * @returns the IP source address
     */
    T GetSource() const
    {
        return m_ipRoute->GetSource();
    }

    /**
     * Get destination address function
     * @returns the IP destination address
     */
    T GetDestination() const
    {
        return m_ipRoute->GetDestination();
    }

    /**
     * Get route function
     * @returns The IP route
     */
    Ptr<IpRoute> GetRoute() const
    {
        return m_ipRoute;
    }

    /**
     * Set route function
     * @param r the IP route
     */
    void SetRoute(Ptr<IpRoute> r)
    {
        m_ipRoute = r;
    }

    /**
     * Set next hop address
     * @param nextHop the next hop IP address
     */
    void SetNextHop(T nextHop)
    {
        m_ipRoute->SetGateway(nextHop);
    }

    /**
     * Get next hop address
     * @returns the next hop address
     */
    T GetNextHop() const
    {
        return m_ipRoute->GetGateway();
    }

    /**
     * Set output device
     * @param dev The output device
     */
    void SetOutputDevice(Ptr<NetDevice> dev)
    {
        m_ipRoute->SetOutputDevice(dev);
    }

    /**
     * Get output device
     * @returns the output device
     */
    Ptr<NetDevice> GetOutputDevice() const
    {
        return m_ipRoute->GetOutputDevice();
    }

    /**
     * Get the IpInterfaceAddress
     * @returns the IpInterfaceAddress
     */
    IpInterfaceAddress GetInterface() const
    {
        return m_nextHopIface;
    }

    /**
     * Set the IpInterfaceAddress
     * @param iface The IpInterfaceAddress
     */
    void SetInterface(IpInterfaceAddress iface)
    {
        m_nextHopIface = iface;
    }

    /**
     * Get the valid sequence number
     * @returns the valid sequence number
     */
    bool GetValidSeqNo() const
    {
        return m_seqNo != 0;
    }

    /**
     * Set the sequence number
     * @param sn the sequence number
     */
    void SetSeqNo(uint16_t sn)
    {
        m_seqNo = sn;
        m_lastSeqNumUpdate = Simulator::Now();
    }

    /**
     * Get the sequence number
     * @returns the sequence number
     */
    uint16_t GetSeqNo() const
    {
        return m_seqNo;
    }

    /**
     * Set the number of hops
     * @param hop the number of hops
     */
    void SetHop(uint32_t hop)
    {
        m_hops = hop;
    }

    /**
     * Get the number of hops
     * @returns the number of hops
     */
    uint32_t GetHop() const
    {
        return m_hops;
    }

    /**
     * Set the lastUsed
     * @param lu The lastUsed
     */
    void SetLastUsed(Time lu)
    {
        m_lastUsed = lu + Simulator::Now();
    }

    /**
     * Get the lastUsed
     * @returns the lastUsed
     */
    Time GetLastUsed() const
    {
        return m_lastUsed;
    }

    /**
     * Set the lastSeqNumUpdate
     * @param lu The lastSeqNumUpdate
     */
    void SetLastSeqNumUpdate(Time lu)
    {
        m_lastSeqNumUpdate = lu + Simulator::Now();
    }

    /**
     * Get the lastSeqNumUpdate
     * @returns the lastSeqNumUpdate
     */
    Time GetLastSeqNumUpdate() const
    {
        return m_lastSeqNumUpdate;
    }

    /**
     * Set the metric type
     * @param type the metric type
     */
    void SetMetricType(uint8_t type)
    {
        m_metric.SetMetricType(type);
    }

    /**
     * Get the metric
     * @returns the metric
     */
    Metric<T> GetMetric() const
    {
        return m_metric;
    }

    /**
     * Get the metric type
     * @returns the metric type
     */
    uint8_t GetMetricType() const
    {
        return m_metric.GetMetricType();
    }

    /**
     * Set the metricValue
     * @param metricValue the metric value
     * @param size the metric size
     */
    void SetMetricValue(uint8_t* metricValue, uint8_t size)
    {
        m_metricValue = metricValue;
        m_metricSize = size;
    }

    /**
     * Get the metric value
     * @returns the metric value
     */
    uint8_t* GetMetricValue() const
    {
        return m_metricValue;
    }

    /**
     * Get the metric size
     * @returns the metric size
     */
    uint8_t GetMetricSize() const
    {
        return m_metricSize;
    }

    /**
     * Set the route state
     * @param state the route state
     */
    void SetState(RouteStates state)
    {
        m_state = state;
    }

    /**
     * Get the route flags
     * @returns the route flags
     */
    RouteStates GetState() const
    {
        return m_state;
    }

    /**
     * Set the RREQ count
     * @param n the RREQ count
     */
    void SetRreqCnt(uint8_t n)
    {
        m_reqCount = n;
    }

    /**
     * Get the RREQ count
     * @returns the RREQ count
     */
    uint8_t GetRreqCnt() const
    {
        return m_reqCount;
    }

    /**
     * Set the RREP count
     * @param n the RREP count
     */
    void SetRrepCnt(uint8_t n)
    {
        m_repCount = n;
    }

    /**
     * Get the RREP count
     * @returns the RREP count
     */
    uint8_t GetRrepCnt() const
    {
        return m_repCount;
    }

    /**
     * Increment the RREQ count
     */
    void IncrementRreqCnt()
    {
        m_reqCount++;
    }

    /**
     * Increment the RREP count
     */
    void IncrementRrepCnt()
    {
        m_repCount++;
    }

    /// RREP_ACK timer
    Timer m_ackTimer;

    /**
     * @brief Compare destination address
     * @param dst IP address to compare
     * @return true if equal
     */
    bool operator==(const T dst) const
    {
        return (m_ipRoute->GetDestination() == dst);
    }

    /**
     * Print packet to trace file
     * @param stream The output stream
     * @param unit The time unit to use (default Time::S)
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
    uint16_t m_seqNo;
    /// Output interface address
    IpInterfaceAddress m_nextHopIface;
    /// Time it was last used to forward a packet
    Time m_lastUsed;
    /// Time the seqNum was last updated
    Time m_lastSeqNumUpdate;
    /// Type of metric used for route
    Metric<T> m_metric;
    /// Cost of route expressed in units
    uint8_t* m_metricValue;
    /// Cost of route size
    uint8_t m_metricSize;
    /// List of precursors
    std::vector<T> m_precursorList;
    /// ip address of the originator router
    T m_seqNoRtr;
    /// Route state: unconfirmed, idle, active, invalid
    RouteStates m_state;

    /// Hop Count (number of hops needed to reach destination)
    uint32_t m_hops;
    /// Number of route requests
    uint8_t m_reqCount;
    /// Number of route replies
    uint8_t m_repCount;
    /// Maximum idle time
    Time m_maxIdleTime;
};

/**
 * @ingroup aodvv2
 * @brief The Local Route Set used by AODVv2 protocol
 */
template <typename T>
class LocalRouteSet
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
     * @param badlinkTime the local route entry badlink time
     * @param unconfirmedTime the local route entry unconfirmed time
     */
    LocalRouteSet(Time badlinkTime, Time unconfirmedTime);

    //\}
    /**
     * Add local route entry if it doesn't yet exist in the set
     * @param r local route entry
     * @return true in success
     */
    bool AddRoute(LocalRoute<T>& r);
    /**
     * Delete local route entry with destination address dst, if it exists.
     * @param dst destination address
     * @return true on success
     */
    bool DeleteRoute(T dst);
    /**
     * Lookup local route entry with destination address dst
     * @param dst destination address
     * @param metricType the metric type
     * @param route entry with destination address dst, if exists
     * @return true on success
     */
    bool LookupRoute(T dst, uint8_t metricType, LocalRoute<T>& route);
    /**
     * Lookup local route entry with destination address dst
     * @param dst destination address
     * @param routes entries with destination address dst, if exists
     * @return true on success
     */
    bool LookupRoutes(T dst, std::vector<LocalRoute<T>>& routes);
    /**
     * Lookup route in VALID state
     * @param dst destination address
     * @param routes entries with destination address dst, if exists
     * @return true on success
     */
    bool LookupValidRoutes(T dst, std::vector<LocalRoute<T>>& routes);
    /**
     * Lookup best route based on metrics
     * @param dst destination address
     * @param route entry with destination address dst, if exists
     * @return true on success
     */
    bool LookupBestRoute(T dst, LocalRoute<T>& route);
    /**
     * Update local route
     * @param rt entry with destination address dst, if exists
     * @return true on success
     */
    bool Update(LocalRoute<T>& rt);
    /**
     * Set local route entry flags
     * @param dst destination address
     * @param state the routing flags
     * @return true on success
     */
    bool SetEntryState(T dst, RouteStates state);
    /**
     * Lookup routing entries with next hop Address dst and not empty list of precursors.
     *
     * @param nextHop the next hop IP address
     * @param unreachable
     */
    void GetListOfDestinationWithNextHop(T nextHop, std::map<T, UnreachableDst>& unreachable);
    /**
     * Activate route with next hop
     *
     * @param nextHop the next hop IP address
     */
    void ActivateRouteWithNextHop(T nextHop);
    /**
     * Update routing entries with this destination as follows:
     * 1. The destination sequence number of this routing entry, if it
     *    exists and is valid, is incremented.
     * 2. The entry is invalidated by marking the route entry as invalid
     * 3. The lastUsed time field is updated to current time plus DELETE_PERIOD.
     * @param unreachable routes to invalidate
     */
    void InvalidateRoutesWithDst(const std::map<T, UnreachableDst>& unreachable);
    /**
     * Delete all route from interface with address iface
     * @param iface the interface IP address
     */
    void DeleteAllRoutesFromInterface(IpInterfaceAddress iface);

    /// Delete all entries from local route set
    void Clear()
    {
        m_ipAddressEntry.clear();
    }

    /// Delete all outdated entries and invalidate valid entry if lastUsed time is expired
    void Purge();
    /** Mark entry as unidirectional (e.g. add this neighbor to "blacklist" for blacklistTimeout
     * period)
     * @param neighbor neighbor address link to which assumed to be unidirectional
     * @param blacklistTimeout time for which the neighboring node is put into the blacklist
     * @return true on success
     */
    bool MarkLinkAsUnidirectional(T neighbor, Time blacklistTimeout);
    /**
     * Print local route
     * @param stream the output stream
     * @param unit The time unit to use (default Time::S)
     */
    void Print(Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const;

  private:
    /// The local route set
    std::vector<LocalRoute<T>> m_ipAddressEntry;
    /// Deletion time for invalid routes
    Time m_badLinkLifetime;
    /// Invalidation time for unconfirmed routes
    Time m_unconfirmedTime;
    /**
     * const version of Purge, for use by Print() method
     * @param table the local route set to purge
     */
    void PurgeTable(std::vector<LocalRoute<T>>& table) const;
};

} // namespace aodvv2
} // namespace ns3

#endif /* AODVV2_LOCAL_ROUTE_SET_H */
