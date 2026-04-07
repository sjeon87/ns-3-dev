/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */
#include "bundle-header.h"

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
    os << "(version=" << (uint32_t)m_version 
       << " procFlags=" << m_procFlags
       << " crcType=" << (uint32_t)m_crcType 
       << " destinationEID=" << m_destinationEID
       << " sourceEID=" << m_sourceEID 
       << " reportToEID=" << m_reportToEID
       << " creationTime=" << m_creationTime.As(Time::S) 
       << " seq=" << m_seq
       << " lifetime=" << m_lifetime.As(Time::S) 
       << " fragmentOffset=" << m_fragmentOffset
       << " totalAppDataLength=" << m_totalAppDataLength << ")";
}

uint32_t
PrimaryBlockHeader::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    uint32_t size = 34;
    size += 4 + m_destinationEID.size();
    size += 4 + m_sourceEID.size();
    size += 4 + m_reportToEID.size();
    return size;
}

void
PrimaryBlockHeader::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    i.WriteU8(m_version);
    i.WriteHtonU32(m_procFlags);
    i.WriteU8(m_crcType);
    i.WriteHtonU32(m_destinationEID.size());
    i.Write(reinterpret_cast<const uint8_t*>(m_destinationEID.data()), m_destinationEID.size());
    i.WriteHtonU32(m_sourceEID.size());
    i.Write(reinterpret_cast<const uint8_t*>(m_sourceEID.data()), m_sourceEID.size());
    i.WriteHtonU32(m_reportToEID.size());
    i.Write(reinterpret_cast<const uint8_t*>(m_reportToEID.data()), m_reportToEID.size());
    i.WriteHtonU64(m_creationTime.GetTimeStep());
    i.WriteHtonU32(m_seq);
    i.WriteHtonU64(m_lifetime.GetTimeStep());
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
    m_crcType = i.ReadU8();

    uint32_t destLen = i.ReadNtohU32();
    if (destLen > 0)
    {
        uint8_t* buf = new uint8_t[destLen];
        i.Read(buf, destLen);
        m_destinationEID.assign(reinterpret_cast<char*>(buf), destLen);
        delete[] buf;
    }

    uint32_t srcLen = i.ReadNtohU32();
    if (srcLen > 0)
    {
        uint8_t* buf = new uint8_t[srcLen];
        i.Read(buf, srcLen);
        m_sourceEID.assign(reinterpret_cast<char*>(buf), srcLen);
        delete[] buf;
    }

    uint32_t repLen = i.ReadNtohU32();
    if (repLen > 0)
    {
        uint8_t* buf = new uint8_t[repLen];
        i.Read(buf, repLen);
        m_reportToEID.assign(reinterpret_cast<char*>(buf), repLen);
        delete[] buf;
    }

    m_creationTime = TimeStep(i.ReadNtohU64());
    m_seq = i.ReadNtohU32();
    m_lifetime = TimeStep(i.ReadNtohU64());
    m_fragmentOffset = i.ReadNtohU32();
    m_totalAppDataLength = i.ReadNtohU32();

    return GetSerializedSize();
}

void PrimaryBlockHeader::SetVersion(uint8_t version) { m_version = version; }
uint8_t PrimaryBlockHeader::GetVersion() const { return m_version; }

void PrimaryBlockHeader::SetProcFlags(uint32_t flags) { m_procFlags = flags; }
uint32_t PrimaryBlockHeader::GetProcFlags() const { return m_procFlags; }

void PrimaryBlockHeader::SetCrcType(uint8_t crcType) { m_crcType = crcType; }
uint8_t PrimaryBlockHeader::GetCrcType() const { return m_crcType; }

void PrimaryBlockHeader::SetDestinationEID(const std::string& eid) { m_destinationEID = eid; }
std::string PrimaryBlockHeader::GetDestinationEID() const { return m_destinationEID; }

