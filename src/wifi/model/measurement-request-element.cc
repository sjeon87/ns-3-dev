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
    : m_body(BasicRequestBody{})
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
    switch (type)
    {
    case 0:
    case 1:
    case 2:
        m_body = BasicRequestBody{};
        break;
    case 3:
        m_body = ChannelLoadRequestBody{};
        break;
    case 4:
        m_body = NoiseHistogramRequestBody{};
        break;
    case 5:
        m_body = BeaconRequestBody{};
        break;
    case 6:
        m_body = FrameRequestBody{};
        break;
    case 7:
        m_body = StaStatisticsRequestBody{};
        break;
    case 8:
        m_body = LciRequestBody{};
        break;
    case 9:
        m_body = TransmitStreamRequestBody{};
        break;
    case 10:
        m_body = MulticastDiagnosticsRequestBody{};
        break;
    case 11:
        m_body = LocationCivicRequestBody{};
        break;
    case 12:
        m_body = LocationIdentifierRequestBody{};
        break;
    case 13:
        m_body = DirectionalChannelQualityRequestBody{};
        break;
    case 14:
        m_body = DirectionalMeasurementRequestBody{};
        break;
    case 15:
        m_body = DirectionalStatisticsRequestBody{};
        break;
    case 16:
        m_body = FtmRangeRequestBody{};
        break;
    case 255:
        m_body = MeasurementPauseRequestBody{};
        break;
    default:
        m_body = std::monostate{};
        break;
    }
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

// --- Type-specific body field accessors (facade over variant) ---

void
MeasurementRequestElement::SetOperatingClass(uint8_t operatingClass)
{
    switch (m_measurementType)
    {
    case 3:
        std::get<ChannelLoadRequestBody>(m_body).operatingClass = operatingClass;
        break;
    case 4:
        std::get<NoiseHistogramRequestBody>(m_body).operatingClass = operatingClass;
        break;
    case 5:
        std::get<BeaconRequestBody>(m_body).operatingClass = operatingClass;
        break;
    case 6:
        std::get<FrameRequestBody>(m_body).operatingClass = operatingClass;
        break;
    case 13:
        std::get<DirectionalChannelQualityRequestBody>(m_body).operatingClass = operatingClass;
        break;
    case 14:
        std::get<DirectionalMeasurementRequestBody>(m_body).operatingClass = operatingClass;
        break;
    case 15:
        std::get<DirectionalStatisticsRequestBody>(m_body).operatingClass = operatingClass;
        break;
    default:
        break;
    }
}

uint8_t
MeasurementRequestElement::GetOperatingClass() const
{
    switch (m_measurementType)
    {
    case 3:
        return std::get<ChannelLoadRequestBody>(m_body).operatingClass;
    case 4:
        return std::get<NoiseHistogramRequestBody>(m_body).operatingClass;
    case 5:
        return std::get<BeaconRequestBody>(m_body).operatingClass;
    case 6:
        return std::get<FrameRequestBody>(m_body).operatingClass;
    case 13:
        return std::get<DirectionalChannelQualityRequestBody>(m_body).operatingClass;
    case 14:
        return std::get<DirectionalMeasurementRequestBody>(m_body).operatingClass;
    case 15:
        return std::get<DirectionalStatisticsRequestBody>(m_body).operatingClass;
    default:
        return 0;
    }
}

void
MeasurementRequestElement::SetChannelNumber(uint8_t channel)
{
    switch (m_measurementType)
    {
    case 0:
    case 1:
    case 2:
        std::get<BasicRequestBody>(m_body).channelNumber = channel;
        break;
    case 3:
        std::get<ChannelLoadRequestBody>(m_body).channelNumber = channel;
        break;
    case 4:
        std::get<NoiseHistogramRequestBody>(m_body).channelNumber = channel;
        break;
    case 5:
        std::get<BeaconRequestBody>(m_body).channelNumber = channel;
        break;
    case 6:
        std::get<FrameRequestBody>(m_body).channelNumber = channel;
        break;
    case 13:
        std::get<DirectionalChannelQualityRequestBody>(m_body).channelNumber = channel;
        break;
    case 14:
        std::get<DirectionalMeasurementRequestBody>(m_body).channelNumber = channel;
        break;
    case 15:
        std::get<DirectionalStatisticsRequestBody>(m_body).channelNumber = channel;
        break;
    default:
        break;
    }
}

uint8_t
MeasurementRequestElement::GetChannelNumber() const
{
    switch (m_measurementType)
    {
    case 0:
    case 1:
    case 2:
        return std::get<BasicRequestBody>(m_body).channelNumber;
    case 3:
        return std::get<ChannelLoadRequestBody>(m_body).channelNumber;
    case 4:
        return std::get<NoiseHistogramRequestBody>(m_body).channelNumber;
    case 5:
        return std::get<BeaconRequestBody>(m_body).channelNumber;
    case 6:
        return std::get<FrameRequestBody>(m_body).channelNumber;
    case 13:
        return std::get<DirectionalChannelQualityRequestBody>(m_body).channelNumber;
    case 14:
        return std::get<DirectionalMeasurementRequestBody>(m_body).channelNumber;
    case 15:
        return std::get<DirectionalStatisticsRequestBody>(m_body).channelNumber;
    default:
        return 0;
    }
}

void
MeasurementRequestElement::SetRandomizationInterval(uint16_t interval)
{
    switch (m_measurementType)
    {
    case 3:
        std::get<ChannelLoadRequestBody>(m_body).randomizationInterval = interval;
        break;
    case 4:
        std::get<NoiseHistogramRequestBody>(m_body).randomizationInterval = interval;
        break;
    case 5:
        std::get<BeaconRequestBody>(m_body).randomizationInterval = interval;
        break;
    case 6:
        std::get<FrameRequestBody>(m_body).randomizationInterval = interval;
        break;
    case 7:
        std::get<StaStatisticsRequestBody>(m_body).randomizationInterval = interval;
        break;
    case 9:
        std::get<TransmitStreamRequestBody>(m_body).randomizationInterval = interval;
        break;
    case 10:
        std::get<MulticastDiagnosticsRequestBody>(m_body).randomizationInterval = interval;
        break;
    case 16:
        std::get<FtmRangeRequestBody>(m_body).randomizationInterval = interval;
        break;
    default:
        break;
    }
}

