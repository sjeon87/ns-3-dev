/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Devansh Gupta <devanshg308@gmail.com>
 */

#include "neighbor-report.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NeighborReport");

TypeId
NeighborReportRequestHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::NeighborReportRequestHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<NeighborReportRequestHeader>();
    return tid;
}

TypeId
NeighborReportRequestHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
NeighborReportRequestHeader::Print(std::ostream& os) const
{
    os << "dialogToken=" << +m_dialogToken;
    if (m_ssid)
    {
        os << " ssid=";
        m_ssid->Print(os);
    }
}

uint32_t
NeighborReportRequestHeader::GetSerializedSize() const
{
    uint32_t size = 1;
    if (m_ssid)
    {
        size += m_ssid->GetSerializedSize();
    }
    return size;
}

void
NeighborReportRequestHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(m_dialogToken);
    if (m_ssid)
    {
        i = m_ssid->Serialize(i);
    }
}

uint32_t
NeighborReportRequestHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_dialogToken = i.ReadU8();

    Ssid ssid;
    Buffer::Iterator post = ssid.DeserializeIfPresent(i);
    if (post.GetDistanceFrom(i) > 0)
    {
        m_ssid = ssid;
        i = post;
    }

    return i.GetDistanceFrom(start);
}

void
NeighborReportRequestHeader::SetDialogToken(uint8_t token)
{
    m_dialogToken = token;
}

uint8_t
NeighborReportRequestHeader::GetDialogToken() const
{
    return m_dialogToken;
}

void
NeighborReportRequestHeader::SetSsid(const Ssid& ssid)
{
    m_ssid = ssid;
}

const std::optional<Ssid>&
NeighborReportRequestHeader::GetSsid() const
{
    return m_ssid;
}

bool
NeighborReportRequestHeader::HasSsid() const
{
    return m_ssid.has_value();
}

} // namespace ns3
