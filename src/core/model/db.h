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

ATTRIBUTE_VALUE_DEFINE_WITH_NAME(ns3::dB_t, Db);
ATTRIBUTE_ACCESSOR_DEFINE(Db);
ATTRIBUTE_CHECKER_DEFINE_WITH_CONVERTER(ns3::dB_t, Db, Double);

} // namespace ns3

#endif // NS3_DB_H
