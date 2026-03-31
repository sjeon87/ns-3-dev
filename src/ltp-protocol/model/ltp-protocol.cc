/*
 * Copyright (c) 2014 Universitat Autònoma de Barcelona
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 *
 * Author: Rubén Martínez <rmartinez@deic.uab.cat>
 */

#include "ltp-protocol.h"

#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/ptr.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"

#include <sstream>

NS_LOG_COMPONENT_DEFINE("LtpProtocol");

namespace ns3
{
namespace ltp
{

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
                            "ns3::TracedCallback<...>")

        ;
    return tid;
}

void
ClientServiceStatus::ReportStatus(SessionId id,
                                  StatusNotificationCode code,
                                  std::vector<uint8_t> data = std::vector<uint8_t>(),
                                  uint32_t dataLength = 0,
                                  bool endFlag = false,
                                  uint64_t srcLtpEngine = 0,
                                  uint32_t offset = 0)
{
    NS_LOG_FUNCTION(this);
    m_reportStatus(id, code, data, dataLength, endFlag, srcLtpEngine, offset);
}

void
ClientServiceStatus::AddSession(SessionId id)
{
    NS_LOG_FUNCTION(this);
    m_activeSessions.insert(m_activeSessions.begin(), id);
}

void
ClientServiceStatus::ClearSessions()
{
    NS_LOG_FUNCTION(this);
    m_activeSessions.clear();
}

SessionId
ClientServiceStatus::GetSession(uint32_t index)
{
    NS_LOG_FUNCTION(this);
    return m_activeSessions.at(index);
}

uint32_t
ClientServiceStatus::GetNSessions() const
{
    NS_LOG_FUNCTION(this);
    return m_activeSessions.size();
}

LtpProtocol::LtpProtocol()
    : m_activeSessions(),
      m_activeClients(),
      m_clas(),
      m_localEngineId(0),
      m_cpRtxLimit(0),
      m_rpRtxLimit(0),
      m_rxProblemLimit(0),
      m_cxRtxLimit(0),
      m_rtxCycleLimit(0),
      m_version(0),
      m_localDelays(Seconds(0)),
      m_onewayLightTime(Seconds(0)),
      m_localOperatingSchedule()
{
    NS_LOG_FUNCTION(this);
    m_uniformRandomVariable = CreateObject<UniformRandomVariable>();
}

LtpProtocol::~LtpProtocol()
{
    NS_LOG_FUNCTION(this);
}

