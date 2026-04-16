/*
 * Copyright (c) 2013 University of New Brunswick
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 *
 * Author: Dizhi Zhou <dizhi.zhou@gmail.com>
 */

#include "bp-routing-protocol.h"

#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE("BpRoutingProtocol");

namespace ns3
{

TypeId
BpRoutingProtocol::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::BpRoutingProtocol").SetParent<Object>();
    return tid;
}

BpRoutingProtocol::BpRoutingProtocol()
{
    NS_LOG_FUNCTION(this);
}

BpRoutingProtocol::~BpRoutingProtocol()
{
    NS_LOG_FUNCTION(this);
}

} // namespace ns3
