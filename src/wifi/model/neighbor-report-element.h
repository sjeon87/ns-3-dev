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
