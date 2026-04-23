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

#include "ns3/nstime.h"

#include <string>
#include <vector>

namespace ns3
{

struct ContactEdge
{
    uint32_t toNode;
    uint32_t dataRate;
};

class ContactOptimizedDijkstraRouting : public BaseRoutingEngine
{
  public:
    static TypeId GetTypeId();
    ContactOptimizedDijkstraRouting();
    ~ContactOptimizedDijkstraRouting() override;

    std::string GetNextHop(Ptr<Bundle> bundle, const std::string& currEID) override;

    void InitializeMap(const std::vector<std::string>& eidList) override;
    void AddContact(const std::string& fromEID,
                    const std::string& toEID,
                    uint32_t dataRate) override;
    void RemoveContact(const std::string& fromEID, const std::string& toEID) override;

  private:
    uint32_t FindIndex(const std::string& eid) const;
    void RecomputeRoutingTable();

    uint32_t m_size;
    std::vector<std::string> m_eidList;
    std::vector<std::vector<ContactEdge>> m_adjList;
    std::vector<std::vector<uint32_t>> m_nextHopTable;
    bool m_isDirty;
};

} // namespace ns3

#endif /* CONTACT_OPTIMIZED_DIJKSTRA_ROUTING_H */
