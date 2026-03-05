/*
 * Copyright (c) 2026 Tom Henderson
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/config.h"
#include "ns3/db.h"
#include "ns3/dbm-per-mhz.h"
#include "ns3/dbm.h"
#include "ns3/double.h"
#include "ns3/mhz.h"
#include "ns3/object.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/tuple.h"
#include "ns3/units.h"

#include <type_traits>

using namespace mp_units::si::unit_symbols;

// Note: mp-units maintains its own tests.
// The below tests are for ns-3-specific usage of mp-units and the
// logarithmic wrapper types (dB_t, dBm_t, dBW_t, PSD types).

/**
 * @defgroup units-tests Tests for mp-units integration
 * @ingroup units
 * @ingroup tests
 */

namespace ns3
{
namespace tests
{

/**
 * @ingroup units-tests
 * Class used to check mp-units attributes
 */
class UnitsObjectTest : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return The object TypeId.
     */
    static TypeId GetTypeId()
    {
        static TypeId tid =
            TypeId("ns3::tests::UnitsObjectTest")
                .AddConstructor<UnitsObjectTest>()
                .SetParent<Object>()
                .HideFromDocumentation()
                .AddAttribute("TestDb",
                              "help text",
                              DbValue(0.0 * dB),
                              MakeDbAccessor(&UnitsObjectTest::m_dbTest),
                              MakeDbChecker())
                .AddAttribute("TestDbWithChecker",
                              "help text",
                              DbValue(0.0 * dB),
                              MakeDbAccessor(&UnitsObjectTest::m_dbTestChecker),
                              MakeDbChecker(-3.0 * dB, 3.0 * dB))
                .AddAttribute("TestDbWithDoubleValue",
                              "help text",
                              DoubleValue(0),
                              MakeDbAccessor(&UnitsObjectTest::m_dbTestDoubleValue),
                              MakeDbChecker())
                .AddAttribute("TestDbm",
                              "help text",
                              DbmValue(30.0 * dBm),
                              MakeDbmAccessor(&UnitsObjectTest::m_dbmTest),
                              MakeDbmChecker())
                .AddAttribute("TestMhz",
                              "help text",
                              MhzValue(MHz_t{}),
                              MakeMhzAccessor(&UnitsObjectTest::m_mhzTest),
                              MakeMhzChecker())
                .AddAttribute("TestDbmPerMhz",
                              "help text",
                              DbmPerMhzValue(dBm_per_MHz_t{}),
                              MakeDbmPerMhzAccessor(&UnitsObjectTest::m_dbmPerMhzTest),
                              MakeDbmPerMhzChecker());
        return tid;
    }

    UnitsObjectTest()
    {
    }

    ~UnitsObjectTest() override
    {
    }

  private:
    dB_t m_dbTest;                 ///< "TestDb" attribute
    dB_t m_dbTestChecker;          ///< "TestDbWithChecker" attribute
    dB_t m_dbTestDoubleValue;      ///< "TestDbWithDoubleValue" attribute
    dBm_t m_dbmTest;               ///< "TestDbm" attribute
    MHz_t m_mhzTest;               ///< "TestMhz" attribute
    dBm_per_MHz_t m_dbmPerMhzTest; ///< "TestDbmPerMhz" attribute
};

NS_OBJECT_ENSURE_REGISTERED(UnitsObjectTest);

/**
 * @ingroup units-tests
 * Test case for frequency units with mp-units
 */
class UnitsFrequencyTestCase : public TestCase
{
  public:
    UnitsFrequencyTestCase();

  private:
    void DoRun() override;
};

UnitsFrequencyTestCase::UnitsFrequencyTestCase()
    : TestCase("Test mp-units for frequency")
{
}

void
UnitsFrequencyTestCase::DoRun()
{
    Hz_t fiveHz{5.0 * Hz};
    NS_TEST_ASSERT_MSG_EQ(fiveHz.numerical_value_in(Hz), 5, "Check Hz construction");

    MHz_t fiveMHz{5.0 * MHz};
    NS_TEST_ASSERT_MSG_EQ(fiveMHz.numerical_value_in(MHz), 5, "Check MHz construction");

    auto tenMHz = 2.0 * fiveMHz;
    MHz_t tenMHzTwo{10.0 * MHz};
    NS_TEST_ASSERT_MSG_EQ(tenMHz, tenMHzTwo, "Check multiplication by scalar");
    auto fiveMHzThree = tenMHz / 2.0;
    NS_TEST_ASSERT_MSG_EQ(fiveMHz, fiveMHzThree, "Check division by scalar");

    // mp-units addition of compatible units produces result in common unit
    auto sum = fiveMHz + fiveHz;
    NS_TEST_ASSERT_MSG_EQ(sum.numerical_value_in(Hz),
                          5000005,
                          "Check addition of compatible units");

    auto difference = fiveMHz - fiveHz;
    NS_TEST_ASSERT_MSG_EQ(difference.numerical_value_in(Hz),
                          4999995,
                          "Check subtraction of compatible units");
    auto negativeDifference = fiveHz - fiveMHz;
    NS_TEST_ASSERT_MSG_EQ(negativeDifference.numerical_value_in(Hz),
                          -4999995,
                          "Frequency is allowed to be negative");

    Hz_t halfHz{0.5 * Hz};
    NS_TEST_ASSERT_MSG_EQ(halfHz.numerical_value_in(Hz), 0.5, "Check fractional frequency");
}

/**
 * @ingroup units-tests
 * Test case for power units with mp-units
 */
class UnitsPowerTestCase : public TestCase
{
  public:
    UnitsPowerTestCase();

