/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Devansh Gupta <devanshg308@gmail.com>
 */

#include "measurement-report-element.h"

#include "ns3/address-utils.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MeasurementReportElement");

// --- BeaconReport ---

uint16_t
BeaconReport::GetSerializedSize() const
{
    uint16_t size = SERIALIZED_SIZE;
    if (m_reportedFrameBody)
    {
        size += 2 + static_cast<uint16_t>(m_reportedFrameBody->size());
    }
    if (m_reportedFrameBodyFragmentId)
    {
        size += 2 + 2;
    }
    if (m_wideBandwidthChannelSwitch)
    {
        size += 2 + 3;
    }
    if (m_lastBeaconReportIndication)
    {
        size += 2 + 1;
    }
    if (m_vendorSpecific)
    {
        size += 2 + static_cast<uint16_t>(m_vendorSpecific->size());
    }
    return size;
}

void
BeaconReport::Serialize(Buffer::Iterator& start) const
{
    start.WriteU8(m_operatingClass);
    start.WriteU8(m_channelNumber);
    start.WriteU64(m_actualMeasurementStartTime);
    start.WriteU16(m_measurementDuration);
    start.WriteU8(m_reportedFrameInfo);
    start.WriteU8(m_rcpi);
    start.WriteU8(m_rsni);
    WriteTo(start, m_bssid);
    start.WriteU8(m_antennaId);
    start.WriteU32(m_parentTsf);

    if (m_reportedFrameBody)
    {
        start.WriteU8(static_cast<uint8_t>(SubelementId::REPORTED_FRAME_BODY));
        start.WriteU8(static_cast<uint8_t>(m_reportedFrameBody->size()));
        for (uint8_t b : *m_reportedFrameBody)
        {
            start.WriteU8(b);
        }
    }
    if (m_reportedFrameBodyFragmentId)
    {
        start.WriteU8(static_cast<uint8_t>(SubelementId::REPORTED_FRAME_BODY_FRAGMENT_ID));
        start.WriteU8(2);
        const auto& f = *m_reportedFrameBodyFragmentId;
        uint16_t packed = static_cast<uint16_t>(f.beaconReportId) |
                          (static_cast<uint16_t>(f.fragmentIdNumber & 0x7F) << 8) |
                          (f.moreFrameBodyFragments ? (1U << 15) : 0U);
        start.WriteU16(packed);
    }
    if (m_wideBandwidthChannelSwitch)
    {
        start.WriteU8(static_cast<uint8_t>(SubelementId::WIDE_BANDWIDTH_CHANNEL_SWITCH));
        start.WriteU8(3);
        start.WriteU8(m_wideBandwidthChannelSwitch->newChannelWidth);
        start.WriteU8(m_wideBandwidthChannelSwitch->newChannelCenterFreqSeg0);
        start.WriteU8(m_wideBandwidthChannelSwitch->newChannelCenterFreqSeg1);
    }
    if (m_lastBeaconReportIndication)
    {
        start.WriteU8(static_cast<uint8_t>(SubelementId::LAST_BEACON_REPORT_INDICATION));
        start.WriteU8(1);
        start.WriteU8(*m_lastBeaconReportIndication ? 1 : 0);
    }
    if (m_vendorSpecific)
    {
        start.WriteU8(static_cast<uint8_t>(SubelementId::VENDOR_SPECIFIC));
        start.WriteU8(static_cast<uint8_t>(m_vendorSpecific->size()));
        for (uint8_t b : *m_vendorSpecific)
        {
            start.WriteU8(b);
        }
    }
}

uint16_t
BeaconReport::Deserialize(Buffer::Iterator& start, uint16_t length)
{
    m_operatingClass = start.ReadU8();
    m_channelNumber = start.ReadU8();
    m_actualMeasurementStartTime = start.ReadU64();
    m_measurementDuration = start.ReadU16();
    m_reportedFrameInfo = start.ReadU8();
    m_rcpi = start.ReadU8();
    m_rsni = start.ReadU8();
    ReadFrom(start, m_bssid);
    m_antennaId = start.ReadU8();
    m_parentTsf = start.ReadU32();
    uint16_t bytesRead = SERIALIZED_SIZE;

    while (bytesRead + 2 <= length)
    {
        uint8_t subId = start.ReadU8();
        uint8_t subLen = start.ReadU8();
        bytesRead += 2;
        if (subId == static_cast<uint8_t>(SubelementId::REPORTED_FRAME_BODY))
        {
            std::vector<uint8_t> body(subLen);
            for (uint8_t j = 0; j < subLen; j++)
            {
                body[j] = start.ReadU8();
            }
            m_reportedFrameBody = std::move(body);
            bytesRead += subLen;
        }
        else if (subId == static_cast<uint8_t>(SubelementId::REPORTED_FRAME_BODY_FRAGMENT_ID))
        {
            uint16_t packed = start.ReadU16();
            ReportedFrameBodyFragmentId f;
            f.beaconReportId = static_cast<uint8_t>(packed & 0xFF);
            f.fragmentIdNumber = static_cast<uint8_t>((packed >> 8) & 0x7F);
            f.moreFrameBodyFragments = (packed & (1U << 15)) != 0;
            m_reportedFrameBodyFragmentId = f;
            bytesRead += 2;
        }
        else if (subId == static_cast<uint8_t>(SubelementId::WIDE_BANDWIDTH_CHANNEL_SWITCH))
        {
            WideBandwidthChannelSwitch wbc;
            wbc.newChannelWidth = start.ReadU8();
            wbc.newChannelCenterFreqSeg0 = start.ReadU8();
            wbc.newChannelCenterFreqSeg1 = start.ReadU8();
            m_wideBandwidthChannelSwitch = wbc;
            bytesRead += subLen;
        }
        else if (subId == static_cast<uint8_t>(SubelementId::LAST_BEACON_REPORT_INDICATION))
        {
            m_lastBeaconReportIndication = (start.ReadU8() != 0);
            bytesRead += subLen;
        }
        else if (subId == static_cast<uint8_t>(SubelementId::VENDOR_SPECIFIC))
        {
            std::vector<uint8_t> data(subLen);
            for (uint8_t j = 0; j < subLen; j++)
            {
                data[j] = start.ReadU8();
            }
            m_vendorSpecific = std::move(data);
            bytesRead += subLen;
        }
        else
        {
            for (uint8_t j = 0; j < subLen; j++)
            {
                start.ReadU8();
            }
            bytesRead += subLen;
        }
    }

    return bytesRead;
}

