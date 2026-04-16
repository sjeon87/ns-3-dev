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
#include <string>
#include <vector>

namespace ns3
{

struct ContactEdge
{
    uint32_t toNode;
    uint32_t dataRate;
};

struct ContactWindow
{
    std::string fromEID;
    std::string toEID;
    Time startTime;
    Time endTime;
    uint32_t dataRate;
    Time delay;
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

    void AddTimedContact(const std::string& fromEID,
                         const std::string& toEID,
                         Time startTime,
                         Time endTime,
                         uint32_t dataRate,
                         Time delay);
    const std::vector<ContactWindow>& GetContactWindows() const;

    std::string GetNextHop(Ptr<Bundle> bundle, const std::string& currEID);
    
    uint32_t FindIndex(const std::string& eid) const;

  private:

    std::vector<std::string> m_eidList;
    std::vector<std::vector<ContactEdge>> m_adjList;
    std::vector<ContactWindow> m_contactWindows;
    uint32_t m_size;
};

} // namespace ns3
#endif /* CONTACT_GRAPH_ROUTING_H */
