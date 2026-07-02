/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/log.h"
#include "ns3/test.h"

#include <list>
#include <map>
#include <set>
#include <sstream>
#include <vector>

/**
 * @file
 * @ingroup core-tests
 * @ingroup logging
 * ParameterLogger test suite.
 */

namespace ns3
{

namespace tests
{

/**
 * Verify that ParameterLogger limits the number of logged container elements.
 */
class ParameterLoggerTestCase : public TestCase
{
  public:
    /** Constructor. */
    ParameterLoggerTestCase();

  private:
    void DoRun() override;
};

ParameterLoggerTestCase::ParameterLoggerTestCase()
    : TestCase("Check ParameterLogger container output")
{
}

void
ParameterLoggerTestCase::DoRun()
{
    std::ostringstream stream;
    ParameterLogger logger(stream);
    const auto originalMaximum = logger.GetMaxLoggedContainerElements();
    logger.SetMaxLoggedContainerElements(2);

    logger << std::vector<int>{1, 2, 3} << 4;
    NS_TEST_EXPECT_MSG_EQ(stream.str(), "vector:[1, 2, ...], 4", "Unexpected vector output");

    stream.str("");
    ParameterLogger(stream) << std::list<int>{1, 2, 3};
    NS_TEST_EXPECT_MSG_EQ(stream.str(), "list:[1, 2, ...]", "Unexpected list output");

    stream.str("");
    ParameterLogger(stream) << std::set<int>{1, 2, 3};
    NS_TEST_EXPECT_MSG_EQ(stream.str(), "set:[1, 2, ...]", "Unexpected set output");

    stream.str("");
    ParameterLogger(stream) << std::map<int, int>{{1, 10}, {2, 20}, {3, 30}};
    NS_TEST_EXPECT_MSG_EQ(stream.str(), "map:[1: 10, 2: 20, ...]", "Unexpected map output");

    stream.str("");
    ParameterLogger(stream) << std::vector<int>{1, 2};
    NS_TEST_EXPECT_MSG_EQ(stream.str(), "vector:[1, 2]", "Unexpected exact-limit output");

    logger.SetMaxLoggedContainerElements(0);
    stream.str("");
    ParameterLogger(stream) << std::vector<int>{1, 2, 3};
    NS_TEST_EXPECT_MSG_EQ(stream.str(), "vector:[...]", "Unexpected zero-limit output");

    logger.SetMaxLoggedContainerElements(originalMaximum);
}

/**
 * ParameterLogger test suite.
 */
class ParameterLoggerTestSuite : public TestSuite
{
  public:
    /** Constructor. */
    ParameterLoggerTestSuite();
};

ParameterLoggerTestSuite::ParameterLoggerTestSuite()
    : TestSuite("parameter-logger", Type::UNIT)
{
    AddTestCase(new ParameterLoggerTestCase, TestCase::Duration::QUICK);
}

/** ParameterLoggerTestSuite instance. */
static ParameterLoggerTestSuite g_parameterLoggerTestSuite;

} // namespace tests

} // namespace ns3
