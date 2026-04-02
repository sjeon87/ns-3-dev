/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */
#include "bundle-header.h"

#include "ns3/header.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("BundleHeader");

NS_OBJECT_ENSURE_REGISTERED("PrimaryBlockHeader");
NS_OBJECT_ENSURE_REGISTERED("PayloadBlockHeader");
NS_OBJECT_ENSURE_REGISTERED("BundleStatusReport");
NS_OBJECT_ENSURE_REGISTERED("CustodySignal");

PrimaryBlockHeader::PrimaryBlockHeader()
    : m_creationTime{Simulator::Now()}
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
PrimaryBlockHeader::GetInstanceType() const
{
    return GetTypeId();
}

void
PrimaryBlockHeader::Print(std::ostream& os) const
{
    NS_LOG_FUNCTION(this << &os);
    os << "(version=" << (uint32_t)m_version << " "
       << "procFlags=" << m_procFlags << " "
       << "blockLength=" << m_blockLength << " "
       << "destinationSchemeOffset=" << m_destinationSchemeOffset << " "
       << "destinationSSPOffset=" << m_destinationSSPOffset << " "
       << "sourceSchemeOffset=" << m_sourceSchemeOffset << " "
       << "sourceSSPOffset=" << m_sourceSSPOffset << " "
       << "reportToSchemeOffset=" << m_reportToSchemeOffset << " "
       << "reportToSSPOffset=" << m_reportToSSPOffset << " "
       << "custodianSchemeOffset=" << m_custodianSchemeOffset << " "
       << "custodianSSPOffset=" << m_custodianSSPOffset << " "
       << "creationTime=" << m_creationTime.As(Time::S) << " "
       << "seq=" << m_seq << " "
       << "TTL=" << m_TTL.As(Time::S) << " "
       << "dictionaryLength=" << m_dictionaryLength << " "
       << "dictByteArray=" << m_dictByteArray << " "
       << "fragmentOffset=" << m_fragmentOffset << " "
       << "totalAppDataLength=" << m_totalAppDataLength << ")";
}

uint32_t
PrimaryBlockHeader::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    return 57 + m_dictByteArray.size();
}

void
PrimaryBlockHeader::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    i.WriteU8(m_version);
    i.WriteHtonU32(m_procFlags);
    i.WriteHtonU32(m_blockLength);
    i.WriteHtonU16(m_destinationSchemeOffset);
    i.WriteHtonU16(m_destinationSSPOffset);
    i.WriteHtonU16(m_sourceSchemeOffset);
    i.WriteHtonU16(m_sourceSSPOffset);
    i.WriteHtonU16(m_reportToSchemeOffset);
    i.WriteHtonU16(m_reportToSSPOffset);
    i.WriteHtonU16(m_custodianSchemeOffset);
    i.WriteHtonU16(m_custodianSSPOffset);
    i.WriteHtonU64(m_creationTime.GetTimeStep());
    i.WriteHtonU32(m_seq);
    i.WriteHtonU64(m_TTL.GetTimeStep());
    i.WriteHtonU32(m_dictionaryLength);
    i.Write(reinterpret_cast<const uint8_t*>(m_dictByteArray.data()), m_dictByteArray.size());
    i.WriteHtonU32(m_fragmentOffset);
    i.WriteHtonU32(m_totalAppDataLength);
}

uint32_t
PrimaryBlockHeader::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    m_version = i.ReadU8();
    m_procFlags = i.ReadNtohU32();
    m_blockLength = i.ReadNtohU32();
    m_destinationSchemeOffset = i.ReadNtohU16();
    m_destinationSSPOffset = i.ReadNtohU16();
    m_sourceSchemeOffset = i.ReadNtohU16();
    m_sourceSSPOffset = i.ReadNtohU16();
    m_reportToSchemeOffset = i.ReadNtohU16();
    m_reportToSSPOffset = i.ReadNtohU16();
    m_custodianSchemeOffset = i.ReadNtohU16();
    m_custodianSSPOffset = i.ReadNtohU16();
    m_creationTime = TimeStep(i.ReadNtohU64());
    m_seq = i.ReadNtohU32();
    m_TTL = TimeStep(i.ReadNtohU64());
    m_dictionaryLength = i.ReadNtohU32();

    uint8_t* buf = new uint8_t[m_dictionaryLength];
    i.Read(buf, m_dictionaryLength);
    m_dictByteArray.assign(reinterpret_cast<char*>(buf), m_dictionaryLength);
    delete[] buf;

    m_fragmentOffset = i.ReadNtohU32();
    m_totalAppDataLength = i.ReadNtohU32();
    return GetSerializedSize();
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
PayloadBlockHeader::GetInstanceType() const
{
    return GetTypeId();
}

void
PayloadBlockHeader::Print(std::ostream& os) const
{
    NS_LOG_FUNCTION(this << &os);
    os << "(blockType=" << (uint32_t)m_blockType << " "
       << "procFlags=" << (uint32_t)m_procFlags << " "
       << "blockLength=" << m_blockLength << ")";
}

