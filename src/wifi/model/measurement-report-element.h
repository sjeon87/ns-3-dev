/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Devansh Gupta <devanshg308@gmail.com>
 */

#ifndef MEASUREMENT_REPORT_ELEMENT_H
#define MEASUREMENT_REPORT_ELEMENT_H

#include "wifi-information-element.h"

#include "ns3/mac48-address.h"

#include <array>
#include <optional>
#include <ostream>
#include <variant>
#include <vector>

namespace ns3
{

/**
 * @brief Measurement Report Type values (IEEE 802.11-2024 Table 9-163)
 * @ingroup wifi
 */
enum class MeasurementReportType : uint8_t
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
    TRANSMIT_STREAM_CATEGORY = 9,
    MULTICAST_DIAGNOSTICS = 10,
    LOCATION_CIVIC = 11,
    LOCATION_IDENTIFIER = 12,
    DIRECTIONAL_CHANNEL_QUALITY = 13,
    DIRECTIONAL_MEASUREMENT = 14,
    DIRECTIONAL_STATISTICS = 15,
    FTM_RANGE = 16,
};

/**
 * @brief Stream insertion for MeasurementReportType.
 * @param os output stream
 * @param type the measurement report type
 * @return the output stream
 */
inline std::ostream&
operator<<(std::ostream& os, MeasurementReportType type)
{
    return os << +static_cast<uint8_t>(type);
}

/**
 * @brief Beacon Report body (IEEE 802.11-2024 Section 9.4.2.20.7, Figure 9-297)
 * @ingroup wifi
 *
 * Fixed fields (26 bytes) plus optional subelements (Table 9-168).
 */
class BeaconReport
{
  public:
    static constexpr uint16_t SERIALIZED_SIZE = 26; ///< Fixed fields size in bytes

    /**
     * @brief Subelement IDs for Beacon Report (IEEE 802.11-2024 Table 9-168)
     */
    enum class SubelementId : uint8_t
    {
        REPORTED_FRAME_BODY = 1,
        REPORTED_FRAME_BODY_FRAGMENT_ID = 2,
        WIDE_BANDWIDTH_CHANNEL_SWITCH = 163,
        LAST_BEACON_REPORT_INDICATION = 164,
        VENDOR_SPECIFIC = 221,
    };

    /**
     * @brief Reported Frame Body Fragment ID subelement (ID 2, Figure 9-1077)
     */
    struct ReportedFrameBodyFragmentId
    {
        uint8_t beaconReportId{0};          ///< Beacon Report ID (B0-B7)
        uint8_t fragmentIdNumber{0};        ///< Fragment ID Number (B8-B14, 7 bits)
        bool moreFrameBodyFragments{false}; ///< More Frame Body Fragments (B15)
    };

    /**
     * @brief Wide Bandwidth Channel Switch subelement (ID 163)
     */
    struct WideBandwidthChannelSwitch
    {
        uint8_t newChannelWidth{0};          ///< New Channel Width
        uint8_t newChannelCenterFreqSeg0{0}; ///< New Channel Center Frequency Segment 0
        uint8_t newChannelCenterFreqSeg1{0}; ///< New Channel Center Frequency Segment 1
    };

    /**
     * @brief Get the serialized size of the beacon report body.
     * @return size in bytes (fixed fields plus any present subelements)
     */
    uint16_t GetSerializedSize() const;

    /**
     * @brief Serialize the beacon report body.
     * @param start the buffer iterator
     */
    void Serialize(Buffer::Iterator& start) const;

    /**
     * @brief Deserialize the beacon report body.
     * @param start the buffer iterator
     * @param length total bytes available for the report body
     * @return number of bytes read
     */
    uint16_t Deserialize(Buffer::Iterator& start, uint16_t length);

    /** @brief Set the Operating Class field. @param operatingClass the value */
    void SetOperatingClass(uint8_t operatingClass);
    /** @brief Get the Operating Class field. @return the value */
    uint8_t GetOperatingClass() const;

    /** @brief Set the Channel Number field. @param channel the value */
    void SetChannelNumber(uint8_t channel);
    /** @brief Get the Channel Number field. @return the value */
    uint8_t GetChannelNumber() const;

    /** @brief Set the Actual Measurement Start Time field. @param startTime the TSF value */
    void SetActualMeasurementStartTime(uint64_t startTime);
    /** @brief Get the Actual Measurement Start Time field. @return the TSF value */
    uint64_t GetActualMeasurementStartTime() const;

    /** @brief Set the Measurement Duration field. @param duration duration in TUs */
    void SetMeasurementDuration(uint16_t duration);
    /** @brief Get the Measurement Duration field. @return duration in TUs */
    uint16_t GetMeasurementDuration() const;

