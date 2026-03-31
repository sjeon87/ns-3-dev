/*
 * Copyright (c) 2026
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
MeasurementRequestElement::SetMeasurementType(MeasurementType type)
{
    m_measurementType = type;
    switch (m_measurementType)
    {
    case MeasurementType::BASIC:
    case MeasurementType::CCA:
    case MeasurementType::RPI_HISTOGRAM:
        m_body = BasicRequestBody{};
        break;
    case MeasurementType::CHANNEL_LOAD:
        m_body = ChannelLoadRequestBody{};
        break;
    case MeasurementType::NOISE_HISTOGRAM:
        m_body = NoiseHistogramRequestBody{};
        break;
    case MeasurementType::BEACON:
        m_body = BeaconRequestBody{};
        break;
    case MeasurementType::FRAME:
        m_body = FrameRequestBody{};
        break;
    case MeasurementType::STA_STATISTICS:
        m_body = StaStatisticsRequestBody{};
        break;
    case MeasurementType::LCI:
        m_body = LciRequestBody{};
        break;
    case MeasurementType::TRANSMIT_STREAM:
        m_body = TransmitStreamRequestBody{};
        break;
    case MeasurementType::MULTICAST_DIAGNOSTICS:
        m_body = MulticastDiagnosticsRequestBody{};
        break;
    case MeasurementType::LOCATION_CIVIC:
        m_body = LocationCivicRequestBody{};
        break;
    case MeasurementType::LOCATION_IDENTIFIER:
        m_body = LocationIdentifierRequestBody{};
        break;
    case MeasurementType::DIRECTIONAL_CHANNEL_QUALITY:
        m_body = DirectionalChannelQualityRequestBody{};
        break;
    case MeasurementType::DIRECTIONAL_MEASUREMENT:
        m_body = DirectionalMeasurementRequestBody{};
        break;
    case MeasurementType::DIRECTIONAL_STATISTICS:
        m_body = DirectionalStatisticsRequestBody{};
        break;
    case MeasurementType::FTM_RANGE:
        m_body = FtmRangeRequestBody{};
        break;
    case MeasurementType::MEASUREMENT_PAUSE:
        m_body = MeasurementPauseRequestBody{};
        break;
    default:
        m_body = std::monostate{};
        break;
    }
}

MeasurementRequestElement::MeasurementType
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

// --- Subelement serialization helpers ---

/** @brief Vendor Specific subelement ID, shared across all request body types */
static constexpr uint8_t VENDOR_SPECIFIC_SUBELEMENT_ID = 221;

