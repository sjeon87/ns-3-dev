//
// Copyright (c) 2025
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Authors: Jiwoong Lee <porce@berkeley.edu>
//          Sébastien Deronne <sebastien.deronne@gmail.com>
//

/// @file si-units-ns3.h
/// @brief Main header for SI Units with ns-3 integration
/// This header provides SI units with ns-3-specific extensions:
/// - Attribute system support (si-units-attributes.h)
/// - ns-3 Time integration (units-frequency-nstime.h)
/// For basic SI units without ns-3 dependencies, use: include <si-units>
/// For ns-3 integration (attributes, Time), use: include "ns3/si-units-ns3.h"

#ifndef SI_UNITS_NS3_H
#define SI_UNITS_NS3_H


// Include base si-units library from third-party
#include <si-units>

// Include ns-3-specific extensions
#include "si-units-attributes.h"
#include "units-frequency-nstime.h"

#endif // SI_UNITS_NS3_H
