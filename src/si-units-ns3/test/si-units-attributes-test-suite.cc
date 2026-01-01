#include "ns3/log.h"
#include "ns3/object.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/tuple.h"
#include "ns3/si-units-attributes.h"

using namespace ns3;
using namespace si_units;

NS_LOG_COMPONENT_DEFINE("SiUnitsAttribsTest");


/// A mock class with attributes
class AttributeMock : public Object
{
  public:
    /// Get the type ID.
    /// @return The type ID.
    static TypeId GetTypeId()
    {
        static TypeId tid = //
            TypeId("ns3:AttributeMock")
                .SetParent<Object>()
                .SetGroupName("AttributeMock")
                .AddConstructor<AttributeMock>()
                .AddAttribute("dB",
                              "help message for dB",
                              dBValue(0_dB),
                              MakedBAccessor(&AttributeMock::m_dB),
                              MakedBChecker())
                .AddAttribute("dBr",
                              "help message for dBr",
                              dBrValue(0_dBr),
                              MakedBrAccessor(&AttributeMock::m_dBr),
                              MakedBrChecker())
                .AddAttribute("dBm",
                              "help message for dBm",
                              dBmValue(20_dBm),
                              MakedBmAccessor(&AttributeMock::m_dBm),
                              MakedBmChecker())
                .AddAttribute("mWatt",
                              "help message for mWatt",
                              mWattValue(100_mWatt),
                              MakemWattAccessor(&AttributeMock::m_mWatt),
                              MakemWattChecker())
                .AddAttribute("Watt",
                              "help message for Watt",
                              WattValue(123_Watt),
                              MakeWattAccessor(&AttributeMock::m_Watt),
                              MakeWattChecker())
                .AddAttribute("dBm_per_Hz",
                              "help message for dBm_per_Hz",
                              dBm_per_HzValue(0.0004_dBm_per_Hz),
                              MakedBm_per_HzAccessor(&AttributeMock::m_dBm_per_Hz),
                              MakedBm_per_HzChecker())
                .AddAttribute("dBm_per_MHz",
                              "help message for dBm_per_MHz",
                              dBm_per_MHzValue(0.001_dBm_per_MHz),
                              MakedBm_per_MHzAccessor(&AttributeMock::m_dBm_per_MHz),
                              MakedBm_per_MHzChecker())
                .AddAttribute("Hz",
                              "help message for Hz",
                              HzValue(415000_Hz),
                              MakeHzAccessor(&AttributeMock::m_Hz),
                              MakeHzChecker())
                .AddAttribute("kHz",
                              "help message for kHz",
                              kHzValue(415_kHz),
                              MakekHzAccessor(&AttributeMock::m_kHz),
                              MakekHzChecker())
                .AddAttribute("MHz",
                              "help message for MHz",
                              MHzValue(300_MHz),
                              MakeMHzAccessor(&AttributeMock::m_MHz),
                              MakeMHzChecker())
                .AddAttribute("GHz",
                              "help message for GHz",
                              GHzValue(300_GHz),
                              MakeGHzAccessor(&AttributeMock::m_GHz),
                              MakeGHzChecker())
                .AddAttribute("THz",
                              "help message for THz",
                              THzValue(300_THz),
                              MakeTHzAccessor(&AttributeMock::m_THz),
                              MakeTHzChecker())
                .AddAttribute("nSEC",
                              "help message for nSEC",
                              nSECValue(-20_nSEC),
                              MakenSECAccessor(&AttributeMock::m_nSEC),
                              MakenSECChecker())
                .AddAttribute("degree",
                              "help message for degree",
                              degreeValue(720_degree),
                              MakedegreeAccessor(&AttributeMock::m_degree),
                              MakedegreeChecker())
                .AddAttribute("radian",
                              "help message for radian",
                              radianValue(20_radian),
                              MakeradianAccessor(&AttributeMock::m_radian),
                              MakeradianChecker())
                .AddAttribute("percent",
                              "help message for percent",
                              percentValue(20.5_percent),
                              MakepercentAccessor(&AttributeMock::m_percent),
                              MakepercentChecker())
                .AddAttribute("tuple1",
                              "a tuple of 1 entry setting with TupleValue",
                              TupleValue<dBmValue>(-1.5_dBm),
                              MakeTupleAccessor<dBmValue>(&AttributeMock::m_tuple1),
                              MakeTupleChecker<dBmValue>(MakedBmChecker()))
                .AddAttribute("tuple2",
                              "a tuple of 1 entry setting with StringValue",
                              StringValue("{-1.5dBm}"),
                              MakeTupleAccessor<dBmValue>(&AttributeMock::m_tuple2),
                              MakeTupleChecker<dBmValue>(MakedBmChecker()))
                .AddAttribute(
                    "tuple3",
                    "a tuple of 2 entries setting with TupleValue",
                    TupleValue<dBmValue, dBmValue>({-1.5_dBm, 1.5_dBm}),
                    MakeTupleAccessor<dBmValue, dBmValue>(&AttributeMock::m_tuple3),
                    MakeTupleChecker<dBmValue, dBmValue>(MakedBmChecker(), MakedBmChecker()))
                .AddAttribute(
                    "tuple4",
                    "a tuple of 2 entries setting with StringValue",
                    StringValue("{-1.5dBm, 1.5dBm}"),
                    MakeTupleAccessor<dBmValue, dBmValue>(&AttributeMock::m_tuple4),
                    MakeTupleChecker<dBmValue, dBmValue>(MakedBmChecker(), MakedBmChecker()));

        return tid;
    }