    /**
     * @brief Set the Reported Frame Information field.
     * B0-B6 = Condensed PHY Type, B7 = Reported Frame Type.
     * @param condensedPhyType PHY type (0-127)
     * @param reportedFrameType true if Measurement Pilot, false if Beacon/Probe Response
     */
    void SetReportedFrameInformation(uint8_t condensedPhyType, bool reportedFrameType);
    /** @brief Get the Condensed PHY Type sub-field (B0-B6). @return value 0-127 */
    uint8_t GetCondensedPhyType() const;
    /** @brief Get the Reported Frame Type sub-field (B7). @return the value */
    bool GetReportedFrameType() const;

    /** @brief Set the RCPI field. @param rcpi the value */
    void SetRcpi(uint8_t rcpi);
    /** @brief Get the RCPI field. @return the value */
    uint8_t GetRcpi() const;

    /** @brief Set the RSNI field. @param rsni the value */
    void SetRsni(uint8_t rsni);
    /** @brief Get the RSNI field. @return the value */
    uint8_t GetRsni() const;

    /** @brief Set the BSSID field. @param bssid the MAC address */
    void SetBssid(Mac48Address bssid);
    /** @brief Get the BSSID field. @return the MAC address */
    Mac48Address GetBssid() const;

    /** @brief Set the Antenna ID field. @param antennaId the value */
    void SetAntennaId(uint8_t antennaId);
    /** @brief Get the Antenna ID field. @return the value */
    uint8_t GetAntennaId() const;

    /** @brief Set the Parent TSF field. @param parentTsf the value */
    void SetParentTsf(uint32_t parentTsf);
    /** @brief Get the Parent TSF field. @return the value */
    uint32_t GetParentTsf() const;

    /**
     * @brief Set the Reported Frame Body subelement (ID 1).
     * @param body raw bytes of the reported frame body
     */
    void SetReportedFrameBody(const std::vector<uint8_t>& body);
    /**
     * @brief Get the Reported Frame Body subelement (ID 1).
     * @return raw bytes, or std::nullopt if not present
     */
    std::optional<std::vector<uint8_t>> GetReportedFrameBody() const;

    /**
     * @brief Set the Reported Frame Body Fragment ID subelement (ID 2).
     * @param fragId the fragment ID fields
     */
    void SetReportedFrameBodyFragmentId(const ReportedFrameBodyFragmentId& fragId);
    /**
     * @brief Get the Reported Frame Body Fragment ID subelement (ID 2).
     * @return the fragment ID fields, or std::nullopt if not present
     */
    std::optional<ReportedFrameBodyFragmentId> GetReportedFrameBodyFragmentId() const;

    /**
     * @brief Set the Wide Bandwidth Channel Switch subelement (ID 163).
     * @param wbc the channel switch fields
     */
    void SetWideBandwidthChannelSwitch(const WideBandwidthChannelSwitch& wbc);
    /**
     * @brief Get the Wide Bandwidth Channel Switch subelement (ID 163).
     * @return the channel switch fields, or std::nullopt if not present
     */
    std::optional<WideBandwidthChannelSwitch> GetWideBandwidthChannelSwitch() const;

    /**
     * @brief Set the Last Beacon Report Indication subelement (ID 164).
     * @param indication true if this is the last beacon report
     */
    void SetLastBeaconReportIndication(bool indication);
    /**
     * @brief Get the Last Beacon Report Indication subelement (ID 164).
     * @return the indication value, or std::nullopt if not present
     */
    std::optional<bool> GetLastBeaconReportIndication() const;

    /**
     * @brief Set the Vendor Specific subelement (ID 221).
     * @param data raw vendor specific bytes
     */
    void SetVendorSpecific(const std::vector<uint8_t>& data);
    /**
     * @brief Get the Vendor Specific subelement (ID 221).
     * @return raw bytes, or std::nullopt if not present
     */
    std::optional<std::vector<uint8_t>> GetVendorSpecific() const;

  private:
    uint8_t m_operatingClass{0};              //!< Operating Class (1 octet)
    uint8_t m_channelNumber{0};               //!< Channel Number (1 octet)
    uint64_t m_actualMeasurementStartTime{0}; //!< Actual Measurement Start Time (8 octets)
    uint16_t m_measurementDuration{0};        //!< Measurement Duration (2 octets)
    uint8_t m_reportedFrameInfo{0};           //!< Reported Frame Information (1 octet)
    uint8_t m_rcpi{0};                        //!< RCPI (1 octet)
    uint8_t m_rsni{0};                        //!< RSNI (1 octet)
    Mac48Address m_bssid;                     //!< BSSID (6 octets)
    uint8_t m_antennaId{0};                   //!< Antenna ID (1 octet)
    uint32_t m_parentTsf{0};                  //!< Parent TSF (4 octets)

