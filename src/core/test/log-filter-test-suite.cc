/*
 * Copyright (c) 2026 University contributors
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/log-filter.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/test.h"

#include <sstream>
#include <string>
#include <vector>

/**
 * @file
 * @ingroup tests
 * Log filter test suite.
 */

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LogFilterTestSuite");

/**
 * @ingroup tests
 * @defgroup log-filter-tests Log Filter Tests
 */

/**
 * @ingroup log-filter-tests
 * Test that LogFilterCheck returns true when no filters are active.
 */
class LogFilterNoFilterTestCase : public TestCase
{
  public:
    /** Constructor. */
    LogFilterNoFilterTestCase()
        : TestCase("LogFilterCheck returns true when no filter is active")
    {
    }

  private:
    void DoRun() override
    {
        LogClearTimeFilter();
        LogClearNodeFilter();
        NS_TEST_ASSERT_MSG_EQ(LogFilterCheck(), true, "No filter active should return true");
    }
};

#ifdef NS3_LOG_ENABLE

/**
 * @ingroup log-filter-tests
 * Test time window filtering: only messages within [start, end] pass.
 */
class LogFilterTimeWindowTestCase : public TestCase
{
  public:
    /** Constructor. */
    LogFilterTimeWindowTestCase()
        : TestCase("Time window filter passes messages within [start, end]")
    {
    }

  private:
    void DoRun() override
    {
        // Ensure providers are registered by initializing the simulator
        Simulator::Destroy();

        // Enable logging for this component
        LogComponentEnable("LogFilterTestSuite", LOG_LEVEL_INFO);

        // Set time filter: 2s to 4s
        int64_t start = Seconds(2).GetTimeStep();
        int64_t end = Seconds(4).GetTimeStep();
        LogSetTimeFilter(start, end);

        // Redirect clog to capture output
        std::ostringstream oss;
        std::streambuf* origBuf = std::clog.rdbuf(oss.rdbuf());

        // Schedule log events at various times
        Simulator::Schedule(Seconds(1), []() { NS_LOG_INFO("at-1s"); });
        Simulator::Schedule(Seconds(2), []() { NS_LOG_INFO("at-2s"); });
        Simulator::Schedule(Seconds(3), []() { NS_LOG_INFO("at-3s"); });
        Simulator::Schedule(Seconds(4), []() { NS_LOG_INFO("at-4s"); });
        Simulator::Schedule(Seconds(5), []() { NS_LOG_INFO("at-5s"); });

        Simulator::Run();
        Simulator::Destroy();

        // Restore clog
        std::clog.rdbuf(origBuf);

        std::string output = oss.str();

        // Messages at 1s and 5s should be filtered out
        NS_TEST_ASSERT_MSG_EQ(output.find("at-1s") == std::string::npos,
                              true,
                              "Message at 1s should be filtered out");
        NS_TEST_ASSERT_MSG_EQ(output.find("at-5s") == std::string::npos,
                              true,
                              "Message at 5s should be filtered out");

        // Messages at 2s, 3s, 4s should pass
        NS_TEST_ASSERT_MSG_NE(output.find("at-2s"), std::string::npos, "Message at 2s should pass");
        NS_TEST_ASSERT_MSG_NE(output.find("at-3s"), std::string::npos, "Message at 3s should pass");
        NS_TEST_ASSERT_MSG_NE(output.find("at-4s"), std::string::npos, "Message at 4s should pass");

        LogClearTimeFilter();
        LogComponentDisable("LogFilterTestSuite", LOG_LEVEL_ALL);
    }
};

/**
 * @ingroup log-filter-tests
 * Test time filter with start only (open end).
 */
class LogFilterTimeStartOnlyTestCase : public TestCase
{
  public:
    /** Constructor. */
    LogFilterTimeStartOnlyTestCase()
        : TestCase("Time filter with start only passes messages from start onward")
    {
    }

