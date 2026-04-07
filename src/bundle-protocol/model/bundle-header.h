/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */
#ifndef BUNDLE_HEADER_H
#define BUNDLE_HEADER_H

#include "bundle-protocol-flags.h"

#include "ns3/header.h"
#include "ns3/nstime.h"

#include <string>

namespace ns3
{

/**
 * @ingroup dtn
 *
 * @brief An implementation of the Primary Block Header for a BPv7 bundle (RFC 9171).
 *
 * The primary block contains the basic parameters of the bundle,
 * including routing information, timestamps, and processing flags.
 */
class PrimaryBlockHeader : public Header
{
  public:
    PrimaryBlockHeader();

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Get the instance type ID.
     * @return the instance TypeId
     */
    TypeId GetInstanceTypeId() const override;

    /**
     * @brief Print the header parameters.
     * @param os the output stream
     */
    void Print(std::ostream& os) const override;

    /**
     * @brief Get the serialized size of the header.
     * @return the size of the header in bytes
     */
    uint32_t GetSerializedSize() const override;

    /**
     * @brief Serialize the header.
     * @param start the iterator to start writing to
     */
    void Serialize(Buffer::Iterator start) const override;

    /**
     * @brief Deserialize the header.
     * @param start the iterator to start reading from
     * @return the number of bytes read
     */
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * @brief Set the bundle protocol version.
     * @param version the BP version
     */
    void SetVersion(uint8_t version);

    /**
     * @brief Get the bundle protocol version.
     * @return the BP version
     */
    uint8_t GetVersion() const;

    /**
     * @brief Set the processing control flags.
     * @param flags the processing flags
     */
    void SetProcFlags(uint32_t flags);

    /**
     * @brief Get the processing control flags.
     * @return the processing flags
     */
    uint32_t GetProcFlags() const;

    /**
     * @brief Set the CRC type.
     * @param crcType the CRC type identifier
     */
    void SetCrcType(uint8_t crcType);

    /**
     * @brief Get the CRC type.
     * @return the CRC type identifier
     */
    uint8_t GetCrcType() const;

    /**
     * @brief Set the destination EID.
     * @param eid the destination EID string
     */
    void SetDestinationEID(const std::string& eid);

    /**
     * @brief Get the destination EID.
     * @return the destination EID string
     */
    std::string GetDestinationEID() const;

    /**
     * @brief Set the source EID.
     * @param eid the source EID string
     */
    void SetSourceEID(const std::string& eid);

    /**
     * @brief Get the source EID.
     * @return the source EID string
     */
    std::string GetSourceEID() const;

    /**
     * @brief Set the report-to EID.
     * @param eid the report-to EID string
     */
    void SetReportToEID(const std::string& eid);

    /**
     * @brief Get the report-to EID.
     * @return the report-to EID string
     */
    std::string GetReportToEID() const;

    /**
     * @brief Set the bundle creation time.
     * @param t the creation time
     */
    void SetCreationTime(Time t);

    /**
     * @brief Get the bundle creation time.
     * @return the creation time
     */
    Time GetCreationTime() const;

    /**
     * @brief Set the bundle Lifetime.
     * @param t the lifetime value
     */
    void SetLifetime(Time t);

    /**
     * @brief Get the bundle Lifetime.
     * @return the lifetime value
     */
    Time GetLifetime() const;

    /**
     * @brief Set the sequence number.
     * @param seq the sequence number
     */
    void SetSequenceNumber(uint32_t seq);

    /**
     * @brief Get the sequence number.
     * @return the sequence number
     */
    uint32_t GetSequenceNumber() const;

    /**
     * @brief Set the fragment offset.
     * @param offset the fragment offset
     */
    void SetFragmentOffset(uint32_t offset);

    /**
     * @brief Get the fragment offset.
     * @return the fragment offset
     */
    uint32_t GetFragmentOffset() const;

    /**
     * @brief Set the total application data length.
     * @param length the total application data length
     */
    void SetTotalAppDataLength(uint32_t length);