void
BeaconReport::SetOperatingClass(uint8_t operatingClass)
{
    m_operatingClass = operatingClass;
}

uint8_t
BeaconReport::GetOperatingClass() const
{
    return m_operatingClass;
}

void
BeaconReport::SetChannelNumber(uint8_t channel)
{
    m_channelNumber = channel;
}

uint8_t
BeaconReport::GetChannelNumber() const
{
    return m_channelNumber;
}

void
BeaconReport::SetActualMeasurementStartTime(uint64_t startTime)
{
    m_actualMeasurementStartTime = startTime;
}

uint64_t
BeaconReport::GetActualMeasurementStartTime() const
{
    return m_actualMeasurementStartTime;
}

void
BeaconReport::SetMeasurementDuration(uint16_t duration)
{
    m_measurementDuration = duration;
}

uint16_t
BeaconReport::GetMeasurementDuration() const
{
    return m_measurementDuration;
}

void
BeaconReport::SetReportedFrameInformation(uint8_t condensedPhyType, bool reportedFrameType)
{
    m_reportedFrameInfo = (condensedPhyType & 0x7F) | (reportedFrameType ? (1 << 7) : 0);
}

uint8_t
BeaconReport::GetCondensedPhyType() const
{
    return m_reportedFrameInfo & 0x7F;
}

bool
BeaconReport::GetReportedFrameType() const
{
    return (m_reportedFrameInfo & (1 << 7)) != 0;
}

void
BeaconReport::SetRcpi(uint8_t rcpi)
{
    m_rcpi = rcpi;
}

uint8_t
BeaconReport::GetRcpi() const
{
    return m_rcpi;
}

void
BeaconReport::SetRsni(uint8_t rsni)
{
    m_rsni = rsni;
}

uint8_t
BeaconReport::GetRsni() const
{
    return m_rsni;
}

void
BeaconReport::SetBssid(Mac48Address bssid)
{
    m_bssid = bssid;
}

Mac48Address
BeaconReport::GetBssid() const
{
    return m_bssid;
}

void
BeaconReport::SetAntennaId(uint8_t antennaId)
{
    m_antennaId = antennaId;
}

uint8_t
BeaconReport::GetAntennaId() const
{
    return m_antennaId;
}

void
BeaconReport::SetParentTsf(uint32_t parentTsf)
{
    m_parentTsf = parentTsf;
}

uint32_t
BeaconReport::GetParentTsf() const
{
    return m_parentTsf;
}

void
BeaconReport::SetReportedFrameBody(const std::vector<uint8_t>& body)
{
    m_reportedFrameBody = body;
}

std::optional<std::vector<uint8_t>>
BeaconReport::GetReportedFrameBody() const
{
    return m_reportedFrameBody;
}

void
BeaconReport::SetReportedFrameBodyFragmentId(const ReportedFrameBodyFragmentId& fragId)
{
    m_reportedFrameBodyFragmentId = fragId;
}

std::optional<BeaconReport::ReportedFrameBodyFragmentId>
BeaconReport::GetReportedFrameBodyFragmentId() const
{
    return m_reportedFrameBodyFragmentId;
}

void
BeaconReport::SetWideBandwidthChannelSwitch(const WideBandwidthChannelSwitch& wbc)
{
    m_wideBandwidthChannelSwitch = wbc;
}

std::optional<BeaconReport::WideBandwidthChannelSwitch>
BeaconReport::GetWideBandwidthChannelSwitch() const
{
    return m_wideBandwidthChannelSwitch;
}

void
BeaconReport::SetLastBeaconReportIndication(bool indication)
{
    m_lastBeaconReportIndication = indication;
}

std::optional<bool>
BeaconReport::GetLastBeaconReportIndication() const
{
    return m_lastBeaconReportIndication;
}

void
BeaconReport::SetVendorSpecific(const std::vector<uint8_t>& data)
{
    m_vendorSpecific = data;
}

std::optional<std::vector<uint8_t>>
BeaconReport::GetVendorSpecific() const
{
    return m_vendorSpecific;
}

// --- ChannelLoadReport ---

uint16_t
ChannelLoadReport::GetSerializedSize() const
{
    uint16_t size = FIXED_FIELDS_SIZE;
    if (m_wideBandwidthChannelSwitch)
    {
        size += 2 + 3;
    }
    if (m_vendorSpecific)
    {
        size += 2 + static_cast<uint16_t>(m_vendorSpecific->size());
    }
    return size;
}

void
ChannelLoadReport::Serialize(Buffer::Iterator& start) const
{
    start.WriteU8(m_operatingClass);
    start.WriteU8(m_channelNumber);
    start.WriteU64(m_actualMeasurementStartTime);
    start.WriteU16(m_measurementDuration);
    start.WriteU8(m_channelLoad);
    if (m_wideBandwidthChannelSwitch)
    {
        start.WriteU8(static_cast<uint8_t>(SubelementId::WIDE_BANDWIDTH_CHANNEL_SWITCH));
        start.WriteU8(3);
        start.WriteU8(m_wideBandwidthChannelSwitch->newChannelWidth);
        start.WriteU8(m_wideBandwidthChannelSwitch->newChannelCenterFreqSeg0);
        start.WriteU8(m_wideBandwidthChannelSwitch->newChannelCenterFreqSeg1);
    }
    if (m_vendorSpecific)
    {
        start.WriteU8(static_cast<uint8_t>(SubelementId::VENDOR_SPECIFIC));
        start.WriteU8(static_cast<uint8_t>(m_vendorSpecific->size()));
        for (uint8_t b : *m_vendorSpecific)
        {
            start.WriteU8(b);
        }
    }
}

