/*
 * Copyright (c) 2026 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/test.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

/**
 * @file
 * @ingroup core-tests
 * Log time window and context filter test suite.
 */

// The tests check logging output, which is compiled out of optimized builds.
#ifdef NS3_LOG_ENABLE

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("LogFilterTestSuite");

namespace tests
{

/**
 * @ingroup core-tests
 * @defgroup log-filter-tests Log filter test suite
 */

/**
 * @ingroup log-filter-tests
 * Log a marker message, to be scheduled at a specific time and context.
 *
 * @param index The marker index.
 */
static void
EmitLogMarker(uint32_t index)
{
    NS_LOG_INFO("[marker " << index << "]");
}

/**
 * @ingroup log-filter-tests
 * Base class for log filter test cases: enables the test log component,
 * captures std::clog output, and cleans up the filter state.
 */
class LogFilterTestCase : public TestCase
{
  public:
    /**
     * Constructor.
     *
     * @param name The test case name.
     */
    LogFilterTestCase(const std::string& name)
        : TestCase(name)
    {
    }

  protected:
    void DoSetup() override
    {
        LogComponentEnable("LogFilterTestSuite", LOG_LEVEL_INFO);
        m_clogBuf = std::clog.rdbuf(m_output.rdbuf());
    }

    void DoTeardown() override
    {
        std::clog.rdbuf(m_clogBuf);
        LogSetTimeWindow("");
        LogSetContextFilter("");
        LogComponentDisable("LogFilterTestSuite", LOG_LEVEL_ALL);
        Simulator::Destroy();
    }

    /**
     * Check whether the marker with the given index was logged.
     *
     * @param index The marker index.
     * @return \c true if the marker is present in the captured output.
     */
    bool Logged(uint32_t index) const
    {
        std::ostringstream marker;
        marker << "[marker " << index << "]";
        return m_output.str().find(marker.str()) != std::string::npos;
    }

    std::ostringstream m_output;        //!< Captured log output.
    std::streambuf* m_clogBuf{nullptr}; //!< Original std::clog buffer.
};

/**
 * @ingroup log-filter-tests
 * Check that a time window set with LogSetTimeWindow(Time, Time)
 * suppresses log statements outside the window, with inclusive bounds.
 */
class LogTimeWindowTestCase : public LogFilterTestCase
{
  public:
    LogTimeWindowTestCase()
        : LogFilterTestCase("log-filter-time-window")
    {
    }

  private:
    void DoRun() override
    {
        LogSetTimeWindow(Seconds(1.2), Seconds(1.5));
        const std::vector<double> times{1.0, 1.2, 1.35, 1.5, 1.6};
        for (uint32_t i = 0; i < times.size(); i++)
        {
            Simulator::Schedule(Seconds(times[i]), &EmitLogMarker, i);
        }
        Simulator::Run();
        NS_TEST_EXPECT_MSG_EQ(Logged(0), false, "log before window should be suppressed");
        NS_TEST_EXPECT_MSG_EQ(Logged(1), true, "window lower bound should be inclusive");
        NS_TEST_EXPECT_MSG_EQ(Logged(2), true, "log inside window should be printed");
        NS_TEST_EXPECT_MSG_EQ(Logged(3), true, "window upper bound should be inclusive");
        NS_TEST_EXPECT_MSG_EQ(Logged(4), false, "log after window should be suppressed");
    }
};

/**
 * @ingroup log-filter-tests
 * Check open-ended time windows set with the string form of
 * LogSetTimeWindow().
 */
class LogTimeWindowStringTestCase : public LogFilterTestCase
{
  public:
    LogTimeWindowStringTestCase()
        : LogFilterTestCase("log-filter-time-window-string")
    {
    }

  private:
    void DoRun() override
    {
        LogSetTimeWindow("1.2s/");
        Simulator::Schedule(Seconds(1.0), &EmitLogMarker, 0);
        Simulator::Schedule(Seconds(1.4), &EmitLogMarker, 1);
        Simulator::Run();
        NS_TEST_EXPECT_MSG_EQ(Logged(0), false, "log before open-ended window start");
        NS_TEST_EXPECT_MSG_EQ(Logged(1), true, "log after open-ended window start");

        Simulator::Destroy();
        m_output.str("");
        LogSetTimeWindow("/1.5s");
        Simulator::Schedule(Seconds(1.4), &EmitLogMarker, 2);
        Simulator::Schedule(Seconds(1.6), &EmitLogMarker, 3);
        Simulator::Run();
        NS_TEST_EXPECT_MSG_EQ(Logged(2), true, "log before open-ended window end");
        NS_TEST_EXPECT_MSG_EQ(Logged(3), false, "log after open-ended window end");
    }
};

/**
 * @ingroup log-filter-tests
 * Check that a context filter suppresses log statements executing in
 * contexts outside the configured set, including the `-1` (no context)
 * value.
 */
class LogContextFilterTestCase : public LogFilterTestCase
{
  public:
    LogContextFilterTestCase()
        : LogFilterTestCase("log-filter-context")
    {
    }

