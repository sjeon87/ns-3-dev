/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Devansh Gupta <devanshg308@gmail.com>
 */

#ifndef MEASUREMENT_REQUEST_ELEMENT_H
#define MEASUREMENT_REQUEST_ELEMENT_H

#include "neighbor-report-element.h"
#include "ssid.h"
#include "wifi-information-element.h"

#include "ns3/mac48-address.h"

#include <optional>
#include <variant>
#include <vector>

namespace ns3
{

/**
 * @brief The Measurement Request element
 * @ingroup wifi
 *
 * IEEE 802.11-2024 Section 9.4.2.19.
 *
 * Contains a measurement token, mode bitmap, measurement type, and
 * a type-specific variable-length body with optional subelements.
 */
class MeasurementRequestElement : public WifiInformationElement
{
  public:
    MeasurementRequestElement();

    WifiInformationElementId ElementId() const override;
    void Print(std::ostream& os) const override;

    /**
     * @brief Channel Load Reporting subelement data (Table 9-137, Figure 9-247)
     */
    struct ChannelLoadReporting
    {
        uint8_t reportingCondition;        //!< Reporting Condition (1 octet, Table 9-138)
        uint8_t channelLoadReferenceValue; //!< Channel Load Reference Value (1 octet)
    };

    /**
     * @brief Noise Histogram Reporting subelement data (Table 9-139, Figure 9-249)
     */
    struct NoiseHistogramReporting
    {
        uint8_t reportingCondition; //!< Reporting Condition (1 octet, Table 9-140)
        uint8_t anpiReferenceValue; //!< ANPI Reference Value (1 octet)
    };

    /**
     * @brief Beacon Reporting subelement data (Table 9-142, Figure 9-251)
     */
    struct BeaconReporting
    {
        uint8_t reportingCondition;       //!< Reporting Condition (1 octet, Table 9-143)
        uint8_t thresholdOffsetReference; //!< Threshold/Offset Reference (1 octet)
    };

    /**
     * @brief Azimuth Request subelement data
     */
    struct AzimuthRequest
    {
        uint8_t azimuthResolution; //!< Azimuth Resolution (4 bits)
        uint8_t azimuthType;       //!< Azimuth Type (4 bits)
    };

    /**
     * @brief AP Channel Report subelement data
     */
    struct ApChannelReport
    {
        uint8_t operatingClass;           //!< Operating Class (1 octet)
        std::vector<uint8_t> channelList; //!< Channel list
    };

    /**
     * @brief Measurement Type values (Table 9-136)
     */
    enum class MeasurementType : uint8_t
    {
        BASIC = 0,
        CCA = 1,
        RPI_HISTOGRAM = 2,
        CHANNEL_LOAD = 3,
        NOISE_HISTOGRAM = 4,
        BEACON = 5,
        FRAME = 6,
        STA_STATISTICS = 7,
        LCI = 8,
        TRANSMIT_STREAM = 9,
        MULTICAST_DIAGNOSTICS = 10,
        LOCATION_CIVIC = 11,
        LOCATION_IDENTIFIER = 12,
        DIRECTIONAL_CHANNEL_QUALITY = 13,
        DIRECTIONAL_MEASUREMENT = 14,
        DIRECTIONAL_STATISTICS = 15,
        FTM_RANGE = 16,
        NEIGHBORING_DMG_APS = 17,
        MEASUREMENT_PAUSE = 255,
    };

    /**
     * @brief Request body for Basic/CCA/RPI Histogram measurements (Figures 9-243, 9-244, 9-245)
     *
     * Shared by types 0 (Basic), 1 (CCA), and 2 (RPI Histogram) which have identical layouts.
     */
    struct BasicRequestBody
    {
        uint8_t channelNumber{0};         //!< Channel Number (1 octet)
        uint64_t measurementStartTime{0}; //!< Measurement Start Time (8 octets)
        uint16_t measurementDuration{0};  //!< Measurement Duration (2 octets)
    };

    /**
     * @brief Request body for Channel Load measurement (Figure 9-246)
     */
    struct ChannelLoadRequestBody
    {
        /**
         * @brief Subelement IDs for Channel Load request (Table 9-137)
         */
        enum SubelementId : uint8_t
        {
            RESERVED = 0,
            CHANNEL_LOAD_REPORTING = 1,
            WIDE_BANDWIDTH_CHANNEL_SWITCH = 163,
            VENDOR_SPECIFIC = 221,
        };

