/*
 * Copyright (c) 2014 Universitat Autònoma de Barcelona
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 *
 * Author: Rubén Martínez <rmartinez@deic.uab.cat>
 */

#include "ns3/abort.h"
#include "ns3/log.h"
#include "ns3/sdnv.h"
#include "ns3/simulator.h"
#include "ns3/test.h"

#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SdnvTest");

class SdnvTest : public TestCase
{
  public:
    SdnvTest();
    virtual ~SdnvTest();

  private:
    virtual void DoRun(void);

    void SetTests();

    class TestVector
    {
      public:
        uint64_t m_data;
        std::vector<uint8_t> m_encodedData;
    };

    TestVectors<TestVector> m_testVectors;
};

SdnvTest::SdnvTest()
    : TestCase("Check correctness of encoding and decoding methods"),
      m_testVectors()
{
}

SdnvTest::~SdnvTest()
{
}

/*
  Prepare tests: Initialize a testvector containing unsigned integers with its corresponding encoded
  values. The tests contained in this vector describe two different cases:
  - The SDNV encoded value and the unsigned integer representation require the same storage space.
  - The SDNV encoded value requires more space than the unsigned integer representation.
  Both cases are tested for different numeric values, values of varying size that would normally fit
  in: 8, 16, 32, and 64 uint variables.
 */