  private:
    void DoRun() override;
};

UnitsPowerTestCase::UnitsPowerTestCase()
    : TestCase("Test mp-units for power")
{
}

void
UnitsPowerTestCase::DoRun()
{
    mWatt_t hundredmw{100.0 * mW};
    NS_TEST_ASSERT_MSG_EQ(hundredmw.numerical_value_in(mW), 100, "Check mW construction");

    // Check mW to W conversion via mp-units
    Watt_t hundredmwInWatts{hundredmw};
    NS_TEST_ASSERT_MSG_EQ_TOL(hundredmwInWatts.numerical_value_in(W),
                              0.1,
                              1e-12,
                              "Check unit conversion mW to W");

    Watt_t oneWatt{1.0 * W};
    auto sum = oneWatt + hundredmw;
    NS_TEST_ASSERT_MSG_EQ_TOL(sum.numerical_value_in(W),
                              1.1,
                              1e-12,
                              "Check sum of compatible units");
    auto difference = oneWatt - hundredmw;
    NS_TEST_ASSERT_MSG_EQ_TOL(difference.numerical_value_in(W),
                              0.9,
                              1e-12,
                              "Check difference of compatible units");

    // Check mW to dBm conversion (logarithmic wrapper)
    dBm_t hundredmwDbm{hundredmw};
    NS_TEST_ASSERT_MSG_EQ_TOL(hundredmwDbm.numerical_value_in(dBm),
                              20.0,
                              1e-10,
                              "Check mW to dBm conversion");
    // Check dBm back to mW
    auto backToMw = mWatt_t(hundredmwDbm);
    NS_TEST_ASSERT_MSG_EQ_TOL(backToMw.numerical_value_in(mW),
                              100.0,
                              1e-10,
                              "Check conversion from dBm back to mW");

    // Check dB + dBm = dBm
    dB_t tenDb = 10.0 * dB;
    dBm_t oneWattDbm = tenDb + hundredmwDbm;
    NS_TEST_ASSERT_MSG_EQ_TOL(oneWattDbm.numerical_value_in(dBm),
                              30.0,
                              1e-10,
                              "Check addition of dB to dBm");
    // Verify by converting back to watts
    auto oneWattBack = Watt_t(oneWattDbm);
    NS_TEST_ASSERT_MSG_EQ_TOL(oneWattBack.numerical_value_in(W),
                              1.0,
                              1e-10,
                              "Check dBm->W round-trip after dB addition");

    // Check dBm - dBm = dB
    dBm_t tenmwDbm = 10.0 * dBm;
    auto differenceDbm = hundredmwDbm - tenmwDbm;
    NS_TEST_ASSERT_MSG_EQ(tenDb, differenceDbm, "Check subtraction of dBm values");
    NS_TEST_ASSERT_MSG_EQ((std::is_same_v<decltype(differenceDbm), dB_t>),
                          true,
                          "Check that (dBm - dBm) produces a variable of type dB");

    // Attribute tests

    // Check Config::SetDefault on attributes
    Config::SetDefault("ns3::tests::UnitsObjectTest::TestDb", DbValue(3.0 * dB));
    Config::SetDefault("ns3::tests::UnitsObjectTest::TestDbm", DbmValue(3.0 * dBm));

    // Check conversion of DoubleValue to DbValue (provided for backward compatibility)
    Config::SetDefault("ns3::tests::UnitsObjectTest::TestDb", DoubleValue(3));
    Config::SetDefault("ns3::tests::UnitsObjectTest::TestDbm", DoubleValue(3));

    auto unitsObject = CreateObject<UnitsObjectTest>();
    dB_t threeDb = 3.0 * dB;
    DbValue v;
    unitsObject->GetAttribute("TestDb", v);
    NS_TEST_ASSERT_MSG_EQ(threeDb, v.Get(), "Check default value override");
    // Check SetAttribute works with DoubleValue
    unitsObject->SetAttribute("TestDb", DoubleValue(0));
    dBm_t threeDbm = 3.0 * dBm;
    DbmValue v2;
    unitsObject->GetAttribute("TestDbm", v2);
    NS_TEST_ASSERT_MSG_EQ(threeDbm, v2.Get(), "Check default value override");
    // Check SetAttribute works with DoubleValue
    unitsObject->SetAttribute("TestDbm", DoubleValue(0));

    DbValue vChecker;
    unitsObject->GetAttribute("TestDbWithChecker", vChecker);
    NS_TEST_ASSERT_MSG_EQ(0.0 * dB, vChecker.Get(), "Check dB value with checker");

    DbValue vDbDoubleValue;
    unitsObject->GetAttribute("TestDbWithDoubleValue", vDbDoubleValue);
    NS_TEST_ASSERT_MSG_EQ(0.0 * dB, vDbDoubleValue.Get(), "Check dB value with double value input");

    // Check operator += and -= for dBm_t, dBW_t, dB_t
    dBm_t dbmQuantity = -60.0 * dBm;
    dB_t dbQuantity = -60.0 * dB;
    dBW_t dbwQuantity = -60.0 * dBW;
    dB_t valueToAdd = 10.0 * dB;
    dbmQuantity += valueToAdd;
    NS_TEST_ASSERT_MSG_EQ(-50.0 * dBm, dbmQuantity, "Could not use operator+= on dBm_t");
    dbwQuantity += valueToAdd;
    NS_TEST_ASSERT_MSG_EQ(-50.0 * dBW, dbwQuantity, "Could not use operator+= on dBW_t");
    dbQuantity += valueToAdd;
    NS_TEST_ASSERT_MSG_EQ(-50.0 * dB, dbQuantity, "Could not use operator+= on dB_t");
    dbmQuantity -= valueToAdd;
    NS_TEST_ASSERT_MSG_EQ(-60.0 * dBm, dbmQuantity, "Could not use operator-= on dBm_t");
    dbwQuantity -= valueToAdd;
    NS_TEST_ASSERT_MSG_EQ(-60.0 * dBW, dbwQuantity, "Could not use operator-= on dBW_t");
    dbQuantity -= valueToAdd;
    NS_TEST_ASSERT_MSG_EQ(-60.0 * dB, dbQuantity, "Could not use operator-= on dB_t");
}

/**
 * @ingroup units-tests
 * Test case for DbmValue deserialization from string
 */
class UnitsDbmValueStringTestCase : public TestCase
{
  public:
    UnitsDbmValueStringTestCase();

