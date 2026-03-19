/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Devansh Gupta <devanshg308@gmail.com>
 */

#ifndef TPC_REPORT_ELEMENT_H
#define TPC_REPORT_ELEMENT_H

#include "wifi-information-element.h"

namespace ns3
{

/**
 * @brief TPC Report element (IEEE 802.11-2024 Section 9.4.2.15, IE 35)
 * @ingroup wifi
 *
 * Layout: Element ID (1) + Length (1) + Transmit Power (1) + Link Margin (1)
 *
 * Used in Link Measurement Report frames, TPC Report frames,
 * and Beacon/Probe Response frames.
 */
class TpcReportElement : public WifiInformationElement
{
  public:
    WifiInformationElementId ElementId() const override;
    void Print(std::ostream& os) const override;

    /**
     * @brief Set the Transmit Power field.
     * @param power transmit power in dBm (signed, 2s complement)
     */
    void SetTransmitPower(int8_t power);
    /**
     * @brief Get the Transmit Power field.
     * @return transmit power in dBm
     */
    int8_t GetTransmitPower() const;

    /**
     * @brief Set the Link Margin field.
     * @param margin link margin in dB (signed, 2s complement)
     */
    void SetLinkMargin(int8_t margin);
    /**
     * @brief Get the Link Margin field.
     * @return link margin in dB
     */
    int8_t GetLinkMargin() const;

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;

    int8_t m_transmitPower{0}; //!< Transmit Power, dBm (9.4.2.15)
    int8_t m_linkMargin{0};    //!< Link Margin, dB (9.4.2.15)
};

} // namespace ns3

#endif // TPC_REPORT_ELEMENT_H
