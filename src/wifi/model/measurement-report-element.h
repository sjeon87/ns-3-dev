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

#include <optional>
#include <variant>

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
 * @brief The Measurement Report element (IEEE 802.11-2024 Section 9.4.2.20, IE 39)
 * @ingroup wifi
 *
 * Carries measurement results. The report body type is determined by
 * the Measurement Type field. Currently only Beacon reports (type 5)
 * are supported. Unsupported types or mode-bit-set reports have no body.
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

    std::variant<std::monostate, BeaconReport> m_report; //!< Report body
};

} // namespace ns3

#endif /* MEASUREMENT_REPORT_ELEMENT_H */