        uint8_t operatingClass{0};         //!< Operating Class (1 octet)
        uint8_t channelNumber{0};          //!< Channel Number (1 octet)
        uint16_t randomizationInterval{0}; //!< Randomization Interval (2 octets)
        uint16_t measurementDuration{0};   //!< Measurement Duration (2 octets)

        std::optional<ChannelLoadReporting> channelLoadReporting; //!< Reporting subelement (ID 1)
        std::optional<std::vector<uint8_t>> vendorSpecific; //!< Vendor Specific subelement (ID 221)
    };

    /**
     * @brief Request body for Noise Histogram measurement (Figure 9-248)
     */
    struct NoiseHistogramRequestBody
    {
        /**
         * @brief Subelement IDs for Noise Histogram request (Table 9-139)
         */
        enum SubelementId : uint8_t
        {
            RESERVED = 0,
            NOISE_HISTOGRAM_REPORTING = 1,
            WIDE_BANDWIDTH_CHANNEL_SWITCH = 163,
            VENDOR_SPECIFIC = 221,
        };

        uint8_t operatingClass{0};         //!< Operating Class (1 octet)
        uint8_t channelNumber{0};          //!< Channel Number (1 octet)
        uint16_t randomizationInterval{0}; //!< Randomization Interval (2 octets)
        uint16_t measurementDuration{0};   //!< Measurement Duration (2 octets)

        std::optional<NoiseHistogramReporting>
            noiseHistogramReporting;                        //!< Reporting subelement (ID 1)
        std::optional<std::vector<uint8_t>> vendorSpecific; //!< Vendor Specific subelement (ID 221)
    };

    /**
     * @brief Request body for Beacon measurement (Figure 9-250)
     */
    struct BeaconRequestBody
    {
        /**
         * @brief Subelement IDs for Beacon request (Table 9-142)
         */
        enum SubelementId : uint8_t
        {
            SSID = 0,
            BEACON_REPORTING = 1,
            REPORTING_DETAIL = 2,
            REQUEST = 10,
            EXTENDED_REQUEST = 11,
            AP_CHANNEL_REPORT = 51,
            WIDE_BANDWIDTH_CHANNEL_SWITCH = 163,
            LAST_BEACON_REPORT_INDICATION_REQUEST = 164,
            VENDOR_SPECIFIC = 221,
        };

        uint8_t operatingClass{0};         //!< Operating Class (1 octet)
        uint8_t channelNumber{0};          //!< Channel Number (1 octet)
        uint16_t randomizationInterval{0}; //!< Randomization Interval (2 octets)
        uint16_t measurementDuration{0};   //!< Measurement Duration (2 octets)
        uint8_t measurementMode{0};        //!< Measurement Mode (1 octet)
        Mac48Address bssid;                //!< BSSID (6 octets)

        std::optional<Ssid> ssid;                           //!< SSID subelement (ID 0)
        std::optional<BeaconReporting> beaconReporting;     //!< Reporting subelement (ID 1)
        std::optional<uint8_t> reportingDetail;             //!< Reporting Detail subelement (ID 2)
        std::vector<ApChannelReport> apChannelReports;      //!< AP Channel Reports (ID 51)
        std::optional<std::vector<uint8_t>> vendorSpecific; //!< Vendor Specific subelement (ID 221)
    };

    /**
     * @brief Request body for Frame measurement (Figure 9-252)
     */
    struct FrameRequestBody
    {
        /**
         * @brief Subelement IDs for Frame request (Table 9-145)
         */
        enum SubelementId : uint8_t
        {
            WIDE_BANDWIDTH_CHANNEL_SWITCH = 163,
            VENDOR_SPECIFIC = 221,
        };

        uint8_t operatingClass{0};         //!< Operating Class (1 octet)
        uint8_t channelNumber{0};          //!< Channel Number (1 octet)
        uint16_t randomizationInterval{0}; //!< Randomization Interval (2 octets)
        uint16_t measurementDuration{0};   //!< Measurement Duration (2 octets)
        uint8_t frameRequestType{0};       //!< Frame Request Type (1 octet)
        Mac48Address macAddress;           //!< MAC Address (6 octets)

        std::optional<std::vector<uint8_t>> vendorSpecific; //!< Vendor Specific subelement (ID 221)
    };