TypeId
LtpProtocol::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::LtpProtocol")
            .SetParent<Object>()
            .AddConstructor<LtpProtocol>()
            .AddAttribute("RandomVariable",
                          "The random variable used to generate serial numbers.",
                          StringValue("ns3::UniformRandomVariable"),
                          MakePointerAccessor(&LtpProtocol::m_uniformRandomVariable),
                          MakePointerChecker<RandomVariableStream>())
            .AddAttribute("LocalEngineId",
                          "Identification of the local LTP engine",
                          UintegerValue(0),
                          MakeUintegerAccessor(&LtpProtocol::m_localEngineId),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("CheckPointRtxLimit",
                          "Maximum number of checkpoints retransmissions allowed",
                          UintegerValue(20),
                          MakeUintegerAccessor(&LtpProtocol::m_cpRtxLimit),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("ReportSegmentRtxLimit",
                          "Maximum number of report segment retransmissions allowed",
                          UintegerValue(20),
                          MakeUintegerAccessor(&LtpProtocol::m_rpRtxLimit),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("ReceptionProblemLimit",
                          "Maximum number of reception failures allowed",
                          UintegerValue(20),
                          MakeUintegerAccessor(&LtpProtocol::m_rxProblemLimit),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("CancelationRtxLimit",
                          "Maximum number of cancelation request retransmissions allowed",
                          UintegerValue(20),
                          MakeUintegerAccessor(&LtpProtocol::m_cxRtxLimit),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("RetransCyclelimit",
                          "Maximum number of cancelation cycle retransmissions allowed",
                          UintegerValue(20),
                          MakeUintegerAccessor(&LtpProtocol::m_rtxCycleLimit),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("LocalProcessingDelays",
                          "Queue Processing Times (for use in timers)",
                          TimeValue(Seconds(0.1)),
                          MakeTimeAccessor(&LtpProtocol::m_localDelays),
                          MakeTimeChecker())
            .AddAttribute("OneWayLightTime",
                          "Time to reach destination (for use in timers)",
                          TimeValue(Seconds(10.0)),
                          MakeTimeAccessor(&LtpProtocol::m_onewayLightTime),
                          MakeTimeChecker());
    return tid;
}

uint32_t
LtpProtocol::GetCheckPointRetransLimit() const
{
    NS_LOG_FUNCTION(this);
    return m_cpRtxLimit;
}

uint32_t
LtpProtocol::GetReportRetransLimit() const
{
    NS_LOG_FUNCTION(this);
    return m_rpRtxLimit;
}

uint32_t
LtpProtocol::GetReceptionProblemLimit() const
{
    NS_LOG_FUNCTION(this);
    return m_rxProblemLimit;
}

uint32_t
LtpProtocol::GetCancellationRetransLimit() const
{
    NS_LOG_FUNCTION(this);
    return m_cxRtxLimit;
}

uint32_t
LtpProtocol::GetRetransCycleLimit() const
{
    NS_LOG_FUNCTION(this);
    return m_rtxCycleLimit;
}

bool
LtpProtocol::RegisterClientService(uint64_t id, const CallbackBase& cb)
{
    NS_LOG_FUNCTION(this << id);
    std::pair<uint64_t, Ptr<ClientServiceStatus>> entry;

    Ptr<ClientServiceStatus> notifications = CreateObject<ClientServiceStatus>();
    notifications->TraceConnectWithoutContext("SessionStatus", cb);
    entry = std::make_pair(id, notifications);

    std::pair<ClientServiceInstances::iterator, bool> ret;
    ret = m_activeClients.insert(entry);
    return ret.second;
}

void
LtpProtocol::UnregisterClientService(uint64_t id)
{
    NS_LOG_FUNCTION(this << id);
    ClientServiceInstances::iterator it = m_activeClients.find(id);

    for (uint32_t i = 0; i < it->second->GetNSessions(); ++i)
    {
        SessionId id = it->second->GetSession(i);
        CancelSession(id);
    }

    it->second->ClearSessions();
    m_activeClients.erase(it);
}

uint32_t
LtpProtocol::StartTransmission(uint64_t sourceId,
                               uint64_t dstClientService,
                               uint64_t dstLtpEngine,
                               std::vector<uint8_t> data,
                               uint64_t rdSize)
{
    NS_LOG_FUNCTION(this << dstClientService << dstLtpEngine << rdSize);

    Ptr<SenderSessionStateRecord> ssr =
        CreateObject<SenderSessionStateRecord>(m_localEngineId,
                                               sourceId,
                                               dstClientService,
                                               dstLtpEngine,
                                               m_uniformRandomVariable);

    SessionId id = ssr->GetSessionId();

    /* Report Session Start to Client Service Instance */
    ClientServiceInstances::iterator it;
    it = m_activeClients.find(sourceId);
    it->second->ReportStatus(id, SESSION_START);
    it->second->AddSession(id);

    /* Keep Track of new session */
    std::pair<SessionId, Ptr<SessionStateRecord>> entry;
    entry = std::make_pair(id, DynamicCast<SessionStateRecord>(ssr));
    m_activeSessions.insert(m_activeSessions.begin(), entry);

    if (rdSize == 0)
    {
        ssr->SetFullGreen();
    }
    else if (rdSize == data.size())
    {
        ssr->SetFullRed();
    }

    Ptr<LtpConvergenceLayerAdapter> link = GetDatalink(dstLtpEngine);
    link->SetSessionId(id);
    EncapsulateBlockData(dstClientService, ssr, data, rdSize);

    std::vector<uint8_t>::const_iterator start = data.begin();
    std::vector<uint8_t>::const_iterator end = data.begin() + rdSize;
    std::vector<uint8_t> redData(start, end);

    ssr->CopyBlockData(redData); // Store red data as it may be needed for retransmission
    return ssr->GetNPackets();
}

void
LtpProtocol::CancelSession(SessionId id)
{
    NS_LOG_FUNCTION(this << id);
    SessionStateRecords::iterator it = m_activeSessions.find(id);
    it->second->Cancel(LOCAL_CANCEL, USR_CNCLD);

    ClientServiceInstances::iterator itCls =
        m_activeClients.find(it->second->GetLocalClientServiceId());
    itCls->second->ReportStatus(id, RX_SESSION_CANCEL);

    m_activeSessions.erase(it);
}

void
LtpProtocol::SignifyRedPartReception(SessionId id)
{
    NS_LOG_FUNCTION(this << id);
    SessionStateRecords::iterator it = m_activeSessions.find(id);

    if (it != m_activeSessions.end())
    {
        ClientServiceInstances::iterator itCls =
            m_activeClients.find(it->second->GetLocalClientServiceId());

        std::vector<uint8_t> blockData;
        bool EOB = false;
        uint64_t remoteLtp = it->second->GetPeerLtpEngineId();

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
                delete raw_data;
            }

            if (header.GetSegmentType() == LTPTYPE_RD_CP_EORP_EOB)
            {
                EOB = true;
            }
        }

        itCls->second->ReportStatus(id, RED_PART_RCV, blockData, blockData.size(), EOB, remoteLtp);
    }
}

void
LtpProtocol::SignifyGreenPartSegmentArrival(SessionId id)
{
    NS_LOG_FUNCTION(this << id);

    SessionStateRecords::iterator it = m_activeSessions.find(id);

    ClientServiceInstances::iterator itCls =
        m_activeClients.find(it->second->GetLocalClientServiceId());

    if (it->second->GetInstanceTypeId() == ReceiverSessionStateRecord::GetTypeId())
    {
        Ptr<ReceiverSessionStateRecord> ssr = DynamicCast<ReceiverSessionStateRecord>(it->second);

        std::vector<uint8_t> packetData;
        Ptr<Packet> p = 0;
        bool EOB = false;
        uint32_t offset = 0;
        uint64_t remoteLtp = it->second->GetPeerLtpEngineId();

        if ((p = ssr->RemoveGreenDataSegment()))
        {
            LtpHeader header;
            LtpContentHeader contentHeader;

            p->RemoveHeader(header);
            contentHeader.SetSegmentType(header.GetSegmentType());
            p->RemoveHeader(contentHeader);

            offset = contentHeader.GetOffset();

            // No better way to determine if this is a full Green block on the receiver side
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
            delete raw_data;
        }

        itCls->second->ReportStatus(id,
                                    GP_SEGMENT_RCV,
                                    packetData,
                                    packetData.size(),
                                    EOB,
                                    remoteLtp,
                                    offset);
    }
}

void
LtpProtocol::CloseSession(SessionId id)
{
    NS_LOG_FUNCTION(this << id);

    SessionStateRecords::iterator it = m_activeSessions.find(id);

    if (it != m_activeSessions.end())
    {
        Ptr<SessionStateRecord> ssr = it->second;
        ssr->SetBlockFinished();

        if (it->second->GetNPackets())
        {
            // Buffer is not empty , schedule again to allow the transmission of buffered segments
            // and exit
            Simulator::Schedule(m_localDelays, &LtpProtocol::CloseSession, this, id);
            return;
        }

        ssr->CancelTimer(CHECKPOINT);
        ssr->CancelTimer(REPORT);

        ClientServiceInstances::iterator itCls =
            m_activeClients.find(ssr->GetLocalClientServiceId());
        itCls->second->ReportStatus(id, SESSION_END);

        /* This should be done later, send a CX request first*/
        m_activeSessions.erase(it);
    }
}

void
LtpProtocol::SetCheckPointTransmissionTimer(SessionId id, RedSegmentInfo info)
{
    NS_LOG_FUNCTION(this << id);

    SessionStateRecords::iterator it = m_activeSessions.find(id);

    Ptr<SessionStateRecord> ssr = 0;

    if (it != m_activeSessions.end())
    {
        // Round trip = time to reach peer * 2 ( one way and back ) + processing times at both sides
        // + margin
        double rtt = m_onewayLightTime.GetSeconds() * 2 + m_localDelays.GetSeconds() * 2 + 1.0;

        ssr = it->second;
        ssr->SetTimerFunction(&LtpProtocol::RetransmitSegment,
                              this,
                              id,
                              info,
                              Seconds(rtt),
                              CHECKPOINT);
        ssr->StartTimer(CHECKPOINT);
    }
}

void
LtpProtocol::SetReportReTransmissionTimer(SessionId id, RedSegmentInfo info)
{
    NS_LOG_FUNCTION(this << id);

    SessionStateRecords::iterator it = m_activeSessions.find(id);
    Ptr<SessionStateRecord> ssr = 0;

    if (it != m_activeSessions.end())
    {
        // Round trip = time to reach peer * 2 ( one way and back ) + processing times at both sides
        // + margin
        double rtt = m_onewayLightTime.GetSeconds() * 2 + m_localDelays.GetSeconds() * 2 + 1.0;

        ssr = it->second;
        ssr->SetTimerFunction(&LtpProtocol::RetransmitReport, this, id, info, Seconds(rtt), REPORT);
        ssr->StartTimer(REPORT);
    }
}

/* Should be called on signal from the LinkStateCue: linkUp */
void
LtpProtocol::Send(Ptr<LtpConvergenceLayerAdapter> cla)
{
    NS_LOG_FUNCTION(this);
    SessionId id = cla->GetSessionId();
    SessionStateRecords::iterator it = m_activeSessions.find(id);

    // Dequeue all segments from SSR
    if (it != m_activeSessions.end())
    {
        Ptr<Packet> packet = it->second->Dequeue();

        if (packet)
        {
            cla->Send(packet);
            Simulator::Schedule(m_localDelays, &LtpProtocol::Send, this, cla);
        }
    }
}

/* Should be called from lower layer */
void
LtpProtocol::Receive(Ptr<Packet> packet, Ptr<LtpConvergenceLayerAdapter> cla)
{
    NS_LOG_FUNCTION(this);

    LtpHeader header;
    LtpContentHeader contentHeader;

    Ptr<Packet> p = packet->Copy();

    uint32_t bytes = packet->RemoveHeader(header);
    NS_ASSERT(bytes == header.GetSerializedSize());

    contentHeader.SetSegmentType(header.GetSegmentType());
    bytes = packet->RemoveHeader(contentHeader);
    NS_ASSERT(bytes == contentHeader.GetSerializedSize());

    SessionId id = header.GetSessionId();
    SegmentType type = header.GetSegmentType();

    SessionStateRecords::iterator itSessions = m_activeSessions.find(id);
    Ptr<ReceiverSessionStateRecord> srecv = 0;
    Ptr<SenderSessionStateRecord> ssend = 0;

    /* First Segment from this peer received: Create a new receiver SSR*/
    if (itSessions == m_activeSessions.end())
    {
        /* Check that Destination Client Service Instance exists*/
        ClientServiceInstances::iterator itClients =
            m_activeClients.find(contentHeader.GetClientServiceId());

        if (itClients == m_activeClients.end())
        {
            // Service Client does not exist
            // Send CX Segment.
            return;
        }

        srecv = CreateObject<ReceiverSessionStateRecord>(m_localEngineId,
                                                         itClients->first,
                                                         itSessions->first,
                                                         m_uniformRandomVariable);

        /* Add to active sessions */
        std::pair<SessionId, Ptr<SessionStateRecord>> entry;
        entry = std::make_pair(id, DynamicCast<SessionStateRecord>(srecv));
        m_activeSessions.insert(m_activeSessions.begin(), entry);
    }
    else
    {
        /* Check if we are on the receiver or the sender side */
        if (itSessions->second->GetInstanceTypeId() == SenderSessionStateRecord::GetTypeId())
        {
            ssend = DynamicCast<SenderSessionStateRecord>(itSessions->second);
        }
        else
        {
            srecv = DynamicCast<ReceiverSessionStateRecord>(itSessions->second);
        }
    }

    std::stringstream ss;
    RedSegmentInfo retrans_info;
    ss << "LtpEngine: " << m_localEngineId << " ";

    /* If data segment*/
    if (LtpHeader::IsDataSegment(type))
    {
        if (LtpHeader::IsRedDataSegment(type) && srecv->IsRedPartFinished())
        {
            ss << " Miss-colored part received : " << header;
            NS_LOG_DEBUG(ss.str());
            // Send CX Segment.
        }
        else
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
                ss << "Received a Red data segment : " << header << contentHeader;
                NS_LOG_DEBUG(ss.str());
                break;
            case LTPTYPE_RD_CP:
                ss << "Received Red data segment and Checkpoint : " << header << contentHeader;
                NS_LOG_DEBUG(ss.str());
                ReportSegmentTransmission(id, contentHeader.GetCpSerialNumber());
                break;
            case LTPTYPE_RD_CP_EORP:
                ss << "Received Red data segment, checkpoint and End of Red Part: " << header
                   << contentHeader;
                NS_LOG_DEBUG(ss.str());
                ReportSegmentTransmission(id, contentHeader.GetCpSerialNumber());
                srecv->SetRedPartLength(contentHeader.GetOffset() + contentHeader.GetLength());
                break;
            case LTPTYPE_RD_CP_EORP_EOB:
                ss << "Received Red data segment, checkpoint and End of Red Part: " << header
                   << contentHeader;
                NS_LOG_DEBUG(ss.str());
                ReportSegmentTransmission(id, contentHeader.GetCpSerialNumber());
                srecv->SetRedPartLength(contentHeader.GetOffset() + contentHeader.GetLength());
                srecv->SetBlockFinished();
                if (srecv->IsRedPartFinished() && srecv->IsBlockFinished())
                {
                    CloseSession(id); // Stop Transmission procedure
                }
                break;
            case LTPTYPE_GD:
                ss << "Received a Green data segment : " << header << contentHeader;
                NS_LOG_DEBUG(ss.str());
                SignifyGreenPartSegmentArrival(id);
                break;
            case LTPTYPE_GD_EOB:
                ss << "Received a Green data segment, End of Block: " << header << contentHeader;
                NS_LOG_DEBUG(ss.str());
                srecv->SetBlockFinished();
                SignifyGreenPartSegmentArrival(id);

                if ((srecv->IsRedPartFinished() && srecv->IsBlockFinished()) ||
                    srecv->IsFullGreen())
                {
                    CloseSession(id); // Stop Transmission procedure
                }
                break;
            default:
                ss << "Undefined packet: " << header << contentHeader;
                NS_LOG_DEBUG(ss.str());
                // Do not enqueue = drop
                break;
            }
        }
    }
    else
    {
        switch (type)
        {
        case LTPTYPE_RS:
            ss << "Received a Report segment : " << header << contentHeader;
            NS_LOG_DEBUG(ss.str());
            ReportSegmentAckTransmission(id, contentHeader.GetRpSerialNumber(), cla);
            if (ssend) // Reports can be received in closed state.
            {
                ssend->CancelTimer(CHECKPOINT); // RS received Stop CP timer;
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
                    CloseSession(id); // Stop Transmission procedure
                }
            }
            break;
        case LTPTYPE_RAS:
            ss << "Received a Report ACK segment : " << header << contentHeader;
            NS_LOG_DEBUG(ss.str());
            srecv->CancelTimer(REPORT); // RAS received Stop Report timer;
            CheckRedPartReceived(id);
            if (srecv->IsRedPartFinished())
            {
                SignifyRedPartReception(id);
            }
            srecv->IncrementRpCurrentSerialNumber();
            if (srecv->IsRedPartFinished() && srecv->IsBlockFinished())
            {
                CloseSession(id); // Stop Transmission procedure
            }
            break;
        default:
            break;
        }
        // Other segments go here
    }

    //  NS_LOG_DEBUG (ss.str ());
}

