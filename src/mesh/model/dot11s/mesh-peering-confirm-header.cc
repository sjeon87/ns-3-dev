/*
 * Copyright (c) 2026 Hamburg University of Applied Sciences
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Christoph Busch <christoph.busch@haw-hamburg.de>
 */

#include "mesh-peering-confirm-header.h"

#include "ns3/mgt-action-headers.h"

namespace ns3
{
namespace dot11s
{
NS_OBJECT_ENSURE_REGISTERED(MeshPeeringConfirmHeader);

TypeId
MeshPeeringConfirmHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MeshPeeringConfirmHeader")
                            .SetParent<Header>()
                            .SetGroupName("Mesh")
                            .AddConstructor<MeshPeeringConfirmHeader>();
    return tid;
}

TypeId
MeshPeeringConfirmHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
MeshPeeringConfirmHeader::GetSerializedSizeImpl() const
{
    uint32_t size = m_capability.GetSerializedSize() + sizeof(m_aid);
    size +=
        WifiMgtHeader<MeshPeeringConfirmHeader, MeshPeeringConfirmElems>::GetSerializedSizeImpl();
    return size;
}

void
MeshPeeringConfirmHeader::SerializeImpl(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i = m_capability.Serialize(i);
    i.WriteU16(m_aid);
    WifiMgtHeader<MeshPeeringConfirmHeader, MeshPeeringConfirmElems>::SerializeImpl(i);
}

uint32_t
MeshPeeringConfirmHeader::DeserializeImpl(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    i = m_capability.Deserialize(i);
    m_aid = i.ReadU16();
    auto distance = i.GetDistanceFrom(start);
    distance +=
        WifiMgtHeader<MeshPeeringConfirmHeader, MeshPeeringConfirmElems>::DeserializeImpl(i);
    return distance;
}

} // namespace dot11s
} // namespace ns3
