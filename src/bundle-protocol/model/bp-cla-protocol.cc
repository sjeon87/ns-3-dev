/*
 * Copyright (c) 2013 University of New Brunswick
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 *
 * Author: Dizhi Zhou <dizhi.zhou@gmail.com>
 */

#include "bp-cla-protocol.h"

#include "ns3/log.h"
#include "ns3/socket.h"

NS_LOG_COMPONENT_DEFINE("BpClaProtocol");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(BpClaProtocol);

TypeId
BpClaProtocol::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::BpClaProtocol").SetParent<Object>();
    return tid;
}

BpClaProtocol::BpClaProtocol()
{
    NS_LOG_FUNCTION(this);
}

BpClaProtocol::~BpClaProtocol()
{
    NS_LOG_FUNCTION(this);
}

} // namespace ns3
