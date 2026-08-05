/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "link-layer-address-provider.h"

#include "net-device.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(LinkLayerAddressProvider);

TypeId
LinkLayerAddressProvider::GetTypeId()
{
    static TypeId tid = TypeId("ns3::LinkLayerAddressProvider")
                            .SetParent<Object>()
                            .SetGroupName("Network")
                            .AddConstructor<LinkLayerAddressProvider>();
    return tid;
}

LinkLayerAddressProvider::LinkLayerAddressProvider()
{
}

LinkLayerAddressProvider::~LinkLayerAddressProvider()
{
}

void
LinkLayerAddressProvider::DoDispose()
{
    m_addressesCallback = MakeNullCallback<std::vector<Address>>();
    Object::DoDispose();
}

void
LinkLayerAddressProvider::SetAddressesCallback(AddressesCallback cb)
{
    m_addressesCallback = cb;
}

std::vector<Address>
LinkLayerAddressProvider::GetLinkLayerAddresses() const
{
    if (m_addressesCallback.IsNull())
    {
        return std::vector<Address>();
    }
    return m_addressesCallback();
}

Address
LinkLayerAddressProvider::GetAutoconfiguredAddress(Ptr<NetDevice> device)
{
    Ptr<LinkLayerAddressProvider> provider = device->GetObject<LinkLayerAddressProvider>();
    if (provider)
    {
        for (const Address& addr : provider->GetLinkLayerAddresses())
        {
            if (!addr.IsInvalid())
            {
                return addr;
            }
        }
    }
    return device->GetAddress();
}

} // namespace ns3
