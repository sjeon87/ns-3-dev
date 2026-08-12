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
#include "bundle-agent.h"

#include "base-routing-engine.h"
#include "bundle-protocol-flags.h"
#include "generic-convergence-layer-adapter.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("BundleAgent");
NS_OBJECT_ENSURE_REGISTERED(BundleAgent);

BundleAgent::BundleAgent()
    : m_seqNumber(0)
{
    NS_LOG_FUNCTION(this);
    m_bundleStorageEngine = CreateObject<BundleStorageEngine>();
}

BundleAgent::~BundleAgent()
{
    for (auto& entry : m_expiryEvents)
    {
        entry.second.Cancel();
    }
    m_expiryEvents.clear();
    m_clas.clear();
}

TypeId
BundleAgent::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::BundleAgent")
            .SetParent<Object>()
            .SetGroupName("BundleProtocol")
            .AddConstructor<BundleAgent>()
            .AddAttribute("LocalEID",
                          "The endpoint ID of this bundle agent",
                          StringValue("dtn:node0"),
                          MakeStringAccessor(&BundleAgent::SetLocalEID, &BundleAgent::GetLocalEID),
                          MakeStringChecker())
            .AddAttribute("StorageLimit",
                          "Maximum storage capacity in bytes (0 = unlimited)",
                          UintegerValue(0),
                          MakeUintegerAccessor(&BundleAgent::SetStorageLimitFromAttribute,
                                               &BundleAgent::GetStorageEngineSize),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("FragmentationMtu",
                          "Maximum payload size in bytes per bundle fragment (0 = fragmentation "
                          "disabled)",
                          UintegerValue(0),
                          MakeUintegerAccessor(&BundleAgent::m_fragmentationMtu),
                          MakeUintegerChecker<uint32_t>());
    return tid;
}

void
BundleAgent::SetLocalEID(const std::string& eid)
{
    NS_LOG_FUNCTION(this << eid);
    if (eid.find(':') == std::string::npos)
    {
        m_localEID = "dtn:" + eid;
    }
    else
    {
        m_localEID = eid;
    }
}

void
BundleAgent::SetBundleStorageEngine(Ptr<BundleStorageEngine> bundleStorageEngine)
{
    NS_LOG_FUNCTION(this << bundleStorageEngine);
    m_bundleStorageEngine = bundleStorageEngine;
}

void
BundleAgent::SetContactGraph(Ptr<BaseRoutingEngine> contactGraph)
{
    NS_LOG_FUNCTION(this << contactGraph);
    m_contactGraph = contactGraph;
}

std::string
BundleAgent::GetLocalEID() const
{
    NS_LOG_FUNCTION(this);
    return m_localEID;
}

uint32_t
BundleAgent::GetStorageEngineSize() const
{
    NS_LOG_FUNCTION(this);
    return m_bundleStorageEngine->GetCurrentSize();
}

void
BundleAgent::SetStorageLimitFromAttribute(uint32_t limit)
{
    NS_LOG_FUNCTION(this << limit);
    m_bundleStorageEngine->SetTotalSize(limit);
}

void
BundleAgent::SetReceiveCallback(BundleReceiveCallback cb)
{
    m_receiveCallback = cb;
}

bool
BundleAgent::RegisterCla(const std::string& destinationEID, Ptr<BundleCla> cla)
{
    NS_LOG_FUNCTION(this << destinationEID << cla);
    cla->SetRxCallback(MakeCallback(&BundleAgent::RecvBundle, this));
    cla->SetTxResultCallback(MakeCallback(&BundleAgent::OnTxResult, this));

    auto ret = m_clas.insert(std::make_pair(destinationEID, cla));

    if (ret.second)
    {
        NS_LOG_INFO("[BP:Agent - " << m_localEID << "] t=" << Simulator::Now().GetSeconds()
                                   << "s: Registered new CLA for " << destinationEID
                                   << ". Checking entire storage backlog...");
        ProcessAllBacklog();
    }

    return ret.second;
}

void
BundleAgent::UnregisterCla(const std::string& destinationEID)
{
    NS_LOG_FUNCTION(this << destinationEID);
    NS_LOG_INFO("[BP:Agent - " << m_localEID << "] t=" << Simulator::Now().GetSeconds()
                               << "s: Deregistered CLA for " << destinationEID);
    m_clas.erase(destinationEID);
}

