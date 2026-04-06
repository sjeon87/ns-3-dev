/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */
#ifndef BUNDLE_HEADER_H
#define BUNDLE_HEADER_H

#include "bundle-flags.h"

#include "ns3/header.h"
#include "ns3/nstime.h"

#include <string>

namespace ns3
{

/**
 * @ingroup dtn
 *
 * @brief An implementation of the Primary Block Header for a BPv7 bundle.
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
     * @brief Set the block length.
     * @param length the block length
     */
    void SetBlockLength(uint32_t length);

    /**
     * @brief Get the block length.
     * @return the block length
     */
    uint32_t GetBlockLength() const;

    /**
     * @brief Set the destination EID scheme offset.
     * @param offset the offset value
     */
    void SetDestinationSchemeOffset(uint16_t offset);

    /**
     * @brief Get the destination EID scheme offset.
     * @return the offset value
     */
    uint16_t GetDestinationSchemeOffset() const;

    /**
     * @brief Set the destination EID SSP offset.
     * @param offset the offset value
     */
    void SetDestinationSSPOffset(uint16_t offset);

    /**
     * @brief Get the destination EID SSP offset.
     * @return the offset value
     */
    uint16_t GetDestinationSSPOffset() const;

    /**
     * @brief Set the source EID scheme offset.
     * @param offset the offset value
     */
    void SetSourceSchemeOffset(uint16_t offset);

    /**
     * @brief Get the source EID scheme offset.
     * @return the offset value
     */
    uint16_t GetSourceSchemeOffset() const;

    /**
     * @brief Set the source EID SSP offset.
     * @param offset the offset value
     */
    void SetSourceSSPOffset(uint16_t offset);

    /**
     * @brief Get the source EID SSP offset.
     * @return the offset value
     */
    uint16_t GetSourceSSPOffset() const;

    /**
     * @brief Set the report-to EID scheme offset.
     * @param offset the offset value
     */
    void SetReportToSchemeOffset(uint16_t offset);

    /**
     * @brief Get the report-to EID scheme offset.
     * @return the offset value
     */
    uint16_t GetReportToSchemeOffset() const;

    /**
     * @brief Set the report-to EID SSP offset.
     * @param offset the offset value
     */
    void SetReportToSSPOffset(uint16_t offset);

    /**
     * @brief Get the report-to EID SSP offset.
     * @return the offset value
     */
    uint16_t GetReportToSSPOffset() const;

    /**
     * @brief Set the custodian EID scheme offset.
     * @param offset the offset value
     */
    void SetCustodianSchemeOffset(uint16_t offset);

    /**
     * @brief Get the custodian EID scheme offset.
     * @return the offset value
     */
    uint16_t GetCustodianSchemeOffset() const;

    /**
     * @brief Set the custodian EID SSP offset.
     * @param offset the offset value
     */
    void SetCustodianSSPOffset(uint16_t offset);

    /**
     * @brief Get the custodian EID SSP offset.
     * @return the offset value
     */
    uint16_t GetCustodianSSPOffset() const;

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
     * @brief Set the Time-To-Live (TTL).
     * @param t the TTL value
     */
    void SetTTL(Time t);

    /**
     * @brief Get the Time-To-Live (TTL).
     * @return the TTL value
     */
    Time GetTTL() const;

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
     * @brief Set the dictionary byte array.
     * @param dict the dictionary string
     */
    void SetDictionary(const std::string& dict);

    /**
     * @brief Get the dictionary byte array.
     * @return the dictionary string
     */
    const std::string& GetDictionary() const;

    /**
     * @brief Set the dictionary length.
     * @param length the dictionary length
     */
    void SetDictionaryLength(uint32_t length);

    /**
     * @brief Get the dictionary length.
     * @return the dictionary length
     */
    uint32_t GetDictionaryLength() const;

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

    /**
     * @brief Helper to configure destination EID details.
     * @param scheme the EID scheme
     * @param ssp the EID Scheme-Specific Part
     */
    void SetDestinationEID(const std::string& scheme, const std::string& ssp);

    /**
     * @brief Helper to configure source EID details.
     * @param scheme the EID scheme
     * @param ssp the EID Scheme-Specific Part
     */
    void SetSourceEID(const std::string& scheme, const std::string& ssp);

    /**
     * @brief Helper to configure report-to EID details.
     * @param scheme the EID scheme
     * @param ssp the EID Scheme-Specific Part
     */
    void SetReportToEID(const std::string& scheme, const std::string& ssp);

    /**
     * @brief Helper to configure custodian EID details.
     * @param scheme the EID scheme
     * @param ssp the EID Scheme-Specific Part
     */
    void SetCustodianEID(const std::string& scheme, const std::string& ssp);

