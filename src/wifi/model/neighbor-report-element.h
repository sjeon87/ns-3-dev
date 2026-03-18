/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Devansh Gupta <devanshg308@gmail.com>
 */

#ifndef NEIGHBOR_REPORT_ELEMENT_H
#define NEIGHBOR_REPORT_ELEMENT_H

#include "wifi-information-element.h"

#include "ns3/mac48-address.h"

namespace ns3
{

/**
 * @brief The Neighbor Report element
 * @ingroup wifi
 *
 * This class implements the Neighbor Report element as defined in
 * IEEE 802.11-2020 Section 9.4.2.37.
 *
 * The Neighbor Report element contains information about a neighboring AP,
 * including its BSSID, operating parameters, and capabilities. It is used
 * in Neighbor Report Response frames (802.11k Radio Resource Management).
 *
 * @todo Add support for Optional Subelements (IEEE 802.11-2020 Section 9.4.2.37):
 *   - TSF Information (ID 1)
 *   - Condensed Country String (ID 2)
 *   - BSS Transition Candidate Preference (ID 3)
 *   - BSS Termination Duration (ID 4)
 *   - Measurement Pilot Transmission Information (ID 66)
 *   - RRM Enabled Capabilities (ID 70)
 *   - Multiple BSSID (ID 71)
 *   - Vendor Specific (ID 221)
 */
class NeighborReportElement : public WifiInformationElement
{
  public:
    NeighborReportElement();

    WifiInformationElementId ElementId() const override;
    void Print(std::ostream& os) const override;

    /**
     * Set the BSSID of the neighboring AP.
     *
     * @param bssid the BSSID
     */
    void SetBssid(Mac48Address bssid);
    /**
     * Get the BSSID of the neighboring AP.
     *
     * @return the BSSID
     */
    Mac48Address GetBssid() const;

    /**
     * Set the BSSID Information field.
     * This 4-octet field contains capability and reachability
     * information about the neighboring AP (see IEEE 802.11-2020
     * Figure 9-331).
     *
     * @param info the BSSID Information field value
     */
    void SetBssidInfo(uint32_t info);
    /**
     * Get the BSSID Information field.
     *
     * @return the BSSID Information field value
     */
    uint32_t GetBssidInfo() const;

    /**
     * @brief Set the AP Reachability sub-field (bits 0-1) of the BSSID Information field.
     * See IEEE 802.11-2020 Figure 9-331.
     *
     * @param reachability the AP Reachability value (0-3)
     */
    void SetApReachability(uint8_t reachability);
    /**
     * @brief Get the AP Reachability sub-field (bits 0-1) of the BSSID Information field.
     *
     * @return the AP Reachability value (0-3)
     */
    uint8_t GetApReachability() const;

    /**
     * @brief Set the Security sub-field (bit 2) of the BSSID Information field.
     * See IEEE 802.11-2020 Figure 9-331.
     *
     * @param security true to set, false to clear
     */
    void SetSecurity(bool security);
    /**
     * @brief Get the Security sub-field (bit 2) of the BSSID Information field.
     *
     * @return the Security sub-field value
     */
    bool GetSecurity() const;

    /**
     * @brief Set the Key Scope sub-field (bit 3) of the BSSID Information field.
     * See IEEE 802.11-2020 Figure 9-331.
     *
     * @param keyScope true to set, false to clear
     */
    void SetKeyScope(bool keyScope);
    /**
     * @brief Get the Key Scope sub-field (bit 3) of the BSSID Information field.
     *
     * @return the Key Scope sub-field value
     */
    bool GetKeyScope() const;

    /**
     * @brief Set the Spectrum Management sub-field (bit 4) of the BSSID Information field.
     * See IEEE 802.11-2020 Figure 9-331.
     *
     * @param spectrumMgmt true to set, false to clear
     */
    void SetSpectrumManagement(bool spectrumMgmt);
    /**
     * @brief Get the Spectrum Management sub-field (bit 4) of the BSSID Information field.
     *
     * @return the Spectrum Management sub-field value
     */
    bool GetSpectrumManagement() const;

    /**
     * @brief Set the QoS sub-field (bit 5) of the BSSID Information field.
     * See IEEE 802.11-2020 Figure 9-331.
     *
     * @param qos true to set, false to clear
     */
    void SetQos(bool qos);
    /**
     * @brief Get the QoS sub-field (bit 5) of the BSSID Information field.
     *
     * @return the QoS sub-field value
     */
    bool GetQos() const;