  private:
    void DoRun() override;
};

UnitsDbmValueStringTestCase::UnitsDbmValueStringTestCase()
    : TestCase("Test DbmValue deserialization from string (with and without unit suffix)")
{
}

void
UnitsDbmValueStringTestCase::DoRun()
{
    // Test deserialization of plain numeric string (backward compatible)
    DbmValue v1;
    bool ok1 = v1.DeserializeFromString("-72.0", MakeDbmChecker());
    NS_TEST_ASSERT_MSG_EQ(ok1, true, "Deserialization of plain number should succeed");
    NS_TEST_ASSERT_MSG_EQ(v1.Get(), -72.0 * dBm, "Plain number -72.0 should parse as -72.0 dBm");

    // Test deserialization with unit suffix
    DbmValue v2;
    bool ok2 = v2.DeserializeFromString("-72.0_dBm", MakeDbmChecker());
    NS_TEST_ASSERT_MSG_EQ(ok2, true, "Deserialization with unit suffix should succeed");
    NS_TEST_ASSERT_MSG_EQ(v2.Get(), -72.0 * dBm, "String -72.0_dBm should parse as -72.0 dBm");

    // Test that both formats produce the same result
    NS_TEST_ASSERT_MSG_EQ(v1.Get(), v2.Get(), "Both formats should produce identical values");

    // Test positive value without suffix
    DbmValue v3;
    bool ok3 = v3.DeserializeFromString("30", MakeDbmChecker());
    NS_TEST_ASSERT_MSG_EQ(ok3, true, "Positive number deserialization should succeed");
    NS_TEST_ASSERT_MSG_EQ(v3.Get(), 30.0 * dBm, "Plain number 30 should parse as 30.0 dBm");

    // Test positive value with suffix
    DbmValue v4;
    bool ok4 = v4.DeserializeFromString("30_dBm", MakeDbmChecker());
    NS_TEST_ASSERT_MSG_EQ(ok4, true, "Positive number with suffix should succeed");
    NS_TEST_ASSERT_MSG_EQ(v4.Get(), 30.0 * dBm, "String 30_dBm should parse as 30.0 dBm");

    // Test fractional value
    DbmValue v5;
    bool ok5 = v5.DeserializeFromString("-72.5", MakeDbmChecker());
    NS_TEST_ASSERT_MSG_EQ(ok5, true, "Fractional number deserialization should succeed");
    NS_TEST_ASSERT_MSG_EQ(v5.Get(), -72.5 * dBm, "Plain number -72.5 should parse as -72.5 dBm");

    // Test invalid unit suffix (should fail)
    DbmValue v6;
    bool ok6 = v6.DeserializeFromString("-72.0_dBW", MakeDbmChecker());
    NS_TEST_ASSERT_MSG_EQ(ok6, false, "Deserialization with wrong unit suffix should fail");
}

/**
 * @ingroup units-tests
 * Test case for DbValue deserialization from string
 */
class UnitsDbValueStringTestCase : public TestCase
{
  public:
    UnitsDbValueStringTestCase();

  private:
    void DoRun() override;
};

UnitsDbValueStringTestCase::UnitsDbValueStringTestCase()
    : TestCase("Test DbValue deserialization from string (with and without unit suffix)")
{
}

void
UnitsDbValueStringTestCase::DoRun()
{
    // Test deserialization of plain numeric string (backward compatible)
    DbValue v1;
    bool ok1 = v1.DeserializeFromString("10.0", MakeDbChecker());
    NS_TEST_ASSERT_MSG_EQ(ok1, true, "Deserialization of plain number should succeed");
    NS_TEST_ASSERT_MSG_EQ(v1.Get(), 10.0 * dB, "Plain number 10.0 should parse as 10.0 dB");

    // Test deserialization with unit suffix
    DbValue v2;
    bool ok2 = v2.DeserializeFromString("10.0_dB", MakeDbChecker());
    NS_TEST_ASSERT_MSG_EQ(ok2, true, "Deserialization with unit suffix should succeed");
    NS_TEST_ASSERT_MSG_EQ(v2.Get(), 10.0 * dB, "String 10.0_dB should parse as 10.0 dB");

    // Test that both formats produce the same result
    NS_TEST_ASSERT_MSG_EQ(v1.Get(), v2.Get(), "Both formats should produce identical values");

    // Test negative value without suffix
    DbValue v3;
    bool ok3 = v3.DeserializeFromString("-3.5", MakeDbChecker());
    NS_TEST_ASSERT_MSG_EQ(ok3, true, "Negative number deserialization should succeed");
    NS_TEST_ASSERT_MSG_EQ(v3.Get(), -3.5 * dB, "Plain number -3.5 should parse as -3.5 dB");

    // Test negative value with suffix
    DbValue v4;
    bool ok4 = v4.DeserializeFromString("-3.5_dB", MakeDbChecker());
    NS_TEST_ASSERT_MSG_EQ(ok4, true, "Negative number with suffix should succeed");
    NS_TEST_ASSERT_MSG_EQ(v4.Get(), -3.5 * dB, "String -3.5_dB should parse as -3.5 dB");

    // Test zero value
    DbValue v5;
    bool ok5 = v5.DeserializeFromString("0", MakeDbChecker());
    NS_TEST_ASSERT_MSG_EQ(ok5, true, "Zero deserialization should succeed");
    NS_TEST_ASSERT_MSG_EQ(v5.Get(), 0.0 * dB, "Plain number 0 should parse as 0.0 dB");

    // Test invalid unit suffix (should fail)
    DbValue v6;
    bool ok6 = v6.DeserializeFromString("10.0_dBm", MakeDbChecker());
    NS_TEST_ASSERT_MSG_EQ(ok6, false, "Deserialization with wrong unit suffix should fail");
}

/**
 * @ingroup units-tests
 * Test case for dBW_t stream operator
 */
class UnitsDbWStreamTestCase : public TestCase
{
  public:
    UnitsDbWStreamTestCase();

