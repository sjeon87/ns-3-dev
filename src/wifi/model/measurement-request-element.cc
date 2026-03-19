/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Devansh Gupta <devanshg308@gmail.com>
 */

#include "measurement-request-element.h"

#include "ns3/address-utils.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MeasurementRequestElement");

MeasurementRequestElement::MeasurementRequestElement()
{
}

WifiInformationElementId
MeasurementRequestElement::ElementId() const
{
    return IE_MEASUREMENT_REQUEST;
}

// --- Fixed header accessors ---

void
MeasurementRequestElement::SetMeasurementToken(uint8_t token)
{
    m_measurementToken = token;
}

uint8_t
MeasurementRequestElement::GetMeasurementToken() const
{
    return m_measurementToken;
}

void
MeasurementRequestElement::SetMeasurementRequestMode(uint8_t mode)
{
    m_measurementRequestMode = mode & 0x1F;
}

uint8_t
MeasurementRequestElement::GetMeasurementRequestMode() const
{
    return m_measurementRequestMode;
}

void
MeasurementRequestElement::SetMeasurementType(uint8_t type)
{
    m_measurementType = type;
}

uint8_t
MeasurementRequestElement::GetMeasurementType() const
{
    return m_measurementType;
}

// --- Mode bitmap bit accessors ---

void
MeasurementRequestElement::SetParallel(bool parallel)
{
    if (parallel)
    {
        m_measurementRequestMode |= 0x01;
    }
    else
    {
        m_measurementRequestMode &= ~0x01;
    }
}

bool
MeasurementRequestElement::GetParallel() const
{
    return (m_measurementRequestMode & 0x01) != 0;
}

void
MeasurementRequestElement::SetEnable(bool enable)
{
    if (enable)
    {
        m_measurementRequestMode |= 0x02;
    }
    else
    {
        m_measurementRequestMode &= ~0x02;
    }
}

bool
MeasurementRequestElement::GetEnable() const
{
    return (m_measurementRequestMode & 0x02) != 0;
}

void
MeasurementRequestElement::SetRequest(bool request)
{
    if (request)
    {
        m_measurementRequestMode |= 0x04;
    }
    else
    {
        m_measurementRequestMode &= ~0x04;
    }
}

bool
MeasurementRequestElement::GetRequest() const
{
    return (m_measurementRequestMode & 0x04) != 0;
}

void
MeasurementRequestElement::SetReport(bool report)
{
    if (report)
    {
        m_measurementRequestMode |= 0x08;
    }
    else
    {
        m_measurementRequestMode &= ~0x08;
    }
}

bool
MeasurementRequestElement::GetReport() const
{
    return (m_measurementRequestMode & 0x08) != 0;
}

void
MeasurementRequestElement::SetDurationMandatory(bool mandatory)
{
    if (mandatory)
    {
        m_measurementRequestMode |= 0x10;
    }
    else
    {
        m_measurementRequestMode &= ~0x10;
    }
}

bool
MeasurementRequestElement::GetDurationMandatory() const
{
    return (m_measurementRequestMode & 0x10) != 0;
}

// --- Type-specific body field accessors ---

void
MeasurementRequestElement::SetOperatingClass(uint8_t operatingClass)
{
    m_operatingClass = operatingClass;
}

uint8_t
MeasurementRequestElement::GetOperatingClass() const
{
    return m_operatingClass;
}

void
MeasurementRequestElement::SetChannelNumber(uint8_t channel)
{
    m_channelNumber = channel;
}

uint8_t
MeasurementRequestElement::GetChannelNumber() const
{
    return m_channelNumber;
}

void
MeasurementRequestElement::SetRandomizationInterval(uint16_t interval)
{
    m_randomizationInterval = interval;
}

uint16_t
MeasurementRequestElement::GetRandomizationInterval() const
{
    return m_randomizationInterval;
}

void
MeasurementRequestElement::SetMeasurementDuration(uint16_t duration)
{
    m_measurementDuration = duration;
}

uint16_t
MeasurementRequestElement::GetMeasurementDuration() const
{
    return m_measurementDuration;
}

void
MeasurementRequestElement::SetBeaconMeasurementMode(uint8_t mode)
{
    m_beaconMeasurementMode = mode;
}

uint8_t
MeasurementRequestElement::GetBeaconMeasurementMode() const
{
    return m_beaconMeasurementMode;
}

