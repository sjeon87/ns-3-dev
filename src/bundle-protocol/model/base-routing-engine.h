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
#include "ns3/object.h"
#include "ns3/ptr.h"

#include <string>

namespace ns3
{

class RoutingEngine : public Object
{
  public:
    static TypeId GetTypeId();
    RoutingEngine();
    ~RoutingEngine() override;

    virtual std::string GetNextHop(Ptr<Bundle> bundle, const std::string& currEID) = 0;
};

} // namespace ns3

#endif /* BASE_ROUTING_ENGINE_H */