    std::optional<std::vector<uint8_t>> m_reportedFrameBody; //!< Reported Frame Body (ID 1)
    std::optional<ReportedFrameBodyFragmentId>
        m_reportedFrameBodyFragmentId;                                      //!< Fragment ID (ID 2)
    std::optional<WideBandwidthChannelSwitch> m_wideBandwidthChannelSwitch; //!< WBC (ID 163)
    std::optional<bool> m_lastBeaconReportIndication; //!< Last Beacon Report Indication (ID 164)
    std::optional<std::vector<uint8_t>> m_vendorSpecific; //!< Vendor Specific (ID 221)
};

/**
 * @brief Channel Load Report body (IEEE 802.11-2024 Section 9.4.2.20.5, Figure 9-295)
 * @ingroup wifi
 *
 * Fixed fields (13 bytes) plus optional subelements (Table 9-165):
 * Wide Bandwidth Channel Switch (ID 163) and Vendor Specific (ID 221).
 */
class ChannelLoadReport
{
  public:
    static constexpr uint16_t FIXED_FIELDS_SIZE = 13; ///< Fixed fields size in bytes

    /**
     * @brief Subelement IDs for Channel Load Report (IEEE 802.11-2024 Table 9-165)
     */
    enum class SubelementId : uint8_t
    {
        WIDE_BANDWIDTH_CHANNEL_SWITCH = 163,
        VENDOR_SPECIFIC = 221,
    };

    /**
     * @brief Get the serialized size of the channel load report body.
     * @return size in bytes (fixed fields plus any present subelements)
     */
    uint16_t GetSerializedSize() const;

    /**
     * @brief Serialize the channel load report body.
     * @param start the buffer iterator
     */
    void Serialize(Buffer::Iterator& start) const;

    /**
     * @brief Deserialize the channel load report body.
     * @param start the buffer iterator
     * @param length total bytes available for the report body
     * @return number of bytes read
     */
    uint16_t Deserialize(Buffer::Iterator& start, uint16_t length);

    /** @brief Set the Operating Class field. @param operatingClass the value */
    void SetOperatingClass(uint8_t operatingClass);
    /** @brief Get the Operating Class field. @return the value */
    uint8_t GetOperatingClass() const;

    /** @brief Set the Channel Number field. @param channel the value */
    void SetChannelNumber(uint8_t channel);
    /** @brief Get the Channel Number field. @return the value */
    uint8_t GetChannelNumber() const;

    /** @brief Set the Actual Measurement Start Time field. @param startTime the TSF value */
    void SetActualMeasurementStartTime(uint64_t startTime);
    /** @brief Get the Actual Measurement Start Time field. @return the TSF value */
    uint64_t GetActualMeasurementStartTime() const;

    /** @brief Set the Measurement Duration field. @param duration duration in TUs */
    void SetMeasurementDuration(uint16_t duration);
    /** @brief Get the Measurement Duration field. @return duration in TUs */
    uint16_t GetMeasurementDuration() const;

    /** @brief Set the Channel Load field. @param load channel load value (0-255) */
    void SetChannelLoad(uint8_t load);
    /** @brief Get the Channel Load field. @return channel load value (0-255) */
    uint8_t GetChannelLoad() const;

    /**
     * @brief Set the Wide Bandwidth Channel Switch subelement (ID 163).
     * @param wbc the channel switch fields
     */
    void SetWideBandwidthChannelSwitch(const BeaconReport::WideBandwidthChannelSwitch& wbc);
    /**
     * @brief Get the Wide Bandwidth Channel Switch subelement (ID 163).
     * @return the channel switch fields, or std::nullopt if not present
     */
    std::optional<BeaconReport::WideBandwidthChannelSwitch> GetWideBandwidthChannelSwitch() const;

    /**
     * @brief Set the Vendor Specific subelement (ID 221).
     * @param data raw vendor specific bytes
     */
    void SetVendorSpecific(const std::vector<uint8_t>& data);
    /**
     * @brief Get the Vendor Specific subelement (ID 221).
     * @return raw bytes, or std::nullopt if not present
     */
    std::optional<std::vector<uint8_t>> GetVendorSpecific() const;

  private:
    uint8_t m_operatingClass{0};              //!< Operating Class (1 octet)
    uint8_t m_channelNumber{0};               //!< Channel Number (1 octet)
    uint64_t m_actualMeasurementStartTime{0}; //!< Actual Measurement Start Time (8 octets)
    uint16_t m_measurementDuration{0};        //!< Measurement Duration (2 octets)
    uint8_t m_channelLoad{0};                 //!< Channel Load (1 octet)

