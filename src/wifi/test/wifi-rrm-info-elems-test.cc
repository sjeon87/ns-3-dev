/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Devansh Gupta <devanshg308@gmail.com>
 */

#include "ns3/header-serialization-test.h"
#include "ns3/log.h"
#include "ns3/neighbor-report-element.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiRrmInfoElemsTest");

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test serialization and deserialization of the Neighbor Report element
 * (IEEE 802.11-2020 Section 9.4.2.37)
 */
class NeighborReportElementTest : public HeaderSerializationTestCase
{
  public:
    NeighborReportElementTest();

  private:
    void DoRun() override;
};

NeighborReportElementTest::NeighborReportElementTest()
    : HeaderSerializationTestCase(
          "Check serialization and deserialization of Neighbor Report elements")
{
}

void
NeighborReportElementTest::DoRun()
{
    NeighborReportElement nre;
    nre.SetBssid(Mac48Address("00:11:22:33:44:55"));
    nre.SetBssidInfo(0x0000001F);
    nre.SetOperatingClass(81);
    nre.SetChannelNumber(6);
    nre.SetPhyType(7);

    TestHeaderSerialization(nre);

    NeighborReportElement nre2;
    nre2.SetBssid(Mac48Address("aa:bb:cc:dd:ee:ff"));
    nre2.SetBssidInfo(0x00000003);
    nre2.SetOperatingClass(115);
    nre2.SetChannelNumber(36);
    nre2.SetPhyType(8);

    TestHeaderSerialization(nre2);

    TestHeaderSerialization(NeighborReportElement{});
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test BSSID Information bit-field accessors of the Neighbor Report element
 * (IEEE 802.11-2020 Figure 9-331)
 */
class BssidInfoFieldTest : public TestCase
{
  public:
    BssidInfoFieldTest();

  private:
    void DoRun() override;
};

BssidInfoFieldTest::BssidInfoFieldTest()
    : TestCase("Check BSSID Information bit-field accessors")
{
}

void
BssidInfoFieldTest::DoRun()
{
    // Test 1: Individual setters produce correct raw value
    {
        NeighborReportElement nre;

        nre.SetApReachability(3);
        NS_TEST_EXPECT_MSG_EQ(nre.GetBssidInfo(), 0x3, "AP Reachability = 3");

        nre.SetBssidInfo(0);
        nre.SetApReachability(1);
        NS_TEST_EXPECT_MSG_EQ(nre.GetBssidInfo(), 0x1, "AP Reachability = 1");

        nre.SetBssidInfo(0);
        nre.SetSecurity(true);
        NS_TEST_EXPECT_MSG_EQ(nre.GetBssidInfo(), (1 << 2), "Security bit");

        nre.SetBssidInfo(0);
        nre.SetKeyScope(true);
        NS_TEST_EXPECT_MSG_EQ(nre.GetBssidInfo(), (1 << 3), "Key Scope bit");

        nre.SetBssidInfo(0);
        nre.SetSpectrumManagement(true);
        NS_TEST_EXPECT_MSG_EQ(nre.GetBssidInfo(), (1 << 4), "Spectrum Management bit");

        nre.SetBssidInfo(0);
        nre.SetQos(true);
        NS_TEST_EXPECT_MSG_EQ(nre.GetBssidInfo(), (1 << 5), "QoS bit");

        nre.SetBssidInfo(0);
        nre.SetApsd(true);
        NS_TEST_EXPECT_MSG_EQ(nre.GetBssidInfo(), (1 << 6), "APSD bit");

        nre.SetBssidInfo(0);
        nre.SetRadioMeasurement(true);
        NS_TEST_EXPECT_MSG_EQ(nre.GetBssidInfo(), (1 << 7), "Radio Measurement bit");

        nre.SetBssidInfo(0);
        nre.SetDelayedBlockAck(true);
        NS_TEST_EXPECT_MSG_EQ(nre.GetBssidInfo(), (1 << 8), "Delayed Block Ack bit");

        nre.SetBssidInfo(0);
        nre.SetImmediateBlockAck(true);
        NS_TEST_EXPECT_MSG_EQ(nre.GetBssidInfo(), (1 << 9), "Immediate Block Ack bit");

        nre.SetBssidInfo(0);
        nre.SetMobilityDomain(true);
        NS_TEST_EXPECT_MSG_EQ(nre.GetBssidInfo(), (1 << 10), "Mobility Domain bit");

        nre.SetBssidInfo(0);
        nre.SetHighThroughput(true);
        NS_TEST_EXPECT_MSG_EQ(nre.GetBssidInfo(), (1 << 11), "High Throughput bit");
    }

    // Test 2: Raw value to individual getters
    {
        NeighborReportElement nre;
        nre.SetBssidInfo(0x00000FFF); // bits 0-11 all set

        NS_TEST_EXPECT_MSG_EQ(nre.GetApReachability(), 3, "AP Reachability from raw");
        NS_TEST_EXPECT_MSG_EQ(nre.GetSecurity(), true, "Security from raw");
        NS_TEST_EXPECT_MSG_EQ(nre.GetKeyScope(), true, "Key Scope from raw");
        NS_TEST_EXPECT_MSG_EQ(nre.GetSpectrumManagement(), true, "Spectrum Management from raw");
        NS_TEST_EXPECT_MSG_EQ(nre.GetQos(), true, "QoS from raw");
        NS_TEST_EXPECT_MSG_EQ(nre.GetApsd(), true, "APSD from raw");
        NS_TEST_EXPECT_MSG_EQ(nre.GetRadioMeasurement(), true, "Radio Measurement from raw");
        NS_TEST_EXPECT_MSG_EQ(nre.GetDelayedBlockAck(), true, "Delayed Block Ack from raw");
        NS_TEST_EXPECT_MSG_EQ(nre.GetImmediateBlockAck(), true, "Immediate Block Ack from raw");
        NS_TEST_EXPECT_MSG_EQ(nre.GetMobilityDomain(), true, "Mobility Domain from raw");
        NS_TEST_EXPECT_MSG_EQ(nre.GetHighThroughput(), true, "High Throughput from raw");
    }

    // Test 3: AP Reachability 2-bit edge cases
    {
        NeighborReportElement nre;
        for (uint8_t val = 0; val <= 3; val++)
        {
            nre.SetApReachability(val);
            NS_TEST_EXPECT_MSG_EQ(nre.GetApReachability(),
                                  val,
                                  "AP Reachability round-trip " << +val);
        }
    }

    // Test 4: Clearing boolean fields
    {
        NeighborReportElement nre;
        nre.SetBssidInfo(0x00000FFF);

        nre.SetSecurity(false);
        NS_TEST_EXPECT_MSG_EQ(nre.GetSecurity(), false, "Security cleared");
        NS_TEST_EXPECT_MSG_EQ(nre.GetKeyScope(), true, "Key Scope unaffected");

        nre.SetHighThroughput(false);
        NS_TEST_EXPECT_MSG_EQ(nre.GetHighThroughput(), false, "HT cleared");
        NS_TEST_EXPECT_MSG_EQ(nre.GetMobilityDomain(), true, "Mobility Domain unaffected");
    }

    // Test 5: Setting fields preserves reserved bits
    {
        NeighborReportElement nre;
        nre.SetBssidInfo(0xFFFFF000); // reserved bits set
        nre.SetSecurity(true);
        NS_TEST_EXPECT_MSG_EQ(nre.GetBssidInfo(),
                              (0xFFFFF000 | (1 << 2)),
                              "Reserved bits preserved");
    }

    // Test 6: Serialize/deserialize round-trip with accessor-set fields
    {
        NeighborReportElement nre;
        nre.SetBssid(Mac48Address("00:11:22:33:44:55"));
        nre.SetApReachability(2);
        nre.SetSecurity(true);
        nre.SetQos(true);
        nre.SetRadioMeasurement(true);
        nre.SetImmediateBlockAck(true);
        nre.SetHighThroughput(true);
        nre.SetOperatingClass(81);
        nre.SetChannelNumber(6);
        nre.SetPhyType(7);

        // Serialize
        Buffer buf;
        buf.AddAtStart(nre.GetSerializedSize());
        Buffer::Iterator iter = buf.Begin();
        nre.Serialize(iter);

        // Deserialize
        NeighborReportElement deserialized;
        iter = buf.Begin();
        deserialized.Deserialize(iter);

        NS_TEST_EXPECT_MSG_EQ(deserialized.GetApReachability(),
                              2,
                              "AP Reachability survives serde");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetSecurity(), true, "Security survives serde");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetQos(), true, "QoS survives serde");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetRadioMeasurement(),
                              true,
                              "Radio Meas survives serde");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetImmediateBlockAck(), true, "Imm BA survives serde");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetHighThroughput(), true, "HT survives serde");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetKeyScope(), false, "Key Scope false survives serde");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetDelayedBlockAck(),
                              false,
                              "Delayed BA false survives serde");
    }
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test suite for IEEE 802.11k Radio Resource Management information elements
 */
class WifiRrmInfoElemsTestSuite : public TestSuite
{
  public:
    WifiRrmInfoElemsTestSuite();
};

WifiRrmInfoElemsTestSuite::WifiRrmInfoElemsTestSuite()
    : TestSuite("wifi-rrm-info-elems", Type::UNIT)
{
    AddTestCase(new NeighborReportElementTest, TestCase::Duration::QUICK);
    AddTestCase(new BssidInfoFieldTest, TestCase::Duration::QUICK);
}

static WifiRrmInfoElemsTestSuite g_wifiRrmInfoElemsTestSuite; ///< the test suite