    /**
     * @brief Request body for STA Statistics measurement (Figure 9-253)
     */
    struct StaStatisticsRequestBody
    {
        /**
         * @brief Subelement IDs for STA Statistics request (Table 9-147)
         */
        enum SubelementId : uint8_t
        {
            TRIGGERED_REPORTING = 1,
            VENDOR_SPECIFIC = 221,
        };

        Mac48Address peerMacAddress;       //!< Peer MAC Address (6 octets)
        uint16_t randomizationInterval{0}; //!< Randomization Interval (2 octets)
        uint16_t measurementDuration{0};   //!< Measurement Duration (2 octets)
        uint8_t groupIdentity{0};          //!< Group Identity (1 octet)

        std::optional<std::vector<uint8_t>> vendorSpecific; //!< Vendor Specific subelement (ID 221)
    };

    /**
     * @brief Request body for LCI measurement (Figure 9-260)
     */
    struct LciRequestBody
    {
        /**
         * @brief Subelement IDs for LCI request (Table 9-149)
         */
        enum SubelementId : uint8_t
        {
            AZIMUTH_REQUEST = 1,
            ORIGINATOR_REQUESTING_STA_MAC_ADDRESS = 2,
            TARGET_MAC_ADDRESS = 3,
            MAXIMUM_AGE = 4,
            VENDOR_SPECIFIC = 221,
        };

        uint8_t locationSubject{0}; //!< Location Subject (1 octet)

        std::optional<AzimuthRequest> azimuthRequest;       //!< Azimuth Request subelement (ID 1)
        std::optional<std::vector<uint8_t>> vendorSpecific; //!< Vendor Specific subelement (ID 221)
    };

    /**
     * @brief Request body for Transmit Stream measurement (Figure 9-266)
     */
    struct TransmitStreamRequestBody
    {
        /**
         * @brief Subelement IDs for Transmit Stream request (Table 9-150)
         */
        enum SubelementId : uint8_t
        {
            TRIGGERED_REPORTING = 1,
            VENDOR_SPECIFIC = 221,
        };

        uint16_t randomizationInterval{0}; //!< Randomization Interval (2 octets)
        uint16_t measurementDuration{0};   //!< Measurement Duration (2 octets)
        Mac48Address peerStaAddress;       //!< Peer STA Address (6 octets)
        uint8_t trafficIdentifier{0};      //!< Traffic Identifier (1 octet)
        uint8_t bin0Range{0};              //!< Bin 0 Range (1 octet)

        std::optional<std::vector<uint8_t>> vendorSpecific; //!< Vendor Specific subelement (ID 221)
    };

    /**
     * @brief Request body for Multicast Diagnostics measurement (Figure 9-273)
     */
    struct MulticastDiagnosticsRequestBody
    {
        /**
         * @brief Subelement IDs for Multicast Diagnostics request (Table 9-153)
         */
        enum SubelementId : uint8_t
        {
            MULTICAST_TRIGGERED_REPORTING = 1,
            VENDOR_SPECIFIC = 221,
        };

        uint16_t randomizationInterval{0}; //!< Randomization Interval (2 octets)
        uint16_t measurementDuration{0};   //!< Measurement Duration (2 octets)
        Mac48Address groupMacAddress;      //!< Group MAC Address (6 octets)

        std::optional<std::vector<uint8_t>> vendorSpecific; //!< Vendor Specific subelement (ID 221)
    };

    /**
     * @brief Request body for Location Civic measurement (Figure 9-276)
     */
    struct LocationCivicRequestBody
    {
        /**
         * @brief Subelement IDs for Location Civic request (Table 9-156)
         */
        enum SubelementId : uint8_t
        {
            ORIGINATOR_REQUESTING_STA_MAC_ADDRESS = 1,
            TARGET_MAC_ADDRESS = 2,
            VENDOR_SPECIFIC = 221,
        };

        uint8_t locationSubject{0};              //!< Location Subject (1 octet)
        uint8_t civicLocationType{0};            //!< Civic Location Type (1 octet)
        uint8_t locationServiceIntervalUnits{0}; //!< Location Service Interval Units (1 octet)
        uint16_t locationServiceInterval{0};     //!< Location Service Interval (2 octets)

        std::optional<std::vector<uint8_t>> vendorSpecific; //!< Vendor Specific subelement (ID 221)
    };