uint16_t
MeasurementRequestElement::GetRandomizationInterval() const
{
    switch (m_measurementType)
    {
    case 3:
        return std::get<ChannelLoadRequestBody>(m_body).randomizationInterval;
    case 4:
        return std::get<NoiseHistogramRequestBody>(m_body).randomizationInterval;
    case 5:
        return std::get<BeaconRequestBody>(m_body).randomizationInterval;
    case 6:
        return std::get<FrameRequestBody>(m_body).randomizationInterval;
    case 7:
        return std::get<StaStatisticsRequestBody>(m_body).randomizationInterval;
    case 9:
        return std::get<TransmitStreamRequestBody>(m_body).randomizationInterval;
    case 10:
        return std::get<MulticastDiagnosticsRequestBody>(m_body).randomizationInterval;
    case 16:
        return std::get<FtmRangeRequestBody>(m_body).randomizationInterval;
    default:
        return 0;
    }
}

void
MeasurementRequestElement::SetMeasurementDuration(uint16_t duration)
{
    switch (m_measurementType)
    {
    case 0:
    case 1:
    case 2:
        std::get<BasicRequestBody>(m_body).measurementDuration = duration;
        break;
    case 3:
        std::get<ChannelLoadRequestBody>(m_body).measurementDuration = duration;
        break;
    case 4:
        std::get<NoiseHistogramRequestBody>(m_body).measurementDuration = duration;
        break;
    case 5:
        std::get<BeaconRequestBody>(m_body).measurementDuration = duration;
        break;
    case 6:
        std::get<FrameRequestBody>(m_body).measurementDuration = duration;
        break;
    case 7:
        std::get<StaStatisticsRequestBody>(m_body).measurementDuration = duration;
        break;
    case 9:
        std::get<TransmitStreamRequestBody>(m_body).measurementDuration = duration;
        break;
    case 10:
        std::get<MulticastDiagnosticsRequestBody>(m_body).measurementDuration = duration;
        break;
    case 13:
        std::get<DirectionalChannelQualityRequestBody>(m_body).measurementDuration = duration;
        break;
    case 14:
        std::get<DirectionalMeasurementRequestBody>(m_body).measurementDurationPerDirection =
            duration;
        break;
    case 15:
        std::get<DirectionalStatisticsRequestBody>(m_body).measurementDurationPerDirection =
            duration;
        break;
    default:
        break;
    }
}

uint16_t
MeasurementRequestElement::GetMeasurementDuration() const
{
    switch (m_measurementType)
    {
    case 0:
    case 1:
    case 2:
        return std::get<BasicRequestBody>(m_body).measurementDuration;
    case 3:
        return std::get<ChannelLoadRequestBody>(m_body).measurementDuration;
    case 4:
        return std::get<NoiseHistogramRequestBody>(m_body).measurementDuration;
    case 5:
        return std::get<BeaconRequestBody>(m_body).measurementDuration;
    case 6:
        return std::get<FrameRequestBody>(m_body).measurementDuration;
    case 7:
        return std::get<StaStatisticsRequestBody>(m_body).measurementDuration;
    case 9:
        return std::get<TransmitStreamRequestBody>(m_body).measurementDuration;
    case 10:
        return std::get<MulticastDiagnosticsRequestBody>(m_body).measurementDuration;
    case 13:
        return std::get<DirectionalChannelQualityRequestBody>(m_body).measurementDuration;
    case 14:
        return std::get<DirectionalMeasurementRequestBody>(m_body).measurementDurationPerDirection;
    case 15:
        return std::get<DirectionalStatisticsRequestBody>(m_body).measurementDurationPerDirection;
    default:
        return 0;
    }
}

void
MeasurementRequestElement::SetBeaconMeasurementMode(uint8_t mode)
{
    std::get<BeaconRequestBody>(m_body).measurementMode = mode;
}

uint8_t
MeasurementRequestElement::GetBeaconMeasurementMode() const
{
    return std::get<BeaconRequestBody>(m_body).measurementMode;
}

void
MeasurementRequestElement::SetBssid(Mac48Address bssid)
{
    std::get<BeaconRequestBody>(m_body).bssid = bssid;
}

Mac48Address
MeasurementRequestElement::GetBssid() const
{
    return std::get<BeaconRequestBody>(m_body).bssid;
}

void
MeasurementRequestElement::SetFrameRequestType(uint8_t frameRequestType)
{
    std::get<FrameRequestBody>(m_body).frameRequestType = frameRequestType;
}

uint8_t
MeasurementRequestElement::GetFrameRequestType() const
{
    return std::get<FrameRequestBody>(m_body).frameRequestType;
}

void
MeasurementRequestElement::SetMacAddress(Mac48Address macAddress)
{
    std::get<FrameRequestBody>(m_body).macAddress = macAddress;
}

Mac48Address
MeasurementRequestElement::GetMacAddress() const
{
    return std::get<FrameRequestBody>(m_body).macAddress;
}

void
MeasurementRequestElement::SetPeerMacAddress(Mac48Address peerMacAddress)
{
    std::get<StaStatisticsRequestBody>(m_body).peerMacAddress = peerMacAddress;
}

Mac48Address
MeasurementRequestElement::GetPeerMacAddress() const
{
    return std::get<StaStatisticsRequestBody>(m_body).peerMacAddress;
}

void
MeasurementRequestElement::SetGroupIdentity(uint8_t groupIdentity)
{
    std::get<StaStatisticsRequestBody>(m_body).groupIdentity = groupIdentity;
}

uint8_t
MeasurementRequestElement::GetGroupIdentity() const
{
    return std::get<StaStatisticsRequestBody>(m_body).groupIdentity;
}

