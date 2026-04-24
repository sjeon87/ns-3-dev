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

class ContactOptimizedDijkstraRouting : public BaseRoutingEngine
{
  public:
    static TypeId GetTypeId();
    ContactOptimizedDijkstraRouting();
    virtual ~ContactOptimizedDijkstraRouting() override;
    virtual void InitializeMap(const std::vector<std::string>& eidList) override;
    virtual void AddContact(const std::string& fromEID,
                            const std::string& toEID,
                            uint32_t dataRate) override;
    virtual void RemoveContact(const std::string& fromEID, const std::string& toEID) override;
    virtual std::string GetNextHop(Ptr<Bundle> bundle, const std::string& currEID) override;

  private:
    void RecomputeRoutingTable();

    struct ContactEdge
    {
        uint32_t toNode;
        uint32_t dataRate;
    };

    std::unordered_map<std::string, uint32_t> m_eidToIndex;
    std::vector<std::string> m_indexToEid;
    uint32_t m_size;
    std::vector<std::vector<ContactEdge>> m_adjList;
    std::vector<uint32_t> m_nextHopTable;
    std::vector<double> m_capacity;
    std::vector<uint32_t> m_parent;
    bool m_isDirty;
};

} // namespace ns3

#endif // CONTACT_GRAPH_ROUTING_H
