#include "ns3/log.h"
#include "ns3/object.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/tuple.h"
#include "ns3/si-units-ns3.h"
#include "ns3/nstime.h"

using namespace ns3;
using namespace si_units;

NS_LOG_COMPONENT_DEFINE("SiUnitsNstimeTest");



/// Test case for SiUnitsNstime
class TestCaseSiUnitsNstime : public TestCase
{
  public:
    TestCaseSiUnitsNstime()
        : TestCase("SiUnitsNstime") ///< Boilerplate code for TestCase
    {
    }

  private:
    void DoRun() override
    {
        NS_TEST_EXPECT_MSG_EQ((1_Hz * MilliSeconds(1)), 0.001, "");
        NS_TEST_EXPECT_MSG_EQ((1_kHz * MilliSeconds(1)), 1.0, "");
        NS_TEST_EXPECT_MSG_EQ((1_MHz * MilliSeconds(1)), 1000.0, "");
        NS_TEST_EXPECT_MSG_EQ((MilliSeconds(1) * 1_MHz), 1000.0, "");
        NS_TEST_EXPECT_MSG_EQ((MilliSeconds(1) * 1_kHz), 1.0, "");
        NS_TEST_EXPECT_MSG_EQ((MilliSeconds(1) * 1_Hz), 0.001, "");
    }
};

/// Test suite for SiUnits Nstime
class SiUnitsNstimeTestSuite : public TestSuite
{
  public:
    SiUnitsNstimeTestSuite()
        : TestSuite("si-units-nstime-test", Type::UNIT) ///< Add test cases
    {
        AddTestCase(new TestCaseSiUnitsNstime, TestCase::Duration::QUICK);
    }
};

static SiUnitsNstimeTestSuite g_SiUnitsNstimeTestSuite; ///< Register the test suite
