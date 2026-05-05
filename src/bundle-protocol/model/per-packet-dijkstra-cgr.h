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
#ifndef PER_PACKET_DIJKSTRA_CGR_H
#define PER_PACKET_DIJKSTRA_CGR_H

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
    uint32_t toNode;      //!< Index of the destination node
    uint32_t dataRate;    //!< Nominal data rate of the contact (bps)
    uint32_t usedVolume;  //!< Bytes already committed on this contact
    uint32_t totalVolume; //!< Maximum bytes this contact window can carry (rate * duration).
};

/**
 * @ingroup BundleProtocol
 * @brief A routing engine that uses an adjacency list to maintain a contact graph and find routes.
 */
class PerPacketDijkstraCGR : public BaseRoutingEngine
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    PerPacketDijkstraCGR();
    ~PerPacketDijkstraCGR() override;

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
     * Adds a contact edge with an explicit total volume budget.
     * Total volume should be set to dataRate (bps) * contactDuration (s) converted to bytes.
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
     * Removes a contact edge between fromEID and toEID.
     * @param fromEID EID 1 (src)
     * @param toEID EID 2 (dest)
     */
    void RemoveContact(const std::string& fromEID, const std::string& toEID) override;

    /**
     * Records the bytes have been committed for transmission over the
     * fromEID -> toEID link.
     *
     * @param fromEID Source EID of the link.
     * @param toEID   Destination EID of the link.
     * @param bytes   Size of the bundle payload in bytes.
     */
    void ReserveVolume(const std::string& fromEID,
                       const std::string& toEID,
                       uint32_t bytes);

    /**
     * Returns the next best hop according to remaining link volume.
     * @param bundle  Bundle to be transmitted (provides destinationEID).
     * @param currEID Current bundle holder's EID.
     * @return EID of the next best hop, or "" if no path exists.
     */
    std::string GetNextHop(Ptr<Bundle> bundle, const std::string& currEID) override;

    /**
     * Finds the internal numerical index for a given EID.
     * @param eid Endpoint ID to look up.
     * @return The integer index corresponding to the EID.
     */
    uint32_t FindIndex(const std::string& eid) const;

  private:
    std::vector<std::string> m_eidList;              //!< EIDs mapped to indices
    std::vector<std::vector<ContactEdge>> m_adjList; //!< Adjacency list
    uint32_t m_size;                                 //!< Total number of nodes
};

} // namespace ns3

#endif /* PER_PACKET_DIJKSTRA_CGR_H */