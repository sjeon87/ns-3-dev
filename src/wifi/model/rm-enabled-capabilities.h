/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Devansh Gupta <devanshg308@gmail.com>
 */

#ifndef RM_ENABLED_CAPABILITIES_H
#define RM_ENABLED_CAPABILITIES_H

#include "wifi-information-element.h"

#include <array>

namespace ns3
{

/**
 * @brief RM Enabled Capabilities element (IEEE 802.11-2024 Section 9.4.2.43, IE 70)
 * @ingroup wifi
 *
 * 40-bit capability bitmap (Table 9-218) signaling which radio measurement
 * capabilities a STA supports. Included in Beacon, Probe Response,
 * Association Request/Response, and Reassociation Request/Response frames
 * when dot11RadioMeasurementActivated is true.
 *
 * Layout: Element ID (1) + Length (1) + 5-octet bitmap
 */
class RmEnabledCapabilities : public WifiInformationElement
{
  public:
    WifiInformationElementId ElementId() const override;
    void Print(std::ostream& os) const override;

    /**
     * @brief Set the Link Measurement capability bit (bit 0).
     * @param val true to set, false to clear
     */
    void SetLinkMeasurement(bool val);
    /**
     * @brief Get the Link Measurement capability bit (bit 0).
     * @return true if the capability is set
     */
    bool GetLinkMeasurement() const;

    /**
     * @brief Set the Neighbor Report capability bit (bit 1).
     * @param val true to set, false to clear
     */
    void SetNeighborReport(bool val);
    /**
     * @brief Get the Neighbor Report capability bit (bit 1).
     * @return true if the capability is set
     */
    bool GetNeighborReport() const;

    /**
     * @brief Set the Parallel Measurements capability bit (bit 2).
     * @param val true to set, false to clear
     */
    void SetParallelMeasurements(bool val);
    /**
     * @brief Get the Parallel Measurements capability bit (bit 2).
     * @return true if the capability is set
     */
    bool GetParallelMeasurements() const;

    /**
     * @brief Set the Repeated Measurements capability bit (bit 3).
     * @param val true to set, false to clear
     */
    void SetRepeatedMeasurements(bool val);
    /**
     * @brief Get the Repeated Measurements capability bit (bit 3).
     * @return true if the capability is set
     */
    bool GetRepeatedMeasurements() const;

    /**
     * @brief Set the Beacon Passive Measurement capability bit (bit 4).
     * @param val true to set, false to clear
     */
    void SetBeaconPassiveMeasurement(bool val);
    /**
     * @brief Get the Beacon Passive Measurement capability bit (bit 4).
     * @return true if the capability is set
     */
    bool GetBeaconPassiveMeasurement() const;

    /**
     * @brief Set the Beacon Active Measurement capability bit (bit 5).
     * @param val true to set, false to clear
     */
    void SetBeaconActiveMeasurement(bool val);
    /**
     * @brief Get the Beacon Active Measurement capability bit (bit 5).
     * @return true if the capability is set
     */
    bool GetBeaconActiveMeasurement() const;

    /**
     * @brief Set the Beacon Table Measurement capability bit (bit 6).
     * @param val true to set, false to clear
     */
    void SetBeaconTableMeasurement(bool val);
    /**
     * @brief Get the Beacon Table Measurement capability bit (bit 6).
     * @return true if the capability is set
     */
    bool GetBeaconTableMeasurement() const;

    /**
     * @brief Set the Beacon Measurement Reporting Conditions capability bit (bit 7).
     * @param val true to set, false to clear
     */
    void SetBeaconMeasurementReportingConditions(bool val);
    /**
     * @brief Get the Beacon Measurement Reporting Conditions capability bit (bit 7).
     * @return true if the capability is set
     */
    bool GetBeaconMeasurementReportingConditions() const;

    /**
     * @brief Set the Frame Measurement capability bit (bit 8).
     * @param val true to set, false to clear
     */
    void SetFrameMeasurement(bool val);
    /**
     * @brief Get the Frame Measurement capability bit (bit 8).
     * @return true if the capability is set
     */
    bool GetFrameMeasurement() const;

