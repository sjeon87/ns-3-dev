/*
 * Copyright (c) 2026 Tom Henderson
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include "db.h"

#include "log.h"
#include "unit-attribute-serialize.h"

/**
 * @file
 * @ingroup attribute_Db
 * ns3::DbValue attribute value implementation.
 *
 * SerializeToString and DeserializeFromString use shared helpers from
 * unit-attribute-serialize.h to separate the display format from the
 * serialization format:
 *
 *   operator<<             "3 dB"    -- standard notation, used in logs/traces
 *   SerializeToString      "3_dB"    -- single token, used by config/CommandLine
 *   DeserializeFromString  accepts "3_dB", "3 dB", or bare "3"
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Db");

ATTRIBUTE_CHECKER_IMPLEMENT_WITH_CONVERTER(ns3::dB_t, Db);
ATTRIBUTE_VALUE_IMPLEMENT_QUANTITY_VALUE(ns3::dB_t, Db);

std::string
DbValue::SerializeToString(Ptr<const AttributeChecker> checker) const
{
    return SerializeUnitValue(m_value.numerical_value_in(dB), "dB");
}

bool
DbValue::DeserializeFromString(std::string value, Ptr<const AttributeChecker> checker)
{
    double v;
    if (!DeserializeUnitValue(value, "dB", v))
    {
        return false;
    }
    m_value = v * dB;
    return true;
}

} // namespace ns3