void
LtpProtocol::ReportSegmentTransmission(SessionId id, uint64_t cpSerialNum)
{
    NS_LOG_FUNCTION(this << id << cpSerialNum);

    SessionStateRecords::iterator it = m_activeSessions.find(id);

    ConvergenceLayerAdapters::iterator itCla = m_clas.find(it->second->GetPeerLtpEngineId());

    Ptr<LtpConvergenceLayerAdapter> cla = itCla->second;
    Ptr<ReceiverSessionStateRecord> srecv = DynamicCast<ReceiverSessionStateRecord>(it->second);

    Ptr<Packet> p = Create<Packet>();
    LtpHeader header;
    LtpContentHeader contentHeader;

    header.SetSegmentType(LTPTYPE_RS);
    header.SetVersion(m_version);
    header.SetSessionId(id);

    uint32_t RpSerial = it->second->GetRpCurrentSerialNumber();
    uint32_t CpSerial = cpSerialNum;
    uint32_t upperBound = srecv->GetHighBound();
    uint32_t lowerBound = srecv->GetLowBound();

    contentHeader.SetSegmentType(LTPTYPE_RS);
    contentHeader.SetRpSerialNumber(RpSerial);
    contentHeader.SetCpSerialNumber(CpSerial);
    contentHeader.SetUpperBound(upperBound); // Size of red part to which this report pertains.
    contentHeader.SetLowerBound(lowerBound); // Size of the previous red part.

    std::set<LtpContentHeader::ReceptionClaim> claims = srecv->GetClaims(RpSerial);

    for (std::set<LtpContentHeader::ReceptionClaim>::iterator it = claims.begin();
         it != claims.end();
         ++it)
    {
        contentHeader.AddReceptionClaim(*it);
    }

    p->AddHeader(contentHeader);
    p->AddHeader(header);

    srecv->StoreClaims(contentHeader);

    srecv->Enqueue(p);
    Simulator::Schedule(m_localDelays, &LtpProtocol::Send, this, cla);
}

