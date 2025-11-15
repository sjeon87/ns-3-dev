/*
 * Copyright (c) 2025 Universita' di Firenze, Italy
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "ipv4-network-address.h"

#include "ns3/assert.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv4NetworkAddress");

Ipv4NetworkAddress::Ipv4NetworkAddress(Ipv4Address address, uint8_t networkLength)
{
    NS_LOG_FUNCTION(this << address << +networkLength);
    NS_ASSERT_MSG(networkLength <= MAX_PREFIX_LENGTH, "Invalid network length");
    m_address = address;
    m_networkLength = networkLength;
}

uint8_t
Ipv4NetworkAddress::GetType()
{
    static uint8_t type = Address::Register("IpAddress", ADDRESS_LENGTH);
    return type;
}

bool
Ipv4NetworkAddress::IsMatchingType(const Address& address)
{
    return address.CheckCompatible(GetType(), ADDRESS_LENGTH);
}

Ipv4NetworkAddress::
operator Address() const
{
    return ConvertTo();
}

Ipv4NetworkAddress
Ipv4NetworkAddress::ConvertFrom(const Address& address)
{
    NS_LOG_FUNCTION(address);
    NS_ASSERT(address.CheckCompatible(GetType(), ADDRESS_LENGTH));
    uint8_t buf[ADDRESS_LENGTH];
    address.CopyTo(buf);
    Ipv4NetworkAddress addr;
    addr.m_address = Ipv4Address::Deserialize(buf);
    addr.m_networkLength = buf[4];

    return addr;
}

Address
Ipv4NetworkAddress::ConvertTo() const
{
    NS_LOG_FUNCTION(this);
    uint8_t buf[ADDRESS_LENGTH];
    m_address.Serialize(buf);
    buf[4] = m_networkLength;
    return Address(GetType(), buf, ADDRESS_LENGTH);
}

Ipv4Address
Ipv4NetworkAddress::GetAddress() const
{
    return m_address;
}

Ipv4Address
Ipv4NetworkAddress::GetMask() const
{
    return Ipv4Address(GetMaskValue());
}

uint32_t
Ipv4NetworkAddress::GetMaskValue() const
{
    // note: the result could be cached into a std::optional.
    // In case someone would like to do that, remember to also modify
    // the <=> operator to skip the optional in the comparison.
    // At the moment of writing, this optimization appears unnecessary.

    uint32_t mask =
        (m_networkLength == 0) ? 0 : 0xffffffff << (MAX_PREFIX_LENGTH - m_networkLength);
    return mask;
}

uint8_t
Ipv4NetworkAddress::GetNetworkLength() const
{
    return m_networkLength;
}

Ipv4NetworkAddress
Ipv4NetworkAddress::GetNetwork() const
{
    if (!m_networkAddr)
    {
        m_networkAddr.emplace(m_address.Get() & GetMaskValue());
    }
    return Ipv4NetworkAddress(*m_networkAddr, m_networkLength);
}

void
Ipv4NetworkAddress::MakeNetwork()
{
    m_address = Ipv4Address(m_address.Get() & GetMaskValue());
    m_networkAddr = m_address;
}

bool
Ipv4NetworkAddress::Includes(const Ipv4NetworkAddress& other) const
{
    NS_LOG_FUNCTION(this << other);

    if (m_networkLength == 0)
    {
        return true;
    }
    else if (m_networkLength > other.m_networkLength)
    {
        return false;
    }

    uint32_t aAddr = m_address.Get();
    uint32_t bAddr = other.GetNetwork().GetAddress().Get();
    uint32_t result = aAddr ^ bAddr;

    result &= GetMaskValue();

    return result == 0;
}

std::ostream&
operator<<(std::ostream& os, const Ipv4NetworkAddress& addr)
{
    os << addr.GetAddress() << "/" << static_cast<uint32_t>(addr.GetNetworkLength());
    return os;
}

} // namespace ns3
