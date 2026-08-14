/*
 * Copyright (c) 2026 Hamburg University of Applied Sciences
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Christoph Busch <christoph.busch@haw-hamburg.de>
 */

#include "mesh-peering-open-header.h"

#include "ns3/mgt-action-headers.h"

namespace ns3
{
namespace dot11s
{
NS_OBJECT_ENSURE_REGISTERED(MeshPeeringOpenHeader);

TypeId
MeshPeeringOpenHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MeshPeeringOpenHeader")
                            .SetParent<Header>()
                            .SetGroupName("Mesh")
                            .AddConstructor<MeshPeeringOpenHeader>();
    return tid;
}

TypeId
MeshPeeringOpenHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
MeshPeeringOpenHeader::GetSerializedSizeImpl() const
{
    uint32_t size = m_capability.GetSerializedSize();
    size += WifiMgtHeader<MeshPeeringOpenHeader, MeshPeeringOpenElems>::GetSerializedSizeImpl();
    return size;
}

void
MeshPeeringOpenHeader::SerializeImpl(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i = m_capability.Serialize(i);
    WifiMgtHeader<MeshPeeringOpenHeader, MeshPeeringOpenElems>::SerializeImpl(i);
}

uint32_t
MeshPeeringOpenHeader::DeserializeImpl(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    i = m_capability.Deserialize(i);
    auto distance = i.GetDistanceFrom(start);
    distance += WifiMgtHeader<MeshPeeringOpenHeader, MeshPeeringOpenElems>::DeserializeImpl(i);
    return distance;
}

} // namespace dot11s
} // namespace ns3
