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

TypeId
NeighborReportResponseHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::NeighborReportResponseHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<NeighborReportResponseHeader>();
    return tid;
}

TypeId
NeighborReportResponseHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
NeighborReportResponseHeader::Print(std::ostream& os) const
{
    os << "dialogToken=" << +m_dialogToken
       << " neighborReportElements=" << m_neighborReportElements.size();
}

uint32_t
NeighborReportResponseHeader::GetSerializedSize() const
{
    uint32_t size = 1;
    for (const auto& elem : m_neighborReportElements)
    {
        size += elem.GetSerializedSize();
    }
    return size;
}

void
NeighborReportResponseHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(m_dialogToken);
    for (const auto& elem : m_neighborReportElements)
    {
        i = elem.Serialize(i);
    }
}

uint32_t
NeighborReportResponseHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_dialogToken = i.ReadU8();

    m_neighborReportElements.clear();
    while (true)
    {
        NeighborReportElement elem;
        Buffer::Iterator post = elem.DeserializeIfPresent(i);
        if (post.GetDistanceFrom(i) == 0)
        {
            break;
        }
        m_neighborReportElements.push_back(elem);
        i = post;
    }

    return i.GetDistanceFrom(start);
}

void
NeighborReportResponseHeader::SetDialogToken(uint8_t token)
{
    m_dialogToken = token;
}

uint8_t
NeighborReportResponseHeader::GetDialogToken() const
{
    return m_dialogToken;
}

void
NeighborReportResponseHeader::AddNeighborReportElement(const NeighborReportElement& elem)
{
    m_neighborReportElements.push_back(elem);
}

const std::vector<NeighborReportElement>&
NeighborReportResponseHeader::GetNeighborReportElements() const
{
    return m_neighborReportElements;
}

} // namespace ns3