    std::optional<BeaconReport::WideBandwidthChannelSwitch>
        m_wideBandwidthChannelSwitch;                     //!< WBC subelement (ID 163)
    std::optional<std::vector<uint8_t>> m_vendorSpecific; //!< Vendor Specific subelement (ID 221)
};

/**
 * @brief Noise Histogram Report body (IEEE 802.11-2024 Section 9.4.2.20.6, Figure 9-296)
 * @ingroup wifi
 *
 * Fixed fields (25 bytes) plus optional subelements (Table 9-167):
 * Wide Bandwidth Channel Switch (ID 163) and Vendor Specific (ID 221).
 */
class NoiseHistogramReport
{
  public:
    static constexpr uint16_t FIXED_FIELDS_SIZE = 25; ///< Fixed fields size in bytes

    /**
     * @brief Subelement IDs for Noise Histogram Report (IEEE 802.11-2024 Table 9-167)
     */
    enum class SubelementId : uint8_t
    {
        WIDE_BANDWIDTH_CHANNEL_SWITCH = 163,
        VENDOR_SPECIFIC = 221,
    };

    /**
     * @brief Get the serialized size of the noise histogram report body.
     * @return size in bytes (fixed fields plus any present subelements)
     */
    uint16_t GetSerializedSize() const;

    /**
     * @brief Serialize the noise histogram report body.
     * @param start the buffer iterator
     */
    void Serialize(Buffer::Iterator& start) const;

    /**
     * @brief Deserialize the noise histogram report body.
     * @param start the buffer iterator
     * @param length total bytes available for the report body
     * @return number of bytes read
     */
    uint16_t Deserialize(Buffer::Iterator& start, uint16_t length);

    /** @brief Set the Operating Class field. @param operatingClass the value */
    void SetOperatingClass(uint8_t operatingClass);
    /** @brief Get the Operating Class field. @return the value */
    uint8_t GetOperatingClass() const;

    /** @brief Set the Channel Number field. @param channel the value */
    void SetChannelNumber(uint8_t channel);
    /** @brief Get the Channel Number field. @return the value */
    uint8_t GetChannelNumber() const;

    /** @brief Set the Actual Measurement Start Time field. @param startTime the TSF value */
    void SetActualMeasurementStartTime(uint64_t startTime);
    /** @brief Get the Actual Measurement Start Time field. @return the TSF value */
    uint64_t GetActualMeasurementStartTime() const;

    /** @brief Set the Measurement Duration field. @param duration duration in TUs */
    void SetMeasurementDuration(uint16_t duration);
    /** @brief Get the Measurement Duration field. @return duration in TUs */
    uint16_t GetMeasurementDuration() const;

    /** @brief Set the Antenna ID field. @param antennaId the value */
    void SetAntennaId(uint8_t antennaId);
    /** @brief Get the Antenna ID field. @return the value */
    uint8_t GetAntennaId() const;

    /** @brief Set the ANPI field. @param anpi the value */
    void SetAnpi(uint8_t anpi);
    /** @brief Get the ANPI field. @return the value */
    uint8_t GetAnpi() const;

    /**
     * @brief Set an IPI density value.
     * @param level the IPI level (0-10, per Table 9-166)
     * @param density the density value
     */
    void SetIpiDensity(uint8_t level, uint8_t density);
    /**
     * @brief Get an IPI density value.
     * @param level the IPI level (0-10, per Table 9-166)
     * @return the density value
     */
    uint8_t GetIpiDensity(uint8_t level) const;

    /**
     * @brief Set the Wide Bandwidth Channel Switch subelement (ID 163).
     * @param wbc the channel switch fields
     */
    void SetWideBandwidthChannelSwitch(const BeaconReport::WideBandwidthChannelSwitch& wbc);
    /**
     * @brief Get the Wide Bandwidth Channel Switch subelement (ID 163).
     * @return the channel switch fields, or std::nullopt if not present
     */
    std::optional<BeaconReport::WideBandwidthChannelSwitch> GetWideBandwidthChannelSwitch() const;

    /**
     * @brief Set the Vendor Specific subelement (ID 221).
     * @param data raw vendor specific bytes
     */
    void SetVendorSpecific(const std::vector<uint8_t>& data);
    /**
     * @brief Get the Vendor Specific subelement (ID 221).
     * @return raw bytes, or std::nullopt if not present
     */
    std::optional<std::vector<uint8_t>> GetVendorSpecific() const;

