/*
 * Copyright (c) 2025
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

    // --- Type-specific body field accessors ---

    /**
     * @brief Set the Operating Class field.
     * @param operatingClass the operating class
     */
    void SetOperatingClass(uint8_t operatingClass);
    /**
     * @brief Get the Operating Class field.
     * @return the operating class
     */
    uint8_t GetOperatingClass() const;

    /**
     * @brief Set the Channel Number field.
     * @param channel the channel number
     */
    void SetChannelNumber(uint8_t channel);
    /**
     * @brief Get the Channel Number field.
     * @return the channel number
     */
    uint8_t GetChannelNumber() const;

    /**
     * @brief Set the Randomization Interval field.
     * @param interval the randomization interval in TUs
     */
    void SetRandomizationInterval(uint16_t interval);
    /**
     * @brief Get the Randomization Interval field.
     * @return the randomization interval
     */
    uint16_t GetRandomizationInterval() const;

    /**
     * @brief Set the Measurement Duration field.
     * @param duration the measurement duration in TUs
     */
    void SetMeasurementDuration(uint16_t duration);
    /**
     * @brief Get the Measurement Duration field.
     * @return the measurement duration
     */
    uint16_t GetMeasurementDuration() const;

    /**
     * @brief Set the Beacon Measurement Reporting Mode field (type 5).
     * @param mode the beacon measurement mode
     */
    void SetBeaconMeasurementMode(uint8_t mode);
    /**
     * @brief Get the Beacon Measurement Reporting Mode field.
     * @return the beacon measurement mode
     */
    uint8_t GetBeaconMeasurementMode() const;

    /**
     * @brief Set the BSSID field (type 5).
     * @param bssid the BSSID
     */
    void SetBssid(Mac48Address bssid);
    /**
     * @brief Get the BSSID field.
     * @return the BSSID
     */
    Mac48Address GetBssid() const;

    /**
     * @brief Set the Frame Request Type field (type 6).
     * @param frameRequestType the frame request type
     */
    void SetFrameRequestType(uint8_t frameRequestType);
    /**
     * @brief Get the Frame Request Type field.
     * @return the frame request type
     */
    uint8_t GetFrameRequestType() const;

    /**
     * @brief Set the MAC Address field (type 6).
     * @param macAddress the MAC address
     */
    void SetMacAddress(Mac48Address macAddress);
    /**
     * @brief Get the MAC Address field.
     * @return the MAC address
     */
    Mac48Address GetMacAddress() const;

    /**
     * @brief Set the Peer MAC Address field (type 7).
     * @param peerMacAddress the peer MAC address
     */
    void SetPeerMacAddress(Mac48Address peerMacAddress);
    /**
     * @brief Get the Peer MAC Address field.
     * @return the peer MAC address
     */
    Mac48Address GetPeerMacAddress() const;

    /**
     * @brief Set the Group Identity field (type 7).
     * @param groupIdentity the group identity
     */
    void SetGroupIdentity(uint8_t groupIdentity);
    /**
     * @brief Get the Group Identity field.
     * @return the group identity
     */
    uint8_t GetGroupIdentity() const;

    /**
     * @brief Set the Location Subject field (types 8, 11, 12).
     * @param locationSubject the location subject
     */
    void SetLocationSubject(uint8_t locationSubject);
    /**
     * @brief Get the Location Subject field.
     * @return the location subject
     */
    uint8_t GetLocationSubject() const;

    /**
     * @brief Set the Peer STA Address field (type 9).
     * @param peerStaAddress the peer STA address
     */
    void SetPeerStaAddress(Mac48Address peerStaAddress);
    /**
     * @brief Get the Peer STA Address field.
     * @return the peer STA address
     */
    Mac48Address GetPeerStaAddress() const;

    /**
     * @brief Set the Traffic Identifier field (type 9).
     * @param tid the traffic identifier
     */
    void SetTrafficIdentifier(uint8_t tid);
    /**
     * @brief Get the Traffic Identifier field.
     * @return the traffic identifier
     */
    uint8_t GetTrafficIdentifier() const;

    /**
     * @brief Set the Bin 0 Range field (type 9).
     * @param bin0Range the Bin 0 range
     */
    void SetBin0Range(uint8_t bin0Range);
    /**
     * @brief Get the Bin 0 Range field.
     * @return the Bin 0 range
     */
    uint8_t GetBin0Range() const;

    /**
     * @brief Set the Group MAC Address field (type 10).
     * @param groupMacAddress the group MAC address
     */
    void SetGroupMacAddress(Mac48Address groupMacAddress);
    /**
     * @brief Get the Group MAC Address field.
     * @return the group MAC address
     */
    Mac48Address GetGroupMacAddress() const;

    /**
     * @brief Set the Civic Location Type field (type 11).
     * @param civicLocationType the civic location type
     */
    void SetCivicLocationType(uint8_t civicLocationType);
    /**
     * @brief Get the Civic Location Type field.
     * @return the civic location type
     */
    uint8_t GetCivicLocationType() const;

    /**
     * @brief Set the Location Service Interval Units field (types 11, 12).
     * @param units the location service interval units
     */
    void SetLocationServiceIntervalUnits(uint8_t units);
    /**
     * @brief Get the Location Service Interval Units field.
     * @return the location service interval units
     */
    uint8_t GetLocationServiceIntervalUnits() const;

    /**
     * @brief Set the Location Service Interval field (types 11, 12).
     * @param interval the location service interval
     */
    void SetLocationServiceInterval(uint16_t interval);
    /**
     * @brief Get the Location Service Interval field.
     * @return the location service interval
     */
    uint16_t GetLocationServiceInterval() const;

    /**
     * @brief Set the AID field (type 13).
     * @param aid the AID
     */
    void SetAid(uint8_t aid);
    /**
     * @brief Get the AID field.
     * @return the AID
     */
    uint8_t GetAid() const;

    /**
     * @brief Set the Measurement Method field (types 13, 15).
     * @param method the measurement method
     */
    void SetMeasurementMethod(uint8_t method);
    /**
     * @brief Get the Measurement Method field.
     * @return the measurement method
     */
    uint8_t GetMeasurementMethod() const;

    /**
     * @brief Set the Measurement Start Time field (types 0, 1, 2, 13, 14, 15).
     * @param startTime the measurement start time
     */
    void SetMeasurementStartTime(uint64_t startTime);
    /**
     * @brief Get the Measurement Start Time field.
     * @return the measurement start time
     */
    uint64_t GetMeasurementStartTime() const;

    /**
     * @brief Set the Number of Time Blocks field (type 13).
     * @param numberOfTimeBlocks the number of time blocks
     */
    void SetNumberOfTimeBlocks(uint8_t numberOfTimeBlocks);
    /**
     * @brief Get the Number of Time Blocks field.
     * @return the number of time blocks
     */
    uint8_t GetNumberOfTimeBlocks() const;

    /**
     * @brief Set the Measurement Method and Antenna Configuration field (type 14).
     * @param value the combined method and antenna configuration
     */
    void SetMeasurementMethodAndAntennaConfiguration(uint8_t value);
    /**
     * @brief Get the Measurement Method and Antenna Configuration field.
     * @return the combined method and antenna configuration
     */
    uint8_t GetMeasurementMethodAndAntennaConfiguration() const;

    /**
     * @brief Set the Directional Statistics Bitmap field (type 15).
     * @param bitmap the directional statistics bitmap
     */
    void SetDirectionalStatisticsBitmap(uint8_t bitmap);
    /**
     * @brief Get the Directional Statistics Bitmap field.
     * @return the directional statistics bitmap
     */
    uint8_t GetDirectionalStatisticsBitmap() const;

    /**
     * @brief Set the Minimum AP Count field (type 16).
     * @param minApCount the minimum AP count
     */
    void SetMinimumApCount(uint8_t minApCount);
    /**
     * @brief Get the Minimum AP Count field.
     * @return the minimum AP count
     */
    uint8_t GetMinimumApCount() const;

    /**
     * @brief Set the Pause Time field (type 255).
     * @param pauseTime the pause time
     */
    void SetPauseTime(uint16_t pauseTime);
    /**
     * @brief Get the Pause Time field.
     * @return the pause time
     */
    uint16_t GetPauseTime() const;

    // --- Optional subelement accessors ---

    /**
     * @brief Set the Channel Load Reporting subelement (ID 1, type 3).
     * @param condition the reporting condition (Table 9-138)
     * @param channelLoadRefValue the Channel Load reference value
     */
    void SetChannelLoadReporting(uint8_t condition, uint8_t channelLoadRefValue);
    /**
     * @brief Get the Channel Load Reporting subelement.
     * @return the Channel Load Reporting data if present
     */
    std::optional<ChannelLoadReporting> GetChannelLoadReporting() const;

    /**
     * @brief Set the Noise Histogram Reporting subelement (ID 1, type 4).
     * @param condition the reporting condition (Table 9-140)
     * @param anpiRefValue the ANPI reference value
     */
    void SetNoiseHistogramReporting(uint8_t condition, uint8_t anpiRefValue);
    /**
     * @brief Get the Noise Histogram Reporting subelement.
     * @return the Noise Histogram Reporting data if present
     */
    std::optional<NoiseHistogramReporting> GetNoiseHistogramReporting() const;

    /**
     * @brief Set the Beacon Reporting subelement (ID 1, type 5).
     * @param condition the reporting condition (Table 9-143)
     * @param thresholdOffsetRef the threshold/offset reference value
     */
    void SetBeaconReporting(uint8_t condition, uint8_t thresholdOffsetRef);
    /**
     * @brief Get the Beacon Reporting subelement.
     * @return the Beacon Reporting data if present
     */
    std::optional<BeaconReporting> GetBeaconReporting() const;

    /**
     * @brief Set the Beacon SSID subelement (ID 0, type 5).
     * @param ssid the SSID
     */
    void SetBeaconSsid(Ssid ssid);
    /**
     * @brief Get the Beacon SSID subelement.
     * @return the SSID if present
     */
    std::optional<Ssid> GetBeaconSsid() const;

    /**
     * @brief Set the Beacon Reporting Detail subelement (ID 2, type 5).
     * @param detail the reporting detail
     */
    void SetBeaconReportingDetail(uint8_t detail);
    /**
     * @brief Get the Beacon Reporting Detail subelement.
     * @return the reporting detail if present
     */
    std::optional<uint8_t> GetBeaconReportingDetail() const;

    /**
     * @brief Add an AP Channel Report subelement (ID 51, type 5).
     * @param opClass the operating class
     * @param channels the channel list
     */
    void AddApChannelReport(uint8_t opClass, std::vector<uint8_t> channels);
    /**
     * @brief Get all AP Channel Report subelements.
     * @return the AP Channel Reports
     */
    std::vector<ApChannelReport> GetApChannelReports() const;

    /**
     * @brief Set the Azimuth Request subelement (ID 1, type 8).
     * @param resolution the azimuth resolution
     * @param type the azimuth type
     */
    void SetAzimuthRequest(uint8_t resolution, uint8_t type);
    /**
     * @brief Get the Azimuth Request subelement.
     * @return the Azimuth Request data if present
     */
    std::optional<AzimuthRequest> GetAzimuthRequest() const;

    /**
     * @brief Add a Neighbor Report subelement for FTM Range (ID 52, type 16).
     * @param nre the Neighbor Report element
     */
    void AddFtmRangeNeighborReport(const NeighborReportElement& nre);
    /**
     * @brief Get the FTM Range Neighbor Report subelements.
     * @return the Neighbor Report elements
     */
    std::vector<NeighborReportElement> GetFtmRangeNeighborReports() const;

    /**
     * @brief Set the Vendor Specific subelement (ID 221).
     * @param data the vendor-specific data
     */
    void SetVendorSpecificSubelement(std::vector<uint8_t> data);
    /**
     * @brief Get the Vendor Specific subelement.
     * @return the vendor-specific data if present
     */
    std::optional<std::vector<uint8_t>> GetVendorSpecificSubelement() const;

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;

    // Fixed header
    uint8_t m_measurementToken{0};       //!< Measurement Token (1 octet)
    uint8_t m_measurementRequestMode{0}; //!< Measurement Request Mode (1 octet, B0-B4 defined)
    uint8_t m_measurementType{0};        //!< Measurement Type (1 octet)
    MeasurementRequestBody m_body;       //!< Type-specific request body

    // Type-specific body fields
    uint8_t m_operatingClass{0};         //!< Operating Class (types 3,4,5,6,13,14,15)
    uint8_t m_channelNumber{0};          //!< Channel Number (types 0,1,2,3,4,5,6,13,14,15)
    uint16_t m_randomizationInterval{0}; //!< Randomization Interval (types 3,4,5,6,7,9,10,16)
    uint16_t m_measurementDuration{
        0}; //!< Measurement Duration (types 0,1,2,3,4,5,6,7,9,10,13,14,15)

    uint8_t m_beaconMeasurementMode{0};        //!< Beacon Measurement Reporting Mode (type 5)
    Mac48Address m_bssid;                      //!< BSSID (type 5)
    uint8_t m_frameRequestType{0};             //!< Frame Request Type (type 6)
    Mac48Address m_macAddress;                 //!< MAC Address (type 6)
    Mac48Address m_peerMacAddress;             //!< Peer MAC Address (type 7)
    uint8_t m_groupIdentity{0};                //!< Group Identity (type 7)
    uint8_t m_locationSubject{0};              //!< Location Subject (types 8,11,12)
    Mac48Address m_peerStaAddress;             //!< Peer STA Address (type 9)
    uint8_t m_trafficIdentifier{0};            //!< Traffic Identifier (type 9)
    uint8_t m_bin0Range{0};                    //!< Bin 0 Range (type 9)
    Mac48Address m_groupMacAddress;            //!< Group MAC Address (type 10)
    uint8_t m_civicLocationType{0};            //!< Civic Location Type (type 11)
    uint8_t m_locationServiceIntervalUnits{0}; //!< Location Service Interval Units (types 11,12)
    uint16_t m_locationServiceInterval{0};     //!< Location Service Interval (types 11,12)
    uint8_t m_aid{0};                          //!< AID (type 13)
    uint8_t m_measurementMethod{0};            //!< Measurement Method (types 13,15)
    uint64_t m_measurementStartTime{0};        //!< Measurement Start Time (types 0,1,2,13,14,15)
    uint8_t m_numberOfTimeBlocks{0};           //!< Number of Time Blocks (type 13)
    uint8_t m_measurementMethodAndAntennaConfiguration{0}; //!< Method and Antenna Config (type 14)
    uint8_t m_directionalStatisticsBitmap{0}; //!< Directional Statistics Bitmap (type 15)
    uint8_t m_minimumApCount{0};              //!< Minimum AP Count (type 16)
    uint16_t m_pauseTime{0};                  //!< Pause Time (type 255)

    // Optional subelements
    std::optional<ChannelLoadReporting>
        m_channelLoadReporting; //!< Channel Load Reporting (ID 1, type 3)
    std::optional<NoiseHistogramReporting>
        m_noiseHistogramReporting;                    //!< Noise Histogram Reporting (ID 1, type 4)
    std::optional<BeaconReporting> m_beaconReporting; //!< Beacon Reporting (ID 1, type 5)
    std::optional<Ssid> m_beaconSsid;                 //!< Beacon SSID subelement (ID 0)
    std::optional<uint8_t> m_beaconReportingDetail;   //!< Beacon Reporting Detail (ID 2)
    std::vector<ApChannelReport> m_apChannelReports;  //!< AP Channel Reports (ID 51)
    std::optional<AzimuthRequest> m_azimuthRequest;   //!< Azimuth Request (ID 1, type 8)
    std::vector<NeighborReportElement> m_ftmRangeNeighborReports;   //!< FTM Range NREs (ID 52)
    std::optional<std::vector<uint8_t>> m_vendorSpecificSubelement; //!< Vendor Specific (ID 221)
};

template <typename T>
void
MeasurementRequestElement::SetBody(const T& body)
{
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