  private:
    void DoRun() override
    {
        LogSetContextFilter("0,[2-4],6");
        for (uint32_t context = 0; context < 8; context++)
        {
            Simulator::ScheduleWithContext(context, Seconds(1), &EmitLogMarker, context);
        }
        // Events scheduled outside of event execution run with no context
        Simulator::Schedule(Seconds(1), &EmitLogMarker, 8);
        Simulator::Run();
        NS_TEST_EXPECT_MSG_EQ(Logged(0), true, "context 0 should be printed");
        NS_TEST_EXPECT_MSG_EQ(Logged(1), false, "context 1 should be suppressed");
        NS_TEST_EXPECT_MSG_EQ(Logged(2), true, "context 2 should be printed (range)");
        NS_TEST_EXPECT_MSG_EQ(Logged(3), true, "context 3 should be printed (range)");
        NS_TEST_EXPECT_MSG_EQ(Logged(4), true, "context 4 should be printed (range)");
        NS_TEST_EXPECT_MSG_EQ(Logged(5), false, "context 5 should be suppressed");
        NS_TEST_EXPECT_MSG_EQ(Logged(6), true, "context 6 should be printed");
        NS_TEST_EXPECT_MSG_EQ(Logged(7), false, "context 7 should be suppressed");
        NS_TEST_EXPECT_MSG_EQ(Logged(8), false, "no-context log should be suppressed");

        Simulator::Destroy();
        m_output.str("");
        LogSetContextFilter("-1");
        Simulator::ScheduleWithContext(5, Seconds(1), &EmitLogMarker, 9);
        Simulator::Schedule(Seconds(1), &EmitLogMarker, 10);
        Simulator::Run();
        NS_TEST_EXPECT_MSG_EQ(Logged(9), false, "context 5 should be suppressed by -1 filter");
        NS_TEST_EXPECT_MSG_EQ(Logged(10), true, "no-context log should be printed by -1 filter");
    }
};

/**
 * @ingroup log-filter-tests
 * Check that time window and context filters combine (both must match).
 */
class LogFilterCombinedTestCase : public LogFilterTestCase
{
  public:
    LogFilterCombinedTestCase()
        : LogFilterTestCase("log-filter-combined")
    {
    }

  private:
    void DoRun() override
    {
        LogSetTimeWindow("1.2s/1.5s");
        LogSetContextFilter("3");
        Simulator::ScheduleWithContext(3, Seconds(1.3), &EmitLogMarker, 0);
        Simulator::ScheduleWithContext(3, Seconds(1.0), &EmitLogMarker, 1);
        Simulator::ScheduleWithContext(4, Seconds(1.3), &EmitLogMarker, 2);
        Simulator::Run();
        NS_TEST_EXPECT_MSG_EQ(Logged(0), true, "matching time and context should be printed");
        NS_TEST_EXPECT_MSG_EQ(Logged(1), false, "log outside time window should be suppressed");
        NS_TEST_EXPECT_MSG_EQ(Logged(2), false, "log outside context set should be suppressed");
    }
};

/**
 * @ingroup log-filter-tests
 * Check that filters can be removed by passing empty strings.
 */
class LogFilterClearTestCase : public LogFilterTestCase
{
  public:
    LogFilterClearTestCase()
        : LogFilterTestCase("log-filter-clear")
    {
    }

  private:
    void DoRun() override
    {
        LogSetTimeWindow("1.2s/1.5s");
        LogSetContextFilter("3");
        LogSetTimeWindow("");
        LogSetContextFilter("");
        Simulator::ScheduleWithContext(7, Seconds(2), &EmitLogMarker, 0);
        Simulator::Run();
        NS_TEST_EXPECT_MSG_EQ(Logged(0), true, "cleared filters should not suppress logs");
    }
};

/**
 * @ingroup log-filter-tests
 * Log filter test suite.
 */
class LogFilterTestSuite : public TestSuite
{
  public:
    LogFilterTestSuite()
        : TestSuite("log-filter")
    {
        AddTestCase(new LogTimeWindowTestCase);
        AddTestCase(new LogTimeWindowStringTestCase);
        AddTestCase(new LogContextFilterTestCase);
        AddTestCase(new LogFilterCombinedTestCase);
        AddTestCase(new LogFilterClearTestCase);
    }
};

/**
 * @ingroup log-filter-tests
 * Static variable for test initialization.
 */
static LogFilterTestSuite g_logFilterTestSuite;

} // namespace tests

} // namespace ns3

#endif // NS3_LOG_ENABLE
