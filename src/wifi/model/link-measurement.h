/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Devansh Gupta <devanshg308@gmail.com>
 */

#ifndef LINK_MEASUREMENT_H
#define LINK_MEASUREMENT_H

#include "tpc-report-element.h"

#include "ns3/header.h"

namespace ns3
{

/**
 * @brief Link Measurement Request frame body
 * @ingroup wifi
 *
 * Implements the Link Measurement Request frame body as defined in
 * IEEE 802.11-2024 Section 9.6.6.4, Figure 9-1187.
 *
 * Fields (after Category + Action, handled by WifiActionHeader):
 *   - Dialog Token (1 octet)
 *   - Transmit Power Used (1 octet, signed dBm)
 *   - Max Transmit Power (1 octet, signed dBm)
 *
 * @todo Add optional Extended Link Measurement element (9.4.2.156, DMG-only)
 */
class LinkMeasurementRequestHeader : public Header
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * @brief Set the Dialog Token field.
     * @param token nonzero value identifying the measurement transaction
     */
    void SetDialogToken(uint8_t token);
    /**
     * @brief Get the Dialog Token field.
     * @return the dialog token
     */
    uint8_t GetDialogToken() const;

    /**
     * @brief Set the Transmit Power Used field.
     * @param power transmit power in dBm (signed)
     */
    void SetTransmitPowerUsed(int8_t power);
    /**
     * @brief Get the Transmit Power Used field.
     * @return transmit power used in dBm
     */
    int8_t GetTransmitPowerUsed() const;

    /**
     * @brief Set the Max Transmit Power field.
     * @param power maximum transmit power in dBm (signed)
     */
    void SetMaxTransmitPower(int8_t power);
    /**
     * @brief Get the Max Transmit Power field.
     * @return maximum transmit power in dBm
     */
    int8_t GetMaxTransmitPower() const;

  private:
    uint8_t m_dialogToken{0}; //!< Dialog Token (9.4.1.12)
    int8_t m_txPowerUsed{0};  //!< Transmit Power Used, dBm (9.4.1.20)
    int8_t m_maxTxPower{0};   //!< Max Transmit Power, dBm (9.4.1.19)
};

/**
 * @brief Link Measurement Report frame body
 * @ingroup wifi
 *
 * Implements the Link Measurement Report frame body as defined in
 * IEEE 802.11-2024 Section 9.6.6.5, Figure 9-1188.
 *
 * Fields (after Category + Action, handled by WifiActionHeader):
 *   - Dialog Token (1 octet)
 *   - TPC Report element (4 octets, IE 35)
 *   - Receive Antenna ID (1 octet)
 *   - Transmit Antenna ID (1 octet)
 *   - RCPI (1 octet)
 *   - RSNI (1 octet)
 *
 * @todo Add optional DMG-specific elements (9.4.2.141, 9.4.2.156)
 */
class LinkMeasurementReportHeader : public Header
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * @brief Set the Dialog Token field.
     * @param token nonzero value identifying the measurement transaction
     */
    void SetDialogToken(uint8_t token);
    /**
     * @brief Get the Dialog Token field.
     * @return the dialog token
     */
    uint8_t GetDialogToken() const;

    /**
     * @brief Set the TPC Report element.
     * @param tpc the TPC Report element
     */
    void SetTpcReport(const TpcReportElement& tpc);
    /**
     * @brief Get the TPC Report element.
     * @return the TPC Report element
     */
    const TpcReportElement& GetTpcReport() const;

    /**
     * @brief Set the TPC Report Transmit Power field.
     * @param power transmit power in dBm (signed)
     */
    void SetTpcTransmitPower(int8_t power);
    /**
     * @brief Get the TPC Report Transmit Power field.
     * @return transmit power in dBm
     */
    int8_t GetTpcTransmitPower() const;

    /**
     * @brief Set the TPC Report Link Margin field.
     * @param margin link margin in dB (signed)
     */
    void SetTpcLinkMargin(int8_t margin);
    /**
     * @brief Get the TPC Report Link Margin field.
     * @return link margin in dB
     */
    int8_t GetTpcLinkMargin() const;

    /**
     * @brief Set the Receive Antenna ID field.
     * @param id the receive antenna ID
     */
    void SetRxAntennaId(uint8_t id);
    /**
     * @brief Get the Receive Antenna ID field.
     * @return the receive antenna ID
     */
    uint8_t GetRxAntennaId() const;

    /**
     * @brief Set the Transmit Antenna ID field.
     * @param id the transmit antenna ID
     */
    void SetTxAntennaId(uint8_t id);
    /**
     * @brief Get the Transmit Antenna ID field.
     * @return the transmit antenna ID
     */
    uint8_t GetTxAntennaId() const;

    /**
     * @brief Set the RCPI field.
     * @param rcpi the Received Channel Power Indicator (9.4.2.36)
     */
    void SetRcpi(uint8_t rcpi);
    /**
     * @brief Get the RCPI field.
     * @return the RCPI value
     */
    uint8_t GetRcpi() const;

    /**
     * @brief Set the RSNI field.
     * @param rsni the Received Signal to Noise Indicator (9.4.2.39)
     */
    void SetRsni(uint8_t rsni);
    /**
     * @brief Get the RSNI field.
     * @return the RSNI value
     */
    uint8_t GetRsni() const;

  private:
    uint8_t m_dialogToken{0};     //!< Dialog Token (9.4.1.12)
    TpcReportElement m_tpcReport; //!< TPC Report element (9.4.2.15, IE 35)
    uint8_t m_rxAntennaId{0};     //!< Receive Antenna ID (9.4.2.38)
    uint8_t m_txAntennaId{0};     //!< Transmit Antenna ID (9.4.2.38)
    uint8_t m_rcpi{0};            //!< RCPI (9.4.2.36)
    uint8_t m_rsni{0};            //!< RSNI (9.4.2.39)
};

} // namespace ns3

#endif // LINK_MEASUREMENT_H
