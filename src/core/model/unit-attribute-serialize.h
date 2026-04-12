/*
 * Copyright (c) 2026 Tom Henderson
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef NS3_UNIT_ATTRIBUTE_SERIALIZE_H
#define NS3_UNIT_ATTRIBUTE_SERIALIZE_H

#include <string>

/**
 * @file
 * @ingroup attributes
 * Shared serialization helpers for unit-based AttributeValue classes
 * (DbValue, DbmValue, MhzValue, DbmPerMhzValue, etc.).
 *
 * These functions factor out the common string-parsing logic so that each
 * unit wrapper only needs to supply its own numeric extraction/construction.
 */

namespace ns3
{

/**
 * Serialize a numeric value with a unit suffix.
 *
 * Produces the single-token format used by Config and CommandLine,
 * e.g. "20_dBm", "2400_MHz".
 *
 * @param numericValue The numeric value to serialize.
 * @param suffix The unit suffix (e.g. "dBm", "MHz", "dB", "dBm_per_MHz").
 * @return The serialized string.
 */
std::string SerializeUnitValue(double numericValue, const char* suffix);

/**
 * Deserialize a string into a bare numeric value, validating an optional
 * unit suffix.
 *
 * Accepted formats:
 *   - Empty string: sets outValue to 0.0, returns true.
 *   - Bare number "20": sets outValue to 20.0, returns true.
 *   - Number with suffix "20_dBm" or "20 dBm": validates suffix, returns true.
 *   - Anything else: returns false.
 *
 * @param str The string to parse.
 * @param expectedSuffix The expected unit suffix (e.g. "dBm", "MHz").
 * @param[out] outValue The parsed numeric value.
 * @return true if parsing succeeded.
 */
bool DeserializeUnitValue(const std::string& str, const char* expectedSuffix, double& outValue);

} // namespace ns3

#endif // NS3_UNIT_ATTRIBUTE_SERIALIZE_H
