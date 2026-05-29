/*
 * SPDX-License-Identifier: GPL-2.0-only and NIST-Software
 */

#include "time-tlv.h"

#include "ns3/assert.h"

#include <cmath>

namespace ns3
{

namespace manet
{

uint8_t
EncodeTimeCode(Time t, double c)
{
    NS_ASSERT_MSG(std::isfinite(c) && c > 0.0,
                  "RFC 5497 time granularity C must be a finite positive number of seconds");
    double tSec = t.GetSeconds();

    // The smallest representable time-value is C, encoded as time-code 0.
    if (tSec <= c)
    {
        return 0;
    }

    // RFC 5497, Sec. 5, step 1: find the largest integer b such that t/C >= 2^b.
    // Determined by integer search (b in [0, 31]) to avoid floating-point edge
    // cases around powers of two.
    uint32_t b = 0;
    while (b < 31 && (tSec / c) >= std::pow(2.0, static_cast<double>(b + 1)))
    {
        ++b;
    }

    // RFC 5497, Sec. 5, step 2: a := 8 * (t / (C * 2^b) - 1), rounded up.
    int a = static_cast<int>(
        std::ceil(8.0 * (tSec / (c * std::pow(2.0, static_cast<double>(b))) - 1.0)));

    // RFC 5497, Sec. 5, step 3: if a == 8, then b := b + 1 and a := 0.
    if (a >= 8)
    {
        b += 1;
        a = 0;
    }

    // RFC 5497, Sec. 5, step 4: if b > 31 the value is not representable; clamp
    // to the maximum representable time-code (b = 31, a = 7).
    if (b > 31)
    {
        b = 31;
        a = 7;
    }

    uint8_t code = static_cast<uint8_t>(8 * b + static_cast<uint32_t>(a));

    // The forward computation above uses floating point and can round to an
    // adjacent time-code at a power-of-two boundary, or for time-values not
    // representable as a double (e.g. 0.1 s). Align to the RFC 5497, Sec. 5
    // definition: the smallest time-code whose decoded value is >= t.
    // Time-codes are monotone in the represented time-value, and the comparison
    // is over integer-valued Time, so this correction is deterministic.
    while (code < 255 && DecodeTimeCode(code, c) < t)
    {
        ++code;
    }
    while (code > 0 && DecodeTimeCode(static_cast<uint8_t>(code - 1), c) >= t)
    {
        --code;
    }
    return code;
}

Time
DecodeTimeCode(uint8_t code, double c)
{
    NS_ASSERT_MSG(std::isfinite(c) && c > 0.0,
                  "RFC 5497 time granularity C must be a finite positive number of seconds");
    uint32_t b = code >> 3;   // 5 most significant bits: exponent
    uint32_t a = code & 0x07; // 3 least significant bits: mantissa
    double tSec = (1.0 + a / 8.0) * std::pow(2.0, static_cast<double>(b)) * c;
    return Seconds(tSec);
}

} // namespace manet

} // namespace ns3
