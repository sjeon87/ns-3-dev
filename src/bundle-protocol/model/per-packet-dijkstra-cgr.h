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

#include "ns3/nstime.h"
#include "ns3/object.h"

#include <string>
#include <vector>

namespace ns3
{

/**
 * @ingroup BundleProtocol
 * @brief A routing engine that uses an adjacency list to maintain a contact graph and find routes.
 * * This engine runs a time-varying Dijkstra algorithm per packet to find the path
 * with the Earliest Arrival Time (EAT).
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
     * Adds a static contact edge. In a CGR context, this maps to an infinite
     * contact window from time 0 to infinity, to support backwards compatibility.
     * @param fromEID EID 1 (src)
     * @param toEID EID 2 (dest)
     * @param dataRate weight of the edge
     */
    void AddContact(const std::string& fromEID,
                    const std::string& toEID,
                    uint32_t dataRate) override;

    /**
     * Removes all contact edges between fromEID and toEID.
     * @param fromEID EID 1 (src)
     * @param toEID EID 2 (dest)
     */
    void RemoveContact(const std::string& fromEID, const std::string& toEID) override;

    /**
     * Records the bytes have been committed for transmission over the
     * fromEID -> toEID link for the currently active contact window.
     *
     * @param fromEID Source EID of the link.
     * @param toEID   Destination EID of the link.
     * @param bytes   Size of the bundle payload in bytes.
     */
    void ReserveVolume(const std::string& fromEID,
                       const std::string& toEID,
                       uint32_t bytes) override;

    /**
     * Store a contact window record parsed from the contact plan.
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
     * Returns the next best hop according to Earliest Arrival Time (EAT).
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
    std::vector<std::string> m_eidList;                //!< EIDs mapped to indices
    std::vector<std::vector<ContactWindow>> m_adjList; //!< Adjacency list of scheduled contacts
    uint32_t m_size;                                   //!< Total number of nodes
};

} // namespace ns3

#endif /* PER_PACKET_DIJKSTRA_CGR_H */