void
LtpProtocol::ReportSegmentAckTransmission(SessionId id,
                                          uint64_t rpSerialNum,
                                          Ptr<LtpConvergenceLayerAdapter> cla)
{
    NS_LOG_FUNCTION(this << id << rpSerialNum << cla);

    SessionStateRecords::iterator it = m_activeSessions.find(id);

    Ptr<SenderSessionStateRecord> ssr = DynamicCast<SenderSessionStateRecord>(it->second);

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
    if (ssr)
    {
        ssr->Enqueue(p);
        Simulator::Schedule(m_localDelays, &LtpProtocol::Send, this, cla);
    }
    else
    {
        // A RAS is received after the session has already been closed
        Simulator::Schedule(m_localDelays, &LtpConvergenceLayerAdapter::Send, cla, p);
    }
}

void
LtpProtocol::CheckRedPartReceived(SessionId id)
{
    NS_LOG_FUNCTION(this << id);

    SessionStateRecords::iterator it = m_activeSessions.find(id);
    Ptr<ReceiverSessionStateRecord> ssr = DynamicCast<ReceiverSessionStateRecord>(it->second);

    RedSegmentInfo info = ssr->FindMissingClaims(ssr->GetRpCurrentSerialNumber());

    if (info.claims.size() == 0)
    {
        ssr->SetRedPartFinished();
    }
}