uint16_t
ChannelLoadReport::Deserialize(Buffer::Iterator& start, uint16_t length)
{
    m_operatingClass = start.ReadU8();
    m_channelNumber = start.ReadU8();
    m_actualMeasurementStartTime = start.ReadU64();
    m_measurementDuration = start.ReadU16();
    m_channelLoad = start.ReadU8();
    uint16_t bytesRead = FIXED_FIELDS_SIZE;

    while (bytesRead + 2 <= length)
    {
        uint8_t subId = start.ReadU8();
        uint8_t subLen = start.ReadU8();
        bytesRead += 2;
        if (subId == static_cast<uint8_t>(SubelementId::WIDE_BANDWIDTH_CHANNEL_SWITCH))
        {
            BeaconReport::WideBandwidthChannelSwitch wbc;
            wbc.newChannelWidth = start.ReadU8();
            wbc.newChannelCenterFreqSeg0 = start.ReadU8();
            wbc.newChannelCenterFreqSeg1 = start.ReadU8();
            m_wideBandwidthChannelSwitch = wbc;
            bytesRead += subLen;
        }
        else if (subId == static_cast<uint8_t>(SubelementId::VENDOR_SPECIFIC))
        {
            std::vector<uint8_t> data(subLen);
            for (uint8_t j = 0; j < subLen; j++)
            {
                data[j] = start.ReadU8();
            }
            m_vendorSpecific = std::move(data);
            bytesRead += subLen;
        }
        else
        {
            for (uint8_t j = 0; j < subLen; j++)
            {
                start.ReadU8();
            }
            bytesRead += subLen;
        }
    }

    return bytesRead;
}

void
ChannelLoadReport::SetOperatingClass(uint8_t operatingClass)
{
    m_operatingClass = operatingClass;
}

uint8_t
ChannelLoadReport::GetOperatingClass() const
{
    return m_operatingClass;
}

void
ChannelLoadReport::SetChannelNumber(uint8_t channel)
{
    m_channelNumber = channel;
}

uint8_t
ChannelLoadReport::GetChannelNumber() const
{
    return m_channelNumber;
}

void
ChannelLoadReport::SetActualMeasurementStartTime(uint64_t startTime)
{
    m_actualMeasurementStartTime = startTime;
}

uint64_t
ChannelLoadReport::GetActualMeasurementStartTime() const
{
    return m_actualMeasurementStartTime;
}

void
ChannelLoadReport::SetMeasurementDuration(uint16_t duration)
{
    m_measurementDuration = duration;
}

uint16_t
ChannelLoadReport::GetMeasurementDuration() const
{
    return m_measurementDuration;
}

void
ChannelLoadReport::SetChannelLoad(uint8_t load)
{
    m_channelLoad = load;
}

uint8_t
ChannelLoadReport::GetChannelLoad() const
{
    return m_channelLoad;
}

void
ChannelLoadReport::SetWideBandwidthChannelSwitch(
    const BeaconReport::WideBandwidthChannelSwitch& wbc)
{
    m_wideBandwidthChannelSwitch = wbc;
}

std::optional<BeaconReport::WideBandwidthChannelSwitch>
ChannelLoadReport::GetWideBandwidthChannelSwitch() const
{
    return m_wideBandwidthChannelSwitch;
}

void
ChannelLoadReport::SetVendorSpecific(const std::vector<uint8_t>& data)
{
    m_vendorSpecific = data;
}

std::optional<std::vector<uint8_t>>
ChannelLoadReport::GetVendorSpecific() const
{
    return m_vendorSpecific;
}

// --- NoiseHistogramReport ---

uint16_t
NoiseHistogramReport::GetSerializedSize() const
{
    uint16_t size = FIXED_FIELDS_SIZE;
    if (m_wideBandwidthChannelSwitch)
    {
        size += 2 + 3;
    }
    if (m_vendorSpecific)
    {
        size += 2 + static_cast<uint16_t>(m_vendorSpecific->size());
    }
    return size;
}

void
NoiseHistogramReport::Serialize(Buffer::Iterator& start) const
{
    start.WriteU8(m_operatingClass);
    start.WriteU8(m_channelNumber);
    start.WriteU64(m_actualMeasurementStartTime);
    start.WriteU16(m_measurementDuration);
    start.WriteU8(m_antennaId);
    start.WriteU8(m_anpi);
    for (uint8_t i = 0; i < 11; i++)
    {
        start.WriteU8(m_ipiDensities[i]);
    }
    if (m_wideBandwidthChannelSwitch)
    {
        start.WriteU8(static_cast<uint8_t>(SubelementId::WIDE_BANDWIDTH_CHANNEL_SWITCH));
        start.WriteU8(3);
        start.WriteU8(m_wideBandwidthChannelSwitch->newChannelWidth);
        start.WriteU8(m_wideBandwidthChannelSwitch->newChannelCenterFreqSeg0);
        start.WriteU8(m_wideBandwidthChannelSwitch->newChannelCenterFreqSeg1);
    }
    if (m_vendorSpecific)
    {
        start.WriteU8(static_cast<uint8_t>(SubelementId::VENDOR_SPECIFIC));
        start.WriteU8(static_cast<uint8_t>(m_vendorSpecific->size()));
        for (uint8_t b : *m_vendorSpecific)
        {
            start.WriteU8(b);
        }
    }
}

uint16_t
NoiseHistogramReport::Deserialize(Buffer::Iterator& start, uint16_t length)
{
    m_operatingClass = start.ReadU8();
    m_channelNumber = start.ReadU8();
    m_actualMeasurementStartTime = start.ReadU64();
    m_measurementDuration = start.ReadU16();
    m_antennaId = start.ReadU8();
    m_anpi = start.ReadU8();
    for (uint8_t i = 0; i < 11; i++)
    {
        m_ipiDensities[i] = start.ReadU8();
    }
    uint16_t bytesRead = FIXED_FIELDS_SIZE;

    while (bytesRead + 2 <= length)
    {
        uint8_t subId = start.ReadU8();
        uint8_t subLen = start.ReadU8();
        bytesRead += 2;
        if (subId == static_cast<uint8_t>(SubelementId::WIDE_BANDWIDTH_CHANNEL_SWITCH))
        {
            BeaconReport::WideBandwidthChannelSwitch wbc;
            wbc.newChannelWidth = start.ReadU8();
            wbc.newChannelCenterFreqSeg0 = start.ReadU8();
            wbc.newChannelCenterFreqSeg1 = start.ReadU8();
            m_wideBandwidthChannelSwitch = wbc;
            bytesRead += subLen;
        }
        else if (subId == static_cast<uint8_t>(SubelementId::VENDOR_SPECIFIC))
        {
            std::vector<uint8_t> data(subLen);
            for (uint8_t j = 0; j < subLen; j++)
            {
                data[j] = start.ReadU8();
            }
            m_vendorSpecific = std::move(data);
            bytesRead += subLen;
        }
        else
        {
            for (uint8_t j = 0; j < subLen; j++)
            {
                start.ReadU8();
            }
            bytesRead += subLen;
        }
    }

    return bytesRead;
}