void
MeasurementRequestElement::SetBssid(Mac48Address bssid)
{
    m_bssid = bssid;
}

Mac48Address
MeasurementRequestElement::GetBssid() const
{
    return m_bssid;
}

void
MeasurementRequestElement::SetFrameRequestType(uint8_t frameRequestType)
{
    m_frameRequestType = frameRequestType;
}

uint8_t
MeasurementRequestElement::GetFrameRequestType() const
{
    return m_frameRequestType;
}

void
MeasurementRequestElement::SetMacAddress(Mac48Address macAddress)
{
    m_macAddress = macAddress;
}

Mac48Address
MeasurementRequestElement::GetMacAddress() const
{
    return m_macAddress;
}

void
MeasurementRequestElement::SetPeerMacAddress(Mac48Address peerMacAddress)
{
    m_peerMacAddress = peerMacAddress;
}

Mac48Address
MeasurementRequestElement::GetPeerMacAddress() const
{
    return m_peerMacAddress;
}

void
MeasurementRequestElement::SetGroupIdentity(uint8_t groupIdentity)
{
    m_groupIdentity = groupIdentity;
}

uint8_t
MeasurementRequestElement::GetGroupIdentity() const
{
    return m_groupIdentity;
}

void
MeasurementRequestElement::SetLocationSubject(uint8_t locationSubject)
{
    m_locationSubject = locationSubject;
}

uint8_t
MeasurementRequestElement::GetLocationSubject() const
{
    return m_locationSubject;
}

void
MeasurementRequestElement::SetPeerStaAddress(Mac48Address peerStaAddress)
{
    m_peerStaAddress = peerStaAddress;
}

Mac48Address
MeasurementRequestElement::GetPeerStaAddress() const
{
    return m_peerStaAddress;
}

void
MeasurementRequestElement::SetTrafficIdentifier(uint8_t tid)
{
    m_trafficIdentifier = tid;
}

uint8_t
MeasurementRequestElement::GetTrafficIdentifier() const
{
    return m_trafficIdentifier;
}

void
MeasurementRequestElement::SetBin0Range(uint8_t bin0Range)
{
    m_bin0Range = bin0Range;
}

uint8_t
MeasurementRequestElement::GetBin0Range() const
{
    return m_bin0Range;
}

void
MeasurementRequestElement::SetGroupMacAddress(Mac48Address groupMacAddress)
{
    m_groupMacAddress = groupMacAddress;
}

Mac48Address
MeasurementRequestElement::GetGroupMacAddress() const
{
    return m_groupMacAddress;
}

void
MeasurementRequestElement::SetCivicLocationType(uint8_t civicLocationType)
{
    m_civicLocationType = civicLocationType;
}

uint8_t
MeasurementRequestElement::GetCivicLocationType() const
{
    return m_civicLocationType;
}

void
MeasurementRequestElement::SetLocationServiceIntervalUnits(uint8_t units)
{
    m_locationServiceIntervalUnits = units;
}

uint8_t
MeasurementRequestElement::GetLocationServiceIntervalUnits() const
{
    return m_locationServiceIntervalUnits;
}

void
MeasurementRequestElement::SetLocationServiceInterval(uint16_t interval)
{
    m_locationServiceInterval = interval;
}

uint16_t
MeasurementRequestElement::GetLocationServiceInterval() const
{
    return m_locationServiceInterval;
}

void
MeasurementRequestElement::SetAid(uint8_t aid)
{
    m_aid = aid;
}

uint8_t
MeasurementRequestElement::GetAid() const
{
    return m_aid;
}

void
MeasurementRequestElement::SetMeasurementMethod(uint8_t method)
{
    m_measurementMethod = method;
}

uint8_t
MeasurementRequestElement::GetMeasurementMethod() const
{
    return m_measurementMethod;
}

void
MeasurementRequestElement::SetMeasurementStartTime(uint64_t startTime)
{
    m_measurementStartTime = startTime;
}

uint64_t
MeasurementRequestElement::GetMeasurementStartTime() const
{
    return m_measurementStartTime;
}

void
MeasurementRequestElement::SetNumberOfTimeBlocks(uint8_t numberOfTimeBlocks)
{
    m_numberOfTimeBlocks = numberOfTimeBlocks;
}

