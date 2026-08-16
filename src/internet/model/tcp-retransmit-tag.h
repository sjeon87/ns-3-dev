/*
 * Copyright (c) 2026 Sergio Andreozzi
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sergio Andreozzi <digitalities@gmail.com>
 */

#ifndef TCP_RETRANSMIT_TAG_H
#define TCP_RETRANSMIT_TAG_H

#include "ns3/packet.h"
#include "ns3/tag.h"

namespace ns3
{

/**
 * @ingroup tcp
 *
 * Tag stamped on retransmitted TCP segments. Stamped by TcpSocketBase
 * before transmit and removed at the receiver. Observers read it via
 * Packet::PeekPacketTag.
 *
 * @note Real networks do not expose per-packet retransmission status on
 * the wire. This tag provides that signal for in-simulation use.
 * Therefore, it can be used for measurement, e.g.: accounting, trace
 * logging, packet capture, or test code. It should not be used for
 * algorithms expected to be deployed in real networks. Some NetDevice
 * models may also remove the tag, for example under L2 header
 * compression, although IP fragmentation preserves it.
 */
class TcpRetransmitTag : public Tag
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    TcpRetransmitTag();

    /**
     * @brief Constructor with explicit retransmission attempt count.
     * @param retxCount retransmission attempt index (1 = first retry)
     */
    explicit TcpRetransmitTag(uint8_t retxCount);

    uint32_t GetSerializedSize() const override;
    void Serialize(TagBuffer i) const override;
    void Deserialize(TagBuffer i) override;
    void Print(std::ostream& os) const override;

    /**
     * @brief Always true; presence of the tag is the signal.
     * @return true
     */
    bool IsRetransmit() const
    {
        return true;
    }

    /**
     * @brief Get the retransmission attempt count.
     * @return retransmission attempt index (1 = first retry)
     */
    uint8_t GetRetxCount() const
    {
        return m_retxCount;
    }

  private:
    uint8_t m_retxCount; //!< retransmission attempt count (1 = first retry)
};

} // namespace ns3

#endif // TCP_RETRANSMIT_TAG_H
