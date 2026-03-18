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
NeighborReportElement::SetApReachability(uint8_t reachability)
{
    m_bssidInfo &= ~0x3;
    m_bssidInfo |= (reachability & 0x3);
}

uint8_t
NeighborReportElement::GetApReachability() const
{
    return m_bssidInfo & 0x3;
}

void
NeighborReportElement::SetSecurity(bool security)
{
    if (security)
    {
        m_bssidInfo |= (1 << 2);
    }
    else
    {
        m_bssidInfo &= ~(1 << 2);
    }
}

bool
NeighborReportElement::GetSecurity() const
{
    return (m_bssidInfo & (1 << 2)) != 0;
}

void
NeighborReportElement::SetKeyScope(bool keyScope)
{
    if (keyScope)
    {
        m_bssidInfo |= (1 << 3);
    }
    else
    {
        m_bssidInfo &= ~(1 << 3);
    }
}

bool
NeighborReportElement::GetKeyScope() const
{
    return (m_bssidInfo & (1 << 3)) != 0;
}

void
NeighborReportElement::SetSpectrumManagement(bool spectrumMgmt)
{
    if (spectrumMgmt)
    {
        m_bssidInfo |= (1 << 4);
    }
    else
    {
        m_bssidInfo &= ~(1 << 4);
    }
}

bool
NeighborReportElement::GetSpectrumManagement() const
{
    return (m_bssidInfo & (1 << 4)) != 0;
}

void
NeighborReportElement::SetQos(bool qos)
{
    if (qos)
    {
        m_bssidInfo |= (1 << 5);
    }
    else
    {
        m_bssidInfo &= ~(1 << 5);
    }
}

bool
NeighborReportElement::GetQos() const
{
    return (m_bssidInfo & (1 << 5)) != 0;
}

void
NeighborReportElement::SetApsd(bool apsd)
{
    if (apsd)
    {
        m_bssidInfo |= (1 << 6);
    }
    else
    {
        m_bssidInfo &= ~(1 << 6);
    }
}

bool
NeighborReportElement::GetApsd() const
{
    return (m_bssidInfo & (1 << 6)) != 0;
}

void
NeighborReportElement::SetRadioMeasurement(bool radioMeasurement)
{
    if (radioMeasurement)
    {
        m_bssidInfo |= (1 << 7);
    }
    else
    {
        m_bssidInfo &= ~(1 << 7);
    }
}

bool
NeighborReportElement::GetRadioMeasurement() const
{
    return (m_bssidInfo & (1 << 7)) != 0;
}

void
NeighborReportElement::SetDelayedBlockAck(bool delayedBa)
{
    if (delayedBa)
    {
        m_bssidInfo |= (1 << 8);
    }
    else
    {
        m_bssidInfo &= ~(1 << 8);
    }
}

bool
NeighborReportElement::GetDelayedBlockAck() const
{
    return (m_bssidInfo & (1 << 8)) != 0;
}

void
NeighborReportElement::SetImmediateBlockAck(bool immediateBa)
{
    if (immediateBa)
    {
        m_bssidInfo |= (1 << 9);
    }
    else
    {
        m_bssidInfo &= ~(1 << 9);
    }
}

bool
NeighborReportElement::GetImmediateBlockAck() const
{
    return (m_bssidInfo & (1 << 9)) != 0;
}

void
NeighborReportElement::SetMobilityDomain(bool mobilityDomain)
{
    if (mobilityDomain)
    {
        m_bssidInfo |= (1 << 10);
    }
    else
    {
        m_bssidInfo &= ~(1 << 10);
    }
}

bool
NeighborReportElement::GetMobilityDomain() const
{
    return (m_bssidInfo & (1 << 10)) != 0;
}

void
NeighborReportElement::SetHighThroughput(bool ht)
{
    if (ht)
    {
        m_bssidInfo |= (1 << 11);
    }
    else
    {
        m_bssidInfo &= ~(1 << 11);
    }
}

bool
NeighborReportElement::GetHighThroughput() const
{
    return (m_bssidInfo & (1 << 11)) != 0;
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
