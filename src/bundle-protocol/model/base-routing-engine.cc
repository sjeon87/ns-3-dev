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

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("BaseRoutingEngine");

TypeId
BaseRoutingEngine::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::BaseRoutingEngine").SetParent<Object>().SetGroupName("BundleProtocol");
    return tid;
}

BaseRoutingEngine::BaseRoutingEngine()
{
}

BaseRoutingEngine::~BaseRoutingEngine()
{
}

void
BaseRoutingEngine::AddTimedContact(const std::string& fromEID,
                                   const std::string& toEID,
                                   Time startTime,
                                   Time endTime,
                                   uint32_t dataRate,
                                   Time delay)
{
    m_contactWindows.push_back({fromEID, toEID, startTime, endTime, dataRate, delay});
}

const std::vector<ContactWindow>&
BaseRoutingEngine::GetContactWindows() const
{
    return m_contactWindows;
}

} // namespace ns3