uint8_t
MeasurementRequestElement::GetNumberOfTimeBlocks() const
{
    return m_numberOfTimeBlocks;
}

void
MeasurementRequestElement::SetMeasurementMethodAndAntennaConfiguration(uint8_t value)
{
    m_measurementMethodAndAntennaConfiguration = value;
}

uint8_t
MeasurementRequestElement::GetMeasurementMethodAndAntennaConfiguration() const
{
    return m_measurementMethodAndAntennaConfiguration;
}

void
MeasurementRequestElement::SetDirectionalStatisticsBitmap(uint8_t bitmap)
{
    m_directionalStatisticsBitmap = bitmap;
}

uint8_t
MeasurementRequestElement::GetDirectionalStatisticsBitmap() const
{
    return m_directionalStatisticsBitmap;
}

void
MeasurementRequestElement::SetMinimumApCount(uint8_t minApCount)
{
    m_minimumApCount = minApCount;
}

uint8_t
MeasurementRequestElement::GetMinimumApCount() const
{
    return m_minimumApCount;
}

void
MeasurementRequestElement::SetPauseTime(uint16_t pauseTime)
{
    m_pauseTime = pauseTime;
}

uint16_t
MeasurementRequestElement::GetPauseTime() const
{
    return m_pauseTime;
}

// --- Optional subelement accessors ---

void
MeasurementRequestElement::SetChannelLoadReporting(uint8_t condition, uint8_t channelLoadRefValue)
{
    m_channelLoadReporting = ChannelLoadReporting{condition, channelLoadRefValue};
}

std::optional<MeasurementRequestElement::ChannelLoadReporting>
MeasurementRequestElement::GetChannelLoadReporting() const
{
    return m_channelLoadReporting;
}

void
MeasurementRequestElement::SetNoiseHistogramReporting(uint8_t condition, uint8_t anpiRefValue)
{
    m_noiseHistogramReporting = NoiseHistogramReporting{condition, anpiRefValue};
}

std::optional<MeasurementRequestElement::NoiseHistogramReporting>
MeasurementRequestElement::GetNoiseHistogramReporting() const
{
    return m_noiseHistogramReporting;
}

void
MeasurementRequestElement::SetBeaconReporting(uint8_t condition, uint8_t thresholdOffsetRef)
{
    m_beaconReporting = BeaconReporting{condition, thresholdOffsetRef};
}

std::optional<MeasurementRequestElement::BeaconReporting>
MeasurementRequestElement::GetBeaconReporting() const
{
    return m_beaconReporting;
}

void
MeasurementRequestElement::SetBeaconSsid(Ssid ssid)
{
    m_beaconSsid = ssid;
}

std::optional<Ssid>
MeasurementRequestElement::GetBeaconSsid() const
{
    return m_beaconSsid;
}

void
MeasurementRequestElement::SetBeaconReportingDetail(uint8_t detail)
{
    m_beaconReportingDetail = detail;
}

std::optional<uint8_t>
MeasurementRequestElement::GetBeaconReportingDetail() const
{
    return m_beaconReportingDetail;
}

void
MeasurementRequestElement::AddApChannelReport(uint8_t opClass, std::vector<uint8_t> channels)
{
    m_apChannelReports.push_back(ApChannelReport{opClass, std::move(channels)});
}

std::vector<MeasurementRequestElement::ApChannelReport>
MeasurementRequestElement::GetApChannelReports() const
{
    return m_apChannelReports;
}

void
MeasurementRequestElement::SetAzimuthRequest(uint8_t resolution, uint8_t type)
{
    m_azimuthRequest = AzimuthRequest{resolution, type};
}

std::optional<MeasurementRequestElement::AzimuthRequest>
MeasurementRequestElement::GetAzimuthRequest() const
{
    return m_azimuthRequest;
}

void
MeasurementRequestElement::AddFtmRangeNeighborReport(const NeighborReportElement& nre)
{
    m_ftmRangeNeighborReports.push_back(nre);
}

std::vector<NeighborReportElement>
MeasurementRequestElement::GetFtmRangeNeighborReports() const
{
    return m_ftmRangeNeighborReports;
}

void
MeasurementRequestElement::SetVendorSpecificSubelement(std::vector<uint8_t> data)
{
    m_vendorSpecificSubelement = std::move(data);
}

