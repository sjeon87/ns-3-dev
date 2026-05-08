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
#include "bundle-header.h"

#include "cbor.h"
#include "eid.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("BundleHeader");

NS_OBJECT_ENSURE_REGISTERED(PrimaryBlockHeader);
NS_OBJECT_ENSURE_REGISTERED(PayloadBlockHeader);
NS_OBJECT_ENSURE_REGISTERED(BundleStatusReport);

PrimaryBlockHeader::PrimaryBlockHeader()
    : m_creationTime(Simulator::Now()),
      m_lifetime(Seconds(0))
{
    NS_LOG_FUNCTION(this);
}

TypeId
PrimaryBlockHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::PrimaryBlockHeader")
                            .SetParent<Header>()
                            .SetGroupName("BundleProtocol")
                            .AddConstructor<PrimaryBlockHeader>();
    return tid;
}

TypeId
PrimaryBlockHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
PrimaryBlockHeader::Print(std::ostream& os) const
{
    NS_LOG_FUNCTION(this << &os);
    os << "(version=" << (uint32_t)m_version << " procFlags=" << m_procFlags
       << " crcType=" << (uint32_t)m_crcType << " destinationEID=" << m_destinationEID
       << " sourceEID=" << m_sourceEID << " reportToEID=" << m_reportToEID
       << " creationTime=" << m_creationTime.As(Time::S) << " seq=" << m_seq
       << " lifetime=" << m_lifetime.As(Time::S) << " fragmentOffset=" << m_fragmentOffset
       << " totalAppDataLength=" << m_totalAppDataLength << ")";
}

uint32_t
PrimaryBlockHeader::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    uint32_t size = 0;

    bool hasCrc = (m_crcType != 0);
    uint32_t arrayLen = hasCrc ? 9 : 8;

    size += Cbor::GetArraySize(arrayLen);
    size += Cbor::GetUintSize(m_version);
    size += Cbor::GetUintSize(m_procFlags);
    size += Cbor::GetUintSize(m_crcType);

    size += Eid::GetSize(m_destinationEID);
    size += Eid::GetSize(m_sourceEID);
    size += Eid::GetSize(m_reportToEID);

    size += Cbor::GetArraySize(2);
    size += Cbor::GetUintSize(m_creationTime.GetMilliSeconds());
    size += Cbor::GetUintSize(m_seq);
    size += Cbor::GetUintSize(m_lifetime.GetMilliSeconds());

    if (hasCrc)
    {
        if (m_crcType == 1)
        {
            size += 3;
        }
        else
        {
            size += 5;
        }
    }

    return size;
}

void
PrimaryBlockHeader::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;

    bool hasCrc = (m_crcType != 0);
    uint32_t arrayLen = hasCrc ? 9 : 8;

    Cbor::WriteArray(i, arrayLen);
    Cbor::WriteUint(i, m_version);
    Cbor::WriteUint(i, m_procFlags);
    Cbor::WriteUint(i, m_crcType);

    Eid::Write(i, m_destinationEID);
    Eid::Write(i, m_sourceEID);
    Eid::Write(i, m_reportToEID);

    Cbor::WriteArray(i, 2);
    Cbor::WriteUint(i, m_creationTime.GetMilliSeconds());
    Cbor::WriteUint(i, m_seq);
    Cbor::WriteUint(i, m_lifetime.GetMilliSeconds());

    if (hasCrc)
    {
        if (m_crcType == 1)
        {
            i.WriteU8(0x42);
            i.WriteHtonU16(0x0000);
        }
        else
        {
            i.WriteU8(0x44);
            i.WriteHtonU32(0x00000000);
        }
    }
}

uint32_t
PrimaryBlockHeader::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;

    Cbor::ReadArray(i);
    m_version = Cbor::ReadUint(i);
    m_procFlags = Cbor::ReadUint(i);
    m_crcType = Cbor::ReadUint(i);

    m_destinationEID = Eid::Read(i);
    m_sourceEID = Eid::Read(i);
    m_reportToEID = Eid::Read(i);

    Cbor::ReadArray(i);
    m_creationTime = MilliSeconds(Cbor::ReadUint(i));
    m_seq = Cbor::ReadUint(i);
    m_lifetime = MilliSeconds(Cbor::ReadUint(i));

    if (m_crcType != 0)
    {
        uint8_t crcHeader = i.ReadU8();
        uint8_t crcLen = crcHeader & 0x1F;
        for (uint8_t b = 0; b < crcLen; ++b)
        {
            i.ReadU8();
        }
    }

    return i.GetDistanceFrom(start);
}

