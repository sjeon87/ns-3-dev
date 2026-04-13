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
#ifndef BUNDLE_H
#define BUNDLE_H

#include "bundle-block.h"
#include "bundle-header.h"

#include "ns3/address.h"
#include "ns3/buffer.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/type-id.h"

#include <vector>

namespace ns3
{

/**
 *
 * @ingroup dtn
 *
 * @brief An implementation of a bundle for BPv7 (RFC 9171).
 *
 * A bundle is represented as an ordered vector of BundleBlock objects.
 * The first block is always a PrimaryBlock, followed by one or more
 * canonical blocks (e.g., PayloadBlock or Extension Blocks). This structure
 * maps directly onto the RFC 9171 bundle format for simulation purposes.
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
     * @brief Add a block to the end of the bundle's block vector.
     * @param block the block to append
     */
    void AddBlock(Ptr<BundleBlock> block);

    /**
     * @brief Get the block at a given index.
     * @param index the position in the block vector
     * @return the BundleBlock at that index
     */
    Ptr<BundleBlock> GetBlock(uint32_t index) const;

    /**
     * @brief Get the number of blocks in the bundle.
     * @return the block count
     */
    uint32_t GetBlockCount() const;

    /**
     * @brief Get all blocks in the bundle.
     * @return const reference to the block vector
     */
    const std::vector<Ptr<BundleBlock>>& GetBlocks() const;

    /**
     * @brief Convenience accessor for the primary block.
     * Assumes the first block is always a PrimaryBlock.
     * @return Ptr to the PrimaryBlock, or nullptr if not set
     */
    Ptr<PrimaryBlock> GetPrimaryBlock() const;

    /**
     * @brief Convenience accessor for the payload block.
     * Searches the block vector for the first block with type 1.
     * @return Ptr to the PayloadBlock, or nullptr if not found
     */
    Ptr<PayloadBlock> GetPayloadBlock() const;

    /**
     * @brief Get the total serialized size of the bundle across all blocks.
     * @return the total size in bytes
     */
    uint32_t GetTotalSize() const;

    /**
     * @brief Serialize all blocks in the bundle into a single packet.
     * Blocks are serialized in order and concatenated.
     * @return the packet holding the full serialized bundle
     */
    Ptr<Packet> Serialize() const;

    /**
     * @brief Deserialize a packet into the bundle's block vector.
     * Reconstructs the PrimaryBlock first, then subsequent blocks.
     * @param p the packet to deserialize from
     */
    void Deserialize(Ptr<Packet> p);

    /**
     * @brief Get the expiry time of the bundle.
     * Derived from the primary block's creation time and Lifetime (BPv7).
     * @return the time at which the bundle expires
     */
    Time GetExpiry() const;

    /**
     * @brief Get the destination EID of the bundle.
     * @return the destination EID string
     */
    std::string GetDestinationEID() const;

    /**
     * @brief Get the source EID of the bundle.
     * @return the source EID string
     */
    std::string GetSourceEID() const;

    /**
     * @brief Get the report-to EID of the bundle.
     * @return the report-to EID string
     */
    std::string GetReportToEID() const;

    /**
     * @brief Checks whether the bundle is an Administrative Record.
     * @return true if the bundle is an administrative record
     */
    bool IsAdminRecord() const;

  private:
    std::vector<Ptr<BundleBlock>> m_blocks; //!< Ordered sequence of blocks comprising the bundle
};

} // namespace ns3

#endif /* BUNDLE_H */
