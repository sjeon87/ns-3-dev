/*
 * Copyright (c) 2008 INRIA
 * 2013 University of New Brunswick
 * 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Dizhi Zhou <dizhi.zhou@gmail.com>
 * Gerard Garcia <ggarcia@deic.uab.cat>
 * Ishaan Lagwankar <lagwanka@msu.edu>
 */

#ifndef CONTACT_OPTIMIZED_DIJKSTRA_ROUTING_H
#define CONTACT_OPTIMIZED_DIJKSTRA_ROUTING_H

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
class ContactOptimizedDijkstraRouting : public BaseRoutingEngine
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    ContactOptimizedDijkstraRouting();
    virtual ~ContactOptimizedDijkstraRouting() override;

    /**
     * Initializes the routing map and internal Dijkstra data structures.
     * @param eidList List of EIDs in the network.
     */
    virtual void InitializeMap(const std::vector<std::string>& eidList) override;

    /**
     * Adds a contact edge between fromEID and toEID and marks the routing table as dirty.
     * @param fromEID EID 1 (src)
     * @param toEID EID 2 (dest)
     * @param dataRate weight of the edge
     */
    virtual void AddContact(const std::string& fromEID,
                            const std::string& toEID,
                            uint32_t dataRate) override;

    /**
     * Removes a contact edge between fromEID and toEID and marks the routing table as dirty.
     * @param fromEID EID 1 (src)
     * @param toEID EID 2 (dest)
     */
    virtual void RemoveContact(const std::string& fromEID, const std::string& toEID) override;

    /**
     * Returns the next best hop based on the precomputed Dijkstra routing table.
     * @param bundle Bundle to be transmitted (provides destinationEID).
     * @param currEID Current bundle holder's EID.
     * @return EID of the next best hop
     */
    virtual std::string GetNextHop(Ptr<Bundle> bundle, const std::string& currEID) override;

  private:
    /**
     * @brief Internal trigger to recalculate the shortest paths.
     * Runs Dijkstra's algorithm across the current adjacency list to update
     * the next hop tables for all destinations.
     */
    void RecomputeRoutingTable();

    /**
     * @brief Structure representing a directed edge in the routing graph.
     */
    struct ContactEdge
    {
        uint32_t toNode;   //!< Index of the destination node
        uint32_t dataRate; //!< Data rate (weight/capacity) of the contact
    };

    std::unordered_map<std::string, uint32_t> m_eidToIndex; //!< Fast lookup map from EID to index
    std::vector<std::string> m_indexToEid;                  //!< Fast lookup array from index to EID
    uint32_t m_size;                                        //!< Total number of nodes in the graph
    std::vector<std::vector<ContactEdge>> m_adjList; //!< Adjacency list representing the network

    std::vector<uint32_t> m_nextHopTable; //!< Precomputed next-hop routing table
    std::vector<double> m_capacity;       //!< Path capacities used during Dijkstra computation
    std::vector<uint32_t> m_parent;       //!< Parent pointers used to reconstruct the shortest path

    bool m_isDirty; //!< Flag indicating if topology changed and recalculation is needed
};

} // namespace ns3

#endif /* CONTACT_OPTIMIZED_DIJKSTRA_ROUTING_H */
