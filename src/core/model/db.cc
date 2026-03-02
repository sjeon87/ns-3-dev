/*
 * Copyright (c) 2026 Tom Henderson
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include "db.h"

#include "fatal-error.h"
#include "log.h"

#include <sstream>

/**
 * @file
 * @ingroup attribute_Db
 * ns3::DbValue attribute value implementation.
 *
 * SerializeToString and DeserializeFromString are implemented manually (rather
 * than via ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME) to separate the display format
 * from the serialization format:
 *
 *   operator<<             "3 dB"    -- standard notation, used in logs/traces
 *   SerializeToString      "3_dB"    -- single token, used by config/CommandLine
 *   DeserializeFromString  accepts "3_dB", "3 dB", or bare "3"
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Db");

ATTRIBUTE_CHECKER_IMPLEMENT_WITH_CONVERTER(ns3::dB_t, Db);

// Constructor, Set, Get, Copy are identical to ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME.

DbValue::DbValue(const ns3::dB_t& value)
    : m_value(value)
{
}

void
DbValue::Set(const ns3::dB_t& v)
{
    m_value = v;
}

ns3::dB_t
DbValue::Get() const
{
    return m_value;
}

Ptr<AttributeValue>
DbValue::Copy() const
{
    return ns3::Create<DbValue>(*this);
}

std::string
DbValue::SerializeToString(Ptr<const AttributeChecker> checker) const
{
    std::ostringstream oss;
    oss << m_value.numerical_value_in(dB) << "_dB";
    return oss.str();
}

bool
DbValue::DeserializeFromString(std::string value, Ptr<const AttributeChecker> checker)
{
    if (value.empty())
    {
        m_value = dB_t{};
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
        // Bare number "3" -- assumed dB.
        m_value = v * dB;
        return true;
    }
    // Optional suffix: "_dB" (token format) or "dB" (space-separated standard notation).
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
    if (suffix != "dB")
    {
        return false;
    }
    m_value = v * dB;
    return true;
}

} // namespace ns3