  private:
    void DoRun() override;
};

UnitsDbWStreamTestCase::UnitsDbWStreamTestCase()
    : TestCase("Test dBW_t stream extraction operator (with and without unit suffix)")
{
}

void
UnitsDbWStreamTestCase::DoRun()
{
    // Test plain numeric string (backward compatible)
    std::istringstream iss1("30.0");
    dBW_t value1;
    iss1 >> value1;
    NS_TEST_ASSERT_MSG_EQ(!iss1.fail(), true, "Parsing plain number should succeed");
    NS_TEST_ASSERT_MSG_EQ(value1, 30.0 * dBW, "Plain number 30.0 should parse as 30.0 dBW");

    // Test with unit suffix
    std::istringstream iss2("30.0_dBW");
    dBW_t value2;
    iss2 >> value2;
    NS_TEST_ASSERT_MSG_EQ(!iss2.fail(), true, "Parsing with unit suffix should succeed");
    NS_TEST_ASSERT_MSG_EQ(value2, 30.0 * dBW, "String 30.0_dBW should parse as 30.0 dBW");

    // Test that both formats produce the same result
    NS_TEST_ASSERT_MSG_EQ(value1, value2, "Both formats should produce identical values");

    // Test negative value
    std::istringstream iss3("-10.5");
    dBW_t value3;
    iss3 >> value3;
    NS_TEST_ASSERT_MSG_EQ(!iss3.fail(), true, "Negative number parsing should succeed");
    NS_TEST_ASSERT_MSG_EQ(value3, -10.5 * dBW, "Plain number -10.5 should parse as -10.5 dBW");

    // Test invalid unit suffix (should fail)
    std::istringstream iss4("30.0_dBm");
    dBW_t value4;
    iss4 >> value4;
    NS_TEST_ASSERT_MSG_EQ(iss4.fail(), true, "Parsing with wrong unit suffix should fail");
}

/**
 * @ingroup units-tests
 * Test case for Watt_t stream operator
 */
class UnitsWattStreamTestCase : public TestCase
{
  public:
    UnitsWattStreamTestCase();

  private:
    void DoRun() override;
};

UnitsWattStreamTestCase::UnitsWattStreamTestCase()
    : TestCase("Test Watt_t stream extraction operator (with and without unit suffix)")
{
}

void
UnitsWattStreamTestCase::DoRun()
{
    // Test plain numeric string (backward compatible)
    std::istringstream iss1("1.0");
    Watt_t value1;
    iss1 >> value1;
    NS_TEST_ASSERT_MSG_EQ(!iss1.fail(), true, "Parsing plain number should succeed");
    NS_TEST_ASSERT_MSG_EQ(value1, 1.0 * W, "Plain number 1.0 should parse as 1.0 W");

    // Test with unit suffix
    std::istringstream iss2("1.0_W");
    Watt_t value2;
    iss2 >> value2;
    NS_TEST_ASSERT_MSG_EQ(!iss2.fail(), true, "Parsing with unit suffix should succeed");
    NS_TEST_ASSERT_MSG_EQ(value2, 1.0 * W, "String 1.0_W should parse as 1.0 W");

    // Test that both formats produce the same result
    NS_TEST_ASSERT_MSG_EQ(value1, value2, "Both formats should produce identical values");

    // Test fractional value
    std::istringstream iss3("0.5");
    Watt_t value3;
    iss3 >> value3;
    NS_TEST_ASSERT_MSG_EQ(!iss3.fail(), true, "Fractional number parsing should succeed");
    NS_TEST_ASSERT_MSG_EQ(value3, 0.5 * W, "Plain number 0.5 should parse as 0.5 W");

    // Test with suffix
    std::istringstream iss4("2.5_W");
    Watt_t value4;
    iss4 >> value4;
    NS_TEST_ASSERT_MSG_EQ(!iss4.fail(), true, "Fractional with suffix should succeed");
    NS_TEST_ASSERT_MSG_EQ(value4, 2.5 * W, "String 2.5_W should parse as 2.5 W");

    // Test invalid unit suffix (should fail)
    std::istringstream iss5("1.0_mW");
    Watt_t value5;
    iss5 >> value5;
    NS_TEST_ASSERT_MSG_EQ(iss5.fail(), true, "Parsing with wrong unit suffix should fail");
}

/**
 * @ingroup units-tests
 * Test case for mWatt_t stream operator
 */
class UnitsMilliwattStreamTestCase : public TestCase
{
  public:
    UnitsMilliwattStreamTestCase();