  private:
    uint8_t m_operatingClass{0};              //!< Operating Class (1 octet)
    uint8_t m_channelNumber{0};               //!< Channel Number (1 octet)
    uint64_t m_actualMeasurementStartTime{0}; //!< Actual Measurement Start Time (8 octets)
    uint16_t m_measurementDuration{0};        //!< Measurement Duration (2 octets)
    uint8_t m_antennaId{0};                   //!< Antenna ID (1 octet)
    uint8_t m_anpi{0};                        //!< ANPI (1 octet)
    std::array<uint8_t, 11> m_ipiDensities{}; //!< IPI 0-10 Densities (11 octets)

    std::optional<BeaconReport::WideBandwidthChannelSwitch>
        m_wideBandwidthChannelSwitch;                     //!< WBC subelement (ID 163)
    std::optional<std::vector<uint8_t>> m_vendorSpecific; //!< Vendor Specific subelement (ID 221)
};

/**
 * @brief Frame Report Entry (IEEE 802.11-2024 Figure 9-302)
 * @ingroup wifi
 *
 * Each entry is 19 bytes and describes frames received from a single transmitter.
 */
class FrameReportEntry
{
  public:
    static constexpr uint16_t SERIALIZED_SIZE = 19; ///< Fixed entry size in bytes

    /**
     * @brief Get the serialized size of a frame report entry.
     * @return 19 bytes
     */
    uint16_t GetSerializedSize() const;

    /**
     * @brief Serialize the frame report entry.
     * @param start the buffer iterator
     */
    void Serialize(Buffer::Iterator& start) const;

    /**
     * @brief Deserialize the frame report entry.
     * @param start the buffer iterator
     * @return number of bytes read
     */
    uint16_t Deserialize(Buffer::Iterator& start);

    /** @brief Set the Transmitter Address field. @param addr the MAC address */
    void SetTransmitterAddress(Mac48Address addr);
    /** @brief Get the Transmitter Address field. @return the MAC address */
    Mac48Address GetTransmitterAddress() const;

    /** @brief Set the BSSID field. @param bssid the MAC address */
    void SetBssid(Mac48Address bssid);
    /** @brief Get the BSSID field. @return the MAC address */
    Mac48Address GetBssid() const;

    /** @brief Set the PHY Type field. @param phyType the value */
    void SetPhyType(uint8_t phyType);
    /** @brief Get the PHY Type field. @return the value */
    uint8_t GetPhyType() const;

    /** @brief Set the Average RCPI field. @param rcpi the value */
    void SetAverageRcpi(uint8_t rcpi);
    /** @brief Get the Average RCPI field. @return the value */
    uint8_t GetAverageRcpi() const;

    /** @brief Set the Last RSNI field. @param rsni the value */
    void SetLastRsni(uint8_t rsni);
    /** @brief Get the Last RSNI field. @return the value */
    uint8_t GetLastRsni() const;

    /** @brief Set the Last RCPI field. @param rcpi the value */
    void SetLastRcpi(uint8_t rcpi);
    /** @brief Get the Last RCPI field. @return the value */
    uint8_t GetLastRcpi() const;

    /** @brief Set the Antenna ID field. @param antennaId the value */
    void SetAntennaId(uint8_t antennaId);
    /** @brief Get the Antenna ID field. @return the value */
    uint8_t GetAntennaId() const;

    /** @brief Set the Frame Count field. @param count the value */
    void SetFrameCount(uint16_t count);
    /** @brief Get the Frame Count field. @return the value */
    uint16_t GetFrameCount() const;

  private:
    Mac48Address m_transmitterAddress; //!< Transmitter Address (6 octets)
    Mac48Address m_bssid;              //!< BSSID (6 octets)
    uint8_t m_phyType{0};              //!< PHY Type (1 octet)
    uint8_t m_averageRcpi{0};          //!< Average RCPI (1 octet)
    uint8_t m_lastRsni{0};             //!< Last RSNI (1 octet)
    uint8_t m_lastRcpi{0};             //!< Last RCPI (1 octet)
    uint8_t m_antennaId{0};            //!< Antenna ID (1 octet)
    uint16_t m_frameCount{0};          //!< Frame Count (2 octets)
};

/**
 * @brief Frame Report body (IEEE 802.11-2024 Section 9.4.2.20.8, Figure 9-300)
 * @ingroup wifi
 *
 * Fixed fields (12 bytes) plus an optional Frame Count Report subelement
 * (ID 1, Table 9-169) containing zero or more FrameReportEntry fields.
 */
class FrameReport
{
  public:
    static constexpr uint16_t FIXED_FIELDS_SIZE = 12; ///< Fixed fields size in bytes