  private:
    void DoRun() override
    {
        Simulator::Destroy();
        LogComponentEnable("LogFilterTestSuite", LOG_LEVEL_INFO);

        // Only set start, leave end open (max)
        int64_t start = Seconds(3).GetTimeStep();
        LogSetTimeFilter(start, std::numeric_limits<int64_t>::max());

        std::ostringstream oss;
        std::streambuf* origBuf = std::clog.rdbuf(oss.rdbuf());

        Simulator::Schedule(Seconds(2), []() { NS_LOG_INFO("at-2s"); });
        Simulator::Schedule(Seconds(4), []() { NS_LOG_INFO("at-4s"); });

        Simulator::Run();
        Simulator::Destroy();
        std::clog.rdbuf(origBuf);

        std::string output = oss.str();
        NS_TEST_ASSERT_MSG_EQ(output.find("at-2s") == std::string::npos,
                              true,
                              "Message at 2s should be filtered out (before start)");
        NS_TEST_ASSERT_MSG_NE(output.find("at-4s"),
                              std::string::npos,
                              "Message at 4s should pass (after start)");

        LogClearTimeFilter();
        LogComponentDisable("LogFilterTestSuite", LOG_LEVEL_ALL);
    }
};

/**
 * @ingroup log-filter-tests
 * Test time filter with end only (open start).
 */
class LogFilterTimeEndOnlyTestCase : public TestCase
{
  public:
    /** Constructor. */
    LogFilterTimeEndOnlyTestCase()
        : TestCase("Time filter with end only passes messages until end")
    {
    }

  private:
    void DoRun() override
    {
        Simulator::Destroy();
        LogComponentEnable("LogFilterTestSuite", LOG_LEVEL_INFO);

        int64_t end = Seconds(3).GetTimeStep();
        LogSetTimeFilter(std::numeric_limits<int64_t>::min(), end);

        std::ostringstream oss;
        std::streambuf* origBuf = std::clog.rdbuf(oss.rdbuf());

        Simulator::Schedule(Seconds(2), []() { NS_LOG_INFO("at-2s"); });
        Simulator::Schedule(Seconds(4), []() { NS_LOG_INFO("at-4s"); });

        Simulator::Run();
        Simulator::Destroy();
        std::clog.rdbuf(origBuf);

        std::string output = oss.str();
        NS_TEST_ASSERT_MSG_NE(output.find("at-2s"),
                              std::string::npos,
                              "Message at 2s should pass (before end)");
        NS_TEST_ASSERT_MSG_EQ(output.find("at-4s") == std::string::npos,
                              true,
                              "Message at 4s should be filtered out (after end)");

        LogClearTimeFilter();
        LogComponentDisable("LogFilterTestSuite", LOG_LEVEL_ALL);
    }
};

/**
 * @ingroup log-filter-tests
 * Test node filtering with a single node.
 */
class LogFilterNodeSingleTestCase : public TestCase
{
  public:
    /** Constructor. */
    LogFilterNodeSingleTestCase()
        : TestCase("Node filter with single node passes only that node's messages")
    {
    }

  private:
    void DoRun() override
    {
        Simulator::Destroy();
        LogComponentEnable("LogFilterTestSuite", LOG_LEVEL_INFO);

        // Filter to node 0 only
        std::vector<uint32_t> nodes = {0};
        LogSetNodeFilter(nodes);

        std::ostringstream oss;
        std::streambuf* origBuf = std::clog.rdbuf(oss.rdbuf());

        // Schedule in context 0
        Simulator::ScheduleWithContext(0, Seconds(1), []() { NS_LOG_INFO("node-0"); });
        // Schedule in context 1
        Simulator::ScheduleWithContext(1, Seconds(1), []() { NS_LOG_INFO("node-1"); });

        Simulator::Run();
        Simulator::Destroy();
        std::clog.rdbuf(origBuf);

        std::string output = oss.str();
        NS_TEST_ASSERT_MSG_NE(output.find("node-0"),
                              std::string::npos,
                              "Message from node 0 should pass");
        NS_TEST_ASSERT_MSG_EQ(output.find("node-1") == std::string::npos,
                              true,
                              "Message from node 1 should be filtered out");

        LogClearNodeFilter();
        LogComponentDisable("LogFilterTestSuite", LOG_LEVEL_ALL);
    }
};

