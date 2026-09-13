/*
 * Copyright (c) 2026 Tom Henderson
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef NS3_DB_H
#define NS3_DB_H

#include "attribute-helper.h"
#include "attribute.h"
#include "double.h"
#include "units.h"

/**
 * @file
 * @ingroup attribute_Db
 * attribute value declaration for ns3::dB_t (mp-units variant)
 *
 * wraps ns3::dB_t
 */

namespace ns3
{

//  Additional docs for class DbValue:
/**
 * Hold variables of type ns3::dB_t (logarithmic ratio in decibels).
 *
 * The AttributeChecker accepts both DbValue and DoubleValue (treating the
 * bare double as dB) for backward compatibility with legacy attribute
 * settings.
 */
ATTRIBUTE_VALUE_DEFINE_WITH_NAME(ns3::dB_t, Db);
ATTRIBUTE_ACCESSOR_DEFINE(Db);
ATTRIBUTE_CHECKER_DEFINE_WITH_CONVERTER(ns3::dB_t, Db, Double);

// Documentation for the MakeDbChecker with range bounds overload generated
// by ATTRIBUTE_CHECKER_DEFINE_WITH_CONVERTER (the unbounded MakeDbChecker
// is documented by print-introspected-doxygen).
/**
 * @ingroup attribute_Db
 * @fn ns3::Ptr<const ns3::AttributeChecker> ns3::MakeDbChecker(ns3::dB_t min, ns3::dB_t max)
 * Make a DbChecker with a minimum and a maximum value.
 *
 * The minimum and maximum values are included in the allowed range.
 *
 * @param [in] min The minimum value.
 * @param [in] max The maximum value.
 * @returns The AttributeChecker.
 * @see AttributeChecker
 */

} // namespace ns3

#endif // NS3_DB_H