void
LtpProtocol::RetransmitSegment(SessionId id, RedSegmentInfo info)
{
    NS_LOG_FUNCTION(this << id);

    SessionStateRecords::iterator it = m_activeSessions.find(id);
    Ptr<SenderSessionStateRecord> ssr = DynamicCast<SenderSessionStateRecord>(it->second);

    ConvergenceLayerAdapters::iterator itCla = m_clas.find(it->second->GetPeerLtpEngineId());

    Ptr<LtpConvergenceLayerAdapter> cla = itCla->second;

    if (m_cpRtxLimit > ssr->GetCpRtxNumber())
    {
        std::vector<uint8_t> rdData = ssr->GetBlockData();
        EncapsulateBlockData(ssr->GetDestination(), ssr, rdData, rdData.size(), true);
        ssr->IncrementCpRtxNumber();
        Simulator::Schedule(m_localDelays, &LtpProtocol::Send, this, cla);
    }
}

void
LtpProtocol::RetransmitReport(SessionId id, RedSegmentInfo info)
{
    NS_LOG_FUNCTION(this << id);
    SessionStateRecords::iterator it = m_activeSessions.find(id);
    Ptr<ReceiverSessionStateRecord> srecv = DynamicCast<ReceiverSessionStateRecord>(it->second);

    if (m_rpRtxLimit > srecv->GetRpRtxNumber())
    {
        ReportSegmentTransmission(id, info.CpserialNum);
    }
}

