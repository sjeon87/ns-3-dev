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

#include "bundle.h"

#include "ns3/nstime.h"
#include "ns3/object.h"

#include <limits>
#include <map>
#include <string>
#include <vector>

namespace ns3
{

struct ContactEdge
{
    uint32_t toNode;
    uint32_t dataRate;
};

class ContactGraphParser : public Object
{
};

class ContactGraph : public Object
{
  public:
    static TypeId GetTypeId();

    ContactGraph();
    ~ContactGraph() override;

    void InitializeMap(const std::vector<std::string>& eidList);

    void AddContact(const std::string& fromEID, const std::string& toEID, uint32_t dataRate);

    void RemoveContact(const std::string& fromEID, const std::string& toEID);

    std::string GetNextHop(Ptr<Bundle> bundle, const std::string& currEID);

  private:
    std::vector<std::vector<ContactEdge>> m_adjList;
    std::map<std::string, uint32_t> m_nodeMap;
    std::map<uint32_t, std::string> m_reverseNodeMap;
    uint32_t m_size;
};

} // namespace ns3

#endif /* CONTACT_GRAPH_ROUTING_H */