Ptr<BundleCla>
BundleAgent::GetClaForDestination(const std::string& eid) const
{
    NS_LOG_FUNCTION(this << eid);
    auto it = m_clas.find(eid);
    if (it != m_clas.end())
    {
        return it->second;
    }
    NS_LOG_WARN("[BP:Agent - " << m_localEID << "] t=" << Simulator::Now().GetSeconds()
                               << "s: No CLA Registered for " << eid);
    return nullptr;
}

bool
BundleAgent::IsLocalDestination(const std::string& eid) const
{
    NS_LOG_FUNCTION(this << eid);
    return eid == m_localEID;
}

uint32_t
BundleAgent::TransmitBundle(const std::string& destinationEID,
                            const std::string& reportToEID,
                            uint8_t* data,
                            uint32_t size,
                            Time ttl,
                            uint32_t procFlags)
{
    NS_LOG_FUNCTION(this << destinationEID << size);
    NS_ASSERT_MSG(!m_localEID.empty(), "LocalEID must be set before transmitting");

    PrimaryBlockHeader primary;
    primary.SetVersion(7);
    primary.SetProcFlags(procFlags);
    primary.SetCrcType(1);
    primary.SetCreationTime(Simulator::Now());
    primary.SetLifetime(ttl);
    primary.SetSequenceNumber(m_seqNumber++);

    primary.SetDestinationEID(destinationEID);
    primary.SetSourceEID(m_localEID);
    primary.SetReportToEID(reportToEID);

    PayloadBlockHeader payloadHeader;
    payloadHeader.SetBlockType(1);
    payloadHeader.SetBlockNumber(1);
    payloadHeader.SetProcFlags(0);
    payloadHeader.SetCrcType(0);
    payloadHeader.SetBlockLength(size);

    Ptr<Packet> payload = Create<Packet>(data, size);

    Ptr<PrimaryBlock> primaryBlock = CreateObject<PrimaryBlock>();
    primaryBlock->GetHeader() = primary;

    Ptr<PayloadBlock> payloadBlock = CreateObject<PayloadBlock>();
    payloadBlock->GetHeader() = payloadHeader;
    payloadBlock->SetPayload(payload);

    Ptr<Bundle> bundle = CreateObject<Bundle>();
    bundle->AddBlock(primaryBlock);
    bundle->AddBlock(payloadBlock);

    if (IsLocalDestination(destinationEID))
    {
        NS_LOG_INFO("[BP:Agent - " << m_localEID << "] t=" << Simulator::Now().GetSeconds()
                                   << "s: TransmitBundle: Destination is local, routing to self.");
        RecvBundle(bundle);
        return 0;
    }

    bool shouldFragment =
        m_fragmentationMtu > 0 && size > m_fragmentationMtu && !(procFlags & (1 << NO_FRAGMENT));

    std::vector<Ptr<Bundle>> bundlesToSend;
    if (shouldFragment)
    {
        bundlesToSend = Bundle::Fragment(bundle, m_fragmentationMtu);
        NS_LOG_INFO("[BP:Agent - " << m_localEID << "] t=" << Simulator::Now().GetSeconds()
                                   << "s: TransmitBundle: fragmented bundle to " << destinationEID
                                   << " into " << bundlesToSend.size() << " fragments");
    }
    else
    {
        bundlesToSend.push_back(bundle);
    }

    uint32_t firstHandle = 0;
    for (const auto& toSend : bundlesToSend)
    {
        uint32_t handle = m_bundleStorageEngine->StoreBundle(toSend);
        if (handle == 0)
        {
            NS_LOG_WARN("[BP:Agent - " << m_localEID << "] t=" << Simulator::Now().GetSeconds()
                                       << "s: TransmitBundle: storage full, dropping bundle to "
                                       << destinationEID);
            continue;
        }

        EventId expiryEvent = Simulator::Schedule(ttl, &BundleAgent::ExpireBundle, this, handle);
        m_expiryEvents[handle] = expiryEvent;

        if (firstHandle == 0)
        {
            firstHandle = handle;
        }

        if (ForwardBundle(handle) != 0)
        {
            NS_LOG_INFO("[BP:Agent - " << m_localEID << "] t=" << Simulator::Now().GetSeconds()
                                       << "s: TransmitBundle: no CLA available yet, bundle "
                                       << handle << " held in storage for " << destinationEID);
        }
    }

    return firstHandle;
}