void
LtpProtocol::RetransmitCheckpoint(SessionId id, RedSegmentInfo info)
{
    NS_LOG_FUNCTION(this << id);
}

uint64_t
LtpProtocol::GetLocalEngineId() const
{
    NS_LOG_FUNCTION(this);
    return m_localEngineId;
}

Ptr<Node>
LtpProtocol::GetNode() const
{
    NS_LOG_FUNCTION(this);
    return m_node;
}

void
LtpProtocol::SetNode(Ptr<Node> node)
{
    NS_LOG_FUNCTION(this << node);
    m_node = node;
}

void
LtpProtocol::SetIpResolutionTable(Ptr<LtpIpResolutionTable> rprot)
{
    NS_LOG_FUNCTION(this << rprot);

    ConvergenceLayerAdapters::iterator it;

    for (it = m_clas.begin(); it != m_clas.end(); ++it)
    {
        it->second->SetRoutingProtocol(rprot);
    }
}

bool
LtpProtocol::AddDatalink(Ptr<LtpConvergenceLayerAdapter> link)
{
    NS_LOG_FUNCTION(this << link);

    std::pair<uint64_t, Ptr<LtpConvergenceLayerAdapter>> entry;
    entry = std::make_pair(link->GetRemoteEngineId(), link);
    std::pair<std::map<uint64_t, Ptr<LtpConvergenceLayerAdapter>>::iterator, bool> ret;
    ret = m_clas.insert(entry);

    return ret.second;
}

