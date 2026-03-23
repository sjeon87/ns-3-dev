/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Devansh Gupta <devanshg308@gmail.com>
 */

#ifndef NEIGHBOR_REPORT_ELEMENT_H
#define NEIGHBOR_REPORT_ELEMENT_H

#include "ht/ht-capabilities.h"
#include "ht/ht-operation.h"
#include "vht/vht-capabilities.h"
#include "vht/vht-operation.h"
#include "wifi-information-element.h"

#include "ns3/mac48-address.h"

#include <array>
#include <optional>
#include <vector>

namespace ns3
{

/**
 * @brief The Neighbor Report element
 * @ingroup wifi
 *
 * This class implements the Neighbor Report element as defined in
 * IEEE 802.11-2024 Section 9.4.2.35.
 *
 * The Neighbor Report element contains information about a neighboring AP,
 * including its BSSID, operating parameters, and capabilities. It is used
 * in Neighbor Report Response frames (802.11k Radio Resource Management).
 *
 * Supports optional subelements (IEEE 802.11-2024 Table 9-212):
 *   - TSF Information (ID 1)
 *   - Condensed Country String (ID 2)
 *   - BSS Transition Candidate Preference (ID 3)
 *   - BSS Termination Duration (ID 4)
 *   - Bearing (ID 5)
 *   - Wide Bandwidth Channel (ID 6)
 *   - HT Capabilities (ID 45)
 *   - HT Operation (ID 61)
 *   - VHT Capabilities (ID 191)
 *   - VHT Operation (ID 192)
 *   - Vendor Specific (ID 221)
 *
 * @todo Add Measurement Report subelement (ID 39, no ns-3 class)
 * @todo Add Secondary Channel Offset subelement (ID 62, no ns-3 class)
 * @todo Add Measurement Pilot Transmission subelement (ID 66, no ns-3 class)
 * @todo Add RM Enabled Capabilities subelement (ID 70, no ns-3 class)
 * @todo Add Multiple BSSID subelement (ID 71, no ns-3 class)
 * @todo Add HE Capabilities subelement (ID 193, awaiting maintainer guidance)
 * @todo Add HE Operation subelement (ID 194, awaiting maintainer guidance)
 * @todo Add BSS Load subelement (ID 195, no ns-3 class)
 * @todo Add HE BSS Load subelement (ID 196, no ns-3 class)
 * @todo Add SSID subelement (ID 197, awaiting maintainer guidance)
 * @todo Add HE 6 GHz Band Capabilities subelement (ID 198, awaiting maintainer guidance)
 */
class NeighborReportElement : public WifiInformationElement
{
  public:
    /**
     * @brief Subelement IDs for Neighbor Report (IEEE 802.11-2024 Table 9-212)
     */
    enum class SubelementId : uint8_t
    {
        TSF_INFORMATION = 1,
        CONDENSED_COUNTRY_STRING = 2,
        BSS_TRANSITION_CANDIDATE_PREFERENCE = 3,
        BSS_TERMINATION_DURATION = 4,
        BEARING = 5,
        WIDE_BANDWIDTH_CHANNEL = 6,
        VENDOR_SPECIFIC = 221,
    };

    NeighborReportElement();

    WifiInformationElementId ElementId() const override;
    void Print(std::ostream& os) const override;

    /**
     * @brief Set the BSSID of the neighboring AP.
     *
     * @param bssid the BSSID
     */
    void SetBssid(Mac48Address bssid);
    /**
     * @brief Get the BSSID of the neighboring AP.
     *
     * @return the BSSID
     */
    Mac48Address GetBssid() const;

    /**
     * @brief Set the BSSID Information field.
     * This 4-octet field contains capability and reachability
     * information about the neighboring AP (see IEEE 802.11-2024
     * Figure 9-417).
     *
     * @param info the BSSID Information field value
     */
    void SetBssidInfo(uint32_t info);
    /**
     * @brief Get the BSSID Information field.
     *
     * @return the BSSID Information field value
     */
    uint32_t GetBssidInfo() const;

    /**
     * @brief Set the AP Reachability sub-field (bits 0-1) of the BSSID Information field.
     * See IEEE 802.11-2024 Figure 9-417.
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
     * See IEEE 802.11-2024 Figure 9-417.
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
     * See IEEE 802.11-2024 Figure 9-417.
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
     * See IEEE 802.11-2024 Figure 9-417.
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
     * See IEEE 802.11-2024 Figure 9-417.
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
     * See IEEE 802.11-2024 Figure 9-417.
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
     * See IEEE 802.11-2024 Figure 9-417.
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
     * @brief Set the Mobility Domain sub-field (bit 10) of the BSSID Information field.
     * See IEEE 802.11-2024 Figure 9-417.
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
     * See IEEE 802.11-2024 Figure 9-417.
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
     * @brief Set the Very High Throughput sub-field (bit 12) of the BSSID Information field.
     * See IEEE 802.11-2024 Figure 9-417.
     *
     * @param vht true to set, false to clear
     */
    void SetVeryHighThroughput(bool vht);
    /**
     * @brief Get the Very High Throughput sub-field (bit 12) of the BSSID Information field.
     *
     * @return the Very High Throughput sub-field value
     */
    bool GetVeryHighThroughput() const;

