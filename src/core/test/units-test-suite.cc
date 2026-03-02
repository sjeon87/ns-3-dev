/*
 * Copyright (c) 2026 Tom Henderson
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

// Stub test suite - will be filled in next step

#include "ns3/test.h"

namespace ns3
{
namespace tests
{

class UnitsTestSuite : public TestSuite
{
  public:
    UnitsTestSuite();
};

UnitsTestSuite::UnitsTestSuite()
    : TestSuite("units", Type::UNIT)
{
}

static UnitsTestSuite unitsTestSuite_g;

} // namespace tests
} // namespace ns3
