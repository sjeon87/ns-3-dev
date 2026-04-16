/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 *
 * Author: Dizhi Zhou <dizhi.zhou@gmail.com>
 */

#include "bundle-protocol-helper.h"

#include "ns3/names.h"
#include "ns3/simulator.h"
#include "ns3/string.h"

namespace ns3
{

BundleProtocolHelper::BundleProtocolHelper()
    : m_eid("dtn:none"),
      m_routingProtocol(nullptr)
{
}

BundleProtocolContainer
BundleProtocolHelper::Install(Ptr<Node> node)
{
    return BundleProtocolContainer(InstallPriv(node));
}

BundleProtocolContainer
BundleProtocolHelper::Install(std::string nodeName)
{
    Ptr<Node> node = Names::Find<Node>(nodeName);
    return BundleProtocolContainer(InstallPriv(node));
}

BundleProtocolContainer
BundleProtocolHelper::Install(NodeContainer c)
{
    BundleProtocolContainer apps;
    for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i)
    {
        apps.Add(InstallPriv(*i));
    }

    return apps;
}

Ptr<BundleProtocol>
BundleProtocolHelper::InstallPriv(Ptr<Node> node)
{
    if (m_eid.Uri() == "dtn:none")
    {
        NS_FATAL_ERROR("BundleProtocolHelper::InstallPriv (): do not have endpoint id!");
    }
    if (m_routingProtocol == nullptr)
    {
        NS_FATAL_ERROR("BundleProtocolHelper::InstallPriv (): do not have bundle routing protocol! "
                       << m_eid.Uri());
    }

    Ptr<BundleProtocol> bundleProtocol = CreateObject<BundleProtocol>();
    bundleProtocol->Open(node);
    bundleProtocol->SetBpEndpointId(m_eid);
    bundleProtocol->SetRoutingProtocol(m_routingProtocol);
    Simulator::Schedule(Seconds(0.0), &BundleProtocol::Initialize, bundleProtocol);

    return bundleProtocol;
}

void
BundleProtocolHelper::SetBpEndpointId(BpEndpointId eid)
{
    m_eid = eid;
}

void
BundleProtocolHelper::SetRoutingProtocol(Ptr<BpRoutingProtocol> rt)
{
    m_routingProtocol = rt;
}

} // namespace ns3
