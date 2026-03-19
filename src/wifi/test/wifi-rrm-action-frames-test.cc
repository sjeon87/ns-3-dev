/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Devansh Gupta <devanshg308@gmail.com>
 */

#include "ns3/header-serialization-test.h"
#include "ns3/log.h"
#include "ns3/measurement-request-element.h"
#include "ns3/radio-measurement-request.h"
#include "ns3/test.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiRrmActionFramesTest");

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test serialization and deserialization of RadioMeasurementRequestHeader
 * (IEEE 802.11-2024 Section 9.6.6.2, Figure 9-1185)
 */
class RadioMeasurementRequestTest : public HeaderSerializationTestCase
{
  public:
    RadioMeasurementRequestTest();

  private:
    void DoRun() override;
};

RadioMeasurementRequestTest::RadioMeasurementRequestTest()
    : HeaderSerializationTestCase(
          "Check serialization and deserialization of RadioMeasurementRequestHeader")
{
}

void
RadioMeasurementRequestTest::DoRun()
{
    // Test 1: Default construction round-trip
    {
        RadioMeasurementRequestHeader hdr;
        TestHeaderSerialization(hdr);
        NS_TEST_EXPECT_MSG_EQ(hdr.GetSerializedSize(), 3, "Default size = 3 (token + repetitions)");
        NS_TEST_EXPECT_MSG_EQ(hdr.GetDialogToken(), 0, "Default dialog token");
        NS_TEST_EXPECT_MSG_EQ(hdr.GetNumberOfRepetitions(), 0, "Default repetitions");
        NS_TEST_EXPECT_MSG_EQ(hdr.GetMeasurementRequestElements().size(),
                              0,
                              "Default has no elements");
    }

    // Test 2: Empty request -- dialog token + repetitions only, zero elements
    {
        RadioMeasurementRequestHeader hdr;
        hdr.SetDialogToken(1);
        hdr.SetNumberOfRepetitions(0);

        NS_TEST_EXPECT_MSG_EQ(hdr.GetSerializedSize(), 3, "Empty request size = 3");
        TestHeaderSerialization(hdr);

        Buffer buf;
        buf.AddAtStart(hdr.GetSerializedSize());
        hdr.Serialize(buf.Begin());

        RadioMeasurementRequestHeader deserialized;
        deserialized.Deserialize(buf.Begin());
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetDialogToken(), 1, "Token round-trip");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetNumberOfRepetitions(), 0, "Repetitions round-trip");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetMeasurementRequestElements().size(),
                              0,
                              "No elements");
    }

    // Test 3: Single MeasurementRequestElement round-trip (Beacon request)
    {
        RadioMeasurementRequestHeader hdr;
        hdr.SetDialogToken(42);
        hdr.SetNumberOfRepetitions(10);

        MeasurementRequestElement elem;
        elem.SetMeasurementToken(1);
        elem.SetMeasurementType(5); // BEACON
        MeasurementRequestElement::BeaconRequestBody body;
        body.operatingClass = 81;
        body.channelNumber = 6;
        body.randomizationInterval = 100;
        body.measurementDuration = 200;
        body.measurementMode = 0;
        body.bssid = Mac48Address("ff:ff:ff:ff:ff:ff");
        elem.SetBody(body);
        hdr.AddMeasurementRequestElement(elem);

        NS_TEST_EXPECT_MSG_EQ(hdr.GetSerializedSize(),
                              3U + elem.GetSerializedSize(),
                              "Size = 3 + element size");
        TestHeaderSerialization(hdr);

        Buffer buf;
        buf.AddAtStart(hdr.GetSerializedSize());
        hdr.Serialize(buf.Begin());

        RadioMeasurementRequestHeader deserialized;
        deserialized.Deserialize(buf.Begin());
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetDialogToken(), 42, "Token round-trip");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetNumberOfRepetitions(), 10, "Repetitions round-trip");
        NS_TEST_ASSERT_MSG_EQ(deserialized.GetMeasurementRequestElements().size(),
                              1,
                              "One element");
    }

    // Test 4: Multiple elements round-trip
    {
        RadioMeasurementRequestHeader hdr;
        hdr.SetDialogToken(7);
        hdr.SetNumberOfRepetitions(3);

        // Element 1: Basic request
        MeasurementRequestElement elem1;
        elem1.SetMeasurementToken(1);
        elem1.SetMeasurementType(0); // BASIC
        elem1.SetBody(MeasurementRequestElement::BasicRequestBody{6, 0, 200});
        hdr.AddMeasurementRequestElement(elem1);

        // Element 2: Beacon request
        MeasurementRequestElement elem2;
        elem2.SetMeasurementToken(2);
        elem2.SetMeasurementType(5); // BEACON
        MeasurementRequestElement::BeaconRequestBody body;
        body.operatingClass = 115;
        body.channelNumber = 36;
        body.measurementDuration = 100;
        body.bssid = Mac48Address("ff:ff:ff:ff:ff:ff");
        elem2.SetBody(body);
        hdr.AddMeasurementRequestElement(elem2);

        // Element 3: Channel Load request
        MeasurementRequestElement elem3;
        elem3.SetMeasurementToken(3);
        elem3.SetMeasurementType(3); // CHANNEL_LOAD
        MeasurementRequestElement::ChannelLoadRequestBody clBody;
        clBody.operatingClass = 81;
        clBody.channelNumber = 1;
        clBody.randomizationInterval = 50;
        clBody.measurementDuration = 300;
        elem3.SetBody(clBody);
        hdr.AddMeasurementRequestElement(elem3);

        uint32_t expectedSize =
            3 + elem1.GetSerializedSize() + elem2.GetSerializedSize() + elem3.GetSerializedSize();
        NS_TEST_EXPECT_MSG_EQ(hdr.GetSerializedSize(), expectedSize, "Size with 3 elements");
        TestHeaderSerialization(hdr);

        Buffer buf;
        buf.AddAtStart(hdr.GetSerializedSize());
        hdr.Serialize(buf.Begin());

        RadioMeasurementRequestHeader deserialized;
        deserialized.Deserialize(buf.Begin());
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetMeasurementRequestElements().size(),
                              3,
                              "Three elements round-trip");
    }

    // Test 5: Number of Repetitions values -- 0 (once), 65535 (until cancelled), arbitrary
    {
        for (uint16_t reps : {uint16_t{0}, uint16_t{1}, uint16_t{100}, uint16_t{65535}})
        {
            RadioMeasurementRequestHeader hdr;
            hdr.SetDialogToken(1);
            hdr.SetNumberOfRepetitions(reps);
            TestHeaderSerialization(hdr);

            Buffer buf;
            buf.AddAtStart(hdr.GetSerializedSize());
            hdr.Serialize(buf.Begin());

            RadioMeasurementRequestHeader deserialized;
            deserialized.Deserialize(buf.Begin());
            NS_TEST_EXPECT_MSG_EQ(deserialized.GetNumberOfRepetitions(),
                                  reps,
                                  "Repetitions round-trip for value " << reps);
        }
    }

    // Test 6: Edge cases -- dialog token 255, max repetitions
    {
        RadioMeasurementRequestHeader hdr;
        hdr.SetDialogToken(255);
        hdr.SetNumberOfRepetitions(65535);

        MeasurementRequestElement elem;
        elem.SetMeasurementToken(255);
        elem.SetMeasurementType(0);
        elem.SetBody(MeasurementRequestElement::BasicRequestBody{255, UINT64_MAX, UINT16_MAX});
        hdr.AddMeasurementRequestElement(elem);

        TestHeaderSerialization(hdr);

        Buffer buf;
        buf.AddAtStart(hdr.GetSerializedSize());
        hdr.Serialize(buf.Begin());

        RadioMeasurementRequestHeader deserialized;
        deserialized.Deserialize(buf.Begin());
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetDialogToken(), 255, "Max dialog token");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetNumberOfRepetitions(), 65535, "Max repetitions");
        NS_TEST_ASSERT_MSG_EQ(deserialized.GetMeasurementRequestElements().size(),
                              1,
                              "One element with edge values");
    }
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test suite for IEEE 802.11k Radio Resource Management action frames
 */
class WifiRrmActionFramesTestSuite : public TestSuite
{
  public:
    WifiRrmActionFramesTestSuite();
};

WifiRrmActionFramesTestSuite::WifiRrmActionFramesTestSuite()
    : TestSuite("wifi-rrm-action-frames", Type::UNIT)
{
    AddTestCase(new RadioMeasurementRequestTest, TestCase::Duration::QUICK);
}

static WifiRrmActionFramesTestSuite g_wifiRrmActionFramesTestSuite; ///< the test suite
