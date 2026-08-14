/*
 * Copyright (c) 2026 Hamburg University of Applied Sciences
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Christoph Busch <christoph.busch@haw-hamburg.de>
 */

#ifndef DOT11S_MESH_PEERING_CONFIRM_HEADER_H
#define DOT11S_MESH_PEERING_CONFIRM_HEADER_H

#include "ie-dot11s-configuration.h"
#include "ie-dot11s-id.h"
#include "ie-dot11s-mesh-peering-management.h"

#include "ns3/capability-information.h"
#include "ns3/header.h"
#include "ns3/supported-rates.h"
#include "ns3/wifi-mgt-header.h"

#include <optional>

namespace ns3
{
namespace dot11s
{
using MeshPeeringConfirmElems = std::tuple<SupportedRates,
                                           std::optional<ExtendedSupportedRatesIE>,
                                           IeMeshId,
                                           IeConfiguration,
                                           IeMeshPeeringManagement>;

/**
 * @ingroup dot11s
 * Header for mesh peering confirm frames.
 */
class MeshPeeringConfirmHeader
    : public WifiMgtHeader<MeshPeeringConfirmHeader, MeshPeeringConfirmElems>
{
    friend class WifiMgtHeader<MeshPeeringConfirmHeader, MeshPeeringConfirmElems>;

  public:
    ~MeshPeeringConfirmHeader() override = default;

    /**
     * Register this type.
     * @return the TypeId.
     */
    static TypeId GetTypeId();

    /** @copydoc Header::GetInstanceTypeId */
    TypeId GetInstanceTypeId() const override;

    CapabilityInformation m_capability; ///< capability information
    uint16_t m_aid{0};                  ///< association id

  protected:
    /** @copydoc Header::GetSerializedSize */
    uint32_t GetSerializedSizeImpl() const;
    /** @copydoc Header::Serialize */
    void SerializeImpl(Buffer::Iterator start) const;
    /** @copydoc Header::Deserialize */
    uint32_t DeserializeImpl(Buffer::Iterator start);
};

} // namespace dot11s
} // namespace ns3

#endif // DOT11S_MESH_PEERING_CONFIRM_HEADER_H