uint32_t
BundleAgent::ForwardBundle(uint32_t handle)
{
    NS_LOG_FUNCTION(this << handle);

    if (m_inTransit.find(handle) != m_inTransit.end())
    {
        return 0;
    }

    Ptr<Bundle> bundle = m_bundleStorageEngine->RetrieveBundle(handle);
    if (!bundle)
    {
        NS_LOG_WARN("[BP:Agent - " << m_localEID << "] t=" << Simulator::Now().GetSeconds()
                                   << "s: ForwardBundle: handle " << handle
                                   << " not found in storage");
        return 1;
    }

    std::string destination = bundle->GetDestinationEID();

    if (!m_contactGraph)
    {
        NS_LOG_WARN("[BP:Agent - " << m_localEID << "] t=" << Simulator::Now().GetSeconds()
                                   << "s: ForwardBundle: No ContactGraph set! Cannot route.");
        return 1;
    }

    std::string nextHopEID = m_contactGraph->GetNextHop(bundle, m_localEID);

    if (nextHopEID.empty())
    {
        NS_LOG_INFO("[BP:Agent - " << m_localEID << "] t=" << Simulator::Now().GetSeconds()
                                   << "s: ForwardBundle: No active route to " << destination
                                   << " yet. Bundle held.");
        return 1;
    }

    Ptr<BundleCla> cla = GetClaForDestination(nextHopEID);
    if (!cla)
    {
        NS_LOG_INFO("[BP:Agent - " << m_localEID << "] t=" << Simulator::Now().GetSeconds()
                                   << "s: ForwardBundle: no CLA for next hop " << nextHopEID
                                   << ", bundle held");
        return 1;
    }

    PrimaryBlockHeader& primaryHeader = bundle->GetPrimaryBlock()->GetHeader();
    primaryHeader.SetHopCount(primaryHeader.GetHopCount() + 1);

    Ptr<Packet> packet = bundle->Serialize();
    uint32_t bundleSize = packet->GetSize();
    m_inTransit.insert(handle);
    cla->Send(packet, handle);

    NS_LOG_INFO("[BP:Agent - " << m_localEID << "] t=" << Simulator::Now().GetSeconds()
                               << "s: ForwardBundle: sent bundle handle=" << handle
                               << " to final destination " << destination << " via next hop "
                               << nextHopEID << " size=" << bundleSize
                               << " bytes, hopCount=" << primaryHeader.GetHopCount());
    m_contactGraph->ReserveVolume(m_localEID, nextHopEID, bundleSize);

    return 0;
}