void
MeasurementRequestElement::SetLocationSubject(uint8_t locationSubject)
{
    switch (m_measurementType)
    {
    case 8:
        std::get<LciRequestBody>(m_body).locationSubject = locationSubject;
        break;
    case 11:
        std::get<LocationCivicRequestBody>(m_body).locationSubject = locationSubject;
        break;
    case 12:
        std::get<LocationIdentifierRequestBody>(m_body).locationSubject = locationSubject;
        break;
    default:
        break;
    }
}

uint8_t
MeasurementRequestElement::GetLocationSubject() const
{
    switch (m_measurementType)
    {
    case 8:
        return std::get<LciRequestBody>(m_body).locationSubject;
    case 11:
        return std::get<LocationCivicRequestBody>(m_body).locationSubject;
    case 12:
        return std::get<LocationIdentifierRequestBody>(m_body).locationSubject;
    default:
        return 0;
    }
}

void
MeasurementRequestElement::SetPeerStaAddress(Mac48Address peerStaAddress)
{
    std::get<TransmitStreamRequestBody>(m_body).peerStaAddress = peerStaAddress;
}

Mac48Address
MeasurementRequestElement::GetPeerStaAddress() const
{
    return std::get<TransmitStreamRequestBody>(m_body).peerStaAddress;
}

void
MeasurementRequestElement::SetTrafficIdentifier(uint8_t tid)
{
    std::get<TransmitStreamRequestBody>(m_body).trafficIdentifier = tid;
}

uint8_t
MeasurementRequestElement::GetTrafficIdentifier() const
{
    return std::get<TransmitStreamRequestBody>(m_body).trafficIdentifier;
}

void
MeasurementRequestElement::SetBin0Range(uint8_t bin0Range)
{
    std::get<TransmitStreamRequestBody>(m_body).bin0Range = bin0Range;
}

uint8_t
MeasurementRequestElement::GetBin0Range() const
{
    return std::get<TransmitStreamRequestBody>(m_body).bin0Range;
}

void
MeasurementRequestElement::SetGroupMacAddress(Mac48Address groupMacAddress)
{
    std::get<MulticastDiagnosticsRequestBody>(m_body).groupMacAddress = groupMacAddress;
}

Mac48Address
MeasurementRequestElement::GetGroupMacAddress() const
{
    return std::get<MulticastDiagnosticsRequestBody>(m_body).groupMacAddress;
}

void
MeasurementRequestElement::SetCivicLocationType(uint8_t civicLocationType)
{
    std::get<LocationCivicRequestBody>(m_body).civicLocationType = civicLocationType;
}

uint8_t
MeasurementRequestElement::GetCivicLocationType() const
{
    return std::get<LocationCivicRequestBody>(m_body).civicLocationType;
}

void
MeasurementRequestElement::SetLocationServiceIntervalUnits(uint8_t units)
{
    switch (m_measurementType)
    {
    case 11:
        std::get<LocationCivicRequestBody>(m_body).locationServiceIntervalUnits = units;
        break;
    case 12:
        std::get<LocationIdentifierRequestBody>(m_body).locationServiceIntervalUnits = units;
        break;
    default:
        break;
    }
}

uint8_t
MeasurementRequestElement::GetLocationServiceIntervalUnits() const
{
    switch (m_measurementType)
    {
    case 11:
        return std::get<LocationCivicRequestBody>(m_body).locationServiceIntervalUnits;
    case 12:
        return std::get<LocationIdentifierRequestBody>(m_body).locationServiceIntervalUnits;
    default:
        return 0;
    }
}

void
MeasurementRequestElement::SetLocationServiceInterval(uint16_t interval)
{
    switch (m_measurementType)
    {
    case 11:
        std::get<LocationCivicRequestBody>(m_body).locationServiceInterval = interval;
        break;
    case 12:
        std::get<LocationIdentifierRequestBody>(m_body).locationServiceInterval = interval;
        break;
    default:
        break;
    }
}

uint16_t
MeasurementRequestElement::GetLocationServiceInterval() const
{
    switch (m_measurementType)
    {
    case 11:
        return std::get<LocationCivicRequestBody>(m_body).locationServiceInterval;
    case 12:
        return std::get<LocationIdentifierRequestBody>(m_body).locationServiceInterval;
    default:
        return 0;
    }
}

void
MeasurementRequestElement::SetAid(uint8_t aid)
{
    std::get<DirectionalChannelQualityRequestBody>(m_body).aid = aid;
}

uint8_t
MeasurementRequestElement::GetAid() const
{
    return std::get<DirectionalChannelQualityRequestBody>(m_body).aid;
}

void
MeasurementRequestElement::SetMeasurementMethod(uint8_t method)
{
    switch (m_measurementType)
    {
    case 13:
        std::get<DirectionalChannelQualityRequestBody>(m_body).measurementMethod = method;
        break;
    case 15:
        std::get<DirectionalStatisticsRequestBody>(m_body).measurementMethod = method;
        break;
    default:
        break;
    }
}

uint8_t
MeasurementRequestElement::GetMeasurementMethod() const
{
    switch (m_measurementType)
    {
    case 13:
        return std::get<DirectionalChannelQualityRequestBody>(m_body).measurementMethod;
    case 15:
        return std::get<DirectionalStatisticsRequestBody>(m_body).measurementMethod;
    default:
        return 0;
    }
}

void
MeasurementRequestElement::SetMeasurementStartTime(uint64_t startTime)
{
    switch (m_measurementType)
    {
    case 0:
    case 1:
    case 2:
        std::get<BasicRequestBody>(m_body).measurementStartTime = startTime;
        break;
    case 13:
        std::get<DirectionalChannelQualityRequestBody>(m_body).measurementStartTime = startTime;
        break;
    case 14:
        std::get<DirectionalMeasurementRequestBody>(m_body).measurementStartTime = startTime;
        break;
    case 15:
        std::get<DirectionalStatisticsRequestBody>(m_body).measurementStartTime = startTime;
        break;
    default:
        break;
    }
}