uint32_t
PayloadBlockHeader::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    return 6;
}

void
PayloadBlockHeader::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    i.WriteU8(m_blockType);
    i.WriteU8(m_procFlags);
    i.WriteHtonU32(m_blockLength);
}

uint32_t
PayloadBlockHeader::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    m_blockType = i.ReadU8();
    m_procFlags = i.ReadU8();
    m_blockLength = i.ReadNtohU32();
    return GetSerializedSize();
}

BundleStatusReport::BundleStatusReport()
    : m_creationTime{Simulator::Now()}
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
BundleStatusReport::GetInstanceType() const
{
    return GetTypeId();
}

void
BundleStatusReport::Print(std::ostream& os) const
{
    NS_LOG_FUNCTION(this << &os);
    os << "(statusFlags=" << (uint32_t)m_statusFlags << " "
       << "reasonCode=" << (uint32_t)m_reasonCode << " "
       << "fragmentOffset=" << m_fragmentOffset << " "
       << "bundleReceipt=" << m_bundleReceipt.As(Time::S) << " "
       << "custodyAccept=" << m_custodyAccept.As(Time::S) << " "
       << "bundleForward=" << m_bundleForward.As(Time::S) << " "
       << "bundleDelivery=" << m_bundleDelivery.As(Time::S) << " "
       << "creationTime=" << m_creationTime.As(Time::S) << " "
       << "seq=" << m_seq << " "
       << "lenSourceEID=" << m_lenSourceEID << " "
       << "sourceID=" << m_sourceID << ")";
}

uint32_t
BundleStatusReport::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    return 58;
}

void
BundleStatusReport::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    i.WriteU8(m_statusFlags);
    i.WriteU8(m_reasonCode);
    i.WriteHtonU32(m_fragmentOffset);
    i.WriteHtonU64(m_bundleReceipt.GetTimeStep());
    i.WriteHtonU64(m_custodyAccept.GetTimeStep());
    i.WriteHtonU64(m_bundleForward.GetTimeStep());
    i.WriteHtonU64(m_bundleDelivery.GetTimeStep());
    i.WriteHtonU64(m_creationTime.GetTimeStep());
    i.WriteHtonU32(m_seq);
    i.WriteHtonU32(m_lenSourceEID);
    i.WriteHtonU32(m_sourceID);
}

uint32_t
BundleStatusReport::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    m_statusFlags = i.ReadU8();
    m_reasonCode = i.ReadU8();
    m_fragmentOffset = i.ReadNtohU32();
    m_bundleReceipt = TimeStep(i.ReadNtohU64());
    m_custodyAccept = TimeStep(i.ReadNtohU64());
    m_bundleForward = TimeStep(i.ReadNtohU64());
    m_bundleDelivery = TimeStep(i.ReadNtohU64());
    m_creationTime = TimeStep(i.ReadNtohU64());
    m_seq = i.ReadNtohU32();
    m_lenSourceEID = i.ReadNtohU32();
    m_sourceID = i.ReadNtohU32();
    return GetSerializedSize();
}

CustodySignal::CustodySignal()
    : m_creationTime{Simulator::Now()}
{
    NS_LOG_FUNCTION(this);
}

TypeId
CustodySignal::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CustodySignal")
                            .SetParent<Header>()
                            .SetGroupName("BundleProtocol")
                            .AddConstructor<CustodySignal>();
    return tid;
}

TypeId
CustodySignal::GetInstanceType() const
{
    return GetTypeId();
}

void
CustodySignal::Print(std::ostream& os) const
{
    NS_LOG_FUNCTION(this << &os);
    os << "(statusFlags=" << (uint32_t)m_statusFlags << " "
       << "fragmentOffset=" << m_fragmentOffset << " "
       << "tos=" << m_tos.As(Time::S) << " "
       << "creationTime=" << m_creationTime.As(Time::S) << " "
       << "seq=" << m_seq << " "
       << "lenSourceEID=" << m_lenSourceEID << " "
       << "sourceID=" << m_sourceID << ")";
}

uint32_t
CustodySignal::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    return 33;
}

void
CustodySignal::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    i.WriteU8(m_statusFlags);
    i.WriteHtonU32(m_fragmentOffset);
    i.WriteHtonU64(m_tos.GetTimeStep());
    i.WriteHtonU64(m_creationTime.GetTimeStep());
    i.WriteHtonU32(m_seq);
    i.WriteHtonU32(m_lenSourceEID);
    i.WriteHtonU32(m_sourceID);
}

uint32_t
CustodySignal::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    m_statusFlags = i.ReadU8();
    m_fragmentOffset = i.ReadNtohU32();
    m_tos = TimeStep(i.ReadNtohU64());
    m_creationTime = TimeStep(i.ReadNtohU64());
    m_seq = i.ReadNtohU32();
    m_lenSourceEID = i.ReadNtohU32();
    m_sourceID = i.ReadNtohU32();
    return GetSerializedSize();
}

} // namespace ns3
