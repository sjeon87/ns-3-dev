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
 * @brief Beacon Report body (IEEE 802.11-2024 Section 9.4.2.20.7, Figure 9-297)
 * @ingroup wifi
 *
 * Fixed fields (26 bytes total). Optional subelements (Table 9-168) are not
 * yet implemented.
 */
class BeaconReport
{
  public:
    /**
     * @brief Get the serialized size of the beacon report body.
     * @return 26 bytes (fixed fields only)
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
     * @return number of bytes read
     */
    uint16_t Deserialize(Buffer::Iterator& start);

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
};

/**
 * @brief Channel Load Report body (IEEE 802.11-2024 Section 9.4.2.20.5, Figure 9-295)
 * @ingroup wifi
 *
 * Fixed fields (13 bytes total). Optional subelements (Table 9-165) are not
 * yet implemented.
 */
class ChannelLoadReport
{
  public:
    /**
     * @brief Get the serialized size of the channel load report body.
     * @return 13 bytes (fixed fields only)
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
     * @return number of bytes read
     */
    uint16_t Deserialize(Buffer::Iterator& start);

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

  private:
    uint8_t m_operatingClass{0};              //!< Operating Class (1 octet)
    uint8_t m_channelNumber{0};               //!< Channel Number (1 octet)
    uint64_t m_actualMeasurementStartTime{0}; //!< Actual Measurement Start Time (8 octets)
    uint16_t m_measurementDuration{0};        //!< Measurement Duration (2 octets)
    uint8_t m_channelLoad{0};                 //!< Channel Load (1 octet)
};

/**
 * @brief Noise Histogram Report body (IEEE 802.11-2024 Section 9.4.2.20.6, Figure 9-296)
 * @ingroup wifi
 *
 * Fixed fields (25 bytes total). Optional subelements (Table 9-167) are not
 * yet implemented.
 */
class NoiseHistogramReport
{
  public:
    /**
     * @brief Get the serialized size of the noise histogram report body.
     * @return 25 bytes (fixed fields only)
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
     * @return number of bytes read
     */
    uint16_t Deserialize(Buffer::Iterator& start);

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

  private:
    uint8_t m_operatingClass{0};              //!< Operating Class (1 octet)
    uint8_t m_channelNumber{0};               //!< Channel Number (1 octet)
    uint64_t m_actualMeasurementStartTime{0}; //!< Actual Measurement Start Time (8 octets)
    uint16_t m_measurementDuration{0};        //!< Measurement Duration (2 octets)
    uint8_t m_antennaId{0};                   //!< Antenna ID (1 octet)
    uint8_t m_anpi{0};                        //!< ANPI (1 octet)
    std::array<uint8_t, 11> m_ipiDensities{}; //!< IPI 0-10 Densities (11 octets)
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
 * Optional subelements (Table 9-171) are not yet implemented.
 */
class StaStatisticsReport
{
  public:
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
     * @return number of bytes read
     */
    uint16_t Deserialize(Buffer::Iterator& start);

    /** @brief Set the Measurement Duration field. @param duration duration in TUs */
    void SetMeasurementDuration(uint16_t duration);
    /** @brief Get the Measurement Duration field. @return duration in TUs */
    uint16_t GetMeasurementDuration() const;

    /** @brief Set the Group Identity field. @param groupIdentity the value (Table 9-170) */
    void SetGroupIdentity(uint8_t groupIdentity);
    /** @brief Get the Group Identity field. @return the value */
    uint8_t GetGroupIdentity() const;

    /** @brief Set the Statistics Group Data field. @param data raw counter bytes */
    void SetStatisticsGroupData(const std::vector<uint8_t>& data);
    /** @brief Get the Statistics Group Data field. @return raw counter bytes */
    const std::vector<uint8_t>& GetStatisticsGroupData() const;

    /**
     * @brief Get the expected Statistics Group Data size for a given Group Identity.
     * @param groupIdentity the Group Identity value
     * @return expected size in bytes, or 0 if reserved/unknown
     */
    static uint16_t GetExpectedGroupDataSize(uint8_t groupIdentity);

  private:
    uint16_t m_measurementDuration{0};          //!< Measurement Duration (2 octets)
    uint8_t m_groupIdentity{0};                 //!< Group Identity (1 octet)
    std::vector<uint8_t> m_statisticsGroupData; //!< Statistics Group Data (variable)
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
    /** @brief Get the Measurement Type field. @return the raw value */
    uint8_t GetMeasurementType() const;

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
    uint8_t m_measurementType{0};       //!< Measurement Type (1 octet)

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