    dB_t m_dB{};                         ///< value of dB
    dBr_t m_dBr{};                       ///< value of dBr
    dBm_t m_dBm{};                       ///< value of dBm
    mWatt_t m_mWatt{};                   ///< value of mWatt
    Watt_t m_Watt{};                     ///< value of Watt
    dBm_per_Hz_t m_dBm_per_Hz{};         ///< value of dBm_per_Hz
    dBm_per_MHz_t m_dBm_per_MHz{};       ///< value of dBm_per_MHz
    Hz_t m_Hz{};                         ///< value of Hz
    kHz_t m_kHz{};                       ///< value of kHz
    GHz_t m_GHz{};                       ///< value of GHz
    THz_t m_THz{};                       ///< value of THz
    MHz_t m_MHz{};                       ///< value of MHz
    nSEC_t m_nSEC{};                     ///< value of nSEC
    degree_t m_degree{};                 ///< value of degree
    radian_t m_radian{};                 ///< value of radian
    percent_t m_percent{};               ///< value of percent
    std::tuple<dBm_t> m_tuple1{};        ///< tuple of dBm_t
    std::tuple<dBm_t> m_tuple2{};        ///< tuple of dBm_t
    std::tuple<dBm_t, dBm_t> m_tuple3{}; ///< tuple of dBm_t
    std::tuple<dBm_t, dBm_t> m_tuple4{}; ///< tuple of dBm_t
};

/// Test case for SiUnitsAttributes
class TestCaseSiUnitsAttributes : public TestCase
{
  public:
    TestCaseSiUnitsAttributes()
        : TestCase("SiUnitsAttributes") ///< Boilerplate code for TestCase
    {
    }

