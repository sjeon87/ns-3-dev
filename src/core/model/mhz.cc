/*
 * Copyright (c) 2026 Tom Henderson
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include "mhz.h"

#include "log.h"
#include "unit-attribute-serialize.h"

/**
 * @file
 * @ingroup attribute_Mhz
 * ns3::MhzValue attribute value implementation.
 *
 * SerializeToString and DeserializeFromString use shared helpers from
 * unit-attribute-serialize.h to separate the display format from the
 * serialization format:
 *
 *   operator<<             "2400 MHz"  -- mp-units format, used in logs/traces
 *   SerializeToString      "2400_MHz"  -- single token, used by config/CommandLine
 *   DeserializeFromString  accepts "2400_MHz", "2400 MHz", or bare "2400"
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Mhz");

ATTRIBUTE_CHECKER_IMPLEMENT_WITH_CONVERTER(ns3::MHz_t, Mhz);
ATTRIBUTE_VALUE_IMPLEMENT_QUANTITY_VALUE(ns3::MHz_t, Mhz);

std::string
MhzValue::SerializeToString(Ptr<const AttributeChecker> checker) const
{
    return SerializeUnitValue(m_value.numerical_value_in(mp_units::si::mega<mp_units::si::hertz>),
                              "MHz");
}

bool
MhzValue::DeserializeFromString(std::string value, Ptr<const AttributeChecker> checker)
{
    double v;
    if (!DeserializeUnitValue(value, "MHz", v))
    {
        return false;
    }
    m_value = v * mp_units::si::mega<mp_units::si::hertz>;
    return true;
}

} // namespace ns3