uint64_t
MeasurementRequestElement::GetMeasurementStartTime() const
{
    switch (m_measurementType)
    {
    case 0:
    case 1:
    case 2:
        return std::get<BasicRequestBody>(m_body).measurementStartTime;
    case 13:
        return std::get<DirectionalChannelQualityRequestBody>(m_body).measurementStartTime;
    case 14:
        return std::get<DirectionalMeasurementRequestBody>(m_body).measurementStartTime;
    case 15:
        return std::get<DirectionalStatisticsRequestBody>(m_body).measurementStartTime;
    default:
        return 0;
    }
}

void
MeasurementRequestElement::SetNumberOfTimeBlocks(uint8_t numberOfTimeBlocks)
{
    std::get<DirectionalChannelQualityRequestBody>(m_body).numberOfTimeBlocks = numberOfTimeBlocks;
}

uint8_t
MeasurementRequestElement::GetNumberOfTimeBlocks() const
{
    return std::get<DirectionalChannelQualityRequestBody>(m_body).numberOfTimeBlocks;
}

void
MeasurementRequestElement::SetMeasurementMethodAndAntennaConfiguration(uint8_t value)
{
    std::get<DirectionalMeasurementRequestBody>(m_body).measurementMethodAndAntennaConfiguration =
        value;
}

uint8_t
MeasurementRequestElement::GetMeasurementMethodAndAntennaConfiguration() const
{
    return std::get<DirectionalMeasurementRequestBody>(m_body)
        .measurementMethodAndAntennaConfiguration;
}

void
MeasurementRequestElement::SetDirectionalStatisticsBitmap(uint8_t bitmap)
{
    std::get<DirectionalStatisticsRequestBody>(m_body).directionalStatisticsBitmap = bitmap;
}

uint8_t
MeasurementRequestElement::GetDirectionalStatisticsBitmap() const
{
    return std::get<DirectionalStatisticsRequestBody>(m_body).directionalStatisticsBitmap;
}

void
MeasurementRequestElement::SetMinimumApCount(uint8_t minApCount)
{
    std::get<FtmRangeRequestBody>(m_body).minimumApCount = minApCount;
}

uint8_t
MeasurementRequestElement::GetMinimumApCount() const
{
    return std::get<FtmRangeRequestBody>(m_body).minimumApCount;
}

void
MeasurementRequestElement::SetPauseTime(uint16_t pauseTime)
{
    std::get<MeasurementPauseRequestBody>(m_body).pauseTime = pauseTime;
}

uint16_t
MeasurementRequestElement::GetPauseTime() const
{
    return std::get<MeasurementPauseRequestBody>(m_body).pauseTime;
}

// --- Optional subelement accessors (facade over variant) ---

void
MeasurementRequestElement::SetChannelLoadReporting(uint8_t condition, uint8_t channelLoadRefValue)
{
    std::get<ChannelLoadRequestBody>(m_body).channelLoadReporting =
        ChannelLoadReporting{condition, channelLoadRefValue};
}

std::optional<MeasurementRequestElement::ChannelLoadReporting>
MeasurementRequestElement::GetChannelLoadReporting() const
{
    return std::get<ChannelLoadRequestBody>(m_body).channelLoadReporting;
}

void
MeasurementRequestElement::SetNoiseHistogramReporting(uint8_t condition, uint8_t anpiRefValue)
{
    std::get<NoiseHistogramRequestBody>(m_body).noiseHistogramReporting =
        NoiseHistogramReporting{condition, anpiRefValue};
}

std::optional<MeasurementRequestElement::NoiseHistogramReporting>
MeasurementRequestElement::GetNoiseHistogramReporting() const
{
    return std::get<NoiseHistogramRequestBody>(m_body).noiseHistogramReporting;
}

void
MeasurementRequestElement::SetBeaconReporting(uint8_t condition, uint8_t thresholdOffsetRef)
{
    std::get<BeaconRequestBody>(m_body).beaconReporting =
        BeaconReporting{condition, thresholdOffsetRef};
}

std::optional<MeasurementRequestElement::BeaconReporting>
MeasurementRequestElement::GetBeaconReporting() const
{
    return std::get<BeaconRequestBody>(m_body).beaconReporting;
}

void
MeasurementRequestElement::SetBeaconSsid(Ssid ssid)
{
    std::get<BeaconRequestBody>(m_body).ssid = ssid;
}

std::optional<Ssid>
MeasurementRequestElement::GetBeaconSsid() const
{
    return std::get<BeaconRequestBody>(m_body).ssid;
}

void
MeasurementRequestElement::SetBeaconReportingDetail(uint8_t detail)
{
    std::get<BeaconRequestBody>(m_body).reportingDetail = detail;
}

std::optional<uint8_t>
MeasurementRequestElement::GetBeaconReportingDetail() const
{
    return std::get<BeaconRequestBody>(m_body).reportingDetail;
}

void
MeasurementRequestElement::AddApChannelReport(uint8_t opClass, std::vector<uint8_t> channels)
{
    std::get<BeaconRequestBody>(m_body).apChannelReports.push_back(
        ApChannelReport{opClass, std::move(channels)});
}

std::vector<MeasurementRequestElement::ApChannelReport>
MeasurementRequestElement::GetApChannelReports() const
{
    return std::get<BeaconRequestBody>(m_body).apChannelReports;
}

void
MeasurementRequestElement::SetAzimuthRequest(uint8_t resolution, uint8_t type)
{
    std::get<LciRequestBody>(m_body).azimuthRequest = AzimuthRequest{resolution, type};
}

std::optional<MeasurementRequestElement::AzimuthRequest>
MeasurementRequestElement::GetAzimuthRequest() const
{
    return std::get<LciRequestBody>(m_body).azimuthRequest;
}

void
MeasurementRequestElement::AddFtmRangeNeighborReport(const NeighborReportElement& nre)
{
    std::get<FtmRangeRequestBody>(m_body).neighborReports.push_back(nre);
}

std::vector<NeighborReportElement>
MeasurementRequestElement::GetFtmRangeNeighborReports() const
{
    return std::get<FtmRangeRequestBody>(m_body).neighborReports;
}

