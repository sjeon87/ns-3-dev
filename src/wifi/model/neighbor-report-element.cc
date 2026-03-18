/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Devansh Gupta <devanshg308@gmail.com>
 */

#include "neighbor-report-element.h"

#include "ns3/address-utils.h"

namespace ns3
{

NeighborReportElement::NeighborReportElement()
    : m_bssid(Mac48Address()),
      m_bssidInfo(0),
      m_operatingClass(0),
      m_channelNumber(0),
      m_phyType(0)
{
}

WifiInformationElementId
NeighborReportElement::ElementId() const
{
    return IE_NEIGHBOR_REPORT;
}

void
NeighborReportElement::SetBssid(Mac48Address bssid)
{
    m_bssid = bssid;
}

Mac48Address
NeighborReportElement::GetBssid() const
{
    return m_bssid;
}

void
NeighborReportElement::SetBssidInfo(uint32_t info)
{
    m_bssidInfo = info;
}

uint32_t
NeighborReportElement::GetBssidInfo() const
{
    return m_bssidInfo;
}

void
NeighborReportElement::SetOperatingClass(uint8_t operatingClass)
{
    m_operatingClass = operatingClass;
}

uint8_t
NeighborReportElement::GetOperatingClass() const
{
    return m_operatingClass;
}

void
NeighborReportElement::SetChannelNumber(uint8_t channel)
{
    m_channelNumber = channel;
}

uint8_t
NeighborReportElement::GetChannelNumber() const
{
    return m_channelNumber;
}

void
NeighborReportElement::SetPhyType(uint8_t phyType)
{
    m_phyType = phyType;
}

uint8_t
NeighborReportElement::GetPhyType() const
{
    return m_phyType;
}

uint16_t
NeighborReportElement::GetInformationFieldSize() const
{
    // TODO: add optional subelements size once they are supported
    return 13; // BSSID (6) + BSSID Info (4) + Operating Class (1) + Channel (1) + PHY Type (1)
}

void
NeighborReportElement::SerializeInformationField(Buffer::Iterator start) const
{
    WriteTo(start, m_bssid);
    start.WriteU32(m_bssidInfo);
    start.WriteU8(m_operatingClass);
    start.WriteU8(m_channelNumber);
    start.WriteU8(m_phyType);
    // TODO: serialize optional subelements here
}

uint16_t
NeighborReportElement::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    Buffer::Iterator i = start;
    ReadFrom(i, m_bssid);
    m_bssidInfo = i.ReadU32();
    m_operatingClass = i.ReadU8();
    m_channelNumber = i.ReadU8();
    m_phyType = i.ReadU8();
    // TODO: deserialize optional subelements from remaining bytes (length - 13)
    return i.GetDistanceFrom(start);
}

void
NeighborReportElement::Print(std::ostream& os) const
{
    os << "NeighborReport=[BSSID=" << m_bssid << ", BSSIDInfo=0x" << std::hex << m_bssidInfo
       << std::dec << ", OpClass=" << +m_operatingClass << ", Channel=" << +m_channelNumber
       << ", PhyType=" << +m_phyType << "]";
}

} // namespace ns3
