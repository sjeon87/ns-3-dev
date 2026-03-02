/*
 * Copyright (c) 2026 Tom Henderson
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

// Minimal stub to verify mp-units CMake integration compiles
#include <iostream>
#include <mp-units/systems/isq.h>
#include <mp-units/systems/si.h>

using namespace mp_units;
using namespace mp_units::si::unit_symbols;

int
main(int argc, char** argv)
{
    quantity distance = 8.0 * m;
    quantity freq = 5.0 * MHz;
    std::cout << "Test of mp-units CMake integration" << std::endl;
    std::cout << "distance = " << distance << std::endl;
    std::cout << "freq = " << freq << std::endl;
    return 0;
}
