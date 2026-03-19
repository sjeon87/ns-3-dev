/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Devansh Gupta <devanshg308@gmail.com>
 */

#include "ns3/header-serialization-test.h"
#include "ns3/log.h"
#include "ns3/measurement-report-element.h"
#include "ns3/measurement-request-element.h"
#include "ns3/radio-measurement-report.h"
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
 * @brief Test serialization and deserialization of RadioMeasurementReportHeader
 * (IEEE 802.11-2024 Section 9.6.6.3, Figure 9-1186)
 */
class RadioMeasurementReportTest : public HeaderSerializationTestCase
{
  public:
    RadioMeasurementReportTest();

  private:
    void DoRun() override;
};

RadioMeasurementReportTest::RadioMeasurementReportTest()
    : HeaderSerializationTestCase(
          "Check serialization and deserialization of RadioMeasurementReportHeader")
{
}

void
RadioMeasurementReportTest::DoRun()
{
    // Test 1: Default construction round-trip
    {
        RadioMeasurementReportHeader hdr;
        TestHeaderSerialization(hdr);
        NS_TEST_EXPECT_MSG_EQ(hdr.GetSerializedSize(), 1, "Default size = 1 (token only)");
        NS_TEST_EXPECT_MSG_EQ(hdr.GetDialogToken(), 0, "Default dialog token");
        NS_TEST_EXPECT_MSG_EQ(hdr.GetMeasurementReportElements().size(),
                              0,
                              "Default has no elements");
    }

    // Test 2: Dialog token 0 (autonomous report) with one element
    {
        RadioMeasurementReportHeader hdr;
        hdr.SetDialogToken(0);

        MeasurementReportElement elem;
        elem.SetMeasurementToken(1);
        elem.SetMeasurementType(MeasurementReportType::BEACON);
        BeaconReport body;
        body.SetOperatingClass(81);
        body.SetChannelNumber(6);
        body.SetActualMeasurementStartTime(1000);
        body.SetMeasurementDuration(200);
        body.SetReportedFrameInformation(0, false);
        body.SetRcpi(100);
        body.SetRsni(50);
        body.SetBssid(Mac48Address("00:11:22:33:44:55"));
        body.SetAntennaId(1);
        body.SetParentTsf(5000);
        elem.SetBeaconReport(body);
        hdr.AddMeasurementReportElement(elem);

        NS_TEST_EXPECT_MSG_EQ(hdr.GetSerializedSize(),
                              1U + elem.GetSerializedSize(),
                              "Size = 1 + element size");
        TestHeaderSerialization(hdr);

        Buffer buf;
        buf.AddAtStart(hdr.GetSerializedSize());
        hdr.Serialize(buf.Begin());

        RadioMeasurementReportHeader deserialized;
        deserialized.Deserialize(buf.Begin());
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetDialogToken(), 0, "Autonomous report token");
        NS_TEST_ASSERT_MSG_EQ(deserialized.GetMeasurementReportElements().size(), 1, "One element");
    }

    // Test 3: Single BeaconReport element round-trip with field verification
    {
        RadioMeasurementReportHeader hdr;
        hdr.SetDialogToken(42);

        MeasurementReportElement elem;
        elem.SetMeasurementToken(7);
        elem.SetMeasurementType(MeasurementReportType::BEACON);
        BeaconReport body;
        body.SetOperatingClass(115);
        body.SetChannelNumber(36);
        body.SetActualMeasurementStartTime(123456789);
        body.SetMeasurementDuration(100);
        body.SetReportedFrameInformation(6, false);
        body.SetRcpi(80);
        body.SetRsni(40);
        body.SetBssid(Mac48Address("aa:bb:cc:dd:ee:ff"));
        body.SetAntennaId(2);
        body.SetParentTsf(99999);
        elem.SetBeaconReport(body);
        hdr.AddMeasurementReportElement(elem);

        TestHeaderSerialization(hdr);

        Buffer buf;
        buf.AddAtStart(hdr.GetSerializedSize());
        hdr.Serialize(buf.Begin());

        RadioMeasurementReportHeader deserialized;
        deserialized.Deserialize(buf.Begin());
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetDialogToken(), 42, "Token round-trip");
        NS_TEST_ASSERT_MSG_EQ(deserialized.GetMeasurementReportElements().size(), 1, "One element");
        const auto& desElem = deserialized.GetMeasurementReportElements()[0];
        NS_TEST_EXPECT_MSG_EQ(desElem.GetMeasurementToken(), 7, "Element token round-trip");
        auto beaconOpt = desElem.GetBeaconReport();
        NS_TEST_ASSERT_MSG_EQ(beaconOpt.has_value(), true, "Beacon report present");
        NS_TEST_EXPECT_MSG_EQ(beaconOpt->GetOperatingClass(), 115, "Operating class round-trip");
        NS_TEST_EXPECT_MSG_EQ(beaconOpt->GetChannelNumber(), 36, "Channel round-trip");
    }

    // Test 4: Multiple elements round-trip
    {
        RadioMeasurementReportHeader hdr;
        hdr.SetDialogToken(7);

        MeasurementReportElement elem1;
        elem1.SetMeasurementToken(1);
        elem1.SetMeasurementType(MeasurementReportType::BEACON);
        BeaconReport body1;
        body1.SetOperatingClass(81);
        body1.SetChannelNumber(1);
        body1.SetBssid(Mac48Address("ff:ff:ff:ff:ff:ff"));
        elem1.SetBeaconReport(body1);
        hdr.AddMeasurementReportElement(elem1);

        MeasurementReportElement elem2;
        elem2.SetMeasurementToken(2);
        elem2.SetMeasurementType(MeasurementReportType::BEACON);
        BeaconReport body2;
        body2.SetOperatingClass(115);
        body2.SetChannelNumber(36);
        body2.SetBssid(Mac48Address("aa:bb:cc:dd:ee:ff"));
        elem2.SetBeaconReport(body2);
        hdr.AddMeasurementReportElement(elem2);

        MeasurementReportElement elem3;
        elem3.SetMeasurementToken(3);
        elem3.SetMeasurementType(MeasurementReportType::BEACON);
        BeaconReport body3;
        body3.SetOperatingClass(124);
        body3.SetChannelNumber(149);
        body3.SetBssid(Mac48Address("11:22:33:44:55:66"));
        elem3.SetBeaconReport(body3);
        hdr.AddMeasurementReportElement(elem3);

        uint32_t expectedSize =
            1 + elem1.GetSerializedSize() + elem2.GetSerializedSize() + elem3.GetSerializedSize();
        NS_TEST_EXPECT_MSG_EQ(hdr.GetSerializedSize(), expectedSize, "Size with 3 elements");
        TestHeaderSerialization(hdr);

        Buffer buf;
        buf.AddAtStart(hdr.GetSerializedSize());
        hdr.Serialize(buf.Begin());

        RadioMeasurementReportHeader deserialized;
        deserialized.Deserialize(buf.Begin());
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetMeasurementReportElements().size(),
                              3,
                              "Three elements round-trip");
    }

    // Test 5: Edge case -- token 255, element with Incapable mode bit set (no body)
    {
        RadioMeasurementReportHeader hdr;
        hdr.SetDialogToken(255);

        MeasurementReportElement elem;
        elem.SetMeasurementToken(255);
        elem.SetIncapable(true);
        elem.SetMeasurementType(MeasurementReportType::BEACON);
        hdr.AddMeasurementReportElement(elem);

        TestHeaderSerialization(hdr);

        Buffer buf;
        buf.AddAtStart(hdr.GetSerializedSize());
        hdr.Serialize(buf.Begin());

        RadioMeasurementReportHeader deserialized;
        deserialized.Deserialize(buf.Begin());
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetDialogToken(), 255, "Max dialog token");
        NS_TEST_ASSERT_MSG_EQ(deserialized.GetMeasurementReportElements().size(),
                              1,
                              "One element with mode bits");
        const auto& desElem = deserialized.GetMeasurementReportElements()[0];
        NS_TEST_EXPECT_MSG_EQ(desElem.GetIncapable(), true, "Incapable bit round-trip");
        NS_TEST_EXPECT_MSG_EQ(desElem.GetBeaconReport().has_value(),
                              false,
                              "No body when Incapable");
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
    AddTestCase(new RadioMeasurementReportTest, TestCase::Duration::QUICK);
}

static WifiRrmActionFramesTestSuite g_wifiRrmActionFramesTestSuite; ///< the test suite