std::optional<std::vector<uint8_t>>
MeasurementRequestElement::GetVendorSpecificSubelement() const
{
    return m_vendorSpecificSubelement;
}

// --- WifiInformationElement overrides ---

uint16_t
MeasurementRequestElement::GetInformationFieldSize() const
{
    uint16_t size = 3; // token(1) + mode(1) + type(1)

    switch (m_measurementType)
    {
    case 0:         // Basic
    case 1:         // CCA
    case 2:         // RPI Histogram
        size += 11; // Channel(1) + StartTime(8) + Duration(2)
        break;
    case 3:        // Channel Load
    case 4:        // Noise Histogram
        size += 6; // OpClass(1) + Channel(1) + RandInterval(2) + Duration(2)
        break;
    case 5:         // Beacon
        size += 13; // OpClass(1) + Channel(1) + RandInterval(2) + Duration(2) + Mode(1) + BSSID(6)
        break;
    case 6:         // Frame
        size += 13; // OpClass(1) + Channel(1) + RandInterval(2) + Duration(2) + FrameReqType(1) +
                    // MAC(6)
        break;
    case 7:         // STA Statistics
        size += 11; // PeerMAC(6) + RandInterval(2) + Duration(2) + GroupID(1)
        break;
    case 8:        // LCI
        size += 1; // LocationSubject(1)
        break;
    case 9:         // Transmit Stream
        size += 12; // RandInterval(2) + Duration(2) + PeerSTA(6) + TID(1) + Bin0Range(1)
        break;
    case 10:        // Multicast Diagnostics
        size += 10; // RandInterval(2) + Duration(2) + GroupMAC(6)
        break;
    case 11:       // Location Civic
        size += 5; // LocSubject(1) + CivicType(1) + ServiceUnits(1) + ServiceInterval(2)
        break;
    case 12:       // Location Identifier
        size += 4; // LocSubject(1) + ServiceUnits(1) + ServiceInterval(2)
        break;
    case 13:        // Directional Channel Quality
        size += 16; // OpClass(1) + Channel(1) + AID(1) + Reserved(1) + Method(1) + StartTime(8) +
                    // Duration(2) + TimeBlocks(1)
        break;
    case 14: // Directional Measurement
        size +=
            13; // OpClass(1) + Channel(1) + StartTime(8) + Duration(2) + MethodAndAntennaConfig(1)
        break;
    case 15: // Directional Statistics
        size +=
            14; // OpClass(1) + Channel(1) + StartTime(8) + Duration(2) + Method(1) + StatsBitmap(1)
        break;
    case 16:       // FTM Range
        size += 3; // RandInterval(2) + MinAPCount(1)
        break;
    case 255:      // Measurement Pause
        size += 2; // PauseTime(2)
        break;
    default:
        break;
    }

    // Subelement sizes (type-scoped)
    if (m_measurementType == 3 && m_channelLoadReporting)
    {
        size += 4; // ID(1) + Len(1) + Condition(1) + ChannelLoadRefValue(1)
    }
    if (m_measurementType == 4 && m_noiseHistogramReporting)
    {
        size += 4; // ID(1) + Len(1) + Condition(1) + ANPIRefValue(1)
    }
    if (m_measurementType == 5 && m_beaconSsid)
    {
        size += 2 + (m_beaconSsid->GetSerializedSize() - 2); // ID(1) + Len(1) + SSID bytes
    }
    if (m_measurementType == 5 && m_beaconReporting)
    {
        size += 4; // ID(1) + Len(1) + Condition(1) + ThresholdOffsetRef(1)
    }
    if (m_measurementType == 5 && m_beaconReportingDetail)
    {
        size += 3; // ID(1) + Len(1) + Detail(1)
    }
    if (m_measurementType == 5)
    {
        for (const auto& report : m_apChannelReports)
        {
            size += 2 + 1 + report.channelList.size(); // ID(1) + Len(1) + OpClass(1) + channels
        }
    }
    if (m_measurementType == 8 && m_azimuthRequest)
    {
        size += 3; // ID(1) + Len(1) + AzimuthRequestField(1)
    }
    if (m_measurementType == 16)
    {
        for (const auto& nre : m_ftmRangeNeighborReports)
        {
            size += 2 + (nre.GetSerializedSize() - 2); // ID(1) + Len(1) + NRE info field
        }
    }
    if (m_vendorSpecificSubelement)
    {
        size += 2 + m_vendorSpecificSubelement->size();
    }

    return size;
}