uint32_t
BundleAgent::RecvBundle(Ptr<Bundle> bundle)
{
    NS_LOG_FUNCTION(this << bundle);
    NS_ASSERT_MSG(bundle, "RecvBundle called with null bundle");

    std::string destination = bundle->GetDestinationEID();
    NS_LOG_INFO("[BP:Agent - " << m_localEID << "] t=" << Simulator::Now().GetSeconds()
                               << "s: RecvBundle: received bundle destined for " << destination);

    if (IsLocalDestination(destination))
    {
        uint32_t procFlags = bundle->GetPrimaryBlock()->GetHeader().GetProcFlags();
        if (procFlags & (1 << IS_FRG))
        {
            Ptr<Bundle> reassembled = TryReassembleFragment(bundle);
            if (!reassembled)
            {
                NS_LOG_INFO("[BP:Agent - "
                            << m_localEID << "] t=" << Simulator::Now().GetSeconds()
                            << "s: RecvBundle: buffered fragment, awaiting remainder");
                return 0;
            }
            bundle = reassembled;
        }

        NS_LOG_INFO("[BP:Agent - " << m_localEID << "] t=" << Simulator::Now().GetSeconds()
                                   << "s: RecvBundle: delivering bundle locally, hopCount="
                                   << bundle->GetHopCount());

        if (!m_receiveCallback.IsNull())
        {
            m_receiveCallback(bundle);
        }

        uint32_t handle = m_bundleStorageEngine->StoreBundle(bundle);
        if (handle == 0)
        {
            NS_LOG_WARN("[BP:Agent - " << m_localEID << "] t=" << Simulator::Now().GetSeconds()
                                       << "s: RecvBundle: storage full, dropping received bundle");
            return 1;
        }

        uint32_t flags = bundle->GetPrimaryBlock()->GetHeader().GetProcFlags();
        if (flags & (1 << REQ_REP_RECV))
        {
            Ptr<Bundle> report = GenerateStatusReport(bundle, (1 << RECVD_BUNDLE), SR_NO_INFO);
            if (report)
            {
                uint32_t reportHandle = m_bundleStorageEngine->StoreBundle(report);
                ForwardBundle(reportHandle);
            }
        }

        m_bundleStorageEngine->DeleteBundle(handle);
        return 0;
    }
    else
    {
        NS_LOG_INFO("[BP:Agent - " << m_localEID << "] t=" << Simulator::Now().GetSeconds()
                                   << "s: RecvBundle: intermediate node, storing for forwarding to "
                                   << destination);

        uint32_t handle = m_bundleStorageEngine->StoreBundle(bundle);
        if (handle == 0)
        {
            NS_LOG_WARN("[BP:Agent - "
                        << m_localEID << "] t=" << Simulator::Now().GetSeconds()
                        << "s: RecvBundle: storage full, cannot store bundle for forwarding");
            return 1;
        }

        Time remaining = bundle->GetExpiry() - Simulator::Now();
        if (remaining <= Time(0))
        {
            ExpireBundle(handle);
            return 0;
        }
        EventId expiryEvent =
            Simulator::Schedule(remaining, &BundleAgent::ExpireBundle, this, handle);
        m_expiryEvents[handle] = expiryEvent;

        if (ForwardBundle(handle) != 0)
        {
            NS_LOG_INFO("[BP:Agent - " << m_localEID << "] t=" << Simulator::Now().GetSeconds()
                                       << "s: RecvBundle: no CLA available for " << destination
                                       << ", bundle " << handle << " held in storage");
        }
        return 0;
    }
}

uint32_t
BundleAgent::ExpireBundle(uint32_t handle)
{
    NS_LOG_FUNCTION(this << handle);

    Ptr<Bundle> bundle = m_bundleStorageEngine->RetrieveBundle(handle);
    if (!bundle)
    {
        return 0;
    }

    NS_LOG_INFO("[BP:Agent - " << m_localEID << "] t=" << Simulator::Now().GetSeconds()
                               << "s: ExpireBundle: bundle handle=" << handle << " destined for "
                               << bundle->GetDestinationEID() << " has expired");

    uint32_t flags = bundle->GetPrimaryBlock()->GetHeader().GetProcFlags();

    if (flags & (1 << REQ_REP_DEL))
    {
        Ptr<Bundle> report = GenerateStatusReport(bundle, (1 << DEL_BUNDLE), SR_LIFE_EXPIRE);
        if (report)
        {
            uint32_t reportHandle = m_bundleStorageEngine->StoreBundle(report);
            ForwardBundle(reportHandle);
        }
    }

    m_expiryEvents.erase(handle);
    m_bundleStorageEngine->DeleteBundle(handle);
    return 0;
}

