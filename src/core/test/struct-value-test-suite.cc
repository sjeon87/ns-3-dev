/*
 * Copyright (c) 2026 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "ns3/double.h"
#include "ns3/object.h"
#include "ns3/string.h"
#include "ns3/struct.h"
#include "ns3/test.h"
#include "ns3/uinteger.h"

#include <cstdint>
#include <string>

using namespace ns3;

/** Structure used by the StructValue tests. */
struct StructTestData
{
    std::string name; //!< Name field
    uint16_t count;   //!< Count field
    double value;     //!< Value field

    /**
     * Compare two structures.
     *
     * @return true if all the fields are equal
     */
    bool operator==(const StructTestData&) const = default;
};

/** StructValue type used by the tests. */
using StructTestValue = StructValue<StructTestData,
                                    StructField<StringValue, &StructTestData::name>,
                                    StructField<UintegerValue, &StructTestData::count>,
                                    StructField<DoubleValue, &StructTestData::value>>;

/** Object containing attributes represented by structures. */
class StructObject : public Object
{
  public:
    /**
     * Get the type ID.
     *
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Set the structure accessed through methods.
     *
     * @param value the structure to set
     */
    void SetMethodStruct(const StructTestData& value);

    /**
     * Get the structure accessed through methods.
     *
     * @return the stored structure
     */
    StructTestData GetMethodStruct() const;

    StructTestData m_memberStruct; //!< Structure accessed as a data member

  private:
    StructTestData m_methodStruct; //!< Structure accessed through methods
};

TypeId
StructObject::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::StructObject")
            .SetParent<Object>()
            .SetGroupName("Test")
            .AddConstructor<StructObject>()
            .AddAttribute("MemberStruct",
                          "Structure accessed as a data member",
                          StructTestValue(StructTestData{"member", 2, 3.5}),
                          MakeStructAccessor<StructTestValue>(&StructObject::m_memberStruct),
                          MakeStructChecker<StructTestValue>(MakeStringChecker(),
                                                             MakeUintegerChecker<uint16_t>(1, 10),
                                                             MakeDoubleChecker<double>(0.0, 10.0)))
            .AddAttribute("MethodStruct",
                          "Structure accessed through a getter and a setter",
                          StructTestValue(StructTestData{"method", 4, 5.5}),
                          MakeStructAccessor<StructTestValue>(&StructObject::SetMethodStruct,
                                                              &StructObject::GetMethodStruct),
                          MakeStructChecker<StructTestValue>(MakeStringChecker(),
                                                             MakeUintegerChecker<uint16_t>(1, 10),
                                                             MakeDoubleChecker<double>(0.0, 10.0)));
    return tid;
}

void
StructObject::SetMethodStruct(const StructTestData& value)
{
    m_methodStruct = value;
}

StructTestData
StructObject::GetMethodStruct() const
{
    return m_methodStruct;
}

/** Test StructValue construction, checking, serialization, and access. */
class StructValueTestCase : public TestCase
{
  public:
    /** Constructor. */
    StructValueTestCase();

  private:
    void DoRun() override;
};

StructValueTestCase::StructValueTestCase()
    : TestCase("test StructValue attribute value")
{
}

void
StructValueTestCase::DoRun()
{
    auto object = CreateObject<StructObject>();

    NS_TEST_ASSERT_MSG_EQ(object->m_memberStruct == StructTestData("member", 2, 3.5),
                          true,
                          "The data member did not receive its default value");
    NS_TEST_ASSERT_MSG_EQ(object->GetMethodStruct() == StructTestData("method", 4, 5.5),
                          true,
                          "The setter did not receive its default value");

    auto success = object->SetAttributeFailSafe("MemberStruct",
                                                StructTestValue(StructTestData{"updated", 6, 7.5}));
    NS_TEST_ASSERT_MSG_EQ(success, true, "Setting a valid StructValue failed");
    NS_TEST_ASSERT_MSG_EQ(object->m_memberStruct == StructTestData("updated", 6, 7.5),
                          true,
                          "The data member was not updated");

    StructTestValue value;
    success = object->GetAttributeFailSafe("MemberStruct", value);
    NS_TEST_ASSERT_MSG_EQ(success, true, "Getting a StructValue failed");
    NS_TEST_ASSERT_MSG_EQ(value.Get() == StructTestData("updated", 6, 7.5),
                          true,
                          "The StructValue does not contain the data member fields");

    success = object->SetAttributeFailSafe("MethodStruct", StringValue("{parsed, 8, 9.5}"));
    NS_TEST_ASSERT_MSG_EQ(success, true, "Deserializing a valid structure failed");
    NS_TEST_ASSERT_MSG_EQ(object->GetMethodStruct() == StructTestData("parsed", 8, 9.5),
                          true,
                          "The deserialized structure was not passed to the setter");

    success = object->SetAttributeFailSafe("MethodStruct",
                                           StructTestValue(StructTestData{"invalid", 11, 2.5}));
    NS_TEST_ASSERT_MSG_EQ(success, false, "A field outside its valid range was accepted");
    NS_TEST_ASSERT_MSG_EQ(object->GetMethodStruct() == StructTestData("parsed", 8, 9.5),
                          true,
                          "The structure changed after a failed assignment");

    success = object->SetAttributeFailSafe("MethodStruct", StringValue("{incomplete}"));
    NS_TEST_ASSERT_MSG_EQ(success, false, "A structure with missing fields was accepted");
    NS_TEST_ASSERT_MSG_EQ(object->GetMethodStruct() == StructTestData("parsed", 8, 9.5),
                          true,
                          "The structure changed after failed deserialization");

    auto checker = MakeStructChecker<StructTestValue>(MakeStringChecker(),
                                                      MakeUintegerChecker<uint16_t>(1, 10),
                                                      MakeDoubleChecker<double>(0.0, 10.0));
    NS_TEST_ASSERT_MSG_EQ(value.SerializeToString(checker),
                          "{updated, 6, 7.5}",
                          "The structure was not serialized as a tuple");

    auto copy = DynamicCast<StructTestValue>(value.Copy());
    NS_TEST_ASSERT_MSG_NE(copy, nullptr, "Copy returned the wrong AttributeValue type");
    NS_TEST_ASSERT_MSG_EQ(copy->Get() == value.Get(),
                          true,
                          "Copy did not preserve the structure fields");
}

/** StructValue test suite. */
class StructValueTestSuite : public TestSuite
{
  public:
    /** Constructor. */
    StructValueTestSuite();
};

StructValueTestSuite::StructValueTestSuite()
    : TestSuite("struct-value-test-suite", Type::UNIT)
{
    AddTestCase(new StructValueTestCase(), TestCase::Duration::QUICK);
}

static StructValueTestSuite g_structValueTestSuite; //!< Static test suite registration