void
PrimaryBlockHeader::SetVersion(uint8_t version)
{
    m_version = version;
}

uint8_t
PrimaryBlockHeader::GetVersion() const
{
    return m_version;
}

void
PrimaryBlockHeader::SetProcFlags(uint32_t flags)
{
    m_procFlags = flags;
}

uint32_t
PrimaryBlockHeader::GetProcFlags() const
{
    return m_procFlags;
}

void
PrimaryBlockHeader::SetCrcType(uint8_t crcType)
{
    m_crcType = crcType;
}

uint8_t
PrimaryBlockHeader::GetCrcType() const
{
    return m_crcType;
}

void
PrimaryBlockHeader::SetDestinationEID(const std::string& eid)
{
    m_destinationEID = eid;
}

std::string
PrimaryBlockHeader::GetDestinationEID() const
{
    return m_destinationEID;
}

void
PrimaryBlockHeader::SetSourceEID(const std::string& eid)
{
    m_sourceEID = eid;
}

std::string
PrimaryBlockHeader::GetSourceEID() const
{
    return m_sourceEID;
}

void
PrimaryBlockHeader::SetReportToEID(const std::string& eid)
{
    m_reportToEID = eid;
}

std::string
PrimaryBlockHeader::GetReportToEID() const
{
    return m_reportToEID;
}

void
PrimaryBlockHeader::SetCreationTime(Time t)
{
    m_creationTime = t;
}

Time
PrimaryBlockHeader::GetCreationTime() const
{
    return m_creationTime;
}

void
PrimaryBlockHeader::SetLifetime(Time t)
{
    m_lifetime = t;
}

Time
PrimaryBlockHeader::GetLifetime() const
{
    return m_lifetime;
}

void
PrimaryBlockHeader::SetSequenceNumber(uint32_t seq)
{
    m_seq = seq;
}

uint32_t
PrimaryBlockHeader::GetSequenceNumber() const
{
    return m_seq;
}

void
PrimaryBlockHeader::SetFragmentOffset(uint32_t offset)
{
    m_fragmentOffset = offset;
}

uint32_t
PrimaryBlockHeader::GetFragmentOffset() const
{
    return m_fragmentOffset;
}

void
PrimaryBlockHeader::SetTotalAppDataLength(uint32_t length)
{
    m_totalAppDataLength = length;
}

uint32_t
PrimaryBlockHeader::GetTotalAppDataLength() const
{
    return m_totalAppDataLength;
}

PayloadBlockHeader::PayloadBlockHeader()
{
    NS_LOG_FUNCTION(this);
}

TypeId
PayloadBlockHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::PayloadBlockHeader")
                            .SetParent<Header>()
                            .SetGroupName("BundleProtocol")
                            .AddConstructor<PayloadBlockHeader>();
    return tid;
}

TypeId
PayloadBlockHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
PayloadBlockHeader::Print(std::ostream& os) const
{
    NS_LOG_FUNCTION(this << &os);
    os << "(blockType=" << (uint32_t)m_blockType << " blockNumber=" << m_blockNumber
       << " procFlags=" << (uint32_t)m_procFlags << " crcType=" << (uint32_t)m_crcType
       << " blockLength=" << m_blockLength << ")";
}

uint32_t
PayloadBlockHeader::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    uint32_t size = 0;
    bool hasCrc = (m_crcType != 0) || (m_deserializedArraySize == 6);

    size += Cbor::GetArraySize(hasCrc ? 6 : 5);
    size += Cbor::GetUintSize(m_blockType);
    size += Cbor::GetUintSize(m_blockNumber);
    size += Cbor::GetUintSize(m_procFlags);
    size += Cbor::GetUintSize(m_crcType);
    size += Cbor::GetByteStringHeaderSize(m_blockLength);

    if (hasCrc)
    {
        if (m_crcType == 1)
        {
            size += 3;
        }
        else
        {
            size += 5;
        }
    }

    return size;
}