/**
 * @ingroup log-filter-tests
 * Test node filtering with multiple nodes.
 */
class LogFilterNodeMultipleTestCase : public TestCase
{
  public:
    /** Constructor. */
    LogFilterNodeMultipleTestCase()
        : TestCase("Node filter with multiple nodes passes matching messages")
    {
    }

  private:
    void DoRun() override
    {
        Simulator::Destroy();
        LogComponentEnable("LogFilterTestSuite", LOG_LEVEL_INFO);

        std::vector<uint32_t> nodes = {0, 2};
        LogSetNodeFilter(nodes);

        std::ostringstream oss;
        std::streambuf* origBuf = std::clog.rdbuf(oss.rdbuf());

        Simulator::ScheduleWithContext(0, Seconds(1), []() { NS_LOG_INFO("node-0"); });
        Simulator::ScheduleWithContext(1, Seconds(1), []() { NS_LOG_INFO("node-1"); });
        Simulator::ScheduleWithContext(2, Seconds(1), []() { NS_LOG_INFO("node-2"); });
        Simulator::ScheduleWithContext(3, Seconds(1), []() { NS_LOG_INFO("node-3"); });

        Simulator::Run();
        Simulator::Destroy();
        std::clog.rdbuf(origBuf);

        std::string output = oss.str();
        NS_TEST_ASSERT_MSG_NE(output.find("node-0"),
                              std::string::npos,
                              "Message from node 0 should pass");
        NS_TEST_ASSERT_MSG_EQ(output.find("node-1") == std::string::npos,
                              true,
                              "Message from node 1 should be filtered out");
        NS_TEST_ASSERT_MSG_NE(output.find("node-2"),
                              std::string::npos,
                              "Message from node 2 should pass");
        NS_TEST_ASSERT_MSG_EQ(output.find("node-3") == std::string::npos,
                              true,
                              "Message from node 3 should be filtered out");

        LogClearNodeFilter();
        LogComponentDisable("LogFilterTestSuite", LOG_LEVEL_ALL);
    }
};

/**
 * @ingroup log-filter-tests
 * Test that NO_CONTEXT events are suppressed by default when
 * node filter is active, and pass when includeNoContext is true.
 */
class LogFilterNodeNoContextTestCase : public TestCase
{
  public:
    /** Constructor. */
    LogFilterNodeNoContextTestCase()
        : TestCase("Node filter suppresses NO_CONTEXT events unless includeNoContext is set")
    {
    }

  private:
    void DoRun() override
    {
        Simulator::Destroy();
        LogComponentEnable("LogFilterTestSuite", LOG_LEVEL_INFO);

        // Filter to node 0, without includeNoContext
        std::vector<uint32_t> nodes = {0};
        LogSetNodeFilter(nodes, false);

        std::ostringstream oss;
        std::streambuf* origBuf = std::clog.rdbuf(oss.rdbuf());

        // Schedule with NO_CONTEXT (no ScheduleWithContext, uses default context)
        Simulator::Schedule(Seconds(1), []() { NS_LOG_INFO("no-context-msg"); });
        Simulator::ScheduleWithContext(0, Seconds(1), []() { NS_LOG_INFO("node-0-msg"); });

        Simulator::Run();
        Simulator::Destroy();
        std::clog.rdbuf(origBuf);

        std::string output = oss.str();
        NS_TEST_ASSERT_MSG_EQ(output.find("no-context-msg") == std::string::npos,
                              true,
                              "NO_CONTEXT message should be filtered out by default");
        NS_TEST_ASSERT_MSG_NE(output.find("node-0-msg"),
                              std::string::npos,
                              "Node 0 message should pass");

        // Now test with includeNoContext = true
        Simulator::Destroy();
        LogSetNodeFilter(nodes, true);

        oss.str("");
        oss.clear();
        origBuf = std::clog.rdbuf(oss.rdbuf());

        Simulator::Schedule(Seconds(1), []() { NS_LOG_INFO("no-context-msg2"); });

        Simulator::Run();
        Simulator::Destroy();
        std::clog.rdbuf(origBuf);

        output = oss.str();
        NS_TEST_ASSERT_MSG_NE(output.find("no-context-msg2"),
                              std::string::npos,
                              "NO_CONTEXT message should pass with includeNoContext=true");

        LogClearNodeFilter();
        LogComponentDisable("LogFilterTestSuite", LOG_LEVEL_ALL);
    }
};

