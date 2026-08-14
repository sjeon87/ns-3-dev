/*
 * Copyright (c) 2026 Hamburg University of Applied Sciences
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Christoph Busch <christoph.busch@haw-hamburg.de>
 */

#ifndef DOT11S_MESH_PEERING_CLOSE_HEADER_H
#define DOT11S_MESH_PEERING_CLOSE_HEADER_H

#include "ie-dot11s-id.h"
#include "ie-dot11s-mesh-peering-management.h"

#include "ns3/wifi-mgt-header.h"

namespace ns3
{
namespace dot11s
{
using MeshPeeringCloseElems = std::tuple<IeMeshId, IeMeshPeeringManagement>;

/**
 * @ingroup dot11s
 * Header for mesh peering close frames.
 */
class MeshPeeringCloseHeader : public WifiMgtHeader<MeshPeeringCloseHeader, MeshPeeringCloseElems>
{
    friend class WifiMgtHeader<MeshPeeringCloseHeader, MeshPeeringCloseElems>;

  public:
    ~MeshPeeringCloseHeader() override = default;

    /**
     * Register this type.
     * @return the TypeId.
     */
    static TypeId GetTypeId();

    /** @copydoc Header::GetInstanceTypeId */
    TypeId GetInstanceTypeId() const override;
};

} // namespace dot11s
} // namespace ns3

#endif // DOT11S_MESH_PEERING_CLOSE_HEADER_H
