/*
 * Copyright (c) 2008,2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Kirill Andreev <andreev@iitp.ru>
 *          Aleksey Kovalenko <kovalenko@iitp.ru>
 *          Christoph Busch <christoph.busch@haw-hamburg.de>
 */

#ifndef IE_DOT11S_MESH_PEERING_MANAGEMENT_H
#define IE_DOT11S_MESH_PEERING_MANAGEMENT_H

#include "ns3/mesh-information-element-vector.h"

#include <optional>

namespace ns3
{
namespace dot11s
{

/**
 * @ingroup dot11s
 * @brief Codes used by 802.11s Peer Management Protocol
 */
enum PmpReasonCode : uint16_t
{
    REASON11S_PEERING_CANCELLED = 52, // according to IEEE 802.11 - 2012
    REASON11S_MESH_MAX_PEERS = 53,
    REASON11S_MESH_CAPABILITY_POLICY_VIOLATION = 54,
    REASON11S_MESH_CLOSE_RCVD = 55,
    REASON11S_MESH_MAX_RETRIES = 56,
    REASON11S_MESH_CONFIRM_TIMEOUT = 57,
    REASON11S_MESH_INVALID_GTK = 58,
    REASON11S_MESH_INCONSISTENT_PARAMETERS = 59,
    REASON11S_MESH_INVALID_SECURITY_CAPABILITY = 60,
    REASON11S_RESERVED = 67,
};

/**
 * @ingroup dot11s
 * @brief Peering protocol identifiers (802.11-2020, table 9-244)
 */
enum MeshPeeringProtocolId : uint16_t
{
    MESH_PEERING_MANAGEMENT_PROTOCOL = 0,
    AUTHENTICATED_MESH_PEERING_MANAGEMENT_PROTOCOL = 1,
    VENDOR_SPECIFIC = 255
};

/**
 * @ingroup dot11s
 * @brief Mesh Peering Management element (IEEE 802.11-2020, 9.4.2.101)
 */
class IeMeshPeeringManagement : public WifiInformationElement
{
  public:
    /**
     * Set local link id
     * @param localLinkId the local link id
     */
    void SetLocalLinkId(uint16_t localLinkId);
    /**
     * Set peer link id
     * @param peerLinkId the peer link id
     */
    void SetPeerLinkId(uint16_t peerLinkId);
    /**
     * Set reason code
     * @param reasonCode the reason code
     */
    void SetReasonCode(PmpReasonCode reasonCode);

    /**
     * Get peering protocol identifier
     * @returns the peering protocol identifier
     */
    MeshPeeringProtocolId GetProtocolId() const;
    /**
     * Get local link ID
     * @returns the local link id
     */
    uint16_t GetLocalLinkId() const;
    /**
     * Get peer link ID
     * @returns the peer link ID
     */
    std::optional<uint16_t> GetPeerLinkId() const;
    /**
     * Get reason code
     * @returns the reason code
     */
    std::optional<PmpReasonCode> GetReasonCode() const;

    // Inherited from WifiInformationElement
    WifiInformationElementId ElementId() const override;
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator i) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;
    void Print(std::ostream& os) const override;

  private:
    MeshPeeringProtocolId m_protocolId{
        MESH_PEERING_MANAGEMENT_PROTOCOL}; ///< peering protocol id (always present)
    uint16_t m_localLinkId{0};             ///< local link id (always present)
    std::optional<uint16_t> m_peerLinkId; ///< peer link id (always in confirm, optionally in close)
    std::optional<PmpReasonCode> m_reasonCode; ///< reason code (only in close)

    /**
     * equality operator
     *
     * @param a lhs
     * @param b rhs
     * @returns true if equal
     */
    friend bool operator==(const IeMeshPeeringManagement& a, const IeMeshPeeringManagement& b);
};

bool operator==(const IeMeshPeeringManagement& a, const IeMeshPeeringManagement& b);

/**
 * Stream insertion operator.
 *
 * @param os the output stream
 * @param peerMan the mesh peering management element
 * @returns the output stream
 */
std::ostream& operator<<(std::ostream& os, const IeMeshPeeringManagement& peerMan);
} // namespace dot11s
} // namespace ns3

#endif /* IE_DOT11S_MESH_PEERING_MANAGEMENT_H */