    /**
     * @brief Subelement IDs for Frame Report (IEEE 802.11-2024 Table 9-169)
     */
    enum SubelementId : uint8_t
    {
        FRAME_COUNT_REPORT = 1,
    };

    /**
     * @brief Get the serialized size of the frame report body.
     * @return size in bytes
     */
    uint16_t GetSerializedSize() const;

    /**
     * @brief Serialize the frame report body.
     * @param start the buffer iterator
     */
    void Serialize(Buffer::Iterator& start) const;

    /**
     * @brief Deserialize the frame report body.
     * @param start the buffer iterator
     * @param length total bytes available for the report body
     * @return number of bytes read
     */
    uint16_t Deserialize(Buffer::Iterator& start, uint16_t length);

    /** @brief Set the Operating Class field. @param operatingClass the value */
    void SetOperatingClass(uint8_t operatingClass);
    /** @brief Get the Operating Class field. @return the value */
    uint8_t GetOperatingClass() const;

    /** @brief Set the Channel Number field. @param channel the value */
    void SetChannelNumber(uint8_t channel);
    /** @brief Get the Channel Number field. @return the value */
    uint8_t GetChannelNumber() const;

    /** @brief Set the Actual Measurement Start Time field. @param startTime the TSF value */
    void SetActualMeasurementStartTime(uint64_t startTime);
    /** @brief Get the Actual Measurement Start Time field. @return the TSF value */
    uint64_t GetActualMeasurementStartTime() const;

    /** @brief Set the Measurement Duration field. @param duration duration in TUs */
    void SetMeasurementDuration(uint16_t duration);
    /** @brief Get the Measurement Duration field. @return duration in TUs */
    uint16_t GetMeasurementDuration() const;

    /** @brief Add a Frame Report Entry. @param entry the entry to add */
    void AddFrameReportEntry(const FrameReportEntry& entry);
    /** @brief Get the Frame Report Entries. @return vector of entries */
    const std::vector<FrameReportEntry>& GetFrameReportEntries() const;

  private:
    uint8_t m_operatingClass{0};              //!< Operating Class (1 octet)
    uint8_t m_channelNumber{0};               //!< Channel Number (1 octet)
    uint64_t m_actualMeasurementStartTime{0}; //!< Actual Measurement Start Time (8 octets)
    uint16_t m_measurementDuration{0};        //!< Measurement Duration (2 octets)
    std::vector<FrameReportEntry> m_frameReportEntries; //!< Frame Report Entries
};

/**
 * @brief STA Statistics Report body (IEEE 802.11-2024 Section 9.4.2.20.9, Figure 9-303)
 * @ingroup wifi
 *
 * Fixed fields: Measurement Duration (2 octets), Group Identity (1 octet),
 * Statistics Group Data (variable, size determined by Group Identity per Table 9-170).
 * Optional subelements (Table 9-171): Reporting Reason (ID 1) and Vendor Specific (ID 221).
 */
class StaStatisticsReport
{
  public:
    /**
     * @brief Subelement IDs for STA Statistics Report (IEEE 802.11-2024 Table 9-171)
     */
    enum class SubelementId : uint8_t
    {
        REPORTING_REASON = 1,
        VENDOR_SPECIFIC = 221,
    };

    /**
     * @brief dot11CountersTable (Figure 9-304, 7 x Counter32 = 28 bytes)
     */
    struct Group0Data
    {
        uint32_t transmittedFragmentCount{0};   ///< TransmittedFragmentCount
        uint32_t groupTransmittedFrameCount{0}; ///< GroupTransmittedFrameCount
        uint32_t failedCount{0};                ///< FailedCount
        uint32_t receivedFragmentCount{0};      ///< ReceivedFragmentCount
        uint32_t groupReceivedFrameCount{0};    ///< GroupReceivedFrameCount
        uint32_t fcsErrorCount{0};              ///< FCSErrorCount
        uint32_t transmittedFrameCount{0};      ///< TransmittedFrameCount
    };

    /**
     * @brief dot11MACStatistics (Figure 9-305, 6 x Counter32 = 24 bytes)
     */
    struct Group1Data
    {
        uint32_t retryCount{0};          ///< RetryCount
        uint32_t multipleRetryCount{0};  ///< MultipleRetryCount
        uint32_t frameDuplicateCount{0}; ///< FrameDuplicateCount
        uint32_t rtsSuccessCount{0};     ///< RTSSuccessCount
        uint32_t rtsFailureCount{0};     ///< RTSFailureCount
        uint32_t ackFailureCount{0};     ///< ACKFailureCount
    };

