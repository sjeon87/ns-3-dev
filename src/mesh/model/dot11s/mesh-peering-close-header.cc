/*
 * Copyright (c) 2026 Hamburg University of Applied Sciences
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Christoph Busch <christoph.busch@haw-hamburg.de>
 */

#include "mesh-peering-close-header.h"

#include "ns3/header.h"
#include "ns3/mgt-action-headers.h"

namespace ns3
{
namespace dot11s
{
NS_OBJECT_ENSURE_REGISTERED(MeshPeeringCloseHeader);

TypeId
MeshPeeringCloseHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MeshPeeringCloseHeader")
                            .SetParent<Header>()
                            .SetGroupName("Mesh")
                            .AddConstructor<MeshPeeringCloseHeader>();
    return tid;
}

TypeId
MeshPeeringCloseHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

} // namespace dot11s
} // namespace ns3