/**
 * @ingroup log-filter-tests
 * Test combined time and node filters (AND behavior).
 */
class LogFilterCombinedTestCase : public TestCase
{
  public:
    /** Constructor. */
    LogFilterCombinedTestCase()
        : TestCase("Combined time and node filters use AND logic")
    {
    }

  private:
    void DoRun() override
    {
        Simulator::Destroy();
        LogComponentEnable("LogFilterTestSuite", LOG_LEVEL_INFO);

        // Time filter: 2s to 4s
        LogSetTimeFilter(Seconds(2).GetTimeStep(), Seconds(4).GetTimeStep());
        // Node filter: node 0 only
        std::vector<uint32_t> nodes = {0};
        LogSetNodeFilter(nodes);

        std::ostringstream oss;
        std::streambuf* origBuf = std::clog.rdbuf(oss.rdbuf());

        // node 0, t=1s -- fails time
        Simulator::ScheduleWithContext(0, Seconds(1), []() { NS_LOG_INFO("n0-t1"); });
        // node 0, t=3s -- passes both
        Simulator::ScheduleWithContext(0, Seconds(3), []() { NS_LOG_INFO("n0-t3"); });
        // node 1, t=3s -- fails node
        Simulator::ScheduleWithContext(1, Seconds(3), []() { NS_LOG_INFO("n1-t3"); });
        // node 0, t=5s -- fails time
        Simulator::ScheduleWithContext(0, Seconds(5), []() { NS_LOG_INFO("n0-t5"); });

        Simulator::Run();
        Simulator::Destroy();
        std::clog.rdbuf(origBuf);

        std::string output = oss.str();
        NS_TEST_ASSERT_MSG_EQ(output.find("n0-t1") == std::string::npos,
                              true,
                              "node 0 at t=1s should fail time filter");
        NS_TEST_ASSERT_MSG_NE(output.find("n0-t3"),
                              std::string::npos,
                              "node 0 at t=3s should pass both filters");
        NS_TEST_ASSERT_MSG_EQ(output.find("n1-t3") == std::string::npos,
                              true,
                              "node 1 at t=3s should fail node filter");
        NS_TEST_ASSERT_MSG_EQ(output.find("n0-t5") == std::string::npos,
                              true,
                              "node 0 at t=5s should fail time filter");

        LogClearTimeFilter();
        LogClearNodeFilter();
        LogComponentDisable("LogFilterTestSuite", LOG_LEVEL_ALL);
    }
};

/**
 * @ingroup log-filter-tests
 * Test that clearing filters restores full output.
 */
class LogFilterClearTestCase : public TestCase
{
  public:
    /** Constructor. */
    LogFilterClearTestCase()
        : TestCase("Clearing filters restores full log output")
    {
    }