void
MeasurementRequestElement::SerializeInformationField(Buffer::Iterator start) const
{
    start.WriteU8(m_measurementToken);
    start.WriteU8(m_measurementRequestMode);
    start.WriteU8(m_measurementType);

    switch (m_measurementType)
    {
    case 0: // Basic
    case 1: // CCA
    case 2: // RPI Histogram
        start.WriteU8(m_channelNumber);
        start.WriteU64(m_measurementStartTime);
        start.WriteU16(m_measurementDuration);
        break;
    case 3: // Channel Load
    case 4: // Noise Histogram
        start.WriteU8(m_operatingClass);
        start.WriteU8(m_channelNumber);
        start.WriteU16(m_randomizationInterval);
        start.WriteU16(m_measurementDuration);
        break;
    case 5: // Beacon
        start.WriteU8(m_operatingClass);
        start.WriteU8(m_channelNumber);
        start.WriteU16(m_randomizationInterval);
        start.WriteU16(m_measurementDuration);
        start.WriteU8(m_beaconMeasurementMode);
        WriteTo(start, m_bssid);
        break;
    case 6: // Frame
        start.WriteU8(m_operatingClass);
        start.WriteU8(m_channelNumber);
        start.WriteU16(m_randomizationInterval);
        start.WriteU16(m_measurementDuration);
        start.WriteU8(m_frameRequestType);
        WriteTo(start, m_macAddress);
        break;
    case 7: // STA Statistics
        WriteTo(start, m_peerMacAddress);
        start.WriteU16(m_randomizationInterval);
        start.WriteU16(m_measurementDuration);
        start.WriteU8(m_groupIdentity);
        break;
    case 8: // LCI
        start.WriteU8(m_locationSubject);
        break;
    case 9: // Transmit Stream
        start.WriteU16(m_randomizationInterval);
        start.WriteU16(m_measurementDuration);
        WriteTo(start, m_peerStaAddress);
        start.WriteU8(m_trafficIdentifier);
        start.WriteU8(m_bin0Range);
        break;
    case 10: // Multicast Diagnostics
        start.WriteU16(m_randomizationInterval);
        start.WriteU16(m_measurementDuration);
        WriteTo(start, m_groupMacAddress);
        break;
    case 11: // Location Civic
        start.WriteU8(m_locationSubject);
        start.WriteU8(m_civicLocationType);
        start.WriteU8(m_locationServiceIntervalUnits);
        start.WriteU16(m_locationServiceInterval);
        break;
    case 12: // Location Identifier
        start.WriteU8(m_locationSubject);
        start.WriteU8(m_locationServiceIntervalUnits);
        start.WriteU16(m_locationServiceInterval);
        break;
    case 13: // Directional Channel Quality
        start.WriteU8(m_operatingClass);
        start.WriteU8(m_channelNumber);
        start.WriteU8(m_aid);
        start.WriteU8(0); // Reserved
        start.WriteU8(m_measurementMethod);
        start.WriteU64(m_measurementStartTime);
        start.WriteU16(m_measurementDuration);
        start.WriteU8(m_numberOfTimeBlocks);
        break;
    case 14: // Directional Measurement
        start.WriteU8(m_operatingClass);
        start.WriteU8(m_channelNumber);
        start.WriteU64(m_measurementStartTime);
        start.WriteU16(m_measurementDuration);
        start.WriteU8(m_measurementMethodAndAntennaConfiguration);
        break;
    case 15: // Directional Statistics
        start.WriteU8(m_operatingClass);
        start.WriteU8(m_channelNumber);
        start.WriteU64(m_measurementStartTime);
        start.WriteU16(m_measurementDuration);
        start.WriteU8(m_measurementMethod);
        start.WriteU8(m_directionalStatisticsBitmap);
        break;
    case 16: // FTM Range
        start.WriteU16(m_randomizationInterval);
        start.WriteU8(m_minimumApCount);
        break;
    case 255: // Measurement Pause
        start.WriteU16(m_pauseTime);
        break;
    default:
        break;
    }

    // Serialize subelements, guarded by measurement type.
    // Subelement IDs are type-scoped, so only write subelements valid for this type.
    if (m_measurementType == 5 && m_beaconSsid)
    {
        uint32_t ssidSerSize = m_beaconSsid->GetSerializedSize();
        uint16_t ssidInfoLen = static_cast<uint16_t>(ssidSerSize - 2);
        start.WriteU8(0); // subelement ID
        start.WriteU8(static_cast<uint8_t>(ssidInfoLen));
        Buffer ssidBuf;
        ssidBuf.AddAtStart(ssidSerSize);
        m_beaconSsid->Serialize(ssidBuf.Begin());
        Buffer::Iterator ssidIter = ssidBuf.Begin();
        ssidIter.Next(2); // skip IE Element ID and Length
        for (uint16_t j = 0; j < ssidInfoLen; j++)
        {
            start.WriteU8(ssidIter.ReadU8());
        }
    }
    if (m_measurementType == 3 && m_channelLoadReporting)
    {
        start.WriteU8(1); // subelement ID
        start.WriteU8(2); // length
        start.WriteU8(m_channelLoadReporting->reportingCondition);
        start.WriteU8(m_channelLoadReporting->channelLoadReferenceValue);
    }
    if (m_measurementType == 5 && m_beaconReporting)
    {
        start.WriteU8(1); // subelement ID
        start.WriteU8(2); // length
        start.WriteU8(m_beaconReporting->reportingCondition);
        start.WriteU8(m_beaconReporting->thresholdOffsetReference);
    }
    if (m_measurementType == 4 && m_noiseHistogramReporting)
    {
        start.WriteU8(1); // subelement ID
        start.WriteU8(2); // length
        start.WriteU8(m_noiseHistogramReporting->reportingCondition);
        start.WriteU8(m_noiseHistogramReporting->anpiReferenceValue);
    }
    if (m_measurementType == 8 && m_azimuthRequest)
    {
        start.WriteU8(1); // subelement ID
        start.WriteU8(1); // length
        uint8_t field = (m_azimuthRequest->azimuthResolution & 0x0F) |
                        ((m_azimuthRequest->azimuthType & 0x01) << 4);
        start.WriteU8(field);
    }
    if (m_measurementType == 5 && m_beaconReportingDetail)
    {
        start.WriteU8(2); // subelement ID
        start.WriteU8(1); // length
        start.WriteU8(*m_beaconReportingDetail);
    }
    if (m_measurementType == 5)
    {
        for (const auto& report : m_apChannelReports)
        {
            start.WriteU8(51); // subelement ID
            start.WriteU8(static_cast<uint8_t>(1 + report.channelList.size()));
            start.WriteU8(report.operatingClass);
            for (auto ch : report.channelList)
            {
                start.WriteU8(ch);
            }
        }
    }
    if (m_measurementType == 16)
    {
        for (const auto& nre : m_ftmRangeNeighborReports)
        {
            // Serialize the full NRE (IE header + info field) to a temp buffer,
            // then write subelement ID + length + info field bytes
            uint32_t nreSerSize = nre.GetSerializedSize();
            uint16_t nreInfoSize = static_cast<uint16_t>(nreSerSize - 2); // subtract IE header
            Buffer nreBuf;
            nreBuf.AddAtStart(nreSerSize);
            nre.Serialize(nreBuf.Begin());
            start.WriteU8(52); // subelement ID
            start.WriteU8(static_cast<uint8_t>(nreInfoSize));
            Buffer::Iterator nreIter = nreBuf.Begin();
            nreIter.Next(2); // skip IE Element ID and Length
            for (uint16_t j = 0; j < nreInfoSize; j++)
            {
                start.WriteU8(nreIter.ReadU8());
            }
        }
    }
    if (m_vendorSpecificSubelement)
    {
        start.WriteU8(221);
        start.WriteU8(static_cast<uint8_t>(m_vendorSpecificSubelement->size()));
        for (auto byte : *m_vendorSpecificSubelement)
        {
            start.WriteU8(byte);
        }
    }
}

