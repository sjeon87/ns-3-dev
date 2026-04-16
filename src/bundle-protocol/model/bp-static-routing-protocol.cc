/*
 * Copyright (c) 2013 University of New Brunswick
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 *
 * Author: Dizhi Zhou <dizhi.zhou@gmail.com>
 */

#include "bp-static-routing-protocol.h"

#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE("BpStaticRoutingProtocol");

namespace ns3
{

TypeId
BpStaticRoutingProtocol::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::BpStaticRoutingProtocol")
                            .SetParent<BpRoutingProtocol>()
                            .AddConstructor<BpStaticRoutingProtocol>();
    return tid;
}

BpStaticRoutingProtocol::BpStaticRoutingProtocol()
    : m_bp(0)
{
    NS_LOG_FUNCTION(this);
}

BpStaticRoutingProtocol::~BpStaticRoutingProtocol()
{
    NS_LOG_FUNCTION(this);
}

void
BpStaticRoutingProtocol::SetBundleProtocol(Ptr<BundleProtocol> bundleProtocol)
{
    NS_LOG_FUNCTION(this << " " << bundleProtocol);
    m_bp = bundleProtocol;
}

int
BpStaticRoutingProtocol::AddRoute(BpEndpointId eid, InetSocketAddress address)
{
    NS_LOG_FUNCTION(this << " " << eid.Uri() << " " << address.GetIpv4() << " "
                         << address.GetPort());
    std::map<BpEndpointId, InetSocketAddress>::iterator it = m_routeMap.end();
    it = m_routeMap.find(eid);
    if (it == m_routeMap.end())
    {
        m_routeMap.insert(std::pair<BpEndpointId, InetSocketAddress>(eid, address));
    }
    else
    {
        // duplicate routing
        return -1;
    }

    return 0;
}

InetSocketAddress
BpStaticRoutingProtocol::GetRoute(BpEndpointId eid)
{
    NS_LOG_FUNCTION(this << " " << eid.Uri());
    std::map<BpEndpointId, InetSocketAddress>::iterator it = m_routeMap.end();
    it = m_routeMap.find(eid);
    if (it == m_routeMap.end())
    {
        InetSocketAddress address("127.0.0.1", 0);
        return address;
    }
    else
    {
        return (*it).second;
    }
}

} // namespace ns3