    /**
     * @brief Get the total application data length.
     * @return the total application data length
     */
    uint32_t GetTotalAppDataLength() const;

  private:
    uint8_t m_version = 7;             //!< Bundle Protocol version (7 for RFC 9171)
    uint32_t m_procFlags = 0;          //!< Bundle processing control flags
    uint8_t m_crcType = 0;             //!< CRC Type (0 = None, 1 = CRC16, 2 = CRC32)
    std::string m_destinationEID;      //!< Destination EID
    std::string m_sourceEID;           //!< Source EID
    std::string m_reportToEID;         //!< Report-to EID
    Time m_creationTime;               //!< Bundle creation time
    uint32_t m_seq = 0;                //!< Bundle sequence number
    Time m_lifetime;                   //!< Bundle Lifetime (formerly TTL in BPv6)
    uint32_t m_fragmentOffset = 0;     //!< Fragment offset (if fragmented)
    uint32_t m_totalAppDataLength = 0; //!< Total application data length
};

/**
 * @ingroup dtn
 *
 * @brief An implementation of the Canonical Block Header for BPv7 (used for Payload).
 *
 * Represents the block containing the actual application data payload or extension data.
 */
class PayloadBlockHeader : public Header
{
  public:
    PayloadBlockHeader();

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Get the instance type ID.
     * @return the instance TypeId
     */
    TypeId GetInstanceTypeId() const override;

    /**
     * @brief Print the header parameters.
     * @param os the output stream
     */
    void Print(std::ostream& os) const override;

    /**
     * @brief Get the serialized size of the header.
     * @return the size of the header in bytes
     */
    uint32_t GetSerializedSize() const override;

    /**
     * @brief Serialize the header.
     * @param start the iterator to start writing to
     */
    void Serialize(Buffer::Iterator start) const override;

    /**
     * @brief Deserialize the header.
     * @param start the iterator to start reading from
     * @return the number of bytes read
     */
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * @brief Set the block type.
     * @param type the block type identifier
     */
    void SetBlockType(uint8_t type);

    /**
     * @brief Get the block type.
     * @return the block type identifier
     */
    uint8_t GetBlockType() const;

    /**
     * @brief Set the block number.
     * @param number the unique block number
     */
    void SetBlockNumber(uint32_t number);

    /**
     * @brief Get the block number.
     * @return the block number
     */
    uint32_t GetBlockNumber() const;

    /**
     * @brief Set the processing control flags.
     * @param flags the processing flags
     */
    void SetProcFlags(uint8_t flags);

    /**
     * @brief Get the processing control flags.
     * @return the processing flags
     */
    uint8_t GetProcFlags() const;

    /**
     * @brief Set the CRC type.
     * @param crcType the CRC type identifier
     */
    void SetCrcType(uint8_t crcType);

    /**
     * @brief Get the CRC type.
     * @return the CRC type identifier
     */
    uint8_t GetCrcType() const;

    /**
     * @brief Set the block length.
     * @param length the block length
     */
    void SetBlockLength(uint32_t length);

    /**
     * @brief Get the block length.
     * @return the block length
     */
    uint32_t GetBlockLength() const;

  private:
    uint8_t m_blockType = 1;    //!< Block type identifier (Payload = 1)
    uint32_t m_blockNumber = 1; //!< Unique block number
    uint8_t m_procFlags = 0;    //!< Block processing control flags
    uint8_t m_crcType = 0;      //!< CRC Type
    uint32_t m_blockLength = 0; //!< Length of the block data
};

/**
 * @ingroup dtn
 *
 * @brief An implementation of the Bundle Status Report header for BPv7.
 *
 * Used for administrative records reporting the status of a bundle
 * (e.g., received, forwarded, delivered, deleted).
 */
class BundleStatusReport : public Header
{
  public:
    BundleStatusReport();

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Get the instance type ID.
     * @return the instance TypeId
     */
    TypeId GetInstanceTypeId() const override;

    /**
     * @brief Print the header parameters.
     * @param os the output stream
     */
    void Print(std::ostream& os) const override;