  private:
    uint8_t m_version = 0;                  //!< Bundle Protocol version
    uint32_t m_procFlags = 0;               //!< Bundle processing control flags
    uint32_t m_blockLength = 0;             //!< Length of the primary block
    uint16_t m_destinationSchemeOffset = 0; //!< Destination EID scheme offset
    uint16_t m_destinationSSPOffset = 0;    //!< Destination EID SSP offset
    uint16_t m_sourceSchemeOffset = 0;      //!< Source EID scheme offset
    uint16_t m_sourceSSPOffset = 0;         //!< Source EID SSP offset
    uint16_t m_reportToSchemeOffset = 0;    //!< Report-to EID scheme offset
    uint16_t m_reportToSSPOffset = 0;       //!< Report-to EID SSP offset
    uint16_t m_custodianSchemeOffset = 0;   //!< Custodian EID scheme offset
    uint16_t m_custodianSSPOffset = 0;      //!< Custodian EID SSP offset
    Time m_creationTime;                    //!< Bundle creation time
    uint32_t m_seq = 0;                     //!< Bundle sequence number
    Time m_TTL;                             //!< Bundle time-to-live
    uint32_t m_dictionaryLength = 0;        //!< Length of the dictionary
    std::string m_dictByteArray;            //!< Dictionary byte array
    uint32_t m_fragmentOffset = 0;          //!< Fragment offset (if fragmented)
    uint32_t m_totalAppDataLength = 0;      //!< Total application data length
};

/**
 * @ingroup dtn
 *
 * @brief An implementation of the Payload Block Header.
 *
 * Represents the block containing the actual application data payload.
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
    uint8_t m_procFlags = 0;    //!< Block processing control flags
    uint32_t m_blockLength = 0; //!< Length of the payload block
};

/**
 * @ingroup dtn
 *
 * @brief An implementation of the Bundle Status Report header.
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
     * @brief Set the time custody of the bundle was accepted.
     * @param t the custody acceptance time
     */
    void SetCustodyAcceptTime(Time t);

    /**
     * @brief Get the time custody of the bundle was accepted.
     * @return the custody acceptance time
     */
    Time GetCustodyAcceptTime() const;

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
     * @brief Set the length of the source EID.
     * @param len the source EID length
     */
    void SetSourceEIDLength(uint32_t len);

    /**
     * @brief Get the length of the source EID.
     * @return the source EID length
     */
    uint32_t GetSourceEIDLength() const;

    /**
     * @brief Set the source EID identifier.
     * @param id the source identifier
     */
    void SetSourceID(uint32_t id);

    /**
     * @brief Get the source EID identifier.
     * @return the source identifier
     */
    uint32_t GetSourceID() const;

  private:
    uint8_t m_statusFlags = 0;     //!< Status flags indicating the event
    uint8_t m_reasonCode = 0;      //!< Reason code for the status
    uint32_t m_fragmentOffset = 0; //!< Fragment offset of the subject bundle
    Time m_bundleReceipt;          //!< Timestamp for bundle receipt
    Time m_custodyAccept;          //!< Timestamp for custody acceptance
    Time m_bundleForward;          //!< Timestamp for bundle forwarding
    Time m_bundleDelivery;         //!< Timestamp for bundle delivery
    Time m_creationTime;           //!< Creation timestamp of subject bundle
    uint32_t m_seq = 0;            //!< Sequence number of subject bundle
    uint32_t m_lenSourceEID = 0;   //!< Source EID string length
    uint32_t m_sourceID = 0;       //!< Source EID identifier
};

/**
 * @ingroup dtn
 *
 * @brief An implementation of the Custody Signal header.
 *
 * Used for administrative records reporting the acceptance or
 * refusal of custody of a bundle.
 */
class CustodySignal : public Header
{
  public:
    CustodySignal();

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
     * @brief Set the custody status flags.
     * @param flags the status flags
     */
    void SetStatusFlags(uint8_t flags);

    /**
     * @brief Get the custody status flags.
     * @return the status flags
     */
    uint8_t GetStatusFlags() const;

    /**
     * @brief Set the fragment offset of the subject bundle.
     * @param offset the fragment offset
     */
    void SetFragmentOffset(uint32_t offset);

    /**
     * @brief Get the fragment offset of the subject bundle.
     * @return the fragment offset
     */
    uint32_t GetFragmentOffset() const;

    /**
     * @brief Set the time the custody signal was generated.
     * @param t the time of signal
     */
    void SetTimeOfSignal(Time t);

    /**
     * @brief Get the time the custody signal was generated.
     * @return the time of signal
     */
    Time GetTimeOfSignal() const;

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
     * @brief Set the length of the source EID.
     * @param len the source EID length
     */
    void SetSourceEIDLength(uint32_t len);

    /**
     * @brief Get the length of the source EID.
     * @return the source EID length
     */
    uint32_t GetSourceEIDLength() const;

    /**
     * @brief Set the source EID identifier.
     * @param id the source identifier
     */
    void SetSourceID(uint32_t id);

    /**
     * @brief Get the source EID identifier.
     * @return the source identifier
     */
    uint32_t GetSourceID() const;

  private:
    uint8_t m_statusFlags = 0;     //!< Status flags (accept/reject reason)
    uint32_t m_fragmentOffset = 0; //!< Fragment offset of the subject bundle
    Time m_tos;                    //!< Time of signal generation
    Time m_creationTime;           //!< Creation timestamp of subject bundle
    uint32_t m_seq = 0;            //!< Sequence number of subject bundle
    uint32_t m_lenSourceEID = 0;   //!< Source EID string length
    uint32_t m_sourceID = 0;       //!< Source EID identifier
};

} // namespace ns3
#endif /* BUNDLE_HEADER_H */
