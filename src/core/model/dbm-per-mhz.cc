/*
 * Copyright (c) 2026 Tom Henderson
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include "dbm-per-mhz.h"

#include "fatal-error.h"
#include "log.h"

#include <sstream>

/**
 * @file
 * @ingroup attribute_DbmPerMhz
 * ns3::DbmPerMhzValue attribute value implementation.
 *
 * SerializeToString and DeserializeFromString are implemented manually (rather
 * than via ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME) to separate the display format
 * from the serialization format:
 *
 *   operator<<             "-20 dBm/MHz"       -- standard notation, used in logs/traces
 *   SerializeToString      "-20_dBm_per_MHz"   -- single token, used by config/CommandLine
 *   DeserializeFromString  accepts "-20_dBm_per_MHz", "-20 dBm_per_MHz", or bare "-20"
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DbmPerMhz");

ATTRIBUTE_CHECKER_IMPLEMENT_WITH_CONVERTER(ns3::dBm_per_MHz_t, DbmPerMhz);

// Constructor, Set, Get, Copy are identical to ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME.

DbmPerMhzValue::DbmPerMhzValue(const ns3::dBm_per_MHz_t& value)
    : m_value(value)
{
}

void
DbmPerMhzValue::Set(const ns3::dBm_per_MHz_t& v)
{
    m_value = v;
}

ns3::dBm_per_MHz_t
DbmPerMhzValue::Get() const
{
    return m_value;
}

Ptr<AttributeValue>
DbmPerMhzValue::Copy() const
{
    return ns3::Create<DbmPerMhzValue>(*this);
}

std::string
DbmPerMhzValue::SerializeToString(Ptr<const AttributeChecker> checker) const
{
    std::ostringstream oss;
    oss << m_value.numerical_value_in(dBm_per_MHz) << "_dBm_per_MHz";
    return oss.str();
}

bool
DbmPerMhzValue::DeserializeFromString(std::string value, Ptr<const AttributeChecker> checker)
{
    if (value.empty())
    {
        m_value = dBm_per_MHz_t{};
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
        // Bare number "-20" -- assumed dBm/MHz.
        m_value = v * dBm_per_MHz;
        return true;
    }
    // Optional suffix: "_dBm_per_MHz" (token format) or "dBm_per_MHz" (space-separated).
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
    if (suffix != "dBm_per_MHz")
    {
        return false;
    }
    m_value = v * dBm_per_MHz;
    return true;
}

} // namespace ns3