Ptr<Bundle>
BundleAgent::TryReassembleFragment(Ptr<Bundle> fragment)
{
    NS_LOG_FUNCTION(this << fragment);

    Ptr<PrimaryBlock> primary = fragment->GetPrimaryBlock();
    const PrimaryBlockHeader& header = primary->GetHeader();
    FragmentKey key(header.GetSourceEID(),
                    header.GetCreationTime().GetTimeStep(),
                    header.GetSequenceNumber());

    auto it = m_fragmentBuffers.find(key);
    if (it == m_fragmentBuffers.end())
    {
        Time remaining = fragment->GetExpiry() - Simulator::Now();
        if (remaining <= Time(0))
        {
            NS_LOG_INFO("[BP:Agent - "
                        << m_localEID << "] t=" << Simulator::Now().GetSeconds()
                        << "s: TryReassembleFragment: fragment already expired, dropping");
            return nullptr;
        }

        FragmentAssembly assembly;
        assembly.totalLength = header.GetTotalAppDataLength();
        assembly.templatePrimary = primary;
        assembly.expiryEvent =
            Simulator::Schedule(remaining, &BundleAgent::ExpireFragmentBuffer, this, key);
        it = m_fragmentBuffers.emplace(key, std::move(assembly)).first;
    }

    FragmentAssembly& assembly = it->second;
    Ptr<PayloadBlock> payloadBlock = fragment->GetPayloadBlock();
    assembly.pieces[header.GetFragmentOffset()] = payloadBlock->GetPayload();

    uint32_t covered = 0;
    for (const auto& piece : assembly.pieces)
    {
        if (piece.first != covered)
        {
            return nullptr;
        }
        covered += piece.second->GetSize();
    }

    if (covered < assembly.totalLength)
    {
        return nullptr;
    }

    Ptr<Packet> fullPayload = Create<Packet>();
    for (const auto& piece : assembly.pieces)
    {
        fullPayload->AddAtEnd(piece.second);
    }

    PrimaryBlockHeader finalHeader = assembly.templatePrimary->GetHeader();
    finalHeader.SetProcFlags(finalHeader.GetProcFlags() & ~(1 << IS_FRG));
    finalHeader.SetFragmentOffset(0);
    finalHeader.SetTotalAppDataLength(0);

    Ptr<PrimaryBlock> finalPrimary = CreateObject<PrimaryBlock>();
    finalPrimary->GetHeader() = finalHeader;

    PayloadBlockHeader finalPayloadHeader;
    finalPayloadHeader.SetBlockType(1);
    finalPayloadHeader.SetBlockNumber(1);
    finalPayloadHeader.SetCrcType(0);
    finalPayloadHeader.SetBlockLength(fullPayload->GetSize());

    Ptr<PayloadBlock> finalPayloadBlock = CreateObject<PayloadBlock>();
    finalPayloadBlock->GetHeader() = finalPayloadHeader;
    finalPayloadBlock->SetPayload(fullPayload);

    Ptr<Bundle> reassembled = CreateObject<Bundle>();
    reassembled->AddBlock(finalPrimary);
    reassembled->AddBlock(finalPayloadBlock);

    assembly.expiryEvent.Cancel();
    m_fragmentBuffers.erase(it);

    return reassembled;
}

void
BundleAgent::ExpireFragmentBuffer(FragmentKey key)
{
    NS_LOG_FUNCTION(this);

    auto it = m_fragmentBuffers.find(key);
    if (it != m_fragmentBuffers.end())
    {
        NS_LOG_INFO("[BP:Agent - " << m_localEID << "] t=" << Simulator::Now().GetSeconds()
                                   << "s: ExpireFragmentBuffer: dropping incomplete fragment set");
        m_fragmentBuffers.erase(it);
    }
}

