/*
 * Copyright (c) 2008,2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Kirill Andreev <andreev@iitp.ru>
 *          Aleksey Kovalenko <kovalenko@iitp.ru>
 *          Christoph Busch <christoph.busch@haw-hamburg.de>
 */

#include "ie-dot11s-mesh-peering-management.h"

#include "ns3/abort.h"

namespace ns3
{
namespace dot11s
{

WifiInformationElementId
IeMeshPeeringManagement::ElementId() const
{
    return IE_MESH_PEERING_MANAGEMENT;
}

void
IeMeshPeeringManagement::SetLocalLinkId(uint16_t localLinkId)
{
    m_localLinkId = localLinkId;
}

void
IeMeshPeeringManagement::SetPeerLinkId(uint16_t peerLinkId)
{
    m_peerLinkId = peerLinkId;
}

void
IeMeshPeeringManagement::SetReasonCode(PmpReasonCode reasonCode)
{
    m_reasonCode = reasonCode;
}

MeshPeeringProtocolId
IeMeshPeeringManagement::GetProtocolId() const
{
    return m_protocolId;
}

uint16_t
IeMeshPeeringManagement::GetLocalLinkId() const
{
    return m_localLinkId;
}

std::optional<uint16_t>
IeMeshPeeringManagement::GetPeerLinkId() const
{
    return m_peerLinkId;
}

std::optional<PmpReasonCode>
IeMeshPeeringManagement::GetReasonCode() const
{
    return m_reasonCode;
}

uint16_t
IeMeshPeeringManagement::GetInformationFieldSize() const
{
    // protocol id (2 bytes) and local link id (2 bytes) are always present
    uint16_t size = 2 + 2;

    if (m_peerLinkId)
    {
        size += 2;
    }
    if (m_reasonCode)
    {
        size += 2;
    }

    return size;
}

void
IeMeshPeeringManagement::SerializeInformationField(Buffer::Iterator i) const
{
    i.WriteU16(m_protocolId);
    i.WriteU16(m_localLinkId);
    if (m_peerLinkId)
    {
        i.WriteU16(m_peerLinkId.value());
    }
    if (m_reasonCode)
    {
        i.WriteU16(m_reasonCode.value());
    }
}

uint16_t
IeMeshPeeringManagement::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    Buffer::Iterator i = start;
    m_protocolId = static_cast<MeshPeeringProtocolId>(i.ReadU16());
    m_localLinkId = i.ReadU16();

    // the three mesh peering frames (open, confirm, close) share this element but carry
    // different fields. according to 802.11-2020, 9.4.2.101, the peering protocol identifier
    // and local link id are always present (length >= 4 bytes): open adds nothing (4 bytes),
    // confirm adds the peer link id (6 bytes), and close adds the reason code plus optionally
    // the peer link id (6 or 8 bytes). a close that omits the peer link id is also 6 bytes and
    // thus indistinguishable from a confirm by its length. to avoid this, the peer link id is
    // always sent in a close (see dot11s::PeerLink::SendPeerLinkClose), so a length of 6 is always
    // a confirm and 8 is always a close.

    switch (length)
    {
    case 4:
        // mesh peering open: no additional fields
        break;
    case 6:
        // mesh peering confirm
        m_peerLinkId = i.ReadU16();
        break;
    case 8:
        // mesh peering close
        m_peerLinkId = i.ReadU16();
        m_reasonCode = static_cast<PmpReasonCode>(i.ReadU16());
        break;
    default:
        NS_ABORT_MSG("unexpected mesh peering management element length " << length);
    }

    return i.GetDistanceFrom(start);
}

void
IeMeshPeeringManagement::Print(std::ostream& os) const
{
    os << "PeerMgmt=(ProtocolId=" << m_protocolId << ", LocalLinkId=" << m_localLinkId;
    if (m_peerLinkId)
    {
        os << ", PeerLinkId=" << m_peerLinkId.value();
    }
    if (m_reasonCode)
    {
        os << ", ReasonCode=" << m_reasonCode.value();
    }
    os << ")";
}

bool
operator==(const IeMeshPeeringManagement& a, const IeMeshPeeringManagement& b)
{
    return ((a.m_protocolId == b.m_protocolId) && (a.m_localLinkId == b.m_localLinkId) &&
            (a.m_peerLinkId == b.m_peerLinkId) && (a.m_reasonCode == b.m_reasonCode));
}

std::ostream&
operator<<(std::ostream& os, const IeMeshPeeringManagement& a)
{
    a.Print(os);
    return os;
}

} // namespace dot11s
} // namespace ns3
