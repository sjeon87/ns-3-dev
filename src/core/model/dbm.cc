/*
 * Copyright (c) 2026 Tom Henderson
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include "dbm.h"

#include "log.h"
#include "unit-attribute-serialize.h"

/**
 * @file
 * @ingroup attribute_Dbm
 * ns3::DbmValue attribute value implementation.
 *
 * SerializeToString and DeserializeFromString use shared helpers from
 * unit-attribute-serialize.h to separate the display format from the
 * serialization format:
 *
 *   operator<<             "20 dBm"  -- standard notation, used in logs/traces
 *   SerializeToString      "20_dBm"  -- single token, used by config/CommandLine
 *   DeserializeFromString  accepts "20_dBm", "20 dBm", or bare "20"
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Dbm");

ATTRIBUTE_CHECKER_IMPLEMENT_WITH_CONVERTER(ns3::dBm_t, Dbm);
ATTRIBUTE_VALUE_IMPLEMENT_QUANTITY_VALUE(ns3::dBm_t, Dbm);

std::string
DbmValue::SerializeToString(Ptr<const AttributeChecker> checker) const
{
    return SerializeUnitValue(m_value.numerical_value_in(dBm), "dBm");
}

bool
DbmValue::DeserializeFromString(std::string value, Ptr<const AttributeChecker> checker)
{
    double v;
    if (!DeserializeUnitValue(value, "dBm", v))
    {
        return false;
    }
    m_value = v * dBm;
    return true;
}

} // namespace ns3
