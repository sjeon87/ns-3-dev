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
    static TypeId GetTypeId();
    ContactMultigraphRouting();
    ~ContactMultigraphRouting() override;

    void InitializeMap(const std::vector<std::string>& eidList) override;

    void AddContact(const std::string& fromEID,
                    const std::string& toEID,
                    uint32_t dataRate) override;

    void AddTimedContact(const std::string& fromEID,
                         const std::string& toEID,
                         Time startTime,
                         Time endTime,
                         uint32_t dataRate,
                         Time delay,
                         uint32_t totalVolume) override;

    void RemoveContact(const std::string& fromEID, const std::string& toEID) override;

    void ReserveVolume(const std::string& fromEID,
                       const std::string& toEID,
                       uint32_t bytes) override;

    std::string GetNextHop(Ptr<Bundle> bundle, const std::string& currEID) override;

    uint32_t FindIndex(const std::string& eid) const;

  private:
    void RecomputeRoutingTable(uint32_t bundleSize);

    std::vector<std::string> m_eidList;
    uint32_t m_size;

    std::vector<std::vector<std::vector<ContactWindow>>> m_multigraph;

    std::vector<uint32_t> m_nextHopTable;

    bool m_isDirty;
    Time m_nextTopologyChangeTime;
};

} // namespace ns3

#endif /* CMR_ROUTING_ENGINE_H */
