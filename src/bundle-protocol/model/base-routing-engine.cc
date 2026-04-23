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

#include "base-routing-engine.h"
#include "bundle.h"
#include "ns3/object.h"
#include "ns3/ptr.h"

#include <string>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RoutingEngine");

TypeId
RoutingEngine::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RoutingEngine")
                            .SetParent<Object>()
                            .SetGroupName("BundleProtocol");
    return tid;
}

RoutingEngine::RoutingEngine()
{
    NS_LOG_FUNCTION(this);
}

RoutingEngine::~RoutingEngine()
{
    NS_LOG_FUNCTION(this);
}

} // namespace ns3