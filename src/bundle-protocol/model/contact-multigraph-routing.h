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
#ifndef CMR_ROUTING_ENGINE_H
#define CMR_ROUTING_ENGINE_H

#include "base-routing-engine.h"

#include "ns3/nstime.h"

#include <string>
#include <vector>

namespace ns3
{

/**
 * @ingroup BundleProtocol
 * @brief A Contact Multigraph Routing (CMR) engine.
 *
 * Implements a multigraph data structure where contacts are grouped by destination
 * and sorted chronologically. Uses a contact-optimized Dijkstra algorithm to
 * find the path with the Earliest Arrival Time (EAT).
 */
class ContactMultigraphRouting : public BaseRoutingEngine
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    ContactMultigraphRouting();
    ~ContactMultigraphRouting() override;

    /**
     * @brief Initialize the mapping of Endpoint IDs.
     * @param eidList List of Endpoint IDs (EIDs) to be registered in the routing engine.
     */
    void InitializeMap(const std::vector<std::string>& eidList) override;

    /**
     * @brief Add a permanent contact link.
     * @param fromEID The source Endpoint ID.
     * @param toEID The destination Endpoint ID.
     * @param dataRate The data rate of the contact in bps.
     */
    void AddContact(const std::string& fromEID,
                    const std::string& toEID,
                    uint32_t dataRate) override;

    /**
     * @brief Add a scheduled/timed contact window.
     * @param fromEID The source Endpoint ID.
     * @param toEID The destination Endpoint ID.
     * @param startTime The time when the contact begins.
     * @param endTime The time when the contact ends.
     * @param dataRate The data rate of the contact in bps.
     * @param delay The propagation delay across the contact link.
     * @param totalVolume The maximum data volume capacity of the contact.
     */
    void AddTimedContact(const std::string& fromEID,
                         const std::string& toEID,
                         Time startTime,
                         Time endTime,
                         uint32_t dataRate,
                         Time delay,
                         uint32_t totalVolume) override;

    /**
     * @brief Remove an existing contact link.
     * @param fromEID The source Endpoint ID.
     * @param toEID The destination Endpoint ID.
     */
    void RemoveContact(const std::string& fromEID, const std::string& toEID) override;

    /**
     * @brief Reserve bundle volume on a specific contact link.
     * @param fromEID The source Endpoint ID.
     * @param toEID The destination Endpoint ID.
     * @param bytes The amount of data volume to reserve in bytes.
     */
    void ReserveVolume(const std::string& fromEID,
                       const std::string& toEID,
                       uint32_t bytes) override;

    /**
     * @brief Determine the next hop to forward a bundle.
     * @param bundle The bundle to be routed.
     * @param currEID The Endpoint ID of the current node holding the bundle.
     * @return The Endpoint ID of the next hop.
     */
    std::string GetNextHop(Ptr<Bundle> bundle, const std::string& currEID) override;

    /**
     * @brief Find the internal index for a given Endpoint ID.
     * @param eid The Endpoint ID to look up.
     * @return The internal numeric index associated with the EID.
     */
    uint32_t FindIndex(const std::string& eid) const;

  private:
    /**
     * @brief Recompute the internal routing table for a given bundle size.
     * @param bundleSize The size of the bundle in bytes to calculate valid paths for.
     */
    void RecomputeRoutingTable(uint32_t bundleSize);

    std::vector<std::string> m_eidList; //!< List of registered EIDs
    uint32_t m_size;                    //!< Number of nodes/EIDs registered

    std::vector<std::vector<std::vector<ContactWindow>>>
        m_multigraph; //!< Multigraph contact storage

    std::vector<uint32_t> m_nextHopTable; //!< Cached next hop routing table

    bool m_isDirty;                //!< Flag indicating if the routing table needs recomputation
    Time m_nextTopologyChangeTime; //!< Time of the next topology change
};

} // namespace ns3

#endif /* CMR_ROUTING_ENGINE_H */
