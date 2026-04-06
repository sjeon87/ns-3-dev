/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */
#include "bundle-agent.h"
#include "generic-convergence-layer-adapter.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"

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
    NS_LOG_FUNCTION(this);
    m_clas.clear();
    m_expiryEvents.clear();
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
                          StringValue("dtn://node0/"),
                          MakeStringAccessor(&BundleAgent::m_localEID),
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
    m_localEID = eid;
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

bool
BundleAgent::RegisterCla(const std::string& destinationEID, Ptr<BundleCla> cla)
{
    NS_LOG_FUNCTION(this << destinationEID << cla);
    auto ret = m_clas.insert(std::make_pair(destinationEID, cla));
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
    primary.SetVersion(6);
    primary.SetProcFlags(procFlags);
    primary.SetCreationTime(Simulator::Now());
    primary.SetTTL(ttl);
    primary.SetSequenceNumber(m_seqNumber++);
    primary.SetDestinationEID("dtn", destinationEID);
    primary.SetSourceEID("dtn", m_localEID);
    primary.SetReportToEID("dtn", reportToEID);
    primary.SetCustodianEID("dtn", "none");  
    PayloadBlockHeader payloadHeader;
    payloadHeader.SetBlockType(1);
    payloadHeader.SetProcFlags(0);
    payloadHeader.SetBlockLength(size);

    Ptr<Packet> payload = Create<Packet>(data, size);

    Ptr<Bundle> bundle = CreateObject<Bundle>();
    bundle->SetPrimaryHeader(primary);
    bundle->SetPayloadHeader(payloadHeader);
    bundle->SetPayload(payload);

    uint32_t handle = m_bundleStorageEngine->StoreBundle(bundle);
    if (handle == 0)
    {
        NS_LOG_WARN("TransmitBundle: storage full, dropping bundle to " << destinationEID);
        return 0;
    }

    EventId expiryEvent = Simulator::Schedule(ttl,
                                               &BundleAgent::ExpireBundle,
                                               this,
                                               handle);
    m_expiryEvents[handle] = expiryEvent;

    uint32_t result = ForwardBundle(handle);
    if (result != 0)
    {
        NS_LOG_DEBUG("TransmitBundle: no CLA available yet, bundle "
                     << handle << " held in storage for " << destinationEID);
    }

    return handle;
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

    NS_LOG_DEBUG("ForwardBundle: sent bundle handle=" << handle
                 << " to " << destination
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

        uint32_t handle = m_bundleStorageEngine->StoreBundle(bundle);
        if (handle == 0)
        {
            NS_LOG_WARN("RecvBundle: storage full, dropping received bundle");
            return 1;
        }

        uint32_t flags = bundle->GetPrimaryHeader().GetProcFlags();
        if ((flags >> BUNDLE_RECEPTION) & 0x1)
        {
            Ptr<Bundle> report = GenerateStatusReport(bundle,
                                                       1 << RECVD_BUNDLE,
                                                       SR_NO_INFO);
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

        Time ttl = bundle->GetPrimaryHeader().GetTTL();
        EventId expiryEvent = Simulator::Schedule(ttl,
                                                   &BundleAgent::ExpireBundle,
                                                   this,
                                                   handle);
        m_expiryEvents[handle] = expiryEvent;

        ForwardBundle(handle);
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

    NS_LOG_DEBUG("ExpireBundle: bundle handle=" << handle
                 << " destined for " << bundle->GetDestinationEID()
                 << " has expired");

    uint32_t flags = bundle->GetPrimaryHeader().GetProcFlags();
    if ((flags >> BUNDLE_DELETION) & 0x1)
    {
        Ptr<Bundle> report = GenerateStatusReport(bundle,
                                                   1 << DEL_BUNDLE,
                                                   SR_LIFE_EXPIRE);
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
BundleAgent::GenerateStatusReport(Ptr<Bundle> bundle,
                                   uint8_t statusFlags,
                                   uint8_t reasonCode)
{
    NS_LOG_FUNCTION(this << bundle << (uint32_t)statusFlags << (uint32_t)reasonCode);
    NS_ASSERT_MSG(bundle, "GenerateStatusReport called with null bundle");

    BundleStatusReport report;
    report.SetStatusFlags(statusFlags);
    report.SetReasonCode(reasonCode);
    report.SetCreationTime(bundle->GetPrimaryHeader().GetCreationTime());
    report.SetSequenceNumber(bundle->GetPrimaryHeader().GetSequenceNumber());

    Time now = Simulator::Now();
    if (statusFlags & (1 << RECVD_BUNDLE))  { report.SetBundleReceiptTime(now); }
    if (statusFlags & (1 << FWD_BUNDLE))    { report.SetBundleForwardTime(now); }
    if (statusFlags & (1 << DELIVER_BUNDLE)){ report.SetBundleDeliveryTime(now); }
    if (statusFlags & (1 << DEL_BUNDLE))    { report.SetBundleReceiptTime(now); }

    Ptr<Packet> reportPayload = Create<Packet>();
    reportPayload->AddHeader(report);

    std::string reportDestination = bundle->GetDestinationEID(); 
    PrimaryBlockHeader primary;
    primary.SetVersion(6);
    primary.SetProcFlags(1 << ADMIN_RECORD);  
    primary.SetCreationTime(now);
    primary.SetTTL(Seconds(3600));            
    primary.SetSequenceNumber(m_seqNumber++);
    primary.SetDestinationEID("dtn", reportDestination);
    primary.SetSourceEID("dtn", m_localEID);
    primary.SetReportToEID("dtn", "none");
    primary.SetCustodianEID("dtn", "none");

    PayloadBlockHeader payloadHeader;
    payloadHeader.SetBlockType(1);
    payloadHeader.SetBlockLength(reportPayload->GetSize());

    Ptr<Bundle> reportBundle = CreateObject<Bundle>();
    reportBundle->SetPrimaryHeader(primary);
    reportBundle->SetPayloadHeader(payloadHeader);
    reportBundle->SetPayload(reportPayload);

    return reportBundle;
}

} // namespace ns3