  private:
    void DoRun() override;
};

UnitsMilliwattStreamTestCase::UnitsMilliwattStreamTestCase()
    : TestCase("Test mWatt_t stream extraction operator (with and without unit suffix)")
{
}

void
UnitsMilliwattStreamTestCase::DoRun()
{
    // Test plain numeric string (backward compatible)
    std::istringstream iss1("100.0");
    mWatt_t value1;
    iss1 >> value1;
    NS_TEST_ASSERT_MSG_EQ(!iss1.fail(), true, "Parsing plain number should succeed");
    NS_TEST_ASSERT_MSG_EQ(value1, 100.0 * mW, "Plain number 100.0 should parse as 100.0 mW");

    // Test with unit suffix
    std::istringstream iss2("100.0_mW");
    mWatt_t value2;
    iss2 >> value2;
    NS_TEST_ASSERT_MSG_EQ(!iss2.fail(), true, "Parsing with unit suffix should succeed");
    NS_TEST_ASSERT_MSG_EQ(value2, 100.0 * mW, "String 100.0_mW should parse as 100.0 mW");

    // Test that both formats produce the same result
    NS_TEST_ASSERT_MSG_EQ(value1, value2, "Both formats should produce identical values");

    // Test fractional value
    std::istringstream iss3("50.5");
    mWatt_t value3;
    iss3 >> value3;
    NS_TEST_ASSERT_MSG_EQ(!iss3.fail(), true, "Fractional number parsing should succeed");
    NS_TEST_ASSERT_MSG_EQ(value3, 50.5 * mW, "Plain number 50.5 should parse as 50.5 mW");

    // Test small value with suffix
    std::istringstream iss4("0.1_mW");
    mWatt_t value4;
    iss4 >> value4;
    NS_TEST_ASSERT_MSG_EQ(!iss4.fail(), true, "Small value with suffix should succeed");
    NS_TEST_ASSERT_MSG_EQ(value4, 0.1 * mW, "String 0.1_mW should parse as 0.1 mW");

    // Test invalid unit suffix (should fail)
    std::istringstream iss5("100.0_W");
    mWatt_t value5;
    iss5 >> value5;
    NS_TEST_ASSERT_MSG_EQ(iss5.fail(), true, "Parsing with wrong unit suffix should fail");
}

/**
 * @ingroup units-tests
 * Test object with dB_t attribute for testing different value types
 */
class DbAttributeTestObject : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return The object TypeId.
     */
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("ns3::tests::DbAttributeTestObject")
                                .AddConstructor<DbAttributeTestObject>()
                                .SetParent<Object>()
                                .HideFromDocumentation()
                                .AddAttribute("Gain",
                                              "Gain in decibels",
                                              DbValue(0.0 * dB),
                                              MakeDbAccessor(&DbAttributeTestObject::m_gain),
                                              MakeDbChecker(0.0 * dB, 10.0 * dB));
        return tid;
    }

    DbAttributeTestObject()
    {
    }

    ~DbAttributeTestObject() override
    {
    }

  private:
    dB_t m_gain; ///< "Gain" attribute
};

NS_OBJECT_ENSURE_REGISTERED(DbAttributeTestObject);

/**
 * @ingroup units-tests
 * Test case for setting dB_t attributes with different value types
 */
class UnitsDbAttributeValueTypesTestCase : public TestCase
{
  public:
    UnitsDbAttributeValueTypesTestCase();

  private:
    void DoRun() override;
};

UnitsDbAttributeValueTypesTestCase::UnitsDbAttributeValueTypesTestCase()
    : TestCase("Test setting dB_t attribute with different value types")
{
}

void
UnitsDbAttributeValueTypesTestCase::DoRun()
{
    auto testObject = CreateObject<DbAttributeTestObject>();

    // Test 1: DbValue(1.0 * dB)
    testObject->SetAttribute("Gain", DbValue(1.0 * dB));
    DbValue v1;
    testObject->GetAttribute("Gain", v1);
    NS_TEST_ASSERT_MSG_EQ(v1.Get(), 1.0 * dB, "Setting with DbValue(1.0 * dB) should work");

    // Test 2: DbValue(3.0 * dB)
    testObject->SetAttribute("Gain", DbValue(3.0 * dB));
    DbValue v2;
    testObject->GetAttribute("Gain", v2);
    NS_TEST_ASSERT_MSG_EQ(v2.Get(), 3.0 * dB, "Setting with DbValue(3.0 * dB) should work");

    // Test 3: DoubleValue(4)
    testObject->SetAttribute("Gain", DoubleValue(4));
    DbValue v3;
    testObject->GetAttribute("Gain", v3);
    NS_TEST_ASSERT_MSG_EQ(v3.Get(), 4.0 * dB, "Setting with DoubleValue(4) should work");

    // Test 4: StringValue("5_dB")
    testObject->SetAttribute("Gain", StringValue("5_dB"));
    DbValue v4;
    testObject->GetAttribute("Gain", v4);
    NS_TEST_ASSERT_MSG_EQ(v4.Get(), 5.0 * dB, "Setting with StringValue(\"5_dB\") should work");

    // Test 5: StringValue("6")
    testObject->SetAttribute("Gain", StringValue("6"));
    DbValue v5;
    testObject->GetAttribute("Gain", v5);
    NS_TEST_ASSERT_MSG_EQ(v5.Get(), 6.0 * dB, "Setting with StringValue(\"6\") should work");
}

/**
 * @ingroup units-tests
 * Test case for dBW conversions
 */
class UnitsDbWConversionTestCase : public TestCase
{
  public:
    UnitsDbWConversionTestCase();

  private:
    void DoRun() override;
};

UnitsDbWConversionTestCase::UnitsDbWConversionTestCase()
    : TestCase("Test dBW conversions")
{
}

void
UnitsDbWConversionTestCase::DoRun()
{
    // dBm to dBW: 20 dBm = -10 dBW
    dBm_t twentyDbm = 20.0 * dBm;
    dBW_t fromDbm{twentyDbm};
    NS_TEST_ASSERT_MSG_EQ(fromDbm, -10.0 * dBW, "20 dBm should equal -10 dBW");

    // dBW to dBm
    auto backToDbm = dBm_t(fromDbm);
    NS_TEST_ASSERT_MSG_EQ(backToDbm, twentyDbm, "dBW->dBm round-trip should preserve value");

    // Watt to dBW: 1 W = 0 dBW
    Watt_t oneWatt{1.0 * W};
    dBW_t fromWatt{oneWatt};
    NS_TEST_ASSERT_MSG_EQ_TOL(fromWatt.numerical_value_in(dBW), 0.0, 1e-10, "1 W should be 0 dBW");

    // dBW to Watt
    auto backToWatt = Watt_t(fromWatt);
    NS_TEST_ASSERT_MSG_EQ_TOL(backToWatt.numerical_value_in(W), 1.0, 1e-10, "0 dBW should be 1 W");

    // dBW to mW: 0 dBW = 1000 mW
    auto toMw = mWatt_t(fromWatt);
    NS_TEST_ASSERT_MSG_EQ_TOL(toMw.numerical_value_in(mW), 1000.0, 1e-8, "0 dBW should be 1000 mW");

    // dBW arithmetic: dBW + dB = dBW
    dBW_t loss = -20.0 * dBW;
    dB_t gain = 10.0 * dB;
    dBW_t result = loss + gain;
    NS_TEST_ASSERT_MSG_EQ(result, -10.0 * dBW, "dBW + dB should work");

    // dBW - dBW = dB
    auto ratio = (0.0 * dBW) - (-30.0 * dBW);
    NS_TEST_ASSERT_MSG_EQ(ratio, 30.0 * dB, "dBW - dBW should produce dB");
    NS_TEST_ASSERT_MSG_EQ((std::is_same_v<decltype(ratio), dB_t>),
                          true,
                          "dBW - dBW should return dB_t type");
}