void
NoiseHistogramReport::SetOperatingClass(uint8_t operatingClass)
{
    m_operatingClass = operatingClass;
}

uint8_t
NoiseHistogramReport::GetOperatingClass() const
{
    return m_operatingClass;
}

void
NoiseHistogramReport::SetChannelNumber(uint8_t channel)
{
    m_channelNumber = channel;
}

uint8_t
NoiseHistogramReport::GetChannelNumber() const
{
    return m_channelNumber;
}

void
NoiseHistogramReport::SetActualMeasurementStartTime(uint64_t startTime)
{
    m_actualMeasurementStartTime = startTime;
}

uint64_t
NoiseHistogramReport::GetActualMeasurementStartTime() const
{
    return m_actualMeasurementStartTime;
}

void
NoiseHistogramReport::SetMeasurementDuration(uint16_t duration)
{
    m_measurementDuration = duration;
}

uint16_t
NoiseHistogramReport::GetMeasurementDuration() const
{
    return m_measurementDuration;
}

void
NoiseHistogramReport::SetAntennaId(uint8_t antennaId)
{
    m_antennaId = antennaId;
}

uint8_t
NoiseHistogramReport::GetAntennaId() const
{
    return m_antennaId;
}

void
NoiseHistogramReport::SetAnpi(uint8_t anpi)
{
    m_anpi = anpi;
}

uint8_t
NoiseHistogramReport::GetAnpi() const
{
    return m_anpi;
}

void
NoiseHistogramReport::SetIpiDensity(uint8_t level, uint8_t density)
{
    NS_ASSERT_MSG(level <= 10, "IPI level must be 0-10");
    m_ipiDensities[level] = density;
}

uint8_t
NoiseHistogramReport::GetIpiDensity(uint8_t level) const
{
    NS_ASSERT_MSG(level <= 10, "IPI level must be 0-10");
    return m_ipiDensities[level];
}

void
NoiseHistogramReport::SetWideBandwidthChannelSwitch(
    const BeaconReport::WideBandwidthChannelSwitch& wbc)
{
    m_wideBandwidthChannelSwitch = wbc;
}

std::optional<BeaconReport::WideBandwidthChannelSwitch>
NoiseHistogramReport::GetWideBandwidthChannelSwitch() const
{
    return m_wideBandwidthChannelSwitch;
}

void
NoiseHistogramReport::SetVendorSpecific(const std::vector<uint8_t>& data)
{
    m_vendorSpecific = data;
}

std::optional<std::vector<uint8_t>>
NoiseHistogramReport::GetVendorSpecific() const
{
    return m_vendorSpecific;
}

// --- FrameReportEntry ---

uint16_t
FrameReportEntry::GetSerializedSize() const
{
    return SERIALIZED_SIZE;
}

void
FrameReportEntry::Serialize(Buffer::Iterator& start) const
{
    WriteTo(start, m_transmitterAddress);
    WriteTo(start, m_bssid);
    start.WriteU8(m_phyType);
    start.WriteU8(m_averageRcpi);
    start.WriteU8(m_lastRsni);
    start.WriteU8(m_lastRcpi);
    start.WriteU8(m_antennaId);
    start.WriteU16(m_frameCount);
}

uint16_t
FrameReportEntry::Deserialize(Buffer::Iterator& start)
{
    ReadFrom(start, m_transmitterAddress);
    ReadFrom(start, m_bssid);
    m_phyType = start.ReadU8();
    m_averageRcpi = start.ReadU8();
    m_lastRsni = start.ReadU8();
    m_lastRcpi = start.ReadU8();
    m_antennaId = start.ReadU8();
    m_frameCount = start.ReadU16();
    return SERIALIZED_SIZE;
}

void
FrameReportEntry::SetTransmitterAddress(Mac48Address addr)
{
    m_transmitterAddress = addr;
}

Mac48Address
FrameReportEntry::GetTransmitterAddress() const
{
    return m_transmitterAddress;
}

void
FrameReportEntry::SetBssid(Mac48Address bssid)
{
    m_bssid = bssid;
}

Mac48Address
FrameReportEntry::GetBssid() const
{
    return m_bssid;
}

void
FrameReportEntry::SetPhyType(uint8_t phyType)
{
    m_phyType = phyType;
}

uint8_t
FrameReportEntry::GetPhyType() const
{
    return m_phyType;
}

void
FrameReportEntry::SetAverageRcpi(uint8_t rcpi)
{
    m_averageRcpi = rcpi;
}

uint8_t
FrameReportEntry::GetAverageRcpi() const
{
    return m_averageRcpi;
}

void
FrameReportEntry::SetLastRsni(uint8_t rsni)
{
    m_lastRsni = rsni;
}

uint8_t
FrameReportEntry::GetLastRsni() const
{
    return m_lastRsni;
}

void
FrameReportEntry::SetLastRcpi(uint8_t rcpi)
{
    m_lastRcpi = rcpi;
}

uint8_t
FrameReportEntry::GetLastRcpi() const
{
    return m_lastRcpi;
}

void
FrameReportEntry::SetAntennaId(uint8_t antennaId)
{
    m_antennaId = antennaId;
}

uint8_t
FrameReportEntry::GetAntennaId() const
{
    return m_antennaId;
}

void
FrameReportEntry::SetFrameCount(uint16_t count)
{
    m_frameCount = count;
}

uint16_t
FrameReportEntry::GetFrameCount() const
{
    return m_frameCount;
}

// --- FrameReport ---

