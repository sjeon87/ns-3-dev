/*
 * Copyright (c) 2026 Tom Henderson
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef NS3_DBM_H
#define NS3_DBM_H

#include "attribute-helper.h"
#include "attribute.h"
#include "double.h"
#include "units.h"

/**
 * @file
 * @ingroup attribute_Dbm
 * attribute value declaration for ns3::dBm_t (mp-units variant)
 *
 * wraps ns3::dBm_t
 */

namespace ns3
{

//  Additional docs for class DbmValue:
/**
 * Hold variables of type ns3::dBm_t (absolute power in decibel-milliwatts).
 *
 * The AttributeChecker accepts both DbmValue and DoubleValue (treating the
 * bare double as dBm) for backward compatibility with legacy attribute
 * settings.
 */
ATTRIBUTE_VALUE_DEFINE_WITH_NAME(ns3::dBm_t, Dbm);
ATTRIBUTE_ACCESSOR_DEFINE(Dbm);
ATTRIBUTE_CHECKER_DEFINE_WITH_CONVERTER(ns3::dBm_t, Dbm, Double);

// Documentation for the MakeDbmChecker with range bounds overload generated
// by ATTRIBUTE_CHECKER_DEFINE_WITH_CONVERTER (the unbounded MakeDbmChecker
// is documented by print-introspected-doxygen).
/**
 * @ingroup attribute_Dbm
 * @fn ns3::Ptr<const ns3::AttributeChecker> ns3::MakeDbmChecker(ns3::dBm_t min, ns3::dBm_t max)
 * Make a DbmChecker with a minimum and a maximum value.
 *
 * The minimum and maximum values are included in the allowed range.
 *
 * @param [in] min The minimum value.
 * @param [in] max The maximum value.
 * @returns The AttributeChecker.
 * @see AttributeChecker
 */

} // namespace ns3

#endif // NS3_DBM_H
