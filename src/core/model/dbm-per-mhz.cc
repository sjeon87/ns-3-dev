/*
 * Copyright (c) 2026 Tom Henderson
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include "dbm-per-mhz.h"

#include "log.h"
#include "unit-attribute-serialize.h"

/**
 * @file
 * @ingroup attribute_DbmPerMhz
 * ns3::DbmPerMhzValue attribute value implementation.
 *
 * SerializeToString and DeserializeFromString use shared helpers from
 * unit-attribute-serialize.h to separate the display format from the
 * serialization format:
 *
 *   operator<<             "-20 dBm/MHz"       -- standard notation, used in logs/traces
 *   SerializeToString      "-20_dBm_per_MHz"   -- single token, used by config/CommandLine
 *   DeserializeFromString  accepts "-20_dBm_per_MHz", "-20 dBm_per_MHz", or bare "-20"
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DbmPerMhz");

ATTRIBUTE_CHECKER_IMPLEMENT_WITH_CONVERTER(ns3::dBm_per_MHz_t, DbmPerMhz);
ATTRIBUTE_VALUE_IMPLEMENT_QUANTITY_VALUE(ns3::dBm_per_MHz_t, DbmPerMhz);

std::string
DbmPerMhzValue::SerializeToString(Ptr<const AttributeChecker> checker) const
{
    return SerializeUnitValue(m_value.numerical_value_in(dBm_per_MHz), "dBm_per_MHz");
}

bool
DbmPerMhzValue::DeserializeFromString(std::string value, Ptr<const AttributeChecker> checker)
{
    double v;
    if (!DeserializeUnitValue(value, "dBm_per_MHz", v))
    {
        return false;
    }
    m_value = v * dBm_per_MHz;
    return true;
}

} // namespace ns3