/**
 * @ingroup units-tests
 * Test case for PSD types
 */
class UnitsPsdTestCase : public TestCase
{
  public:
    UnitsPsdTestCase();

  private:
    void DoRun() override;
};

UnitsPsdTestCase::UnitsPsdTestCase()
    : TestCase("Test power spectral density types")
{
}

void
UnitsPsdTestCase::DoRun()
{
    // dBm_per_Hz * Hz = dBm
    // -50 dBm/Hz * 1000 Hz:
    // Linear: 10^(-50/10) = 1e-5 mW/Hz * 1000 Hz = 0.01 mW = -20 dBm
    dBm_per_Hz_t psd1 = -50.0 * dBm_per_Hz;
    Hz_t bw1{1000.0 * Hz};
    auto total1 = psd1 * bw1;
    NS_TEST_ASSERT_MSG_EQ((std::is_same_v<decltype(total1), dBm_t>),
                          true,
                          "dBm_per_Hz * Hz should return dBm_t");
    NS_TEST_ASSERT_MSG_EQ_TOL(total1.numerical_value_in(dBm),
                              -20.0,
                              1e-10,
                              "-50 dBm/Hz * 1000 Hz should be -20 dBm");

    // Commutativity: Hz * dBm_per_Hz = dBm
    auto total1c = bw1 * psd1;
    NS_TEST_ASSERT_MSG_EQ(total1, total1c, "PSD multiplication should be commutative");

    // dBm_per_MHz * MHz = dBm
    // -20 dBm/MHz * 10 MHz:
    // Linear: 10^(-20/10) = 0.01 mW/MHz * 10 MHz = 0.1 mW = -10 dBm
    dBm_per_MHz_t psd2 = -20.0 * dBm_per_MHz;
    MHz_t bw2{10.0 * MHz};
    auto total2 = psd2 * bw2;
    NS_TEST_ASSERT_MSG_EQ((std::is_same_v<decltype(total2), dBm_t>),
                          true,
                          "dBm_per_MHz * MHz should return dBm_t");
    NS_TEST_ASSERT_MSG_EQ_TOL(total2.numerical_value_in(dBm),
                              -10.0,
                              1e-10,
                              "-20 dBm/MHz * 10 MHz should be -10 dBm");

    // Commutativity: MHz * dBm_per_MHz = dBm
    auto total2c = bw2 * psd2;
    NS_TEST_ASSERT_MSG_EQ(total2, total2c, "PSD multiplication should be commutative");
}

/**
 * @ingroup units-tests
 * Test object with a TupleValue attribute combining DbmValue and DbValue.
 *
 * Models a transmitter configuration: transmit power (dBm) paired with antenna gain (dB).
 * Demonstrates that TupleValue<DbmValue, DbValue> works with mp-units -- the same
 * generic TupleValue mechanism works with any AttributeValue subclass, including the
 * mp-units wrappers.
 */
class UnitsTupleTestObject : public Object
{
  public:
    /** Type alias for the stored tuple: (tx power, antenna gain) */
    using TxConfigTuple = std::tuple<dBm_t, dB_t>;

    /**
     * @brief Get the type ID.
     * @return The object TypeId.
     */
    static TypeId GetTypeId()
    {
        static TypeId tid =
            TypeId("ns3::tests::UnitsTupleTestObject")
                .AddConstructor<UnitsTupleTestObject>()
                .SetParent<Object>()
                .HideFromDocumentation()
                .AddAttribute(
                    "TxConfig",
                    "Transmit power (dBm) and antenna gain (dB)",
                    TupleValue<DbmValue, DbValue>({-20.0 * dBm, 0.0 * dB}),
                    MakeTupleAccessor<DbmValue, DbValue>(&UnitsTupleTestObject::m_txConfig),
                    MakeTupleChecker<DbmValue, DbValue>(MakeDbmChecker(), MakeDbChecker()));
        return tid;
    }

    UnitsTupleTestObject() = default;
    ~UnitsTupleTestObject() override = default;

  private:
    TxConfigTuple m_txConfig; //!< (tx power, antenna gain)
};

NS_OBJECT_ENSURE_REGISTERED(UnitsTupleTestObject);

/**
 * @ingroup units-tests
 * Test case demonstrating TupleValue integration with mp-units AttributeValues.
 *
 * TupleValue<Args...> is a generic ns-3 template parameterised on AttributeValue
 * subclasses. Because DbmValue and DbValue are AttributeValue subclasses with
 * working stream operators, TupleValue<DbmValue, DbValue> is fully functional --
 * both from C++ and via string deserialization. The string deserialization path
 * works because:
 * 1. TupleValue::DeserializeFromString splits "{-15, 3}" on commas -> ["-15", " 3"]
 * 2. Each element is passed to AttributeChecker::CreateValidValue(StringValue(elem))
 * 3. The base-class CreateValidValue creates a DbmValue/DbValue and calls
 *    DeserializeFromString on it (tested separately in UnitsDbmValueStringTestCase
 *    and UnitsDbValueStringTestCase).
 *
 * String format note: no space before the closing '}' to avoid a trailing space
 * in the last element after comma-splitting. This is the same convention used by
 * the existing tuple-value-test-suite.cc.
 */
