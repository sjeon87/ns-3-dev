/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Devansh Gupta <devanshg308@gmail.com>
 */

#include "neighbor-report-element.h"

#include "ns3/address-utils.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NeighborReportElement");

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
NeighborReportElement::SetVeryHighThroughput(bool vht)
{
    if (vht)
    {
        m_bssidInfo |= (1 << 12);
    }
    else
    {
        m_bssidInfo &= ~(1 << 12);
    }
}

bool
NeighborReportElement::GetVeryHighThroughput() const
{
    return (m_bssidInfo & (1 << 12)) != 0;
}

void
NeighborReportElement::SetFtm(bool ftm)
{
    if (ftm)
    {
        m_bssidInfo |= (1 << 13);
    }
    else
    {
        m_bssidInfo &= ~(1 << 13);
    }
}

bool
NeighborReportElement::GetFtm() const
{
    return (m_bssidInfo & (1 << 13)) != 0;
}

void
NeighborReportElement::SetHighEfficiency(bool he)
{
    if (he)
    {
        m_bssidInfo |= (1 << 14);
    }
    else
    {
        m_bssidInfo &= ~(1 << 14);
    }
}

bool
NeighborReportElement::GetHighEfficiency() const
{
    return (m_bssidInfo & (1 << 14)) != 0;
}

void
NeighborReportElement::SetErBss(bool erBss)
{
    if (erBss)
    {
        m_bssidInfo |= (1 << 15);
    }
    else
    {
        m_bssidInfo &= ~(1 << 15);
    }
}

bool
NeighborReportElement::GetErBss() const
{
    return (m_bssidInfo & (1 << 15)) != 0;
}

void
NeighborReportElement::SetColocatedAp(bool colocatedAp)
{
    if (colocatedAp)
    {
        m_bssidInfo |= (1 << 16);
    }
    else
    {
        m_bssidInfo &= ~(1 << 16);
    }
}

bool
NeighborReportElement::GetColocatedAp() const
{
    return (m_bssidInfo & (1 << 16)) != 0;
}

void
NeighborReportElement::SetUnsolicitedProbeResponsesActive(bool active)
{
    if (active)
    {
        m_bssidInfo |= (1 << 17);
    }
    else
    {
        m_bssidInfo &= ~(1 << 17);
    }
}

bool
NeighborReportElement::GetUnsolicitedProbeResponsesActive() const
{
    return (m_bssidInfo & (1 << 17)) != 0;
}

void
NeighborReportElement::SetMemberOfEssWith2gOr5gColocatedAp(bool member)
{
    if (member)
    {
        m_bssidInfo |= (1 << 18);
    }
    else
    {
        m_bssidInfo &= ~(1 << 18);
    }
}

bool
NeighborReportElement::GetMemberOfEssWith2gOr5gColocatedAp() const
{
    return (m_bssidInfo & (1 << 18)) != 0;
}

void
NeighborReportElement::SetOctSupportedWithReportingAp(bool oct)
{
    if (oct)
    {
        m_bssidInfo |= (1 << 19);
    }
    else
    {
        m_bssidInfo &= ~(1 << 19);
    }
}

bool
NeighborReportElement::GetOctSupportedWithReportingAp() const
{
    return (m_bssidInfo & (1 << 19)) != 0;
}

void
NeighborReportElement::SetColocatedWith6gAp(bool colocated6g)
{
    if (colocated6g)
    {
        m_bssidInfo |= (1 << 20);
    }
    else
    {
        m_bssidInfo &= ~(1 << 20);
    }
}

bool
NeighborReportElement::GetColocatedWith6gAp() const
{
    return (m_bssidInfo & (1 << 20)) != 0;
}

void
NeighborReportElement::SetDmgPositioning(bool dmg)
{
    if (dmg)
    {
        m_bssidInfo |= (1 << 22);
    }
    else
    {
        m_bssidInfo &= ~(1 << 22);
    }
}

