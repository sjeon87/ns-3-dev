/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef LINK_LAYER_ADDRESS_PROVIDER_H
#define LINK_LAYER_ADDRESS_PROVIDER_H

#include "address.h"

#include "ns3/callback.h"
#include "ns3/object.h"
#include "ns3/ptr.h"

#include <vector>

namespace ns3
{

class NetDevice;

/**
 * @ingroup network
 *
 * @brief Exposes the link-layer addresses a device offers for IPv6 interface
 *        identifier (IID) formation, beyond the single NetDevice::GetAddress().
 *
 * Some link layers have more than one link-layer address - e.g. IEEE 802.15.4 (a
 * 16-bit short and a 64-bit extended address) or Bluetooth LE (public, random-static
 * and private addresses). When such a device uses one address as its NetDevice
 * identity but a different one is preferable for IID formation, it aggregates one of
 * these providers and configures it with a callback returning its addresses in
 * preference order (most preferred first). Consumers query it generically via
 * ``device->GetObject<LinkLayerAddressProvider>()``; a device with a single address
 * simply does not aggregate one.
 *
 * The provider is generic: it makes no assumption about the address kind or width.
 * A future consumer needing to distinguish address roles (e.g. a BLE public vs.
 * random address, both 48-bit) can be served by a role-tagged extension without
 * changing this interface or its current consumers.
 */
class LinkLayerAddressProvider : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    LinkLayerAddressProvider();
    ~LinkLayerAddressProvider() override;

    /// Callback returning the device's IID-candidate link-layer addresses.
    typedef Callback<std::vector<Address>> AddressesCallback;

    /**
     * @brief Set the source of this device's link-layer addresses.
     * @param cb callback returning the addresses, most-preferred first
     */
    void SetAddressesCallback(AddressesCallback cb);

    /**
     * @brief Get the device's link-layer addresses, most preferred first.
     * @return the addresses, or an empty vector if none are configured
     */
    std::vector<Address> GetLinkLayerAddresses() const;

    /**
     * @brief Get the link-layer address a device should use to form its IPv6 IID.
     *
     * Returns the first valid address advertised by the device's aggregated
     * LinkLayerAddressProvider, otherwise NetDevice::GetAddress(). The address type
     * (Mac16/Mac48/Mac64) then selects the link-type-specific IID rule in
     * Ipv6Address::MakeAutoconfiguredAddress().
     *
     * @param device the device to query
     * @return the address to use for IID formation
     */
    static Address GetAutoconfiguredAddress(Ptr<NetDevice> device);

  protected:
    void DoDispose() override;

  private:
    AddressesCallback m_addressesCallback; //!< source of the device's addresses
};

} // namespace ns3

#endif /* LINK_LAYER_ADDRESS_PROVIDER_H */
