/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */
#include "bundle-agent.h"

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

    auto ret = m_clas.insert(std::make_pair(destinationEID, cla));

    if (ret.second)
    {
        NS_LOG_DEBUG("Registered new CLA for " << destinationEID
                                               << ". Checking storage backlog...");
        ProcessBacklog(destinationEID);
    }

    return ret.second;
}

void
BundleAgent::UnregisterCla(const std::string& destinationEID)
{
    NS_LOG_FUNCTION(this << destinationEID);
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
    NS_LOG_WARN("No CLA registered for destination EID: " << eid);
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

    // BPv7 takes whole string EIDs directly
    primary.SetDestinationEID(destinationEID);
    primary.SetSourceEID(m_localEID);
    primary.SetReportToEID(reportToEID);

    // BPv7 Payload Block Setup
    PayloadBlockHeader payloadHeader;
    payloadHeader.SetBlockType(1);
    payloadHeader.SetBlockNumber(2);
    payloadHeader.SetProcFlags(0);
    payloadHeader.SetCrcType(1);
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
        NS_LOG_DEBUG("TransmitBundle: Destination is local, routing to self.");
        RecvBundle(bundle);
        return 0;
    }

    uint32_t handle = m_bundleStorageEngine->StoreBundle(bundle);
    if (handle == 0)
    {
        NS_LOG_WARN("TransmitBundle: storage full, dropping bundle to " << destinationEID);
        return 0;
    }

    EventId expiryEvent = Simulator::Schedule(ttl, &BundleAgent::ExpireBundle, this, handle);
    m_expiryEvents[handle] = expiryEvent;

    uint32_t result = ForwardBundle(handle);
    if (result != 0)
    {
        NS_LOG_DEBUG("TransmitBundle: no CLA available yet, bundle "
                     << handle << " held in storage for " << destinationEID);

        return handle;
    }
    return 0;
}

uint32_t
BundleAgent::ForwardBundle(uint32_t handle)
{
    NS_LOG_FUNCTION(this << handle);

    Ptr<Bundle> bundle = m_bundleStorageEngine->RetrieveBundle(handle);
    if (!bundle)
    {
        NS_LOG_WARN("ForwardBundle: handle " << handle << " not found in storage");
        return 1;
    }

    std::string destination = bundle->GetDestinationEID();
    Ptr<BundleCla> cla = GetClaForDestination(destination);
    if (!cla)
    {
        NS_LOG_DEBUG("ForwardBundle: no CLA for " << destination << ", bundle held");
        return 1;
    }

    Ptr<Packet> packet = bundle->Serialize();
    cla->Send(packet);

    NS_LOG_DEBUG("ForwardBundle: sent bundle handle=" << handle << " to " << destination
                                                      << " size=" << packet->GetSize());

    auto evIt = m_expiryEvents.find(handle);
    if (evIt != m_expiryEvents.end())
    {
        evIt->second.Cancel();
        m_expiryEvents.erase(evIt);
    }
    m_bundleStorageEngine->DeleteBundle(handle);
    return 0;
}

uint32_t
BundleAgent::RecvBundle(Ptr<Bundle> bundle)
{
    NS_LOG_FUNCTION(this << bundle);
    NS_ASSERT_MSG(bundle, "RecvBundle called with null bundle");

    std::string destination = bundle->GetDestinationEID();
    NS_LOG_DEBUG("RecvBundle: received bundle destined for " << destination);

    if (IsLocalDestination(destination))
    {
        NS_LOG_DEBUG("RecvBundle: delivering bundle locally");

        if (!m_receiveCallback.IsNull())
        {
            m_receiveCallback(bundle);
        }

        uint32_t handle = m_bundleStorageEngine->StoreBundle(bundle);
        if (handle == 0)
        {
            NS_LOG_WARN("RecvBundle: storage full, dropping received bundle");
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
        NS_LOG_DEBUG("RecvBundle: intermediate node, storing for forwarding to " << destination);

        uint32_t handle = m_bundleStorageEngine->StoreBundle(bundle);
        if (handle == 0)
        {
            NS_LOG_WARN("RecvBundle: storage full, cannot store bundle for forwarding");
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
            NS_LOG_DEBUG("RecvBundle: no CLA available for " << destination << ", bundle " << handle
                                                             << " held in storage");
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

    NS_LOG_DEBUG("ExpireBundle: bundle handle=" << handle << " destined for "
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
BundleAgent::GenerateStatusReport(Ptr<Bundle> bundle, uint8_t statusFlags, uint8_t reasonCode)
{
    NS_LOG_FUNCTION(this << bundle << (uint32_t)statusFlags << (uint32_t)reasonCode);
    NS_ASSERT_MSG(bundle, "GenerateStatusReport called with null bundle");

    if (bundle->IsAdminRecord())
    {
        NS_LOG_DEBUG(
            "GenerateStatusReport: Bundle is already an admin record. Suppressing report.");
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
    primary.SetVersion(7); // BPv7
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
    payloadHeader.SetBlockNumber(2);
    payloadHeader.SetCrcType(1);
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

    NS_LOG_DEBUG("ProcessBacklog: Found " << pendingHandles.size() << " bundles waiting for "
                                          << destinationEID);

    for (uint32_t handle : pendingHandles)
    {
        ForwardBundle(handle);
    }
}

} // namespace ns3
