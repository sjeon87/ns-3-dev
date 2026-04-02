/*
 * Copyright (c) 2014 Universitat Autònoma de Barcelona
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 *
 * Author: Rubén Martínez <rmartinez@deic.uab.cat>
 */

#include "ltp-protocol-helper.h"

#include "ns3/enum.h"
#include "ns3/ipv4.h"
#include "ns3/ipv6.h"
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/node-list.h"
#include "ns3/node.h"
#include "ns3/object.h"
#include "ns3/uinteger.h"

NS_LOG_COMPONENT_DEFINE("LtpHelper");

namespace ns3
{
namespace ltp
{

LtpProtocolHelper::LtpProtocolHelper()
    : m_startTime(Seconds(0)),
      m_ltpid(0),
      m_routingProtocol(0)
{
    NS_LOG_FUNCTION(this);
    m_claFactory.SetTypeId(LtpUdpConvergenceLayerAdapter::GetTypeId()); // Default UDP
    m_ltpFactory.SetTypeId(LtpProtocol::GetTypeId());
}

void
LtpProtocolHelper::SetAttributes(std::string n1,
                                 const AttributeValue& v1,
                                 std::string n2,
                                 const AttributeValue& v2,
                                 std::string n3,
                                 const AttributeValue& v3,
                                 std::string n4,
                                 const AttributeValue& v4,
                                 std::string n5,
                                 const AttributeValue& v5,
                                 std::string n6,
                                 const AttributeValue& v6,
                                 std::string n7,
                                 const AttributeValue& v7,
                                 std::string n8,
                                 const AttributeValue& v8,
                                 std::string n9,
                                 const AttributeValue& v9)
{
    m_ltpFactory.Set(n1, v1);
    m_ltpFactory.Set(n2, v2);
    m_ltpFactory.Set(n3, v3);
    m_ltpFactory.Set(n4, v4);
    m_ltpFactory.Set(n5, v5);
    m_ltpFactory.Set(n6, v6);
    m_ltpFactory.Set(n7, v7);
    m_ltpFactory.Set(n8, v8);
    m_ltpFactory.Set(n9, v9);
}

void
LtpProtocolHelper::SetConvergenceLayerAdapter(std::string type,
                                              std::string n1,
                                              const AttributeValue& v1,
                                              std::string n2,
                                              const AttributeValue& v2,
                                              std::string n3,
                                              const AttributeValue& v3,
                                              std::string n4,
                                              const AttributeValue& v4,
                                              std::string n5,
                                              const AttributeValue& v5,
                                              std::string n6,
                                              const AttributeValue& v6,
                                              std::string n7,
                                              const AttributeValue& v7,
                                              std::string n8,
                                              const AttributeValue& v8,
                                              std::string n9,
                                              const AttributeValue& v9)
{
    m_claFactory.SetTypeId(type);
    m_claFactory.Set(n1, v1);
    m_claFactory.Set(n2, v2);
    m_claFactory.Set(n3, v3);
    m_claFactory.Set(n4, v4);
    m_claFactory.Set(n5, v5);
    m_claFactory.Set(n6, v6);
    m_claFactory.Set(n7, v7);
    m_claFactory.Set(n8, v8);
    m_claFactory.Set(n9, v9);
}

void
LtpProtocolHelper::InstallAndLink(NodeContainer c)
{
    NS_LOG_FUNCTION(this);

    uint64_t base_addr = m_ltpid;
    uint32_t numNode = 0;

    for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i)
    {
        if (++numNode < c.GetN())
        {
            m_claFactory.Set("RemotePeer", UintegerValue(m_ltpid + 1));
        }
        else
        {
            m_claFactory.Set("RemotePeer", UintegerValue(base_addr));
        }

        InstallAndLink(*i);
    }
}

void
LtpProtocolHelper::InstallAndLink(Ptr<Node> n)
{
    NS_LOG_FUNCTION(this);
    Ptr<LtpConvergenceLayerAdapter> link =
        m_claFactory.Create()->GetObject<LtpConvergenceLayerAdapter>();
    if (link == nullptr)
    {
        NS_FATAL_ERROR("The requested cla does not exist: \"" << m_claFactory.GetTypeId().GetName()
                                                              << "\"");
    }

    Install(n);
    Ptr<LtpProtocol> ltpProtocol = n->GetObject<LtpProtocol>();

    link->SetProtocol(ltpProtocol);
    ltpProtocol->AddDatalink(link);
    ltpProtocol->SetIpResolutionTable(m_routingProtocol);
    link->EnableReceive(ltpProtocol->GetLocalEngineId());

    ltpProtocol->SetLinkStateCues(link);

    EnumValue<AddressMode> mode(Ipv4);
    m_routingProtocol->GetAttribute("Addressing", mode);

    UintegerValue port = 0;
    link->GetAttribute("ServerPort", port);

    if (mode.Get() == Ipv4)
    {
        Ptr<ns3::Ipv4> ipv4 = n->GetObject<ns3::Ipv4>();
        m_routingProtocol->AddBinding(m_ltpid++, ipv4->GetAddress(1, 0).GetLocal(), port.Get());
    }
    else
    {
        Ptr<ns3::Ipv6> ipv6 = n->GetObject<ns3::Ipv6>();
        m_routingProtocol->AddBinding(m_ltpid++, ipv6->GetAddress(1, 0).GetAddress(), port.Get());
    }
    link->SetLinkUpCallback(MakeCallback(&LtpProtocol::Send, ltpProtocol));

    Simulator::Schedule(m_startTime, &LtpUdpConvergenceLayerAdapter::SetLinkUp, link);
}

void
LtpProtocolHelper::Install(Ptr<Node> node)
{
    NS_LOG_FUNCTION(this);
    if (node->GetObject<LtpProtocol>() != nullptr)
    {
        NS_FATAL_ERROR("LtpProtocolHelper::Install (): Aggregating "
                       "an LtpProtocol to a node with an existing Ltp object");
        return;
    }

    Ptr<LtpProtocol> ltpProtocol = m_ltpFactory.Create()->GetObject<LtpProtocol>();
    ltpProtocol->SetAttribute("LocalEngineId", UintegerValue(m_ltpid));
    ltpProtocol->SetNode(node);

    if (m_routingProtocol == nullptr)
    {
        NS_FATAL_ERROR("LtpProtocolHelper::Install (): "
                       "Routing protocol is not set");
    }

    node->AggregateObject(ltpProtocol);
}

void
LtpProtocolHelper::Install(NodeContainer c)
{
    NS_LOG_FUNCTION(this);
    uint64_t base_addr = m_ltpid;
    uint32_t numNode = 0;

    for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i)
    {
        if (++numNode < c.GetN())
        {
            m_claFactory.Set("RemotePeer", UintegerValue(m_ltpid + 1));
        }
        else
        {
            m_claFactory.Set("RemotePeer", UintegerValue(base_addr));
        }

        Install(*i);
    }
}