    /**
     * @brief Request body for Location Identifier measurement (Figure 9-277)
     */
    struct LocationIdentifierRequestBody
    {
        /**
         * @brief Subelement IDs for Location Identifier request (Table 9-157)
         */
        enum SubelementId : uint8_t
        {
            ORIGINATOR_REQUESTING_STA_MAC_ADDRESS = 1,
            TARGET_MAC_ADDRESS = 2,
            VENDOR_SPECIFIC = 221,
        };

        uint8_t locationSubject{0};              //!< Location Subject (1 octet)
        uint8_t locationServiceIntervalUnits{0}; //!< Location Service Interval Units (1 octet)
        uint16_t locationServiceInterval{0};     //!< Location Service Interval (2 octets)

        std::optional<std::vector<uint8_t>> vendorSpecific; //!< Vendor Specific subelement (ID 221)
    };

    /**
     * @brief Request body for Directional Channel Quality measurement (Figure 9-278)
     */
    struct DirectionalChannelQualityRequestBody
    {
        /**
         * @brief Subelement IDs for Directional Channel Quality request (Table 9-158)
         */
        enum SubelementId : uint8_t
        {
            DIRECTIONAL_CHANNEL_QUALITY_REPORTING = 1,
            MEASUREMENT_CONFIGURATION = 2,
            EXTENDED_MEASUREMENT_CONFIGURATION = 3,
            VENDOR_SPECIFIC = 221,
        };

        uint8_t operatingClass{0};        //!< Operating Class (1 octet)
        uint8_t channelNumber{0};         //!< Channel Number (1 octet)
        uint8_t aid{0};                   //!< AID (1 octet)
        uint8_t reserved{0};              //!< Reserved (1 octet)
        uint8_t measurementMethod{0};     //!< Measurement Method (1 octet)
        uint64_t measurementStartTime{0}; //!< Measurement Start Time (8 octets)
        uint16_t measurementDuration{0};  //!< Measurement Duration (2 octets)
        uint8_t numberOfTimeBlocks{0};    //!< Number of Time Blocks (1 octet)

        std::optional<std::vector<uint8_t>> vendorSpecific; //!< Vendor Specific subelement (ID 221)
    };

    /**
     * @brief Request body for Directional Measurement (Figure 9-284)
     */
    struct DirectionalMeasurementRequestBody
    {
        /**
         * @brief Subelement IDs for Directional Measurement request (Table 9-160)
         */
        enum SubelementId : uint8_t
        {
            VENDOR_SPECIFIC = 221,
        };

        uint8_t operatingClass{0};        //!< Operating Class (1 octet)
        uint8_t channelNumber{0};         //!< Channel Number (1 octet)
        uint64_t measurementStartTime{0}; //!< Measurement Start Time (8 octets)
        uint16_t measurementDurationPerDirection{
            0}; //!< Measurement Duration Per Direction (2 octets)
        uint8_t measurementMethodAndAntennaConfiguration{
            0}; //!< Method and Antenna Configuration (1 octet)

        std::optional<std::vector<uint8_t>> vendorSpecific; //!< Vendor Specific subelement (ID 221)
    };

    /**
     * @brief Request body for Directional Statistics measurement (Figure 9-286)
     */
    struct DirectionalStatisticsRequestBody
    {
        /**
         * @brief Subelement IDs for Directional Statistics request (Table 9-161)
         */
        enum SubelementId : uint8_t
        {
            VENDOR_SPECIFIC = 221,
        };

        uint8_t operatingClass{0};        //!< Operating Class (1 octet)
        uint8_t channelNumber{0};         //!< Channel Number (1 octet)
        uint64_t measurementStartTime{0}; //!< Measurement Start Time (8 octets)
        uint16_t measurementDurationPerDirection{
            0};                                 //!< Measurement Duration Per Direction (2 octets)
        uint8_t measurementMethod{0};           //!< Measurement Method (1 octet)
        uint8_t directionalStatisticsBitmap{0}; //!< Directional Statistics Bitmap (1 octet)

        std::optional<std::vector<uint8_t>> vendorSpecific; //!< Vendor Specific subelement (ID 221)
    };

    /**
     * @brief Request body for FTM Range measurement (Figure 9-288)
     */
    struct FtmRangeRequestBody
    {
        /**
         * @brief Subelement IDs for FTM Range request (Table 9-162)
         */
        enum SubelementId : uint8_t
        {
            MAXIMUM_AGE = 4,
            NEIGHBOR_REPORT = 52,
            VENDOR_SPECIFIC = 221,
        };

        uint16_t randomizationInterval{0}; //!< Randomization Interval (2 octets)
        uint8_t minimumApCount{0};         //!< Minimum AP Count (1 octet)

