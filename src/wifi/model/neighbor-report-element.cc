/*
 * Copyright (c) 2026
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
NeighborReportElement::SetBit(BssidInfoBit bit, bool val)
{
    auto pos = static_cast<uint8_t>(bit);
    if (val)
    {
        m_bssidInfo |= (1U << pos);
    }
    else
    {
        m_bssidInfo &= ~(1U << pos);
    }
}

bool
NeighborReportElement::GetBit(BssidInfoBit bit) const
{
    return (m_bssidInfo & (1U << static_cast<uint8_t>(bit))) != 0;
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
    SetBit(BssidInfoBit::SECURITY, security);
}

bool
NeighborReportElement::GetSecurity() const
{
    return GetBit(BssidInfoBit::SECURITY);
}

void
NeighborReportElement::SetKeyScope(bool keyScope)
{
    SetBit(BssidInfoBit::KEY_SCOPE, keyScope);
}

bool
NeighborReportElement::GetKeyScope() const
{
    return GetBit(BssidInfoBit::KEY_SCOPE);
}

void
NeighborReportElement::SetSpectrumManagement(bool spectrumMgmt)
{
    SetBit(BssidInfoBit::SPECTRUM_MANAGEMENT, spectrumMgmt);
}

bool
NeighborReportElement::GetSpectrumManagement() const
{
    return GetBit(BssidInfoBit::SPECTRUM_MANAGEMENT);
}

void
NeighborReportElement::SetQos(bool qos)
{
    SetBit(BssidInfoBit::QOS, qos);
}

bool
NeighborReportElement::GetQos() const
{
    return GetBit(BssidInfoBit::QOS);
}

void
NeighborReportElement::SetApsd(bool apsd)
{
    SetBit(BssidInfoBit::APSD, apsd);
}

bool
NeighborReportElement::GetApsd() const
{
    return GetBit(BssidInfoBit::APSD);
}

void
NeighborReportElement::SetRadioMeasurement(bool radioMeasurement)
{
    SetBit(BssidInfoBit::RADIO_MEASUREMENT, radioMeasurement);
}

bool
NeighborReportElement::GetRadioMeasurement() const
{
    return GetBit(BssidInfoBit::RADIO_MEASUREMENT);
}

void
NeighborReportElement::SetMobilityDomain(bool mobilityDomain)
{
    SetBit(BssidInfoBit::MOBILITY_DOMAIN, mobilityDomain);
}

bool
NeighborReportElement::GetMobilityDomain() const
{
    return GetBit(BssidInfoBit::MOBILITY_DOMAIN);
}

void
NeighborReportElement::SetHighThroughput(bool ht)
{
    SetBit(BssidInfoBit::HIGH_THROUGHPUT, ht);
}

bool
NeighborReportElement::GetHighThroughput() const
{
    return GetBit(BssidInfoBit::HIGH_THROUGHPUT);
}

void
NeighborReportElement::SetVeryHighThroughput(bool vht)
{
    SetBit(BssidInfoBit::VERY_HIGH_THROUGHPUT, vht);
}

bool
NeighborReportElement::GetVeryHighThroughput() const
{
    return GetBit(BssidInfoBit::VERY_HIGH_THROUGHPUT);
}

void
NeighborReportElement::SetFtm(bool ftm)
{
    SetBit(BssidInfoBit::FTM, ftm);
}

bool
NeighborReportElement::GetFtm() const
{
    return GetBit(BssidInfoBit::FTM);
}

void
NeighborReportElement::SetHighEfficiency(bool he)
{
    SetBit(BssidInfoBit::HIGH_EFFICIENCY, he);
}

bool
NeighborReportElement::GetHighEfficiency() const
{
    return GetBit(BssidInfoBit::HIGH_EFFICIENCY);
}

void
NeighborReportElement::SetErBss(bool erBss)
{
    SetBit(BssidInfoBit::ER_BSS, erBss);
}

bool
NeighborReportElement::GetErBss() const
{
    return GetBit(BssidInfoBit::ER_BSS);
}

void
NeighborReportElement::SetColocatedAp(bool colocatedAp)
{
    SetBit(BssidInfoBit::COLOCATED_AP, colocatedAp);
}

bool
NeighborReportElement::GetColocatedAp() const
{
    return GetBit(BssidInfoBit::COLOCATED_AP);
}

void
NeighborReportElement::SetUnsolicitedProbeResponsesActive(bool active)
{
    SetBit(BssidInfoBit::UNSOLICITED_PROBE_RESPONSES_ACTIVE, active);
}

bool
NeighborReportElement::GetUnsolicitedProbeResponsesActive() const
{
    return GetBit(BssidInfoBit::UNSOLICITED_PROBE_RESPONSES_ACTIVE);
}

void
NeighborReportElement::SetMemberOfEssWith2gOr5gColocatedAp(bool member)
{
    SetBit(BssidInfoBit::MEMBER_OF_ESS_WITH_2G_OR_5G_COLOCATED_AP, member);
}

bool
NeighborReportElement::GetMemberOfEssWith2gOr5gColocatedAp() const
{
    return GetBit(BssidInfoBit::MEMBER_OF_ESS_WITH_2G_OR_5G_COLOCATED_AP);
}

void
NeighborReportElement::SetOctSupportedWithReportingAp(bool oct)
{
    SetBit(BssidInfoBit::OCT_SUPPORTED_WITH_REPORTING_AP, oct);
}

bool
NeighborReportElement::GetOctSupportedWithReportingAp() const
{
    return GetBit(BssidInfoBit::OCT_SUPPORTED_WITH_REPORTING_AP);
}

void
NeighborReportElement::SetColocatedWith6gAp(bool colocated6g)
{
    SetBit(BssidInfoBit::COLOCATED_WITH_6G_AP, colocated6g);
}

bool
NeighborReportElement::GetColocatedWith6gAp() const
{
    return GetBit(BssidInfoBit::COLOCATED_WITH_6G_AP);
}

void
NeighborReportElement::SetDmgPositioning(bool dmg)
{
    SetBit(BssidInfoBit::DMG_POSITIONING, dmg);
}

bool
NeighborReportElement::GetDmgPositioning() const
{
    return GetBit(BssidInfoBit::DMG_POSITIONING);
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
NeighborReportElement::TsfInformation::GetSerializedSize() const
{
    return SUBELEMENT_HEADER_SIZE + sizeof(tsfOffset) + sizeof(beaconInterval);
}

uint16_t
NeighborReportElement::CondensedCountryString::GetSerializedSize() const
{
    return SUBELEMENT_HEADER_SIZE + sizeof(c1) + sizeof(c2);
}

uint16_t
NeighborReportElement::CandidatePreference::GetSerializedSize() const
{
    return SUBELEMENT_HEADER_SIZE + sizeof(preference);
}

uint16_t
NeighborReportElement::BssTerminationDuration::GetSerializedSize() const
{
    return SUBELEMENT_HEADER_SIZE + sizeof(terminationTsf) + sizeof(duration);
}

uint16_t
NeighborReportElement::Bearing::GetSerializedSize() const
{
    return SUBELEMENT_HEADER_SIZE + sizeof(bearing) + sizeof(distance) + sizeof(relativeHeight);
}

uint16_t
NeighborReportElement::WideBandwidthChannel::GetSerializedSize() const
{
    return SUBELEMENT_HEADER_SIZE + sizeof(channelWidth) + sizeof(centerFreqSegment0) +
           sizeof(centerFreqSegment1);
}

uint16_t
NeighborReportElement::VendorSpecificData::GetSerializedSize() const
{
    return SUBELEMENT_HEADER_SIZE + static_cast<uint16_t>(data.size());
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
    m_condensedCountryString = CondensedCountryString{c1, c2};
}

std::optional<NeighborReportElement::CondensedCountryString>
NeighborReportElement::GetCondensedCountryString() const
{
    return m_condensedCountryString;
}

void
NeighborReportElement::SetCandidatePreference(uint8_t preference)
{
    m_candidatePreference = CandidatePreference{preference};
}

std::optional<NeighborReportElement::CandidatePreference>
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
NeighborReportElement::SetBearing(uint16_t bearing, uint32_t distance, int16_t relativeHeight)
{
    m_bearing = Bearing{bearing, distance, relativeHeight};
}

std::optional<NeighborReportElement::Bearing>
NeighborReportElement::GetBearing() const
{
    return m_bearing;
}

void
NeighborReportElement::SetWideBandwidthChannel(uint8_t channelWidth,
                                               uint8_t centerFreqSegment0,
                                               uint8_t centerFreqSegment1)
{
    m_wideBandwidth = WideBandwidthChannel{channelWidth, centerFreqSegment0, centerFreqSegment1};
}

std::optional<NeighborReportElement::WideBandwidthChannel>
NeighborReportElement::GetWideBandwidthChannel() const
{
    return m_wideBandwidth;
}

void
NeighborReportElement::SetHtCapabilities(const HtCapabilities& htCapabilities)
{
    m_htCapabilities = htCapabilities;
}

std::optional<HtCapabilities>
NeighborReportElement::GetHtCapabilities() const
{
    return m_htCapabilities;
}

void
NeighborReportElement::SetHtOperation(const HtOperation& htOperation)
{
    m_htOperation = htOperation;
}

std::optional<HtOperation>
NeighborReportElement::GetHtOperation() const
{
    return m_htOperation;
}

void
NeighborReportElement::SetVhtCapabilities(const VhtCapabilities& vhtCapabilities)
{
    m_vhtCapabilities = vhtCapabilities;
}

std::optional<VhtCapabilities>
NeighborReportElement::GetVhtCapabilities() const
{
    return m_vhtCapabilities;
}

void
NeighborReportElement::SetVhtOperation(const VhtOperation& vhtOperation)
{
    m_vhtOperation = vhtOperation;
}

std::optional<VhtOperation>
NeighborReportElement::GetVhtOperation() const
{
    return m_vhtOperation;
}

void
NeighborReportElement::SetVendorSpecificData(std::vector<uint8_t> data)
{
    NS_ASSERT_MSG(data.size() <= 255,
                  "Vendor specific data exceeds subelement length field (max 255 bytes)");
    m_vendorSpecific = VendorSpecificData{std::move(data)};
}

std::optional<NeighborReportElement::VendorSpecificData>
NeighborReportElement::GetVendorSpecificData() const
{
    return m_vendorSpecific;
}

uint16_t
NeighborReportElement::GetInformationFieldSize() const
{
    uint16_t size = 6 /* BSSID (Mac48Address) */ + sizeof(m_bssidInfo) + sizeof(m_operatingClass) +
                    sizeof(m_channelNumber) + sizeof(m_phyType);
    if (m_tsfInfo)
    {
        size += m_tsfInfo->GetSerializedSize();
    }
    if (m_condensedCountryString)
    {
        size += m_condensedCountryString->GetSerializedSize();
    }
    if (m_candidatePreference)
    {
        size += m_candidatePreference->GetSerializedSize();
    }
    if (m_bssTerminationDuration)
    {
        size += m_bssTerminationDuration->GetSerializedSize();
    }
    if (m_bearing)
    {
        size += m_bearing->GetSerializedSize();
    }
    if (m_wideBandwidth)
    {
        size += m_wideBandwidth->GetSerializedSize();
    }
    if (m_htCapabilities)
    {
        size += m_htCapabilities->GetSerializedSize();
    }
    if (m_htOperation)
    {
        size += m_htOperation->GetSerializedSize();
    }
    if (m_vhtCapabilities)
    {
        size += m_vhtCapabilities->GetSerializedSize();
    }
    if (m_vhtOperation)
    {
        size += m_vhtOperation->GetSerializedSize();
    }
    if (m_vendorSpecific)
    {
        size += m_vendorSpecific->GetSerializedSize();
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
        start.WriteU8(static_cast<uint8_t>(SubelementId::TSF_INFORMATION));
        start.WriteU8(sizeof(m_tsfInfo->tsfOffset) + sizeof(m_tsfInfo->beaconInterval));
        start.WriteU16(m_tsfInfo->tsfOffset);
        start.WriteU16(m_tsfInfo->beaconInterval);
    }
    if (m_condensedCountryString)
    {
        start.WriteU8(static_cast<uint8_t>(SubelementId::CONDENSED_COUNTRY_STRING));
        start.WriteU8(sizeof(m_condensedCountryString->c1) + sizeof(m_condensedCountryString->c2));
        start.WriteU8(static_cast<uint8_t>(m_condensedCountryString->c1));
        start.WriteU8(static_cast<uint8_t>(m_condensedCountryString->c2));
    }
    if (m_candidatePreference)
    {
        start.WriteU8(static_cast<uint8_t>(SubelementId::BSS_TRANSITION_CANDIDATE_PREFERENCE));
        start.WriteU8(sizeof(m_candidatePreference->preference));
        start.WriteU8(m_candidatePreference->preference);
    }
    if (m_bssTerminationDuration)
    {
        start.WriteU8(static_cast<uint8_t>(SubelementId::BSS_TERMINATION_DURATION));
        start.WriteU8(sizeof(m_bssTerminationDuration->terminationTsf) +
                      sizeof(m_bssTerminationDuration->duration));
        start.WriteU64(m_bssTerminationDuration->terminationTsf);
        start.WriteU16(m_bssTerminationDuration->duration);
    }
    if (m_bearing)
    {
        start.WriteU8(static_cast<uint8_t>(SubelementId::BEARING));
        start.WriteU8(sizeof(m_bearing->bearing) + sizeof(m_bearing->distance) +
                      sizeof(m_bearing->relativeHeight));
        start.WriteU16(m_bearing->bearing);
        start.WriteU32(m_bearing->distance);
        start.WriteU16(static_cast<uint16_t>(m_bearing->relativeHeight));
    }
    if (m_wideBandwidth)
    {
        start.WriteU8(static_cast<uint8_t>(SubelementId::WIDE_BANDWIDTH_CHANNEL));
        start.WriteU8(sizeof(m_wideBandwidth->channelWidth) +
                      sizeof(m_wideBandwidth->centerFreqSegment0) +
                      sizeof(m_wideBandwidth->centerFreqSegment1));
        start.WriteU8(m_wideBandwidth->channelWidth);
        start.WriteU8(m_wideBandwidth->centerFreqSegment0);
        start.WriteU8(m_wideBandwidth->centerFreqSegment1);
    }
    if (m_htCapabilities)
    {
        start = m_htCapabilities->Serialize(start);
    }
    if (m_htOperation)
    {
        start = m_htOperation->Serialize(start);
    }
    if (m_vhtCapabilities)
    {
        start = m_vhtCapabilities->Serialize(start);
    }
    if (m_vhtOperation)
    {
        start = m_vhtOperation->Serialize(start);
    }
    if (m_vendorSpecific)
    {
        start.WriteU8(static_cast<uint8_t>(SubelementId::VENDOR_SPECIFIC));
        start.WriteU8(static_cast<uint8_t>(m_vendorSpecific->data.size()));
        for (auto byte : m_vendorSpecific->data)
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

    uint16_t bytesRead = 6 /* BSSID (Mac48Address) */ + sizeof(m_bssidInfo) +
                         sizeof(m_operatingClass) + sizeof(m_channelNumber) + sizeof(m_phyType);
    while (bytesRead < length)
    {
        uint8_t subelemId = i.PeekU8();

        // IE-format subelements: subelement ID == IE Element ID,
        // so DeserializeIfPresent can parse them directly.
        bool ieHandled = true;
        Buffer::Iterator before = i;
        switch (subelemId)
        {
        case IE_HT_CAPABILITIES: {
            HtCapabilities htCap;
            i = htCap.DeserializeIfPresent(i);
            m_htCapabilities = htCap;
            break;
        }
        case IE_HT_OPERATION: {
            HtOperation htOp;
            i = htOp.DeserializeIfPresent(i);
            m_htOperation = htOp;
            break;
        }
        case IE_VHT_CAPABILITIES: {
            VhtCapabilities vhtCap;
            i = vhtCap.DeserializeIfPresent(i);
            m_vhtCapabilities = vhtCap;
            break;
        }
        case IE_VHT_OPERATION: {
            VhtOperation vhtOp;
            i = vhtOp.DeserializeIfPresent(i);
            m_vhtOperation = vhtOp;
            break;
        }
        default:
            ieHandled = false;
            break;
        }

        if (ieHandled)
        {
            bytesRead += i.GetDistanceFrom(before);
            continue;
        }

        // Manual subelements: read ID + length + data
        subelemId = i.ReadU8();
        uint8_t subelemLen = i.ReadU8();
        bytesRead += 2;

        switch (subelemId)
        {
        case static_cast<uint8_t>(SubelementId::TSF_INFORMATION): {
            uint16_t tsfOffset = i.ReadU16();
            uint16_t beaconInterval = i.ReadU16();
            m_tsfInfo = TsfInformation{tsfOffset, beaconInterval};
            break;
        }
        case static_cast<uint8_t>(SubelementId::CONDENSED_COUNTRY_STRING): {
            char c1 = static_cast<char>(i.ReadU8());
            char c2 = static_cast<char>(i.ReadU8());
            m_condensedCountryString = CondensedCountryString{c1, c2};
            break;
        }
        case static_cast<uint8_t>(SubelementId::BSS_TRANSITION_CANDIDATE_PREFERENCE): {
            m_candidatePreference = CandidatePreference{i.ReadU8()};
            break;
        }
        case static_cast<uint8_t>(SubelementId::BSS_TERMINATION_DURATION): {
            uint64_t tsf = i.ReadU64();
            uint16_t duration = i.ReadU16();
            m_bssTerminationDuration = BssTerminationDuration{tsf, duration};
            break;
        }
        case static_cast<uint8_t>(SubelementId::BEARING): {
            uint16_t bearing = i.ReadU16();
            uint32_t distance = i.ReadU32();
            auto relativeHeight = static_cast<int16_t>(i.ReadU16());
            m_bearing = Bearing{bearing, distance, relativeHeight};
            break;
        }
        case static_cast<uint8_t>(SubelementId::WIDE_BANDWIDTH_CHANNEL): {
            uint8_t chWidth = i.ReadU8();
            uint8_t seg0 = i.ReadU8();
            uint8_t seg1 = i.ReadU8();
            m_wideBandwidth = WideBandwidthChannel{chWidth, seg0, seg1};
            break;
        }
        case static_cast<uint8_t>(SubelementId::VENDOR_SPECIFIC): {
            std::vector<uint8_t> vsData(subelemLen);
            for (uint8_t j = 0; j < subelemLen; j++)
            {
                vsData[j] = i.ReadU8();
            }
            m_vendorSpecific = VendorSpecificData{std::move(vsData)};
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
        os << ", Country=" << m_condensedCountryString->c1 << m_condensedCountryString->c2;
    }
    if (m_candidatePreference)
    {
        os << ", CandPref=" << +m_candidatePreference->preference;
    }
    if (m_bssTerminationDuration)
    {
        os << ", BssTermination(tsf=" << m_bssTerminationDuration->terminationTsf
           << ", dur=" << m_bssTerminationDuration->duration << ")";
    }
    if (m_bearing)
    {
        os << ", Bearing(deg=" << m_bearing->bearing << ", dist=0x" << std::hex
           << m_bearing->distance << std::dec << ", height=" << m_bearing->relativeHeight << ")";
    }
    if (m_wideBandwidth)
    {
        os << ", WBC(width=" << +m_wideBandwidth->channelWidth
           << ", seg0=" << +m_wideBandwidth->centerFreqSegment0
           << ", seg1=" << +m_wideBandwidth->centerFreqSegment1 << ")";
    }
    if (m_htCapabilities)
    {
        os << ", HtCapabilities";
    }
    if (m_htOperation)
    {
        os << ", HtOperation";
    }
    if (m_vhtCapabilities)
    {
        os << ", VhtCapabilities";
    }
    if (m_vhtOperation)
    {
        os << ", VhtOperation";
    }
    if (m_vendorSpecific)
    {
        os << ", VendorSpecific(" << m_vendorSpecific->data.size() << " bytes)";
    }
    os << "]";
}

} // namespace ns3