    /**
     * @brief Set the FTM sub-field (bit 13) of the BSSID Information field.
     * See IEEE 802.11-2024 Figure 9-417.
     *
     * @param ftm true to set, false to clear
     */
    void SetFtm(bool ftm);
    /**
     * @brief Get the FTM sub-field (bit 13) of the BSSID Information field.
     *
     * @return the FTM sub-field value
     */
    bool GetFtm() const;

    /**
     * @brief Set the High Efficiency sub-field (bit 14) of the BSSID Information field.
     * See IEEE 802.11-2024 Figure 9-417.
     *
     * @param he true to set, false to clear
     */
    void SetHighEfficiency(bool he);
    /**
     * @brief Get the High Efficiency sub-field (bit 14) of the BSSID Information field.
     *
     * @return the High Efficiency sub-field value
     */
    bool GetHighEfficiency() const;

    /**
     * @brief Set the ER BSS sub-field (bit 15) of the BSSID Information field.
     * See IEEE 802.11-2024 Figure 9-417.
     *
     * @param erBss true to set, false to clear
     */
    void SetErBss(bool erBss);
    /**
     * @brief Get the ER BSS sub-field (bit 15) of the BSSID Information field.
     *
     * @return the ER BSS sub-field value
     */
    bool GetErBss() const;

    /**
     * @brief Set the Colocated AP sub-field (bit 16) of the BSSID Information field.
     * See IEEE 802.11-2024 Figure 9-417.
     *
     * @param colocatedAp true to set, false to clear
     */
    void SetColocatedAp(bool colocatedAp);
    /**
     * @brief Get the Colocated AP sub-field (bit 16) of the BSSID Information field.
     *
     * @return the Colocated AP sub-field value
     */
    bool GetColocatedAp() const;

    /**
     * @brief Set the Unsolicited Probe Responses Active sub-field (bit 17) of the BSSID
     * Information field. See IEEE 802.11-2024 Figure 9-417.
     *
     * @param active true to set, false to clear
     */
    void SetUnsolicitedProbeResponsesActive(bool active);
    /**
     * @brief Get the Unsolicited Probe Responses Active sub-field (bit 17) of the BSSID
     * Information field.
     *
     * @return the Unsolicited Probe Responses Active sub-field value
     */
    bool GetUnsolicitedProbeResponsesActive() const;

    /**
     * @brief Set the Member of ESS with 2.4/5 GHz Colocated AP sub-field (bit 18) of the
     * BSSID Information field. See IEEE 802.11-2024 Figure 9-417.
     *
     * @param member true to set, false to clear
     */
    void SetMemberOfEssWith2gOr5gColocatedAp(bool member);
    /**
     * @brief Get the Member of ESS with 2.4/5 GHz Colocated AP sub-field (bit 18) of the
     * BSSID Information field.
     *
     * @return the Member of ESS with 2.4/5 GHz Colocated AP sub-field value
     */
    bool GetMemberOfEssWith2gOr5gColocatedAp() const;

    /**
     * @brief Set the OCT Supported with Reporting AP sub-field (bit 19) of the BSSID
     * Information field. See IEEE 802.11-2024 Figure 9-417.
     *
     * @param oct true to set, false to clear
     */
    void SetOctSupportedWithReportingAp(bool oct);
    /**
     * @brief Get the OCT Supported with Reporting AP sub-field (bit 19) of the BSSID
     * Information field.
     *
     * @return the OCT Supported with Reporting AP sub-field value
     */
    bool GetOctSupportedWithReportingAp() const;

    /**
     * @brief Set the Colocated with 6 GHz AP sub-field (bit 20) of the BSSID Information
     * field. See IEEE 802.11-2024 Figure 9-417.
     *
     * @param colocated6g true to set, false to clear
     */
    void SetColocatedWith6gAp(bool colocated6g);
    /**
     * @brief Get the Colocated with 6 GHz AP sub-field (bit 20) of the BSSID Information
     * field.
     *
     * @return the Colocated with 6 GHz AP sub-field value
     */
    bool GetColocatedWith6gAp() const;