void PrimaryBlockHeader::SetSourceEID(const std::string& eid) { m_sourceEID = eid; }
std::string PrimaryBlockHeader::GetSourceEID() const { return m_sourceEID; }

void PrimaryBlockHeader::SetReportToEID(const std::string& eid) { m_reportToEID = eid; }
std::string PrimaryBlockHeader::GetReportToEID() const { return m_reportToEID; }

void PrimaryBlockHeader::SetCreationTime(Time t) { m_creationTime = t; }
Time PrimaryBlockHeader::GetCreationTime() const { return m_creationTime; }

void PrimaryBlockHeader::SetLifetime(Time t) { m_lifetime = t; }
Time PrimaryBlockHeader::GetLifetime() const { return m_lifetime; }

void PrimaryBlockHeader::SetSequenceNumber(uint32_t seq) { m_seq = seq; }
uint32_t PrimaryBlockHeader::GetSequenceNumber() const { return m_seq; }

void PrimaryBlockHeader::SetFragmentOffset(uint32_t offset) { m_fragmentOffset = offset; }
uint32_t PrimaryBlockHeader::GetFragmentOffset() const { return m_fragmentOffset; }

void PrimaryBlockHeader::SetTotalAppDataLength(uint32_t length) { m_totalAppDataLength = length; }
uint32_t PrimaryBlockHeader::GetTotalAppDataLength() const { return m_totalAppDataLength; }


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
    os << "(blockType=" << (uint32_t)m_blockType 
       << " blockNumber=" << m_blockNumber
       << " procFlags=" << (uint32_t)m_procFlags
       << " crcType=" << (uint32_t)m_crcType
       << " blockLength=" << m_blockLength << ")";
}

uint32_t
PayloadBlockHeader::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    return 11;
}

void
PayloadBlockHeader::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    i.WriteU8(m_blockType);
    i.WriteHtonU32(m_blockNumber);
    i.WriteU8(m_procFlags);
    i.WriteU8(m_crcType);
    i.WriteHtonU32(m_blockLength);
}

uint32_t
PayloadBlockHeader::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    m_blockType = i.ReadU8();
    m_blockNumber = i.ReadNtohU32();
    m_procFlags = i.ReadU8();
    m_crcType = i.ReadU8();
    m_blockLength = i.ReadNtohU32();
    return GetSerializedSize();
}

void PayloadBlockHeader::SetBlockType(uint8_t type) { m_blockType = type; }
uint8_t PayloadBlockHeader::GetBlockType() const { return m_blockType; }

void PayloadBlockHeader::SetBlockNumber(uint32_t number) { m_blockNumber = number; }
uint32_t PayloadBlockHeader::GetBlockNumber() const { return m_blockNumber; }

void PayloadBlockHeader::SetProcFlags(uint8_t flags) { m_procFlags = flags; }
uint8_t PayloadBlockHeader::GetProcFlags() const { return m_procFlags; }

void PayloadBlockHeader::SetCrcType(uint8_t crcType) { m_crcType = crcType; }
uint8_t PayloadBlockHeader::GetCrcType() const { return m_crcType; }

void PayloadBlockHeader::SetBlockLength(uint32_t length) { m_blockLength = length; }
uint32_t PayloadBlockHeader::GetBlockLength() const { return m_blockLength; }

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
    os << "(statusFlags=" << (uint32_t)m_statusFlags 
       << " reasonCode=" << (uint32_t)m_reasonCode
       << " sourceEID=" << m_sourceEID
       << " creationTime=" << m_creationTime.As(Time::S) 
       << " seq=" << m_seq
       << " fragmentOffset=" << m_fragmentOffset 
       << " bundleReceipt=" << m_bundleReceipt.As(Time::S)
       << " bundleForward=" << m_bundleForward.As(Time::S)
       << " bundleDelivery=" << m_bundleDelivery.As(Time::S)
       << " bundleDeletion=" << m_bundleDeletion.As(Time::S) << ")";
}

