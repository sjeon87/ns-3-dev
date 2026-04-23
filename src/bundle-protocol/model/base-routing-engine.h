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

#ifndef BASE_ROUTING_ENGINE_H
#define BASE_ROUTING_ENGINE_H

#include "bundle.h"

#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/ptr.h"

#include <string>
#include <vector>

namespace ns3
{

struct ContactWindow
{
    std::string fromEID;
    std::string toEID;
    Time startTime;
    Time endTime;
    uint32_t dataRate;
    Time delay;
};

class BaseRoutingEngine : public Object
{
  public:
    static TypeId GetTypeId();
    BaseRoutingEngine();
    ~BaseRoutingEngine() override;

    virtual std::string GetNextHop(Ptr<Bundle> bundle, const std::string& currEID) = 0;
    virtual void InitializeMap(const std::vector<std::string>& eidList) = 0;
    virtual void AddContact(const std::string& fromEID,
                            const std::string& toEID,
                            uint32_t dataRate) = 0;
    virtual void RemoveContact(const std::string& fromEID, const std::string& toEID) = 0;

    void AddTimedContact(const std::string& fromEID,
                         const std::string& toEID,
                         Time startTime,
                         Time endTime,
                         uint32_t dataRate,
                         Time delay);

    const std::vector<ContactWindow>& GetContactWindows() const;

  protected:
    std::vector<ContactWindow> m_contactWindows;
};

} // namespace ns3

#endif /* BASE_ROUTING_ENGINE_H */
