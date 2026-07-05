/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#ifndef NTP_HEADER_H
#define NTP_HEADER_H

#include "ns3/header.h"
#include "ns3/nstime.h"

namespace ns3
{

/**
 * @brief NTP-style packet header carrying the four timestamps used to compute clock offset
 * and round-trip delay.
 *
 */
class NtpHeader : public Header
{
  public:
    NtpHeader();

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
     * @brief Mark this header as carrying a server reply rather than a client request.
     * @param isReply true if this is a reply.
     */
    void SetReply(bool isReply);

    /**
     * @brief Check whether this header carries a server reply.
     * @return true if this is a reply.
     */
    bool IsReply() const;

    /**
     * @brief Set T1, the client's local send time.
     * @param t1 the timestamp.
     */
    void SetT1(Time t1);

    /**
     * @brief Get T1, the client's local send time.
     * @return the timestamp.
     */
    Time GetT1() const;

    /**
     * @brief Set T2, the server's local receive time.
     * @param t2 the timestamp.
     */
    void SetT2(Time t2);

    /**
     * @brief Get T2, the server's local receive time.
     * @return the timestamp.
     */
    Time GetT2() const;

    /**
     * @brief Set T3, the server's local send time.
     * @param t3 the timestamp.
     */
    void SetT3(Time t3);

    /**
     * @brief Get T3, the server's local send time.
     * @return the timestamp.
     */
    Time GetT3() const;

  private:
    uint64_t m_t1{0};     //!< Client's local send time, in simulation timesteps
    uint64_t m_t2{0};     //!< Server's local receive time, in simulation timesteps
    uint64_t m_t3{0};     //!< Server's local send time, in simulation timesteps
    uint8_t m_isReply{0}; //!< 1 if this header carries a server reply, 0 for a client request
};

} // namespace ns3

#endif /* NTP_HEADER_H */
