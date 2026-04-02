/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#ifndef BUNDLE_HEADER_H
#define BUNDLE_HEADER_H

#include "ns3/header.h"
#include "ns3/nstime.h"

#include <string>

/**
 * I'm assuming all integers here are SDNVs, will
 * make them actual SDNV once the implementation is finalized.
 *
 */

namespace ns3
{

class PrimaryBlockHeader : public Header
{
  public:
    PrimaryBlockHeader();

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    uint8_t m_version = 0;  //!< version of the bundle protocol that constructed this block
    uint32_t m_procFlags;   //!< an SDNV that contains the bundle processing control flags
    uint32_t m_blockLength; //!< an SDNV that contains the aggregate length of all remaining fields
                            //!< of the block.
    uint16_t m_destinationSchemeOffset; //!< contains the the scheme name of the endpoint ID of the
                                        //!< bundle's destination
    uint16_t m_destinationSSPOffset;    //!< contains the scheme-specific part of the endpoint ID of
                                        //!< the bundle's destination.
    uint16_t m_sourceSchemeOffset; //!< contains the scheme name of the endpoint ID of the bundle's
                                   //!< nominal source
    uint16_t m_sourceSSPOffset;    //!< contains the scheme-specific part of the endpoint ID of the
                                   //!< bundle's nominal source.
    uint16_t m_reportToSchemeOffset; //!< contains the scheme name of the ID of the endpoint to
                                     //!< which status reports pertaining to the forwarding and
                                     //!< delivery of this bundle are to be transmitted.
    uint16_t m_reportToSSPOffset; //!< contains the scheme-specific part of the ID of the endpoint
                                  //!< to which status reports pertaining to the forwarding and
                                  //!< delivery of this bundle are to be transmitted.
    uint16_t
        m_custodianSchemeOffset; //!< contains the scheme name of the current custodian endpoint ID
    uint16_t m_custodianSSPOffset; //!< contains the scheme-specific part of the current custodian
                                   //!< endpoint ID.
    Time m_creationTime;           //!< creation time of the bundle
    uint32_t m_seq;                //!< sequence number of the bundle
    Time m_TTL;                    //!< time at which bundle is expired
    uint32_t m_dictionaryLength;   //!< length of the dictionary byte array
    std::string m_dictByteArray;   //!< concatenation of scheme names and SSPs of all endpoint IDs
    uint32_t m_fragmentOffset; //!< If bundle is a fragment, offset of the fragment from the start
    uint32_t
        m_totalAppDataLength; //!< If bundle is a fragment, the length of the original data unit
};

class PayloadBlockHeader : public Header
{
  public:
    PayloadBlockHeader();

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    uint8_t m_blockType =
        1; //!< bundle block type code 1 indicates that the block is a bundle payload block.
    uint8_t m_procFlags;    //!< an SDNV that contains the bundle processing control flags
    uint32_t m_blockLength; //!< aggregate length of the bundle payload
};

class BundleStatusReport : public Header
{
  public:
    BundleStatusReport();

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    uint8_t m_statusFlags;
    uint8_t m_reasonCode;
    uint32_t m_fragmentOffset;
    Time m_bundleReceipt;
    Time m_custodyAccept;
    Time m_bundleForward;
    Time m_bundleDelivery;
    Time m_creationTime;
    uint32_t m_seq;
    uint32_t m_lenSourceEID;
    uint32_t m_sourceID;
};

class CustodySignal : public Header
{
  public:
    CustodySignal();

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    uint8_t m_statusFlags;
    uint32_t m_fragmentOffset;
    Time m_tos;
    Time m_creationTime;
    uint32_t m_seq;
    uint32_t m_lenSourceEID;
    uint32_t m_sourceID;
};

} // namespace ns3

#endif /* BUNDLE_H */