    /**
     * @brief Get the serialized size of the header.
     * @return the size of the header in bytes
     */
    uint32_t GetSerializedSize() const override;

    /**
     * @brief Serialize the header.
     * @param start the iterator to start writing to
     */
    void Serialize(Buffer::Iterator start) const override;

    /**
     * @brief Deserialize the header.
     * @param start the iterator to start reading from
     * @return the number of bytes read
     */
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * @brief Set the status flags.
     * @param flags the status flags
     */
    void SetStatusFlags(uint8_t flags);

    /**
     * @brief Get the status flags.
     * @return the status flags
     */
    uint8_t GetStatusFlags() const;

    /**
     * @brief Set the reason code for the status report.
     * @param code the reason code
     */
    void SetReasonCode(uint8_t code);

    /**
     * @brief Get the reason code for the status report.
     * @return the reason code
     */
    uint8_t GetReasonCode() const;

    /**
     * @brief Set the source EID of the subject bundle.
     * @param eid the source EID string
     */
    void SetSourceEID(const std::string& eid);

    /**
     * @brief Get the source EID of the subject bundle.
     * @return the source EID string
     */
    std::string GetSourceEID() const;

    /**
     * @brief Set the creation time of the subject bundle.
     * @param t the creation time
     */
    void SetCreationTime(Time t);

    /**
     * @brief Get the creation time of the subject bundle.
     * @return the creation time
     */
    Time GetCreationTime() const;

    /**
     * @brief Set the sequence number of the subject bundle.
     * @param seq the sequence number
     */
    void SetSequenceNumber(uint32_t seq);

    /**
     * @brief Get the sequence number of the subject bundle.
     * @return the sequence number
     */
    uint32_t GetSequenceNumber() const;

    /**
     * @brief Set the fragment offset of the reported bundle.
     * @param offset the fragment offset
     */
    void SetFragmentOffset(uint32_t offset);

    /**
     * @brief Get the fragment offset of the reported bundle.
     * @return the fragment offset
     */
    uint32_t GetFragmentOffset() const;

    /**
     * @brief Set the time the bundle was received.
     * @param t the receipt time
     */
    void SetBundleReceiptTime(Time t);

    /**
     * @brief Get the time the bundle was received.
     * @return the receipt time
     */
    Time GetBundleReceiptTime() const;

    /**
     * @brief Set the time the bundle was forwarded.
     * @param t the forwarding time
     */
    void SetBundleForwardTime(Time t);

    /**
     * @brief Get the time the bundle was forwarded.
     * @return the forwarding time
     */
    Time GetBundleForwardTime() const;

    /**
     * @brief Set the time the bundle was delivered.
     * @param t the delivery time
     */
    void SetBundleDeliveryTime(Time t);

    /**
     * @brief Get the time the bundle was delivered.
     * @return the delivery time
     */
    Time GetBundleDeliveryTime() const;

    /**
     * @brief Set the time the bundle was deleted.
     * @param t the deletion time
     */
    void SetBundleDeletionTime(Time t);

    /**
     * @brief Get the time the bundle was deleted.
     * @return the deletion time
     */
    Time GetBundleDeletionTime() const;

  private:
    uint8_t m_statusFlags = 0;     //!< Status flags indicating the event
    uint8_t m_reasonCode = 0;      //!< Reason code for the status
    std::string m_sourceEID;       //!< Source EID of the subject bundle
    Time m_creationTime;           //!< Creation timestamp of subject bundle
    uint32_t m_seq = 0;            //!< Sequence number of subject bundle
    uint32_t m_fragmentOffset = 0; //!< Fragment offset of the subject bundle
    Time m_bundleReceipt;          //!< Timestamp for bundle receipt
    Time m_bundleForward;          //!< Timestamp for bundle forwarding
    Time m_bundleDelivery;         //!< Timestamp for bundle delivery
    Time m_bundleDeletion;         //!< Timestamp for bundle deletion (Replaces CustodyAccept)
};

} // namespace ns3
#endif /* BUNDLE_HEADER_H */
