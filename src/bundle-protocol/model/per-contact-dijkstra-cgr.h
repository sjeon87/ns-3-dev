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

#include <unordered_map>
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
     * @param dataRate    Nominal link data rate (bps).
     * @param totalVolume Maximum bytes this contact window can carry.
     */
    void AddContact(const std::string& fromEID,
                    const std::string& toEID,
                    uint32_t dataRate,
                    uint32_t totalVolume);

    /**
     * Removes a contact edge between fromEID and toEID and marks the routing
     * table dirty.  Volume accounting is discarded with the edge.
     *
     * @param fromEID Source EID.
     * @param toEID   Destination EID.
     */
    void RemoveContact(const std::string& fromEID, const std::string& toEID) override;

    /**
     * Records that `bytes` have been committed for transmission over the
     * fromEID -> toEID link, and marks the routing table dirty so the next
     * GetNextHop call recomputes paths against the updated volumes.
     *
     * @param fromEID Source EID of the link.
     * @param toEID   Destination EID of the link.
     * @param bytes   Size of the bundle in bytes.
     */
    void ReserveVolume(const std::string& fromEID, const std::string& toEID, uint32_t bytes);

    /**
     * Returns the next best hop based on the precomputed Dijkstra routing table.
     * Triggers a recompute if the topology or any volume reservation has changed.
     *
     * @param bundle  Bundle to be transmitted (provides destinationEID).
     * @param currEID Current bundle holder's EID.
     * @return EID of the next best hop, or "" if no path exists.
     */
    std::string GetNextHop(Ptr<Bundle> bundle, const std::string& currEID) override;

  private:
    /**
     * @brief Structure representing a directed edge in the routing graph.
     */
    struct ContactEdge
    {
        uint32_t toNode;      //!< Index of the destination node
        uint32_t dataRate;    //!< Nominal data rate (bps)
        uint32_t usedVolume;  //!< Bytes already committed on this contact
        uint32_t totalVolume; //!< Max bytes this contact can carry (0 = unknown/unlimited)
    };

    /**
     * @brief Recomputes the all-pairs widest-path routing table.
     * Runs one Dijkstra per source node, using remaining volume as the edge
     * weight.  Called lazily whenever m_isDirty is true.
     */
    void RecomputeRoutingTable();

    std::unordered_map<std::string, uint32_t> m_eidToIndex; //!< EID -> index lookup
    std::vector<std::string> m_indexToEid;                  //!< Index -> EID lookup
    uint32_t m_size;                                        //!< Total number of nodes

    std::vector<std::vector<ContactEdge>> m_adjList; //!< Adjacency list
    std::vector<uint32_t> m_nextHopTable;            //!< Flattened next-hop table
    std::vector<double> m_capacity; //!< Per-node capacities (reused across Dijkstra runs)
    std::vector<uint32_t> m_parent; //!< Parent pointers for path reconstruction

    bool m_isDirty; //!< True when topology or volumes changed and table needs recompute
};

} // namespace ns3

#endif /* PER_CONTACT_DIJKSTRA_CGR_H */
