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

        /**
         * @return serialized size in bytes
         */
        uint8_t GetSerializedSize() const
        {
            return sizeof(reportingCondition) + sizeof(channelLoadReferenceValue);
        }
    };

    /**
     * @brief Noise Histogram Reporting subelement data (Table 9-139, Figure 9-249)
     */
    struct NoiseHistogramReporting
    {
        uint8_t reportingCondition; //!< Reporting Condition (1 octet, Table 9-140)
        uint8_t anpiReferenceValue; //!< ANPI Reference Value (1 octet)

        /**
         * @return serialized size in bytes
         */
        uint8_t GetSerializedSize() const
        {
            return sizeof(reportingCondition) + sizeof(anpiReferenceValue);
        }
    };

    /**
     * @brief Beacon Reporting subelement data (Table 9-142, Figure 9-251)
     */
    struct BeaconReporting
    {
        uint8_t reportingCondition;       //!< Reporting Condition (1 octet, Table 9-143)
        uint8_t thresholdOffsetReference; //!< Threshold/Offset Reference (1 octet)

        /**
         * @return serialized size in bytes
         */
        uint8_t GetSerializedSize() const
        {
            return sizeof(reportingCondition) + sizeof(thresholdOffsetReference);
        }
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
    class BasicRequestBody
    {
      public:
        /**
         * @brief Set the Channel Number field.
         * @param channelNumber the channel number
         */
        void SetChannelNumber(uint8_t channelNumber)
        {
            m_channelNumber = channelNumber;
        }

        /**
         * @brief Get the Channel Number field.
         * @return the channel number
         */
        uint8_t GetChannelNumber() const
        {
            return m_channelNumber;
        }

        /**
         * @brief Set the Measurement Start Time field.
         * @param measurementStartTime the measurement start time
         */
        void SetMeasurementStartTime(uint64_t measurementStartTime)
        {
            m_measurementStartTime = measurementStartTime;
        }

        /**
         * @brief Get the Measurement Start Time field.
         * @return the measurement start time
         */
        uint64_t GetMeasurementStartTime() const
        {
            return m_measurementStartTime;
        }

        /**
         * @brief Set the Measurement Duration field.
         * @param measurementDuration the measurement duration
         */
        void SetMeasurementDuration(uint16_t measurementDuration)
        {
            m_measurementDuration = measurementDuration;
        }

        /**
         * @brief Get the Measurement Duration field.
         * @return the measurement duration
         */
        uint16_t GetMeasurementDuration() const
        {
            return m_measurementDuration;
        }

      private:
        uint8_t m_channelNumber{0};         ///< Channel Number (1 octet)
        uint64_t m_measurementStartTime{0}; ///< Measurement Start Time (8 octets)
        uint16_t m_measurementDuration{0};  ///< Measurement Duration (2 octets)
    };

    /**
     * @brief Request body for Channel Load measurement (Figure 9-246)
     */
    class ChannelLoadRequestBody
    {
      public:
        /**
         * @brief Subelement IDs for Channel Load request (Table 9-137)
         */
        enum class SubelementId : uint8_t
        {
            RESERVED = 0,
            CHANNEL_LOAD_REPORTING = 1,
            WIDE_BANDWIDTH_CHANNEL_SWITCH = 163,
            VENDOR_SPECIFIC = 221,
        };

        /**
         * @brief Set the Operating Class field.
         * @param operatingClass the operating class
         */
        void SetOperatingClass(uint8_t operatingClass)
        {
            m_operatingClass = operatingClass;
        }

        /**
         * @brief Get the Operating Class field.
         * @return the operating class
         */
        uint8_t GetOperatingClass() const
        {
            return m_operatingClass;
        }

        /**
         * @brief Set the Channel Number field.
         * @param channelNumber the channel number
         */
        void SetChannelNumber(uint8_t channelNumber)
        {
            m_channelNumber = channelNumber;
        }

        /**
         * @brief Get the Channel Number field.
         * @return the channel number
         */
        uint8_t GetChannelNumber() const
        {
            return m_channelNumber;
        }

        /**
         * @brief Set the Randomization Interval field.
         * @param randomizationInterval the randomization interval
         */
        void SetRandomizationInterval(uint16_t randomizationInterval)
        {
            m_randomizationInterval = randomizationInterval;
        }

        /**
         * @brief Get the Randomization Interval field.
         * @return the randomization interval
         */
        uint16_t GetRandomizationInterval() const
        {
            return m_randomizationInterval;
        }

        /**
         * @brief Set the Measurement Duration field.
         * @param measurementDuration the measurement duration
         */
        void SetMeasurementDuration(uint16_t measurementDuration)
        {
            m_measurementDuration = measurementDuration;
        }

        /**
         * @brief Get the Measurement Duration field.
         * @return the measurement duration
         */
        uint16_t GetMeasurementDuration() const
        {
            return m_measurementDuration;
        }

        /**
         * @brief Set the Channel Load Reporting subelement.
         * @param channelLoadReporting the channel load reporting parameters
         */
        void SetChannelLoadReporting(const ChannelLoadReporting& channelLoadReporting)
        {
            m_channelLoadReporting = channelLoadReporting;
        }

        /**
         * @brief Get the Channel Load Reporting subelement.
         * @return the channel load reporting parameters, or std::nullopt if not present
         */
        const std::optional<ChannelLoadReporting>& GetChannelLoadReporting() const
        {
            return m_channelLoadReporting;
        }

        /**
         * @brief Set the Vendor Specific subelement.
         * @param vendorSpecific the vendor specific data
         */
        void SetVendorSpecific(std::vector<uint8_t> vendorSpecific)
        {
            m_vendorSpecific = std::move(vendorSpecific);
        }

        /**
         * @brief Get the Vendor Specific subelement.
         * @return the vendor specific data, or std::nullopt if not present
         */
        const std::optional<std::vector<uint8_t>>& GetVendorSpecific() const
        {
            return m_vendorSpecific;
        }

      private:
        uint8_t m_operatingClass{0};         ///< Operating Class (1 octet)
        uint8_t m_channelNumber{0};          ///< Channel Number (1 octet)
        uint16_t m_randomizationInterval{0}; ///< Randomization Interval (2 octets)
        uint16_t m_measurementDuration{0};   ///< Measurement Duration (2 octets)

        std::optional<ChannelLoadReporting> m_channelLoadReporting; ///< Reporting subelement (ID 1)
        std::optional<std::vector<uint8_t>>
            m_vendorSpecific; ///< Vendor Specific subelement (ID 221)
    };

    /**
     * @brief Request body for Noise Histogram measurement (Figure 9-248)
     */
    class NoiseHistogramRequestBody
    {
      public:
        /**
         * @brief Subelement IDs for Noise Histogram request (Table 9-139)
         */
        enum class SubelementId : uint8_t
        {
            RESERVED = 0,
            NOISE_HISTOGRAM_REPORTING = 1,
            WIDE_BANDWIDTH_CHANNEL_SWITCH = 163,
            VENDOR_SPECIFIC = 221,
        };

        /**
         * @brief Set the Operating Class field.
         * @param operatingClass the operating class
         */
        void SetOperatingClass(uint8_t operatingClass)
        {
            m_operatingClass = operatingClass;
        }

        /**
         * @brief Get the Operating Class field.
         * @return the operating class
         */
        uint8_t GetOperatingClass() const
        {
            return m_operatingClass;
        }

        /**
         * @brief Set the Channel Number field.
         * @param channelNumber the channel number
         */
        void SetChannelNumber(uint8_t channelNumber)
        {
            m_channelNumber = channelNumber;
        }

        /**
         * @brief Get the Channel Number field.
         * @return the channel number
         */
        uint8_t GetChannelNumber() const
        {
            return m_channelNumber;
        }

        /**
         * @brief Set the Randomization Interval field.
         * @param randomizationInterval the randomization interval
         */
        void SetRandomizationInterval(uint16_t randomizationInterval)
        {
            m_randomizationInterval = randomizationInterval;
        }

        /**
         * @brief Get the Randomization Interval field.
         * @return the randomization interval
         */
        uint16_t GetRandomizationInterval() const
        {
            return m_randomizationInterval;
        }

        /**
         * @brief Set the Measurement Duration field.
         * @param measurementDuration the measurement duration
         */
        void SetMeasurementDuration(uint16_t measurementDuration)
        {
            m_measurementDuration = measurementDuration;
        }

        /**
         * @brief Get the Measurement Duration field.
         * @return the measurement duration
         */
        uint16_t GetMeasurementDuration() const
        {
            return m_measurementDuration;
        }

        /**
         * @brief Set the Noise Histogram Reporting subelement.
         * @param noiseHistogramReporting the noise histogram reporting parameters
         */
        void SetNoiseHistogramReporting(const NoiseHistogramReporting& noiseHistogramReporting)
        {
            m_noiseHistogramReporting = noiseHistogramReporting;
        }

        /**
         * @brief Get the Noise Histogram Reporting subelement.
         * @return the noise histogram reporting parameters, or std::nullopt if not present
         */
        const std::optional<NoiseHistogramReporting>& GetNoiseHistogramReporting() const
        {
            return m_noiseHistogramReporting;
        }

        /**
         * @brief Set the Vendor Specific subelement.
         * @param vendorSpecific the vendor specific data
         */
        void SetVendorSpecific(std::vector<uint8_t> vendorSpecific)
        {
            m_vendorSpecific = std::move(vendorSpecific);
        }

        /**
         * @brief Get the Vendor Specific subelement.
         * @return the vendor specific data, or std::nullopt if not present
         */
        const std::optional<std::vector<uint8_t>>& GetVendorSpecific() const
        {
            return m_vendorSpecific;
        }

      private:
        uint8_t m_operatingClass{0};         ///< Operating Class (1 octet)
        uint8_t m_channelNumber{0};          ///< Channel Number (1 octet)
        uint16_t m_randomizationInterval{0}; ///< Randomization Interval (2 octets)
        uint16_t m_measurementDuration{0};   ///< Measurement Duration (2 octets)

        std::optional<NoiseHistogramReporting>
            m_noiseHistogramReporting; ///< Reporting subelement (ID 1)
        std::optional<std::vector<uint8_t>>
            m_vendorSpecific; ///< Vendor Specific subelement (ID 221)
    };

    /**
     * @brief Request body for Beacon measurement (Figure 9-250)
     */
    class BeaconRequestBody
    {
      public:
        /**
         * @brief Subelement IDs for Beacon request (Table 9-142)
         */
        enum class SubelementId : uint8_t
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

        /**
         * @brief Set the Operating Class field.
         * @param operatingClass the operating class
         */
        void SetOperatingClass(uint8_t operatingClass)
        {
            m_operatingClass = operatingClass;
        }

        /**
         * @brief Get the Operating Class field.
         * @return the operating class
         */
        uint8_t GetOperatingClass() const
        {
            return m_operatingClass;
        }

        /**
         * @brief Set the Channel Number field.
         * @param channelNumber the channel number
         */
        void SetChannelNumber(uint8_t channelNumber)
        {
            m_channelNumber = channelNumber;
        }

        /**
         * @brief Get the Channel Number field.
         * @return the channel number
         */
        uint8_t GetChannelNumber() const
        {
            return m_channelNumber;
        }

        /**
         * @brief Set the Randomization Interval field.
         * @param randomizationInterval the randomization interval
         */
        void SetRandomizationInterval(uint16_t randomizationInterval)
        {
            m_randomizationInterval = randomizationInterval;
        }

        /**
         * @brief Get the Randomization Interval field.
         * @return the randomization interval
         */
        uint16_t GetRandomizationInterval() const
        {
            return m_randomizationInterval;
        }

        /**
         * @brief Set the Measurement Duration field.
         * @param measurementDuration the measurement duration
         */
        void SetMeasurementDuration(uint16_t measurementDuration)
        {
            m_measurementDuration = measurementDuration;
        }

        /**
         * @brief Get the Measurement Duration field.
         * @return the measurement duration
         */
        uint16_t GetMeasurementDuration() const
        {
            return m_measurementDuration;
        }

        /**
         * @brief Set the Measurement Mode field.
         * @param measurementMode the measurement mode
         */
        void SetMeasurementMode(uint8_t measurementMode)
        {
            m_measurementMode = measurementMode;
        }

        /**
         * @brief Get the Measurement Mode field.
         * @return the measurement mode
         */
        uint8_t GetMeasurementMode() const
        {
            return m_measurementMode;
        }

        /**
         * @brief Set the BSSID field.
         * @param bssid the BSSID
         */
        void SetBssid(const Mac48Address& bssid)
        {
            m_bssid = bssid;
        }

        /**
         * @brief Get the BSSID field.
         * @return the BSSID
         */
        Mac48Address GetBssid() const
        {
            return m_bssid;
        }

        /**
         * @brief Set the SSID subelement.
         * @param ssid the SSID
         */
        void SetSsid(const Ssid& ssid)
        {
            m_ssid = ssid;
        }

        /**
         * @brief Get the SSID subelement.
         * @return the SSID, or std::nullopt if not present
         */
        const std::optional<Ssid>& GetSsid() const
        {
            return m_ssid;
        }

        /**
         * @brief Set the Beacon Reporting subelement.
         * @param beaconReporting the beacon reporting parameters
         */
        void SetBeaconReporting(const BeaconReporting& beaconReporting)
        {
            m_beaconReporting = beaconReporting;
        }

        /**
         * @brief Get the Beacon Reporting subelement.
         * @return the beacon reporting parameters, or std::nullopt if not present
         */
        const std::optional<BeaconReporting>& GetBeaconReporting() const
        {
            return m_beaconReporting;
        }

        /**
         * @brief Set the Reporting Detail subelement.
         * @param reportingDetail the reporting detail value
         */
        void SetReportingDetail(uint8_t reportingDetail)
        {
            m_reportingDetail = reportingDetail;
        }

        /**
         * @brief Get the Reporting Detail subelement.
         * @return the reporting detail value, or std::nullopt if not present
         */
        std::optional<uint8_t> GetReportingDetail() const
        {
            return m_reportingDetail;
        }

        /**
         * @brief Add an AP Channel Report subelement.
         * @param apChannelReport the AP channel report to add
         */
        void AddApChannelReport(const ApChannelReport& apChannelReport)
        {
            m_apChannelReports.push_back(apChannelReport);
        }

        /**
         * @brief Get all AP Channel Report subelements.
         * @return the list of AP channel reports
         */
        const std::vector<ApChannelReport>& GetApChannelReports() const
        {
            return m_apChannelReports;
        }

        /**
         * @brief Set the Vendor Specific subelement.
         * @param vendorSpecific the vendor specific data
         */
        void SetVendorSpecific(std::vector<uint8_t> vendorSpecific)
        {
            m_vendorSpecific = std::move(vendorSpecific);
        }

        /**
         * @brief Get the Vendor Specific subelement.
         * @return the vendor specific data, or std::nullopt if not present
         */
        const std::optional<std::vector<uint8_t>>& GetVendorSpecific() const
        {
            return m_vendorSpecific;
        }

      private:
        uint8_t m_operatingClass{0};         ///< Operating Class (1 octet)
        uint8_t m_channelNumber{0};          ///< Channel Number (1 octet)
        uint16_t m_randomizationInterval{0}; ///< Randomization Interval (2 octets)
        uint16_t m_measurementDuration{0};   ///< Measurement Duration (2 octets)
        uint8_t m_measurementMode{0};        ///< Measurement Mode (1 octet)
        Mac48Address m_bssid;                ///< BSSID (6 octets)

        std::optional<Ssid> m_ssid;                       ///< SSID subelement (ID 0)
        std::optional<BeaconReporting> m_beaconReporting; ///< Reporting subelement (ID 1)
        std::optional<uint8_t> m_reportingDetail;         ///< Reporting Detail subelement (ID 2)
        std::vector<ApChannelReport> m_apChannelReports;  ///< AP Channel Reports (ID 51)
        std::optional<std::vector<uint8_t>>
            m_vendorSpecific; ///< Vendor Specific subelement (ID 221)
    };

    /**
     * @brief Request body for Frame measurement (Figure 9-252)
     */
    class FrameRequestBody
    {
      public:
        /**
         * @brief Subelement IDs for Frame request (Table 9-145)
         */
        enum class SubelementId : uint8_t
        {
            WIDE_BANDWIDTH_CHANNEL_SWITCH = 163,
            VENDOR_SPECIFIC = 221,
        };

        /**
         * @brief Set the Operating Class field.
         * @param operatingClass the operating class
         */
        void SetOperatingClass(uint8_t operatingClass)
        {
            m_operatingClass = operatingClass;
        }

        /**
         * @brief Get the Operating Class field.
         * @return the operating class
         */
        uint8_t GetOperatingClass() const
        {
            return m_operatingClass;
        }

        /**
         * @brief Set the Channel Number field.
         * @param channelNumber the channel number
         */
        void SetChannelNumber(uint8_t channelNumber)
        {
            m_channelNumber = channelNumber;
        }

        /**
         * @brief Get the Channel Number field.
         * @return the channel number
         */
        uint8_t GetChannelNumber() const
        {
            return m_channelNumber;
        }

        /**
         * @brief Set the Randomization Interval field.
         * @param randomizationInterval the randomization interval
         */
        void SetRandomizationInterval(uint16_t randomizationInterval)
        {
            m_randomizationInterval = randomizationInterval;
        }

        /**
         * @brief Get the Randomization Interval field.
         * @return the randomization interval
         */
        uint16_t GetRandomizationInterval() const
        {
            return m_randomizationInterval;
        }

        /**
         * @brief Set the Measurement Duration field.
         * @param measurementDuration the measurement duration
         */
        void SetMeasurementDuration(uint16_t measurementDuration)
        {
            m_measurementDuration = measurementDuration;
        }

        /**
         * @brief Get the Measurement Duration field.
         * @return the measurement duration
         */
        uint16_t GetMeasurementDuration() const
        {
            return m_measurementDuration;
        }

        /**
         * @brief Set the Frame Request Type field.
         * @param frameRequestType the frame request type
         */
        void SetFrameRequestType(uint8_t frameRequestType)
        {
            m_frameRequestType = frameRequestType;
        }

        /**
         * @brief Get the Frame Request Type field.
         * @return the frame request type
         */
        uint8_t GetFrameRequestType() const
        {
            return m_frameRequestType;
        }

        /**
         * @brief Set the MAC Address field.
         * @param macAddress the MAC address
         */
        void SetMacAddress(const Mac48Address& macAddress)
        {
            m_macAddress = macAddress;
        }

        /**
         * @brief Get the MAC Address field.
         * @return the MAC address
         */
        Mac48Address GetMacAddress() const
        {
            return m_macAddress;
        }

        /**
         * @brief Set the Vendor Specific subelement.
         * @param vendorSpecific the vendor specific data
         */
        void SetVendorSpecific(std::vector<uint8_t> vendorSpecific)
        {
            m_vendorSpecific = std::move(vendorSpecific);
        }

        /**
         * @brief Get the Vendor Specific subelement.
         * @return the vendor specific data, or std::nullopt if not present
         */
        const std::optional<std::vector<uint8_t>>& GetVendorSpecific() const
        {
            return m_vendorSpecific;
        }

      private:
        uint8_t m_operatingClass{0};         ///< Operating Class (1 octet)
        uint8_t m_channelNumber{0};          ///< Channel Number (1 octet)
        uint16_t m_randomizationInterval{0}; ///< Randomization Interval (2 octets)
        uint16_t m_measurementDuration{0};   ///< Measurement Duration (2 octets)
        uint8_t m_frameRequestType{0};       ///< Frame Request Type (1 octet)
        Mac48Address m_macAddress;           ///< MAC Address (6 octets)

        std::optional<std::vector<uint8_t>>
            m_vendorSpecific; ///< Vendor Specific subelement (ID 221)
    };

    /**
     * @brief Request body for STA Statistics measurement (Figure 9-253)
     */
    class StaStatisticsRequestBody
    {
      public:
        /**
         * @brief Subelement IDs for STA Statistics request (Table 9-147)
         */
        enum class SubelementId : uint8_t
        {
            TRIGGERED_REPORTING = 1,
            VENDOR_SPECIFIC = 221,
        };

        /**
         * @brief Set the Peer MAC Address field.
         * @param peerMacAddress the peer MAC address
         */
        void SetPeerMacAddress(const Mac48Address& peerMacAddress)
        {
            m_peerMacAddress = peerMacAddress;
        }

        /**
         * @brief Get the Peer MAC Address field.
         * @return the peer MAC address
         */
        Mac48Address GetPeerMacAddress() const
        {
            return m_peerMacAddress;
        }

        /**
         * @brief Set the Randomization Interval field.
         * @param randomizationInterval the randomization interval
         */
        void SetRandomizationInterval(uint16_t randomizationInterval)
        {
            m_randomizationInterval = randomizationInterval;
        }

        /**
         * @brief Get the Randomization Interval field.
         * @return the randomization interval
         */
        uint16_t GetRandomizationInterval() const
        {
            return m_randomizationInterval;
        }

        /**
         * @brief Set the Measurement Duration field.
         * @param measurementDuration the measurement duration
         */
        void SetMeasurementDuration(uint16_t measurementDuration)
        {
            m_measurementDuration = measurementDuration;
        }

        /**
         * @brief Get the Measurement Duration field.
         * @return the measurement duration
         */
        uint16_t GetMeasurementDuration() const
        {
            return m_measurementDuration;
        }

        /**
         * @brief Set the Group Identity field.
         * @param groupIdentity the group identity
         */
        void SetGroupIdentity(uint8_t groupIdentity)
        {
            m_groupIdentity = groupIdentity;
        }

        /**
         * @brief Get the Group Identity field.
         * @return the group identity
         */
        uint8_t GetGroupIdentity() const
        {
            return m_groupIdentity;
        }

        /**
         * @brief Set the Vendor Specific subelement.
         * @param vendorSpecific the vendor specific data
         */
        void SetVendorSpecific(std::vector<uint8_t> vendorSpecific)
        {
            m_vendorSpecific = std::move(vendorSpecific);
        }

        /**
         * @brief Get the Vendor Specific subelement.
         * @return the vendor specific data, or std::nullopt if not present
         */
        const std::optional<std::vector<uint8_t>>& GetVendorSpecific() const
        {
            return m_vendorSpecific;
        }

      private:
        Mac48Address m_peerMacAddress;       ///< Peer MAC Address (6 octets)
        uint16_t m_randomizationInterval{0}; ///< Randomization Interval (2 octets)
        uint16_t m_measurementDuration{0};   ///< Measurement Duration (2 octets)
        uint8_t m_groupIdentity{0};          ///< Group Identity (1 octet)

        std::optional<std::vector<uint8_t>>
            m_vendorSpecific; ///< Vendor Specific subelement (ID 221)
    };

    /**
     * @brief Request body for LCI measurement (Figure 9-260)
     */
    class LciRequestBody
    {
      public:
        /**
         * @brief Subelement IDs for LCI request (Table 9-149)
         */
        enum class SubelementId : uint8_t
        {
            AZIMUTH_REQUEST = 1,
            ORIGINATOR_REQUESTING_STA_MAC_ADDRESS = 2,
            TARGET_MAC_ADDRESS = 3,
            MAXIMUM_AGE = 4,
            VENDOR_SPECIFIC = 221,
        };

        /**
         * @brief Set the Location Subject field.
         * @param locationSubject the location subject
         */
        void SetLocationSubject(uint8_t locationSubject)
        {
            m_locationSubject = locationSubject;
        }

        /**
         * @brief Get the Location Subject field.
         * @return the location subject
         */
        uint8_t GetLocationSubject() const
        {
            return m_locationSubject;
        }

        /**
         * @brief Set the Azimuth Request subelement.
         * @param azimuthRequest the azimuth request
         */
        void SetAzimuthRequest(AzimuthRequest azimuthRequest)
        {
            m_azimuthRequest = std::move(azimuthRequest);
        }

        /**
         * @brief Get the Azimuth Request subelement.
         * @return the azimuth request, or std::nullopt if not present
         */
        std::optional<AzimuthRequest> GetAzimuthRequest() const
        {
            return m_azimuthRequest;
        }

        /**
         * @brief Set the Vendor Specific subelement.
         * @param vendorSpecific the vendor specific data
         */
        void SetVendorSpecific(std::vector<uint8_t> vendorSpecific)
        {
            m_vendorSpecific = std::move(vendorSpecific);
        }

        /**
         * @brief Get the Vendor Specific subelement.
         * @return the vendor specific data, or std::nullopt if not present
         */
        const std::optional<std::vector<uint8_t>>& GetVendorSpecific() const
        {
            return m_vendorSpecific;
        }

      private:
        uint8_t m_locationSubject{0};                   ///< Location Subject (1 octet)
        std::optional<AzimuthRequest> m_azimuthRequest; ///< Azimuth Request subelement (ID 1)
        std::optional<std::vector<uint8_t>>
            m_vendorSpecific; ///< Vendor Specific subelement (ID 221)
    };

    /**
     * @brief Request body for Transmit Stream measurement (Figure 9-266)
     */
    class TransmitStreamRequestBody
    {
      public:
        /**
         * @brief Subelement IDs for Transmit Stream request (Table 9-150)
         */
        enum class SubelementId : uint8_t
        {
            TRIGGERED_REPORTING = 1,
            VENDOR_SPECIFIC = 221,
        };

        /**
         * @brief Set the Randomization Interval field.
         * @param randomizationInterval the randomization interval
         */
        void SetRandomizationInterval(uint16_t randomizationInterval)
        {
            m_randomizationInterval = randomizationInterval;
        }

        /**
         * @brief Get the Randomization Interval field.
         * @return the randomization interval
         */
        uint16_t GetRandomizationInterval() const
        {
            return m_randomizationInterval;
        }

        /**
         * @brief Set the Measurement Duration field.
         * @param measurementDuration the measurement duration
         */
        void SetMeasurementDuration(uint16_t measurementDuration)
        {
            m_measurementDuration = measurementDuration;
        }

        /**
         * @brief Get the Measurement Duration field.
         * @return the measurement duration
         */
        uint16_t GetMeasurementDuration() const
        {
            return m_measurementDuration;
        }

        /**
         * @brief Set the Peer STA Address field.
         * @param peerStaAddress the peer STA MAC address
         */
        void SetPeerStaAddress(const Mac48Address& peerStaAddress)
        {
            m_peerStaAddress = peerStaAddress;
        }

        /**
         * @brief Get the Peer STA Address field.
         * @return the peer STA MAC address
         */
        Mac48Address GetPeerStaAddress() const
        {
            return m_peerStaAddress;
        }

        /**
         * @brief Set the Traffic Identifier field.
         * @param trafficIdentifier the traffic identifier
         */
        void SetTrafficIdentifier(uint8_t trafficIdentifier)
        {
            m_trafficIdentifier = trafficIdentifier;
        }

        /**
         * @brief Get the Traffic Identifier field.
         * @return the traffic identifier
         */
        uint8_t GetTrafficIdentifier() const
        {
            return m_trafficIdentifier;
        }

        /**
         * @brief Set the Bin 0 Range field.
         * @param bin0Range the bin 0 range
         */
        void SetBin0Range(uint8_t bin0Range)
        {
            m_bin0Range = bin0Range;
        }

        /**
         * @brief Get the Bin 0 Range field.
         * @return the bin 0 range
         */
        uint8_t GetBin0Range() const
        {
            return m_bin0Range;
        }

        /**
         * @brief Set the Vendor Specific subelement.
         * @param vendorSpecific the vendor specific data
         */
        void SetVendorSpecific(std::vector<uint8_t> vendorSpecific)
        {
            m_vendorSpecific = std::move(vendorSpecific);
        }

        /**
         * @brief Get the Vendor Specific subelement.
         * @return the vendor specific data, or std::nullopt if not present
         */
        const std::optional<std::vector<uint8_t>>& GetVendorSpecific() const
        {
            return m_vendorSpecific;
        }

      private:
        uint16_t m_randomizationInterval{0}; ///< Randomization Interval (2 octets)
        uint16_t m_measurementDuration{0};   ///< Measurement Duration (2 octets)
        Mac48Address m_peerStaAddress;       ///< Peer STA Address (6 octets)
        uint8_t m_trafficIdentifier{0};      ///< Traffic Identifier (1 octet)
        uint8_t m_bin0Range{0};              ///< Bin 0 Range (1 octet)
        std::optional<std::vector<uint8_t>>
            m_vendorSpecific; ///< Vendor Specific subelement (ID 221)
    };

    /**
     * @brief Request body for Multicast Diagnostics measurement (Figure 9-273)
     */
    class MulticastDiagnosticsRequestBody
    {
      public:
        /**
         * @brief Subelement IDs for Multicast Diagnostics request (Table 9-153)
         */
        enum class SubelementId : uint8_t
        {
            MULTICAST_TRIGGERED_REPORTING = 1,
            VENDOR_SPECIFIC = 221,
        };

        /**
         * @brief Set the Randomization Interval field.
         * @param randomizationInterval the randomization interval
         */
        void SetRandomizationInterval(uint16_t randomizationInterval)
        {
            m_randomizationInterval = randomizationInterval;
        }

        /**
         * @brief Get the Randomization Interval field.
         * @return the randomization interval
         */
        uint16_t GetRandomizationInterval() const
        {
            return m_randomizationInterval;
        }

        /**
         * @brief Set the Measurement Duration field.
         * @param measurementDuration the measurement duration
         */
        void SetMeasurementDuration(uint16_t measurementDuration)
        {
            m_measurementDuration = measurementDuration;
        }

        /**
         * @brief Get the Measurement Duration field.
         * @return the measurement duration
         */
        uint16_t GetMeasurementDuration() const
        {
            return m_measurementDuration;
        }

        /**
         * @brief Set the Group MAC Address field.
         * @param groupMacAddress the group MAC address
         */
        void SetGroupMacAddress(const Mac48Address& groupMacAddress)
        {
            m_groupMacAddress = groupMacAddress;
        }

        /**
         * @brief Get the Group MAC Address field.
         * @return the group MAC address
         */
        Mac48Address GetGroupMacAddress() const
        {
            return m_groupMacAddress;
        }

        /**
         * @brief Set the Vendor Specific subelement.
         * @param vendorSpecific the vendor specific data
         */
        void SetVendorSpecific(std::vector<uint8_t> vendorSpecific)
        {
            m_vendorSpecific = std::move(vendorSpecific);
        }

        /**
         * @brief Get the Vendor Specific subelement.
         * @return the vendor specific data, or std::nullopt if not present
         */
        const std::optional<std::vector<uint8_t>>& GetVendorSpecific() const
        {
            return m_vendorSpecific;
        }

      private:
        uint16_t m_randomizationInterval{0}; ///< Randomization Interval (2 octets)
        uint16_t m_measurementDuration{0};   ///< Measurement Duration (2 octets)
        Mac48Address m_groupMacAddress;      ///< Group MAC Address (6 octets)

        std::optional<std::vector<uint8_t>>
            m_vendorSpecific; ///< Vendor Specific subelement (ID 221)
    };

    /**
     * @brief Request body for Location Civic measurement (Figure 9-276)
     */
    class LocationCivicRequestBody
    {
      public:
        /**
         * @brief Subelement IDs for Location Civic request (Table 9-156)
         */
        enum class SubelementId : uint8_t
        {
            ORIGINATOR_REQUESTING_STA_MAC_ADDRESS = 1,
            TARGET_MAC_ADDRESS = 2,
            VENDOR_SPECIFIC = 221,
        };

        /**
         * @brief Set the Location Subject field.
         * @param locationSubject the location subject
         */
        void SetLocationSubject(uint8_t locationSubject)
        {
            m_locationSubject = locationSubject;
        }

        /**
         * @brief Get the Location Subject field.
         * @return the location subject
         */
        uint8_t GetLocationSubject() const
        {
            return m_locationSubject;
        }

        /**
         * @brief Set the Civic Location Type field.
         * @param civicLocationType the civic location type
         */
        void SetCivicLocationType(uint8_t civicLocationType)
        {
            m_civicLocationType = civicLocationType;
        }

        /**
         * @brief Get the Civic Location Type field.
         * @return the civic location type
         */
        uint8_t GetCivicLocationType() const
        {
            return m_civicLocationType;
        }

        /**
         * @brief Set the Location Service Interval Units field.
         * @param locationServiceIntervalUnits the location service interval units
         */
        void SetLocationServiceIntervalUnits(uint8_t locationServiceIntervalUnits)
        {
            m_locationServiceIntervalUnits = locationServiceIntervalUnits;
        }

        /**
         * @brief Get the Location Service Interval Units field.
         * @return the location service interval units
         */
        uint8_t GetLocationServiceIntervalUnits() const
        {
            return m_locationServiceIntervalUnits;
        }

        /**
         * @brief Set the Location Service Interval field.
         * @param locationServiceInterval the location service interval
         */
        void SetLocationServiceInterval(uint16_t locationServiceInterval)
        {
            m_locationServiceInterval = locationServiceInterval;
        }

        /**
         * @brief Get the Location Service Interval field.
         * @return the location service interval
         */
        uint16_t GetLocationServiceInterval() const
        {
            return m_locationServiceInterval;
        }

        /**
         * @brief Set the Vendor Specific subelement.
         * @param vendorSpecific the vendor specific data
         */
        void SetVendorSpecific(std::vector<uint8_t> vendorSpecific)
        {
            m_vendorSpecific = std::move(vendorSpecific);
        }

        /**
         * @brief Get the Vendor Specific subelement.
         * @return the vendor specific data, or std::nullopt if not present
         */
        const std::optional<std::vector<uint8_t>>& GetVendorSpecific() const
        {
            return m_vendorSpecific;
        }

      private:
        uint8_t m_locationSubject{0};              ///< Location Subject (1 octet)
        uint8_t m_civicLocationType{0};            ///< Civic Location Type (1 octet)
        uint8_t m_locationServiceIntervalUnits{0}; ///< Location Service Interval Units (1 octet)
        uint16_t m_locationServiceInterval{0};     ///< Location Service Interval (2 octets)

        std::optional<std::vector<uint8_t>>
            m_vendorSpecific; ///< Vendor Specific subelement (ID 221)
    };

    /**
     * @brief Request body for Location Identifier measurement (Figure 9-277)
     */
    class LocationIdentifierRequestBody
    {
      public:
        /**
         * @brief Subelement IDs for Location Identifier request (Table 9-157)
         */
        enum class SubelementId : uint8_t
        {
            ORIGINATOR_REQUESTING_STA_MAC_ADDRESS = 1,
            TARGET_MAC_ADDRESS = 2,
            VENDOR_SPECIFIC = 221,
        };

        /**
         * @brief Set the Location Subject field.
         * @param locationSubject the location subject
         */
        void SetLocationSubject(uint8_t locationSubject)
        {
            m_locationSubject = locationSubject;
        }

        /**
         * @brief Get the Location Subject field.
         * @return the location subject
         */
        uint8_t GetLocationSubject() const
        {
            return m_locationSubject;
        }

        /**
         * @brief Set the Location Service Interval Units field.
         * @param locationServiceIntervalUnits the location service interval units
         */
        void SetLocationServiceIntervalUnits(uint8_t locationServiceIntervalUnits)
        {
            m_locationServiceIntervalUnits = locationServiceIntervalUnits;
        }

        /**
         * @brief Get the Location Service Interval Units field.
         * @return the location service interval units
         */
        uint8_t GetLocationServiceIntervalUnits() const
        {
            return m_locationServiceIntervalUnits;
        }

        /**
         * @brief Set the Location Service Interval field.
         * @param locationServiceInterval the location service interval
         */
        void SetLocationServiceInterval(uint16_t locationServiceInterval)
        {
            m_locationServiceInterval = locationServiceInterval;
        }

        /**
         * @brief Get the Location Service Interval field.
         * @return the location service interval
         */
        uint16_t GetLocationServiceInterval() const
        {
            return m_locationServiceInterval;
        }

        /**
         * @brief Set the Vendor Specific subelement.
         * @param vendorSpecific the vendor specific data
         */
        void SetVendorSpecific(std::vector<uint8_t> vendorSpecific)
        {
            m_vendorSpecific = std::move(vendorSpecific);
        }

        /**
         * @brief Get the Vendor Specific subelement.
         * @return the vendor specific data, or std::nullopt if not present
         */
        const std::optional<std::vector<uint8_t>>& GetVendorSpecific() const
        {
            return m_vendorSpecific;
        }

      private:
        uint8_t m_locationSubject{0};              ///< Location Subject (1 octet)
        uint8_t m_locationServiceIntervalUnits{0}; ///< Location Service Interval Units (1 octet)
        uint16_t m_locationServiceInterval{0};     ///< Location Service Interval (2 octets)

        std::optional<std::vector<uint8_t>>
            m_vendorSpecific; ///< Vendor Specific subelement (ID 221)
    };

    /**
     * @brief Request body for Directional Channel Quality measurement (Figure 9-278)
     */
    class DirectionalChannelQualityRequestBody
    {
      public:
        /**
         * @brief Subelement IDs for Directional Channel Quality request (Table 9-158)
         */
        enum class SubelementId : uint8_t
        {
            DIRECTIONAL_CHANNEL_QUALITY_REPORTING = 1,
            MEASUREMENT_CONFIGURATION = 2,
            EXTENDED_MEASUREMENT_CONFIGURATION = 3,
            VENDOR_SPECIFIC = 221,
        };

        /**
         * @brief Set the Operating Class field.
         * @param operatingClass the operating class
         */
        void SetOperatingClass(uint8_t operatingClass)
        {
            m_operatingClass = operatingClass;
        }

        /**
         * @brief Get the Operating Class field.
         * @return the operating class
         */
        uint8_t GetOperatingClass() const
        {
            return m_operatingClass;
        }

        /**
         * @brief Set the Channel Number field.
         * @param channelNumber the channel number
         */
        void SetChannelNumber(uint8_t channelNumber)
        {
            m_channelNumber = channelNumber;
        }

        /**
         * @brief Get the Channel Number field.
         * @return the channel number
         */
        uint8_t GetChannelNumber() const
        {
            return m_channelNumber;
        }

        /**
         * @brief Set the AID field.
         * @param aid the AID
         */
        void SetAid(uint8_t aid)
        {
            m_aid = aid;
        }

        /**
         * @brief Get the AID field.
         * @return the AID
         */
        uint8_t GetAid() const
        {
            return m_aid;
        }

        /**
         * @brief Set the Measurement Method field.
         * @param measurementMethod the measurement method
         */
        void SetMeasurementMethod(uint8_t measurementMethod)
        {
            m_measurementMethod = measurementMethod;
        }

        /**
         * @brief Get the Measurement Method field.
         * @return the measurement method
         */
        uint8_t GetMeasurementMethod() const
        {
            return m_measurementMethod;
        }

        /**
         * @brief Set the Measurement Start Time field.
         * @param measurementStartTime the measurement start time
         */
        void SetMeasurementStartTime(uint64_t measurementStartTime)
        {
            m_measurementStartTime = measurementStartTime;
        }

        /**
         * @brief Get the Measurement Start Time field.
         * @return the measurement start time
         */
        uint64_t GetMeasurementStartTime() const
        {
            return m_measurementStartTime;
        }

        /**
         * @brief Set the Measurement Duration field.
         * @param measurementDuration the measurement duration
         */
        void SetMeasurementDuration(uint16_t measurementDuration)
        {
            m_measurementDuration = measurementDuration;
        }

        /**
         * @brief Get the Measurement Duration field.
         * @return the measurement duration
         */
        uint16_t GetMeasurementDuration() const
        {
            return m_measurementDuration;
        }

        /**
         * @brief Set the Number of Time Blocks field.
         * @param numberOfTimeBlocks the number of time blocks
         */
        void SetNumberOfTimeBlocks(uint8_t numberOfTimeBlocks)
        {
            m_numberOfTimeBlocks = numberOfTimeBlocks;
        }

        /**
         * @brief Get the Number of Time Blocks field.
         * @return the number of time blocks
         */
        uint8_t GetNumberOfTimeBlocks() const
        {
            return m_numberOfTimeBlocks;
        }

        /**
         * @brief Set the Vendor Specific subelement.
         * @param vendorSpecific the vendor specific data
         */
        void SetVendorSpecific(std::vector<uint8_t> vendorSpecific)
        {
            m_vendorSpecific = std::move(vendorSpecific);
        }

        /**
         * @brief Get the Vendor Specific subelement.
         * @return the vendor specific data, or std::nullopt if not present
         */
        const std::optional<std::vector<uint8_t>>& GetVendorSpecific() const
        {
            return m_vendorSpecific;
        }

      private:
        uint8_t m_operatingClass{0};        ///< Operating Class (1 octet)
        uint8_t m_channelNumber{0};         ///< Channel Number (1 octet)
        uint8_t m_aid{0};                   ///< AID (1 octet)
        uint8_t m_measurementMethod{0};     ///< Measurement Method (1 octet)
        uint64_t m_measurementStartTime{0}; ///< Measurement Start Time (8 octets)
        uint16_t m_measurementDuration{0};  ///< Measurement Duration (2 octets)
        uint8_t m_numberOfTimeBlocks{0};    ///< Number of Time Blocks (1 octet)

        std::optional<std::vector<uint8_t>>
            m_vendorSpecific; ///< Vendor Specific subelement (ID 221)
    };

    /**
     * @brief Request body for Directional Measurement (Figure 9-284)
     */
    class DirectionalMeasurementRequestBody
    {
      public:
        /**
         * @brief Subelement IDs for Directional Measurement request (Table 9-160)
         */
        enum class SubelementId : uint8_t
        {
            VENDOR_SPECIFIC = 221,
        };

        /**
         * @brief Set the Operating Class field.
         * @param operatingClass the operating class
         */
        void SetOperatingClass(uint8_t operatingClass)
        {
            m_operatingClass = operatingClass;
        }

        /**
         * @brief Get the Operating Class field.
         * @return the operating class
         */
        uint8_t GetOperatingClass() const
        {
            return m_operatingClass;
        }

        /**
         * @brief Set the Channel Number field.
         * @param channelNumber the channel number
         */
        void SetChannelNumber(uint8_t channelNumber)
        {
            m_channelNumber = channelNumber;
        }

        /**
         * @brief Get the Channel Number field.
         * @return the channel number
         */
        uint8_t GetChannelNumber() const
        {
            return m_channelNumber;
        }

        /**
         * @brief Set the Measurement Start Time field.
         * @param measurementStartTime the measurement start time
         */
        void SetMeasurementStartTime(uint64_t measurementStartTime)
        {
            m_measurementStartTime = measurementStartTime;
        }

        /**
         * @brief Get the Measurement Start Time field.
         * @return the measurement start time
         */
        uint64_t GetMeasurementStartTime() const
        {
            return m_measurementStartTime;
        }

        /**
         * @brief Set the Measurement Duration Per Direction field.
         * @param measurementDurationPerDirection the measurement duration per direction
         */
        void SetMeasurementDurationPerDirection(uint16_t measurementDurationPerDirection)
        {
            m_measurementDurationPerDirection = measurementDurationPerDirection;
        }

        /**
         * @brief Get the Measurement Duration Per Direction field.
         * @return the measurement duration per direction
         */
        uint16_t GetMeasurementDurationPerDirection() const
        {
            return m_measurementDurationPerDirection;
        }

        /**
         * @brief Set the Measurement Method and Antenna Configuration field.
         * @param measurementMethodAndAntennaConfiguration the method and antenna configuration
         */
        void SetMeasurementMethodAndAntennaConfiguration(
            uint8_t measurementMethodAndAntennaConfiguration)
        {
            m_measurementMethodAndAntennaConfiguration = measurementMethodAndAntennaConfiguration;
        }

        /**
         * @brief Get the Measurement Method and Antenna Configuration field.
         * @return the method and antenna configuration
         */
        uint8_t GetMeasurementMethodAndAntennaConfiguration() const
        {
            return m_measurementMethodAndAntennaConfiguration;
        }

        /**
         * @brief Set the Vendor Specific subelement.
         * @param vendorSpecific the vendor specific data
         */
        void SetVendorSpecific(std::vector<uint8_t> vendorSpecific)
        {
            m_vendorSpecific = std::move(vendorSpecific);
        }

        /**
         * @brief Get the Vendor Specific subelement.
         * @return the vendor specific data, or std::nullopt if not present
         */
        const std::optional<std::vector<uint8_t>>& GetVendorSpecific() const
        {
            return m_vendorSpecific;
        }

      private:
        uint8_t m_operatingClass{0};        ///< Operating Class (1 octet)
        uint8_t m_channelNumber{0};         ///< Channel Number (1 octet)
        uint64_t m_measurementStartTime{0}; ///< Measurement Start Time (8 octets)
        uint16_t m_measurementDurationPerDirection{
            0}; ///< Measurement Duration Per Direction (2 octets)
        uint8_t m_measurementMethodAndAntennaConfiguration{
            0}; ///< Method and Antenna Configuration (1 octet)

        std::optional<std::vector<uint8_t>>
            m_vendorSpecific; ///< Vendor Specific subelement (ID 221)
    };

    /**
     * @brief Request body for Directional Statistics measurement (Figure 9-286)
     */
    class DirectionalStatisticsRequestBody
    {
      public:
        /**
         * @brief Subelement IDs for Directional Statistics request (Table 9-161)
         */
        enum class SubelementId : uint8_t
        {
            VENDOR_SPECIFIC = 221,
        };

        /**
         * @brief Set the Operating Class field.
         * @param operatingClass the operating class
         */
        void SetOperatingClass(uint8_t operatingClass)
        {
            m_operatingClass = operatingClass;
        }

        /**
         * @brief Get the Operating Class field.
         * @return the operating class
         */
        uint8_t GetOperatingClass() const
        {
            return m_operatingClass;
        }

        /**
         * @brief Set the Channel Number field.
         * @param channelNumber the channel number
         */
        void SetChannelNumber(uint8_t channelNumber)
        {
            m_channelNumber = channelNumber;
        }

        /**
         * @brief Get the Channel Number field.
         * @return the channel number
         */
        uint8_t GetChannelNumber() const
        {
            return m_channelNumber;
        }

        /**
         * @brief Set the Measurement Start Time field.
         * @param measurementStartTime the measurement start time
         */
        void SetMeasurementStartTime(uint64_t measurementStartTime)
        {
            m_measurementStartTime = measurementStartTime;
        }

        /**
         * @brief Get the Measurement Start Time field.
         * @return the measurement start time
         */
        uint64_t GetMeasurementStartTime() const
        {
            return m_measurementStartTime;
        }

        /**
         * @brief Set the Measurement Duration Per Direction field.
         * @param measurementDurationPerDirection the measurement duration per direction
         */
        void SetMeasurementDurationPerDirection(uint16_t measurementDurationPerDirection)
        {
            m_measurementDurationPerDirection = measurementDurationPerDirection;
        }

        /**
         * @brief Get the Measurement Duration Per Direction field.
         * @return the measurement duration per direction
         */
        uint16_t GetMeasurementDurationPerDirection() const
        {
            return m_measurementDurationPerDirection;
        }

        /**
         * @brief Set the Measurement Method field.
         * @param measurementMethod the measurement method
         */
        void SetMeasurementMethod(uint8_t measurementMethod)
        {
            m_measurementMethod = measurementMethod;
        }

        /**
         * @brief Get the Measurement Method field.
         * @return the measurement method
         */
        uint8_t GetMeasurementMethod() const
        {
            return m_measurementMethod;
        }

        /**
         * @brief Set the Directional Statistics Bitmap field.
         * @param directionalStatisticsBitmap the directional statistics bitmap
         */
        void SetDirectionalStatisticsBitmap(uint8_t directionalStatisticsBitmap)
        {
            m_directionalStatisticsBitmap = directionalStatisticsBitmap;
        }

        /**
         * @brief Get the Directional Statistics Bitmap field.
         * @return the directional statistics bitmap
         */
        uint8_t GetDirectionalStatisticsBitmap() const
        {
            return m_directionalStatisticsBitmap;
        }

        /**
         * @brief Set the Vendor Specific subelement.
         * @param vendorSpecific the vendor specific data
         */
        void SetVendorSpecific(std::vector<uint8_t> vendorSpecific)
        {
            m_vendorSpecific = std::move(vendorSpecific);
        }

        /**
         * @brief Get the Vendor Specific subelement.
         * @return the vendor specific data, or std::nullopt if not present
         */
        const std::optional<std::vector<uint8_t>>& GetVendorSpecific() const
        {
            return m_vendorSpecific;
        }

      private:
        uint8_t m_operatingClass{0};        ///< Operating Class (1 octet)
        uint8_t m_channelNumber{0};         ///< Channel Number (1 octet)
        uint64_t m_measurementStartTime{0}; ///< Measurement Start Time (8 octets)
        uint16_t m_measurementDurationPerDirection{
            0};                                   ///< Measurement Duration Per Direction (2 octets)
        uint8_t m_measurementMethod{0};           ///< Measurement Method (1 octet)
        uint8_t m_directionalStatisticsBitmap{0}; ///< Directional Statistics Bitmap (1 octet)

        std::optional<std::vector<uint8_t>>
            m_vendorSpecific; ///< Vendor Specific subelement (ID 221)
    };

    /**
     * @brief Request body for FTM Range measurement (Figure 9-288)
     */
    class FtmRangeRequestBody
    {
      public:
        /**
         * @brief Subelement IDs for FTM Range request (Table 9-162)
         */
        enum class SubelementId : uint8_t
        {
            MAXIMUM_AGE = 4,
            NEIGHBOR_REPORT = 52,
            VENDOR_SPECIFIC = 221,
        };

        /**
         * @brief Set the Randomization Interval field.
         * @param randomizationInterval the randomization interval
         */
        void SetRandomizationInterval(uint16_t randomizationInterval)
        {
            m_randomizationInterval = randomizationInterval;
        }

        /**
         * @brief Get the Randomization Interval field.
         * @return the randomization interval
         */
        uint16_t GetRandomizationInterval() const
        {
            return m_randomizationInterval;
        }

        /**
         * @brief Set the Minimum AP Count field.
         * @param minimumApCount the minimum AP count
         */
        void SetMinimumApCount(uint8_t minimumApCount)
        {
            m_minimumApCount = minimumApCount;
        }

        /**
         * @brief Get the Minimum AP Count field.
         * @return the minimum AP count
         */
        uint8_t GetMinimumApCount() const
        {
            return m_minimumApCount;
        }

        /**
         * @brief Add a Neighbor Report subelement.
         * @param neighborReport the neighbor report element to add
         */
        void AddNeighborReport(const NeighborReportElement& neighborReport)
        {
            m_neighborReports.push_back(neighborReport);
        }

        /**
         * @brief Get the Neighbor Report subelements.
         * @return the neighbor report elements
         */
        const std::vector<NeighborReportElement>& GetNeighborReports() const
        {
            return m_neighborReports;
        }

        /**
         * @brief Remove all Neighbor Report subelements.
         */
        void ClearNeighborReports()
        {
            m_neighborReports.clear();
        }

        /**
         * @brief Set the Vendor Specific subelement.
         * @param vendorSpecific the vendor specific data
         */
        void SetVendorSpecific(std::vector<uint8_t> vendorSpecific)
        {
            m_vendorSpecific = std::move(vendorSpecific);
        }

        /**
         * @brief Get the Vendor Specific subelement.
         * @return the vendor specific data, or std::nullopt if not present
         */
        const std::optional<std::vector<uint8_t>>& GetVendorSpecific() const
        {
            return m_vendorSpecific;
        }

      private:
        uint16_t m_randomizationInterval{0}; ///< Randomization Interval (2 octets)
        uint8_t m_minimumApCount{0};         ///< Minimum AP Count (1 octet)

        std::vector<NeighborReportElement>
            m_neighborReports; ///< Neighbor Report subelements (ID 52)
        std::optional<std::vector<uint8_t>>
            m_vendorSpecific; ///< Vendor Specific subelement (ID 221)
    };

    /**
     * @brief Request body for Measurement Pause (Figure 9-272)
     */
    class MeasurementPauseRequestBody
    {
      public:
        /**
         * @brief Subelement IDs for Measurement Pause request (Table 9-152)
         */
        enum class SubelementId : uint8_t
        {
            VENDOR_SPECIFIC = 221,
        };

        /**
         * @brief Set the Pause Time field.
         * @param pauseTime the pause time
         */
        void SetPauseTime(uint16_t pauseTime)
        {
            m_pauseTime = pauseTime;
        }

        /**
         * @brief Get the Pause Time field.
         * @return the pause time
         */
        uint16_t GetPauseTime() const
        {
            return m_pauseTime;
        }

        /**
         * @brief Set the Vendor Specific subelement.
         * @param vendorSpecific the vendor specific data
         */
        void SetVendorSpecific(std::vector<uint8_t> vendorSpecific)
        {
            m_vendorSpecific = std::move(vendorSpecific);
        }

        /**
         * @brief Get the Vendor Specific subelement.
         * @return the vendor specific data, or std::nullopt if not present
         */
        const std::optional<std::vector<uint8_t>>& GetVendorSpecific() const
        {
            return m_vendorSpecific;
        }

      private:
        uint16_t m_pauseTime{0}; ///< Pause Time (2 octets)

        std::optional<std::vector<uint8_t>>
            m_vendorSpecific; ///< Vendor Specific subelement (ID 221)
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
    void SetMeasurementType(MeasurementType type);
    /**
     * @brief Get the Measurement Type field.
     * @return the measurement type
     */
    MeasurementType GetMeasurementType() const;

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
    MeasurementType m_measurementType{MeasurementType::BASIC}; //!< Measurement Type (1 octet)
    MeasurementRequestBody m_body;                             //!< Type-specific request body
};

/**
 * @brief Stream insertion for MeasurementRequestElement::MeasurementType.
 * @param os output stream
 * @param type the measurement type
 * @return the output stream
 */
inline std::ostream&
operator<<(std::ostream& os, MeasurementRequestElement::MeasurementType type)
{
    return os << +static_cast<uint8_t>(type);
}

namespace detail
{

/**
 * @brief Map a body struct type to its MeasurementType value.
 *
 * BasicRequestBody is excluded because it is shared by types 0/1/2;
 * the caller must set m_measurementType explicitly for those.
 */
template <typename T>
constexpr MeasurementRequestElement::MeasurementType
MeasurementTypeFor()
{
    using MT = MeasurementRequestElement::MeasurementType;
    if constexpr (std::is_same_v<T, MeasurementRequestElement::ChannelLoadRequestBody>)
    {
        return MT::CHANNEL_LOAD;
    }
    else if constexpr (std::is_same_v<T, MeasurementRequestElement::NoiseHistogramRequestBody>)
    {
        return MT::NOISE_HISTOGRAM;
    }
    else if constexpr (std::is_same_v<T, MeasurementRequestElement::BeaconRequestBody>)
    {
        return MT::BEACON;
    }
    else if constexpr (std::is_same_v<T, MeasurementRequestElement::FrameRequestBody>)
    {
        return MT::FRAME;
    }
    else if constexpr (std::is_same_v<T, MeasurementRequestElement::StaStatisticsRequestBody>)
    {
        return MT::STA_STATISTICS;
    }
    else if constexpr (std::is_same_v<T, MeasurementRequestElement::LciRequestBody>)
    {
        return MT::LCI;
    }
    else if constexpr (std::is_same_v<T, MeasurementRequestElement::TransmitStreamRequestBody>)
    {
        return MT::TRANSMIT_STREAM;
    }
    else if constexpr (std::is_same_v<T,
                                      MeasurementRequestElement::MulticastDiagnosticsRequestBody>)
    {
        return MT::MULTICAST_DIAGNOSTICS;
    }
    else if constexpr (std::is_same_v<T, MeasurementRequestElement::LocationCivicRequestBody>)
    {
        return MT::LOCATION_CIVIC;
    }
    else if constexpr (std::is_same_v<T, MeasurementRequestElement::LocationIdentifierRequestBody>)
    {
        return MT::LOCATION_IDENTIFIER;
    }
    else if constexpr (std::is_same_v<
                           T,
                           MeasurementRequestElement::DirectionalChannelQualityRequestBody>)
    {
        return MT::DIRECTIONAL_CHANNEL_QUALITY;
    }
    else if constexpr (std::is_same_v<T,
                                      MeasurementRequestElement::DirectionalMeasurementRequestBody>)
    {
        return MT::DIRECTIONAL_MEASUREMENT;
    }
    else if constexpr (std::is_same_v<T,
                                      MeasurementRequestElement::DirectionalStatisticsRequestBody>)
    {
        return MT::DIRECTIONAL_STATISTICS;
    }
    else if constexpr (std::is_same_v<T, MeasurementRequestElement::FtmRangeRequestBody>)
    {
        return MT::FTM_RANGE;
    }
    else if constexpr (std::is_same_v<T, MeasurementRequestElement::MeasurementPauseRequestBody>)
    {
        return MT::MEASUREMENT_PAUSE;
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
