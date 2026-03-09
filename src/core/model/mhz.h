/*
 * Copyright (c) 2026 Tom Henderson
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef NS3_MHZ_H
#define NS3_MHZ_H

#include "attribute-helper.h"
#include "attribute.h"
#include "double.h"
#include "uinteger.h"
#include "units.h"

/**
 * @file
 * @ingroup attribute_Mhz
 * attribute value declaration for ns3::MHz_t (mp-units variant)
 *
 * wraps ns3::MHz_t
 */

namespace ns3
{

ATTRIBUTE_VALUE_DEFINE_WITH_NAME(ns3::MHz_t, Mhz);
ATTRIBUTE_ACCESSOR_DEFINE(Mhz);

/**
 * @ingroup attribute_Mhz
 * AttributeChecker for MhzValue.
 *
 * Accepts both MhzValue and DoubleValue (treating the bare double as MHz,
 * for backward compatibility with legacy attribute settings).
 */
class MhzChecker : public AttributeChecker
{
    Ptr<AttributeValue> CreateValidValue(const AttributeValue& value) const override
    {
        // Accept DoubleValue: treat numeric value as MHz (backward compatibility)
        const auto dblPtr = dynamic_cast<const DoubleValue*>(&value);
        if (dblPtr)
        {
            return AttributeChecker::CreateValidValue(MhzValue(dblPtr->Get() * MHz));
        }
        // Accept UintegerValue: treat integer value as MHz (backward compatibility)
        const auto uintPtr = dynamic_cast<const UintegerValue*>(&value);
        if (uintPtr)
        {
            return AttributeChecker::CreateValidValue(
                MhzValue(static_cast<double>(uintPtr->Get()) * MHz));
        }
        return AttributeChecker::CreateValidValue(value);
    }
};

Ptr<const AttributeChecker> MakeMhzChecker();
Ptr<const AttributeChecker> MakeMhzChecker(ns3::MHz_t min, ns3::MHz_t max);

} // namespace ns3

#endif // NS3_MHZ_H