bool
NeighborReportElement::GetDmgPositioning() const
{
    return (m_bssidInfo & (1 << 22)) != 0;
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

void
NeighborReportElement::SetTsfInformation(uint16_t tsfOffset, uint16_t beaconInterval)
{
    m_tsfInfo = TsfInformation{tsfOffset, beaconInterval};
}

std::optional<NeighborReportElement::TsfInformation>
NeighborReportElement::GetTsfInformation() const
{
    return m_tsfInfo;
}

void
NeighborReportElement::SetCondensedCountryString(char c1, char c2)
{
    m_condensedCountryString = std::array<char, 2>{c1, c2};
}

std::optional<std::array<char, 2>>
NeighborReportElement::GetCondensedCountryString() const
{
    return m_condensedCountryString;
}

void
NeighborReportElement::SetCandidatePreference(uint8_t preference)
{
    m_candidatePreference = preference;
}

std::optional<uint8_t>
NeighborReportElement::GetCandidatePreference() const
{
    return m_candidatePreference;
}

void
NeighborReportElement::SetBssTerminationDuration(uint64_t terminationTsf, uint16_t duration)
{
    m_bssTerminationDuration = BssTerminationDuration{terminationTsf, duration};
}

std::optional<NeighborReportElement::BssTerminationDuration>
NeighborReportElement::GetBssTerminationDuration() const
{
    return m_bssTerminationDuration;
}

void
NeighborReportElement::SetVendorSpecificData(std::vector<uint8_t> data)
{
    NS_ASSERT_MSG(data.size() <= 255,
                  "Vendor specific data exceeds subelement length field (max 255 bytes)");
    m_vendorSpecific = std::move(data);
}

const std::optional<std::vector<uint8_t>>&
NeighborReportElement::GetVendorSpecificData() const
{
    return m_vendorSpecific;
}

uint16_t
NeighborReportElement::GetInformationFieldSize() const
{
    uint16_t size =
        13; // BSSID (6) + BSSID Info (4) + Operating Class (1) + Channel (1) + PHY Type (1)
    if (m_tsfInfo)
    {
        size += 2 + 4; // ID + Length + 4 bytes data
    }
    if (m_condensedCountryString)
    {
        size += 2 + 2;
    }
    if (m_candidatePreference)
    {
        size += 2 + 1;
    }
    if (m_bssTerminationDuration)
    {
        size += 2 + 10;
    }
    if (m_vendorSpecific)
    {
        size += 2 + m_vendorSpecific->size();
    }
    return size;
}

void
NeighborReportElement::SerializeInformationField(Buffer::Iterator start) const
{
    WriteTo(start, m_bssid);
    start.WriteU32(m_bssidInfo);
    start.WriteU8(m_operatingClass);
    start.WriteU8(m_channelNumber);
    start.WriteU8(m_phyType);

    if (m_tsfInfo)
    {
        start.WriteU8(1); // subelement ID
        start.WriteU8(4); // length
        start.WriteU16(m_tsfInfo->tsfOffset);
        start.WriteU16(m_tsfInfo->beaconInterval);
    }
    if (m_condensedCountryString)
    {
        start.WriteU8(2);
        start.WriteU8(2);
        start.WriteU8(static_cast<uint8_t>((*m_condensedCountryString)[0]));
        start.WriteU8(static_cast<uint8_t>((*m_condensedCountryString)[1]));
    }
    if (m_candidatePreference)
    {
        start.WriteU8(3);
        start.WriteU8(1);
        start.WriteU8(*m_candidatePreference);
    }
    if (m_bssTerminationDuration)
    {
        start.WriteU8(4);
        start.WriteU8(10);
        start.WriteU64(m_bssTerminationDuration->terminationTsf);
        start.WriteU16(m_bssTerminationDuration->duration);
    }
    if (m_vendorSpecific)
    {
        start.WriteU8(221);
        start.WriteU8(static_cast<uint8_t>(m_vendorSpecific->size()));
        for (auto byte : *m_vendorSpecific)
        {
            start.WriteU8(byte);
        }
    }
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

    uint16_t bytesRead = 13;
    while (bytesRead < length)
    {
        uint8_t subelemId = i.ReadU8();
        uint8_t subelemLen = i.ReadU8();
        bytesRead += 2;

        switch (subelemId)
        {
        case 1: { // TSF Information
            uint16_t tsfOffset = i.ReadU16();
            uint16_t beaconInterval = i.ReadU16();
            m_tsfInfo = TsfInformation{tsfOffset, beaconInterval};
            break;
        }
        case 2: { // Condensed Country String
            char c1 = static_cast<char>(i.ReadU8());
            char c2 = static_cast<char>(i.ReadU8());
            m_condensedCountryString = std::array<char, 2>{c1, c2};
            break;
        }
        case 3: { // BSS Transition Candidate Preference
            m_candidatePreference = i.ReadU8();
            break;
        }
        case 4: { // BSS Termination Duration
            uint64_t tsf = i.ReadU64();
            uint16_t duration = i.ReadU16();
            m_bssTerminationDuration = BssTerminationDuration{tsf, duration};
            break;
        }
        case 221: { // Vendor Specific
            std::vector<uint8_t> data(subelemLen);
            for (uint8_t j = 0; j < subelemLen; j++)
            {
                data[j] = i.ReadU8();
            }
            m_vendorSpecific = std::move(data);
            break;
        }
        default: // Unknown subelement, skip
            for (uint8_t j = 0; j < subelemLen; j++)
            {
                i.ReadU8();
            }
            break;
        }
        bytesRead += subelemLen;
    }

    return i.GetDistanceFrom(start);
}

void
NeighborReportElement::Print(std::ostream& os) const
{
    os << "NeighborReport=[BSSID=" << m_bssid << ", BSSIDInfo=0x" << std::hex << m_bssidInfo
       << std::dec << ", OpClass=" << +m_operatingClass << ", Channel=" << +m_channelNumber
       << ", PhyType=" << +m_phyType;
    if (m_tsfInfo)
    {
        os << ", TSF(offset=" << m_tsfInfo->tsfOffset << ", interval=" << m_tsfInfo->beaconInterval
           << ")";
    }
    if (m_condensedCountryString)
    {
        os << ", Country=" << (*m_condensedCountryString)[0] << (*m_condensedCountryString)[1];
    }
    if (m_candidatePreference)
    {
        os << ", CandPref=" << +(*m_candidatePreference);
    }
    if (m_bssTerminationDuration)
    {
        os << ", BssTermination(tsf=" << m_bssTerminationDuration->terminationTsf
           << ", dur=" << m_bssTerminationDuration->duration << ")";
    }
    if (m_vendorSpecific)
    {
        os << ", VendorSpecific(" << m_vendorSpecific->size() << " bytes)";
    }
    os << "]";
}

} // namespace ns3
