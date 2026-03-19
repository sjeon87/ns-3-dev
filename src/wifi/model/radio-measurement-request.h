/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Devansh Gupta <devanshg308@gmail.com>
 */

#ifndef RADIO_MEASUREMENT_REQUEST_H
#define RADIO_MEASUREMENT_REQUEST_H

#include "measurement-request-element.h"

#include "ns3/header.h"

#include <vector>

namespace ns3
{

/**
 * @brief Radio Measurement Request frame body
 * @ingroup wifi
 *
 * IEEE 802.11-2024 Section 9.6.6.2, Figure 9-1185.
 *
 * Fields after Category + Action (handled by WifiActionHeader):
 * - Dialog Token (1 octet)
 * - Number of Repetitions (2 octets)
 * - Measurement Request Elements (variable, zero or more IE 38)
 */
class RadioMeasurementRequestHeader : public Header
{
  public:
    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * @param token the dialog token (nonzero, chosen by sender)
     */
    void SetDialogToken(uint8_t token);
    /**
     * @return the dialog token
     */
    uint8_t GetDialogToken() const;

    /**
     * @param repetitions the number of repetitions (0 = once, 65535 = until cancelled)
     */
    void SetNumberOfRepetitions(uint16_t repetitions);
    /**
     * @return the number of repetitions
     */
    uint16_t GetNumberOfRepetitions() const;

    /**
     * @param elem a Measurement Request element to append
     */
    void AddMeasurementRequestElement(const MeasurementRequestElement& elem);
    /**
     * @return the vector of Measurement Request elements
     */
    const std::vector<MeasurementRequestElement>& GetMeasurementRequestElements() const;

  private:
    uint8_t m_dialogToken{0};          //!< Dialog Token (1 octet)
    uint16_t m_numberOfRepetitions{0}; //!< Number of Repetitions (2 octets)
    std::vector<MeasurementRequestElement>
        m_measurementRequestElements; //!< Measurement Request elements
};

} // namespace ns3

#endif // RADIO_MEASUREMENT_REQUEST_H
