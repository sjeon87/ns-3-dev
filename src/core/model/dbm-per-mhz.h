/*
 * Copyright (c) 2026 Tom Henderson
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef NS3_DBM_PER_MHZ_H
#define NS3_DBM_PER_MHZ_H

#include "attribute-helper.h"
#include "attribute.h"
#include "double.h"
#include "units.h"

/**
 * @file
 * @ingroup attribute_DbmPerMhz
 * attribute value declaration for ns3::dBm_per_MHz_t (mp-units variant)
 *
 * wraps ns3::dBm_per_MHz_t
 */

namespace ns3
{

ATTRIBUTE_VALUE_DEFINE_WITH_NAME(ns3::dBm_per_MHz_t, DbmPerMhz);
ATTRIBUTE_ACCESSOR_DEFINE(DbmPerMhz);
ATTRIBUTE_CHECKER_DEFINE_WITH_CONVERTER(ns3::dBm_per_MHz_t, DbmPerMhz, Double);

} // namespace ns3

#endif // NS3_DBM_PER_MHZ_H
