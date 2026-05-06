/*
 * Copyright (c) 2008 INRIA
 *                  2013 University of New Brunswick
 *                  2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *           Dizhi Zhou <dizhi.zhou@gmail.com>
 *           Gerard Garcia <ggarcia@deic.uab.cat>
 *           Ishaan Lagwankar <lagwanka@msu.edu>
 */
#ifndef PER_CONTACT_DIJKSTRA_CGR_H
#define PER_CONTACT_DIJKSTRA_CGR_H

#include "base-routing-engine.h"

#include "ns3/nstime.h"

#include <string>
#include <vector>

namespace ns3
{

/**
 * @ingroup BundleProtocol
 * @brief A routing engine that precomputes paths using a contact-optimized Dijkstra algorithm.
 *
 * This routing engine maintains a precomputed routing table. It uses a dirty flag
 * approach to lazily recompute Dijkstra only when the topology changes.
 */
class PerContactDijkstraCGR : public BaseRoutingEngine
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    PerContactDijkstraCGR();
    ~PerContactDijkstraCGR() override;

    /**
     * Initializes the routing map and internal Dijkstra data structures.
     * @param eidList List of EIDs in the network.
     */
    void InitializeMap(const std::vector<std::string>& eidList) override;

    /**
     * Adds a contact edge with an unknown total volume.
     * The router will fall back to using the raw dataRate as the capacity metric.
     * Prefer the four-argument overload when the contact duration is known.
     *
     * @param fromEID  Source EID.
     * @param toEID    Destination EID.
     * @param dataRate Nominal link data rate (bps).
     */
    void AddContact(const std::string& fromEID,
                    const std::string& toEID,
                    uint32_t dataRate) override;

    /**
     * Adds a contact edge with an explicit total volume budget and marks the
     * routing table dirty.
     * Total volume should equal dataRate (bps) * contactDuration (s) / 8 (bytes).
     *
     * @param fromEID     Source EID.
     * @param toEID       Destination EID.
     * @param startTime   Simulation time when the contact opens.
     * @param endTime     Simulation time when the contact closes.
     * @param dataRate    Nominal link data rate (bps).
     * @param delay       Propagation delay.
     * @param totalVolume Maximum bytes this contact window can carry.
     */
    void AddTimedContact(const std::string& fromEID,
                         const std::string& toEID,
                         Time startTime,
                         Time endTime,
                         uint32_t dataRate,
                         Time delay,
                         uint32_t totalVolume) override;

    /**
     * @brief Removes all contact edges between fromEID and toEID and marks the
     * routing table dirty.
     *
     * @param fromEID Source EID.
     * @param toEID   Destination EID.
     */
    void RemoveContact(const std::string& fromEID, const std::string& toEID) override;

    /**
     * @brief Records that bytes have been committed for transmission over the
     * currently active fromEID -> toEID link, and marks the routing table dirty
     * so it re-evaluates capacity constraints on the next routing request.
     *
     * @param fromEID Source EID of the link.
     * @param toEID   Destination EID of the link.
     * @param bytes   Size of the bundle payload in bytes.
     */
    void ReserveVolume(const std::string& fromEID,
                       const std::string& toEID,
                       uint32_t bytes) override;

    /**
     * @brief Returns the next best hop based on the precomputed Dijkstra routing table.
     * * Checks the cache against volume depletion or simulation clock progression
     * past a scheduled topology change, triggering a recompute if necessary.
     *
     * @param bundle  Bundle to be transmitted (provides destinationEID).
     * @param currEID Current bundle holder's EID.
     * @return EID of the next best hop, or "" if no valid time-varying path exists.
     */
    std::string GetNextHop(Ptr<Bundle> bundle, const std::string& currEID) override;

    /**
     * @brief Finds the internal numerical index for a given EID.
     * @param eid Endpoint ID to look up.
     * @return The integer index corresponding to the EID.
     */
    uint32_t FindIndex(const std::string& eid) const;

  private:
    /**
     * @brief Recomputes the all-pairs earliest-arrival routing table.
     * * Runs one Dijkstra per source node using Wait Time, Transmission Time, and
     * Propagation Delay to find the path with the Earliest Arrival Time (EAT).
     * Calculates and sets the next time the cache naturally expires based on the contact plan.
     */
    void RecomputeRoutingTable();

    std::vector<std::string> m_eidList;                //!< Index -> EID lookup
    uint32_t m_size;                                   //!< Total number of nodes
    std::vector<std::vector<ContactWindow>> m_adjList; //!< Adjacency list of scheduled contacts

    std::vector<uint32_t> m_nextHopTable; //!< Flattened all-pairs next-hop table

    bool m_isDirty;                //!< True when volume runs out or edges are manually removed
    Time m_nextTopologyChangeTime; //!< The exact simulation time the cache naturally expires
};

} // namespace ns3

#endif /* PER_CONTACT_DIJKSTRA_CGR_H */
