/*
 * Copyright (c) 2025 Universita' di Firenze, Italy
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#ifndef IPV4_NETWORK_ADDRESS_H
#define IPV4_NETWORK_ADDRESS_H

#include "ipv4-address.h"

#include <compare>
#include <optional>
#include <ostream>
#include <stdint.h>

namespace ns3
{

/**
 * @ingroup address
 * @ingroup ipv4
 *
 * @brief a class to store IPv4 network address information, i.e.,
 * an address and the number of bits in the network part.
 *
 * This class can be used to represent either an address and its
 * network mask (e.g., 169.254.10.42/24), or a network (e.g., 169.254.10.0/24).
 */
class Ipv4NetworkAddress
{
  public:
    /**
     * Address length when converted to an Address[bytes].
     * In this case it's 18: 4 (IPv4 address) + 1 (prefix length)
     */
    static constexpr uint8_t ADDRESS_LENGTH = 5;

    Ipv4NetworkAddress() = default;

    /**
     * @brief Configure address, mask and broadcast address.
     * @param address the address
     * @param networkLength the CIDR size
     */
    Ipv4NetworkAddress(Ipv4Address address, uint8_t networkLength = MAX_PREFIX_LENGTH);

    /**
     * @brief Return the Type of address.
     * @return type of address
     */
    static uint8_t GetType();

    /**
     * @brief If the Address matches the type.
     * @param address other address
     * @return true if the type matches, false otherwise
     */
    static bool IsMatchingType(const Address& address);

    /**
     * @brief Convert to Address object
     */
    operator Address() const;

    /**
     * @brief Convert the Address object into an Ipv4NetworkAddress.
     * @param address address to convert
     * @return an Ipv4NetworkAddress
     */
    static Ipv4NetworkAddress ConvertFrom(const Address& address);

    /**
     * @brief convert the Ipv4NetworkAddress object to an Address object.
     * @return the Address object corresponding to this object.
     */
    Address ConvertTo() const;

    /**
     * @brief Get the IPv4 address
     * @returns the IPv4 address
     */
    Ipv4Address GetAddress() const;

    /**
     * @brief Get the network mask
     *
     * This generates the network mask from the network length,
     * e.g., a "/24" generates "255.255.255.0".
     *
     * This function is useful only for printing.
     * It returns an `Ipv4Address` because `Ipv4Mask`
     * is discouraged.
     *
     * @returns An IPv4Address representing the mask
     */
    Ipv4Address GetMask() const;

    /**
     * @brief Get the network part length (in bits)
     * @returns the network part length (in bits)
     */
    uint8_t GetNetworkLength() const;

    /**
     * @brief Get the network address
     *
     * The network address is made by the first n bits of the address,
     * where n is the NetworkLength.
     *
     * @returns the network address
     */
    Ipv4NetworkAddress GetNetwork() const;

    /**
     * @brief Clean the entry so that this is a pure network address.
     */
    void MakeNetwork();

    /**
     * @brief Checks if a network is a subnet of another network.
     *
     * As an example, 192.168.1.0/24 is contained in 192.168.0.0/16
     *
     * This can be used to check if an address is part of a network,
     * because an address is automatically converted into an address with
     * a /32 mask, e.g., "192.168.1.1/32"
     *
     * Note: The comparison is based on the network length of the object.
     * The operand network length is irrelevant. I.e.,
     * "169.254.10.2/16" will include "169.254.20.42/24".
     *
     * @param other the network to check
     * @return true if the network is a subnet of this network.
     */
    bool Includes(const Ipv4NetworkAddress& other) const;

    /// Maximum prefix length.
    static constexpr uint16_t MAX_PREFIX_LENGTH = 32;

    /**
     * @brief Equality comparison operator.
     *
     * @param other the other address to compare with
     * @return true if the addresses and network lengths are equal
     */
    constexpr bool operator==(const Ipv4NetworkAddress& other) const
    {
        return m_address == other.m_address && m_networkLength == other.m_networkLength;
    }

    /**
     * @brief Three-way comparison operator.
     *
     * The ordering is address-first, length-second.
     *
     * @param other the other address to compare with
     * @return comparison result
     */
    constexpr std::strong_ordering operator<=>(const Ipv4NetworkAddress& other) const
    {
        if (m_address == other.m_address)
        {
            return m_networkLength <=> other.m_networkLength;
        }

        return m_address <=> other.m_address;
    }

  private:
    /**
     * @brief Calculate the mask value.
     *
     * @return the mask value.
     */
    uint32_t GetMaskValue() const;

    // Initialized by default to "0.0.0.0/0", to enable checks on correct initialization.
    Ipv4Address m_address;                            //!< Address
    uint8_t m_networkLength{0};                       //!< Network length
    mutable std::optional<Ipv4Address> m_networkAddr; //!< Cached network address
};

/**
 * @brief Stream insertion operator. The address is printed in CIDR notation,
 * e.g., 169.254.10.42/24
 *
 * @param os the reference to the output stream
 * @param addr the Ipv4InterfaceAddress
 * @returns the reference to the output stream
 */
std::ostream& operator<<(std::ostream& os, const Ipv4NetworkAddress& addr);

} // namespace ns3

#endif /* IPV4_NETWORK_ADDRESS_H */