void
PayloadBlockHeader::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    bool hasCrc = (m_crcType != 0);

    Cbor::WriteArray(i, hasCrc ? 6 : 5);
    Cbor::WriteUint(i, m_blockType);
    Cbor::WriteUint(i, m_blockNumber);
    Cbor::WriteUint(i, m_procFlags);
    Cbor::WriteUint(i, m_crcType);
    Cbor::WriteByteStringHeader(i, m_blockLength);

    if (hasCrc)
    {
        if (m_crcType == 1)
        {
            i.WriteU8(0x42);
            i.WriteHtonU16(0x0000);
        }
        else
        {
            i.WriteU8(0x44);
            i.WriteHtonU32(0x00000000);
        }
    }
}

uint32_t
PayloadBlockHeader::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;

    uint64_t arraySize = Cbor::ReadArray(i);
    m_deserializedArraySize = static_cast<uint8_t>(arraySize);
    m_blockType = Cbor::ReadUint(i);
    m_blockNumber = Cbor::ReadUint(i);
    m_procFlags = Cbor::ReadUint(i);
    m_crcType = Cbor::ReadUint(i);
    m_blockLength = Cbor::ReadByteStringHeader(i);

    if (arraySize == 6)
    {
        uint8_t crcHeader = i.ReadU8();
        uint8_t crcLen = crcHeader & 0x1F;
        for (uint8_t b = 0; b < crcLen; ++b)
        {
            i.ReadU8();
        }
    }

    return i.GetDistanceFrom(start);
}

void
PayloadBlockHeader::SetBlockType(uint8_t type)
{
    m_blockType = type;
}

uint8_t
PayloadBlockHeader::GetBlockType() const
{
    return m_blockType;
}

void
PayloadBlockHeader::SetBlockNumber(uint32_t number)
{
    m_blockNumber = number;
}

uint32_t
PayloadBlockHeader::GetBlockNumber() const
{
    return m_blockNumber;
}

void
PayloadBlockHeader::SetProcFlags(uint8_t flags)
{
    m_procFlags = flags;
}

uint8_t
PayloadBlockHeader::GetProcFlags() const
{
    return m_procFlags;
}

void
PayloadBlockHeader::SetCrcType(uint8_t crcType)
{
    m_crcType = crcType;
}

uint8_t
PayloadBlockHeader::GetCrcType() const
{
    return m_crcType;
}

void
PayloadBlockHeader::SetBlockLength(uint32_t length)
{
    m_blockLength = length;
}

uint32_t
PayloadBlockHeader::GetBlockLength() const
{
    return m_blockLength;
}

BundleStatusReport::BundleStatusReport()
    : m_creationTime(Simulator::Now())
{
    NS_LOG_FUNCTION(this);
}

TypeId
BundleStatusReport::GetTypeId()
{
    static TypeId tid = TypeId("ns3::BundleStatusReport")
                            .SetParent<Header>()
                            .SetGroupName("BundleProtocol")
                            .AddConstructor<BundleStatusReport>();
    return tid;
}

TypeId
BundleStatusReport::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
BundleStatusReport::Print(std::ostream& os) const
{
    NS_LOG_FUNCTION(this << &os);
    os << "(statusFlags=" << (uint32_t)m_statusFlags << " reasonCode=" << (uint32_t)m_reasonCode
       << " sourceEID=" << m_sourceEID << " creationTime=" << m_creationTime.As(Time::S)
       << " seq=" << m_seq << " fragmentOffset=" << m_fragmentOffset
       << " bundleReceipt=" << m_bundleReceipt.As(Time::S)
       << " bundleForward=" << m_bundleForward.As(Time::S)
       << " bundleDelivery=" << m_bundleDelivery.As(Time::S)
       << " bundleDeletion=" << m_bundleDeletion.As(Time::S) << ")";
}

