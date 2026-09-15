/*
 * Copyright (c) 2025 Universita' di Firenze, Italy
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "ipv6-network-address.h"

#include "ns3/assert.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv6NetworkAddress");

Ipv6NetworkAddress::Ipv6NetworkAddress(Ipv6Address address, uint8_t networkLength)
{
    NS_LOG_FUNCTION(this << address << +networkLength);
    NS_ASSERT_MSG(networkLength <= MAX_PREFIX_LENGTH, "Invalid network length");
    m_address = address;
    m_networkLength = networkLength;
}

uint8_t
Ipv6NetworkAddress::GetType()
{
    static uint8_t type = Address::Register("IpAddress", ADDRESS_LENGTH);
    return type;
}

bool
Ipv6NetworkAddress::IsMatchingType(const Address& address)
{
    return address.CheckCompatible(GetType(), ADDRESS_LENGTH);
}

Ipv6NetworkAddress::
operator Address() const
{
    return ConvertTo();
}

Ipv6NetworkAddress
Ipv6NetworkAddress::ConvertFrom(const Address& address)
{
    NS_LOG_FUNCTION(address);
    NS_ASSERT(address.CheckCompatible(GetType(), ADDRESS_LENGTH));
    uint8_t buf[ADDRESS_LENGTH];
    address.CopyTo(buf);
    Ipv6NetworkAddress addr;
    addr.m_address = Ipv6Address::Deserialize(buf);
    addr.m_networkLength = buf[16];

    return addr;
}

Address
Ipv6NetworkAddress::ConvertTo() const
{
    NS_LOG_FUNCTION(this);
    uint8_t buf[ADDRESS_LENGTH];
    m_address.Serialize(buf);
    buf[16] = m_networkLength;
    return Address(GetType(), buf, ADDRESS_LENGTH);
}

Ipv6Address
Ipv6NetworkAddress::GetAddress() const
{
    return m_address;
}

uint8_t
Ipv6NetworkAddress::GetNetworkLength() const
{
    return m_networkLength;
}

Ipv6NetworkAddress
Ipv6NetworkAddress::GetNetwork() const
{
    if (!m_networkAddr)
    {
        if (m_networkLength == MAX_PREFIX_LENGTH)
        {
            m_networkAddr = m_address;
        }
        else if (m_networkLength == 0)
        {
            m_networkAddr = Ipv6Address::GetAny();
        }
        else
        {
            uint8_t network[16];
            uint8_t fullBytes = m_networkLength / 8;
            uint8_t remainingBits = m_networkLength % 8;

            m_address.GetBytes(network);

            if (remainingBits > 0)
            {
                // If remaining bits exist, apply bitmask to the next byte and zero the others.
                auto mask = static_cast<uint8_t>(0xFF << (8 - remainingBits));
                network[fullBytes] &= mask;
                std::fill(network + fullBytes + 1, network + 16, 0);
            }
            else
            {
                // network length is a multiple of 8, just zero the irrelevant part.
                std::fill(network + fullBytes, network + 16, 0);
            }

            m_networkAddr.emplace(network);
        }
    }

    return Ipv6NetworkAddress(*m_networkAddr, m_networkLength);
}

void
Ipv6NetworkAddress::MakeNetwork()
{
    m_address = GetNetwork().GetAddress();
}

bool
Ipv6NetworkAddress::Includes(const Ipv6NetworkAddress& other) const
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

    Ipv6NetworkAddress compared = Ipv6NetworkAddress(other.GetAddress(), GetNetworkLength());

    return compared.GetNetwork() == GetNetwork();
}

std::ostream&
operator<<(std::ostream& os, const Ipv6NetworkAddress& addr)
{
    os << addr.GetAddress() << "/" << static_cast<uint32_t>(addr.GetNetworkLength());
    return os;
}

} // namespace ns3
