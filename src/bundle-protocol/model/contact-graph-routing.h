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
#ifndef CONTACT_GRAPH_ROUTING_H
#define CONTACT_GRAPH_ROUTING_H

#include "base-routing-engine.h"
#include "bundle.h"

#include "ns3/object.h"

#include <string>
#include <vector>

namespace ns3
{

/**
 * @brief Structure representing a directed edge in the contact graph.
 */
struct ContactEdge
{
    uint32_t toNode;   //!< Index of the destination node
    uint32_t dataRate; //!< Data rate (weight) of the contact
};

/**
 * @ingroup BundleProtocol
 * @brief A routing engine that uses an adjacency list to maintain a contact graph and find routes.
 */
class ContactGraph : public BaseRoutingEngine
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    ContactGraph();
    ~ContactGraph() override;

    /**
     * Initializes the contact graph's adjacency list structure.
     * @param eidList List of EIDs in the network.
     */
    void InitializeMap(const std::vector<std::string>& eidList) override;

    /**
     * Adds a contact edge between fromEID and toEID with weight dataRate.
     * @param fromEID EID 1 (src)
     * @param toEID EID 2 (dest)
     * @param dataRate weight of the edge
     */
    void AddContact(const std::string& fromEID,
                    const std::string& toEID,
                    uint32_t dataRate) override;

    /**
     * Removes a contact edge between fromEID and toEID.
     * @param fromEID EID 1 (src)
     * @param toEID EID 2 (dest)
     */
    void RemoveContact(const std::string& fromEID, const std::string& toEID) override;

    /**
     * Returns the next best hop according to the current contact graph.
     * @param bundle Bundle to be transmitted (provides destinationEID).
     * @param currEID Current bundle holder's EID.
     * @return EID of the next best hop
     */
    std::string GetNextHop(Ptr<Bundle> bundle, const std::string& currEID) override;

    /**
     * Finds the internal numerical index for a given EID.
     * @param eid Endpoint ID to look up.
     * @return The integer index corresponding to the EID.
     */
    uint32_t FindIndex(const std::string& eid) const;

  private:
    std::vector<std::string> m_eidList; //!< List of registered EIDs mapped to indices
    std::vector<std::vector<ContactEdge>>
        m_adjList;   //!< Adjacency list representing the contact graph
    uint32_t m_size; //!< Total number of nodes in the graph
};

} // namespace ns3
#endif /* CONTACT_GRAPH_ROUTING_H */