uint32_t
BundleStatusReport::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    uint32_t size = 0;

    size += Cbor::GetArraySize(10);
    size += Cbor::GetUintSize(m_statusFlags);
    size += Cbor::GetUintSize(m_reasonCode);
    size += Cbor::GetTextStringSize(m_sourceEID);

    size += Cbor::GetUintSize(m_creationTime.GetTimeStep());
    size += Cbor::GetUintSize(m_seq);
    size += Cbor::GetUintSize(m_fragmentOffset);

    size += Cbor::GetUintSize(m_bundleReceipt.GetTimeStep());
    size += Cbor::GetUintSize(m_bundleForward.GetTimeStep());
    size += Cbor::GetUintSize(m_bundleDelivery.GetTimeStep());
    size += Cbor::GetUintSize(m_bundleDeletion.GetTimeStep());

    return size;
}

void
BundleStatusReport::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;

    Cbor::WriteArray(i, 10);
    Cbor::WriteUint(i, m_statusFlags);
    Cbor::WriteUint(i, m_reasonCode);
    Cbor::WriteTextString(i, m_sourceEID);

    Cbor::WriteUint(i, m_creationTime.GetTimeStep());
    Cbor::WriteUint(i, m_seq);
    Cbor::WriteUint(i, m_fragmentOffset);

    Cbor::WriteUint(i, m_bundleReceipt.GetTimeStep());
    Cbor::WriteUint(i, m_bundleForward.GetTimeStep());
    Cbor::WriteUint(i, m_bundleDelivery.GetTimeStep());
    Cbor::WriteUint(i, m_bundleDeletion.GetTimeStep());
}

uint32_t
BundleStatusReport::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;

    Cbor::ReadArray(i);

    m_statusFlags = Cbor::ReadUint(i);
    m_reasonCode = Cbor::ReadUint(i);
    m_sourceEID = Cbor::ReadTextString(i);

    m_creationTime = TimeStep(Cbor::ReadUint(i));
    m_seq = Cbor::ReadUint(i);
    m_fragmentOffset = Cbor::ReadUint(i);

    m_bundleReceipt = TimeStep(Cbor::ReadUint(i));
    m_bundleForward = TimeStep(Cbor::ReadUint(i));
    m_bundleDelivery = TimeStep(Cbor::ReadUint(i));
    m_bundleDeletion = TimeStep(Cbor::ReadUint(i));

    return i.GetDistanceFrom(start);
}

void
BundleStatusReport::SetStatusFlags(uint8_t flags)
{
    m_statusFlags = flags;
}

uint8_t
BundleStatusReport::GetStatusFlags() const
{
    return m_statusFlags;
}

void
BundleStatusReport::SetReasonCode(uint8_t code)
{
    m_reasonCode = code;
}

uint8_t
BundleStatusReport::GetReasonCode() const
{
    return m_reasonCode;
}

void
BundleStatusReport::SetSourceEID(const std::string& eid)
{
    m_sourceEID = eid;
}

std::string
BundleStatusReport::GetSourceEID() const
{
    return m_sourceEID;
}

void
BundleStatusReport::SetCreationTime(Time t)
{
    m_creationTime = t;
}

Time
BundleStatusReport::GetCreationTime() const
{
    return m_creationTime;
}

void
BundleStatusReport::SetSequenceNumber(uint32_t seq)
{
    m_seq = seq;
}

uint32_t
BundleStatusReport::GetSequenceNumber() const
{
    return m_seq;
}

void
BundleStatusReport::SetFragmentOffset(uint32_t offset)
{
    m_fragmentOffset = offset;
}

uint32_t
BundleStatusReport::GetFragmentOffset() const
{
    return m_fragmentOffset;
}

void
BundleStatusReport::SetBundleReceiptTime(Time t)
{
    m_bundleReceipt = t;
}

Time
BundleStatusReport::GetBundleReceiptTime() const
{
    return m_bundleReceipt;
}

void
BundleStatusReport::SetBundleForwardTime(Time t)
{
    m_bundleForward = t;
}

Time
BundleStatusReport::GetBundleForwardTime() const
{
    return m_bundleForward;
}

void
BundleStatusReport::SetBundleDeliveryTime(Time t)
{
    m_bundleDelivery = t;
}

Time
BundleStatusReport::GetBundleDeliveryTime() const
{
    return m_bundleDelivery;
}

void
BundleStatusReport::SetBundleDeletionTime(Time t)
{
    m_bundleDeletion = t;
}

Time
BundleStatusReport::GetBundleDeletionTime() const
{
    return m_bundleDeletion;
}

} // namespace ns3