uint16_t
FrameReport::GetSerializedSize() const
{
    uint16_t size = FIXED_FIELDS_SIZE;
    if (!m_frameReportEntries.empty())
    {
        size += 2 + static_cast<uint16_t>(m_frameReportEntries.size()) *
                        FrameReportEntry::SERIALIZED_SIZE;
    }
    return size;
}

void
FrameReport::Serialize(Buffer::Iterator& start) const
{
    start.WriteU8(m_operatingClass);
    start.WriteU8(m_channelNumber);
    start.WriteU64(m_actualMeasurementStartTime);
    start.WriteU16(m_measurementDuration);
    if (!m_frameReportEntries.empty())
    {
        NS_ASSERT_MSG(
            m_frameReportEntries.size() <= 13,
            "Frame Count Report subelement Length is 1 octet, max 13 entries (13*19=247)");
        start.WriteU8(FRAME_COUNT_REPORT);
        start.WriteU8(
            static_cast<uint8_t>(m_frameReportEntries.size() * FrameReportEntry::SERIALIZED_SIZE));
        for (const auto& entry : m_frameReportEntries)
        {
            entry.Serialize(start);
        }
    }
}

uint16_t
FrameReport::Deserialize(Buffer::Iterator& start, uint16_t length)
{
    m_operatingClass = start.ReadU8();
    m_channelNumber = start.ReadU8();
    m_actualMeasurementStartTime = start.ReadU64();
    m_measurementDuration = start.ReadU16();
    uint16_t bytesRead = FIXED_FIELDS_SIZE;

    while (bytesRead + 2 <= length)
    {
        uint8_t subId = start.ReadU8();
        uint8_t subLen = start.ReadU8();
        bytesRead += 2;
        if (subId == FRAME_COUNT_REPORT)
        {
            uint16_t remaining = subLen;
            while (remaining >= FrameReportEntry::SERIALIZED_SIZE)
            {
                FrameReportEntry entry;
                entry.Deserialize(start);
                m_frameReportEntries.push_back(entry);
                remaining -= FrameReportEntry::SERIALIZED_SIZE;
                bytesRead += FrameReportEntry::SERIALIZED_SIZE;
            }
            // Skip any leftover bytes in this subelement
            for (uint16_t j = 0; j < remaining; j++)
            {
                start.ReadU8();
                bytesRead++;
            }
        }
        else
        {
            for (uint8_t j = 0; j < subLen; j++)
            {
                start.ReadU8();
                bytesRead++;
            }
        }
    }

    return bytesRead;
}

void
FrameReport::SetOperatingClass(uint8_t operatingClass)
{
    m_operatingClass = operatingClass;
}

uint8_t
FrameReport::GetOperatingClass() const
{
    return m_operatingClass;
}

void
FrameReport::SetChannelNumber(uint8_t channel)
{
    m_channelNumber = channel;
}

uint8_t
FrameReport::GetChannelNumber() const
{
    return m_channelNumber;
}

void
FrameReport::SetActualMeasurementStartTime(uint64_t startTime)
{
    m_actualMeasurementStartTime = startTime;
}

uint64_t
FrameReport::GetActualMeasurementStartTime() const
{
    return m_actualMeasurementStartTime;
}

void
FrameReport::SetMeasurementDuration(uint16_t duration)
{
    m_measurementDuration = duration;
}

uint16_t
FrameReport::GetMeasurementDuration() const
{
    return m_measurementDuration;
}

void
FrameReport::AddFrameReportEntry(const FrameReportEntry& entry)
{
    m_frameReportEntries.push_back(entry);
}

const std::vector<FrameReportEntry>&
FrameReport::GetFrameReportEntries() const
{
    return m_frameReportEntries;
}

// --- StaStatisticsReport ---

uint16_t
StaStatisticsReport::GetExpectedGroupDataSize(uint8_t groupIdentity)
{
    // Sizes from IEEE 802.11-2024 Table 9-170
    switch (groupIdentity)
    {
    case 0:
        return 28; // dot11CountersTable (7 x Counter32)
    case 1:
        return 24; // dot11MACStatistics (6 x Counter32)
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        return 52; // dot11QosCountersTable (13 x Counter32)
    case 10:
        return 8; // BSSAverageAccessDelay
    case 11:
        return 40; // dot11CountersGroup11
    case 12:
    case 13:
    case 14:
        return 36; // dot11CountersGroup12-14
    case 15:
        return 20; // dot11CountersGroup15
    case 16:
        return 28; // dot11RSNAStatsTable
    default:
        return 0; // reserved/unknown
    }
}

uint16_t
StaStatisticsReport::GetSerializedSize() const
{
    uint16_t size = 3 + static_cast<uint16_t>(m_statisticsGroupData.size());
    if (m_reportingReason)
    {
        size += 2 + 1;
    }
    if (m_vendorSpecific)
    {
        size += 2 + static_cast<uint16_t>(m_vendorSpecific->size());
    }
    return size;
}

void
StaStatisticsReport::Serialize(Buffer::Iterator& start) const
{
    start.WriteU16(m_measurementDuration);
    start.WriteU8(m_groupIdentity);
    for (const auto& byte : m_statisticsGroupData)
    {
        start.WriteU8(byte);
    }
    if (m_reportingReason)
    {
        start.WriteU8(static_cast<uint8_t>(SubelementId::REPORTING_REASON));
        start.WriteU8(1);
        start.WriteU8(*m_reportingReason);
    }
    if (m_vendorSpecific)
    {
        start.WriteU8(static_cast<uint8_t>(SubelementId::VENDOR_SPECIFIC));
        start.WriteU8(static_cast<uint8_t>(m_vendorSpecific->size()));
        for (uint8_t b : *m_vendorSpecific)
        {
            start.WriteU8(b);
        }
    }
}