    /**
     * @brief dot11BSSAverageAccessDelay (Figure 9-307, 8 bytes)
     */
    struct Group10Data
    {
        uint8_t apAverageAccessDelay{0};         ///< AP Average Access Delay
        uint8_t averageAccessDelayBestEffort{0}; ///< Average Access Delay Best Effort
        uint8_t averageAccessDelayBackGround{0}; ///< Average Access Delay Background
        uint8_t averageAccessDelayVideo{0};      ///< Average Access Delay Video
        uint8_t averageAccessDelayVoice{0};      ///< Average Access Delay Voice
        uint16_t stationCount{0};                ///< Station Count
        uint8_t channelUtilization{0};           ///< Channel Utilization
    };

    /**
     * @brief Get the serialized size of the STA Statistics report body.
     * @return size in bytes (3 + statistics group data length)
     */
    uint16_t GetSerializedSize() const;

    /**
     * @brief Serialize the STA Statistics report body.
     * @param start the buffer iterator
     */
    void Serialize(Buffer::Iterator& start) const;

    /**
     * @brief Deserialize the STA Statistics report body.
     * @param start the buffer iterator
     * @param length total bytes available for the report body
     * @return number of bytes read
     */
    uint16_t Deserialize(Buffer::Iterator& start, uint16_t length);

    /** @brief Set the Measurement Duration field. @param duration duration in TUs */
    void SetMeasurementDuration(uint16_t duration);
    /** @brief Get the Measurement Duration field. @return duration in TUs */
    uint16_t GetMeasurementDuration() const;

    /** @brief Get the Group Identity field. @return the value (set implicitly by Set*Data) */
    uint8_t GetGroupIdentity() const;

    /** @brief Set Group 0 (dot11CountersTable) data. @param data the counter values */
    void SetGroup0Data(const Group0Data& data);
    /**
     * @brief Get Group 0 (dot11CountersTable) data.
     * @return the counter values, or std::nullopt if group identity is not 0
     */
    std::optional<Group0Data> GetGroup0Data() const;

    /** @brief Set Group 1 (dot11MACStatistics) data. @param data the counter values */
    void SetGroup1Data(const Group1Data& data);
    /**
     * @brief Get Group 1 (dot11MACStatistics) data.
     * @return the counter values, or std::nullopt if group identity is not 1
     */
    std::optional<Group1Data> GetGroup1Data() const;

    /** @brief Set Group 10 (dot11BSSAverageAccessDelay) data. @param data the field values */
    void SetGroup10Data(const Group10Data& data);
    /**
     * @brief Get Group 10 (dot11BSSAverageAccessDelay) data.
     * @return the field values, or std::nullopt if group identity is not 10
     */
    std::optional<Group10Data> GetGroup10Data() const;

    /**
     * @brief Get the expected Statistics Group Data size for a given Group Identity.
     * @param groupIdentity the Group Identity value
     * @return expected size in bytes, or 0 if reserved/unknown
     */
    static uint16_t GetExpectedGroupDataSize(uint8_t groupIdentity);

    /**
     * @brief Set the Reporting Reason subelement (ID 1).
     *
     * The 1-byte Data field encodes which triggered condition caused the report.
     * Set to 0 for non-triggered reports. The bit layout depends on Group Identity
     * (Figures 9-309, 9-310, 9-311).
     * @param reason the reporting reason byte
     */
    void SetReportingReason(uint8_t reason);
    /**
     * @brief Get the Reporting Reason subelement (ID 1).
     * @return the reason byte, or std::nullopt if not present
     */
    std::optional<uint8_t> GetReportingReason() const;

    /**
     * @brief Set the Vendor Specific subelement (ID 221).
     * @param data raw vendor specific bytes
     */
    void SetVendorSpecific(const std::vector<uint8_t>& data);
    /**
     * @brief Get the Vendor Specific subelement (ID 221).
     * @return raw bytes, or std::nullopt if not present
     */
    std::optional<std::vector<uint8_t>> GetVendorSpecific() const;

  private:
    uint16_t m_measurementDuration{0};          //!< Measurement Duration (2 octets)
    uint8_t m_groupIdentity{0};                 //!< Group Identity (1 octet)
    std::vector<uint8_t> m_statisticsGroupData; //!< Statistics Group Data (variable)

    std::optional<uint8_t> m_reportingReason;             //!< Reporting Reason subelement (ID 1)
    std::optional<std::vector<uint8_t>> m_vendorSpecific; //!< Vendor Specific subelement (ID 221)
};

