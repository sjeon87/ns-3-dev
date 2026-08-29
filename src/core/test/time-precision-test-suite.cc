/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/nstime.h"
#include "ns3/test.h"

#include <cmath>
#include <iostream>

/**
 * @file
 * @ingroup core-tests
 * Tests for Time precision characteristics when constructing
 * from double-precision floating-point values via FromDouble().
 *
 * These tests codify the expected precision behavior documented
 * in issue #1237 and prevent future regressions.
 */

using namespace ns3;

/**
 * @ingroup core-tests
 * @defgroup time-precision-tests Time Precision Tests
 */

/**
 * @ingroup time-precision-tests
 * @brief Verify that Seconds(double) matches the equivalent integer-unit
 *        constructor for exact powers-of-10 values.
 */
class FromDoubleIntegerEquivalenceTestCase : public TestCase
{
  public:
    FromDoubleIntegerEquivalenceTestCase()
        : TestCase("FromDouble integer equivalence for powers-of-10")
    {
    }

  private:
    void DoRun() override
    {
        NS_TEST_ASSERT_MSG_EQ(Seconds(1e-3),
                              MilliSeconds(1),
                              "Seconds(1e-3) should equal MilliSeconds(1)");

        NS_TEST_ASSERT_MSG_EQ(Seconds(1e-6),
                              MicroSeconds(1),
                              "Seconds(1e-6) should equal MicroSeconds(1)");

        NS_TEST_ASSERT_MSG_EQ(Seconds(1e-9),
                              NanoSeconds(1),
                              "Seconds(1e-9) should equal NanoSeconds(1)");

        NS_TEST_ASSERT_MSG_EQ(MilliSeconds(1),
                              MicroSeconds(1000),
                              "MilliSeconds(1) should equal MicroSeconds(1000)");

        NS_TEST_ASSERT_MSG_EQ(MicroSeconds(1),
                              NanoSeconds(1000),
                              "MicroSeconds(1) should equal NanoSeconds(1000)");

        NS_TEST_ASSERT_MSG_EQ(Seconds(1.0),
                              NanoSeconds(1000000000),
                              "Seconds(1.0) should equal NanoSeconds(1e9)");

        NS_TEST_ASSERT_MSG_EQ(Minutes(1.0),
                              Seconds(60.0),
                              "Minutes(1.0) should equal Seconds(60.0)");

        NS_TEST_ASSERT_MSG_EQ(Hours(1.0), Minutes(60.0), "Hours(1.0) should equal Minutes(60.0)");
    }
};

/**
 * @ingroup time-precision-tests
 * @brief Verify round-trip: Seconds(x).GetSeconds() preserves the input
 *        to within expected double precision.
 */
class FromDoubleRoundTripTestCase : public TestCase
{
  public:
    FromDoubleRoundTripTestCase()
        : TestCase("FromDouble round-trip through GetSeconds()")
    {
    }

  private:
    void DoRun() override
    {
        // Exactly representable values
        double values[] = {1.0, 0.5, 0.25, 0.125, 2.0, 100.0, 1e-3, 1e-6};

        for (double v : values)
        {
            Time t = Seconds(v);
            double recovered = t.GetSeconds();
            // Tolerance: 1 ns expressed in seconds
            double tolerance = 1e-9;
            NS_TEST_ASSERT_MSG_EQ_TOL(recovered,
                                      v,
                                      tolerance,
                                      "Round-trip failed for Seconds(" << v << ")");
        }

        // Non-trivial value with many significant digits
        double pi_us = 3.14159265358979e-6; // ~pi microseconds
        Time tPi = Seconds(pi_us);
        double recoveredPi = tPi.GetSeconds();
        // The tolerance here is 1 ns in seconds
        NS_TEST_ASSERT_MSG_EQ_TOL(recoveredPi,
                                  pi_us,
                                  1e-9,
                                  "Round-trip failed for pi microseconds");
    }
};

/**
 * @ingroup time-precision-tests
 * @brief Verify that worst-case conversions (small double value in a large unit)
 *        are within +/- 1 resolution unit of the mathematically correct answer.
 */
class FromDoublePrecisionBoundTestCase : public TestCase
{
  public:
    FromDoublePrecisionBoundTestCase()
        : TestCase("FromDouble precision bound for small-value large-unit conversions")
    {
    }

