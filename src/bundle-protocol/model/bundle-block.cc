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
#include "bundle-block.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("BundleBlock");

NS_OBJECT_ENSURE_REGISTERED(BundleBlock);
NS_OBJECT_ENSURE_REGISTERED(PrimaryBlock);
NS_OBJECT_ENSURE_REGISTERED(PayloadBlock);

TypeId
BundleBlock::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::BundleBlock").SetParent<Object>().SetGroupName("BundleProtocol");
    return tid;
}

TypeId
PrimaryBlock::GetTypeId()
{
    static TypeId tid = TypeId("ns3::PrimaryBlock")
                            .SetParent<BundleBlock>()
                            .SetGroupName("BundleProtocol")
                            .AddConstructor<PrimaryBlock>();
    return tid;
}

PrimaryBlock::PrimaryBlock()
{
    NS_LOG_FUNCTION(this);
}

Ptr<Packet>
PrimaryBlock::SerializeToPacket() const
{
    NS_LOG_FUNCTION(this);
    Ptr<Packet> p = Create<Packet>();
    p->AddHeader(m_header);
    return p;
}

uint32_t
PrimaryBlock::Deserialize(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this << p);
    p->RemoveHeader(m_header);
    return m_header.GetSerializedSize();
}

PrimaryBlockHeader&
PrimaryBlock::GetHeader()
{
    NS_LOG_FUNCTION(this);
    return m_header;
}

const PrimaryBlockHeader&
PrimaryBlock::GetHeader() const
{
    NS_LOG_FUNCTION(this);
    return m_header;
}

TypeId
PayloadBlock::GetTypeId()
{
    static TypeId tid = TypeId("ns3::PayloadBlock")
                            .SetParent<BundleBlock>()
                            .SetGroupName("BundleProtocol")
                            .AddConstructor<PayloadBlock>();
    return tid;
}

PayloadBlock::PayloadBlock()
    : m_payload(nullptr)
{
    NS_LOG_FUNCTION(this);
}

Ptr<Packet>
PayloadBlock::SerializeToPacket() const
{
    NS_LOG_FUNCTION(this);

    Ptr<Packet> p;
    if (m_payload)
    {
        p = m_payload->Copy();
    }
    else
    {
        p = Create<Packet>();
    }

    p->AddHeader(m_header);
    return p;
}

uint32_t
PayloadBlock::Deserialize(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this << p);

    p->RemoveHeader(m_header);
    uint32_t payloadSize = m_header.GetBlockLength();

    if (payloadSize > 0)
    {
        m_payload = p->CreateFragment(0, payloadSize);
        p->RemoveAtStart(payloadSize);
    }
    else
    {
        m_payload = Create<Packet>();
    }

    return m_header.GetSerializedSize() + payloadSize;
}

uint8_t
PayloadBlock::GetBlockType() const
{
    NS_LOG_FUNCTION(this);
    return m_header.GetBlockType();
}

PayloadBlockHeader&
PayloadBlock::GetHeader()
{
    NS_LOG_FUNCTION(this);
    return m_header;
}

const PayloadBlockHeader&
PayloadBlock::GetHeader() const
{
    NS_LOG_FUNCTION(this);
    return m_header;
}

void
PayloadBlock::SetPayload(Ptr<Packet> payload)
{
    NS_LOG_FUNCTION(this << payload);
    m_payload = payload;
    m_header.SetBlockLength(payload ? payload->GetSize() : 0);
}

Ptr<Packet>
PayloadBlock::GetPayload() const
{
    NS_LOG_FUNCTION(this);
    return m_payload;
}

} // namespace ns3