void
MeasurementRequestElement::SetVendorSpecificSubelement(std::vector<uint8_t> data)
{
    std::visit(
        [&data](auto& body) {
            using T = std::decay_t<decltype(body)>;
            if constexpr (!std::is_same_v<T, std::monostate> &&
                          !std::is_same_v<T, BasicRequestBody>)
            {
                body.vendorSpecific = std::move(data);
            }
        },
        m_body);
}

std::optional<std::vector<uint8_t>>
MeasurementRequestElement::GetVendorSpecificSubelement() const
{
    return std::visit(
        [](const auto& body) -> std::optional<std::vector<uint8_t>> {
            using T = std::decay_t<decltype(body)>;
            if constexpr (!std::is_same_v<T, std::monostate> &&
                          !std::is_same_v<T, BasicRequestBody>)
            {
                return body.vendorSpecific;
            }
            else
            {
                return std::nullopt;
            }
        },
        m_body);
}

// --- Subelement serialization helpers ---

namespace
{

uint16_t
VendorSpecificSize(const std::optional<std::vector<uint8_t>>& vs)
{
    return vs ? static_cast<uint16_t>(2 + vs->size()) : 0;
}

void
SerializeVendorSpecific(Buffer::Iterator& start, const std::optional<std::vector<uint8_t>>& vs)
{
    if (vs)
    {
        start.WriteU8(221);
        start.WriteU8(static_cast<uint8_t>(vs->size()));
        for (auto byte : *vs)
        {
            start.WriteU8(byte);
        }
    }
}

void
SerializeSsidSubelement(Buffer::Iterator& start, const std::optional<Ssid>& ssid)
{
    if (ssid)
    {
        uint32_t ssidSerSize = ssid->GetSerializedSize();
        uint16_t ssidInfoLen = static_cast<uint16_t>(ssidSerSize - 2);
        start.WriteU8(0); // subelement ID
        start.WriteU8(static_cast<uint8_t>(ssidInfoLen));
        Buffer ssidBuf;
        ssidBuf.AddAtStart(ssidSerSize);
        ssid->Serialize(ssidBuf.Begin());
        Buffer::Iterator ssidIter = ssidBuf.Begin();
        ssidIter.Next(2); // skip IE Element ID and Length
        for (uint16_t j = 0; j < ssidInfoLen; j++)
        {
            start.WriteU8(ssidIter.ReadU8());
        }
    }
}

void
SerializeNeighborReports(Buffer::Iterator& start,
                         const std::vector<NeighborReportElement>& neighborReports)
{
    for (const auto& nre : neighborReports)
    {
        uint32_t nreSerSize = nre.GetSerializedSize();
        uint16_t nreInfoSize = static_cast<uint16_t>(nreSerSize - 2);
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

} // anonymous namespace

// --- WifiInformationElement overrides ---

uint16_t
MeasurementRequestElement::GetInformationFieldSize() const
{
    uint16_t size = 3; // token(1) + mode(1) + type(1)

    std::visit(
        [&size](const auto& body) {
            using T = std::decay_t<decltype(body)>;
            if constexpr (std::is_same_v<T, std::monostate>)
            {
                // no body
            }
            else if constexpr (std::is_same_v<T, BasicRequestBody>)
            {
                size += 11; // Channel(1) + StartTime(8) + Duration(2)
            }
            else if constexpr (std::is_same_v<T, ChannelLoadRequestBody>)
            {
                size += 6; // OpClass(1) + Channel(1) + RandInterval(2) + Duration(2)
                if (body.channelLoadReporting)
                {
                    size += 4;
                }
                size += VendorSpecificSize(body.vendorSpecific);
            }
            else if constexpr (std::is_same_v<T, NoiseHistogramRequestBody>)
            {
                size += 6;
                if (body.noiseHistogramReporting)
                {
                    size += 4;
                }
                size += VendorSpecificSize(body.vendorSpecific);
            }
            else if constexpr (std::is_same_v<T, BeaconRequestBody>)
            {
                size += 13; // OpClass(1)+Ch(1)+Rand(2)+Dur(2)+Mode(1)+BSSID(6)
                if (body.ssid)
                {
                    size += 2 + (body.ssid->GetSerializedSize() - 2);
                }
                if (body.beaconReporting)
                {
                    size += 4;
                }
                if (body.reportingDetail)
                {
                    size += 3;
                }
                for (const auto& report : body.apChannelReports)
                {
                    size += static_cast<uint16_t>(2 + 1 + report.channelList.size());
                }
                size += VendorSpecificSize(body.vendorSpecific);
            }
            else if constexpr (std::is_same_v<T, FrameRequestBody>)
            {
                size += 13; // OpClass(1)+Ch(1)+Rand(2)+Dur(2)+FReqType(1)+MAC(6)
                size += VendorSpecificSize(body.vendorSpecific);
            }
            else if constexpr (std::is_same_v<T, StaStatisticsRequestBody>)
            {
                size += 11; // PeerMAC(6)+Rand(2)+Dur(2)+GroupID(1)
                size += VendorSpecificSize(body.vendorSpecific);
            }
            else if constexpr (std::is_same_v<T, LciRequestBody>)
            {
                size += 1; // LocationSubject(1)
                if (body.azimuthRequest)
                {
                    size += 3;
                }
                size += VendorSpecificSize(body.vendorSpecific);
            }
            else if constexpr (std::is_same_v<T, TransmitStreamRequestBody>)
            {
                size += 12; // Rand(2)+Dur(2)+PeerSTA(6)+TID(1)+Bin0Range(1)
                size += VendorSpecificSize(body.vendorSpecific);
            }
            else if constexpr (std::is_same_v<T, MulticastDiagnosticsRequestBody>)
            {
                size += 10; // Rand(2)+Dur(2)+GroupMAC(6)
                size += VendorSpecificSize(body.vendorSpecific);
            }
            else if constexpr (std::is_same_v<T, LocationCivicRequestBody>)
            {
                size += 5; // LocSubject(1)+CivicType(1)+ServiceUnits(1)+ServiceInterval(2)
                size += VendorSpecificSize(body.vendorSpecific);
            }
            else if constexpr (std::is_same_v<T, LocationIdentifierRequestBody>)
            {
                size += 4; // LocSubject(1)+ServiceUnits(1)+ServiceInterval(2)
                size += VendorSpecificSize(body.vendorSpecific);
            }
            else if constexpr (std::is_same_v<T, DirectionalChannelQualityRequestBody>)
            {
                size += 16; // OpClass(1)+Ch(1)+AID(1)+Rsv(1)+Method(1)+Start(8)+Dur(2)+Blocks(1)
                size += VendorSpecificSize(body.vendorSpecific);
            }
            else if constexpr (std::is_same_v<T, DirectionalMeasurementRequestBody>)
            {
                size += 13; // OpClass(1)+Ch(1)+Start(8)+Dur(2)+MethodAntenna(1)
                size += VendorSpecificSize(body.vendorSpecific);
            }
            else if constexpr (std::is_same_v<T, DirectionalStatisticsRequestBody>)
            {
                size += 14; // OpClass(1)+Ch(1)+Start(8)+Dur(2)+Method(1)+Bitmap(1)
                size += VendorSpecificSize(body.vendorSpecific);
            }
            else if constexpr (std::is_same_v<T, FtmRangeRequestBody>)
            {
                size += 3; // Rand(2)+MinAPCount(1)
                for (const auto& nre : body.neighborReports)
                {
                    size += static_cast<uint16_t>(2 + (nre.GetSerializedSize() - 2));
                }
                size += VendorSpecificSize(body.vendorSpecific);
            }
            else if constexpr (std::is_same_v<T, MeasurementPauseRequestBody>)
            {
                size += 2; // PauseTime(2)
                size += VendorSpecificSize(body.vendorSpecific);
            }
        },
        m_body);

    return size;
}

void
MeasurementRequestElement::SerializeInformationField(Buffer::Iterator start) const
{
    start.WriteU8(m_measurementToken);
    start.WriteU8(m_measurementRequestMode);
    start.WriteU8(m_measurementType);

    std::visit(
        [&start](const auto& body) {
            using T = std::decay_t<decltype(body)>;
            if constexpr (std::is_same_v<T, std::monostate>)
            {
                // no body
            }
            else if constexpr (std::is_same_v<T, BasicRequestBody>)
            {
                start.WriteU8(body.channelNumber);
                start.WriteU64(body.measurementStartTime);
                start.WriteU16(body.measurementDuration);
            }
            else if constexpr (std::is_same_v<T, ChannelLoadRequestBody>)
            {
                start.WriteU8(body.operatingClass);
                start.WriteU8(body.channelNumber);
                start.WriteU16(body.randomizationInterval);
                start.WriteU16(body.measurementDuration);
                if (body.channelLoadReporting)
                {
                    start.WriteU8(1);
                    start.WriteU8(2);
                    start.WriteU8(body.channelLoadReporting->reportingCondition);
                    start.WriteU8(body.channelLoadReporting->channelLoadReferenceValue);
                }
                SerializeVendorSpecific(start, body.vendorSpecific);
            }
            else if constexpr (std::is_same_v<T, NoiseHistogramRequestBody>)
            {
                start.WriteU8(body.operatingClass);
                start.WriteU8(body.channelNumber);
                start.WriteU16(body.randomizationInterval);
                start.WriteU16(body.measurementDuration);
                if (body.noiseHistogramReporting)
                {
                    start.WriteU8(1);
                    start.WriteU8(2);
                    start.WriteU8(body.noiseHistogramReporting->reportingCondition);
                    start.WriteU8(body.noiseHistogramReporting->anpiReferenceValue);
                }
                SerializeVendorSpecific(start, body.vendorSpecific);
            }
            else if constexpr (std::is_same_v<T, BeaconRequestBody>)
            {
                start.WriteU8(body.operatingClass);
                start.WriteU8(body.channelNumber);
                start.WriteU16(body.randomizationInterval);
                start.WriteU16(body.measurementDuration);
                start.WriteU8(body.measurementMode);
                WriteTo(start, body.bssid);
                SerializeSsidSubelement(start, body.ssid);
                if (body.beaconReporting)
                {
                    start.WriteU8(1);
                    start.WriteU8(2);
                    start.WriteU8(body.beaconReporting->reportingCondition);
                    start.WriteU8(body.beaconReporting->thresholdOffsetReference);
                }
                if (body.reportingDetail)
                {
                    start.WriteU8(2);
                    start.WriteU8(1);
                    start.WriteU8(*body.reportingDetail);
                }
                for (const auto& report : body.apChannelReports)
                {
                    start.WriteU8(51);
                    start.WriteU8(static_cast<uint8_t>(1 + report.channelList.size()));
                    start.WriteU8(report.operatingClass);
                    for (auto ch : report.channelList)
                    {
                        start.WriteU8(ch);
                    }
                }
                SerializeVendorSpecific(start, body.vendorSpecific);
            }
            else if constexpr (std::is_same_v<T, FrameRequestBody>)
            {
                start.WriteU8(body.operatingClass);
                start.WriteU8(body.channelNumber);
                start.WriteU16(body.randomizationInterval);
                start.WriteU16(body.measurementDuration);
                start.WriteU8(body.frameRequestType);
                WriteTo(start, body.macAddress);
                SerializeVendorSpecific(start, body.vendorSpecific);
            }
            else if constexpr (std::is_same_v<T, StaStatisticsRequestBody>)
            {
                WriteTo(start, body.peerMacAddress);
                start.WriteU16(body.randomizationInterval);
                start.WriteU16(body.measurementDuration);
                start.WriteU8(body.groupIdentity);
                SerializeVendorSpecific(start, body.vendorSpecific);
            }
            else if constexpr (std::is_same_v<T, LciRequestBody>)
            {
                start.WriteU8(body.locationSubject);
                if (body.azimuthRequest)
                {
                    start.WriteU8(1);
                    start.WriteU8(1);
                    uint8_t field = (body.azimuthRequest->azimuthResolution & 0x0F) |
                                    ((body.azimuthRequest->azimuthType & 0x01) << 4);
                    start.WriteU8(field);
                }
                SerializeVendorSpecific(start, body.vendorSpecific);
            }
            else if constexpr (std::is_same_v<T, TransmitStreamRequestBody>)
            {
                start.WriteU16(body.randomizationInterval);
                start.WriteU16(body.measurementDuration);
                WriteTo(start, body.peerStaAddress);
                start.WriteU8(body.trafficIdentifier);
                start.WriteU8(body.bin0Range);
                SerializeVendorSpecific(start, body.vendorSpecific);
            }
            else if constexpr (std::is_same_v<T, MulticastDiagnosticsRequestBody>)
            {
                start.WriteU16(body.randomizationInterval);
                start.WriteU16(body.measurementDuration);
                WriteTo(start, body.groupMacAddress);
                SerializeVendorSpecific(start, body.vendorSpecific);
            }
            else if constexpr (std::is_same_v<T, LocationCivicRequestBody>)
            {
                start.WriteU8(body.locationSubject);
                start.WriteU8(body.civicLocationType);
                start.WriteU8(body.locationServiceIntervalUnits);
                start.WriteU16(body.locationServiceInterval);
                SerializeVendorSpecific(start, body.vendorSpecific);
            }
            else if constexpr (std::is_same_v<T, LocationIdentifierRequestBody>)
            {
                start.WriteU8(body.locationSubject);
                start.WriteU8(body.locationServiceIntervalUnits);
                start.WriteU16(body.locationServiceInterval);
                SerializeVendorSpecific(start, body.vendorSpecific);
            }
            else if constexpr (std::is_same_v<T, DirectionalChannelQualityRequestBody>)
            {
                start.WriteU8(body.operatingClass);
                start.WriteU8(body.channelNumber);
                start.WriteU8(body.aid);
                start.WriteU8(body.reserved);
                start.WriteU8(body.measurementMethod);
                start.WriteU64(body.measurementStartTime);
                start.WriteU16(body.measurementDuration);
                start.WriteU8(body.numberOfTimeBlocks);
                SerializeVendorSpecific(start, body.vendorSpecific);
            }
            else if constexpr (std::is_same_v<T, DirectionalMeasurementRequestBody>)
            {
                start.WriteU8(body.operatingClass);
                start.WriteU8(body.channelNumber);
                start.WriteU64(body.measurementStartTime);
                start.WriteU16(body.measurementDurationPerDirection);
                start.WriteU8(body.measurementMethodAndAntennaConfiguration);
                SerializeVendorSpecific(start, body.vendorSpecific);
            }
            else if constexpr (std::is_same_v<T, DirectionalStatisticsRequestBody>)
            {
                start.WriteU8(body.operatingClass);
                start.WriteU8(body.channelNumber);
                start.WriteU64(body.measurementStartTime);
                start.WriteU16(body.measurementDurationPerDirection);
                start.WriteU8(body.measurementMethod);
                start.WriteU8(body.directionalStatisticsBitmap);
                SerializeVendorSpecific(start, body.vendorSpecific);
            }
            else if constexpr (std::is_same_v<T, FtmRangeRequestBody>)
            {
                start.WriteU16(body.randomizationInterval);
                start.WriteU8(body.minimumApCount);
                SerializeNeighborReports(start, body.neighborReports);
                SerializeVendorSpecific(start, body.vendorSpecific);
            }
            else if constexpr (std::is_same_v<T, MeasurementPauseRequestBody>)
            {
                start.WriteU16(body.pauseTime);
                SerializeVendorSpecific(start, body.vendorSpecific);
            }
        },
        m_body);
}

uint16_t
MeasurementRequestElement::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    Buffer::Iterator i = start;
    m_measurementToken = i.ReadU8();
    m_measurementRequestMode = i.ReadU8() & 0x1F;
    m_measurementType = i.ReadU8();

    uint16_t bytesRead = 3;

    // Initialize variant for the type, then populate body fields
    SetMeasurementType(m_measurementType);

    switch (m_measurementType)
    {
    case 0:
    case 1:
    case 2: {
        auto& body = std::get<BasicRequestBody>(m_body);
        body.channelNumber = i.ReadU8();
        body.measurementStartTime = i.ReadU64();
        body.measurementDuration = i.ReadU16();
        bytesRead += 11;
        break;
    }
    case 3: {
        auto& body = std::get<ChannelLoadRequestBody>(m_body);
        body.operatingClass = i.ReadU8();
        body.channelNumber = i.ReadU8();
        body.randomizationInterval = i.ReadU16();
        body.measurementDuration = i.ReadU16();
        bytesRead += 6;
        break;
    }
    case 4: {
        auto& body = std::get<NoiseHistogramRequestBody>(m_body);
        body.operatingClass = i.ReadU8();
        body.channelNumber = i.ReadU8();
        body.randomizationInterval = i.ReadU16();
        body.measurementDuration = i.ReadU16();
        bytesRead += 6;
        break;
    }
    case 5: {
        auto& body = std::get<BeaconRequestBody>(m_body);
        body.operatingClass = i.ReadU8();
        body.channelNumber = i.ReadU8();
        body.randomizationInterval = i.ReadU16();
        body.measurementDuration = i.ReadU16();
        body.measurementMode = i.ReadU8();
        ReadFrom(i, body.bssid);
        bytesRead += 13;
        break;
    }
    case 6: {
        auto& body = std::get<FrameRequestBody>(m_body);
        body.operatingClass = i.ReadU8();
        body.channelNumber = i.ReadU8();
        body.randomizationInterval = i.ReadU16();
        body.measurementDuration = i.ReadU16();
        body.frameRequestType = i.ReadU8();
        ReadFrom(i, body.macAddress);
        bytesRead += 13;
        break;
    }
    case 7: {
        auto& body = std::get<StaStatisticsRequestBody>(m_body);
        ReadFrom(i, body.peerMacAddress);
        body.randomizationInterval = i.ReadU16();
        body.measurementDuration = i.ReadU16();
        body.groupIdentity = i.ReadU8();
        bytesRead += 11;
        break;
    }
    case 8: {
        auto& body = std::get<LciRequestBody>(m_body);
        body.locationSubject = i.ReadU8();
        bytesRead += 1;
        break;
    }
    case 9: {
        auto& body = std::get<TransmitStreamRequestBody>(m_body);
        body.randomizationInterval = i.ReadU16();
        body.measurementDuration = i.ReadU16();
        ReadFrom(i, body.peerStaAddress);
        body.trafficIdentifier = i.ReadU8();
        body.bin0Range = i.ReadU8();
        bytesRead += 12;
        break;
    }
    case 10: {
        auto& body = std::get<MulticastDiagnosticsRequestBody>(m_body);
        body.randomizationInterval = i.ReadU16();
        body.measurementDuration = i.ReadU16();
        ReadFrom(i, body.groupMacAddress);
        bytesRead += 10;
        break;
    }
    case 11: {
        auto& body = std::get<LocationCivicRequestBody>(m_body);
        body.locationSubject = i.ReadU8();
        body.civicLocationType = i.ReadU8();
        body.locationServiceIntervalUnits = i.ReadU8();
        body.locationServiceInterval = i.ReadU16();
        bytesRead += 5;
        break;
    }
    case 12: {
        auto& body = std::get<LocationIdentifierRequestBody>(m_body);
        body.locationSubject = i.ReadU8();
        body.locationServiceIntervalUnits = i.ReadU8();
        body.locationServiceInterval = i.ReadU16();
        bytesRead += 4;
        break;
    }
    case 13: {
        auto& body = std::get<DirectionalChannelQualityRequestBody>(m_body);
        body.operatingClass = i.ReadU8();
        body.channelNumber = i.ReadU8();
        body.aid = i.ReadU8();
        i.ReadU8(); // Reserved
        body.measurementMethod = i.ReadU8();
        body.measurementStartTime = i.ReadU64();
        body.measurementDuration = i.ReadU16();
        body.numberOfTimeBlocks = i.ReadU8();
        bytesRead += 16;
        break;
    }
    case 14: {
        auto& body = std::get<DirectionalMeasurementRequestBody>(m_body);
        body.operatingClass = i.ReadU8();
        body.channelNumber = i.ReadU8();
        body.measurementStartTime = i.ReadU64();
        body.measurementDurationPerDirection = i.ReadU16();
        body.measurementMethodAndAntennaConfiguration = i.ReadU8();
        bytesRead += 13;
        break;
    }
    case 15: {
        auto& body = std::get<DirectionalStatisticsRequestBody>(m_body);
        body.operatingClass = i.ReadU8();
        body.channelNumber = i.ReadU8();
        body.measurementStartTime = i.ReadU64();
        body.measurementDurationPerDirection = i.ReadU16();
        body.measurementMethod = i.ReadU8();
        body.directionalStatisticsBitmap = i.ReadU8();
        bytesRead += 14;
        break;
    }
    case 16: {
        auto& body = std::get<FtmRangeRequestBody>(m_body);
        body.randomizationInterval = i.ReadU16();
        body.minimumApCount = i.ReadU8();
        bytesRead += 3;
        break;
    }
    case 255: {
        auto& body = std::get<MeasurementPauseRequestBody>(m_body);
        body.pauseTime = i.ReadU16();
        bytesRead += 2;
        break;
    }
    default:
        break;
    }

    // Parse subelements from remaining bytes
    while (bytesRead < length)
    {
        uint8_t subelemId = i.ReadU8();
        uint8_t subelemLen = i.ReadU8();
        bytesRead += 2;

        bool handled = false;

        if (subelemId == 221)
        {
            std::vector<uint8_t> data(subelemLen);
            for (uint8_t j = 0; j < subelemLen; j++)
            {
                data[j] = i.ReadU8();
            }
            std::visit(
                [&data](auto& body) {
                    using T = std::decay_t<decltype(body)>;
                    if constexpr (!std::is_same_v<T, std::monostate> &&
                                  !std::is_same_v<T, BasicRequestBody>)
                    {
                        body.vendorSpecific = std::move(data);
                    }
                },
                m_body);
            handled = true;
        }
        else if (m_measurementType == 5)
        {
            auto& body = std::get<BeaconRequestBody>(m_body);
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
                body.ssid = ssid;
                handled = true;
                break;
            }
            case 1: { // Beacon Reporting
                uint8_t condition = i.ReadU8();
                uint8_t thresholdOffsetRef = i.ReadU8();
                body.beaconReporting = BeaconReporting{condition, thresholdOffsetRef};
                handled = true;
                break;
            }
            case 2: { // Reporting Detail
                body.reportingDetail = i.ReadU8();
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
                body.apChannelReports.push_back(ApChannelReport{opClass, std::move(channels)});
                handled = true;
                break;
            }
            default:
                break;
            }
        }
        else if (m_measurementType == 8)
        {
            if (subelemId == 1)
            {
                auto& body = std::get<LciRequestBody>(m_body);
                uint8_t field = i.ReadU8();
                body.azimuthRequest = AzimuthRequest{static_cast<uint8_t>(field & 0x0F),
                                                     static_cast<uint8_t>((field >> 4) & 0x01)};
                handled = true;
            }
        }
        else if (m_measurementType == 3)
        {
            if (subelemId == 1)
            {
                auto& body = std::get<ChannelLoadRequestBody>(m_body);
                uint8_t condition = i.ReadU8();
                uint8_t refValue = i.ReadU8();
                body.channelLoadReporting = ChannelLoadReporting{condition, refValue};
                handled = true;
            }
        }
        else if (m_measurementType == 4)
        {
            if (subelemId == 1)
            {
                auto& body = std::get<NoiseHistogramRequestBody>(m_body);
                uint8_t condition = i.ReadU8();
                uint8_t refValue = i.ReadU8();
                body.noiseHistogramReporting = NoiseHistogramReporting{condition, refValue};
                handled = true;
            }
        }
        else if (m_measurementType == 16)
        {
            if (subelemId == 52)
            {
                auto& body = std::get<FtmRangeRequestBody>(m_body);
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
                body.neighborReports.push_back(nre);
                handled = true;
            }
        }

        if (!handled)
        {
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
