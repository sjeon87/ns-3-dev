/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */
#include "bundle.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Bundle");
NS_OBJECT_ENSURE_REGISTERED(Bundle);

Bundle::Bundle()
    : m_payload(nullptr)
{
    NS_LOG_FUNCTION(this);
}

Bundle::~Bundle()
{
    NS_LOG_FUNCTION(this);
    m_payload = nullptr;
}

TypeId
Bundle::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Bundle")
                            .SetParent<Object>()
                            .SetGroupName("BundleProtocol")
                            .AddConstructor<Bundle>();
    return tid;
}

TypeId
Bundle::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
Bundle::SetPrimaryHeader(PrimaryBlockHeader h)
{
    NS_LOG_FUNCTION(this);
    m_primaryHeader = h;
}

PrimaryBlockHeader
Bundle::GetPrimaryHeader() const
{
    NS_LOG_FUNCTION(this);
    return m_primaryHeader;
}

void
Bundle::SetPayloadHeader(PayloadBlockHeader h)
{
    NS_LOG_FUNCTION(this);
    m_payloadHeader = h;
}

PayloadBlockHeader
Bundle::GetPayloadHeader() const
{
    NS_LOG_FUNCTION(this);
    return m_payloadHeader;
}

void
Bundle::SetPayload(Ptr<Packet> payload)
{
    NS_LOG_FUNCTION(this << payload);
    m_payload = payload;
}

Ptr<Packet>
Bundle::GetPayload() const
{
    NS_LOG_FUNCTION(this);
    return m_payload;
}

uint32_t
Bundle::GetTotalSize() const
{
    NS_LOG_FUNCTION(this);
    uint32_t size = 0;
    size += m_primaryHeader.GetSerializedSize();
    size += m_payloadHeader.GetSerializedSize();
    if (m_payload)
    {
        size += m_payload->GetSize();
    }
    return size;
}

Ptr<Packet>
Bundle::Serialize() const
{
    NS_LOG_FUNCTION(this);

    Ptr<Packet> bundle;
    if (m_payload)
    {
        bundle = m_payload->Copy();
    }
    else
    {
        bundle = Create<Packet>();
    }
    bundle->AddHeader(m_payloadHeader);
    bundle->AddHeader(m_primaryHeader);
    return bundle;
}

void
Bundle::Deserialize(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this << p);
    NS_ASSERT_MSG(p, "Bundle::Deserialize called with null packet");

    Ptr<Packet> copy = p->Copy();

    uint32_t bytes = copy->RemoveHeader(m_primaryHeader);
    NS_ASSERT_MSG(bytes == m_primaryHeader.GetSerializedSize(),
                  "Primary header deserialization size mismatch");

    bytes = copy->RemoveHeader(m_payloadHeader);
    NS_ASSERT_MSG(bytes == m_payloadHeader.GetSerializedSize(),
                  "Payload header deserialization size mismatch");

    m_payload = copy;
}

Time
Bundle::GetExpiry() const
{
    NS_LOG_FUNCTION(this);
    return m_primaryHeader.GetCreationTime() + m_primaryHeader.GetTTL();
}

std::string
Bundle::GetDestinationEID() const
{
    NS_LOG_FUNCTION(this);
    const std::string& dict = m_primaryHeader.GetDictionary();
    std::string scheme = dict.substr(m_primaryHeader.GetDestinationSchemeOffset());
    scheme = scheme.substr(0, scheme.find('\0'));
    std::string ssp = dict.substr(m_primaryHeader.GetDestinationSSPOffset());
    ssp = ssp.substr(0, ssp.find('\0'));
    return scheme + ":" + ssp;
}

std::string
Bundle::GetSourceEID() const
{
    NS_LOG_FUNCTION(this);
    const std::string& dict = m_primaryHeader.GetDictionary();
    std::string scheme = dict.substr(m_primaryHeader.GetSourceSchemeOffset());
    scheme = scheme.substr(0, scheme.find('\0'));
    std::string ssp = dict.substr(m_primaryHeader.GetSourceSSPOffset());
    ssp = ssp.substr(0, ssp.find('\0'));
    return scheme + ":" + ssp;
}

bool
Bundle::isAdminRecord() const
{
    NS_LOG_FUNCTION(this);
    return (m_primaryHeader.GetProcFlags() >> ADMIN_RECORD) & 0x1;
}

} // namespace ns3