    /**
     * @brief Set the Channel Load Measurement capability bit (bit 9).
     * @param val true to set, false to clear
     */
    void SetChannelLoadMeasurement(bool val);
    /**
     * @brief Get the Channel Load Measurement capability bit (bit 9).
     * @return true if the capability is set
     */
    bool GetChannelLoadMeasurement() const;

    /**
     * @brief Set the Noise Histogram Measurement capability bit (bit 10).
     * @param val true to set, false to clear
     */
    void SetNoiseHistogramMeasurement(bool val);
    /**
     * @brief Get the Noise Histogram Measurement capability bit (bit 10).
     * @return true if the capability is set
     */
    bool GetNoiseHistogramMeasurement() const;

    /**
     * @brief Set the Statistics Measurement capability bit (bit 11).
     * @param val true to set, false to clear
     */
    void SetStatisticsMeasurement(bool val);
    /**
     * @brief Get the Statistics Measurement capability bit (bit 11).
     * @return true if the capability is set
     */
    bool GetStatisticsMeasurement() const;

    /**
     * @brief Set the LCI Measurement capability bit (bit 12).
     * @param val true to set, false to clear
     */
    void SetLciMeasurement(bool val);
    /**
     * @brief Get the LCI Measurement capability bit (bit 12).
     * @return true if the capability is set
     */
    bool GetLciMeasurement() const;

    /**
     * @brief Set the LCI Azimuth capability bit (bit 13).
     * @param val true to set, false to clear
     */
    void SetLciAzimuth(bool val);
    /**
     * @brief Get the LCI Azimuth capability bit (bit 13).
     * @return true if the capability is set
     */
    bool GetLciAzimuth() const;

    /**
     * @brief Set the Transmit Stream/Category Measurement capability bit (bit 14).
     * @param val true to set, false to clear
     */
    void SetTransmitStreamCategoryMeasurement(bool val);
    /**
     * @brief Get the Transmit Stream/Category Measurement capability bit (bit 14).
     * @return true if the capability is set
     */
    bool GetTransmitStreamCategoryMeasurement() const;

    /**
     * @brief Set the Triggered Transmit Stream/Category Measurement capability bit (bit 15).
     * @param val true to set, false to clear
     */
    void SetTriggeredTransmitStreamCategoryMeasurement(bool val);
    /**
     * @brief Get the Triggered Transmit Stream/Category Measurement capability bit (bit 15).
     * @return true if the capability is set
     */
    bool GetTriggeredTransmitStreamCategoryMeasurement() const;

    /**
     * @brief Set the AP Channel Report capability bit (bit 16).
     * @param val true to set, false to clear
     */
    void SetApChannelReport(bool val);
    /**
     * @brief Get the AP Channel Report capability bit (bit 16).
     * @return true if the capability is set
     */
    bool GetApChannelReport() const;

    /**
     * @brief Set the RM MIB capability bit (bit 17).
     * @param val true to set, false to clear
     */
    void SetRmMib(bool val);
    /**
     * @brief Get the RM MIB capability bit (bit 17).
     * @return true if the capability is set
     */
    bool GetRmMib() const;

    /**
     * @brief Set the Operating Channel Max Measurement Duration field (bits 18-20).
     * @param val 3-bit value (0-7)
     */
    void SetOperatingChannelMaxMeasurementDuration(uint8_t val);
    /**
     * @brief Get the Operating Channel Max Measurement Duration field (bits 18-20).
     * @return 3-bit value (0-7)
     */
    uint8_t GetOperatingChannelMaxMeasurementDuration() const;

    /**
     * @brief Set the Nonoperating Channel Max Measurement Duration field (bits 21-23).
     * @param val 3-bit value (0-7)
     */
    void SetNonoperatingChannelMaxMeasurementDuration(uint8_t val);
    /**
     * @brief Get the Nonoperating Channel Max Measurement Duration field (bits 21-23).
     * @return 3-bit value (0-7)
     */
    uint8_t GetNonoperatingChannelMaxMeasurementDuration() const;