  private:
    void DoRun() override
    {
        // 1 microsecond expressed as seconds
        // Mathematically: 1e-6 s = 1000 ns
        Time t1 = Seconds(1e-6);
        NS_TEST_ASSERT_MSG_EQ(t1.GetNanoSeconds(), 1000, "Seconds(1e-6) should be exactly 1000 ns");

        // 1 microsecond expressed as minutes
        // Mathematically: 1e-6 min = 60e-6 s = 60000 ns
        Time t2 = Minutes(1e-6);
        int64_t expected2 = 60000;
        int64_t actual2 = t2.GetNanoSeconds();
        NS_TEST_ASSERT_MSG_EQ_TOL(actual2,
                                  expected2,
                                  1,
                                  "Minutes(1e-6) should be ~60000 ns (+/- 1 ns)");

        // 1 microsecond expressed as hours
        // Mathematically: 1e-6 h = 3.6e-3 s = 3600000 ns
        Time t3 = Hours(1e-6);
        int64_t expected3 = 3600000;
        int64_t actual3 = t3.GetNanoSeconds();
        NS_TEST_ASSERT_MSG_EQ_TOL(actual3,
                                  expected3,
                                  1,
                                  "Hours(1e-6) should be ~3600000 ns (+/- 1 ns)");

        // 1 nanosecond expressed as days
        // Mathematically: 1e-9 d = 86.4e-6 s = 86400 ns
        Time t4 = Days(1e-9);
        int64_t expected4 = 86400;
        int64_t actual4 = t4.GetNanoSeconds();
        NS_TEST_ASSERT_MSG_EQ_TOL(actual4,
                                  expected4,
                                  1,
                                  "Days(1e-9) should be ~86400 ns (+/- 1 ns)");
    }
};

/**
 * @ingroup time-precision-tests
 * @brief Verify that sub-resolution fractional values round correctly
 *        (to the nearest integer resolution unit).
 */
class FromDoubleSubResolutionTestCase : public TestCase
{
  public:
    FromDoubleSubResolutionTestCase()
        : TestCase("FromDouble sub-resolution rounding")
    {
    }

  private:
    void DoRun() override
    {
        // 1.5 ns should round to 2 ns (round half away from zero)
        Time t1 = Seconds(1.5e-9);
        NS_TEST_ASSERT_MSG_EQ(t1.GetNanoSeconds(), 2, "Seconds(1.5e-9) should round to 2 ns");

        // 1.4 ns should round to 1 ns
        Time t2 = Seconds(1.4e-9);
        NS_TEST_ASSERT_MSG_EQ(t2.GetNanoSeconds(), 1, "Seconds(1.4e-9) should round to 1 ns");

        // 0.6 ns should round to 1 ns (above the round-to-zero threshold)
        Time t3 = Seconds(0.6e-9);
        NS_TEST_ASSERT_MSG_EQ(t3.GetNanoSeconds(), 1, "Seconds(0.6e-9) should round to 1 ns");

        // 2.999 ns should round to 3 ns
        Time t4 = Seconds(2.999e-9);
        NS_TEST_ASSERT_MSG_EQ(t4.GetNanoSeconds(), 3, "Seconds(2.999e-9) should round to 3 ns");
    }
};

/**
 * @ingroup time-precision-tests
 * @brief Verify FromDouble produces correct results for each large unit
 *        (Y, D, H, MIN, S) with representative magnitudes.
 */
class FromDoubleAllUnitsTestCase : public TestCase
{
  public:
    FromDoubleAllUnitsTestCase()
        : TestCase("FromDouble correctness across all large units")
    {
    }

  private:
    void DoRun() override
    {
        // Seconds
        NS_TEST_ASSERT_MSG_EQ(Seconds(1.0).GetNanoSeconds(), 1000000000LL, "Seconds(1.0) in ns");

        // Minutes: 1 min = 60 s = 60e9 ns
        NS_TEST_ASSERT_MSG_EQ(Minutes(1.0).GetNanoSeconds(), 60000000000LL, "Minutes(1.0) in ns");

        // Hours: 1 h = 3600 s = 3.6e12 ns
        NS_TEST_ASSERT_MSG_EQ(Hours(1.0).GetNanoSeconds(), 3600000000000LL, "Hours(1.0) in ns");

        // Days: 1 d = 86400 s = 8.64e13 ns
        NS_TEST_ASSERT_MSG_EQ(Days(1.0).GetNanoSeconds(), 86400000000000LL, "Days(1.0) in ns");

        // Years: 1 y = 365 d = 31536000 s = 3.1536e16 ns
        NS_TEST_ASSERT_MSG_EQ(Years(1.0).GetNanoSeconds(), 31536000000000000LL, "Years(1.0) in ns");

        // Fractional: 0.5 hours = 1800 s = 1.8e12 ns
        NS_TEST_ASSERT_MSG_EQ(Hours(0.5).GetNanoSeconds(), 1800000000000LL, "Hours(0.5) in ns");
    }
};

/**
 * @ingroup time-precision-tests
 * @brief Time precision test suite.
 *
 * Tests the precision characteristics of Time construction
 * from double-precision floating-point values.
 */
static class TimePrecisionTestSuite : public TestSuite
{
  public:
    TimePrecisionTestSuite()
        : TestSuite("time-precision", Type::UNIT)
    {
        AddTestCase(new FromDoubleIntegerEquivalenceTestCase(), TestCase::Duration::QUICK);
        AddTestCase(new FromDoubleRoundTripTestCase(), TestCase::Duration::QUICK);
        AddTestCase(new FromDoublePrecisionBoundTestCase(), TestCase::Duration::QUICK);
        AddTestCase(new FromDoubleSubResolutionTestCase(), TestCase::Duration::QUICK);
        AddTestCase(new FromDoubleAllUnitsTestCase(), TestCase::Duration::QUICK);
    }
}
/** @brief Member variable for time precision test suite */
g_timePrecisionTestSuite;
