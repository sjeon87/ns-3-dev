/*
 * Copyright (c) 2026 Universita' di Firenze, Italy
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#ifndef IANA_INTERNET_PROTOCOL_NUMBERS_H
#define IANA_INTERNET_PROTOCOL_NUMBERS_H

#include <cstdint>

namespace ns3
{
namespace iana
{
/**
 * Centralized definitions for Internet protocol numbers.
 * This enumeration contains the Internet protocol numbers as defined by IANA.
 * References:
 * - https://www.iana.org/assignments/protocol-numbers/protocol-numbers.xhtml
 */
enum InternetProtocolNumbers : uint16_t
{
    HOPOPT = 0,      //!< IPv6 Hop-by-Hop Option [RFC8200]
    ICMP = 1,        //!< ICMP [RFC792]
    TCP = 6,         //!< TCP [RFC9293]
    UDP = 17,        //!< UDP [RFC768]
    DSR = 48,        //!< DSR [RFC4728]
    ICMPv6 = 58,     //!< ICMP for IPv6 [RFC8200]
    IPv6_NoNxt = 59, //!< No Next Header for IPv6 [RFC8200]
};
} // namespace iana
} // namespace ns3

#endif // IANA_INTERNET_PROTOCOL_NUMBERS_H
