/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Devansh Gupta <devanshg308@gmail.com>
 */

#ifndef LINK_MEASUREMENT_H
#define LINK_MEASUREMENT_H

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

} // namespace ns3

#endif // LINK_MEASUREMENT_H