    /**
     * @brief Set the APSD sub-field (bit 6) of the BSSID Information field.
     * See IEEE 802.11-2020 Figure 9-331.
     *
     * @param apsd true to set, false to clear
     */
    void SetApsd(bool apsd);
    /**
     * @brief Get the APSD sub-field (bit 6) of the BSSID Information field.
     *
     * @return the APSD sub-field value
     */
    bool GetApsd() const;

    /**
     * @brief Set the Radio Measurement sub-field (bit 7) of the BSSID Information field.
     * See IEEE 802.11-2020 Figure 9-331.
     *
     * @param radioMeasurement true to set, false to clear
     */
    void SetRadioMeasurement(bool radioMeasurement);
    /**
     * @brief Get the Radio Measurement sub-field (bit 7) of the BSSID Information field.
     *
     * @return the Radio Measurement sub-field value
     */
    bool GetRadioMeasurement() const;

    /**
     * @brief Set the Delayed Block Ack sub-field (bit 8) of the BSSID Information field.
     * See IEEE 802.11-2020 Figure 9-331.
     *
     * @param delayedBa true to set, false to clear
     */
    void SetDelayedBlockAck(bool delayedBa);
    /**
     * @brief Get the Delayed Block Ack sub-field (bit 8) of the BSSID Information field.
     *
     * @return the Delayed Block Ack sub-field value
     */
    bool GetDelayedBlockAck() const;

    /**
     * @brief Set the Immediate Block Ack sub-field (bit 9) of the BSSID Information field.
     * See IEEE 802.11-2020 Figure 9-331.
     *
     * @param immediateBa true to set, false to clear
     */
    void SetImmediateBlockAck(bool immediateBa);
    /**
     * @brief Get the Immediate Block Ack sub-field (bit 9) of the BSSID Information field.
     *
     * @return the Immediate Block Ack sub-field value
     */
    bool GetImmediateBlockAck() const;

    /**
     * @brief Set the Mobility Domain sub-field (bit 10) of the BSSID Information field.
     * See IEEE 802.11-2020 Figure 9-331.
     *
     * @param mobilityDomain true to set, false to clear
     */
    void SetMobilityDomain(bool mobilityDomain);
    /**
     * @brief Get the Mobility Domain sub-field (bit 10) of the BSSID Information field.
     *
     * @return the Mobility Domain sub-field value
     */
    bool GetMobilityDomain() const;

    /**
     * @brief Set the High Throughput sub-field (bit 11) of the BSSID Information field.
     * See IEEE 802.11-2020 Figure 9-331.
     *
     * @param ht true to set, false to clear
     */
    void SetHighThroughput(bool ht);
    /**
     * @brief Get the High Throughput sub-field (bit 11) of the BSSID Information field.
     *
     * @return the High Throughput sub-field value
     */
    bool GetHighThroughput() const;

    /**
     * Set the Operating Class field.
     * Indicates the operating class of the neighboring AP
     * as defined in Annex E of IEEE 802.11-2020.
     *
     * @param operatingClass the operating class
     */
    void SetOperatingClass(uint8_t operatingClass);
    /**
     * Get the Operating Class field.
     *
     * @return the operating class
     */
    uint8_t GetOperatingClass() const;

    /**
     * Set the Channel Number field.
     *
     * @param channel the channel number
     */
    void SetChannelNumber(uint8_t channel);
    /**
     * Get the Channel Number field.
     *
     * @return the channel number
     */
    uint8_t GetChannelNumber() const;

    /**
     * Set the PHY Type field.
     * Indicates the PHY type of the neighboring AP
     * (see IEEE 802.11-2020 Table 9-176).
     *
     * @param phyType the PHY type
     */
    void SetPhyType(uint8_t phyType);
    /**
     * Get the PHY Type field.
     *
     * @return the PHY type
     */
    uint8_t GetPhyType() const;

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;

    Mac48Address m_bssid;     //!< BSSID (6 octets)
    uint32_t m_bssidInfo;     //!< BSSID Information (4 octets)
    uint8_t m_operatingClass; //!< Operating Class (1 octet)
    uint8_t m_channelNumber;  //!< Channel Number (1 octet)
    uint8_t m_phyType;        //!< PHY Type (1 octet)
};

} // namespace ns3

#endif /* NEIGHBOR_REPORT_ELEMENT_H */
