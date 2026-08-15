/*
 * Copyright (c) 2026 Somaiya Vidyavihar University, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manmita Das <das.manmita12@gmail.com>
 */

#ifndef ETHERNET_FLOW_CONTROL_HEADER_H
#define ETHERNET_FLOW_CONTROL_HEADER_H

#include "ns3/header.h"

namespace ns3
{

namespace ethernet
{
/**
 * @ingroup ethernet
 * @brief Ethernet flow control (PAUSE) frame header.
 *
 * EthernetFlowControlHeader models the IEEE 802.3x MAC Control PAUSE
 * frame used for link-level flow control in full-duplex Ethernet.
 */
class EthernetFlowControlHeader : public Header
{
  public:
    /**
     * @brief Get the type ID
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Get the instance type ID
     * @return the object TypeId
     */
    TypeId GetInstanceTypeId() const override;

    EthernetFlowControlHeader();

    ~EthernetFlowControlHeader() override;

    /**
     * @brief Set the pause quanta for the flow control frame
     * @param pauseQuanta the pause quanta to set where each quantum is equal to 512 bit times.
     */
    void SetPauseQuanta(uint16_t pauseQuanta);

    /**
     * @brief Get the pause quanta for the flow control frame
     * @return the pause quanta
     */
    uint16_t GetPauseQuanta() const;

    // Header overrides
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    /**
     * @brief The pause quanta for the flow control frame
     */
    uint16_t m_pauseQuanta;
};
} // namespace ethernet
} // namespace ns3

#endif // ETHERNET_FLOW_CONTROL_HEADER_H