void
SdnvTest::SetTests()
{
    TestVector testVector;

    testVector.m_data = 0x02;
    testVector.m_encodedData.push_back(0b00000010);
    m_testVectors.Add(testVector);

    testVector.m_data = 0x7F;
    testVector.m_encodedData.clear();
    testVector.m_encodedData.push_back(0b01111111);
    m_testVectors.Add(testVector);

    testVector.m_data = 0x80;
    testVector.m_encodedData.clear();
    testVector.m_encodedData.push_back(0b10000001);
    testVector.m_encodedData.push_back(0b00000000);
    m_testVectors.Add(testVector);

    testVector.m_data = 0xDC;
    testVector.m_encodedData.clear();
    testVector.m_encodedData.push_back(0b10000001);
    testVector.m_encodedData.push_back(0b01011100);
    m_testVectors.Add(testVector);

    testVector.m_data = 0x0ABC;
    testVector.m_encodedData.clear();
    testVector.m_encodedData.push_back(0b10010101);
    testVector.m_encodedData.push_back(0b00111100);
    m_testVectors.Add(testVector);

    testVector.m_data = 0x1234;
    testVector.m_encodedData.clear();
    testVector.m_encodedData.push_back(0b10100100);
    testVector.m_encodedData.push_back(0b00110100);
    m_testVectors.Add(testVector);

    testVector.m_data = 0x4234;
    testVector.m_encodedData.clear();
    testVector.m_encodedData.push_back(0b10000001);
    testVector.m_encodedData.push_back(0b10000100);
    testVector.m_encodedData.push_back(0b00110100);
    m_testVectors.Add(testVector);

    testVector.m_data = 0xFFFF;
    testVector.m_encodedData.clear();
    testVector.m_encodedData.push_back(0b10000011);
    testVector.m_encodedData.push_back(0b11111111);
    testVector.m_encodedData.push_back(0b01111111);
    m_testVectors.Add(testVector);

    testVector.m_data = 0x0FFFFFFF;
    testVector.m_encodedData.clear();
    testVector.m_encodedData.push_back(0b11111111);
    testVector.m_encodedData.push_back(0b11111111);
    testVector.m_encodedData.push_back(0b11111111);
    testVector.m_encodedData.push_back(0b01111111);
    m_testVectors.Add(testVector);

    testVector.m_data = 0x12345678;
    testVector.m_encodedData.clear();
    testVector.m_encodedData.push_back(0b10000001);
    testVector.m_encodedData.push_back(0b10010001);
    testVector.m_encodedData.push_back(0b11010001);
    testVector.m_encodedData.push_back(0b10101100);
    testVector.m_encodedData.push_back(0b01111000);
    m_testVectors.Add(testVector);

    testVector.m_data = 0x80000000;
    testVector.m_encodedData.clear();
    testVector.m_encodedData.push_back(0b10001000);
    testVector.m_encodedData.push_back(0b10000000);
    testVector.m_encodedData.push_back(0b10000000);
    testVector.m_encodedData.push_back(0b10000000);
    testVector.m_encodedData.push_back(0b00000000);
    m_testVectors.Add(testVector);

    testVector.m_data = 0x00000ABC00000ABCLLU;
    testVector.m_encodedData.clear();
    testVector.m_encodedData.push_back(0b10000010);
    testVector.m_encodedData.push_back(0b11010111);
    testVector.m_encodedData.push_back(0b11000000);
    testVector.m_encodedData.push_back(0b10000000);
    testVector.m_encodedData.push_back(0b10000000);
    testVector.m_encodedData.push_back(0b10010101);
    testVector.m_encodedData.push_back(0b00111100);
    m_testVectors.Add(testVector);

    testVector.m_data = 0x0002FABC00000ABCLLU;
    testVector.m_encodedData.clear();
    testVector.m_encodedData.push_back(0b10000001);
    testVector.m_encodedData.push_back(0b10111110);
    testVector.m_encodedData.push_back(0b11010111);
    testVector.m_encodedData.push_back(0b11000000);
    testVector.m_encodedData.push_back(0b10000000);
    testVector.m_encodedData.push_back(0b10000000);
    testVector.m_encodedData.push_back(0b10010101);
    testVector.m_encodedData.push_back(0b00111100);
    m_testVectors.Add(testVector);

    testVector.m_data = 0x01FFFABC00000ABCLLU;
    testVector.m_encodedData.clear();
    testVector.m_encodedData.push_back(0b10000001);
    testVector.m_encodedData.push_back(0b11111111);
    testVector.m_encodedData.push_back(0b11111110);
    testVector.m_encodedData.push_back(0b11010111);
    testVector.m_encodedData.push_back(0b11000000);
    testVector.m_encodedData.push_back(0b10000000);
    testVector.m_encodedData.push_back(0b10000000);
    testVector.m_encodedData.push_back(0b10010101);
    testVector.m_encodedData.push_back(0b00111100);
    m_testVectors.Add(testVector);

    testVector.m_data = 0xFFFFFFFFFFFFFFFFLLU;
    testVector.m_encodedData.clear();
    testVector.m_encodedData.push_back(0b10000001);
    testVector.m_encodedData.push_back(0b11111111);
    testVector.m_encodedData.push_back(0b11111111);
    testVector.m_encodedData.push_back(0b11111111);
    testVector.m_encodedData.push_back(0b11111111);
    testVector.m_encodedData.push_back(0b11111111);
    testVector.m_encodedData.push_back(0b11111111);
    testVector.m_encodedData.push_back(0b11111111);
    testVector.m_encodedData.push_back(0b11111111);
    testVector.m_encodedData.push_back(0b01111111);
    m_testVectors.Add(testVector);
}

void
SdnvTest::DoRun(void)
{
    Sdnv codec;

    SetTests();

    for (uint32_t i = 0; i < m_testVectors.GetN(); ++i)
    {
        TestVector testVector = m_testVectors.Get(i);
        bool test = false;

        // Test 1: Check size of encoded value
        test = codec.Encode(testVector.m_data).size() == testVector.m_encodedData.size();
        NS_TEST_EXPECT_MSG_EQ(test, true, "sdnv wrong size");

        // Test 2: Check encode method.
        test = codec.Encode(testVector.m_data) == testVector.m_encodedData;
        NS_TEST_EXPECT_MSG_EQ(test, true, "sdnv encoding failed");

        // Test 3: Check decode method.
        test = codec.Decode(testVector.m_encodedData) == testVector.m_data;
        NS_TEST_EXPECT_MSG_EQ(test, true, "sdnv decoding failed");
    }
}

class SdnvTestSuite : public TestSuite
{
  public:
    SdnvTestSuite();
};

SdnvTestSuite::SdnvTestSuite()
    : TestSuite("sdnv-test-suite", Type::UNIT)
{
    AddTestCase(new SdnvTest, TestCase::Duration::QUICK);
}

static SdnvTestSuite SdnvTestSuite;