Ptr<Bundle>
BundleAgent::GenerateStatusReport(Ptr<Bundle> bundle, uint8_t statusFlags, uint8_t reasonCode)
{
    NS_LOG_FUNCTION(this << bundle << (uint32_t)statusFlags << (uint32_t)reasonCode);
    NS_ASSERT_MSG(bundle, "GenerateStatusReport called with null bundle");

    if (bundle->IsAdminRecord())
    {
        NS_LOG_INFO(
            "[BP:Agent - "
            << m_localEID << "] t=" << Simulator::Now().GetSeconds()
            << "s: GenerateStatusReport: Bundle is already an admin record. Suppressing report.");
        return nullptr;
    }

    BundleStatusReport report;
    report.SetStatusFlags(statusFlags);
    report.SetReasonCode(reasonCode);
    report.SetSourceEID(bundle->GetSourceEID());
    report.SetCreationTime(bundle->GetPrimaryBlock()->GetHeader().GetCreationTime());
    report.SetSequenceNumber(bundle->GetPrimaryBlock()->GetHeader().GetSequenceNumber());

    Time now = Simulator::Now();
    if (statusFlags & (1 << RECVD_BUNDLE))
    {
        report.SetBundleReceiptTime(now);
    }
    if (statusFlags & (1 << FWD_BUNDLE))
    {
        report.SetBundleForwardTime(now);
    }
    if (statusFlags & (1 << DELIVER_BUNDLE))
    {
        report.SetBundleDeliveryTime(now);
    }
    if (statusFlags & (1 << DEL_BUNDLE))
    {
        report.SetBundleDeletionTime(now);
    }

    Ptr<Packet> reportPayload = Create<Packet>();
    reportPayload->AddHeader(report);

    std::string reportDestination = bundle->GetReportToEID();

    PrimaryBlockHeader primary;
    primary.SetVersion(7);
    primary.SetProcFlags(1 << ADMIN_RECORD);
    primary.SetCrcType(1);
    primary.SetCreationTime(now);
    primary.SetLifetime(Seconds(3600));
    primary.SetSequenceNumber(m_seqNumber++);
    primary.SetDestinationEID(reportDestination);
    primary.SetSourceEID(m_localEID);
    primary.SetReportToEID("dtn:none");

    PayloadBlockHeader payloadHeader;
    payloadHeader.SetBlockType(1);
    payloadHeader.SetBlockNumber(1);
    payloadHeader.SetCrcType(0);
    payloadHeader.SetBlockLength(reportPayload->GetSize());

    Ptr<PrimaryBlock> reportPrimaryBlock = CreateObject<PrimaryBlock>();
    reportPrimaryBlock->GetHeader() = primary;

    Ptr<PayloadBlock> reportPayloadBlock = CreateObject<PayloadBlock>();
    reportPayloadBlock->GetHeader() = payloadHeader;
    reportPayloadBlock->SetPayload(reportPayload);

    Ptr<Bundle> reportBundle = CreateObject<Bundle>();
    reportBundle->AddBlock(reportPrimaryBlock);
    reportBundle->AddBlock(reportPayloadBlock);

    return reportBundle;
}

void
BundleAgent::ProcessBacklog(const std::string& destinationEID)
{
    NS_LOG_FUNCTION(this << destinationEID);

    std::vector<uint32_t> pendingHandles =
        m_bundleStorageEngine->GetHandlesForDestination(destinationEID);

    if (pendingHandles.empty())
    {
        return;
    }

    NS_LOG_INFO("[BP:Agent - " << m_localEID << "] t=" << Simulator::Now().GetSeconds()
                               << "s: ProcessBacklog: Found " << pendingHandles.size()
                               << " bundles waiting for " << destinationEID << " from "
                               << m_localEID);

    for (uint32_t handle : pendingHandles)
    {
        ForwardBundle(handle);
    }
}

void
BundleAgent::ProcessAllBacklog()
{
    std::vector<uint32_t> pendingHandles = m_bundleStorageEngine->GetAllHandles();

    if (!pendingHandles.empty())
    {
        NS_LOG_INFO("[BP:Agent - " << m_localEID << "] t=" << Simulator::Now().GetSeconds()
                                   << "s: ProcessAllBacklog: Attempting to forward "
                                   << pendingHandles.size() << " stored bundles.");
        for (uint32_t handle : pendingHandles)
        {
            ForwardBundle(handle);
        }
    }

    // m_backlogCheckEvent = Simulator::Schedule(Seconds(10.0), &BundleAgent::ProcessAllBacklog,
    // this);
}

void
BundleAgent::OnTxResult(uint32_t handle, bool success)
{
    m_inTransit.erase(handle);

    if (success)
    {
        NS_LOG_INFO("[BP:Agent - " << m_localEID << "] t=" << Simulator::Now().GetSeconds()
                                   << "s: Delivery confirmed for handle " << handle);
        auto evIt = m_expiryEvents.find(handle);
        if (evIt != m_expiryEvents.end())
        {
            evIt->second.Cancel();
            m_expiryEvents.erase(evIt);
        }
        m_bundleStorageEngine->DeleteBundle(handle);
    }
    else
    {
        NS_LOG_WARN("[BP:Agent - " << m_localEID << "] t=" << Simulator::Now().GetSeconds()
                                   << "s: Tx failed for handle " << handle
                                   << ". Retained in storage.");
    }
}

} // namespace ns3