namespace
{

/**
 * @param vs optional vendor specific data
 * @return serialized size of vendor specific subelement, or 0 if absent
 */
uint16_t
VendorSpecificSize(const std::optional<std::vector<uint8_t>>& vs)
{
    return vs ? static_cast<uint16_t>(2 + vs->size()) : 0;
}

/**
 * @param start buffer iterator to write to
 * @param vs optional vendor specific data to serialize
 */
void
SerializeVendorSpecific(Buffer::Iterator& start, const std::optional<std::vector<uint8_t>>& vs)
{
    if (vs)
    {
        start.WriteU8(VENDOR_SPECIFIC_SUBELEMENT_ID);
        start.WriteU8(static_cast<uint8_t>(vs->size()));
        for (auto byte : *vs)
        {
            start.WriteU8(byte);
        }
    }
}

/**
 * @param start buffer iterator to write to
 * @param ssid optional SSID to serialize as subelement
 */
void
SerializeSsidSubelement(Buffer::Iterator& start, const std::optional<Ssid>& ssid)
{
    if (ssid)
    {
        uint32_t ssidSerSize = ssid->GetSerializedSize();
        auto ssidInfoLen = static_cast<uint16_t>(ssidSerSize - 2);
        start.WriteU8(
            static_cast<uint8_t>(MeasurementRequestElement::BeaconRequestBody::SubelementId::SSID));
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

/**
 * @param start buffer iterator to write to
 * @param neighborReports neighbor report elements to serialize as subelements
 */
void
SerializeNeighborReports(Buffer::Iterator& start,
                         const std::vector<NeighborReportElement>& neighborReports)
{
    for (const auto& nre : neighborReports)
    {
        uint32_t nreSerSize = nre.GetSerializedSize();
        auto nreInfoSize = static_cast<uint16_t>(nreSerSize - 2);
        Buffer nreBuf;
        nreBuf.AddAtStart(nreSerSize);
        nre.Serialize(nreBuf.Begin());
        start.WriteU8(static_cast<uint8_t>(
            MeasurementRequestElement::FtmRangeRequestBody::SubelementId::NEIGHBOR_REPORT));
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
                if (body.GetChannelLoadReporting())
                {
                    size += 4;
                }
                size += VendorSpecificSize(body.GetVendorSpecific());
            }
            else if constexpr (std::is_same_v<T, NoiseHistogramRequestBody>)
            {
                size += 6;
                if (body.GetNoiseHistogramReporting())
                {
                    size += 4;
                }
                size += VendorSpecificSize(body.GetVendorSpecific());
            }
            else if constexpr (std::is_same_v<T, BeaconRequestBody>)
            {
                size += 13; // OpClass(1)+Ch(1)+Rand(2)+Dur(2)+Mode(1)+BSSID(6)
                if (const auto& ssid = body.GetSsid())
                {
                    size += 2 + (ssid->GetSerializedSize() - 2);
                }
                if (body.GetBeaconReporting())
                {
                    size += 4;
                }
                if (body.GetReportingDetail())
                {
                    size += 3;
                }
                for (const auto& report : body.GetApChannelReports())
                {
                    size += static_cast<uint16_t>(2 + 1 + report.channelList.size());
                }
                size += VendorSpecificSize(body.GetVendorSpecific());
            }
            else if constexpr (std::is_same_v<T, FrameRequestBody>)
            {
                // OpClass(1)+Ch(1)+Rand(2)+Dur(2)+FReqType(1)+MAC(6)
                size += 13 + VendorSpecificSize(body.GetVendorSpecific());
            }
            else if constexpr (std::is_same_v<T, StaStatisticsRequestBody>)
            {
                size += 11; // PeerMAC(6)+Rand(2)+Dur(2)+GroupID(1)
                size += VendorSpecificSize(body.GetVendorSpecific());
            }
            else if constexpr (std::is_same_v<T, LciRequestBody>)
            {
                size += 1; // LocationSubject(1)
                if (body.GetAzimuthRequest())
                {
                    size += 3;
                }
                size += VendorSpecificSize(body.GetVendorSpecific());
            }
            else if constexpr (std::is_same_v<T, TransmitStreamRequestBody>)
            {
                size += 12; // Rand(2)+Dur(2)+PeerSTA(6)+TID(1)+Bin0Range(1)
                size += VendorSpecificSize(body.GetVendorSpecific());
            }
            else if constexpr (std::is_same_v<T, MulticastDiagnosticsRequestBody>)
            {
                size += 10; // Rand(2)+Dur(2)+GroupMAC(6)
                size += VendorSpecificSize(body.GetVendorSpecific());
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
    start.WriteU8(static_cast<uint8_t>(m_measurementType));

    std::visit(
        [&start](const auto& body) {
            using T = std::decay_t<decltype(body)>;
            if constexpr (std::is_same_v<T, std::monostate>)
            {
                // no body
            }
            else if constexpr (std::is_same_v<T, BasicRequestBody>)
            {
                start.WriteU8(body.GetChannelNumber());
                start.WriteU64(body.GetMeasurementStartTime());
                start.WriteU16(body.GetMeasurementDuration());
            }
            else if constexpr (std::is_same_v<T, ChannelLoadRequestBody>)
            {
                start.WriteU8(body.GetOperatingClass());
                start.WriteU8(body.GetChannelNumber());
                start.WriteU16(body.GetRandomizationInterval());
                start.WriteU16(body.GetMeasurementDuration());
                if (const auto& clr = body.GetChannelLoadReporting())
                {
                    start.WriteU8(static_cast<uint8_t>(
                        ChannelLoadRequestBody::SubelementId::CHANNEL_LOAD_REPORTING));
                    start.WriteU8(clr->GetSerializedSize());
                    start.WriteU8(clr->reportingCondition);
                    start.WriteU8(clr->channelLoadReferenceValue);
                }
                SerializeVendorSpecific(start, body.GetVendorSpecific());
            }
            else if constexpr (std::is_same_v<T, NoiseHistogramRequestBody>)
            {
                start.WriteU8(body.GetOperatingClass());
                start.WriteU8(body.GetChannelNumber());
                start.WriteU16(body.GetRandomizationInterval());
                start.WriteU16(body.GetMeasurementDuration());
                if (const auto& nhr = body.GetNoiseHistogramReporting())
                {
                    start.WriteU8(static_cast<uint8_t>(
                        NoiseHistogramRequestBody::SubelementId::NOISE_HISTOGRAM_REPORTING));
                    start.WriteU8(nhr->GetSerializedSize());
                    start.WriteU8(nhr->reportingCondition);
                    start.WriteU8(nhr->anpiReferenceValue);
                }
                SerializeVendorSpecific(start, body.GetVendorSpecific());
            }
            else if constexpr (std::is_same_v<T, BeaconRequestBody>)
            {
                start.WriteU8(body.GetOperatingClass());
                start.WriteU8(body.GetChannelNumber());
                start.WriteU16(body.GetRandomizationInterval());
                start.WriteU16(body.GetMeasurementDuration());
                start.WriteU8(body.GetMeasurementMode());
                WriteTo(start, body.GetBssid());
                SerializeSsidSubelement(start, body.GetSsid());
                if (const auto& br = body.GetBeaconReporting())
                {
                    start.WriteU8(
                        static_cast<uint8_t>(BeaconRequestBody::SubelementId::BEACON_REPORTING));
                    start.WriteU8(br->GetSerializedSize());
                    start.WriteU8(br->reportingCondition);
                    start.WriteU8(br->thresholdOffsetReference);
                }
                if (body.GetReportingDetail())
                {
                    start.WriteU8(
                        static_cast<uint8_t>(BeaconRequestBody::SubelementId::REPORTING_DETAIL));
                    start.WriteU8(1); // length
                    start.WriteU8(*body.GetReportingDetail());
                }
                for (const auto& report : body.GetApChannelReports())
                {
                    start.WriteU8(
                        static_cast<uint8_t>(BeaconRequestBody::SubelementId::AP_CHANNEL_REPORT));
                    start.WriteU8(static_cast<uint8_t>(1 + report.channelList.size()));
                    start.WriteU8(report.operatingClass);
                    for (auto ch : report.channelList)
                    {
                        start.WriteU8(ch);
                    }
                }
                SerializeVendorSpecific(start, body.GetVendorSpecific());
            }
            else if constexpr (std::is_same_v<T, FrameRequestBody>)
            {
                start.WriteU8(body.GetOperatingClass());
                start.WriteU8(body.GetChannelNumber());
                start.WriteU16(body.GetRandomizationInterval());
                start.WriteU16(body.GetMeasurementDuration());
                start.WriteU8(body.GetFrameRequestType());
                WriteTo(start, body.GetMacAddress());
                SerializeVendorSpecific(start, body.GetVendorSpecific());
            }
            else if constexpr (std::is_same_v<T, StaStatisticsRequestBody>)
            {
                WriteTo(start, body.GetPeerMacAddress());
                start.WriteU16(body.GetRandomizationInterval());
                start.WriteU16(body.GetMeasurementDuration());
                start.WriteU8(body.GetGroupIdentity());
                SerializeVendorSpecific(start, body.GetVendorSpecific());
            }
            else if constexpr (std::is_same_v<T, LciRequestBody>)
            {
                start.WriteU8(body.GetLocationSubject());
                if (body.GetAzimuthRequest())
                {
                    start.WriteU8(
                        static_cast<uint8_t>(LciRequestBody::SubelementId::AZIMUTH_REQUEST));
                    start.WriteU8(1); // length
                    auto azimuth = body.GetAzimuthRequest();
                    uint8_t field =
                        (azimuth->azimuthResolution & 0x0F) | ((azimuth->azimuthType & 0x01) << 4);
                    start.WriteU8(field);
                }
                SerializeVendorSpecific(start, body.GetVendorSpecific());
            }
            else if constexpr (std::is_same_v<T, TransmitStreamRequestBody>)
            {
                start.WriteU16(body.GetRandomizationInterval());
                start.WriteU16(body.GetMeasurementDuration());
                WriteTo(start, body.GetPeerStaAddress());
                start.WriteU8(body.GetTrafficIdentifier());
                start.WriteU8(body.GetBin0Range());
                SerializeVendorSpecific(start, body.GetVendorSpecific());
            }
            else if constexpr (std::is_same_v<T, MulticastDiagnosticsRequestBody>)
            {
                start.WriteU16(body.GetRandomizationInterval());
                start.WriteU16(body.GetMeasurementDuration());
                WriteTo(start, body.GetGroupMacAddress());
                SerializeVendorSpecific(start, body.GetVendorSpecific());
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
    m_measurementType = static_cast<MeasurementType>(i.ReadU8());

    uint16_t bytesRead = 3;

    // Initialize variant for the type, then populate body fields
    SetMeasurementType(m_measurementType);

    switch (m_measurementType)
    {
    case MeasurementType::BASIC:
    case MeasurementType::CCA:
    case MeasurementType::RPI_HISTOGRAM: {
        auto& body = std::get<BasicRequestBody>(m_body);
        body.SetChannelNumber(i.ReadU8());
        body.SetMeasurementStartTime(i.ReadU64());
        body.SetMeasurementDuration(i.ReadU16());
        bytesRead += 11;
        break;
    }
    case MeasurementType::CHANNEL_LOAD: {
        auto& body = std::get<ChannelLoadRequestBody>(m_body);
        body.SetOperatingClass(i.ReadU8());
        body.SetChannelNumber(i.ReadU8());
        body.SetRandomizationInterval(i.ReadU16());
        body.SetMeasurementDuration(i.ReadU16());
        bytesRead += 6;
        break;
    }
    case MeasurementType::NOISE_HISTOGRAM: {
        auto& body = std::get<NoiseHistogramRequestBody>(m_body);
        body.SetOperatingClass(i.ReadU8());
        body.SetChannelNumber(i.ReadU8());
        body.SetRandomizationInterval(i.ReadU16());
        body.SetMeasurementDuration(i.ReadU16());
        bytesRead += 6;
        break;
    }
    case MeasurementType::BEACON: {
        auto& body = std::get<BeaconRequestBody>(m_body);
        body.SetOperatingClass(i.ReadU8());
        body.SetChannelNumber(i.ReadU8());
        body.SetRandomizationInterval(i.ReadU16());
        body.SetMeasurementDuration(i.ReadU16());
        body.SetMeasurementMode(i.ReadU8());
        Mac48Address bssid;
        ReadFrom(i, bssid);
        body.SetBssid(bssid);
        bytesRead += 13;
        break;
    }
    case MeasurementType::FRAME: {
        auto& body = std::get<FrameRequestBody>(m_body);
        body.SetOperatingClass(i.ReadU8());
        body.SetChannelNumber(i.ReadU8());
        body.SetRandomizationInterval(i.ReadU16());
        body.SetMeasurementDuration(i.ReadU16());
        body.SetFrameRequestType(i.ReadU8());
        Mac48Address macAddr;
        ReadFrom(i, macAddr);
        body.SetMacAddress(macAddr);
        bytesRead += 13;
        break;
    }
    case MeasurementType::STA_STATISTICS: {
        auto& body = std::get<StaStatisticsRequestBody>(m_body);
        Mac48Address peerMac;
        ReadFrom(i, peerMac);
        body.SetPeerMacAddress(peerMac);
        body.SetRandomizationInterval(i.ReadU16());
        body.SetMeasurementDuration(i.ReadU16());
        body.SetGroupIdentity(i.ReadU8());
        bytesRead += 11;
        break;
    }
    case MeasurementType::LCI: {
        auto& body = std::get<LciRequestBody>(m_body);
        body.SetLocationSubject(i.ReadU8());
        bytesRead += 1;
        break;
    }
    case MeasurementType::TRANSMIT_STREAM: {
        auto& body = std::get<TransmitStreamRequestBody>(m_body);
        body.SetRandomizationInterval(i.ReadU16());
        body.SetMeasurementDuration(i.ReadU16());
        Mac48Address peerStaAddr;
        ReadFrom(i, peerStaAddr);
        body.SetPeerStaAddress(peerStaAddr);
        body.SetTrafficIdentifier(i.ReadU8());
        body.SetBin0Range(i.ReadU8());
        bytesRead += 12;
        break;
    }
    case MeasurementType::MULTICAST_DIAGNOSTICS: {
        auto& body = std::get<MulticastDiagnosticsRequestBody>(m_body);
        body.SetRandomizationInterval(i.ReadU16());
        body.SetMeasurementDuration(i.ReadU16());
        Mac48Address groupMac;
        ReadFrom(i, groupMac);
        body.SetGroupMacAddress(groupMac);
        bytesRead += 10;
        break;
    }
    case MeasurementType::LOCATION_CIVIC: {
        auto& body = std::get<LocationCivicRequestBody>(m_body);
        body.locationSubject = i.ReadU8();
        body.civicLocationType = i.ReadU8();
        body.locationServiceIntervalUnits = i.ReadU8();
        body.locationServiceInterval = i.ReadU16();
        bytesRead += 5;
        break;
    }
    case MeasurementType::LOCATION_IDENTIFIER: {
        auto& body = std::get<LocationIdentifierRequestBody>(m_body);
        body.locationSubject = i.ReadU8();
        body.locationServiceIntervalUnits = i.ReadU8();
        body.locationServiceInterval = i.ReadU16();
        bytesRead += 4;
        break;
    }
    case MeasurementType::DIRECTIONAL_CHANNEL_QUALITY: {
        auto& body = std::get<DirectionalChannelQualityRequestBody>(m_body);
        body.operatingClass = i.ReadU8();
        body.channelNumber = i.ReadU8();
        body.aid = i.ReadU8();
        body.reserved = i.ReadU8();
        body.measurementMethod = i.ReadU8();
        body.measurementStartTime = i.ReadU64();
        body.measurementDuration = i.ReadU16();
        body.numberOfTimeBlocks = i.ReadU8();
        bytesRead += 16;
        break;
    }
    case MeasurementType::DIRECTIONAL_MEASUREMENT: {
        auto& body = std::get<DirectionalMeasurementRequestBody>(m_body);
        body.operatingClass = i.ReadU8();
        body.channelNumber = i.ReadU8();
        body.measurementStartTime = i.ReadU64();
        body.measurementDurationPerDirection = i.ReadU16();
        body.measurementMethodAndAntennaConfiguration = i.ReadU8();
        bytesRead += 13;
        break;
    }
    case MeasurementType::DIRECTIONAL_STATISTICS: {
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
    case MeasurementType::FTM_RANGE: {
        auto& body = std::get<FtmRangeRequestBody>(m_body);
        body.randomizationInterval = i.ReadU16();
        body.minimumApCount = i.ReadU8();
        bytesRead += 3;
        break;
    }
    case MeasurementType::MEASUREMENT_PAUSE: {
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

        if (subelemId == VENDOR_SPECIFIC_SUBELEMENT_ID)
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
                        if constexpr (std::is_same_v<T, ChannelLoadRequestBody> ||
                                      std::is_same_v<T, NoiseHistogramRequestBody> ||
                                      std::is_same_v<T, BeaconRequestBody> ||
                                      std::is_same_v<T, FrameRequestBody> ||
                                      std::is_same_v<T, StaStatisticsRequestBody> ||
                                      std::is_same_v<T, MulticastDiagnosticsRequestBody> ||
                                      std::is_same_v<T, LciRequestBody> ||
                                      std::is_same_v<T, TransmitStreamRequestBody>)
                        {
                            body.SetVendorSpecific(std::move(data));
                        }
                        else
                        {
                            body.vendorSpecific = std::move(data);
                        }
                    }
                },
                m_body);
            handled = true;
        }
        else if (m_measurementType == MeasurementType::BEACON)
        {
            auto& body = std::get<BeaconRequestBody>(m_body);
            switch (subelemId)
            {
            case static_cast<uint8_t>(BeaconRequestBody::SubelementId::SSID): {
                Buffer ssidBuf;
                ssidBuf.AddAtStart(2 + subelemLen);
                Buffer::Iterator ssidIter = ssidBuf.Begin();
                ssidIter.WriteU8(IE_SSID);
                ssidIter.WriteU8(subelemLen);
                for (uint8_t j = 0; j < subelemLen; j++)
                {
                    ssidIter.WriteU8(i.ReadU8());
                }
                Ssid ssid;
                ssid.Deserialize(ssidBuf.Begin());
                body.SetSsid(ssid);
                handled = true;
                break;
            }
            case static_cast<uint8_t>(BeaconRequestBody::SubelementId::BEACON_REPORTING): {
                uint8_t condition = i.ReadU8();
                uint8_t thresholdOffsetRef = i.ReadU8();
                body.SetBeaconReporting(BeaconReporting{condition, thresholdOffsetRef});
                handled = true;
                break;
            }
            case static_cast<uint8_t>(BeaconRequestBody::SubelementId::REPORTING_DETAIL): {
                body.SetReportingDetail(i.ReadU8());
                handled = true;
                break;
            }
            case static_cast<uint8_t>(BeaconRequestBody::SubelementId::AP_CHANNEL_REPORT): {
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
                body.AddApChannelReport(ApChannelReport{opClass, std::move(channels)});
                handled = true;
                break;
            }
            default:
                break;
            }
        }
        else if (m_measurementType == MeasurementType::LCI)
        {
            if (subelemId == static_cast<uint8_t>(LciRequestBody::SubelementId::AZIMUTH_REQUEST))
            {
                auto& body = std::get<LciRequestBody>(m_body);
                uint8_t field = i.ReadU8();
                body.SetAzimuthRequest(AzimuthRequest{static_cast<uint8_t>(field & 0x0F),
                                                      static_cast<uint8_t>((field >> 4) & 0x01)});
                handled = true;
            }
        }
        else if (m_measurementType == MeasurementType::CHANNEL_LOAD)
        {
            if (subelemId ==
                static_cast<uint8_t>(ChannelLoadRequestBody::SubelementId::CHANNEL_LOAD_REPORTING))
            {
                auto& body = std::get<ChannelLoadRequestBody>(m_body);
                uint8_t condition = i.ReadU8();
                uint8_t refValue = i.ReadU8();
                body.SetChannelLoadReporting(ChannelLoadReporting{condition, refValue});
                handled = true;
            }
        }
        else if (m_measurementType == MeasurementType::NOISE_HISTOGRAM)
        {
            if (subelemId ==
                static_cast<uint8_t>(
                    NoiseHistogramRequestBody::SubelementId::NOISE_HISTOGRAM_REPORTING))
            {
                auto& body = std::get<NoiseHistogramRequestBody>(m_body);
                uint8_t condition = i.ReadU8();
                uint8_t refValue = i.ReadU8();
                body.SetNoiseHistogramReporting(NoiseHistogramReporting{condition, refValue});
                handled = true;
            }
        }
        else if (m_measurementType == MeasurementType::FTM_RANGE)
        {
            if (subelemId ==
                static_cast<uint8_t>(FtmRangeRequestBody::SubelementId::NEIGHBOR_REPORT))
            {
                auto& body = std::get<FtmRangeRequestBody>(m_body);
                Buffer nreBuf;
                nreBuf.AddAtStart(2 + subelemLen);
                Buffer::Iterator nreIter = nreBuf.Begin();
                nreIter.WriteU8(IE_NEIGHBOR_REPORT);
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
       << +m_measurementRequestMode << std::dec
       << ", Type=" << +static_cast<uint8_t>(m_measurementType) << "]";
}

} // namespace ns3
