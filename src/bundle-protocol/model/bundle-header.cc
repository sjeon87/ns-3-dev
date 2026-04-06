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
NS_OBJECT_ENSURE_REGISTERED(CustodySignal);

PrimaryBlockHeader::PrimaryBlockHeader()
    : m_creationTime(Simulator::Now()),
      m_TTL(Seconds(0))
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
       << " blockLength=" << m_blockLength
       << " destinationSchemeOffset=" << m_destinationSchemeOffset
       << " destinationSSPOffset=" << m_destinationSSPOffset
       << " sourceSchemeOffset=" << m_sourceSchemeOffset
       << " sourceSSPOffset=" << m_sourceSSPOffset
       << " reportToSchemeOffset=" << m_reportToSchemeOffset
       << " reportToSSPOffset=" << m_reportToSSPOffset
       << " custodianSchemeOffset=" << m_custodianSchemeOffset
       << " custodianSSPOffset=" << m_custodianSSPOffset
       << " creationTime=" << m_creationTime.As(Time::S)
       << " seq=" << m_seq
       << " TTL=" << m_TTL.As(Time::S)
       << " dictionaryLength=" << m_dictionaryLength
       << " dict=" << m_dictByteArray << ")";
}

uint32_t
PrimaryBlockHeader::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    // 1  (version)
    // 4  (procFlags)
    // 4  (blockLength)
    // 2+2+2+2+2+2+2+2 = 16  (eight EID offset fields, 2 bytes each)
    // 8  (creationTime)
    // 4  (seq)
    // 8  (TTL)
    // 4  (dictionaryLength)
    // variable (dictionary contents)
    return 1 + 4 + 4 + 16 + 8 + 4 + 8 + 4 + m_dictByteArray.size();
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
    // fragment fields intentionally omitted
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

    if (m_dictionaryLength > 0)
    {
        uint8_t* buf = new uint8_t[m_dictionaryLength];
        i.Read(buf, m_dictionaryLength);
        m_dictByteArray.assign(reinterpret_cast<char*>(buf), m_dictionaryLength);
        delete[] buf;
    }

    return GetSerializedSize();
}

void PrimaryBlockHeader::SetVersion(uint8_t version)       { m_version = version; }
uint8_t PrimaryBlockHeader::GetVersion() const             { return m_version; }

void PrimaryBlockHeader::SetProcFlags(uint32_t flags)      { m_procFlags = flags; }
uint32_t PrimaryBlockHeader::GetProcFlags() const          { return m_procFlags; }

void PrimaryBlockHeader::SetBlockLength(uint32_t length)   { m_blockLength = length; }
uint32_t PrimaryBlockHeader::GetBlockLength() const        { return m_blockLength; }

void PrimaryBlockHeader::SetDestinationSchemeOffset(uint16_t offset) { m_destinationSchemeOffset = offset; }
uint16_t PrimaryBlockHeader::GetDestinationSchemeOffset() const      { return m_destinationSchemeOffset; }

void PrimaryBlockHeader::SetDestinationSSPOffset(uint16_t offset)    { m_destinationSSPOffset = offset; }
uint16_t PrimaryBlockHeader::GetDestinationSSPOffset() const         { return m_destinationSSPOffset; }

void PrimaryBlockHeader::SetSourceSchemeOffset(uint16_t offset)      { m_sourceSchemeOffset = offset; }
uint16_t PrimaryBlockHeader::GetSourceSchemeOffset() const           { return m_sourceSchemeOffset; }

void PrimaryBlockHeader::SetSourceSSPOffset(uint16_t offset)         { m_sourceSSPOffset = offset; }
uint16_t PrimaryBlockHeader::GetSourceSSPOffset() const              { return m_sourceSSPOffset; }

void PrimaryBlockHeader::SetReportToSchemeOffset(uint16_t offset)    { m_reportToSchemeOffset = offset; }
uint16_t PrimaryBlockHeader::GetReportToSchemeOffset() const         { return m_reportToSchemeOffset; }

void PrimaryBlockHeader::SetReportToSSPOffset(uint16_t offset)       { m_reportToSSPOffset = offset; }
uint16_t PrimaryBlockHeader::GetReportToSSPOffset() const            { return m_reportToSSPOffset; }

void PrimaryBlockHeader::SetCustodianSchemeOffset(uint16_t offset)   { m_custodianSchemeOffset = offset; }
uint16_t PrimaryBlockHeader::GetCustodianSchemeOffset() const        { return m_custodianSchemeOffset; }