/**
 * @brief The Measurement Report element (IEEE 802.11-2024 Section 9.4.2.20, IE 39)
 * @ingroup wifi
 *
 * Carries measurement results. The report body type is determined by
 * the Measurement Type field. Currently Beacon (type 5), Channel Load
 * (type 3), Noise Histogram (type 4), Frame (type 6), and STA Statistics
 * (type 7) reports are supported. Unsupported types or mode-bit-set reports have no body.
 */
class MeasurementReportElement : public WifiInformationElement
{
  public:
    WifiInformationElementId ElementId() const override;
    void Print(std::ostream& os) const override;

    /** @brief Set the Measurement Token field. @param token the value */
    void SetMeasurementToken(uint8_t token);
    /** @brief Get the Measurement Token field. @return the value */
    uint8_t GetMeasurementToken() const;

    /** @brief Set the Late bit (B0) of the Measurement Report Mode field. @param late the value */
    void SetLate(bool late);
    /** @brief Get the Late bit (B0). @return the value */
    bool GetLate() const;

    /** @brief Set the Incapable bit (B1) of the Measurement Report Mode field. @param incapable the
     * value */
    void SetIncapable(bool incapable);
    /** @brief Get the Incapable bit (B1). @return the value */
    bool GetIncapable() const;

    /** @brief Set the Refused bit (B2) of the Measurement Report Mode field. @param refused the
     * value */
    void SetRefused(bool refused);
    /** @brief Get the Refused bit (B2). @return the value */
    bool GetRefused() const;

    /** @brief Set the Measurement Type field. @param type the MeasurementReportType */
    void SetMeasurementType(MeasurementReportType type);
    /** @brief Get the Measurement Type field. @return the measurement type */
    MeasurementReportType GetMeasurementType() const;

    /**
     * @brief Set the Beacon Report body.
     * Also sets Measurement Type to BEACON.
     * @param report the BeaconReport
     */
    void SetBeaconReport(const BeaconReport& report);
    /**
     * @brief Get the Beacon Report body if present.
     * @return the BeaconReport if type is BEACON and no mode bits are set
     */
    std::optional<BeaconReport> GetBeaconReport() const;

    /**
     * @brief Set the Channel Load Report body.
     * Also sets Measurement Type to CHANNEL_LOAD.
     * @param report the ChannelLoadReport
     */
    void SetChannelLoadReport(const ChannelLoadReport& report);
    /**
     * @brief Get the Channel Load Report body if present.
     * @return the ChannelLoadReport if type is CHANNEL_LOAD and no mode bits are set
     */
    std::optional<ChannelLoadReport> GetChannelLoadReport() const;

    /**
     * @brief Set the Noise Histogram Report body.
     * Also sets Measurement Type to NOISE_HISTOGRAM.
     * @param report the NoiseHistogramReport
     */
    void SetNoiseHistogramReport(const NoiseHistogramReport& report);
    /**
     * @brief Get the Noise Histogram Report body if present.
     * @return the NoiseHistogramReport if type is NOISE_HISTOGRAM and no mode bits are set
     */
    std::optional<NoiseHistogramReport> GetNoiseHistogramReport() const;

    /**
     * @brief Set the STA Statistics Report body.
     * Also sets Measurement Type to STA_STATISTICS.
     * @param report the StaStatisticsReport
     */
    void SetStaStatisticsReport(const StaStatisticsReport& report);
    /**
     * @brief Get the STA Statistics Report body if present.
     * @return the StaStatisticsReport if type is STA_STATISTICS and no mode bits are set
     */
    std::optional<StaStatisticsReport> GetStaStatisticsReport() const;

    /**
     * @brief Set the Frame Report body.
     * Also sets Measurement Type to FRAME.
     * @param report the FrameReport
     */
    void SetFrameReport(const FrameReport& report);
    /**
     * @brief Get the Frame Report body if present.
     * @return the FrameReport if type is FRAME and no mode bits are set
     */
    std::optional<FrameReport> GetFrameReport() const;

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;

    /**
     * @brief Check if any mode bit is set (Late, Incapable, or Refused).
     * @return true if any mode bit is set
     */
    bool HasModeSet() const;

    uint8_t m_measurementToken{0};      //!< Measurement Token (1 octet)
    uint8_t m_measurementReportMode{0}; //!< Measurement Report Mode (1 octet)
    MeasurementReportType m_measurementType{
        MeasurementReportType::BASIC}; //!< Measurement Type (1 octet)

    std::variant<std::monostate,
                 BeaconReport,
                 ChannelLoadReport,
                 NoiseHistogramReport,
                 FrameReport,
                 StaStatisticsReport>
        m_report; //!< Report body
};

} // namespace ns3

#endif /* MEASUREMENT_REPORT_ELEMENT_H */
