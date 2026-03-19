/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Devansh Gupta <devanshg308@gmail.com>
 */

#ifndef NEIGHBOR_REPORT_H
#define NEIGHBOR_REPORT_H

#include "neighbor-report-element.h"
#include "ssid.h"

#include "ns3/header.h"

#include <optional>
#include <vector>

namespace ns3
{

/**
 * @brief Neighbor Report Request frame body
 * @ingroup wifi
 *
 * Implements the Neighbor Report Request frame body as defined in
 * IEEE 802.11-2024 Section 9.6.6.6, Figure 9-1189.
 *
 * Fields (after Category + Action, handled by WifiActionHeader):
 *   - Dialog Token (1 octet)
 *   - SSID (variable, optional)
 *
 * @todo Add optional LCI Measurement Request element (IE 38, Type=LCI)
 * @todo Add optional Location Civic Measurement Request element (IE 38, Type=Location Civic)
 * @todo Add optional Neighbor DMG Request element (IE 38, Type=Neighboring DMG APs)
 */
class NeighborReportRequestHeader : public Header
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
     * @brief Set the optional SSID element.
     * @param ssid the SSID to include in the request
     */
    void SetSsid(const Ssid& ssid);
    /**
     * @brief Get the optional SSID element.
     * @return the SSID if present
     */
    const std::optional<Ssid>& GetSsid() const;
    /**
     * @brief Check whether an SSID element is present.
     * @return true if SSID is set
     */
    bool HasSsid() const;

  private:
    uint8_t m_dialogToken{0};   //!< Dialog Token (9.4.1.12)
    std::optional<Ssid> m_ssid; //!< Optional SSID element
};

/**
 * @brief Neighbor Report Response frame body
 * @ingroup wifi
 *
 * Implements the Neighbor Report Response frame body as defined in
 * IEEE 802.11-2024 Section 9.6.6.7, Figure 9-1190.
 *
 * Fields (after Category + Action, handled by WifiActionHeader):
 *   - Dialog Token (1 octet)
 *   - Neighbor Report Elements (variable, zero or more IE 52 instances)
 */
class NeighborReportResponseHeader : public Header
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
     * @param token value from corresponding request, or 0 if unsolicited
     */
    void SetDialogToken(uint8_t token);
    /**
     * @brief Get the Dialog Token field.
     * @return the dialog token
     */
    uint8_t GetDialogToken() const;

    /**
     * @brief Append a Neighbor Report element.
     * @param elem the Neighbor Report element to add
     */
    void AddNeighborReportElement(const NeighborReportElement& elem);
    /**
     * @brief Get the list of Neighbor Report elements.
     * @return const reference to the vector of elements
     */
    const std::vector<NeighborReportElement>& GetNeighborReportElements() const;

  private:
    uint8_t m_dialogToken{0};                                    //!< Dialog Token (9.4.1.12)
    std::vector<NeighborReportElement> m_neighborReportElements; //!< Neighbor Report elements
};

} // namespace ns3

#endif // NEIGHBOR_REPORT_H