        std::vector<NeighborReportElement> neighborReports; //!< Neighbor Report subelements (ID 52)
        std::optional<std::vector<uint8_t>> vendorSpecific; //!< Vendor Specific subelement (ID 221)
    };

    /**
     * @brief Request body for Measurement Pause (Figure 9-272)
     */
    struct MeasurementPauseRequestBody
    {
        /**
         * @brief Subelement IDs for Measurement Pause request (Table 9-152)
         */
        enum SubelementId : uint8_t
        {
            VENDOR_SPECIFIC = 221,
        };

        uint16_t pauseTime{0}; //!< Pause Time (2 octets)

        std::optional<std::vector<uint8_t>> vendorSpecific; //!< Vendor Specific subelement (ID 221)
    };

    /**
     * @brief Variant holding the type-specific request body
     */
    using MeasurementRequestBody = std::variant<std::monostate,
                                                BasicRequestBody,
                                                ChannelLoadRequestBody,
                                                NoiseHistogramRequestBody,
                                                BeaconRequestBody,
                                                FrameRequestBody,
                                                StaStatisticsRequestBody,
                                                LciRequestBody,
                                                TransmitStreamRequestBody,
                                                MulticastDiagnosticsRequestBody,
                                                LocationCivicRequestBody,
                                                LocationIdentifierRequestBody,
                                                DirectionalChannelQualityRequestBody,
                                                DirectionalMeasurementRequestBody,
                                                DirectionalStatisticsRequestBody,
                                                FtmRangeRequestBody,
                                                MeasurementPauseRequestBody>;

    /**
     * @brief Set the type-specific request body.
     * @tparam T the body struct type
     * @param body the body to set
     */
    template <typename T>
    void SetBody(const T& body);

    /**
     * @brief Get the type-specific request body.
     * @tparam T the body struct type
     * @return const reference to the body
     */
    template <typename T>
    const T& GetBody() const;

    // --- Fixed header fields ---

    /**
     * @brief Set the Measurement Token field.
     * @param token the measurement token
     */
    void SetMeasurementToken(uint8_t token);
    /**
     * @brief Get the Measurement Token field.
     * @return the measurement token
     */
    uint8_t GetMeasurementToken() const;

    /**
     * @brief Set the Measurement Request Mode field.
     * Bits B5-B7 are reserved and masked to zero.
     * @param mode the mode bitmap
     */
    void SetMeasurementRequestMode(uint8_t mode);
    /**
     * @brief Get the Measurement Request Mode field.
     * @return the mode bitmap (B5-B7 always zero)
     */
    uint8_t GetMeasurementRequestMode() const;

    /**
     * @brief Set the Measurement Type field.
     * @param type the measurement type
     */
    void SetMeasurementType(uint8_t type);
    /**
     * @brief Get the Measurement Type field.
     * @return the measurement type
     */
    uint8_t GetMeasurementType() const;

    // --- Mode bitmap bit accessors (Figure 9-242) ---

    /**
     * @brief Set the Parallel bit (B0).
     * @param parallel true to set, false to clear
     */
    void SetParallel(bool parallel);
    /**
     * @brief Get the Parallel bit (B0).
     * @return the Parallel bit value
     */
    bool GetParallel() const;

    /**
     * @brief Set the Enable bit (B1).
     * @param enable true to set, false to clear
     */
    void SetEnable(bool enable);
    /**
     * @brief Get the Enable bit (B1).
     * @return the Enable bit value
     */
    bool GetEnable() const;

    /**
     * @brief Set the Request bit (B2).
     * @param request true to set, false to clear
     */
    void SetRequest(bool request);
    /**
     * @brief Get the Request bit (B2).
     * @return the Request bit value
     */
    bool GetRequest() const;

    /**
     * @brief Set the Report bit (B3).
     * @param report true to set, false to clear
     */
    void SetReport(bool report);
    /**
     * @brief Get the Report bit (B3).
     * @return the Report bit value
     */
    bool GetReport() const;

    /**
     * @brief Set the Duration Mandatory bit (B4).
     * @param mandatory true to set, false to clear
     */
    void SetDurationMandatory(bool mandatory);
    /**
     * @brief Get the Duration Mandatory bit (B4).
     * @return the Duration Mandatory bit value
     */
    bool GetDurationMandatory() const;

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;

