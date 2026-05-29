/*
 * SPDX-License-Identifier: NIST-Software
 */

#ifndef TIME_TLV_H
#define TIME_TLV_H

#include "ns3/nstime.h"

#include <cstdint>

/**
 * @file
 * @ingroup manet
 *
 * RFC 5497 ("Representing Multi-Value Time in MANETs") time-code support.
 *
 * RFC 5497 represents a time-value in a single 8-bit time-code, where the 5
 * most significant bits are an exponent (b) and the 3 least significant bits
 * are a mantissa (a), so that:
 *
 *     time-value := (1 + a/8) * 2^b * C
 *     time-code  := 8 * b + a
 *
 * The granularity constant C is protocol-defined; RFC 6130 (NHDP), Sec. 18,
 * specifies C := 1/1024 second, which is the default value used here. The
 * encode/decode helpers accept C as a defaulted argument so that another
 * RFC 5444-based protocol defining a different C can reuse them; RFC 5497
 * requires C > 0 and that all nodes in the MANET use the same value.
 *
 * These helpers are deliberately protocol-neutral (they live in the manet
 * namespace rather than being tied to NHDP) so that other RFC 5444-based
 * protocols (e.g. OLSRv2) can reuse them.
 */

namespace ns3
{

namespace manet
{

/**
 * RFC 5497 time granularity constant C, in seconds (RFC 6130, Sec. 18).
 *
 * Note: We use double type here rather than ns3::Time because 1/1024 is not
 * fully representable as a Time with ns-3's default nanosecond resolution.
 */
constexpr double TIME_TLV_C = 1.0 / 1024.0;

/**
 * Encode a time-value as an RFC 5497 8-bit time-code.
 *
 * Implements the RFC 5497, Sec. 5 algorithm for the smallest representable
 * time-value not less than t. Values at or below C encode to time-code 0
 * (the minimum representable value, C). Values above the maximum representable
 * time-value (15 * 2^28 * C) are clamped to time-code 255.
 *
 * @param t The time-value to encode.
 * @param c The time granularity constant C, in seconds (must be finite and > 0).
 * @return The 8-bit time-code.
 */
uint8_t EncodeTimeCode(Time t, double c = TIME_TLV_C);

/**
 * Decode an RFC 5497 8-bit time-code into a time-value.
 *
 * @param code The 8-bit time-code.
 * @param c The time granularity constant C, in seconds (must be finite and > 0).
 * @return The represented time-value, (1 + a/8) * 2^b * C.
 */
Time DecodeTimeCode(uint8_t code, double c = TIME_TLV_C);

} // namespace manet

} // namespace ns3

#endif /* TIME_TLV_H */