    /**
     * @brief Set the Measurement Pilot Capability field (bits 24-26).
     * @param val 3-bit value (0-7)
     */
    void SetMeasurementPilotCapability(uint8_t val);
    /**
     * @brief Get the Measurement Pilot Capability field (bits 24-26).
     * @return 3-bit value (0-7)
     */
    uint8_t GetMeasurementPilotCapability() const;

    /**
     * @brief Set the Measurement Pilot Transmission Information capability bit (bit 27).
     * @param val true to set, false to clear
     */
    void SetMeasurementPilotTransmissionInformation(bool val);
    /**
     * @brief Get the Measurement Pilot Transmission Information capability bit (bit 27).
     * @return true if the capability is set
     */
    bool GetMeasurementPilotTransmissionInformation() const;

    /**
     * @brief Set the Neighbor Report TSF Offset capability bit (bit 28).
     * @param val true to set, false to clear
     */
    void SetNeighborReportTsfOffset(bool val);
    /**
     * @brief Get the Neighbor Report TSF Offset capability bit (bit 28).
     * @return true if the capability is set
     */
    bool GetNeighborReportTsfOffset() const;

    /**
     * @brief Set the RCPI Measurement capability bit (bit 29).
     * @param val true to set, false to clear
     */
    void SetRcpiMeasurement(bool val);
    /**
     * @brief Get the RCPI Measurement capability bit (bit 29).
     * @return true if the capability is set
     */
    bool GetRcpiMeasurement() const;

    /**
     * @brief Set the RSNI Measurement capability bit (bit 30).
     * @param val true to set, false to clear
     */
    void SetRsniMeasurement(bool val);
    /**
     * @brief Get the RSNI Measurement capability bit (bit 30).
     * @return true if the capability is set
     */
    bool GetRsniMeasurement() const;

    /**
     * @brief Set the BSS Average Access Delay capability bit (bit 31).
     * @param val true to set, false to clear
     */
    void SetBssAverageAccessDelay(bool val);
    /**
     * @brief Get the BSS Average Access Delay capability bit (bit 31).
     * @return true if the capability is set
     */
    bool GetBssAverageAccessDelay() const;

    /**
     * @brief Set the BSS Available Admission Capacity capability bit (bit 32).
     * @param val true to set, false to clear
     */
    void SetBssAvailableAdmissionCapacity(bool val);
    /**
     * @brief Get the BSS Available Admission Capacity capability bit (bit 32).
     * @return true if the capability is set
     */
    bool GetBssAvailableAdmissionCapacity() const;

    /**
     * @brief Set the Antenna capability bit (bit 33).
     * @param val true to set, false to clear
     */
    void SetAntenna(bool val);
    /**
     * @brief Get the Antenna capability bit (bit 33).
     * @return true if the capability is set
     */
    bool GetAntenna() const;

    /**
     * @brief Set the FTM Range Report capability bit (bit 34).
     * @param val true to set, false to clear
     */
    void SetFtmRangeReport(bool val);
    /**
     * @brief Get the FTM Range Report capability bit (bit 34).
     * @return true if the capability is set
     */
    bool GetFtmRangeReport() const;

    /**
     * @brief Set the Civic Location Measurement capability bit (bit 35).
     * @param val true to set, false to clear
     */
    void SetCivicLocationMeasurement(bool val);
    /**
     * @brief Get the Civic Location Measurement capability bit (bit 35).
     * @return true if the capability is set
     */
    bool GetCivicLocationMeasurement() const;

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;

    bool GetBit(uint8_t bitPos) const;
    void SetBit(uint8_t bitPos, bool val);
    uint8_t Get3BitField(uint8_t startBit) const;
    void Set3BitField(uint8_t startBit, uint8_t val);

    std::array<uint8_t, 5> m_capabilities{}; //!< RM Enabled Capabilities bitmap (Table 9-218)
};

} // namespace ns3

#endif // RM_ENABLED_CAPABILITIES_H