class UnitsTupleValueTestCase : public TestCase
{
  public:
    UnitsTupleValueTestCase();

  private:
    void DoRun() override;
};

UnitsTupleValueTestCase::UnitsTupleValueTestCase()
    : TestCase("Test TupleValue integration with mp-units AttributeValues")
{
}

void
UnitsTupleValueTestCase::DoRun()
{
    auto obj = CreateObject<UnitsTupleTestObject>();

    // Test 1: Verify default values are set correctly via TupleValue constructor
    TupleValue<DbmValue, DbValue> tv;
    obj->GetAttribute("TxConfig", tv);
    auto [defaultPower, defaultGain] = tv.Get();
    NS_TEST_ASSERT_MSG_EQ(defaultPower, -20.0 * dBm, "Default tx power should be -20 dBm");
    NS_TEST_ASSERT_MSG_EQ(defaultGain, 0.0 * dB, "Default gain should be 0 dB");

    // Test 2: Set via TupleValue<DbmValue, DbValue> and retrieve
    obj->SetAttribute("TxConfig", TupleValue<DbmValue, DbValue>({-15.0 * dBm, 3.0 * dB}));
    TupleValue<DbmValue, DbValue> tv2;
    obj->GetAttribute("TxConfig", tv2);
    auto [txPower2, gain2] = tv2.Get();
    NS_TEST_ASSERT_MSG_EQ(txPower2, -15.0 * dBm, "TupleValue tx power should be -15 dBm");
    NS_TEST_ASSERT_MSG_EQ(gain2, 3.0 * dB, "TupleValue gain should be 3 dB");

    // Test 3: Set via plain-number StringValue -- exercises TupleValue::DeserializeFromString
    // No space before '}': avoids trailing whitespace in the last element after split
    obj->SetAttribute("TxConfig", StringValue("{-10, 5}"));
    TupleValue<DbmValue, DbValue> tv3;
    obj->GetAttribute("TxConfig", tv3);
    auto [txPower3, gain3] = tv3.Get();
    NS_TEST_ASSERT_MSG_EQ(txPower3,
                          -10.0 * dBm,
                          "Plain-number StringValue tx power should be -10 dBm");
    NS_TEST_ASSERT_MSG_EQ(gain3, 5.0 * dB, "Plain-number StringValue gain should be 5 dB");

    // Test 4: Set via unit-suffix StringValue (underscore format accepted by operator>>)
    obj->SetAttribute("TxConfig", StringValue("{ -5_dBm, 2_dB}"));
    TupleValue<DbmValue, DbValue> tv4;
    obj->GetAttribute("TxConfig", tv4);
    auto [txPower4, gain4] = tv4.Get();
    NS_TEST_ASSERT_MSG_EQ(txPower4,
                          -5.0 * dBm,
                          "Unit-suffix StringValue tx power should be -5 dBm");
    NS_TEST_ASSERT_MSG_EQ(gain4, 2.0 * dB, "Unit-suffix StringValue gain should be 2 dB");
}

/**
 * @ingroup units-tests
 * Test case for MhzValue deserialization from string
 */
class UnitsMhzValueStringTestCase : public TestCase
{
  public:
    UnitsMhzValueStringTestCase();

  private:
    void DoRun() override;
};

UnitsMhzValueStringTestCase::UnitsMhzValueStringTestCase()
    : TestCase("Test MhzValue deserialization from string (with and without unit suffix)")
{
}

void
UnitsMhzValueStringTestCase::DoRun()
{
    // Bare number -- assumed MHz
    MhzValue v1;
    bool ok1 = v1.DeserializeFromString("2400", MakeMhzChecker());
    NS_TEST_ASSERT_MSG_EQ(ok1, true, "Bare number deserialization should succeed");
    NS_TEST_ASSERT_MSG_EQ(v1.Get(), 2400.0 * MHz, "Plain 2400 should parse as 2400 MHz");

    // With suffix
    MhzValue v2;
    bool ok2 = v2.DeserializeFromString("2400_MHz", MakeMhzChecker());
    NS_TEST_ASSERT_MSG_EQ(ok2, true, "Suffix deserialization should succeed");
    NS_TEST_ASSERT_MSG_EQ(v2.Get(), 2400.0 * MHz, "2400_MHz should parse as 2400 MHz");

    NS_TEST_ASSERT_MSG_EQ(v1.Get(), v2.Get(), "Both formats should produce identical values");

    // Wrong suffix -- should fail
    MhzValue v3;
    bool ok3 = v3.DeserializeFromString("2400_Hz", MakeMhzChecker());
    NS_TEST_ASSERT_MSG_EQ(ok3, false, "Wrong unit suffix should fail");
}

/**
 * @ingroup units-tests
 * Test case for DbmPerMhzValue deserialization from string
 */
class UnitsDbmPerMhzValueStringTestCase : public TestCase
{
  public:
    UnitsDbmPerMhzValueStringTestCase();

  private:
    void DoRun() override;
};

UnitsDbmPerMhzValueStringTestCase::UnitsDbmPerMhzValueStringTestCase()
    : TestCase("Test DbmPerMhzValue deserialization from string (with and without unit suffix)")
{
}

