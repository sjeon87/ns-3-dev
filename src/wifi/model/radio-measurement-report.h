/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Devansh Gupta <devanshg308@gmail.com>
 */

#ifndef RADIO_MEASUREMENT_REPORT_H
#define RADIO_MEASUREMENT_REPORT_H

#include "measurement-report-element.h"

#include "ns3/header.h"

#include <vector>

namespace ns3
{

/**
 * @brief Radio Measurement Report frame body
 * @ingroup wifi
 *
 * IEEE 802.11-2024 Section 9.6.6.3, Figure 9-1186.
 *
 * Fields after Category + Action (handled by WifiActionHeader):
 * - Dialog Token (1 octet)
 * - Measurement Report Elements (variable, one or more IE 39)
 */
class RadioMeasurementReportHeader : public Header
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
     * @param token the dialog token (matches request token, or 0 for autonomous reports)
     */
    void SetDialogToken(uint8_t token);
    /**
     * @return the dialog token
     */
    uint8_t GetDialogToken() const;

    /**
     * @param elem a Measurement Report element to append
     */
    void AddMeasurementReportElement(const MeasurementReportElement& elem);
    /**
     * @return the vector of Measurement Report elements
     */
    const std::vector<MeasurementReportElement>& GetMeasurementReportElements() const;

  private:
    uint8_t m_dialogToken{0}; //!< Dialog Token (1 octet)
    std::vector<MeasurementReportElement>
        m_measurementReportElements; //!< Measurement Report elements
};

} // namespace ns3

#endif // RADIO_MEASUREMENT_REPORT_H