  private:
    void DoRun() override
    {
        Simulator::Destroy();
        LogComponentEnable("LogFilterTestSuite", LOG_LEVEL_INFO);

        // Set restrictive filters
        LogSetTimeFilter(Seconds(10).GetTimeStep(), Seconds(20).GetTimeStep());
        std::vector<uint32_t> nodes = {99};
        LogSetNodeFilter(nodes);

        // Clear them
        LogClearTimeFilter();
        LogClearNodeFilter();

        std::ostringstream oss;
        std::streambuf* origBuf = std::clog.rdbuf(oss.rdbuf());

        Simulator::ScheduleWithContext(0, Seconds(1), []() { NS_LOG_INFO("after-clear"); });

        Simulator::Run();
        Simulator::Destroy();
        std::clog.rdbuf(origBuf);

        std::string output = oss.str();
        NS_TEST_ASSERT_MSG_NE(output.find("after-clear"),
                              std::string::npos,
                              "Message should pass after filters are cleared");

        LogComponentDisable("LogFilterTestSuite", LOG_LEVEL_ALL);
    }
};

/**
 * @ingroup log-filter-tests
 * Test LogAddNodeFilter incrementally adds nodes.
 */
class LogFilterAddNodeTestCase : public TestCase
{
  public:
    /** Constructor. */
    LogFilterAddNodeTestCase()
        : TestCase("LogAddNodeFilter incrementally adds nodes to the filter")
    {
    }

  private:
    void DoRun() override
    {
        Simulator::Destroy();
        LogComponentEnable("LogFilterTestSuite", LOG_LEVEL_INFO);

        // Start with empty filter, add node 1
        LogClearNodeFilter();
        LogAddNodeFilter(1);

        std::ostringstream oss;
        std::streambuf* origBuf = std::clog.rdbuf(oss.rdbuf());

        Simulator::ScheduleWithContext(0, Seconds(1), []() { NS_LOG_INFO("node-0"); });
        Simulator::ScheduleWithContext(1, Seconds(1), []() { NS_LOG_INFO("node-1"); });
        Simulator::ScheduleWithContext(2, Seconds(1), []() { NS_LOG_INFO("node-2"); });

        Simulator::Run();
        Simulator::Destroy();
        std::clog.rdbuf(origBuf);

        std::string output = oss.str();
        NS_TEST_ASSERT_MSG_EQ(output.find("node-0") == std::string::npos,
                              true,
                              "Node 0 should be filtered out");
        NS_TEST_ASSERT_MSG_NE(output.find("node-1"), std::string::npos, "Node 1 should pass");
        NS_TEST_ASSERT_MSG_EQ(output.find("node-2") == std::string::npos,
                              true,
                              "Node 2 should be filtered out");

        LogClearNodeFilter();
        LogComponentDisable("LogFilterTestSuite", LOG_LEVEL_ALL);
    }
};

#endif /* NS3_LOG_ENABLE */

/**
 * @ingroup log-filter-tests
 * The log filter test suite.
 */
class LogFilterTestSuite : public TestSuite
{
  public:
    /** Constructor. */
    LogFilterTestSuite()
        : TestSuite("log-filter", Type::UNIT)
    {
        AddTestCase(new LogFilterNoFilterTestCase(), TestCase::Duration::QUICK);
#ifdef NS3_LOG_ENABLE
        AddTestCase(new LogFilterTimeWindowTestCase(), TestCase::Duration::QUICK);
        AddTestCase(new LogFilterTimeStartOnlyTestCase(), TestCase::Duration::QUICK);
        AddTestCase(new LogFilterTimeEndOnlyTestCase(), TestCase::Duration::QUICK);
        AddTestCase(new LogFilterNodeSingleTestCase(), TestCase::Duration::QUICK);
        AddTestCase(new LogFilterNodeMultipleTestCase(), TestCase::Duration::QUICK);
        AddTestCase(new LogFilterNodeNoContextTestCase(), TestCase::Duration::QUICK);
        AddTestCase(new LogFilterCombinedTestCase(), TestCase::Duration::QUICK);
        AddTestCase(new LogFilterClearTestCase(), TestCase::Duration::QUICK);
        AddTestCase(new LogFilterAddNodeTestCase(), TestCase::Duration::QUICK);
#endif /* NS3_LOG_ENABLE */
    }
};

/**
 * @ingroup log-filter-tests
 * LogFilterTestSuite instance variable.
 */
static LogFilterTestSuite g_logFilterTestSuite;