    /**
     * @brief Set the DMG Positioning sub-field (bit 22) of the BSSID Information field.
     * See IEEE 802.11-2024 Figure 9-417.
     *
     * @param dmg true to set, false to clear
     */
    void SetDmgPositioning(bool dmg);
    /**
     * @brief Get the DMG Positioning sub-field (bit 22) of the BSSID Information field.
     *
     * @return the DMG Positioning sub-field value
     */
    bool GetDmgPositioning() const;

    /**
     * @brief Set the Operating Class field.
     * Indicates the operating class of the neighboring AP
     * as defined in Annex E of IEEE 802.11-2024.
     *
     * @param operatingClass the operating class
     */
    void SetOperatingClass(uint8_t operatingClass);
    /**
     * @brief Get the Operating Class field.
     *
     * @return the operating class
     */
    uint8_t GetOperatingClass() const;

    /**
     * @brief Set the Channel Number field.
     *
     * @param channel the channel number
     */
    void SetChannelNumber(uint8_t channel);
    /**
     * @brief Get the Channel Number field.
     *
     * @return the channel number
     */
    uint8_t GetChannelNumber() const;

    /**
     * @brief Set the PHY Type field.
     * Indicates the PHY type of the neighboring AP
     * (see IEEE 802.11-2024 Table 9-176).
     *
     * @param phyType the PHY type
     */
    void SetPhyType(uint8_t phyType);
    /**
     * @brief Get the PHY Type field.
     *
     * @return the PHY type
     */
    uint8_t GetPhyType() const;

    /**
     * @brief TSF Information subelement data (IEEE 802.11-2024 Table 9-212, ID 1)
     */
    struct TsfInformation
    {
        uint16_t tsfOffset;      //!< TSF Offset (2 octets)
        uint16_t beaconInterval; //!< Beacon Interval (2 octets)
    };

    /**
     * @brief Bearing subelement data (IEEE 802.11-2024 Figure 9-422, ID 5)
     */
    struct Bearing
    {
        uint16_t bearing;  //!< Bearing in degrees, 0-359 (2 octets)
        uint32_t distance; //!< Distance in meters, IEEE 754 binary32 stored as uint32_t (4 octets).
                           //!< Caller must convert via std::bit_cast<uint32_t>(floatVal).
        int16_t relativeHeight; //!< Relative height in meters, signed (2 octets)
    };

    /**
     * @brief Wide Bandwidth Channel subelement data (IEEE 802.11-2024 Figure 9-423, ID 6)
     */
    struct WideBandwidthChannel
    {
        uint8_t channelWidth;       //!< Channel Width (1 octet)
        uint8_t centerFreqSegment0; //!< Channel Center Frequency Segment 0 (1 octet)
        uint8_t centerFreqSegment1; //!< Channel Center Frequency Segment 1 (1 octet)
    };

    /**
     * @brief BSS Termination Duration subelement data (IEEE 802.11-2024 Table 9-212, ID 4)
     */
    struct BssTerminationDuration
    {
        uint64_t terminationTsf; //!< BSS Termination TSF (8 octets)
        uint16_t duration;       //!< Duration (2 octets)
    };

    /**
     * @brief Set the TSF Information subelement (ID 1).
     * @param tsfOffset the TSF offset
     * @param beaconInterval the beacon interval
     */
    void SetTsfInformation(uint16_t tsfOffset, uint16_t beaconInterval);
    /**
     * @brief Get the TSF Information subelement.
     * @return the TSF Information if present
     */
    std::optional<TsfInformation> GetTsfInformation() const;

    /**
     * @brief Set the Condensed Country String subelement (ID 2).
     * @param c1 first country character
     * @param c2 second country character
     */
    void SetCondensedCountryString(char c1, char c2);
    /**
     * @brief Get the Condensed Country String subelement.
     * @return the two-character country string if present
     */
    std::optional<std::array<char, 2>> GetCondensedCountryString() const;

    /**
     * @brief Set the BSS Transition Candidate Preference subelement (ID 3).
     * @param preference the preference value (0-255)
     */
    void SetCandidatePreference(uint8_t preference);
    /**
     * @brief Get the BSS Transition Candidate Preference subelement.
     * @return the preference value if present
     */
    std::optional<uint8_t> GetCandidatePreference() const;

    /**
     * @brief Set the BSS Termination Duration subelement (ID 4).
     * @param terminationTsf the BSS Termination TSF
     * @param duration the duration in minutes
     */
    void SetBssTerminationDuration(uint64_t terminationTsf, uint16_t duration);
    /**
     * @brief Get the BSS Termination Duration subelement.
     * @return the BSS Termination Duration if present
     */
    std::optional<BssTerminationDuration> GetBssTerminationDuration() const;

