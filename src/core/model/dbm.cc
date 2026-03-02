/*
 * Copyright (c) 2026 Tom Henderson
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include "dbm.h"

#include "fatal-error.h"
#include "log.h"

#include <sstream>

/**
 * @file
 * @ingroup attribute_Dbm
 * ns3::DbmValue attribute value implementation.
 *
 * SerializeToString and DeserializeFromString are implemented manually (rather
 * than via ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME) to separate the display format
 * from the serialization format:
 *
 *   operator<<             "20 dBm"  -- standard notation, used in logs/traces
 *   SerializeToString      "20_dBm"  -- single token, used by config/CommandLine
 *   DeserializeFromString  accepts "20_dBm", "20 dBm", or bare "20"
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Dbm");

ATTRIBUTE_CHECKER_IMPLEMENT_WITH_CONVERTER(ns3::dBm_t, Dbm);

// Constructor, Set, Get, Copy are identical to ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME.

DbmValue::DbmValue(const ns3::dBm_t& value)
    : m_value(value)
{
}

void
DbmValue::Set(const ns3::dBm_t& v)
{
    m_value = v;
}

ns3::dBm_t
DbmValue::Get() const
{
    return m_value;
}

Ptr<AttributeValue>
DbmValue::Copy() const
{
    return ns3::Create<DbmValue>(*this);
}

std::string
DbmValue::SerializeToString(Ptr<const AttributeChecker> checker) const
{
    std::ostringstream oss;
    oss << m_value.numerical_value_in(dBm) << "_dBm";
    return oss.str();
}

bool
DbmValue::DeserializeFromString(std::string value, Ptr<const AttributeChecker> checker)
{
    if (value.empty())
    {
        m_value = dBm_t{};
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
        // Bare number "20" -- assumed dBm.
        m_value = v * dBm;
        return true;
    }
    // Optional suffix: "_dBm" (token format) or "dBm" (space-separated standard notation).
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
    if (suffix != "dBm")
    {
        return false;
    }
    m_value = v * dBm;
    return true;
}

} // namespace ns3