void PrimaryBlockHeader::SetCustodianSSPOffset(uint16_t offset)      { m_custodianSSPOffset = offset; }
uint16_t PrimaryBlockHeader::GetCustodianSSPOffset() const           { return m_custodianSSPOffset; }

void PrimaryBlockHeader::SetCreationTime(Time t)           { m_creationTime = t; }
Time PrimaryBlockHeader::GetCreationTime() const           { return m_creationTime; }

void PrimaryBlockHeader::SetTTL(Time t)                    { m_TTL = t; }
Time PrimaryBlockHeader::GetTTL() const                    { return m_TTL; }

void PrimaryBlockHeader::SetSequenceNumber(uint32_t seq)   { m_seq = seq; }
uint32_t PrimaryBlockHeader::GetSequenceNumber() const     { return m_seq; }

void PrimaryBlockHeader::SetDictionaryLength(uint32_t len) { m_dictionaryLength = len; }
uint32_t PrimaryBlockHeader::GetDictionaryLength() const   { return m_dictionaryLength; }

void
PrimaryBlockHeader::SetDictionary(const std::string& dict)
{
    m_dictByteArray = dict;
    m_dictionaryLength = dict.size();
}

const std::string&
PrimaryBlockHeader::GetDictionary() const
{
    return m_dictByteArray;
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

void
PrimaryBlockHeader::SetDestinationEID(const std::string& scheme, const std::string& ssp)
{
    NS_LOG_FUNCTION(this << scheme << ssp);
    m_destinationSchemeOffset = m_dictByteArray.size();
    m_dictByteArray += scheme;
    m_dictByteArray += '\0';
    m_destinationSSPOffset = m_dictByteArray.size();
    m_dictByteArray += ssp;
    m_dictByteArray += '\0';
    m_dictionaryLength = m_dictByteArray.size();
}

void
PrimaryBlockHeader::SetSourceEID(const std::string& scheme, const std::string& ssp)
{
    NS_LOG_FUNCTION(this << scheme << ssp);
    m_sourceSchemeOffset = m_dictByteArray.size();
    m_dictByteArray += scheme;
    m_dictByteArray += '\0';
    m_sourceSSPOffset = m_dictByteArray.size();
    m_dictByteArray += ssp;
    m_dictByteArray += '\0';
    m_dictionaryLength = m_dictByteArray.size();
}

void
PrimaryBlockHeader::SetReportToEID(const std::string& scheme, const std::string& ssp)
{
    NS_LOG_FUNCTION(this << scheme << ssp);
    m_reportToSchemeOffset = m_dictByteArray.size();
    m_dictByteArray += scheme;
    m_dictByteArray += '\0';
    m_reportToSSPOffset = m_dictByteArray.size();
    m_dictByteArray += ssp;
    m_dictByteArray += '\0';
    m_dictionaryLength = m_dictByteArray.size();
}

void
PrimaryBlockHeader::SetCustodianEID(const std::string& scheme, const std::string& ssp)
{
    NS_LOG_FUNCTION(this << scheme << ssp);
    m_custodianSchemeOffset = m_dictByteArray.size();
    m_dictByteArray += scheme;
    m_dictByteArray += '\0';
    m_custodianSSPOffset = m_dictByteArray.size();
    m_dictByteArray += ssp;
    m_dictByteArray += '\0';
    m_dictionaryLength = m_dictByteArray.size();
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
    os << "(blockType=" << (uint32_t)m_blockType
       << " procFlags=" << (uint32_t)m_procFlags
       << " blockLength=" << m_blockLength << ")";
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

void PayloadBlockHeader::SetBlockType(uint8_t type)        { m_blockType = type; }
uint8_t PayloadBlockHeader::GetBlockType() const           { return m_blockType; }

void PayloadBlockHeader::SetProcFlags(uint8_t flags)       { m_procFlags = flags; }
uint8_t PayloadBlockHeader::GetProcFlags() const           { return m_procFlags; }

void PayloadBlockHeader::SetBlockLength(uint32_t length)   { m_blockLength = length; }
uint32_t PayloadBlockHeader::GetBlockLength() const        { return m_blockLength; }


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
       << " fragmentOffset=" << m_fragmentOffset
       << " bundleReceipt=" << m_bundleReceipt.As(Time::S)
       << " custodyAccept=" << m_custodyAccept.As(Time::S)
       << " bundleForward=" << m_bundleForward.As(Time::S)
       << " bundleDelivery=" << m_bundleDelivery.As(Time::S)
       << " creationTime=" << m_creationTime.As(Time::S)
       << " seq=" << m_seq
       << " lenSourceEID=" << m_lenSourceEID
       << " sourceID=" << m_sourceID << ")";
}

uint32_t
BundleStatusReport::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    return 1 + 1 + 4 + (8 * 4) + 4 + 4 + 4;
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

void BundleStatusReport::SetStatusFlags(uint8_t flags)         { m_statusFlags = flags; }
uint8_t BundleStatusReport::GetStatusFlags() const             { return m_statusFlags; }

void BundleStatusReport::SetReasonCode(uint8_t code)           { m_reasonCode = code; }
uint8_t BundleStatusReport::GetReasonCode() const              { return m_reasonCode; }

void BundleStatusReport::SetFragmentOffset(uint32_t offset)    { m_fragmentOffset = offset; }
uint32_t BundleStatusReport::GetFragmentOffset() const         { return m_fragmentOffset; }

void BundleStatusReport::SetBundleReceiptTime(Time t)          { m_bundleReceipt = t; }
Time BundleStatusReport::GetBundleReceiptTime() const          { return m_bundleReceipt; }

void BundleStatusReport::SetCustodyAcceptTime(Time t)          { m_custodyAccept = t; }
Time BundleStatusReport::GetCustodyAcceptTime() const          { return m_custodyAccept; }

void BundleStatusReport::SetBundleForwardTime(Time t)          { m_bundleForward = t; }
Time BundleStatusReport::GetBundleForwardTime() const          { return m_bundleForward; }

void BundleStatusReport::SetBundleDeliveryTime(Time t)         { m_bundleDelivery = t; }
Time BundleStatusReport::GetBundleDeliveryTime() const         { return m_bundleDelivery; }

void BundleStatusReport::SetCreationTime(Time t)               { m_creationTime = t; }
Time BundleStatusReport::GetCreationTime() const               { return m_creationTime; }

void BundleStatusReport::SetSequenceNumber(uint32_t seq)       { m_seq = seq; }
uint32_t BundleStatusReport::GetSequenceNumber() const         { return m_seq; }

void BundleStatusReport::SetSourceEIDLength(uint32_t len)      { m_lenSourceEID = len; }
uint32_t BundleStatusReport::GetSourceEIDLength() const        { return m_lenSourceEID; }

void BundleStatusReport::SetSourceID(uint32_t id)              { m_sourceID = id; }
uint32_t BundleStatusReport::GetSourceID() const               { return m_sourceID; }


CustodySignal::CustodySignal()
    : m_creationTime(Simulator::Now())
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
CustodySignal::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
CustodySignal::Print(std::ostream& os) const
{
    NS_LOG_FUNCTION(this << &os);
    os << "(statusFlags=" << (uint32_t)m_statusFlags
       << " fragmentOffset=" << m_fragmentOffset
       << " tos=" << m_tos.As(Time::S)
       << " creationTime=" << m_creationTime.As(Time::S)
       << " seq=" << m_seq
       << " lenSourceEID=" << m_lenSourceEID
       << " sourceID=" << m_sourceID << ")";
}

uint32_t
CustodySignal::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    return 1 + 4 + 8 + 8 + 4 + 4 + 4;
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

void CustodySignal::SetStatusFlags(uint8_t flags)          { m_statusFlags = flags; }
uint8_t CustodySignal::GetStatusFlags() const              { return m_statusFlags; }

void CustodySignal::SetFragmentOffset(uint32_t offset)     { m_fragmentOffset = offset; }
uint32_t CustodySignal::GetFragmentOffset() const          { return m_fragmentOffset; }

void CustodySignal::SetTimeOfSignal(Time t)                { m_tos = t; }
Time CustodySignal::GetTimeOfSignal() const                { return m_tos; }

void CustodySignal::SetCreationTime(Time t)                { m_creationTime = t; }
Time CustodySignal::GetCreationTime() const                { return m_creationTime; }

void CustodySignal::SetSequenceNumber(uint32_t seq)        { m_seq = seq; }
uint32_t CustodySignal::GetSequenceNumber() const          { return m_seq; }

void CustodySignal::SetSourceEIDLength(uint32_t len)       { m_lenSourceEID = len; }
uint32_t CustodySignal::GetSourceEIDLength() const         { return m_lenSourceEID; }

void CustodySignal::SetSourceID(uint32_t id)               { m_sourceID = id; }
uint32_t CustodySignal::GetSourceID() const                { return m_sourceID; }

} // namespace ns3