uint32_t
BundleStatusReport::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    // Flags(1) + Reason(1) + Creation(8) + Seq(4) + FragOff(4) + 4xTimestamps(32) = 50 bytes
    // String size: 4 bytes for length prefix + actual string length
    uint32_t size = 50;
    size += 4 + m_sourceEID.size();
    return size;
}

void
BundleStatusReport::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    i.WriteU8(m_statusFlags);
    i.WriteU8(m_reasonCode);

    i.WriteHtonU32(m_sourceEID.size());
    i.Write(reinterpret_cast<const uint8_t*>(m_sourceEID.data()), m_sourceEID.size());

    i.WriteHtonU64(m_creationTime.GetTimeStep());
    i.WriteHtonU32(m_seq);
    i.WriteHtonU32(m_fragmentOffset);
    i.WriteHtonU64(m_bundleReceipt.GetTimeStep());
    i.WriteHtonU64(m_bundleForward.GetTimeStep());
    i.WriteHtonU64(m_bundleDelivery.GetTimeStep());
    i.WriteHtonU64(m_bundleDeletion.GetTimeStep());
}

uint32_t
BundleStatusReport::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    m_statusFlags = i.ReadU8();
    m_reasonCode = i.ReadU8();

    uint32_t srcLen = i.ReadNtohU32();
    if (srcLen > 0)
    {
        uint8_t* buf = new uint8_t[srcLen];
        i.Read(buf, srcLen);
        m_sourceEID.assign(reinterpret_cast<char*>(buf), srcLen);
        delete[] buf;
    }

    m_creationTime = TimeStep(i.ReadNtohU64());
    m_seq = i.ReadNtohU32();
    m_fragmentOffset = i.ReadNtohU32();
    m_bundleReceipt = TimeStep(i.ReadNtohU64());
    m_bundleForward = TimeStep(i.ReadNtohU64());
    m_bundleDelivery = TimeStep(i.ReadNtohU64());
    m_bundleDeletion = TimeStep(i.ReadNtohU64());

    return GetSerializedSize();
}

void BundleStatusReport::SetStatusFlags(uint8_t flags) { m_statusFlags = flags; }
uint8_t BundleStatusReport::GetStatusFlags() const { return m_statusFlags; }

void BundleStatusReport::SetReasonCode(uint8_t code) { m_reasonCode = code; }
uint8_t BundleStatusReport::GetReasonCode() const { return m_reasonCode; }

void BundleStatusReport::SetSourceEID(const std::string& eid) { m_sourceEID = eid; }
std::string BundleStatusReport::GetSourceEID() const { return m_sourceEID; }

void BundleStatusReport::SetCreationTime(Time t) { m_creationTime = t; }
Time BundleStatusReport::GetCreationTime() const { return m_creationTime; }

void BundleStatusReport::SetSequenceNumber(uint32_t seq) { m_seq = seq; }
uint32_t BundleStatusReport::GetSequenceNumber() const { return m_seq; }

void BundleStatusReport::SetFragmentOffset(uint32_t offset) { m_fragmentOffset = offset; }
uint32_t BundleStatusReport::GetFragmentOffset() const { return m_fragmentOffset; }

void BundleStatusReport::SetBundleReceiptTime(Time t) { m_bundleReceipt = t; }
Time BundleStatusReport::GetBundleReceiptTime() const { return m_bundleReceipt; }

void BundleStatusReport::SetBundleForwardTime(Time t) { m_bundleForward = t; }
Time BundleStatusReport::GetBundleForwardTime() const { return m_bundleForward; }

void BundleStatusReport::SetBundleDeliveryTime(Time t) { m_bundleDelivery = t; }
Time BundleStatusReport::GetBundleDeliveryTime() const { return m_bundleDelivery; }

void BundleStatusReport::SetBundleDeletionTime(Time t) { m_bundleDeletion = t; }
Time BundleStatusReport::GetBundleDeletionTime() const { return m_bundleDeletion; }

} // namespace ns3