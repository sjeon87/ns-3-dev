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

#include "cbor.h"

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

    uint8_t crcType = m_header.GetCrcType();
    if (crcType == 1 || crcType == 2)
    {
        uint32_t crcLen = (crcType == 1) ? 2 : 4;
        uint32_t size = p->GetSize();
        auto* buf = new uint8_t[size];
        p->CopyData(buf, size);

        for (uint32_t i = 0; i < crcLen; ++i)
        {
            buf[size - 1 - i] = 0x00;
        }

        if (crcType == 1)
        {
            uint16_t crc = Cbor::ComputeCrc16(buf, size);
            buf[size - 2] = (crc >> 8) & 0xFF;
            buf[size - 1] = crc & 0xFF;
        }
        else
        {
            uint32_t crc = Cbor::ComputeCrc32(buf, size);
            buf[size - 4] = (crc >> 24) & 0xFF;
            buf[size - 3] = (crc >> 16) & 0xFF;
            buf[size - 2] = (crc >> 8) & 0xFF;
            buf[size - 1] = crc & 0xFF;
        }

        Ptr<Packet> patched = Create<Packet>(buf, size);
        delete[] buf;
        return patched;
    }

    return p;
}

uint32_t
PrimaryBlock::Deserialize(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this << p);

    uint32_t available = p->GetSize();
    auto* buf = new uint8_t[available];
    p->CopyData(buf, available);

    p->RemoveHeader(m_header);
    uint32_t consumed = m_header.GetSerializedSize();

    uint8_t crcType = m_header.GetCrcType();
    if (crcType == 1 || crcType == 2)
    {
        uint32_t crcLen = (crcType == 1) ? 2 : 4;
        for (uint32_t i = 0; i < crcLen; ++i)
        {
            buf[consumed - 1 - i] = 0x00;
        }

        uint32_t expectedCrc =
            (crcType == 1) ? Cbor::ComputeCrc16(buf, consumed) : Cbor::ComputeCrc32(buf, consumed);
        if (expectedCrc != m_header.GetReceivedCrc())
        {
            NS_LOG_ERROR("PrimaryBlock CRC mismatch: expected="
                         << expectedCrc << " received=" << m_header.GetReceivedCrc()
                         << " -- bundle may be corrupted in transit");
        }
    }

    delete[] buf;
    return consumed;
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

    uint8_t crcType = m_header.GetCrcType();
    if (crcType == 1 || crcType == 2)
    {
        uint32_t crcLen = (crcType == 1) ? 2 : 4;
        uint32_t crcOffset = m_header.GetSerializedSize() - crcLen;

        uint32_t size = p->GetSize();
        auto* buf = new uint8_t[size];
        p->CopyData(buf, size);

        for (uint32_t i = 0; i < crcLen; ++i)
        {
            buf[crcOffset + i] = 0x00;
        }

        if (crcType == 1)
        {
            uint16_t crc = Cbor::ComputeCrc16(buf, size);
            buf[crcOffset] = (crc >> 8) & 0xFF;
            buf[crcOffset + 1] = crc & 0xFF;
        }
        else
        {
            uint32_t crc = Cbor::ComputeCrc32(buf, size);
            buf[crcOffset] = (crc >> 24) & 0xFF;
            buf[crcOffset + 1] = (crc >> 16) & 0xFF;
            buf[crcOffset + 2] = (crc >> 8) & 0xFF;
            buf[crcOffset + 3] = crc & 0xFF;
        }

        Ptr<Packet> patched = Create<Packet>(buf, size);
        delete[] buf;
        return patched;
    }

    return p;
}

uint32_t
PayloadBlock::Deserialize(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this << p);

    uint32_t available = p->GetSize();
    auto* buf = new uint8_t[available];
    p->CopyData(buf, available);

    Buffer tmp;
    tmp.AddAtEnd(available);
    Buffer::Iterator it = tmp.Begin();
    it.Write(buf, available);

    Buffer::Iterator start = tmp.Begin();
    uint32_t consumed = m_header.Deserialize(start);

    p->RemoveAtStart(consumed);

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

    uint8_t crcType = m_header.GetCrcType();
    if (crcType == 1 || crcType == 2)
    {
        uint32_t crcLen = (crcType == 1) ? 2 : 4;
        uint32_t crcOffset = consumed - crcLen;
        uint32_t blockSize = consumed + payloadSize;

        for (uint32_t i = 0; i < crcLen; ++i)
        {
            buf[crcOffset + i] = 0x00;
        }

        uint32_t expectedCrc = (crcType == 1) ? Cbor::ComputeCrc16(buf, blockSize)
                                              : Cbor::ComputeCrc32(buf, blockSize);
        if (expectedCrc != m_header.GetReceivedCrc())
        {
            NS_LOG_ERROR("PayloadBlock CRC mismatch: expected="
                         << expectedCrc << " received=" << m_header.GetReceivedCrc()
                         << " -- payload may be corrupted in transit");
        }
    }

    delete[] buf;
    return consumed + payloadSize;
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