    /**
     * @brief Set the Bearing subelement (ID 5).
     * @param bearing direction in degrees (0-359)
     * @param distance distance in meters as IEEE 754 binary32 bits stored in uint32_t
     *        (use std::bit_cast<uint32_t>(floatVal) to convert)
     * @param relativeHeight relative height in meters (signed)
     */
    void SetBearing(uint16_t bearing, uint32_t distance, int16_t relativeHeight);
    /**
     * @brief Get the Bearing subelement.
     * @return the Bearing data if present
     */
    std::optional<Bearing> GetBearing() const;

    /**
     * @brief Set the Wide Bandwidth Channel subelement (ID 6).
     * @param channelWidth the channel width
     * @param centerFreqSegment0 center frequency segment 0
     * @param centerFreqSegment1 center frequency segment 1
     */
    void SetWideBandwidthChannel(uint8_t channelWidth,
                                 uint8_t centerFreqSegment0,
                                 uint8_t centerFreqSegment1);
    /**
     * @brief Get the Wide Bandwidth Channel subelement.
     * @return the Wide Bandwidth Channel data if present
     */
    std::optional<WideBandwidthChannel> GetWideBandwidthChannel() const;

    /**
     * @brief Set the HT Capabilities subelement (ID 45).
     * @param htCapabilities the HT Capabilities element
     */
    void SetHtCapabilities(const HtCapabilities& htCapabilities);
    /**
     * @brief Get the HT Capabilities subelement.
     * @return the HT Capabilities if present
     */
    std::optional<HtCapabilities> GetHtCapabilities() const;

    /**
     * @brief Set the HT Operation subelement (ID 61).
     * @param htOperation the HT Operation element
     */
    void SetHtOperation(const HtOperation& htOperation);
    /**
     * @brief Get the HT Operation subelement.
     * @return the HT Operation if present
     */
    std::optional<HtOperation> GetHtOperation() const;

    /**
     * @brief Set the VHT Capabilities subelement (ID 191).
     * @param vhtCapabilities the VHT Capabilities element
     */
    void SetVhtCapabilities(const VhtCapabilities& vhtCapabilities);
    /**
     * @brief Get the VHT Capabilities subelement.
     * @return the VHT Capabilities if present
     */
    std::optional<VhtCapabilities> GetVhtCapabilities() const;

    /**
     * @brief Set the VHT Operation subelement (ID 192).
     * @param vhtOperation the VHT Operation element
     */
    void SetVhtOperation(const VhtOperation& vhtOperation);
    /**
     * @brief Get the VHT Operation subelement.
     * @return the VHT Operation if present
     */
    std::optional<VhtOperation> GetVhtOperation() const;

    /**
     * @brief Set the Vendor Specific subelement (ID 221).
     * @param data the vendor-specific data
     */
    void SetVendorSpecificData(std::vector<uint8_t> data);
    /**
     * @brief Get the Vendor Specific subelement.
     * @return the vendor-specific data if present
     */
    const std::optional<std::vector<uint8_t>>& GetVendorSpecificData() const;

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;

    Mac48Address m_bssid;     //!< BSSID (6 octets)
    uint32_t m_bssidInfo;     //!< BSSID Information (4 octets)
    uint8_t m_operatingClass; //!< Operating Class (1 octet)
    uint8_t m_channelNumber;  //!< Channel Number (1 octet)
    uint8_t m_phyType;        //!< PHY Type (1 octet)

    std::optional<TsfInformation> m_tsfInfo; //!< TSF Information (ID 1)
    std::optional<std::array<char, 2>>
        m_condensedCountryString;                 //!< Condensed Country String (ID 2)
    std::optional<uint8_t> m_candidatePreference; //!< BSS Transition Candidate Preference (ID 3)
    std::optional<BssTerminationDuration>
        m_bssTerminationDuration;                         //!< BSS Termination Duration (ID 4)
    std::optional<Bearing> m_bearing;                     //!< Bearing (ID 5)
    std::optional<WideBandwidthChannel> m_wideBandwidth;  //!< Wide Bandwidth Channel (ID 6)
    std::optional<HtCapabilities> m_htCapabilities;       //!< HT Capabilities (ID 45)
    std::optional<HtOperation> m_htOperation;             //!< HT Operation (ID 61)
    std::optional<VhtCapabilities> m_vhtCapabilities;     //!< VHT Capabilities (ID 191)
    std::optional<VhtOperation> m_vhtOperation;           //!< VHT Operation (ID 192)
    std::optional<std::vector<uint8_t>> m_vendorSpecific; //!< Vendor Specific (ID 221)
};

} // namespace ns3

#endif /* NEIGHBOR_REPORT_ELEMENT_H */