uint16_t
StaStatisticsReport::Deserialize(Buffer::Iterator& start, uint16_t length)
{
    m_measurementDuration = start.ReadU16();
    m_groupIdentity = start.ReadU8();
    uint16_t dataSize = GetExpectedGroupDataSize(m_groupIdentity);
    m_statisticsGroupData.resize(dataSize);
    for (uint16_t i = 0; i < dataSize; i++)
    {
        m_statisticsGroupData[i] = start.ReadU8();
    }
    uint16_t bytesRead = 3 + dataSize;

    while (bytesRead + 2 <= length)
    {
        uint8_t subId = start.ReadU8();
        uint8_t subLen = start.ReadU8();
        bytesRead += 2;
        if (subId == static_cast<uint8_t>(SubelementId::REPORTING_REASON))
        {
            m_reportingReason = start.ReadU8();
            bytesRead += subLen;
        }
        else if (subId == static_cast<uint8_t>(SubelementId::VENDOR_SPECIFIC))
        {
            std::vector<uint8_t> data(subLen);
            for (uint8_t j = 0; j < subLen; j++)
            {
                data[j] = start.ReadU8();
            }
            m_vendorSpecific = std::move(data);
            bytesRead += subLen;
        }
        else
        {
            for (uint8_t j = 0; j < subLen; j++)
            {
                start.ReadU8();
            }
            bytesRead += subLen;
        }
    }

    return bytesRead;
}

void
StaStatisticsReport::SetMeasurementDuration(uint16_t duration)
{
    m_measurementDuration = duration;
}

uint16_t
StaStatisticsReport::GetMeasurementDuration() const
{
    return m_measurementDuration;
}

uint8_t
StaStatisticsReport::GetGroupIdentity() const
{
    return m_groupIdentity;
}

void
StaStatisticsReport::SetGroup0Data(const Group0Data& data)
{
    m_groupIdentity = 0;
    Buffer buf;
    buf.AddAtStart(28);
    auto it = buf.Begin();
    it.WriteU32(data.transmittedFragmentCount);
    it.WriteU32(data.groupTransmittedFrameCount);
    it.WriteU32(data.failedCount);
    it.WriteU32(data.receivedFragmentCount);
    it.WriteU32(data.groupReceivedFrameCount);
    it.WriteU32(data.fcsErrorCount);
    it.WriteU32(data.transmittedFrameCount);
    m_statisticsGroupData.resize(28);
    it = buf.Begin();
    for (auto& byte : m_statisticsGroupData)
    {
        byte = it.ReadU8();
    }
}

std::optional<StaStatisticsReport::Group0Data>
StaStatisticsReport::GetGroup0Data() const
{
    if (m_groupIdentity != 0)
    {
        return std::nullopt;
    }
    Buffer buf;
    buf.AddAtStart(28);
    auto it = buf.Begin();
    for (const auto& byte : m_statisticsGroupData)
    {
        it.WriteU8(byte);
    }
    it = buf.Begin();
    Group0Data data;
    data.transmittedFragmentCount = it.ReadU32();
    data.groupTransmittedFrameCount = it.ReadU32();
    data.failedCount = it.ReadU32();
    data.receivedFragmentCount = it.ReadU32();
    data.groupReceivedFrameCount = it.ReadU32();
    data.fcsErrorCount = it.ReadU32();
    data.transmittedFrameCount = it.ReadU32();
    return data;
}

void
StaStatisticsReport::SetGroup1Data(const Group1Data& data)
{
    m_groupIdentity = 1;
    Buffer buf;
    buf.AddAtStart(24);
    auto it = buf.Begin();
    it.WriteU32(data.retryCount);
    it.WriteU32(data.multipleRetryCount);
    it.WriteU32(data.frameDuplicateCount);
    it.WriteU32(data.rtsSuccessCount);
    it.WriteU32(data.rtsFailureCount);
    it.WriteU32(data.ackFailureCount);
    m_statisticsGroupData.resize(24);
    it = buf.Begin();
    for (auto& byte : m_statisticsGroupData)
    {
        byte = it.ReadU8();
    }
}

std::optional<StaStatisticsReport::Group1Data>
StaStatisticsReport::GetGroup1Data() const
{
    if (m_groupIdentity != 1)
    {
        return std::nullopt;
    }
    Buffer buf;
    buf.AddAtStart(24);
    auto it = buf.Begin();
    for (const auto& byte : m_statisticsGroupData)
    {
        it.WriteU8(byte);
    }
    it = buf.Begin();
    Group1Data data;
    data.retryCount = it.ReadU32();
    data.multipleRetryCount = it.ReadU32();
    data.frameDuplicateCount = it.ReadU32();
    data.rtsSuccessCount = it.ReadU32();
    data.rtsFailureCount = it.ReadU32();
    data.ackFailureCount = it.ReadU32();
    return data;
}

void
StaStatisticsReport::SetGroup10Data(const Group10Data& data)
{
    m_groupIdentity = 10;
    Buffer buf;
    buf.AddAtStart(8);
    auto it = buf.Begin();
    it.WriteU8(data.apAverageAccessDelay);
    it.WriteU8(data.averageAccessDelayBestEffort);
    it.WriteU8(data.averageAccessDelayBackGround);
    it.WriteU8(data.averageAccessDelayVideo);
    it.WriteU8(data.averageAccessDelayVoice);
    it.WriteU16(data.stationCount);
    it.WriteU8(data.channelUtilization);
    m_statisticsGroupData.resize(8);
    it = buf.Begin();
    for (auto& byte : m_statisticsGroupData)
    {
        byte = it.ReadU8();
    }
}

std::optional<StaStatisticsReport::Group10Data>
StaStatisticsReport::GetGroup10Data() const
{
    if (m_groupIdentity != 10)
    {
        return std::nullopt;
    }
    Buffer buf;
    buf.AddAtStart(8);
    auto it = buf.Begin();
    for (const auto& byte : m_statisticsGroupData)
    {
        it.WriteU8(byte);
    }
    it = buf.Begin();
    Group10Data data;
    data.apAverageAccessDelay = it.ReadU8();
    data.averageAccessDelayBestEffort = it.ReadU8();
    data.averageAccessDelayBackGround = it.ReadU8();
    data.averageAccessDelayVideo = it.ReadU8();
    data.averageAccessDelayVoice = it.ReadU8();
    data.stationCount = it.ReadU16();
    data.channelUtilization = it.ReadU8();
    return data;
}

void
StaStatisticsReport::SetReportingReason(uint8_t reason)
{
    m_reportingReason = reason;
}

std::optional<uint8_t>
StaStatisticsReport::GetReportingReason() const
{
    return m_reportingReason;
}

void
StaStatisticsReport::SetVendorSpecific(const std::vector<uint8_t>& data)
{
    m_vendorSpecific = data;
}