void
LtpProtocolHelper::Install(std::string nodeName)
{
    NS_LOG_FUNCTION(this);
    Ptr<Node> node = Names::Find<Node>(nodeName);
    Install(node);
}

void
LtpProtocolHelper::SetBaseLtpEngineId(uint64_t id)
{
    NS_LOG_FUNCTION(this << id);
    m_ltpid = id;
}

void
LtpProtocolHelper::SetLtpEngineId(Ptr<Node> node, uint64_t id)
{
    NS_LOG_FUNCTION(this << node << id);
    Ptr<LtpProtocol> ltpProtocol = node->GetObject<LtpProtocol>();
    ltpProtocol->SetAttribute("LocalEngineId", UintegerValue(id));
}

void
LtpProtocolHelper::SetLtpIpResolutionTable(Ptr<LtpIpResolutionTable> rprot)
{
    NS_LOG_FUNCTION(this << rprot);
    m_routingProtocol = rprot;
}

bool
LtpProtocolHelper::AddBinding(uint64_t dstLtpEngineId, Ipv4Address dstAddr, uint16_t port)
{
    NS_LOG_FUNCTION(this << dstLtpEngineId << dstAddr << port);
    return m_routingProtocol->AddBinding(dstLtpEngineId, dstAddr, port);
}

bool
LtpProtocolHelper::AddBinding(uint64_t dstLtpEngineId, Ipv4Address dstAddr)
{
    NS_LOG_FUNCTION(this << dstLtpEngineId << dstAddr);
    return AddBinding(dstLtpEngineId, dstAddr, 0);
}

void
LtpProtocolHelper::SetStartTransmissionTime(Time start)
{
    NS_LOG_FUNCTION(this << start);
    m_startTime = start;
}

} // namespace ltp
} // namespace ns3
