/*
 * Copyright (c) 2026 Tom Henderson
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/command-line.h"
#include "ns3/db.h"
#include "ns3/dbm-per-mhz.h"
#include "ns3/mhz.h"
#include "ns3/units.h"

#include <iostream>

/**
 * @file
 * @ingroup unit-examples
 * Demonstrate use and capabilities of mp-units in ns-3.
 */

using namespace ns3;
using namespace mp_units::si::unit_symbols;

// Program formatted to produce Markdown text
int
main(int argc, char** argv)
{
    dB_t cmdLineDecibel = 3.0 * dB;
    ns3::CommandLine cmd(__FILE__);
    cmd.Usage("This program demonstrates strongly-typed quantities using mp-units in ns-3.");
    cmd.AddValue("cmdLineDecibel", "Decibel variable for command line", cmdLineDecibel);
    cmd.Parse(argc, argv);

    std::cout << std::endl;
    std::cout << "## mp-units ns-3 demo\n\n";
    std::cout << "This example demonstrates mp-units, a C++20 quantities and units library\n";
    std::cout << "targeting C++29 standardization.\n\n";
    std::cout << "To run this example (from src/core/examples directory) yourself, type:\n\n";
    std::cout << "    ./ns3 run units-example\n";
    std::cout << std::endl;

    // =========================================================================
    // Linear units via mp-units
    // =========================================================================

    std::cout << "### Linear units\n\n";
    std::cout << "mp-units uses a multiplication syntax:\n\n";
    std::cout << "    auto distance = 8.0 * m;\n";
    std::cout << "    auto freq = 5.0 * MHz;\n";
    std::cout << "    auto power = 100.0 * mW;\n\n";

    auto distance = 8.0 * m;
    auto freq = 5.0 * MHz;
    auto power = 100.0 * mW;

    std::cout << "    std::cout << distance = << distance << std::endl;\n";
    std::cout << "    std::cout << freq = << freq << std::endl;\n";
    std::cout << "    std::cout << power = << power << std::endl;\n\n";

    std::cout << "Running the above code produces:\n\n";
    std::cout << "distance = " << distance << std::endl;
    std::cout << "freq = " << freq << std::endl;
    std::cout << "power = " << power << std::endl;
    std::cout << std::endl;

    // =========================================================================
    // Types and type aliases
    // =========================================================================
    std::cout << "### Types deduction and aliases\n\n";
    std::cout << "The auto type deduction is deducing the following types:\n\n";
    std::cout << "    mp_units::quantity<mp_units::si::metre, double> frequency;\n";
    std::cout
        << "    mp_units::quantity<mp_units::si::mega<mp_units::si::hertz>, double> txPower;\n";
    std::cout
        << "    mp_units::quantity<mp_units::si::milli<mp_units::si::watt>, double> range;\n\n";

    std::cout << "ns-3 provides type aliases in namespace ns3 for these commonly used types:\n\n";
    std::cout << "    MHz_t frequency{2400.0 * MHz};\n";
    std::cout << "    Watt_t txPower{1.0 * W};\n";
    std::cout << "    meter_t range{100.0 * m};\n\n";

    MHz_t frequency{2400.0 * MHz};
    Watt_t txPower{1.0 * W};
    meter_t range{100.0 * m};

    std::cout << "    std::cout << frequency = << frequency << std::endl;\n";
    std::cout << "    std::cout << txPower = << txPower << std::endl;\n";
    std::cout << "    std::cout << range = << range << std::endl;\n\n";

    std::cout << "Running the above code produces:\n\n";
    std::cout << "frequency = " << frequency << std::endl;
    std::cout << "txPower = " << txPower << std::endl;
    std::cout << "range = " << range << std::endl;
    std::cout << std::endl;

    // =========================================================================
    // Additional type aliases
    // =========================================================================

    std::cout << "### Additional Type Aliases\n\n";
    std::cout << "ns-3 also provides these additional aliases:\n\n";
    std::cout << "    scalar_t     ratio{d1 / d2};                 // dimensionless quantity\n";
    std::cout << "    degree_t     beamwidth{60.0 * deg};          // angle in degrees\n";
    std::cout
        << "    dBr_t        relativeLevel{3};               // alias for dB_t (relative dB)\n";
    std::cout << "    mW_per_Hz_t  noisePsd{1.0e-9 * mW / Hz};    // linear PSD in mW/Hz\n";
    std::cout << "    mW_per_MHz_t channelPsd{1.0e-3 * mW / MHz}; // linear PSD in mW/MHz\n\n";

    std::cout << "degree_t demonstrates an important detail about mp-units stream output.\n";
    std::cout << "All ns-3 mp-units aliases except degree_t use ASCII-safe unit symbols\n";
    std::cout << "(MHz, mW, Hz, W, m, etc.). degree is the sole exception: its symbol is\n";
    std::cout << "U+00B0 DEGREE SIGN, and mp-units defaults operator<< to UTF-8 output.\n\n";

    degree_t beamwidth{60.0 * deg};

    std::cout << "    degree_t beamwidth{60.0 * deg};\n";
    std::cout << "    std::cout << beamwidth;\n\n";
    std::cout << "Running the above produces (UTF-8 degree sign, no preceding space):\n\n";
    std::cout << "beamwidth = " << beamwidth << "\n\n";
    std::cout << "mp-units specializes space_before_unit_symbol<non_si::degree> = false,\n";
    std::cout << "so no space is inserted between the numeric value and the degree symbol.\n\n";

    std::cout << "For ASCII-only output, use std::format with the ::U[P] sub-entity spec,\n";
    std::cout << "which selects character_set::portable for the unit symbol:\n\n";
    std::cout << "    std::format(\"{::U[P]}\", beamwidth)  // portable: no UTF-8 degree sign\n\n";
    std::cout << "Portable output: " << std::format("{::U[P]}", beamwidth) << "\n\n";
    std::cout << "::U[P] is an mp-units format extension, not standard C++. The outer :\n";
    std::cout << "enters the defaults-specs block; U[P] applies format spec P (portable)\n";
    std::cout << "to the unit sub-entity. Since space_before_unit_symbol<degree> = false,\n";
    std::cout << "the portable output is \"60deg\" with no space, matching the UTF-8 form.\n\n";

    std::cout << "dBr_t is a straight alias for dB_t used as a naming convention for\n";
    std::cout << "relative levels (e.g., antenna gain relative to isotropic). It carries\n";
    std::cout << "no additional type-safety over dB_t.\n\n";

    std::cout
        << "The key design feature of this library is visible in the expression \"8.0 * m\"\n\n";
    std::cout << "This means literally \"8.0 times the unit meter,\" which yields a quantity of\n";
    std::cout << "dimension length. The * is standard arithmetic multiplication, just overloaded\n";
    std::cout << "so that one operand is a unit object rather than a dimensionless scalar.\n\n";

    std::cout << "In other units libraries, such as nholthaus, user-defined literals (UDLs)\n";
    std::cout
        << "are used, such as \"8.0_m.\" UDLs require every unit to be registered as a literal\n";
    std::cout
        << "suffix at compile time, and they can't express derived or scaled units as cleanly.\n";
    std::cout << "With mp-units you can naturally write things like \"9.8 * m / (s * s)\" for\n";
    std::cout << "acceleration using only standard arithmetic operators.\n\n";

    // =========================================================================
    // Numeric value extraction
    // =========================================================================

    std::cout << "### Extracting Numeric Values\n\n";
    std::cout << "mp-units uses numerical_value_in() to extract the underlying value:\n\n";
    std::cout << "    auto distValue = distance.numerical_value_in(m);  // 8.0\n\n";

    auto distValue = distance.numerical_value_in(m);

    std::cout << "Printing this out yields:\n\n";
    std::cout << "distValue = " << distValue << " (raw double)\n\n";

    std::cout << "There are some key points to know about extraction:\n\n";
    std::cout << "1. The representation type is controlled by the numeric literal used\n";
    std::cout << "   in construction. operator* deduces the representation (Rep) from\n";
    std::cout << "   the left-hand operand:\n\n";
    std::cout << "       auto d1 = 8.0 * m;   // quantity<si::metre, double>\n";
    std::cout << "       auto d2 = 8.0f * m;  // quantity<si::metre, float>\n";
    std::cout << "       auto d3 = 8 * m;     // quantity<si::metre, int>\n\n";
    std::cout << "   The ns3 aliases fix the representation to double (e.g., meter_t\n";
    std::cout << "   is quantity<si::metre, double>), consistent with ns-3 conventions.\n\n";
    std::cout << "   Assigning an integer-representation quantity to a double-representation\n";
    std::cout << "   alias (e.g., MHz_t freq = 0 * MHz;) is a widening conversion and works\n";
    std::cout << "   correctly. However, in template contexts where the type is deduced from\n";
    std::cout << "   the initial value -- such as std::accumulate -- the accumulator will\n";
    std::cout << "   have int representation, and assigning a double-representation result\n";
    std::cout << "   back to it is a narrowing conversion that the compiler will reject.\n";
    std::cout << "   When in doubt, use a floating-point literal (0.0 * MHz) to ensure\n";
    std::cout << "   double representation.\n\n";
    std::cout << "2. numerical_value_in() performs a unit conversion and returns the\n";
    std::cout << "   bare representation type. The argument is the target unit, which\n";
    std::cout << "   need not match the stored unit -- dimensions must be compatible,\n";
    std::cout << "   enforced at compile time:\n\n";
    std::cout << "       auto d = 8.0 * m;\n";
    std::cout << "       d.numerical_value_in(m);   // 8.0   -- no conversion needed\n";
    std::cout << "       d.numerical_value_in(km);  // 0.008 -- conversion performed\n";
    std::cout
        << "       d.numerical_value_in(Hz);  // compile error -- incompatible dimensions\n\n";
    std::cout << "   The result is a raw double -- unit information is intentionally\n";
    std::cout << "   discarded. This is the recommended technique for passing a raw\n";
    std::cout << "   number to legacy code; the required unit argument makes the intent\n";
    std::cout << "   self-documenting.\n\n";

    auto distValueKm = distance.numerical_value_in(km);
    std::cout << "Two examples:\n\n";
    std::cout << "distance.numerical_value_in(m)  = " << distValue << "\n";
    std::cout << "distance.numerical_value_in(km) = " << distValueKm << "\n\n";

    std::cout << "This flexible representation type support is one reason why ns3::Time\n";
    std::cout << "could potentially be ported to mp-units. ns3::Time currently stores\n";
    std::cout << "time as int64_t with a runtime-selectable resolution (nanoseconds by\n";
    std::cout << "default). If runtime resolution were abandoned in favor of a fixed\n";
    std::cout << "compile-time unit, mp-units could express Time as, for example,\n";
    std::cout << "quantity<si::nano<si::second>, int64_t> -- preserving the integral\n";
    std::cout << "representation and gaining full dimensional safety and unit-conversion\n";
    std::cout << "arithmetic at no runtime cost.\n\n";

    // =========================================================================
    // Compile-time safety
    // =========================================================================

    std::cout << "### Compile-Time Safety\n\n";
    std::cout << "Expressions with incompatible types will not compile:\n\n";
    std::cout << "    // will fail: no match for 'operator+'\n";
    std::cout << "    auto sum = distance + txPower;  // meters + watts\n\n";
    std::cout << "    // will fail: no match for 'operator+'\n";
    std::cout << "    double rawDouble{4};\n";
    std::cout << "    auto bad = distance + rawDouble;    // quantity + raw double\n\n";

#ifdef WONT_COMPILE
    auto sum = distance + txPower;
    double rawDouble{4};
    auto bad = distance + rawDouble;
#endif

    std::cout << "mp-units uses C++ concepts to constrain its operator templates, so\n";
    std::cout << "the compiler error message names the actual quantity types rather than\n";
    std::cout << "a wall of template instantiation noise. For example, the first error\n";
    std::cout << "above produces:\n\n";
    std::cout << "    error: invalid operands to binary expression\n";
    std::cout << "           ('quantity<metre{}, double>' and\n";
    std::cout << "            'Watt_t' (aka 'quantity<mp_units::si::watt, double>'))\n";
    std::cout << "        auto sum = distance + txPower;\n";
    std::cout << "                   ~~~~~~~~ ^ ~~~~~~~\n\n";
    std::cout << "The types are stated in the error rather than embedded inside\n";
    std::cout << "template parameter packs several levels deep.\n\n";

    // =========================================================================
    // Compatible unit arithmetic
    // =========================================================================

    std::cout << "### Compatible Unit Arithmetic\n\n";
    std::cout << "Different units of the same dimension work correctly:\n\n";
    std::cout << "    auto d1 = 8.0 * m;\n";
    std::cout << "    auto d2 = 8.0 * km;\n\n";

    auto d1 = 8.0 * m;
    auto d2 = 8.0 * km;

    std::cout << "The result is:\n\n";
    std::cout << "d1 + d2 = " << d1 + d2 << "\n\n";

    std::cout << "The result unit is the common reference unit. For SI-prefixed length\n";
    std::cout << "that is metres, so 8 m + 8 km = 8008 m (not 8.008 km). This matters\n";
    std::cout << "when storing the result in an explicit type alias: meter_t receives\n";
    std::cout << "the result without conversion because the stored unit is already metres.\n\n";

    std::cout << "Multiplication and division track dimension changes at compile time:\n\n";
    std::cout << "    auto speed = (8.0 * m) / (2.0 * s);  // quantity<m/s, double>\n\n";

    auto speed = (8.0 * m) / (2.0 * s);

    std::cout << "The result is:\n\n";
    std::cout << "speed = " << speed << "\n\n";

    std::cout << "Dividing two quantities of the same dimension yields a dimensionless\n";
    std::cout << "quantity, not a raw double. Use numerical_value_in(mp_units::one)\n";
    std::cout << "to extract the ratio as a raw number:\n\n";
    std::cout << "    auto ratio = d1 / d2;                          // dimensionless quantity\n";
    std::cout << "    ratio.numerical_value_in(mp_units::one);       // raw double\n\n";

    auto ratio = d1 / d2;

    std::cout << "The result is:\n\n";
    std::cout << "d1 / d2 = " << ratio << "\n";
    std::cout << "d1 / d2 as double = " << ratio.numerical_value_in(mp_units::one) << "\n\n";

    std::cout << "A quantity variable can also be scaled by a raw number:\n\n";
    std::cout << "    auto d3 = d1 * 2.0;     // 16 m -- scales an existing quantity\n";
    std::cout << "    auto d4 = d2 / 1000.0;  // 8 m  -- divides an existing quantity\n\n";

    auto d3 = d1 * 2.0;
    auto d4 = d2 / 1000.0;

    std::cout << "The results are:\n\n";
    std::cout << "d1 * 2.0    = " << d3 << "\n";
    std::cout << "d2 / 1000.0 = " << d4 << "\n\n";
    std::cout << "This is distinct from the construction pattern \"8.0 * m\", where the\n";
    std::cout << "right operand is a unit object. Here the right operand is a raw double\n";
    std::cout << "and the left operand is an already-typed quantity. Both use operator*\n";
    std::cout << "but serve different roles: construction vs. scaling.\n\n";

    // =========================================================================
    // Logarithmic units (ns-3 wrapper types)
    // =========================================================================

    std::cout << "### Logarithmic Units (ns-3 wrapper types in namespace ns3)\n\n";
    std::cout << "mp-units does not natively support logarithmic units (dB, dBm, dBW).\n";
    std::cout << "ns-3 instead provides wrapper classes with type-safe arithmetic:\n\n";
    std::cout << "    dBm_t txPwrDbm = 20.0 * dBm;    // absolute power\n";
    std::cout << "    dB_t gain = 10.0 * dB;           // relative gain\n";
    std::cout << "    dBW_t txPwrDbW{txPwrDbm};        // explicit conversion\n\n";

    std::cout << "These print out as:\n\n";
    mWatt_t txPwr{100.0 * mW};
    std::cout << "txPwr = " << txPwr << std::endl;
    dBm_t txPwrDbm{txPwr};
    std::cout << "txPwrDbm = dBm_t{txPwr} = " << txPwrDbm << std::endl;
    dBW_t txPwrDbW{txPwrDbm};
    std::cout << "txPwrDbW = dBW_t{txPwrDbm} = " << txPwrDbW << std::endl;
    std::cout << std::endl;

    // =========================================================================
    // Logarithmic arithmetic
    // =========================================================================

    std::cout << "### Logarithmic Arithmetic Rules\n\n";

    std::cout << "Linear power addition:\n\n";
    std::cout << "txPwr + txPwr = " << txPwr + txPwr << "\n\n";

    std::cout << "Linear power scaling:\n\n";
    std::cout << "txPwr * 2 = " << 2 * txPwr << "\n\n";

    std::cout << "dBm + dB = dBm (adding gain to absolute power):\n\n";
    dB_t gain = 10.0 * dB;
    std::cout << "txPwrDbm (" << txPwrDbm << ") + gain (" << gain << ") = " << txPwrDbm + gain
              << std::endl;
    std::cout << std::endl;

    std::cout << "dBm - dB = dBm (subtracting path loss; common in path loss calculations):\n\n";
    dB_t pathLoss = 80.0 * dB;
    std::cout << "txPwrDbm (" << txPwrDbm << ") - pathLoss (" << pathLoss
              << ") = " << txPwrDbm - pathLoss << std::endl;
    std::cout << std::endl;

    std::cout << "dBm - dBm = dB (computing a ratio):\n\n";
    dBm_t rxPwr = -50.0 * dBm;
    std::cout << "txPwrDbm (" << txPwrDbm << ") - rxPwr (" << rxPwr << ") = " << txPwrDbm - rxPwr
              << std::endl;
    std::cout << std::endl;

    std::cout << "dBm + dBm is deleted (won't compile):\n\n";
    std::cout << "    #ifdef WONT_COMPILE\n";
    std::cout << "    auto bad = txPwrDbm + txPwrDbm;  // compile error\n";
    std::cout << "    #endif\n\n";

#ifdef WONT_COMPILE
    auto bad2 = txPwrDbm + txPwrDbm;
#endif

    std::cout << "Compound assignment operators += and -= are available for all\n";
    std::cout << "logarithmic types (dB_t, dBm_t, dBW_t), useful in simulation loops:\n\n";
    std::cout << "    dBm_t power = -60.0 * dBm;\n";
    std::cout << "    power += 10.0 * dB;  // -50 dBm\n";
    std::cout << "    power -= 10.0 * dB;  // -60 dBm\n\n";

    dBm_t accPwr = -60.0 * dBm;
    dB_t step = 10.0 * dB;
    accPwr += step;
    std::cout << "-60.0 * dBm after += 10.0 * dB: " << accPwr << "\n";
    accPwr -= step;
    std::cout << "then again after -= 10.0 * dB: " << accPwr << "\n\n";

    // =========================================================================
    // Linear <-> logarithmic conversion
    // =========================================================================

    std::cout << "### Linear <-> logarithmic conversion\n\n";
    std::cout << "Conversions are explicit:\n\n";
    std::cout << "    auto txPwr = 100.0 * mW;\n";
    std::cout << "    dBm_t txPwrDbm{txPwr};                 // 100 mW -> 20 dBm\n";
    std::cout << "    auto back = mWatt_t(txPwrDbm); // 20 dBm -> 100 mW\n\n";

    auto back = mWatt_t(txPwrDbm);
    std::cout << "back = mWatt_t(txPwrDbm) = " << back << std::endl;

    auto wattBack = Watt_t(txPwrDbm);
    std::cout << "wattBack = Watt_t(txPwrDbm) = " << wattBack << std::endl;
    std::cout << std::endl;

    std::cout << "### Extracting raw dB values\n\n";
    std::cout << "The numerical_value_in(unit) method extracts the raw double from both\n";
    std::cout << "linear and logarithmic types. For logarithmic types, the unit tag\n";
    std::cout << "argument (dB, dBm, dBW, dBm_per_Hz, dBm_per_MHz) must match the type:\n\n";
    std::cout << "    auto p = 20.0 * dBm;   p.numerical_value_in(dBm)  // 20.0\n";
    std::cout << "    auto w = -10.0 * dBW;  w.numerical_value_in(dBW)  // -10.0\n";
    std::cout << "    auto g = 3.0 * dB;     g.numerical_value_in(dB)   //  3.0\n\n";
    std::cout << "These will print as:\n\n";
    std::cout << "txPwrDbm.numerical_value_in(dBm) = " << txPwrDbm.numerical_value_in(dBm) << "\n";
    std::cout << "txPwrDbW.numerical_value_in(dBW) = " << txPwrDbW.numerical_value_in(dBW) << "\n";
    std::cout << "gain.numerical_value_in(dB)      = " << gain.numerical_value_in(dB) << "\n\n";

    std::cout << "This is the same API pattern as mp-units linear quantities:\n";
    std::cout << "    MHz_t freq = 2400.0 * MHz;\n";
    std::cout << "    freq.numerical_value_in(MHz)  // 2400.0\n";
    std::cout << "    freq.numerical_value_in(Hz)   // 2.4e9\n\n";

    // =========================================================================
    // dBW arithmetic
    // =========================================================================

    std::cout << "### dBW arithmetic\n\n";
    dBW_t loss = -20.0 * dBW;
    std::cout << "This variable: -20.0 * dBW, will print as:\n\n";
    std::cout << "loss = " << loss << "\n\n";
    std::cout << "Using 10 dB gain from above:\n\n";
    std::cout << "loss + gain = " << loss + gain << std::endl;

    std::cout << "Mixing linear watts and logarithmic dBW will not compile\n\n";
#ifdef WONT_COMPILE
    // Mixing linear and logarithmic won't compile (different types entirely)
    std::cout << txPwr - loss << std::endl;
#endif

    std::cout << "Next, we illustrate converting loss to a linear mWatt_t type:\n\n";
    std::cout << "mWatt_t(loss) = " << mWatt_t(loss) << std::endl;
    std::cout << "Reusing txPwr from above (recall that it was 100 mW):\n\n";
    std::cout << "txPwr - mWatt_t(loss) = " << txPwr - mWatt_t(loss) << std::endl;
    std::cout << std::endl;

    // =========================================================================
    // std::min works
    // =========================================================================

    std::cout << "### std::min/max\n\n";
    auto x = 3.0 * dBm;
    auto y = 4.0 * dBm;
    std::cout << "std::min of " << x << " and " << y << " is: " << std::min(x, y) << std::endl;
    std::cout << std::endl;

    // =========================================================================
    // Power spectral density
    // =========================================================================

    std::cout << "### Power Spectral Density\n\n";
    std::cout << "PSD types support multiplication by frequency to yield total power:\n\n";
    std::cout << "    auto psd1 = -50.0 * dBm_per_Hz;\n";
    std::cout << "    auto psd2 = -20.0 * dBm_per_MHz;\n";
    std::cout << "    Hz_t bw1{1000.0 * Hz};\n";
    std::cout << "    MHz_t bw2{10.0 * MHz};\n";
    std::cout << "    auto total1 = psd1 * bw1;  // dBm_t\n";
    std::cout << "    auto total2 = psd2 * bw2;  // dBm_t\n\n";

    auto psd1 = -50.0 * dBm_per_Hz;
    auto psd2 = -20.0 * dBm_per_MHz;
    Hz_t bw1{1000.0 * Hz};
    MHz_t bw2{10.0 * MHz};
    auto total1 = psd1 * bw1;
    auto total2 = psd2 * bw2;
    auto total3 = bw1 * psd1; // commutative

    std::cout << "Printing out these variables yields:\n\n";
    std::cout << "psd1 = " << psd1 << std::endl;
    std::cout << "psd2 = " << psd2 << std::endl;
    std::cout << "bw1 = " << bw1 << std::endl;
    std::cout << "bw2 = " << bw2 << std::endl;
    std::cout << "total1 (psd1 * bw1) = " << total1 << std::endl;
    std::cout << "total2 (psd2 * bw2) = " << total2 << std::endl;
    std::cout << "total3 (bw1 * psd1) = " << total3 << std::endl;
    std::cout << std::endl;

    // =========================================================================
    // Linear PSD
    // =========================================================================

    std::cout << "### Linear Power Spectral Density\n\n";
    std::cout << "mW_per_Hz_t and mW_per_MHz_t are the linear counterparts of the\n";
    std::cout << "logarithmic PSD types. mp-units performs the dimensional reduction\n";
    std::cout << "automatically on multiplication:\n\n";
    std::cout << "    mW_per_Hz_t  noisePsd{1.0e-9 * mW / Hz};  // 1 nW/Hz\n";
    std::cout << "    Hz_t         bwHz{1.0e9 * Hz};             // 1 GHz\n";
    std::cout << "    auto         noiseFloor = noisePsd * bwHz; // mWatt_t result: 1 mW\n\n";
    std::cout << "    mW_per_MHz_t channelPsd{0.1 * mW / MHz};        // 0.1 mW/MHz\n";
    std::cout << "    MHz_t        bwMHz{20.0 * MHz};                 // 20 MHz\n";
    std::cout << "    auto         channelPower = channelPsd * bwMHz; // mWatt_t result: 2 mW\n\n";

    mW_per_Hz_t noisePsd{1.0e-9 * mW / Hz};
    Hz_t bwHz{1.0e9 * Hz};
    auto noiseFloor = noisePsd * bwHz;

    mW_per_MHz_t channelPsd{0.1 * mW / MHz};
    MHz_t bwMHz{20.0 * MHz};
    auto channelPower = channelPsd * bwMHz;

    std::cout << "Running the above code produces:\n\n";
    std::cout << "noiseFloor   (1 nW/Hz * 1 GHz)      = " << noiseFloor << "\n";
    std::cout << "channelPower (0.1 mW/MHz * 20 MHz)  = " << channelPower << "\n\n";

    // =========================================================================
    // CommandLine integration
    // =========================================================================

    std::cout << "### CommandLine Integration\n\n";
    std::cout << "dB_t and dBm_t work with ns-3 CommandLine. Three input formats\n";
    std::cout << "are accepted:\n\n";
    std::cout << "    --cmdLineDecibel=5_dB   token format (recommended)\n";
    std::cout << "    --cmdLineDecibel=5      raw number, native unit assumed\n";
    std::cout << "    --cmdLineDecibel=\"5 dB\" standard notation, must be shell-quoted\n\n";
    std::cout << "Note: \"5_dB\" is not a C++ user-defined literal. It is a string\n";
    std::cout << "token parsed by DeserializeFromString. The underscore is a separator\n";
    std::cout << "between the number and the unit label in the serialized form.\n\n";
    std::cout << "operator<< displays values in standard notation (\"5 dB\"), while\n";
    std::cout << "SerializeToString (used by --PrintAttributes and config files) emits\n";
    std::cout << "the token form (\"5_dB\"). These intentionally differ: the token form\n";
    std::cout << "is a single whitespace-free string that can be passed directly on the\n";
    std::cout << "command line or written to a config file without quoting.\n\n";
    std::cout << "Pasting a displayed value (e.g. from NS_LOG output) back to the\n";
    std::cout << "command line requires either quoting (--cmdLineDecibel=\"5 dB\") or\n";
    std::cout << "converting the space to an underscore (--cmdLineDecibel=5_dB).\n\n";

    std::cout << "The variable cmdLineDecibel from above prints as:\n\n";
    std::cout << "cmdLineDecibel = " << cmdLineDecibel << "\n\n";

    // =========================================================================
    // Attribute integration
    // =========================================================================

    std::cout << "### Attribute integration\n\n";
    std::cout << "Attribute wrappers are available for DbValue, DbmValue, MhzValue, and "
                 "DbmPerMhzValue:\n\n";
    std::cout << "    .AddAttribute(\"TxGain\",\n";
    std::cout << "                  \"Transmission gain.\",\n";
    std::cout << "                  DbValue{0.0 * dB},\n";
    std::cout << "                  MakeDbAccessor(&MyClass::SetGain, &MyClass::GetGain),\n";
    std::cout << "                  MakeDbChecker())\n\n";
    std::cout << "Client code -- all three forms set the same value:\n\n";
    std::cout << "    obj->SetAttribute(\"TxGain\", DbValue{2.0 * dB});\n";
    std::cout << "    obj->SetAttribute(\"TxGain\", StringValue{\"2_dB\"});  // token format\n";
    std::cout
        << "    obj->SetAttribute(\"TxGain\", StringValue{\"2 dB\"});  // standard notation\n";
    std::cout << "    obj->SetAttribute(\"TxGain\", StringValue{\"2\"});     // raw number\n";
    std::cout << "    obj->SetAttribute(\"TxGain\", DoubleValue{2});        // numeric converter\n";
    std::cout << std::endl;

    std::cout << "MhzValue -- for channel frequency attributes:\n\n";
    std::cout << "    .AddAttribute(\"Frequency\",\n";
    std::cout << "                  \"Channel center frequency.\",\n";
    std::cout << "                  MhzValue{MHz_t{2400.0 * MHz}},\n";
    std::cout << "                  MakeMhzAccessor(&MyClass::m_frequency),\n";
    std::cout << "                  MakeMhzChecker())\n\n";
    std::cout << "All four input forms work:\n\n";
    std::cout << "    obj->SetAttribute(\"Frequency\", MhzValue{MHz_t{5180.0 * MHz}});\n";
    std::cout
        << "    obj->SetAttribute(\"Frequency\", DoubleValue{5180});      // raw double -> MHz\n";
    std::cout << "    obj->SetAttribute(\"Frequency\", StringValue{\"5180_MHz\"});\n";
    std::cout << "    obj->SetAttribute(\"Frequency\", StringValue{\"5180\"});   // raw number -> "
                 "MHz\n\n";
    std::cout << "MhzValue accepts DoubleValue (treating the raw double as MHz) for\n";
    std::cout << "backward compatibility with legacy attribute settings. This is\n";
    std::cout << "implemented via a custom MhzChecker::CreateValidValue override rather\n";
    std::cout << "than ATTRIBUTE_CHECKER_DEFINE_WITH_CONVERTER (which cannot construct\n";
    std::cout << "an mp-units quantity from a raw double).\n\n";

    std::cout << "DbmPerMhzValue -- for noise or interference mask attributes:\n\n";
    std::cout << "    .AddAttribute(\"NoisePsd\",\n";
    std::cout << "                  \"Noise power spectral density.\",\n";
    std::cout << "                  DbmPerMhzValue{-174.0 * dBm_per_MHz},\n";
    std::cout << "                  MakeDbmPerMhzAccessor(&MyClass::m_noisePsd),\n";
    std::cout << "                  MakeDbmPerMhzChecker())\n\n";
    std::cout << "Input forms:\n\n";
    std::cout << "    obj->SetAttribute(\"NoisePsd\", DbmPerMhzValue{-174.0 * dBm_per_MHz});\n";
    std::cout
        << "    obj->SetAttribute(\"NoisePsd\", DoubleValue{-174});             // raw double\n";
    std::cout << "    obj->SetAttribute(\"NoisePsd\", StringValue{\"-174_dBm_per_MHz\"});\n";
    std::cout << "    obj->SetAttribute(\"NoisePsd\", StringValue{\"-174\"});          // raw "
                 "number\n\n";

    // =========================================================================
    // TupleValue integration
    // =========================================================================

    std::cout << "### TupleValue Integration\n\n";
    std::cout << "DbValue and DbmValue can be combined in ns-3 TupleValue to model\n";
    std::cout << "composite attributes, for example a (tx_power, antenna_gain) pair:\n\n";
    std::cout << "    .AddAttribute(\"TxConfig\",\n";
    std::cout << "                  \"Transmit power (dBm) and antenna gain (dB).\",\n";
    std::cout << "                  TupleValue<DbmValue, DbValue>(\n";
    std::cout << "                      {-20.0 * dBm, 0.0 * dB}),\n";
    std::cout << "                  MakeTupleAccessor<DbmValue, DbValue>(\n";
    std::cout << "                      &MyClass::m_txConfig),\n";
    std::cout << "                  MakeTupleChecker<DbmValue, DbValue>(\n";
    std::cout << "                      MakeDbmChecker(), MakeDbChecker()))\n\n";
    std::cout << "All input formats carry through TupleValue string deserialization:\n\n";
    std::cout << "    obj->SetAttribute(\"TxConfig\", StringValue(\"{-15_dBm, 3_dB}\"));\n";
    std::cout << "    obj->SetAttribute(\"TxConfig\", StringValue(\"{-15, 3}\"));\n\n";
    std::cout << "This works because TupleValue is generic over any AttributeValue\n";
    std::cout << "subclass. DbmValue and DbValue satisfy that interface directly --\n";
    std::cout << "no special TupleValue support was needed beyond the standard\n";
    std::cout << "AttributeValue machinery.\n\n";

    // =========================================================================
    // Time: mp-units quantities vs ns3::Time
    // =========================================================================

    std::cout << "### Time: mp-units quantities vs ns3::Time\n\n";
    std::cout << "mp-units time quantities are completely distinct from ns3::Time.\n";
    std::cout << "There is no implicit conversion between them; the bridge is explicit\n";
    std::cout << "in both directions.\n\n";
    std::cout << "mp-units time quantities carry their unit at compile time and deduce\n";
    std::cout << "their representation type from the numeric literal:\n\n";
    std::cout << "    auto t1 = 1.5 * s;    // quantity<si::second, double>\n";
    std::cout << "    auto t2 = 100.0 * ms; // quantity<si::milli<si::second>, double>\n";
    std::cout << "    auto t3 = 10.0 * ns;  // quantity<si::nano<si::second>, double>\n\n";

    auto t1 = 1.5 * s;
    auto t2 = 100.0 * ms;
    auto t3 = 10.0 * ns;

    std::cout << "These print as:\n\n";
    std::cout << "t1 = " << t1 << "\n";
    std::cout << "t2 = " << t2 << "\n";
    std::cout << "t3 = " << t3 << "\n\n";

    std::cout << "Converting mp-units time to ns3::Time -- extract the numerical value\n";
    std::cout << "in a known unit and pass to the corresponding ns3::Time factory:\n\n";
    std::cout << "    Time nsTime1 = Seconds(t1.numerical_value_in(s));\n";
    std::cout << "    Time nsTime2 = Seconds(t2.numerical_value_in(s));\n\n";

    Time nsTime1 = Seconds(t1.numerical_value_in(s));
    Time nsTime2 = Seconds(t2.numerical_value_in(s));

    std::cout << "These print as:\n\n";
    std::cout << "nsTime1 = " << nsTime1.As(Time::S) << "\n";
    std::cout << "nsTime2 = " << nsTime2.As(Time::MS) << "\n\n";

    std::cout << "Converting ns3::Time to mp-units -- extract via a Get*() accessor\n";
    std::cout << "and multiply by the matching unit symbol:\n\n";
    std::cout << "    Time simTime = MilliSeconds(250);\n";
    std::cout << "    auto mpTime = simTime.GetSeconds() * s;    // double representation\n\n";
    std::cout << "GetSeconds() returns double, so mpTime has representation type double.\n";
    std::cout << "GetNanoSeconds() returns int64_t; multiplying directly by ns gives a\n";
    std::cout << "quantity<si::nanosecond, int64_t>, which may be unexpected. Cast to\n";
    std::cout << "double first if a floating-point representation is needed:\n\n";
    std::cout << "    auto mpTimeNs = static_cast<double>(simTime.GetNanoSeconds()) * ns;\n\n";

    Time simTime = MilliSeconds(250);
    auto mpTime = simTime.GetSeconds() * s;
    auto mpTimeNs = static_cast<double>(simTime.GetNanoSeconds()) * ns;
    std::cout << "simTime  = " << simTime.As(Time::MS) << "\n";
    std::cout << "mpTime   = " << mpTime << "\n";
    std::cout << "mpTimeNs = " << mpTimeNs << "\n\n";

    std::cout << "ns3::Time stores time as int64_t at a runtime-selectable resolution\n";
    std::cout << "(nanoseconds by default). mp-units time quantities carry their unit\n";
    std::cout << "at compile time and use whatever representation type was used in\n";
    std::cout << "construction. The two types model time differently and coexist;\n";
    std::cout << "ns-3 simulation infrastructure continues to use ns3::Time throughout.\n\n";
    std::cout << "The mp-units design leaves open the possibility of integrating\n";
    std::cout << "ns3::Time more deeply in the future. The primary prerequisite would\n";
    std::cout << "be replacing ns3::Time's runtime-selectable resolution with a\n";
    std::cout << "compile-time fixed unit. Further discussion of that migration path\n";
    std::cout << "is out of scope for this example.\n\n";

    return 0;
}