    // Fixed header
    uint8_t m_measurementToken{0};       //!< Measurement Token (1 octet)
    uint8_t m_measurementRequestMode{0}; //!< Measurement Request Mode (1 octet, B0-B4 defined)
    uint8_t m_measurementType{0};        //!< Measurement Type (1 octet)
    MeasurementRequestBody m_body;       //!< Type-specific request body
};

namespace detail
{

/**
 * @brief Map a body struct type to its MeasurementType value.
 *
 * BasicRequestBody is excluded because it is shared by types 0/1/2;
 * the caller must set m_measurementType explicitly for those.
 */
template <typename T>
constexpr uint8_t
MeasurementTypeFor()
{
    using MT = MeasurementRequestElement::MeasurementType;
    if constexpr (std::is_same_v<T, MeasurementRequestElement::ChannelLoadRequestBody>)
    {
        return static_cast<uint8_t>(MT::CHANNEL_LOAD);
    }
    else if constexpr (std::is_same_v<T, MeasurementRequestElement::NoiseHistogramRequestBody>)
    {
        return static_cast<uint8_t>(MT::NOISE_HISTOGRAM);
    }
    else if constexpr (std::is_same_v<T, MeasurementRequestElement::BeaconRequestBody>)
    {
        return static_cast<uint8_t>(MT::BEACON);
    }
    else if constexpr (std::is_same_v<T, MeasurementRequestElement::FrameRequestBody>)
    {
        return static_cast<uint8_t>(MT::FRAME);
    }
    else if constexpr (std::is_same_v<T, MeasurementRequestElement::StaStatisticsRequestBody>)
    {
        return static_cast<uint8_t>(MT::STA_STATISTICS);
    }
    else if constexpr (std::is_same_v<T, MeasurementRequestElement::LciRequestBody>)
    {
        return static_cast<uint8_t>(MT::LCI);
    }
    else if constexpr (std::is_same_v<T, MeasurementRequestElement::TransmitStreamRequestBody>)
    {
        return static_cast<uint8_t>(MT::TRANSMIT_STREAM);
    }
    else if constexpr (std::is_same_v<T,
                                      MeasurementRequestElement::MulticastDiagnosticsRequestBody>)
    {
        return static_cast<uint8_t>(MT::MULTICAST_DIAGNOSTICS);
    }
    else if constexpr (std::is_same_v<T, MeasurementRequestElement::LocationCivicRequestBody>)
    {
        return static_cast<uint8_t>(MT::LOCATION_CIVIC);
    }
    else if constexpr (std::is_same_v<T, MeasurementRequestElement::LocationIdentifierRequestBody>)
    {
        return static_cast<uint8_t>(MT::LOCATION_IDENTIFIER);
    }
    else if constexpr (std::is_same_v<
                           T,
                           MeasurementRequestElement::DirectionalChannelQualityRequestBody>)
    {
        return static_cast<uint8_t>(MT::DIRECTIONAL_CHANNEL_QUALITY);
    }
    else if constexpr (std::is_same_v<T,
                                      MeasurementRequestElement::DirectionalMeasurementRequestBody>)
    {
        return static_cast<uint8_t>(MT::DIRECTIONAL_MEASUREMENT);
    }
    else if constexpr (std::is_same_v<T,
                                      MeasurementRequestElement::DirectionalStatisticsRequestBody>)
    {
        return static_cast<uint8_t>(MT::DIRECTIONAL_STATISTICS);
    }
    else if constexpr (std::is_same_v<T, MeasurementRequestElement::FtmRangeRequestBody>)
    {
        return static_cast<uint8_t>(MT::FTM_RANGE);
    }
    else if constexpr (std::is_same_v<T, MeasurementRequestElement::MeasurementPauseRequestBody>)
    {
        return static_cast<uint8_t>(MT::MEASUREMENT_PAUSE);
    }
    else
    {
        static_assert(!std::is_same_v<T, T>, "Unrecognized body type for MeasurementTypeFor");
    }
}

} // namespace detail

template <typename T>
void
MeasurementRequestElement::SetBody(const T& body)
{
    if constexpr (!std::is_same_v<T, BasicRequestBody>)
    {
        m_measurementType = detail::MeasurementTypeFor<T>();
    }
    m_body = body;
}

template <typename T>
const T&
MeasurementRequestElement::GetBody() const
{
    return std::get<T>(m_body);
}

} // namespace ns3

#endif /* MEASUREMENT_REQUEST_ELEMENT_H */