  private:
    void DoRun() override
    {
        auto mock = CreateObject<AttributeMock>();

        {
            auto want = 9_dB;
            mock->SetAttribute("dB", dBValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_dB, want, "");
        }
        {
            auto want = 10_dB;
            mock->SetAttribute("dB", DoubleValue(10));
            NS_TEST_EXPECT_MSG_EQ(mock->m_dB, want, "");
        }
        {
            auto want = 25_dBr;
            mock->SetAttribute("dBr", dBrValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_dBr, want, "");
        }
        {
            auto want = 5_dBr;
            mock->SetAttribute("dBr", DoubleValue(5));
            NS_TEST_EXPECT_MSG_EQ(mock->m_dBr, want, "");
        }
        {
            auto want = 20_dBm;
            mock->SetAttribute("dBm", dBmValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_dBm, want, "");
        }
        {
            auto want = 10_dBm;
            mock->SetAttribute("dBm", DoubleValue(10));
            NS_TEST_EXPECT_MSG_EQ(mock->m_dBm, want, "");
        }
        {
            auto want = 100_mWatt;
            mock->SetAttribute("mWatt", mWattValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_mWatt, want, "");
        }
        {
            auto want = 50_mWatt;
            mock->SetAttribute("mWatt", DoubleValue(50));
            NS_TEST_EXPECT_MSG_EQ(mock->m_mWatt, want, "");
        }
        {
            auto want = 221_Watt;
            mock->SetAttribute("Watt", WattValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_Watt, want, "");
        }
        {
            auto want = 0.0001_dBm_per_Hz;
            mock->SetAttribute("dBm_per_Hz", dBm_per_HzValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_dBm_per_Hz, want, "");
        }
        {
            auto want = 0.001_dBm_per_Hz;
            mock->SetAttribute("dBm_per_Hz", dBm_per_HzValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_dBm_per_Hz, want, "");
        }
        {
            auto want = 0.02_dBm_per_Hz;
            mock->SetAttribute("dBm_per_Hz", DoubleValue(0.02));
            NS_TEST_EXPECT_MSG_EQ(mock->m_dBm_per_Hz, want, "");
        }
        {
            auto want = 0.001_dBm_per_MHz;
            mock->SetAttribute("dBm_per_MHz", dBm_per_MHzValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_dBm_per_MHz, want, "");
        }
        {
            auto want = 0.02_dBm_per_MHz;
            mock->SetAttribute("dBm_per_MHz", DoubleValue(0.02));
            NS_TEST_EXPECT_MSG_EQ(mock->m_dBm_per_MHz, want, "");
        }
        {
            auto want = 365_Hz;
            mock->SetAttribute("Hz", HzValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_Hz, want, "");
        }
        {
            auto want = 500_Hz;
            mock->SetAttribute("Hz", DoubleValue(500));
            NS_TEST_EXPECT_MSG_EQ(mock->m_Hz, want, "");
        }
        {
            auto want = 10_MHz;
            mock->SetAttribute("MHz", MHzValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_MHz, want, "");
        }
        {
            auto want = 123.4_kHz;
            mock->SetAttribute("kHz", kHzValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_kHz, want, "");
        }
        {
            auto want = 100_MHz;
            mock->SetAttribute("MHz", DoubleValue(100));
            NS_TEST_EXPECT_MSG_EQ(mock->m_MHz, want, "");
        }
        {
            auto want = 123.4_GHz;
            mock->SetAttribute("GHz", GHzValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_GHz, want, "");
        }
        {
            auto want = 123.4_THz;
            mock->SetAttribute("THz", THzValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_THz, want, "");
        }
        {
            auto want = 100_nSEC;
            mock->SetAttribute("nSEC", nSECValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_nSEC, want, "");
        }
        {
            auto want = 720_degree;
            mock->SetAttribute("degree", degreeValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_degree, want, "");
        }
        {
            auto want = 360_degree;
            mock->SetAttribute("degree", DoubleValue(360));
            NS_TEST_EXPECT_MSG_EQ(mock->m_degree, want, "");
        }
        {
            auto want = 2.4_radian;
            mock->SetAttribute("radian", radianValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_radian, want, "");
        }
        {
            auto want = 2_radian;
            mock->SetAttribute("radian", DoubleValue(2));
            NS_TEST_EXPECT_MSG_EQ(mock->m_radian, want, "");
        }
        {
            auto want = 2.4_percent;
            mock->SetAttribute("percent", percentValue(want));
            NS_TEST_EXPECT_MSG_EQ(mock->m_percent, want, "");
        }
        {
            auto want = 5.9_percent;
            mock->SetAttribute("percent", DoubleValue(5.9));
            NS_TEST_EXPECT_MSG_EQ(mock->m_percent, want, "");
        }
        { // A tuple of single entry setting with TupleValue
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple1), -1.5_dBm, "");
            mock->SetAttribute("tuple1", TupleValue<dBmValue>(-15_dBm));
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple1), -15_dBm, "");
        }
        { // A tuple of single entry setting with StringValue
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple2), -1.5_dBm, "");

            // change value
            mock->SetAttribute("tuple2", StringValue("{-15dBm}"));
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple2), -15_dBm, "");

            // space after opening brace
            mock->SetAttribute("tuple2", StringValue("{ -27dBm}"));
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple2), -27_dBm, "");

            // space before closing brace
            mock->SetAttribute("tuple2", StringValue("{-35dBm }"));
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple2), -35_dBm, "");

            // space combination of after opening brace and before closing brace
            mock->SetAttribute("tuple2", StringValue("{ -45dBm }"));
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple2), -45_dBm, "");

            // space between the number and the SI unit
            mock->SetAttribute("tuple2", StringValue("{-15 dBm}"));
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple2), -15_dBm, "");
        }
        { // A tuple of two entries setting with TupleValue
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple3), -1.5_dBm, "");
            NS_TEST_EXPECT_MSG_EQ(std::get<1>(mock->m_tuple3), 1.5_dBm, "");
            mock->SetAttribute("tuple3", TupleValue<dBmValue, dBmValue>({-15_dBm, 15_dBm}));
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple3), -15_dBm, "");
            NS_TEST_EXPECT_MSG_EQ(std::get<1>(mock->m_tuple3), 15_dBm, "");
        }
        { // A tuple of two entries setting with StringValueStrong type StringValue attribute
            // tuple access
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple4), -1.5_dBm, "");
            NS_TEST_EXPECT_MSG_EQ(std::get<1>(mock->m_tuple4), 1.5_dBm, "");

            // no space before and after comma
            mock->SetAttribute("tuple4", StringValue("{-1dBm,1dBm}"));
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple4), -1_dBm, "");
            NS_TEST_EXPECT_MSG_EQ(std::get<1>(mock->m_tuple4), 1_dBm, "");

            // space after comma
            mock->SetAttribute("tuple4", StringValue("{-15dBm, 15dBm}"));
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple4), -15_dBm, "");
            NS_TEST_EXPECT_MSG_EQ(std::get<1>(mock->m_tuple4), 15_dBm, "");

            // space before comma
            mock->SetAttribute("tuple4", StringValue("{-25dBm , 25dBm}"));
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple4), -25_dBm, "");
            NS_TEST_EXPECT_MSG_EQ(std::get<1>(mock->m_tuple4), 25_dBm, "");

            // space between comma and between the number and the SI unit
            mock->SetAttribute("tuple4", StringValue("{-35 dBm , 35 dBm}"));
            NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple4), -35_dBm, "");
            NS_TEST_EXPECT_MSG_EQ(std::get<1>(mock->m_tuple4), 35_dBm, "");
        }
        { // Test cases crashing or failing: StringValue for TupleValue

            // // space before opening brace
            // mock->SetAttribute("tuple2", StringValue(" {-25dBm}"));
            // NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple2), -25_dBm, "");

            // // space after closing brace
            // mock->SetAttribute("tuple2", StringValue("{-37dBm} "));
            // NS_TEST_EXPECT_MSG_EQ(std::get<0>(mock->m_tuple2), -37_dBm, "");
        }
    }
};

/// Test suite for SiUnits Attributes
class SiUnitsAttribsTestSuite : public TestSuite
{
  public:
    SiUnitsAttribsTestSuite()
        : TestSuite("si-units-attributes-test", Type::UNIT) ///< Add test cases
    {
        AddTestCase(new TestCaseSiUnitsAttributes, TestCase::Duration::QUICK);
    }
};

static SiUnitsAttribsTestSuite g_siUnitsAttribsTestSuite; ///< Register the test suite