Ptr<LtpConvergenceLayerAdapter>
LtpProtocol::GetDatalink(uint64_t engineId)
{
    NS_LOG_FUNCTION(this << engineId);
    ConvergenceLayerAdapters::iterator it = m_clas.find(engineId);
    if (it != m_clas.end())
    {
        return it->second;
    }
    else
    {
        return 0;
    }
}

void
LtpProtocol::SetLinkStateCues(Ptr<LtpConvergenceLayerAdapter> link)
{
    NS_LOG_FUNCTION(this << link);

    link->SetCheckPointSentCallback(
        MakeCallback(&LtpProtocol::SetCheckPointTransmissionTimer, this));
    link->SetReportSentCallback(MakeCallback(&LtpProtocol::SetReportReTransmissionTimer, this));
    link->SetEndOfBlockSentCallback(MakeCallback(&LtpProtocol::SetEndOfBlockTransmission, this));
}

void
LtpProtocol::EncapsulateBlockData(uint64_t dstClientService,
                                  Ptr<SessionStateRecord> ssr,
                                  std::vector<uint8_t> data,
                                  uint64_t rdSize,
                                  bool rtx)
{
    NS_LOG_FUNCTION(this << dstClientService << ssr << rdSize << rtx);

    SessionId id = ssr->GetSessionId();

    LtpHeader header;
    uint8_t version = 0;
    SegmentType type = LTPTYPE_RD;
    uint8_t extensionCntHeader = 0b00000000;
    uint8_t extensionCntTrailer = 0b00000000;

    LtpContentHeader contentHeader;
    uint64_t clientServiceId = dstClientService;
    uint64_t offset = 0;
    uint64_t length = 0;

    ConvergenceLayerAdapters::iterator itCla = m_clas.find(ssr->GetPeerLtpEngineId());
    uint16_t mtu = itCla->second->GetMtu();

    uint64_t dataSize = data.size();
    NS_LOG_DEBUG("mtu: " << mtu << " dataSize: " << dataSize);

    /* This loop creates and enqueues all the segments corresponding to the block */
    while (offset < dataSize)
    {
        NS_LOG_DEBUG("offset: " << offset << " length:" << length);
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

        /* The Content Header includes values encoded in SDNV format
         * their size depends on the length of the data. The last operation
         * may cause the length to surpass the MTU, iterate decreasing data size
         * until it converges to the right value. */
        uint64_t size = headerSize + contentHeader.GetSerializedSize() + length;
        while (size > mtu)
        {
            length--;
            contentHeader.SetLength(length);
            size = headerSize + contentHeader.GetSerializedSize() + length;
        }

        if (rdSize == 0) /* If this a full green block*/
        {
            type = LTPTYPE_GD;
            header.SetSegmentType(type);
            contentHeader.SetSegmentType(type);
        }
        else if ((offset < rdSize) &&
                 (offset + length >
                  rdSize)) /* If it is the last segment from the red part mark it as a checkpoint*/
        {
            NS_LOG_DEBUG("Last segment from red part");

            type = LTPTYPE_RD_CP_EORP;

            /* If it is the last segment of the block */
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

            if (rdSize > 0) // Last Segment from red part smaller than MTU
            {
                length = rdSize - offset;
                contentHeader.SetLength(length);
            }

            header.SetSegmentType(type);

            contentHeader.SetSegmentType(type);
            contentHeader.SetCpSerialNumber(ssr->GetCpCurrentSerialNumber());

            type = LTPTYPE_GD; // Set type to green segment for next iteration
        }

        /* If we are on the Green part and this is the last segment */
        if ((offset > rdSize) && (offset + length >= dataSize))
        {
            type = LTPTYPE_GD_EOB;
            header.SetSegmentType(type);
            contentHeader.SetSegmentType(type);
        }

        /* Create packet of MTU size */
        std::vector<uint8_t>::const_iterator start = data.begin() + offset;
        std::vector<uint8_t>::const_iterator end =
            (offset + length > data.size()) ? data.end() : data.begin() + offset + length;
        std::vector<uint8_t> segmentData(start, end);

        Ptr<Packet> packet =
            Create<Packet>((uint8_t*)segmentData.data(), (uint32_t)segmentData.size());
        packet->AddHeader(contentHeader);
        packet->AddHeader(header);

        /* Enqueue for transmission */
        ssr->Enqueue(packet);

        offset += length;
    }
}

void
LtpProtocol::SetEndOfBlockTransmission(SessionId id)
{
    NS_LOG_FUNCTION(this << id);

    SessionStateRecords::iterator it = m_activeSessions.find(id);

    Ptr<SenderSessionStateRecord> ssend = DynamicCast<SenderSessionStateRecord>(it->second);
    ssend->SetBlockFinished();

    if (ssend->IsRedPartFinished() && ssend->IsBlockFinished())
    {
        CloseSession(id); // Stop Transmission procedure
    }
}

} // namespace ltp
} // namespace ns3
