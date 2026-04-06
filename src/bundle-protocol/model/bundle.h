/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#ifndef BUNDLE_H
#define BUNDLE_H

#include "bundle-header.h"

#include "ns3/address.h"
#include "ns3/buffer.h"
#include "ns3/object.h"
#include "ns3/type-id.h"

#include <vector>

namespace ns3
{

/**
 *
 * @ingroup dtn
 *
 * @brief An implementation of a bundle for Bpv7
 *
 * Each bundle stores a set of blocks. However in this implementation,
 * I'm going to assume that these blocks are just buffers. I can't figure out what
 * the difference between a block and buffer would be apart from the CBOR encoding
 * in RFC 9171, which I'm not sure yet is relevant to a simulation for complexity.
 * For now, I'm just going to dump all the blocks into a buffer vector and then
 * convert them to packets that I send to the TCP/UDP/LTP layers through CLAs.
 *
 */
class Bundle : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    Bundle();
    ~Bundle() override;

    /**
     * @brief Get the instance type ID.
     * @return the instance TypeId
     */
    TypeId GetInstanceTypeId() const override;

    /**
     * @brief set the primary block header for the bundle
     * @param h the primary block header
     */
    void SetPrimaryHeader(PrimaryBlockHeader h);

    /**
     * @brief Gets the currently attached primary
     * block header
     * @return PrimaryBlockHeader
     */
    PrimaryBlockHeader GetPrimaryHeader() const;

    /**
     * @brief set the primary block header for the bundle
     * @param h the primary block header
     */
    void SetPayloadHeader(PayloadBlockHeader h);

    /**
     * @brief get the payload block header for the bundle
     * @return PayloadBlockHeader
     */
    PayloadBlockHeader GetPayloadHeader() const;

    /**
     * @brief set the payload for the bundle
     * @param payload the payload for the bundle
     */
    void SetPayload(Ptr<Packet> payload);

    /**
     * @brief Get the payload for the bundle
     * @return the payload for the bundle
     */
    Ptr<Packet> GetPayload() const;

    /**
     * @brief Get the total size of the bundle
     * @return the total size for the bundle
     */
    uint32_t GetTotalSize() const;

    /**
     * @brief Serialize bundle into a packet
     * @return the packet holding the serialization of the bundle
     */
    Ptr<Packet> Serialize() const;

    /**
     * @brief Deserialize packet into a bundle
     * @param p the packet to deserialize bundle from
     */
    void Deserialize(Ptr<Packet> p);

    /**
     * @brief Get the expiry time of the bundle
     * @return the time to expire the bundle
     */
    Time GetExpiry() const;

    /**
     * @brief Get the destination EID of the bundle
     * @return the destination EID
     */
    std::string GetDestinationEID() const;

    /**
     * @brief Get the source EID of the bundle
     * @return the source EID
     */
    std::string GetSourceEID() const;

    /**
     * @brief Get the report to EID of the bundle
     * @return the report to EID
     */
    std::string GetReportToEID() const

        /**
         * @brief Checks whether the bundle is an Administrative Record (RFC 1971)
         * @return bool whether bundle is Admin record
         */
        bool isAdminRecord() const;

  private:
    PrimaryBlockHeader m_primaryHeader; //!< The primary block's header
    PayloadBlockHeader m_payloadHeader; //!< The payload block's header
    Ptr<Packet> m_payload;              //!< Bundle payload
};

} // namespace ns3

#endif /* BUNDLE_H */