uint16_t
MeasurementRequestElement::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    Buffer::Iterator i = start;
    m_measurementToken = i.ReadU8();
    m_measurementRequestMode = i.ReadU8() & 0x1F;
    m_measurementType = i.ReadU8();

    uint16_t bytesRead = 3;

    switch (m_measurementType)
    {
    case 0: // Basic
    case 1: // CCA
    case 2: // RPI Histogram
        m_channelNumber = i.ReadU8();
        m_measurementStartTime = i.ReadU64();
        m_measurementDuration = i.ReadU16();
        bytesRead += 11;
        break;
    case 3: // Channel Load
    case 4: // Noise Histogram
        m_operatingClass = i.ReadU8();
        m_channelNumber = i.ReadU8();
        m_randomizationInterval = i.ReadU16();
        m_measurementDuration = i.ReadU16();
        bytesRead += 6;
        break;
    case 5: // Beacon
        m_operatingClass = i.ReadU8();
        m_channelNumber = i.ReadU8();
        m_randomizationInterval = i.ReadU16();
        m_measurementDuration = i.ReadU16();
        m_beaconMeasurementMode = i.ReadU8();
        ReadFrom(i, m_bssid);
        bytesRead += 13;
        break;
    case 6: // Frame
        m_operatingClass = i.ReadU8();
        m_channelNumber = i.ReadU8();
        m_randomizationInterval = i.ReadU16();
        m_measurementDuration = i.ReadU16();
        m_frameRequestType = i.ReadU8();
        ReadFrom(i, m_macAddress);
        bytesRead += 13;
        break;
    case 7: // STA Statistics
        ReadFrom(i, m_peerMacAddress);
        m_randomizationInterval = i.ReadU16();
        m_measurementDuration = i.ReadU16();
        m_groupIdentity = i.ReadU8();
        bytesRead += 11;
        break;
    case 8: // LCI
        m_locationSubject = i.ReadU8();
        bytesRead += 1;
        break;
    case 9: // Transmit Stream
        m_randomizationInterval = i.ReadU16();
        m_measurementDuration = i.ReadU16();
        ReadFrom(i, m_peerStaAddress);
        m_trafficIdentifier = i.ReadU8();
        m_bin0Range = i.ReadU8();
        bytesRead += 12;
        break;
    case 10: // Multicast Diagnostics
        m_randomizationInterval = i.ReadU16();
        m_measurementDuration = i.ReadU16();
        ReadFrom(i, m_groupMacAddress);
        bytesRead += 10;
        break;
    case 11: // Location Civic
        m_locationSubject = i.ReadU8();
        m_civicLocationType = i.ReadU8();
        m_locationServiceIntervalUnits = i.ReadU8();
        m_locationServiceInterval = i.ReadU16();
        bytesRead += 5;
        break;
    case 12: // Location Identifier
        m_locationSubject = i.ReadU8();
        m_locationServiceIntervalUnits = i.ReadU8();
        m_locationServiceInterval = i.ReadU16();
        bytesRead += 4;
        break;
    case 13: // Directional Channel Quality
        m_operatingClass = i.ReadU8();
        m_channelNumber = i.ReadU8();
        m_aid = i.ReadU8();
        i.ReadU8(); // Reserved
        m_measurementMethod = i.ReadU8();
        m_measurementStartTime = i.ReadU64();
        m_measurementDuration = i.ReadU16();
        m_numberOfTimeBlocks = i.ReadU8();
        bytesRead += 16;
        break;
    case 14: // Directional Measurement
        m_operatingClass = i.ReadU8();
        m_channelNumber = i.ReadU8();
        m_measurementStartTime = i.ReadU64();
        m_measurementDuration = i.ReadU16();
        m_measurementMethodAndAntennaConfiguration = i.ReadU8();
        bytesRead += 13;
        break;
    case 15: // Directional Statistics
        m_operatingClass = i.ReadU8();
        m_channelNumber = i.ReadU8();
        m_measurementStartTime = i.ReadU64();
        m_measurementDuration = i.ReadU16();
        m_measurementMethod = i.ReadU8();
        m_directionalStatisticsBitmap = i.ReadU8();
        bytesRead += 14;
        break;
    case 16: // FTM Range
        m_randomizationInterval = i.ReadU16();
        m_minimumApCount = i.ReadU8();
        bytesRead += 3;
        break;
    case 255: // Measurement Pause
        m_pauseTime = i.ReadU16();
        bytesRead += 2;
        break;
    default:
        break;
    }

    // Parse subelements from remaining bytes.
    // Subelement IDs are scoped per measurement type, so dispatch on (type, ID).
    while (bytesRead < length)
    {
        uint8_t subelemId = i.ReadU8();
        uint8_t subelemLen = i.ReadU8();
        bytesRead += 2;

        bool handled = false;

        if (subelemId == 221)
        {
            // Vendor Specific -- valid for all types
            std::vector<uint8_t> data(subelemLen);
            for (uint8_t j = 0; j < subelemLen; j++)
            {
                data[j] = i.ReadU8();
            }
            m_vendorSpecificSubelement = std::move(data);
            handled = true;
        }
        else if (m_measurementType == 5)
        {
            // Beacon subelements
            switch (subelemId)
            {
            case 0: { // SSID
                Buffer ssidBuf;
                ssidBuf.AddAtStart(2 + subelemLen);
                Buffer::Iterator ssidIter = ssidBuf.Begin();
                ssidIter.WriteU8(0); // IE_SSID
                ssidIter.WriteU8(subelemLen);
                for (uint8_t j = 0; j < subelemLen; j++)
                {
                    ssidIter.WriteU8(i.ReadU8());
                }
                Ssid ssid;
                ssid.Deserialize(ssidBuf.Begin());
                m_beaconSsid = ssid;
                handled = true;
                break;
            }
            case 1: { // Beacon Reporting
                uint8_t condition = i.ReadU8();
                uint8_t thresholdOffsetRef = i.ReadU8();
                m_beaconReporting = BeaconReporting{condition, thresholdOffsetRef};
                handled = true;
                break;
            }
            case 2: { // Reporting Detail
                m_beaconReportingDetail = i.ReadU8();
                handled = true;
                break;
            }
            case 51: { // AP Channel Report
                if (subelemLen < 1)
                {
                    break;
                }
                uint8_t opClass = i.ReadU8();
                uint8_t channelCount = subelemLen - 1;
                std::vector<uint8_t> channels(channelCount);
                for (uint8_t j = 0; j < channelCount; j++)
                {
                    channels[j] = i.ReadU8();
                }
                m_apChannelReports.push_back(ApChannelReport{opClass, std::move(channels)});
                handled = true;
                break;
            }
            default:
                break;
            }
        }
        else if (m_measurementType == 8)
        {
            // LCI subelements
            if (subelemId == 1)
            {
                // Azimuth Request
                uint8_t field = i.ReadU8();
                m_azimuthRequest = AzimuthRequest{static_cast<uint8_t>(field & 0x0F),
                                                  static_cast<uint8_t>((field >> 4) & 0x01)};
                handled = true;
            }
        }
        else if (m_measurementType == 3)
        {
            // Channel Load subelements
            if (subelemId == 1)
            {
                uint8_t condition = i.ReadU8();
                uint8_t refValue = i.ReadU8();
                m_channelLoadReporting = ChannelLoadReporting{condition, refValue};
                handled = true;
            }
        }
        else if (m_measurementType == 4)
        {
            // Noise Histogram subelements
            if (subelemId == 1)
            {
                uint8_t condition = i.ReadU8();
                uint8_t refValue = i.ReadU8();
                m_noiseHistogramReporting = NoiseHistogramReporting{condition, refValue};
                handled = true;
            }
        }
        else if (m_measurementType == 16)
        {
            // FTM Range subelements
            if (subelemId == 52)
            {
                // Neighbor Report
                Buffer nreBuf;
                nreBuf.AddAtStart(2 + subelemLen);
                Buffer::Iterator nreIter = nreBuf.Begin();
                nreIter.WriteU8(52); // IE_NEIGHBOR_REPORT
                nreIter.WriteU8(subelemLen);
                for (uint8_t j = 0; j < subelemLen; j++)
                {
                    nreIter.WriteU8(i.ReadU8());
                }
                NeighborReportElement nre;
                nre.Deserialize(nreBuf.Begin());
                m_ftmRangeNeighborReports.push_back(nre);
                handled = true;
            }
        }

        if (!handled)
        {
            // Unknown subelement for this type, skip
            for (uint8_t j = 0; j < subelemLen; j++)
            {
                i.ReadU8();
            }
        }

        bytesRead += subelemLen;
    }

    return length;
}

void
MeasurementRequestElement::Print(std::ostream& os) const
{
    os << "MeasurementRequest=[Token=" << +m_measurementToken << ", Mode=0x" << std::hex
       << +m_measurementRequestMode << std::dec << ", Type=" << +m_measurementType << "]";
}

} // namespace ns3