std::optional<std::vector<uint8_t>>
StaStatisticsReport::GetVendorSpecific() const
{
    return m_vendorSpecific;
}

// --- MeasurementReportElement ---

WifiInformationElementId
MeasurementReportElement::ElementId() const
{
    return IE_MEASUREMENT_REPORT;
}

void
MeasurementReportElement::SetMeasurementToken(uint8_t token)
{
    m_measurementToken = token;
}

uint8_t
MeasurementReportElement::GetMeasurementToken() const
{
    return m_measurementToken;
}

void
MeasurementReportElement::SetLate(bool late)
{
    if (late)
    {
        // 0x06 masks B1|B2 (Incapable|Refused) -- spec requires at most one mode bit set
        NS_ABORT_MSG_IF(m_measurementReportMode & 0x06,
                        "No more than one bit is set to 1 within a Measurement Report Mode field");
        m_measurementReportMode |= (1 << 0);
    }
    else
    {
        m_measurementReportMode &= ~(1 << 0);
    }
}

bool
MeasurementReportElement::GetLate() const
{
    return (m_measurementReportMode & (1 << 0)) != 0;
}

void
MeasurementReportElement::SetIncapable(bool incapable)
{
    if (incapable)
    {
        // 0x05 masks B0|B2 (Late|Refused) -- spec requires at most one mode bit set
        NS_ABORT_MSG_IF(m_measurementReportMode & 0x05,
                        "No more than one bit is set to 1 within a Measurement Report Mode field");
        m_measurementReportMode |= (1 << 1);
    }
    else
    {
        m_measurementReportMode &= ~(1 << 1);
    }
}

bool
MeasurementReportElement::GetIncapable() const
{
    return (m_measurementReportMode & (1 << 1)) != 0;
}

void
MeasurementReportElement::SetRefused(bool refused)
{
    if (refused)
    {
        // 0x03 masks B0|B1 (Late|Incapable) -- spec requires at most one mode bit set
        NS_ABORT_MSG_IF(m_measurementReportMode & 0x03,
                        "No more than one bit is set to 1 within a Measurement Report Mode field");
        m_measurementReportMode |= (1 << 2);
    }
    else
    {
        m_measurementReportMode &= ~(1 << 2);
    }
}

bool
MeasurementReportElement::GetRefused() const
{
    return (m_measurementReportMode & (1 << 2)) != 0;
}

void
MeasurementReportElement::SetMeasurementType(MeasurementReportType type)
{
    m_measurementType = type;
}

MeasurementReportType
MeasurementReportElement::GetMeasurementType() const
{
    return m_measurementType;
}

void
MeasurementReportElement::SetBeaconReport(const BeaconReport& report)
{
    m_measurementType = MeasurementReportType::BEACON;
    m_report = report;
}

std::optional<BeaconReport>
MeasurementReportElement::GetBeaconReport() const
{
    if (std::holds_alternative<BeaconReport>(m_report))
    {
        return std::get<BeaconReport>(m_report);
    }
    return std::nullopt;
}

void
MeasurementReportElement::SetChannelLoadReport(const ChannelLoadReport& report)
{
    m_measurementType = MeasurementReportType::CHANNEL_LOAD;
    m_report = report;
}

std::optional<ChannelLoadReport>
MeasurementReportElement::GetChannelLoadReport() const
{
    if (std::holds_alternative<ChannelLoadReport>(m_report))
    {
        return std::get<ChannelLoadReport>(m_report);
    }
    return std::nullopt;
}

void
MeasurementReportElement::SetNoiseHistogramReport(const NoiseHistogramReport& report)
{
    m_measurementType = MeasurementReportType::NOISE_HISTOGRAM;
    m_report = report;
}

std::optional<NoiseHistogramReport>
MeasurementReportElement::GetNoiseHistogramReport() const
{
    if (std::holds_alternative<NoiseHistogramReport>(m_report))
    {
        return std::get<NoiseHistogramReport>(m_report);
    }
    return std::nullopt;
}

void
MeasurementReportElement::SetStaStatisticsReport(const StaStatisticsReport& report)
{
    m_measurementType = MeasurementReportType::STA_STATISTICS;
    m_report = report;
}

std::optional<StaStatisticsReport>
MeasurementReportElement::GetStaStatisticsReport() const
{
    if (std::holds_alternative<StaStatisticsReport>(m_report))
    {
        return std::get<StaStatisticsReport>(m_report);
    }
    return std::nullopt;
}

void
MeasurementReportElement::SetFrameReport(const FrameReport& report)
{
    m_measurementType = MeasurementReportType::FRAME;
    m_report = report;
}

std::optional<FrameReport>
MeasurementReportElement::GetFrameReport() const
{
    if (std::holds_alternative<FrameReport>(m_report))
    {
        return std::get<FrameReport>(m_report);
    }
    return std::nullopt;
}

bool
MeasurementReportElement::HasModeSet() const
{
    return (m_measurementReportMode & 0x07) != 0;
}

uint16_t
MeasurementReportElement::GetInformationFieldSize() const
{
    uint16_t size = 3; // token(1) + mode(1) + type(1)
    if (!HasModeSet())
    {
        if (std::holds_alternative<BeaconReport>(m_report))
        {
            size += std::get<BeaconReport>(m_report).GetSerializedSize();
        }
        else if (std::holds_alternative<ChannelLoadReport>(m_report))
        {
            size += std::get<ChannelLoadReport>(m_report).GetSerializedSize();
        }
        else if (std::holds_alternative<NoiseHistogramReport>(m_report))
        {
            size += std::get<NoiseHistogramReport>(m_report).GetSerializedSize();
        }
        else if (std::holds_alternative<FrameReport>(m_report))
        {
            size += std::get<FrameReport>(m_report).GetSerializedSize();
        }
        else if (std::holds_alternative<StaStatisticsReport>(m_report))
        {
            size += std::get<StaStatisticsReport>(m_report).GetSerializedSize();
        }
    }
    return size;
}