void
UnitsDbmPerMhzValueStringTestCase::DoRun()
{
    // Bare number -- assumed dBm/MHz
    DbmPerMhzValue v1;
    bool ok1 = v1.DeserializeFromString("-20", MakeDbmPerMhzChecker());
    NS_TEST_ASSERT_MSG_EQ(ok1, true, "Bare number deserialization should succeed");
    NS_TEST_ASSERT_MSG_EQ(v1.Get(), -20.0 * dBm_per_MHz, "Plain -20 should parse as -20 dBm/MHz");

    // With suffix
    DbmPerMhzValue v2;
    bool ok2 = v2.DeserializeFromString("-20_dBm_per_MHz", MakeDbmPerMhzChecker());
    NS_TEST_ASSERT_MSG_EQ(ok2, true, "Suffix deserialization should succeed");
    NS_TEST_ASSERT_MSG_EQ(v2.Get(),
                          -20.0 * dBm_per_MHz,
                          "-20_dBm_per_MHz should parse as -20 dBm/MHz");

    NS_TEST_ASSERT_MSG_EQ(v1.Get(), v2.Get(), "Both formats should produce identical values");

    // Wrong suffix -- should fail
    DbmPerMhzValue v3;
    bool ok3 = v3.DeserializeFromString("-20_dBm", MakeDbmPerMhzChecker());
    NS_TEST_ASSERT_MSG_EQ(ok3, false, "Wrong unit suffix should fail");
}

/**
 * @ingroup units-tests
 * Test case for new type aliases: scalar_t, degree_t, dBr_t, mW_per_Hz_t, mW_per_MHz_t
 */
class UnitsTypeAliasesTestCase : public TestCase
{
  public:
    UnitsTypeAliasesTestCase();

  private:
    void DoRun() override;
};

UnitsTypeAliasesTestCase::UnitsTypeAliasesTestCase()
    : TestCase("Test new type aliases (scalar_t, degree_t, dBr_t, mW_per_Hz_t, mW_per_MHz_t)")
{
}

void
UnitsTypeAliasesTestCase::DoRun()
{
    // scalar_t: dimensionless quantity
    scalar_t s{1.5 * mp_units::one};
    NS_TEST_ASSERT_MSG_EQ_TOL(s.numerical_value_in(mp_units::one),
                              1.5,
                              1e-12,
                              "scalar_t construction and value");
    scalar_t s2 = s * 2.0;
    NS_TEST_ASSERT_MSG_EQ_TOL(s2.numerical_value_in(mp_units::one),
                              3.0,
                              1e-12,
                              "scalar_t arithmetic");

    // degree_t: angle in degrees
    degree_t angle{45.0 * deg};
    NS_TEST_ASSERT_MSG_EQ_TOL(angle.numerical_value_in(deg),
                              45.0,
                              1e-12,
                              "degree_t construction and value");

    // dBr_t is the same type as dB_t
    NS_TEST_ASSERT_MSG_EQ((std::is_same_v<dBr_t, dB_t>), true, "dBr_t must be same type as dB_t");
    dBr_t rel = 6.0 * dB;
    dB_t db = 6.0 * dB;
    NS_TEST_ASSERT_MSG_EQ(rel, db, "dBr_t and dB_t with same value should compare equal");

    // mW_per_Hz_t * Hz_t = mWatt_t (dimensional analysis)
    // 1e-6 mW/Hz * 1e6 Hz = 1 mW
    mW_per_Hz_t psd1 = 1.0e-6 * mW / Hz;
    Hz_t bw1{1.0e6 * Hz};
    auto power1 = psd1 * bw1;
    NS_TEST_ASSERT_MSG_EQ_TOL(power1.numerical_value_in(mW),
                              1.0,
                              1e-10,
                              "mW_per_Hz_t * Hz_t should give mWatt_t (1 mW)");

    // mW_per_MHz_t * MHz_t = mWatt_t (dimensional analysis)
    // 1 mW/MHz * 20 MHz = 20 mW
    mW_per_MHz_t psd2 = 1.0 * mW / MHz;
    MHz_t bw2{20.0 * MHz};
    auto power2 = psd2 * bw2;
    NS_TEST_ASSERT_MSG_EQ_TOL(power2.numerical_value_in(mW),
                              20.0,
                              1e-10,
                              "mW_per_MHz_t * MHz_t should give mWatt_t (20 mW)");
}

/**
 * @ingroup units-tests
 * TestSuite for mp-units integration
 */
class UnitsTestSuite : public TestSuite
{
  public:
    UnitsTestSuite();
};

UnitsTestSuite::UnitsTestSuite()
    : TestSuite("units", Type::UNIT)
{
    AddTestCase(new UnitsFrequencyTestCase, TestCase::Duration::QUICK);
    AddTestCase(new UnitsPowerTestCase, TestCase::Duration::QUICK);
    AddTestCase(new UnitsDbmValueStringTestCase, TestCase::Duration::QUICK);
    AddTestCase(new UnitsDbValueStringTestCase, TestCase::Duration::QUICK);
    AddTestCase(new UnitsDbWStreamTestCase, TestCase::Duration::QUICK);
    AddTestCase(new UnitsWattStreamTestCase, TestCase::Duration::QUICK);
    AddTestCase(new UnitsMilliwattStreamTestCase, TestCase::Duration::QUICK);
    AddTestCase(new UnitsDbAttributeValueTypesTestCase, TestCase::Duration::QUICK);
    AddTestCase(new UnitsDbWConversionTestCase, TestCase::Duration::QUICK);
    AddTestCase(new UnitsPsdTestCase, TestCase::Duration::QUICK);
    AddTestCase(new UnitsTupleValueTestCase, TestCase::Duration::QUICK);
    AddTestCase(new UnitsMhzValueStringTestCase, TestCase::Duration::QUICK);
    AddTestCase(new UnitsDbmPerMhzValueStringTestCase, TestCase::Duration::QUICK);
    AddTestCase(new UnitsTypeAliasesTestCase, TestCase::Duration::QUICK);
}

/**
 * @ingroup units-tests
 * Static variable for test initialization
 */
static UnitsTestSuite unitsTestSuite_g;

} // namespace tests
} // namespace ns3
