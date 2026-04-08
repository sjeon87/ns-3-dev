/*
 * Copyright (c) 2014 Universitat Autònoma de Barcelona
 * 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Rubén Martínez <rmartinez@deic.uab.cat>
 * Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ltp-convergence-layer-adapter.h"

#include "ns3/ipv4-header.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/udp-header.h"
#include "ns3/udp-socket-factory.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("LtpBundleCla");

NS_OBJECT_ENSURE_REGISTERED(LtpQueueSet);
NS_OBJECT_ENSURE_REGISTERED(ClientServiceStatus);
NS_OBJECT_ENSURE_REGISTERED(SessionStateRecord);
NS_OBJECT_ENSURE_REGISTERED(SenderSessionStateRecord);
NS_OBJECT_ENSURE_REGISTERED(ReceiverSessionStateRecord);
NS_OBJECT_ENSURE_REGISTERED(LtpBundleCla);

TypeId
LtpQueueSet::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::LtpQueueSet").SetParent<Object>();
    return tid;
}

LtpQueueSet::LtpQueueSet()
    : Object(),
      m_internalOps(),
      m_appData()
{
    NS_LOG_FUNCTION(this);
}

LtpQueueSet::~LtpQueueSet()
{
    NS_LOG_FUNCTION(this);
}

Ptr<Packet>
LtpQueueSet::Dequeue(void)
{
    NS_LOG_FUNCTION(this);
    Ptr<Packet> p;
    if (!m_internalOps.empty())
    {
        p = m_internalOps.front();
        m_internalOps.pop();
    }
    else if (!m_appData.empty())
    {
        p = m_appData.front();
        m_appData.pop();
    }
    else
    {
        NS_LOG_LOGIC("QueueSet is empty");
        return nullptr;
    }
    return p;
}

bool
LtpQueueSet::Enqueue(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this);
    LtpHeader header;
    p->PeekHeader(header);
    SegmentType type = header.GetSegmentType();
    switch (type)
    {
    case LTPTYPE_RD:
    case LTPTYPE_RD_CP:
    case LTPTYPE_RD_CP_EORP:
    case LTPTYPE_RD_CP_EORP_EOB:
    case LTPTYPE_GD:
    case LTPTYPE_GD_EOB:
        m_appData.push(p);
        break;
    case LTPTYPE_RS:
    case LTPTYPE_RAS:
    case LTPTYPE_CS:
    case LTPTYPE_CAS:
    case LTPTYPE_CR:
    case LTPTYPE_CAR:
        m_internalOps.push(p);
        break;
    default:
        NS_LOG_LOGIC("Unexpected Segment type");
        return false;
    }
    return true;
}

Ptr<const Packet>
LtpQueueSet::Peek(void) const
{
    NS_LOG_FUNCTION(this);
    if (!m_internalOps.empty())
    {
        return m_internalOps.front();
    }
    else if (!m_appData.empty())
    {
        return m_appData.front();
    }
    NS_LOG_LOGIC("QueueSet is empty");
    return nullptr;
}

Ptr<Packet>
LtpQueueSet::Remove(void)
{
    NS_LOG_FUNCTION(this);
    return Dequeue();
}

uint32_t
LtpQueueSet::GetNPackets(void) const
{
    return m_internalOps.size() + m_appData.size();
}

TypeId
ClientServiceStatus::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::ClientServiceStatus")
            .SetParent<Object>()
            .AddConstructor<ClientServiceStatus>()
            .AddTraceSource("SessionStatus",
                            "Trace used to report changes in session status",
                            MakeTraceSourceAccessor(&ClientServiceStatus::m_reportStatus),
                            "ns3::TracedCallback<...>");
    return tid;
}

ClientServiceStatus::ClientServiceStatus()
    : m_activeSessions(),
      m_reportStatus()
{
    NS_LOG_FUNCTION(this);
}

ClientServiceStatus::~ClientServiceStatus()
{
    NS_LOG_FUNCTION(this);
}

void
ClientServiceStatus::ReportStatus(SessionId id,
                                  StatusNotificationCode code,
                                  std::vector<uint8_t> data,
                                  uint32_t dataLength,
                                  bool endFlag,
                                  Address srcLtpEngine,
                                  uint32_t offset)
{
    NS_LOG_FUNCTION(this << id << code);
    m_reportStatus(id, code, data, dataLength, endFlag, srcLtpEngine, offset);
}

void
ClientServiceStatus::ReportCancelStatus(SessionId id, StatusNotificationCode code, CxReasonCode cx)
{
    NS_LOG_FUNCTION(this << id << code << cx);
}

void
ClientServiceStatus::AddSession(SessionId id)
{
    NS_LOG_FUNCTION(this << id);
    m_activeSessions.insert(m_activeSessions.begin(), id);
}

void
ClientServiceStatus::ClearSessions()
{
    NS_LOG_FUNCTION(this);
    m_activeSessions.clear();
}

uint32_t
ClientServiceStatus::GetNSessions() const
{
    NS_LOG_FUNCTION(this);
    return m_activeSessions.size();
}

SessionId
ClientServiceStatus::GetSession(uint32_t index)
{
    NS_LOG_FUNCTION(this << index);
    return m_activeSessions.at(index);
}

TypeId
SessionStateRecord::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::SessionStateRecord")
                            .SetParent<Object>()
                            .SetGroupName("BundleProtocol")
                            .AddConstructor<SessionStateRecord>();
    return tid;
}

SessionStateRecord::SessionStateRecord()
    : m_sessionId(),
      m_txQueue(),
      m_localLtpEngine(),
      m_peerLtpEngine(),
      m_localClientService(0),
      m_CpTimer(Timer::CANCEL_ON_DESTROY),
      m_RsTimer(Timer::CANCEL_ON_DESTROY),
      m_CxTimer(Timer::CANCEL_ON_DESTROY),
      m_firstCpSerialNumber(0),
      m_currentCpSerialNumber(0),
      m_firstRpSerialNumber(0),
      m_currentRpSerialNumber(0),
      m_rcvSegments(),
      m_redpartSuccess(false),
      m_blockSuccess(false),
      m_fullRedData(false),
      m_fullGreenData(false),
      m_redPartLength(0),
      m_lowBound(0),
      m_highBound(0),
      m_rTxCnt(0),
      m_canceled(NOT_CANCELED),
      m_canceledReason((CxReasonCode)0),
      m_suspended(false)
{
    NS_LOG_FUNCTION(this);
}

SessionStateRecord::SessionStateRecord(Address localLtpEngine,
                                       uint64_t localClientServiceId,
                                       Address peerLtpEngine)
    : m_sessionId(),
      m_txQueue(),
      m_localLtpEngine(localLtpEngine),
      m_peerLtpEngine(peerLtpEngine),
      m_localClientService(localClientServiceId),
      m_CpTimer(Timer::CANCEL_ON_DESTROY),
      m_RsTimer(Timer::CANCEL_ON_DESTROY),
      m_CxTimer(Timer::CANCEL_ON_DESTROY),
      m_firstCpSerialNumber(0),
      m_currentCpSerialNumber(0),
      m_firstRpSerialNumber(0),
      m_currentRpSerialNumber(0),
      m_rcvSegments(),
      m_redpartSuccess(false),
      m_blockSuccess(false),
      m_fullRedData(false),
      m_fullGreenData(false),
      m_redPartLength(0),
      m_lowBound(0),
      m_highBound(0),
      m_rTxCnt(0),
      m_canceled(NOT_CANCELED),
      m_canceledReason((CxReasonCode)0),
      m_suspended(false)
{
    NS_LOG_FUNCTION(this << localLtpEngine << localClientServiceId << peerLtpEngine);
}

SessionStateRecord::~SessionStateRecord()
{
    NS_LOG_FUNCTION(this);
}

void
SessionStateRecord::StartTimer(TimerCode type)
{
    NS_LOG_FUNCTION(this << type);
    switch (type)
    {
    case CHECKPOINT:
        m_CpTimer.Cancel();
        m_CpTimer.Schedule();
        break;
    case REPORT:
        m_RsTimer.Cancel();
        m_RsTimer.Schedule();
        break;
    case CANCEL:
        m_CxTimer.Cancel();
        m_CxTimer.Schedule();
        break;
    default:
        break;
    }
}

void
SessionStateRecord::CancelTimer(TimerCode type)
{
    NS_LOG_FUNCTION(this << type);

    switch (type)
    {
    case CHECKPOINT:
        m_CpTimer.Cancel();
        break;
    case REPORT:
        m_RsTimer.Cancel();
        break;
    case CANCEL:
        m_CxTimer.Cancel();
        break;
    default:
        break;
    }
}

void
SessionStateRecord::SuspendTimer(TimerCode type)
{
    NS_LOG_FUNCTION(this << type);
    switch (type)
    {
    case CHECKPOINT:
        m_CpTimer.Suspend();
        break;
    case REPORT:
        m_RsTimer.Suspend();
        break;
    case CANCEL:
        m_CxTimer.Suspend();
        break;
    default:
        break;
    }
}

void
SessionStateRecord::ResumeTimer(TimerCode type)
{
    NS_LOG_FUNCTION(this << type);
    switch (type)
    {
    case CHECKPOINT:
        m_CpTimer.Resume();
        break;
    case REPORT:
        m_RsTimer.Resume();
        break;
    case CANCEL:
        m_CxTimer.Resume();
        break;
    default:
        break;
    }
}

bool
SessionStateRecord::Enqueue(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this << p);
    return m_txQueue.Enqueue(p);
}

Ptr<Packet>
SessionStateRecord::Dequeue(void)
{
    NS_LOG_FUNCTION(this);
    return m_txQueue.Dequeue();
}

uint32_t
SessionStateRecord::GetNPackets(void) const
{
    NS_LOG_FUNCTION(this);
    return m_txQueue.GetNPackets();
}

void
SessionStateRecord::SetCpStartSerialNumber(uint64_t serialNum)
{
    NS_LOG_FUNCTION(this << serialNum);
    if (m_firstCpSerialNumber == 0)
    {
        m_firstCpSerialNumber = serialNum;

        if (m_currentCpSerialNumber == 0)
        {
            m_currentCpSerialNumber = serialNum;
        }
    }
}

void
SessionStateRecord::SetRpStartSerialNumber(uint64_t serialNum)
{
    NS_LOG_FUNCTION(this << serialNum);
    if (m_firstCpSerialNumber == 0)
    {
        m_firstRpSerialNumber = serialNum;
    }
}

bool
SessionStateRecord::IsSuspended() const
{
    NS_LOG_FUNCTION(this);
    return m_suspended;
}

void
SessionStateRecord::Suspend()
{
    NS_LOG_FUNCTION(this);
    m_suspended = true;
}

void
SessionStateRecord::Resume()
{
    NS_LOG_FUNCTION(this);
    m_suspended = false;
}

bool
SessionStateRecord::InsertClaim(uint32_t serialNum,
                                uint32_t low,
                                uint32_t high,
                                LtpContentHeader::ReceptionClaim claim)
{
    NS_LOG_FUNCTION(this << serialNum << low << high << claim.offset << claim.length);

    std::map<uint64_t, RedSegmentInfo>::iterator it = m_rcvSegments.find(serialNum);
    bool ret = false;

    if (it == m_rcvSegments.end())
    {
        RedSegmentInfo info;
        info.low_bound = low;
        info.high_bound = high;
        info.claims.insert(claim);

        std::pair<uint64_t, RedSegmentInfo> entry;
        entry = std::make_pair(serialNum, info);
        ret = m_rcvSegments.insert(entry).second;
    }
    else
    {
        it->second.low_bound = low;
        it->second.high_bound = high;
        ret = it->second.claims.insert(claim).second;
    }

    return ret;
}

bool
SessionStateRecord::StoreClaims(LtpContentHeader reportHeader)
{
    NS_LOG_FUNCTION(this);

    bool ret = false;

    for (uint32_t i = 0; i < reportHeader.GetRxClaimCnt(); i++)
    {
        ret = InsertClaim(reportHeader.GetRpSerialNumber(),
                          reportHeader.GetLowerBound(),
                          reportHeader.GetUpperBound(),
                          reportHeader.GetReceptionClaim(i));

        if (!ret)
        {
            break;
        }
    }

    return ret;
}

uint32_t
SessionStateRecord::GetNClaims(uint32_t serialNum) const
{
    NS_LOG_FUNCTION(this << serialNum);

    std::map<uint64_t, RedSegmentInfo>::const_iterator it = m_rcvSegments.find(serialNum);

    if (it != m_rcvSegments.end())
    {
        return it->second.claims.size();
    }

    return 0;
}

uint32_t
SessionStateRecord::GetClaimsUpperBound(uint32_t serialNum)
{
    NS_LOG_FUNCTION(this << serialNum);

    std::map<uint64_t, RedSegmentInfo>::const_iterator it = m_rcvSegments.find(serialNum);

    uint32_t upper = 0;

    if (it != m_rcvSegments.end())
    {
        upper = it->second.high_bound;
    }
    return upper;
}

uint32_t
SessionStateRecord::GetClaimsLowerBound(uint32_t serialNum)
{
    NS_LOG_FUNCTION(this << serialNum);

    uint32_t lower = 0;

    std::map<uint64_t, RedSegmentInfo>::const_iterator it = m_rcvSegments.find(serialNum);

    if (it != m_rcvSegments.end())
    {
        lower = it->second.high_bound;
    }
    return lower;
}

std::set<LtpContentHeader::ReceptionClaim>
SessionStateRecord::GetClaims(uint64_t reportSerialNumber)
{
    NS_LOG_FUNCTION(this << reportSerialNumber);

    std::map<uint64_t, RedSegmentInfo>::const_iterator it = m_rcvSegments.find(reportSerialNumber);

    if (it != m_rcvSegments.end())
    {
        return it->second.claims;
    }
    else
    {
        return std::set<LtpContentHeader::ReceptionClaim>();
    }
}

RedSegmentInfo
SessionStateRecord::FindMissingClaims(uint32_t serialNum)
{
    NS_LOG_FUNCTION(this << serialNum);
    uint32_t last_upper = 0;
    std::set<LtpContentHeader::ReceptionClaim> report_claims = GetClaims(serialNum);

    RedSegmentInfo missing_claims;
    missing_claims.high_bound = GetRedPartLength();
    missing_claims.low_bound = 0;

    for (std::set<LtpContentHeader::ReceptionClaim>::iterator it = report_claims.begin();
         it != report_claims.end();
         ++it)
    {
        LtpContentHeader::ReceptionClaim claim = *it;
        if (last_upper != claim.offset)
        {
            LtpContentHeader::ReceptionClaim missing;
            missing.offset = last_upper;
            missing.length = claim.offset - last_upper;
            missing_claims.claims.insert(missing);
        }
        last_upper = claim.offset + claim.length;
    }

    uint32_t redPartLength = GetRedPartLength();
    if (redPartLength > 0 && last_upper < redPartLength)
    {
        LtpContentHeader::ReceptionClaim missing;
        missing.offset = last_upper;
        missing.length = redPartLength - last_upper;
        missing_claims.claims.insert(missing);
    }

    return missing_claims;
}

uint64_t
SessionStateRecord::IncrementCpCurrentSerialNumber()
{
    NS_LOG_FUNCTION(this);
    return m_currentCpSerialNumber++;
}

uint64_t
SessionStateRecord::IncrementRpCurrentSerialNumber()
{
    NS_LOG_FUNCTION(this);
    return m_currentRpSerialNumber++;
}

void
SessionStateRecord::IncrementRtxNumber()
{
    NS_LOG_FUNCTION(this);
    m_rTxCnt++;
}

void
SessionStateRecord::SetRedPartFinished()
{
    NS_LOG_FUNCTION(this);
    m_redpartSuccess = true;
}

void
SessionStateRecord::SetBlockFinished()
{
    NS_LOG_FUNCTION(this);
    m_blockSuccess = true;
}

void
SessionStateRecord::SetFullRed()
{
    NS_LOG_FUNCTION(this);
    m_fullRedData = true;
}

void
SessionStateRecord::SetFullGreen()
{
    NS_LOG_FUNCTION(this);
    m_fullGreenData = true;
}

void
SessionStateRecord::SetRedPartLength(uint32_t len)
{
    NS_LOG_FUNCTION(this << len);
    m_redPartLength = len;
}

void
SessionStateRecord::Cancel(CancellationState s, CxReasonCode r)
{
    NS_LOG_FUNCTION(this << s << r);
    m_canceled = s;
    m_canceledReason = r;
}

uint64_t
SessionStateRecord::GetCpStartSerialNumber() const
{
    NS_LOG_FUNCTION(this);
    return m_firstCpSerialNumber;
}

uint64_t
SessionStateRecord::GetCpCurrentSerialNumber() const
{
    NS_LOG_FUNCTION(this);
    return m_currentCpSerialNumber;
}

uint64_t
SessionStateRecord::GetRpStartSerialNumber() const
{
    NS_LOG_FUNCTION(this);
    return m_firstRpSerialNumber;
}

uint64_t
SessionStateRecord::GetRpCurrentSerialNumber() const
{
    NS_LOG_FUNCTION(this);
    return m_currentRpSerialNumber;
}

Address
SessionStateRecord::GetPeerLtpEngineId() const
{
    NS_LOG_FUNCTION(this);
    return m_peerLtpEngine;
}

uint64_t
SessionStateRecord::GetLocalClientServiceId() const
{
    NS_LOG_FUNCTION(this);
    return m_localClientService;
}

bool
SessionStateRecord::IsRedPartFinished() const
{
    NS_LOG_FUNCTION(this);
    return m_redpartSuccess;
}

bool
SessionStateRecord::IsBlockFinished() const
{
    NS_LOG_FUNCTION(this);
    return m_blockSuccess;
}

bool
SessionStateRecord::IsCanceled() const
{
    NS_LOG_FUNCTION(this);
    return ((m_canceled == NOT_CANCELED) ? false : true);
}

bool
SessionStateRecord::IsFullRed() const
{
    NS_LOG_FUNCTION(this);
    return m_fullRedData;
}

bool
SessionStateRecord::IsFullGreen() const
{
    NS_LOG_FUNCTION(this);
    return m_fullGreenData;
}

CxReasonCode
SessionStateRecord::GetCancellationReason() const
{
    NS_LOG_FUNCTION(this);
    return m_canceledReason;
}

SessionId
SessionStateRecord::GetSessionId() const
{
    NS_LOG_FUNCTION(this);
    return m_sessionId;
}

uint64_t
SessionStateRecord::GetRTxNumber() const
{
    NS_LOG_FUNCTION(this);
    return m_rTxCnt;
}

uint32_t
SessionStateRecord::GetRedPartLength() const
{
    NS_LOG_FUNCTION(this);
    return m_redPartLength;
}

TypeId
SenderSessionStateRecord::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::SenderSessionStateRecord")
                            .SetParent<SessionStateRecord>()
                            .SetGroupName("BundleProtocol")
                            .AddConstructor<SenderSessionStateRecord>();
    return tid;
}

SenderSessionStateRecord::SenderSessionStateRecord()
    : SessionStateRecord(),
      m_destinationClientServiceId(0),
      m_txData(),
      m_cpTxCnt(0),
      m_redpartAckSuccess(false)
{
    NS_LOG_FUNCTION(this);
}

SenderSessionStateRecord::SenderSessionStateRecord(Address localLtpEngineId,
                                                   uint64_t localClientServiceId,
                                                   uint64_t destinationClientService,
                                                   Address destinationLtpEngine,
                                                   Ptr<UniformRandomVariable> number)
    : SessionStateRecord(localLtpEngineId, localClientServiceId, destinationLtpEngine),
      m_destinationClientServiceId(destinationClientService),
      m_txData(),
      m_cpTxCnt(0),
      m_redpartAckSuccess(false)
{
    NS_LOG_FUNCTION(this << localLtpEngineId << localClientServiceId << destinationClientService
                         << destinationLtpEngine << number);

    m_sessionId =
        SessionId(0, number->GetInteger(MIN_INITIAL_SERIAL_NUMBER, MAX_INITIAL_SERIAL_NUMBER));

    m_firstCpSerialNumber =
        number->GetInteger(MIN_INITIAL_SERIAL_NUMBER, MAX_INITIAL_SERIAL_NUMBER);
    m_currentCpSerialNumber = m_firstCpSerialNumber;
    m_firstRpSerialNumber = 0;
    m_currentRpSerialNumber = 0;
}

SenderSessionStateRecord::~SenderSessionStateRecord()
{
    NS_LOG_FUNCTION(this);
}

uint64_t
SenderSessionStateRecord::GetDestination() const
{
    NS_LOG_FUNCTION(this);
    return m_destinationClientServiceId;
}

uint64_t
SenderSessionStateRecord::GetCpRtxNumber() const
{
    NS_LOG_FUNCTION(this);
    return m_cpTxCnt;
}

bool
SenderSessionStateRecord::IsRedPartAck() const
{
    NS_LOG_FUNCTION(this);
    return m_redpartAckSuccess;
}

std::vector<uint8_t>
SenderSessionStateRecord::GetBlockData()
{
    NS_LOG_FUNCTION(this);
    return m_txData;
}

std::vector<uint8_t>
SenderSessionStateRecord::GetBlockData(uint32_t offset, uint32_t length)
{
    NS_LOG_FUNCTION(this << offset << length);
    std::vector<uint8_t> ret(length);
    std::copy(m_txData.begin() + offset, m_txData.begin() + offset + length, ret.begin());
    return ret;
}

void
SenderSessionStateRecord::CopyBlockData(std::vector<uint8_t> data)
{
    NS_LOG_FUNCTION(this << data.size());
    m_txData = data;
}

void
SenderSessionStateRecord::SetRedPartAck()
{
    NS_LOG_FUNCTION(this);
    m_redpartAckSuccess = true;
}

void
SenderSessionStateRecord::IncrementCpRtxNumber()
{
    NS_LOG_FUNCTION(this);
    m_cpTxCnt++;
}

TypeId
ReceiverSessionStateRecord::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::ReceiverSessionStateRecord")
                            .SetParent<SessionStateRecord>()
                            .SetGroupName("BundleProtocol")
                            .AddConstructor<ReceiverSessionStateRecord>();
    return tid;
}

ReceiverSessionStateRecord::ReceiverSessionStateRecord()
    : SessionStateRecord(),
      m_rpTxCnt(0),
      m_rxRedBuffer(),
      m_rxGreendBuffer()
{
    NS_LOG_FUNCTION(this);
}

ReceiverSessionStateRecord::ReceiverSessionStateRecord(Address localLtpEngineId,
                                                       uint64_t localClientServiceId,
                                                       SessionId session,
                                                       Ptr<UniformRandomVariable> number)
    : SessionStateRecord(localLtpEngineId, localClientServiceId, Address()),
      m_rpTxCnt(0),
      m_rxRedBuffer(),
      m_rxGreendBuffer()
{
    NS_LOG_FUNCTION(this << localLtpEngineId << localClientServiceId << session);

    m_sessionId = session;
    m_firstCpSerialNumber = 0;
    m_currentCpSerialNumber = 0;
    m_firstRpSerialNumber =
        number->GetInteger(MIN_INITIAL_SERIAL_NUMBER, MAX_INITIAL_SERIAL_NUMBER);
    m_currentRpSerialNumber = m_firstRpSerialNumber;
}

ReceiverSessionStateRecord::~ReceiverSessionStateRecord()
{
    NS_LOG_FUNCTION(this);
}

void
ReceiverSessionStateRecord::StoreRedDataSegment(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this << p);

    LtpHeader header;
    LtpContentHeader contentHeader;

    Ptr<Packet> packet = p->Copy();

    packet->RemoveHeader(header);
    contentHeader.SetSegmentType(header.GetSegmentType());
    packet->RemoveHeader(contentHeader);

    std::pair<uint32_t, Ptr<Packet>> entry;
    entry = std::make_pair(contentHeader.GetOffset(), p);

    m_rxRedBuffer.insert(entry);
}

void
ReceiverSessionStateRecord::StoreGreenDataSegment(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this << p);
    m_rxGreendBuffer.push(p);
}

Ptr<Packet>
ReceiverSessionStateRecord::RemoveRedDataSegment()
{
    NS_LOG_FUNCTION(this);
    Ptr<Packet> p = nullptr;
    if (m_rxRedBuffer.size() > 0)
    {
        std::map<uint32_t, Ptr<Packet>>::iterator it = m_rxRedBuffer.begin();
        p = it->second;
        m_rxRedBuffer.erase(it);
    }
    return p;
}

Ptr<Packet>
ReceiverSessionStateRecord::RemoveGreenDataSegment()
{
    NS_LOG_FUNCTION(this);
    Ptr<Packet> p = nullptr;
    if (m_rxGreendBuffer.size() > 0)
    {
        p = m_rxGreendBuffer.front();
        m_rxGreendBuffer.pop();
    }
    return p;
}

void
ReceiverSessionStateRecord::SetLowBound(uint32_t offset)
{
    NS_LOG_FUNCTION(this << offset);
    m_lowBound = offset;
}

void
ReceiverSessionStateRecord::SetHighBound(uint32_t offset)
{
    NS_LOG_FUNCTION(this << offset);
    if (offset > m_highBound)
    {
        m_highBound = offset;
    }
}

void
ReceiverSessionStateRecord::IncrementRpRtxNumber()
{
    NS_LOG_FUNCTION(this);
    m_rpTxCnt++;
}

uint32_t
ReceiverSessionStateRecord::GetLowBound() const
{
    NS_LOG_FUNCTION(this);
    return m_lowBound;
}

uint32_t
ReceiverSessionStateRecord::GetHighBound() const
{
    NS_LOG_FUNCTION(this);
    return m_highBound;
}

uint64_t
ReceiverSessionStateRecord::GetRpRtxNumber() const
{
    NS_LOG_FUNCTION(this);
    return m_rpTxCnt;
}

TypeId
LtpBundleCla::GetTypeId()
{
    static TypeId tid = TypeId("ns3::LtpBundleCla")
                            .SetParent<BundleCla>()
                            .SetGroupName("BundleProtocol")
                            .AddConstructor<LtpBundleCla>();
    return tid;
}

LtpBundleCla::LtpBundleCla()
{
    NS_LOG_FUNCTION(this);

    m_linkUp = MakeNullCallback<void, Ptr<LtpBundleCla>>();
    m_linkDown = MakeNullCallback<void, Ptr<LtpBundleCla>>();
    m_checkpointSent = MakeNullCallback<void, SessionId, RedSegmentInfo>();
    m_reportSent = MakeNullCallback<void, SessionId, RedSegmentInfo>();
    m_cancelSent = MakeNullCallback<void, SessionId>();
    m_endOfBlockSent = MakeNullCallback<void, SessionId>();
}

LtpBundleCla::~LtpBundleCla()
{
    NS_LOG_FUNCTION(this);
}

void
LtpBundleCla::Setup(Ptr<Node> node, Address localAddress, Address remoteAddress)
{
    NS_LOG_FUNCTION(this << node << localAddress << remoteAddress);
    m_node = node;
    m_localEngineId = localAddress;
    SetRemoteEngineId(remoteAddress);

    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    m_rcvSocket = Socket::CreateSocket(m_node, tid);

    if (m_rcvSocket->Bind(localAddress) == -1)
    {
        NS_LOG_ERROR("Failed to bind LTP socket to local address");
    }

    m_rcvSocket->SetRecvCallback(MakeCallback(&LtpBundleCla::HandleRead, this));
}

void
LtpBundleCla::RegisterClientService(uint64_t id, Ptr<ClientServiceStatus> client)
{
    m_activeClients.insert(std::make_pair(id, client));
}

void
LtpBundleCla::SendSegment(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this << p);

    m_rcvSocket->SendTo(p, 0, GetRemoteEngineId());

    LtpHeader header;
    LtpContentHeader contentHeader;

    Ptr<Packet> packet = p->Copy();

    packet->RemoveHeader(header);
    contentHeader.SetSegmentType(header.GetSegmentType());
    packet->RemoveHeader(contentHeader);

    RedSegmentInfo info;

    switch (header.GetSegmentType())
    {
    case LTPTYPE_RD_CP_EORP_EOB:
        if (!m_endOfBlockSent.IsNull())
        {
            m_endOfBlockSent(m_activeSessionId);
        }
    case LTPTYPE_RD_CP_EORP:
    case LTPTYPE_RD_CP:
        if (!m_checkpointSent.IsNull())
        {
            info.CpserialNum = contentHeader.GetCpSerialNumber();
            info.RpserialNum = contentHeader.GetRpSerialNumber();
            info.high_bound = contentHeader.GetUpperBound();
            info.low_bound = contentHeader.GetLowerBound();

            m_checkpointSent(m_activeSessionId, info);
        }
        break;
    case LTPTYPE_GD_EOB:
        if (!m_endOfBlockSent.IsNull())
        {
            m_endOfBlockSent(m_activeSessionId);
        }
        break;
    case LTPTYPE_RS:
        if (!m_reportSent.IsNull())
        {
            info.CpserialNum = contentHeader.GetCpSerialNumber();
            info.RpserialNum = contentHeader.GetRpSerialNumber();
            info.high_bound = contentHeader.GetUpperBound();
            info.low_bound = contentHeader.GetLowerBound();

            m_reportSent(m_activeSessionId, info);
        }
        break;
    case LTPTYPE_CS:
        if (!m_cancelSent.IsNull())
        {
            m_cancelSent(m_activeSessionId);
        }
        break;
    default:
        break;
    }
}

void
LtpBundleCla::Send(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this << p);

    Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
    Ptr<SenderSessionStateRecord> ssend =
        CreateObject<SenderSessionStateRecord>(m_localEngineId, 0, 0, GetRemoteEngineId(), uv);

    SessionId id = ssend->GetSessionId();

    m_activeSessions.insert(std::make_pair(id, ssend));
    m_activeSessionId = id;

    uint64_t rdSize = p->GetSize();

    EncapsulateBlockData(GetRemoteEngineId(), ssend, p, rdSize, false);

    Ptr<Packet> segment;
    while ((segment = ssend->Dequeue()))
    {
        SendSegment(segment);
    }

    ssend->SetBlockFinished();

    RedSegmentInfo info;
    info.CpserialNum = ssend->GetCpCurrentSerialNumber();
    info.RpserialNum = 0;
    info.low_bound = 0;
    info.high_bound = rdSize;

    SetCheckPointTransmissionTimer(id, info);
}

bool
LtpBundleCla::IsUp() const
{
    NS_LOG_FUNCTION(this);
    return m_rcvSocket != nullptr;
}

void
LtpBundleCla::SetNode(Ptr<Node> node)
{
    NS_LOG_FUNCTION(this << node);
    m_node = node;
}

Ptr<Node>
LtpBundleCla::GetNode() const
{
    NS_LOG_FUNCTION(this);
    return m_node;
}

Address
LtpBundleCla::GetLocalEngineId() const
{
    NS_LOG_FUNCTION(this);
    return m_localEngineId;
}

uint32_t
LtpBundleCla::GetCheckPointRetransLimit() const
{
    NS_LOG_FUNCTION(this);
    return m_cpRtxLimit;
}

uint32_t
LtpBundleCla::GetReportRetransLimit() const
{
    NS_LOG_FUNCTION(this);
    return m_rpRtxLimit;
}

uint32_t
LtpBundleCla::GetReceptionProblemLimit() const
{
    NS_LOG_FUNCTION(this);
    return m_rxProblemLimit;
}

uint32_t
LtpBundleCla::GetCancellationRetransLimit() const
{
    NS_LOG_FUNCTION(this);
    return m_cxRtxLimit;
}

uint32_t
LtpBundleCla::GetRetransCycleLimit() const
{
    NS_LOG_FUNCTION(this);
    return m_rtxCycleLimit;
}

void
LtpBundleCla::CancelSession(SessionId id)
{
    NS_LOG_FUNCTION(this << id);
    SessionStateRecords::iterator it = m_activeSessions.find(id);
    if (it != m_activeSessions.end())
    {
        it->second->Cancel(LOCAL_CANCEL, (CxReasonCode)0);
        ClientServiceInstances::iterator itCls =
            m_activeClients.find(it->second->GetLocalClientServiceId());
        if (itCls != m_activeClients.end())
        {
            itCls->second->ReportStatus(id, RX_SESSION_CANCEL);
        }
        m_activeSessions.erase(it);
    }
}

void
LtpBundleCla::EncapsulateBlockData(Address remoteAddress,
                                   Ptr<SessionStateRecord> ssr,
                                   Ptr<Packet> p,
                                   uint64_t rdSize,
                                   bool rtx)
{
    NS_LOG_FUNCTION(this << remoteAddress << ssr << p << rdSize << rtx);
    SessionId id = ssr->GetSessionId();
    LtpHeader header;
    uint8_t version = m_version;
    SegmentType type = LTPTYPE_RD;
    uint8_t extensionCntHeader = 0;
    uint8_t extensionCntTrailer = 0;
    LtpContentHeader contentHeader;
    uint64_t clientServiceId = ssr->GetLocalClientServiceId();
    uint64_t offset = 0;
    uint64_t length = 0;
    uint16_t mtu = GetMtu();
    uint64_t dataSize = p->GetSize();
    uint8_t* buffer = new uint8_t[dataSize];
    p->CopyData(buffer, dataSize);
    std::vector<uint8_t> data(buffer, buffer + dataSize);
    delete[] buffer;

    while (offset < dataSize)
    {
        header.SetVersion(version);
        header.SetSegmentType(type);
        header.SetSessionId(id);
        header.SetHeaderExtensionCount(extensionCntHeader);
        header.SetTrailerExtensionCount(extensionCntTrailer);
        contentHeader.SetSegmentType(type);
        contentHeader.SetClientServiceId(clientServiceId);
        contentHeader.SetOffset(offset);
        contentHeader.SetLength(length);
        uint64_t headerSize = header.GetSerializedSize();
        uint64_t contentHeaderSize = contentHeader.GetSerializedSize();
        length = mtu - headerSize - contentHeaderSize;
        contentHeader.SetLength(length);
        uint64_t size = headerSize + contentHeader.GetSerializedSize() + length;
        while (size > mtu)
        {
            length--;
            contentHeader.SetLength(length);
            size = headerSize + contentHeader.GetSerializedSize() + length;
        }
        if (rdSize == 0)
        {
            type = LTPTYPE_GD;
            header.SetSegmentType(type);
            contentHeader.SetSegmentType(type);
        }
        else if ((offset < rdSize) && (offset + length > rdSize))
        {
            type = LTPTYPE_RD_CP_EORP;
            if (offset + length >= dataSize)
            {
                if (!rtx)
                {
                    ssr->SetFullRed();
                    type = LTPTYPE_RD_CP_EORP_EOB;
                }
                else if (ssr->IsFullRed())
                {
                    type = LTPTYPE_RD_CP_EORP_EOB;
                }
            }
            if (rdSize > 0)
            {
                length = rdSize - offset;
                contentHeader.SetLength(length);
            }
            header.SetSegmentType(type);
            contentHeader.SetSegmentType(type);
            contentHeader.SetCpSerialNumber(ssr->GetCpCurrentSerialNumber());
            type = LTPTYPE_GD;
        }
        if ((offset > rdSize) && (offset + length >= dataSize))
        {
            type = LTPTYPE_GD_EOB;
            header.SetSegmentType(type);
            contentHeader.SetSegmentType(type);
        }
        std::vector<uint8_t>::const_iterator start = data.begin() + offset;
        std::vector<uint8_t>::const_iterator end =
            (offset + length > data.size()) ? data.end() : data.begin() + offset + length;
        std::vector<uint8_t> segmentData(start, end);
        Ptr<Packet> packet =
            Create<Packet>((uint8_t*)segmentData.data(), (uint32_t)segmentData.size());
        packet->AddHeader(contentHeader);
        packet->AddHeader(header);
        ssr->Enqueue(packet);
        offset += length;
    }
}

void
LtpBundleCla::CloseSession(SessionId id)
{
    NS_LOG_FUNCTION(this << id);
    SessionStateRecords::iterator it = m_activeSessions.find(id);
    if (it != m_activeSessions.end())
    {
        Ptr<SessionStateRecord> ssr = it->second;
        ssr->SetBlockFinished();
        if (ssr->GetNPackets())
        {
            Simulator::Schedule(m_localDelays, &LtpBundleCla::CloseSession, this, id);
            return;
        }
        ssr->CancelTimer(CHECKPOINT);
        ssr->CancelTimer(REPORT);
        ClientServiceInstances::iterator itCls =
            m_activeClients.find(ssr->GetLocalClientServiceId());
        if (itCls != m_activeClients.end())
        {
            itCls->second->ReportStatus(id, SESSION_END);
        }
        m_activeSessions.erase(it);
    }
}

void
LtpBundleCla::SignifyRedPartReception(SessionId id)
{
    NS_LOG_FUNCTION(this << id);
    SessionStateRecords::iterator it = m_activeSessions.find(id);
    if (it != m_activeSessions.end())
    {
        ClientServiceInstances::iterator itCls =
            m_activeClients.find(it->second->GetLocalClientServiceId());
        std::vector<uint8_t> blockData;
        bool EOB = false;
        Address remoteLtp = it->second->GetPeerLtpEngineId();
        if (it->second->GetInstanceTypeId() == ReceiverSessionStateRecord::GetTypeId())
        {
            Ptr<ReceiverSessionStateRecord> ssr =
                DynamicCast<ReceiverSessionStateRecord>(it->second);
            Ptr<Packet> p = 0;
            LtpHeader header;
            LtpContentHeader contentHeader;
            while ((p = ssr->RemoveRedDataSegment()))
            {
                p->RemoveHeader(header);
                contentHeader.SetSegmentType(header.GetSegmentType());
                p->RemoveHeader(contentHeader);
                uint32_t size = p->GetSize();
                uint8_t* raw_data = new uint8_t[size];
                p->CopyData(raw_data, size);
                std::vector<uint8_t> packetData(raw_data, raw_data + size);
                blockData.insert(blockData.end(), packetData.begin(), packetData.end());
                delete[] raw_data;
            }
            if (header.GetSegmentType() == LTPTYPE_RD_CP_EORP_EOB)
            {
                EOB = true;
            }
        }
        if (itCls != m_activeClients.end())
        {
            itCls->second
                ->ReportStatus(id, RED_PART_RCV, blockData, blockData.size(), EOB, remoteLtp);

            if (!blockData.empty())
            {
                Ptr<Packet> assembledPacket = Create<Packet>(blockData.data(), blockData.size());
                Ptr<Bundle> bundle = CreateObject<Bundle>();
                bundle->Deserialize(assembledPacket);
                NotifyReception(bundle);
            }
        }
    }
}

void
LtpBundleCla::SignifyGreenPartSegmentArrival(SessionId id)
{
    NS_LOG_FUNCTION(this << id);
    SessionStateRecords::iterator it = m_activeSessions.find(id);
    if (it == m_activeSessions.end())
    {
        return;
    }
    ClientServiceInstances::iterator itCls =
        m_activeClients.find(it->second->GetLocalClientServiceId());
    if (it->second->GetInstanceTypeId() == ReceiverSessionStateRecord::GetTypeId())
    {
        Ptr<ReceiverSessionStateRecord> ssr = DynamicCast<ReceiverSessionStateRecord>(it->second);
        std::vector<uint8_t> packetData;
        Ptr<Packet> p = 0;
        bool EOB = false;
        uint32_t offset = 0;
        Address remoteLtp = it->second->GetPeerLtpEngineId();
        if ((p = ssr->RemoveGreenDataSegment()))
        {
            LtpHeader header;
            LtpContentHeader contentHeader;
            p->RemoveHeader(header);
            contentHeader.SetSegmentType(header.GetSegmentType());
            p->RemoveHeader(contentHeader);
            offset = contentHeader.GetOffset();
            if (offset == 0)
            {
                ssr->SetFullGreen();
            }
            if (header.GetSegmentType() == LTPTYPE_GD_EOB)
            {
                EOB = true;
            }
            uint32_t size = p->GetSize();
            uint8_t* raw_data = new uint8_t[size];
            p->CopyData(raw_data, size);
            packetData.insert(packetData.end(), raw_data, raw_data + size);
            delete[] raw_data;
        }
        if (itCls != m_activeClients.end())
        {
            itCls->second->ReportStatus(id,
                                        GP_SEGMENT_RCV,
                                        packetData,
                                        packetData.size(),
                                        EOB,
                                        remoteLtp,
                                        offset);
        }
    }
}

void
LtpBundleCla::CheckRedPartReceived(SessionId id)
{
    NS_LOG_FUNCTION(this << id);
    SessionStateRecords::iterator it = m_activeSessions.find(id);
    if (it != m_activeSessions.end())
    {
        Ptr<ReceiverSessionStateRecord> ssr = DynamicCast<ReceiverSessionStateRecord>(it->second);
        if (ssr)
        {
            if (ssr->GetRedPartLength() == 0)
            {
                return;
            }
            RedSegmentInfo info = ssr->FindMissingClaims(ssr->GetRpCurrentSerialNumber());
            if (info.claims.size() == 0)
            {
                ssr->SetRedPartFinished();
            }
        }
    }
}

void
LtpBundleCla::ReportSegmentTransmission(SessionId id, uint64_t cpSerialNum)
{
    NS_LOG_FUNCTION(this << id << cpSerialNum);
    SessionStateRecords::iterator it = m_activeSessions.find(id);
    if (it == m_activeSessions.end())
    {
        return;
    }
    Ptr<ReceiverSessionStateRecord> srecv = DynamicCast<ReceiverSessionStateRecord>(it->second);
    if (!srecv)
    {
        return;
    }
    Ptr<Packet> p = Create<Packet>();
    LtpHeader header;
    LtpContentHeader contentHeader;
    header.SetSegmentType(LTPTYPE_RS);
    header.SetVersion(m_version);
    header.SetSessionId(id);
    uint32_t RpSerial = srecv->GetRpCurrentSerialNumber();
    uint32_t CpSerial = cpSerialNum;
    uint32_t upperBound = srecv->GetHighBound();
    uint32_t lowerBound = srecv->GetLowBound();
    contentHeader.SetSegmentType(LTPTYPE_RS);
    contentHeader.SetRpSerialNumber(RpSerial);
    contentHeader.SetCpSerialNumber(CpSerial);
    contentHeader.SetUpperBound(upperBound);
    contentHeader.SetLowerBound(lowerBound);
    std::set<LtpContentHeader::ReceptionClaim> claims = srecv->GetClaims(RpSerial);
    for (std::set<LtpContentHeader::ReceptionClaim>::iterator itC = claims.begin();
         itC != claims.end();
         ++itC)
    {
        contentHeader.AddReceptionClaim(*itC);
    }
    p->AddHeader(contentHeader);
    p->AddHeader(header);
    srecv->StoreClaims(contentHeader);
    SendSegment(p);
}

void
LtpBundleCla::ReportSegmentAckTransmission(SessionId id, uint64_t rpSerialNum)
{
    NS_LOG_FUNCTION(this << id << rpSerialNum);
    SessionStateRecords::iterator it = m_activeSessions.find(id);
    Ptr<Packet> p = Create<Packet>();
    LtpHeader header;
    LtpContentHeader contentHeader;
    header.SetSegmentType(LTPTYPE_RAS);
    header.SetVersion(m_version);
    header.SetSessionId(id);
    contentHeader.SetSegmentType(LTPTYPE_RAS);
    contentHeader.SetRpSerialNumber(rpSerialNum);
    p->AddHeader(contentHeader);
    p->AddHeader(header);
    if (it != m_activeSessions.end())
    {
        Ptr<SenderSessionStateRecord> ssr = DynamicCast<SenderSessionStateRecord>(it->second);
        if (ssr)
        {
            SendSegment(p);
        }
    }
    else
    {
        SendSegment(p);
    }
}

void
LtpBundleCla::RetransmitSegment(SessionId id, RedSegmentInfo info)
{
    NS_LOG_FUNCTION(this << id);
    SessionStateRecords::iterator it = m_activeSessions.find(id);
    if (it != m_activeSessions.end())
    {
        Ptr<SenderSessionStateRecord> ssr = DynamicCast<SenderSessionStateRecord>(it->second);
        if (ssr && m_cpRtxLimit > ssr->GetCpRtxNumber())
        {
            std::vector<uint8_t> rdData = ssr->GetBlockData();
            Ptr<Packet> p = Create<Packet>((uint8_t*)rdData.data(), rdData.size());
            EncapsulateBlockData(ssr->GetPeerLtpEngineId(), ssr, p, rdData.size(), true);
            ssr->IncrementCpRtxNumber();
            while (Ptr<Packet> pkt = ssr->Dequeue())
            {
                SendSegment(pkt);
            }
        }
    }
}

void
LtpBundleCla::RetransmitReport(SessionId id, RedSegmentInfo info)
{
    NS_LOG_FUNCTION(this << id);
    SessionStateRecords::iterator it = m_activeSessions.find(id);
    if (it != m_activeSessions.end())
    {
        Ptr<ReceiverSessionStateRecord> srecv = DynamicCast<ReceiverSessionStateRecord>(it->second);
        if (srecv && m_rpRtxLimit > srecv->GetRpRtxNumber())
        {
            ReportSegmentTransmission(id, info.CpserialNum);
        }
    }
}

void
LtpBundleCla::RetransmitCheckpoint(SessionId id, RedSegmentInfo info)
{
    NS_LOG_FUNCTION(this << id);
}

void
LtpBundleCla::HandleRead(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    Ptr<Packet> packet;
    Address peer;
    while ((packet = socket->RecvFrom(peer)))
    {
        LtpHeader header;
        LtpContentHeader contentHeader;
        Ptr<Packet> p = packet->Copy();
        packet->RemoveHeader(header);
        contentHeader.SetSegmentType(header.GetSegmentType());
        packet->RemoveHeader(contentHeader);
        SessionId id = header.GetSessionId();
        SegmentType type = header.GetSegmentType();
        SessionStateRecords::iterator itSessions = m_activeSessions.find(id);
        Ptr<ReceiverSessionStateRecord> srecv = 0;
        Ptr<SenderSessionStateRecord> ssend = 0;
        if (itSessions == m_activeSessions.end())
        {
            if (!LtpHeader::IsDataSegment(type))
            {
                continue;
            }
            ClientServiceInstances::iterator itClients =
                m_activeClients.find(contentHeader.GetClientServiceId());
            if (itClients == m_activeClients.end())
            {
                continue;
            }
            Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
            srecv =
                CreateObject<ReceiverSessionStateRecord>(m_localEngineId, itClients->first, id, uv);
            std::pair<SessionId, Ptr<SessionStateRecord>> entry;
            entry = std::make_pair(id, DynamicCast<SessionStateRecord>(srecv));
            m_activeSessions.insert(m_activeSessions.begin(), entry);
        }
        else
        {
            if (itSessions->second->GetInstanceTypeId() == SenderSessionStateRecord::GetTypeId())
            {
                ssend = DynamicCast<SenderSessionStateRecord>(itSessions->second);
            }
            else
            {
                srecv = DynamicCast<ReceiverSessionStateRecord>(itSessions->second);
            }
        }
        RedSegmentInfo retrans_info;
        if (LtpHeader::IsDataSegment(type))
        {
            if (srecv && LtpHeader::IsRedDataSegment(type) && srecv->IsRedPartFinished())
            {
            }
            else if (srecv)
            {
                if (LtpHeader::IsRedDataSegment(type))
                {
                    srecv->StoreRedDataSegment(p);
                    LtpContentHeader::ReceptionClaim claim;
                    claim.offset = contentHeader.GetOffset();
                    claim.length = contentHeader.GetLength();
                    uint32_t upperBound = claim.offset + claim.length;
                    srecv->SetHighBound(upperBound);
                    srecv->InsertClaim(srecv->GetRpCurrentSerialNumber(),
                                       srecv->GetLowBound(),
                                       srecv->GetHighBound(),
                                       claim);
                }
                else
                {
                    srecv->StoreGreenDataSegment(p);
                }
                switch (type)
                {
                case LTPTYPE_RD:
                    break;
                case LTPTYPE_RD_CP:
                    ReportSegmentTransmission(id, contentHeader.GetCpSerialNumber());
                    break;
                case LTPTYPE_RD_CP_EORP:
                    ReportSegmentTransmission(id, contentHeader.GetCpSerialNumber());
                    srecv->SetRedPartLength(contentHeader.GetOffset() + contentHeader.GetLength());
                    break;
                case LTPTYPE_RD_CP_EORP_EOB:
                    ReportSegmentTransmission(id, contentHeader.GetCpSerialNumber());
                    srecv->SetRedPartLength(contentHeader.GetOffset() + contentHeader.GetLength());
                    srecv->SetBlockFinished();
                    if (srecv->IsRedPartFinished() && srecv->IsBlockFinished())
                    {
                        CloseSession(id);
                    }
                    break;
                case LTPTYPE_GD:
                    SignifyGreenPartSegmentArrival(id);
                    break;
                case LTPTYPE_GD_EOB:
                    srecv->SetBlockFinished();
                    SignifyGreenPartSegmentArrival(id);
                    if ((srecv->IsRedPartFinished() && srecv->IsBlockFinished()) ||
                        srecv->IsFullGreen())
                    {
                        CloseSession(id);
                    }
                    break;
                default:
                    break;
                }
            }
        }
        else
        {
            switch (type)
            {
            case LTPTYPE_RS:
                ReportSegmentAckTransmission(id, contentHeader.GetRpSerialNumber());
                if (ssend)
                {
                    ssend->CancelTimer(CHECKPOINT);
                    ssend->StoreClaims(contentHeader);
                    retrans_info = ssend->FindMissingClaims(contentHeader.GetRpSerialNumber());
                    if (retrans_info.claims.size())
                    {
                        RetransmitSegment(id, retrans_info);
                    }
                    else
                    {
                        ssend->SetRedPartFinished();
                    }
                    ssend->IncrementCpCurrentSerialNumber();
                    if (ssend->IsRedPartFinished() && ssend->IsBlockFinished())
                    {
                        CloseSession(id);
                    }
                }
                break;
            case LTPTYPE_RAS:
                if (srecv)
                {
                    srecv->CancelTimer(REPORT);
                    CheckRedPartReceived(id);
                    if (srecv->IsRedPartFinished())
                    {
                        SignifyRedPartReception(id);
                    }
                    srecv->IncrementRpCurrentSerialNumber();
                    if (srecv->IsRedPartFinished() && srecv->IsBlockFinished())
                    {
                        CloseSession(id);
                    }
                }
                break;
            default:
                break;
            }
        }
    }
}

uint16_t
LtpBundleCla::GetMtu() const
{
    NS_LOG_FUNCTION(this);
    // MTU = 1500 bytes, IPv4 = 20 bytes, UDP = 8 bytes.
    return 1472;
}

void
LtpBundleCla::SetCheckPointTransmissionTimer(SessionId id, RedSegmentInfo info)
{
    NS_LOG_FUNCTION(this << id);
    SessionStateRecords::iterator it = m_activeSessions.find(id);
    if (it != m_activeSessions.end())
    {
        double rtt = m_onewayLightTime.GetSeconds() * 2 + m_localDelays.GetSeconds() * 2 + 1.0;
        Ptr<SessionStateRecord> ssr = it->second;
        ssr->SetTimerFunction(&LtpBundleCla::RetransmitSegment,
                              this,
                              id,
                              info,
                              Seconds(rtt),
                              CHECKPOINT);
        ssr->StartTimer(CHECKPOINT);
    }
}

void
LtpBundleCla::SetReportReTransmissionTimer(SessionId id, RedSegmentInfo info)
{
    NS_LOG_FUNCTION(this << id);
    SessionStateRecords::iterator it = m_activeSessions.find(id);
    if (it != m_activeSessions.end())
    {
        double rtt = m_onewayLightTime.GetSeconds() * 2 + m_localDelays.GetSeconds() * 2 + 1.0;
        Ptr<SessionStateRecord> ssr = it->second;
        ssr->SetTimerFunction(&LtpBundleCla::RetransmitReport,
                              this,
                              id,
                              info,
                              Seconds(rtt),
                              REPORT);
        ssr->StartTimer(REPORT);
    }
}

void
LtpBundleCla::SetEndOfBlockTransmission(SessionId id)
{
    NS_LOG_FUNCTION(this << id);
    SessionStateRecords::iterator it = m_activeSessions.find(id);
    if (it != m_activeSessions.end())
    {
        Ptr<SenderSessionStateRecord> ssend = DynamicCast<SenderSessionStateRecord>(it->second);
        if (ssend)
        {
            ssend->SetBlockFinished();
            if (ssend->IsRedPartFinished() && ssend->IsBlockFinished())
            {
                CloseSession(id);
            }
        }
    }
}

Address
LtpBundleCla::GetRemoteEngineId() const
{
    NS_LOG_FUNCTION(this);
    return m_peerLtpEngineId;
}

void
LtpBundleCla::SetRemoteEngineId(Address id)
{
    NS_LOG_FUNCTION(this << id);
    m_peerLtpEngineId = id;
}

SessionId
LtpBundleCla::GetSessionId() const
{
    NS_LOG_FUNCTION(this);
    return m_activeSessionId;
}

void
LtpBundleCla::SetSessionId(SessionId id)
{
    NS_LOG_FUNCTION(this << id);
    m_activeSessionId = id;
}

void
LtpBundleCla::SetLinkUpCallback(Callback<void, Ptr<LtpBundleCla>> cb)
{
    NS_LOG_FUNCTION(this);
    m_linkUp = cb;
}

void
LtpBundleCla::SetLinkDownCallback(Callback<void, Ptr<LtpBundleCla>> cb)
{
    NS_LOG_FUNCTION(this);
    m_linkDown = cb;
}

void
LtpBundleCla::SetCheckPointSentCallback(Callback<void, SessionId, RedSegmentInfo> cb)
{
    NS_LOG_FUNCTION(this);
    m_checkpointSent = cb;
}

void
LtpBundleCla::SetReportSentCallback(Callback<void, SessionId, RedSegmentInfo> cb)
{
    NS_LOG_FUNCTION(this);
    m_reportSent = cb;
}

void
LtpBundleCla::SetEndOfBlockSentCallback(Callback<void, SessionId> cb)
{
    NS_LOG_FUNCTION(this);
    m_endOfBlockSent = cb;
}

void
LtpBundleCla::SetCancellationCallback(Callback<void, SessionId> cb)
{
    NS_LOG_FUNCTION(this);
    m_cancelSent = cb;
}

void
LtpBundleCla::SetOnewayLightTime(Time owlt)
{
    m_onewayLightTime = owlt;
}

} // namespace ns3
