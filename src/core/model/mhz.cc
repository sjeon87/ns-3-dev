/*
 * Copyright (c) 2026 Tom Henderson
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include "mhz.h"

#include "fatal-error.h"
#include "log.h"

#include <sstream>

/**
 * @file
 * @ingroup attribute_Mhz
 * ns3::MhzValue attribute value implementation.
 *
 * SerializeToString and DeserializeFromString are implemented manually (rather
 * than via ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME) to separate the display format
 * from the serialization format:
 *
 *   operator<<             "2400 MHz"  -- mp-units format, used in logs/traces
 *   SerializeToString      "2400_MHz"  -- single token, used by config/CommandLine
 *   DeserializeFromString  accepts "2400_MHz", "2400 MHz", or bare "2400"
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Mhz");

ATTRIBUTE_CHECKER_IMPLEMENT_WITH_CONVERTER(ns3::MHz_t, Mhz);

// Constructor, Set, Get, Copy are identical to ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME.

MhzValue::MhzValue(const ns3::MHz_t& value)
    : m_value(value)
{
}

void
MhzValue::Set(const ns3::MHz_t& v)
{
    m_value = v;
}

ns3::MHz_t
MhzValue::Get() const
{
    return m_value;
}

Ptr<AttributeValue>
MhzValue::Copy() const
{
    return ns3::Create<MhzValue>(*this);
}

std::string
MhzValue::SerializeToString(Ptr<const AttributeChecker> checker) const
{
    std::ostringstream oss;
    oss << m_value.numerical_value_in(mp_units::si::mega<mp_units::si::hertz>) << "_MHz";
    return oss.str();
}

bool
MhzValue::DeserializeFromString(std::string value, Ptr<const AttributeChecker> checker)
{
    if (value.empty())
    {
        m_value = MHz_t{};
        return true;
    }
    std::istringstream iss(value);
    double v;
    iss >> v;
    if (iss.fail())
    {
        return false;
    }
    if (iss.eof())
    {
        // Bare number "2400" -- assumed MHz.
        m_value = v * mp_units::si::mega<mp_units::si::hertz>;
        return true;
    }
    // Optional suffix: "_MHz" (token format) or "MHz" (space-separated standard notation).
    std::string suffix;
    iss >> suffix;
    if (!iss.eof())
    {
        return false; // unexpected trailing content
    }
    if (!suffix.empty() && suffix[0] == '_')
    {
        suffix = suffix.substr(1);
    }
    if (suffix != "MHz")
    {
        return false;
    }
    m_value = v * mp_units::si::mega<mp_units::si::hertz>;
    return true;
}

} // namespace ns3