void
MeasurementReportElement::SerializeInformationField(Buffer::Iterator start) const
{
    start.WriteU8(m_measurementToken);
    start.WriteU8(m_measurementReportMode);
    start.WriteU8(static_cast<uint8_t>(m_measurementType));

    if (!HasModeSet())
    {
        if (std::holds_alternative<BeaconReport>(m_report))
        {
            std::get<BeaconReport>(m_report).Serialize(start);
        }
        else if (std::holds_alternative<ChannelLoadReport>(m_report))
        {
            std::get<ChannelLoadReport>(m_report).Serialize(start);
        }
        else if (std::holds_alternative<NoiseHistogramReport>(m_report))
        {
            std::get<NoiseHistogramReport>(m_report).Serialize(start);
        }
        else if (std::holds_alternative<FrameReport>(m_report))
        {
            std::get<FrameReport>(m_report).Serialize(start);
        }
        else if (std::holds_alternative<StaStatisticsReport>(m_report))
        {
            std::get<StaStatisticsReport>(m_report).Serialize(start);
        }
    }
}

uint16_t
MeasurementReportElement::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    Buffer::Iterator i = start;
    m_measurementToken = i.ReadU8();
    m_measurementReportMode = i.ReadU8();
    m_measurementType = static_cast<MeasurementReportType>(i.ReadU8());

    uint16_t bytesRead = 3;

    if (!HasModeSet() && bytesRead < length)
    {
        if (m_measurementType == MeasurementReportType::BEACON)
        {
            BeaconReport br;
            bytesRead += br.Deserialize(i, length - bytesRead);
            m_report = br;
        }
        else if (m_measurementType == MeasurementReportType::CHANNEL_LOAD)
        {
            ChannelLoadReport clr;
            bytesRead += clr.Deserialize(i, length - bytesRead);
            m_report = clr;
        }
        else if (m_measurementType == MeasurementReportType::NOISE_HISTOGRAM)
        {
            NoiseHistogramReport nhr;
            bytesRead += nhr.Deserialize(i, length - bytesRead);
            m_report = nhr;
        }
        else if (m_measurementType == MeasurementReportType::FRAME)
        {
            FrameReport fr;
            bytesRead += fr.Deserialize(i, length - bytesRead);
            m_report = fr;
        }
        else if (m_measurementType == MeasurementReportType::STA_STATISTICS)
        {
            StaStatisticsReport ssr;
            bytesRead += ssr.Deserialize(i, length - bytesRead);
            m_report = ssr;
        }
        else
        {
            // Skip unknown report body
            uint16_t remaining = length - bytesRead;
            for (uint16_t j = 0; j < remaining; j++)
            {
                i.ReadU8();
            }
            bytesRead += remaining;
            m_report = std::monostate{};
        }
    }

    return bytesRead;
}

void
MeasurementReportElement::Print(std::ostream& os) const
{
    os << "MeasurementReport=[Token=" << +m_measurementToken << ", Mode=0x" << std::hex
       << +m_measurementReportMode << std::dec
       << ", Type=" << +static_cast<uint8_t>(m_measurementType);
    if (HasModeSet())
    {
        if (GetLate())
        {
            os << " Late";
        }
        if (GetIncapable())
        {
            os << " Incapable";
        }
        if (GetRefused())
        {
            os << " Refused";
        }
    }
    else if (std::holds_alternative<BeaconReport>(m_report))
    {
        const auto& br = std::get<BeaconReport>(m_report);
        os << ", BeaconReport=[OpClass=" << +br.GetOperatingClass()
           << ", Channel=" << +br.GetChannelNumber()
           << ", StartTime=" << br.GetActualMeasurementStartTime()
           << ", Duration=" << br.GetMeasurementDuration()
           << ", PHYType=" << +br.GetCondensedPhyType()
           << ", FrameType=" << br.GetReportedFrameType() << ", RCPI=" << +br.GetRcpi()
           << ", RSNI=" << +br.GetRsni() << ", BSSID=" << br.GetBssid()
           << ", AntennaId=" << +br.GetAntennaId() << ", ParentTSF=0x" << std::hex
           << br.GetParentTsf() << std::dec << "]";
    }
    else if (std::holds_alternative<ChannelLoadReport>(m_report))
    {
        const auto& clr = std::get<ChannelLoadReport>(m_report);
        os << ", ChannelLoadReport=[OpClass=" << +clr.GetOperatingClass()
           << ", Channel=" << +clr.GetChannelNumber()
           << ", StartTime=" << clr.GetActualMeasurementStartTime()
           << ", Duration=" << clr.GetMeasurementDuration()
           << ", ChannelLoad=" << +clr.GetChannelLoad() << "]";
    }
    else if (std::holds_alternative<NoiseHistogramReport>(m_report))
    {
        const auto& nhr = std::get<NoiseHistogramReport>(m_report);
        os << ", NoiseHistogramReport=[OpClass=" << +nhr.GetOperatingClass()
           << ", Channel=" << +nhr.GetChannelNumber()
           << ", StartTime=" << nhr.GetActualMeasurementStartTime()
           << ", Duration=" << nhr.GetMeasurementDuration() << ", AntennaId=" << +nhr.GetAntennaId()
           << ", ANPI=" << +nhr.GetAnpi() << ", IPI=[";
        for (uint8_t j = 0; j <= 10; j++)
        {
            if (j > 0)
            {
                os << ",";
            }
            os << +nhr.GetIpiDensity(j);
        }
        os << "]]";
    }
    else if (std::holds_alternative<FrameReport>(m_report))
    {
        const auto& fr = std::get<FrameReport>(m_report);
        os << ", FrameReport=[OpClass=" << +fr.GetOperatingClass()
           << ", Channel=" << +fr.GetChannelNumber()
           << ", StartTime=" << fr.GetActualMeasurementStartTime()
           << ", Duration=" << fr.GetMeasurementDuration()
           << ", Entries=" << fr.GetFrameReportEntries().size() << "]";
    }
    else if (std::holds_alternative<StaStatisticsReport>(m_report))
    {
        const auto& ssr = std::get<StaStatisticsReport>(m_report);
        os << ", StaStatisticsReport=[Duration=" << ssr.GetMeasurementDuration()
           << ", GroupId=" << +ssr.GetGroupIdentity()
           << ", DataSize=" << StaStatisticsReport::GetExpectedGroupDataSize(ssr.GetGroupIdentity())
           << "]";
    }
    os << "]";
}

} // namespace ns3
