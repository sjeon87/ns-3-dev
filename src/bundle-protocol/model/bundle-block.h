/*
 * Copyright (c) 2008 INRIA
 *                  2013 University of New Brunswick
 *                  2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *           Dizhi Zhou <dizhi.zhou@gmail.com>
 *           Gerard Garcia <ggarcia@deic.uab.cat>
 *           Ishaan Lagwankar <lagwanka@msu.edu>
 */

#ifndef BUNDLE_BLOCK_H
#define BUNDLE_BLOCK_H

#include "bundle-header.h"

#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/type-id.h"

namespace ns3
{

/**
 * @ingroup dtn
 *
 * @brief Abstract base class representing a single block within a Bundle.
 *
 * In Delay Tolerant Networking (DTN), a bundle is composed of a sequence of blocks.
 * This base class defines the interface for serializing and deserializing these
 * blocks using native ns-3 Packet and Header paradigms. Instead of manual byte
 * manipulation, child classes should wrap their respective ns3::Header implementations.
 */
class BundleBlock : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    BundleBlock() = default;
    ~BundleBlock() override = default;

    /**
     * @brief Serialize this block into an ns-3 Packet.
     * * This method constructs a packet representing the block, adding its specific
     * header (and payload, if applicable) to it.
     * * @return Ptr to the fully constructed packet for this block
     */
    virtual Ptr<Packet> SerializeToPacket() const = 0;

    /**
     * @brief Deserialize this block from a master packet stream.
     * * This method removes the block's specific header from the front of the
     * provided packet and extracts any associated payload data. This modifies
     * the incoming packet by consuming the bytes belonging to this block.
     * * @param p The master packet containing the serialized bundle stream
     * @return The number of bytes consumed from the packet
     */
    virtual uint32_t Deserialize(Ptr<Packet> p) = 0;

    /**
     * @brief Retrieve the standardized block type identifier.
     * * @return the uint8_t block type (e.g., 0 for Primary, 1 for Payload)
     */
    virtual uint8_t GetBlockType() const = 0;
};

/**
 * @ingroup dtn
 * * @brief Implementation of the Primary Bundle Block.
 * * The Primary Block is always the first block in a bundle sequence. It contains
 * critical routing, identification, and lifetime information required by the
 * Bundle Protocol Agent to process the bundle. It does not carry application payload.
 */
class PrimaryBlock : public BundleBlock
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    PrimaryBlock();
    ~PrimaryBlock() override = default;

    /**
     * @brief Serialize the Primary Block to a packet.
     * @return Ptr to the packet containing the PrimaryBlockHeader
     */
    Ptr<Packet> SerializeToPacket() const override;

    /**
     * @brief Deserialize the Primary Block from a packet stream.
     * @param p The packet to deserialize from
     * @return The number of bytes consumed (size of the PrimaryBlockHeader)
     */
    uint32_t Deserialize(Ptr<Packet> p) override;

    /**
     * @brief Get the block type.
     * @return Always returns 0 for the Primary Block.
     */
    uint8_t GetBlockType() const override
    {
        return 0;
    }

    /**
     * @brief Get a modifiable reference to the underlying PrimaryBlockHeader.
     * @return Reference to the PrimaryBlockHeader
     */
    PrimaryBlockHeader& GetHeader();

    /**
     * @brief Get a constant reference to the underlying PrimaryBlockHeader.
     * @return Const reference to the PrimaryBlockHeader
     */
    const PrimaryBlockHeader& GetHeader() const;

  private:
    PrimaryBlockHeader m_header; //!< The native ns-3 header managing primary block fields
};

/**
 * @ingroup dtn
 * * @brief Implementation of the Payload Bundle Block.
 * * The Payload Block carries the actual application data being transported
 * across the DTN. It consists of a PayloadBlockHeader followed by the
 * opaque data payload.
 */
class PayloadBlock : public BundleBlock
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    PayloadBlock();
    ~PayloadBlock() override = default;

    /**
     * @brief Serialize the Payload Block to a packet.
     * @return Ptr to the packet containing the PayloadBlockHeader and the payload data
     */
    Ptr<Packet> SerializeToPacket() const override;

    /**
     * @brief Deserialize the Payload Block from a packet stream.
     * @param p The packet to deserialize from
     * @return The total number of bytes consumed (header size + payload size)
     */
    uint32_t Deserialize(Ptr<Packet> p) override;

    /**
     * @brief Get the block type.
     * @return Returns the block type defined in the underlying header (usually 1).
     */
    uint8_t GetBlockType() const override;

    /**
     * @brief Get a modifiable reference to the underlying PayloadBlockHeader.
     * @return Reference to the PayloadBlockHeader
     */
    PayloadBlockHeader& GetHeader();

    /**
     * @brief Get a constant reference to the underlying PayloadBlockHeader.
     * @return Const reference to the PayloadBlockHeader
     */
    const PayloadBlockHeader& GetHeader() const;

    /**
     * @brief Set the application data payload for this block.
     * * This automatically updates the length field in the underlying PayloadBlockHeader.
     * * @param payload The ns-3 packet containing the application data
     */
    void SetPayload(Ptr<Packet> payload);

    /**
     * @brief Get the application data payload carried by this block.
     * @return Ptr to the packet containing the application data
     */
    Ptr<Packet> GetPayload() const;

  private:
    PayloadBlockHeader m_header; //!< The native ns-3 header managing payload metadata
    Ptr<Packet> m_payload;       //!< The core application data carried by this block
};

} // namespace ns3

#endif /* BUNDLE_BLOCK_H */
