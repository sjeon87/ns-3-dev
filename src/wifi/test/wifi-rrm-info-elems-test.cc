/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Devansh Gupta <devanshg308@gmail.com>
 */

#include "ns3/header-serialization-test.h"
#include "ns3/link-measurement.h"
#include "ns3/log.h"
#include "ns3/neighbor-report-element.h"

#include <array>
#include <vector>

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
 * @brief Test optional subelements of the Neighbor Report element
 * (IEEE 802.11-2020 Table 9-150)
 */
class NeighborReportSubelementsTest : public HeaderSerializationTestCase
{
  public:
    NeighborReportSubelementsTest();

  private:
    void DoRun() override;
};

NeighborReportSubelementsTest::NeighborReportSubelementsTest()
    : HeaderSerializationTestCase(
          "Check serialization and deserialization of Neighbor Report subelements")
{
}

void
NeighborReportSubelementsTest::DoRun()
{
    // Test 1: TSF Information subelement round-trip
    {
        NeighborReportElement nre;
        nre.SetBssid(Mac48Address("00:11:22:33:44:55"));
        nre.SetBssidInfo(0x0000001F);
        nre.SetOperatingClass(81);
        nre.SetChannelNumber(6);
        nre.SetPhyType(7);
        nre.SetTsfInformation(1000, 100);

        TestHeaderSerialization(nre);

        auto tsf = nre.GetTsfInformation();
        NS_TEST_ASSERT_MSG_EQ(tsf.has_value(), true, "TSF Information should be present");
        NS_TEST_ASSERT_MSG_EQ(tsf->tsfOffset, 1000, "TSF offset");
        NS_TEST_ASSERT_MSG_EQ(tsf->beaconInterval, 100, "Beacon interval");

        // Other subelements should be empty
        NS_TEST_ASSERT_MSG_EQ(nre.GetCondensedCountryString().has_value(),
                              false,
                              "Country string absent");
        NS_TEST_ASSERT_MSG_EQ(nre.GetCandidatePreference().has_value(),
                              false,
                              "Candidate pref absent");
        NS_TEST_ASSERT_MSG_EQ(nre.GetBssTerminationDuration().has_value(),
                              false,
                              "BSS termination absent");
        NS_TEST_ASSERT_MSG_EQ(nre.GetVendorSpecificData().has_value(),
                              false,
                              "Vendor specific absent");
    }

    // Test 2: Condensed Country String subelement round-trip
    {
        NeighborReportElement nre;
        nre.SetBssid(Mac48Address("00:11:22:33:44:55"));
        nre.SetOperatingClass(81);
        nre.SetChannelNumber(6);
        nre.SetPhyType(7);
        nre.SetCondensedCountryString('U', 'S');

        TestHeaderSerialization(nre);

        auto cc = nre.GetCondensedCountryString();
        NS_TEST_ASSERT_MSG_EQ(cc.has_value(), true, "Country string should be present");
        NS_TEST_ASSERT_MSG_EQ((*cc)[0], 'U', "Country char 1");
        NS_TEST_ASSERT_MSG_EQ((*cc)[1], 'S', "Country char 2");
    }

    // Test 3: BSS Transition Candidate Preference subelement round-trip
    {
        NeighborReportElement nre;
        nre.SetBssid(Mac48Address("00:11:22:33:44:55"));
        nre.SetOperatingClass(81);
        nre.SetChannelNumber(6);
        nre.SetPhyType(7);
        nre.SetCandidatePreference(200);

        TestHeaderSerialization(nre);

        auto pref = nre.GetCandidatePreference();
        NS_TEST_ASSERT_MSG_EQ(pref.has_value(), true, "Candidate pref should be present");
        NS_TEST_ASSERT_MSG_EQ(*pref, 200, "Candidate preference value");
    }

    // Test 4: BSS Termination Duration subelement round-trip
    {
        NeighborReportElement nre;
        nre.SetBssid(Mac48Address("00:11:22:33:44:55"));
        nre.SetOperatingClass(81);
        nre.SetChannelNumber(6);
        nre.SetPhyType(7);
        nre.SetBssTerminationDuration(0x0102030405060708ULL, 3600);

        TestHeaderSerialization(nre);

        auto term = nre.GetBssTerminationDuration();
        NS_TEST_ASSERT_MSG_EQ(term.has_value(), true, "BSS termination should be present");
        NS_TEST_ASSERT_MSG_EQ(term->terminationTsf, 0x0102030405060708ULL, "Termination TSF");
        NS_TEST_ASSERT_MSG_EQ(term->duration, 3600, "Duration");
    }

    // Test 5: Vendor Specific subelement round-trip
    {
        NeighborReportElement nre;
        nre.SetBssid(Mac48Address("00:11:22:33:44:55"));
        nre.SetOperatingClass(81);
        nre.SetChannelNumber(6);
        nre.SetPhyType(7);
        nre.SetVendorSpecificData({0xAA, 0xBB, 0xCC, 0xDD});

        TestHeaderSerialization(nre);

        auto vs = nre.GetVendorSpecificData();
        NS_TEST_ASSERT_MSG_EQ(vs.has_value(), true, "Vendor specific should be present");
        NS_TEST_ASSERT_MSG_EQ(vs->size(), 4, "Vendor specific size");
        NS_TEST_ASSERT_MSG_EQ((*vs)[0], 0xAA, "Vendor byte 0");
        NS_TEST_ASSERT_MSG_EQ((*vs)[3], 0xDD, "Vendor byte 3");
    }

    // Test 6: All subelements together
    {
        NeighborReportElement nre;
        nre.SetBssid(Mac48Address("aa:bb:cc:dd:ee:ff"));
        nre.SetBssidInfo(0x00000FFF);
        nre.SetOperatingClass(115);
        nre.SetChannelNumber(36);
        nre.SetPhyType(8);
        nre.SetTsfInformation(500, 200);
        nre.SetCondensedCountryString('G', 'B');
        nre.SetCandidatePreference(150);
        nre.SetBssTerminationDuration(999999, 120);
        nre.SetVendorSpecificData({0x01, 0x02});

        TestHeaderSerialization(nre);

        // Verify all survive round-trip
        Buffer buf;
        buf.AddAtStart(nre.GetSerializedSize());
        nre.Serialize(buf.Begin());

        NeighborReportElement deserialized;
        deserialized.Deserialize(buf.Begin());

        auto tsf = deserialized.GetTsfInformation();
        NS_TEST_ASSERT_MSG_EQ(tsf.has_value(), true, "TSF survives all-together");
        NS_TEST_ASSERT_MSG_EQ(tsf->tsfOffset, 500, "TSF offset all-together");
        NS_TEST_ASSERT_MSG_EQ(tsf->beaconInterval, 200, "Beacon interval all-together");

        auto cc = deserialized.GetCondensedCountryString();
        NS_TEST_ASSERT_MSG_EQ(cc.has_value(), true, "Country survives all-together");
        NS_TEST_ASSERT_MSG_EQ((*cc)[0], 'G', "Country c1 all-together");
        NS_TEST_ASSERT_MSG_EQ((*cc)[1], 'B', "Country c2 all-together");

        auto pref = deserialized.GetCandidatePreference();
        NS_TEST_ASSERT_MSG_EQ(pref.has_value(), true, "Pref survives all-together");
        NS_TEST_ASSERT_MSG_EQ(*pref, 150, "Pref value all-together");

        auto term = deserialized.GetBssTerminationDuration();
        NS_TEST_ASSERT_MSG_EQ(term.has_value(), true, "Term survives all-together");
        NS_TEST_ASSERT_MSG_EQ(term->terminationTsf, 999999, "Term TSF all-together");
        NS_TEST_ASSERT_MSG_EQ(term->duration, 120, "Term duration all-together");

        auto vs = deserialized.GetVendorSpecificData();
        NS_TEST_ASSERT_MSG_EQ(vs.has_value(), true, "VS survives all-together");
        NS_TEST_ASSERT_MSG_EQ(vs->size(), 2, "VS size all-together");
    }

    // Test 7: No subelements (fixed-only) still works and size is 13
    {
        NeighborReportElement nre;
        nre.SetBssid(Mac48Address("00:11:22:33:44:55"));
        nre.SetOperatingClass(81);
        nre.SetChannelNumber(6);
        nre.SetPhyType(7);

        // Element ID (1) + Length (1) + 13 fixed = 15 total serialized size
        NS_TEST_ASSERT_MSG_EQ(nre.GetSerializedSize(), 15, "Fixed-only serialized size");
        TestHeaderSerialization(nre);
    }

    // Test 8: Unknown subelement is silently skipped during deserialization
    {
        NeighborReportElement nre;
        nre.SetBssid(Mac48Address("00:11:22:33:44:55"));
        nre.SetBssidInfo(0x0000001F);
        nre.SetOperatingClass(81);
        nre.SetChannelNumber(6);
        nre.SetPhyType(7);
        nre.SetCandidatePreference(42);

        // Serialize to get a valid buffer
        Buffer buf;
        buf.AddAtStart(nre.GetSerializedSize());
        nre.Serialize(buf.Begin());

        // Build a new buffer with an unknown subelement (ID=99) injected before
        // the candidate preference subelement
        // Original layout: ElemID(1) + Length(1) + BSSID(6) + BSSIDInfo(4) + OpClass(1) +
        //                  Channel(1) + PhyType(1) + [SubelID=3(1) + Len=1(1) + Pref(1)]
        // We'll rebuild: fixed fields + unknown(ID=99, len=3, data={0xDE,0xAD,0xBE}) +
        //                candidate pref subelement
        uint16_t fixedFieldsSize = 13;
        uint16_t unknownSubelemSize = 2 + 3;  // ID + Len + 3 bytes data
        uint16_t candPrefSubelemSize = 2 + 1; // ID + Len + 1 byte
        uint16_t totalInfoFieldSize = fixedFieldsSize + unknownSubelemSize + candPrefSubelemSize;
        uint16_t totalSize = 2 + totalInfoFieldSize; // ElemID + Length + info field

        Buffer manualBuf;
        manualBuf.AddAtStart(totalSize);
        Buffer::Iterator it = manualBuf.Begin();

        // Element header
        it.WriteU8(52); // IE_NEIGHBOR_REPORT = 52
        it.WriteU8(static_cast<uint8_t>(totalInfoFieldSize));

        // Fixed fields (copy from original)
        Buffer::Iterator orig = buf.Begin();
        orig.Next(2); // skip element header
        for (uint16_t j = 0; j < fixedFieldsSize; j++)
        {
            it.WriteU8(orig.ReadU8());
        }

        // Unknown subelement
        it.WriteU8(99); // unknown ID
        it.WriteU8(3);  // length
        it.WriteU8(0xDE);
        it.WriteU8(0xAD);
        it.WriteU8(0xBE);

        // Candidate preference subelement
        it.WriteU8(3);  // ID
        it.WriteU8(1);  // length
        it.WriteU8(42); // preference value

        // Deserialize
        NeighborReportElement deserialized;
        deserialized.Deserialize(manualBuf.Begin());

        NS_TEST_ASSERT_MSG_EQ(deserialized.GetBssid(),
                              Mac48Address("00:11:22:33:44:55"),
                              "BSSID after unknown skip");
        NS_TEST_ASSERT_MSG_EQ(deserialized.GetBssidInfo(),
                              0x0000001F,
                              "BSSIDInfo after unknown skip");
        auto pref = deserialized.GetCandidatePreference();
        NS_TEST_ASSERT_MSG_EQ(pref.has_value(), true, "Candidate pref parsed after unknown skip");
        NS_TEST_ASSERT_MSG_EQ(*pref, 42, "Candidate pref value after unknown skip");
    }

    // Test 9: Edge cases
    {
        // Max TSF offset
        NeighborReportElement nre;
        nre.SetBssid(Mac48Address("00:11:22:33:44:55"));
        nre.SetOperatingClass(81);
        nre.SetChannelNumber(6);
        nre.SetPhyType(7);
        nre.SetTsfInformation(0xFFFF, 0xFFFF);
        TestHeaderSerialization(nre);

        auto tsf = nre.GetTsfInformation();
        NS_TEST_ASSERT_MSG_EQ(tsf->tsfOffset, 0xFFFF, "Max TSF offset");
        NS_TEST_ASSERT_MSG_EQ(tsf->beaconInterval, 0xFFFF, "Max beacon interval");

        // Max candidate preference
        NeighborReportElement nre2;
        nre2.SetBssid(Mac48Address("00:11:22:33:44:55"));
        nre2.SetOperatingClass(81);
        nre2.SetChannelNumber(6);
        nre2.SetPhyType(7);
        nre2.SetCandidatePreference(255);
        TestHeaderSerialization(nre2);
        NS_TEST_ASSERT_MSG_EQ(*nre2.GetCandidatePreference(), 255, "Max candidate preference");

        // Empty vendor specific data
        NeighborReportElement nre3;
        nre3.SetBssid(Mac48Address("00:11:22:33:44:55"));
        nre3.SetOperatingClass(81);
        nre3.SetChannelNumber(6);
        nre3.SetPhyType(7);
        nre3.SetVendorSpecificData({});
        TestHeaderSerialization(nre3);
        NS_TEST_ASSERT_MSG_EQ(nre3.GetVendorSpecificData()->empty(), true, "Empty vendor specific");

        // Large vendor specific data
        NeighborReportElement nre4;
        nre4.SetBssid(Mac48Address("00:11:22:33:44:55"));
        nre4.SetOperatingClass(81);
        nre4.SetChannelNumber(6);
        nre4.SetPhyType(7);
        std::vector<uint8_t> largeData(200, 0x42);
        nre4.SetVendorSpecificData(largeData);
        TestHeaderSerialization(nre4);
        NS_TEST_ASSERT_MSG_EQ(nre4.GetVendorSpecificData()->size(),
                              200,
                              "Large vendor specific size");
    }
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test serialization and deserialization of the Link Measurement Request header
 * (IEEE 802.11-2024 Section 9.6.6.4)
 */
class LinkMeasurementRequestTest : public HeaderSerializationTestCase
{
  public:
    LinkMeasurementRequestTest();

  private:
    void DoRun() override;
};

LinkMeasurementRequestTest::LinkMeasurementRequestTest()
    : HeaderSerializationTestCase(
          "Check serialization and deserialization of Link Measurement Request header")
{
}

void
LinkMeasurementRequestTest::DoRun()
{
    // Test 1: Basic round-trip
    {
        LinkMeasurementRequestHeader hdr;
        hdr.SetDialogToken(1);
        hdr.SetTransmitPowerUsed(20);
        hdr.SetMaxTransmitPower(23);

        TestHeaderSerialization(hdr);

        Buffer buf;
        buf.AddAtStart(hdr.GetSerializedSize());
        hdr.Serialize(buf.Begin());

        LinkMeasurementRequestHeader deserialized;
        deserialized.Deserialize(buf.Begin());

        NS_TEST_EXPECT_MSG_EQ(deserialized.GetDialogToken(), 1, "Dialog token round-trip");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetTransmitPowerUsed(), 20, "Tx power round-trip");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetMaxTransmitPower(), 23, "Max tx power round-trip");
    }

    // Test 2: Negative power values
    {
        LinkMeasurementRequestHeader hdr;
        hdr.SetDialogToken(42);
        hdr.SetTransmitPowerUsed(-10);
        hdr.SetMaxTransmitPower(-127);

        TestHeaderSerialization(hdr);

        Buffer buf;
        buf.AddAtStart(hdr.GetSerializedSize());
        hdr.Serialize(buf.Begin());

        LinkMeasurementRequestHeader deserialized;
        deserialized.Deserialize(buf.Begin());

        NS_TEST_EXPECT_MSG_EQ(deserialized.GetTransmitPowerUsed(),
                              -10,
                              "Negative tx power round-trip");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetMaxTransmitPower(),
                              -127,
                              "Negative max tx power round-trip");
    }

    // Test 3: Edge cases
    {
        LinkMeasurementRequestHeader hdr;
        hdr.SetDialogToken(255);
        hdr.SetTransmitPowerUsed(127);
        hdr.SetMaxTransmitPower(-128);

        TestHeaderSerialization(hdr);

        Buffer buf;
        buf.AddAtStart(hdr.GetSerializedSize());
        hdr.Serialize(buf.Begin());

        LinkMeasurementRequestHeader deserialized;
        deserialized.Deserialize(buf.Begin());

        NS_TEST_EXPECT_MSG_EQ(deserialized.GetDialogToken(), 255, "Max dialog token");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetTransmitPowerUsed(), 127, "Max tx power");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetMaxTransmitPower(), -128, "Min max tx power");
    }

    // Test 4: Zero max transmit power
    {
        LinkMeasurementRequestHeader hdr;
        hdr.SetDialogToken(100);
        hdr.SetTransmitPowerUsed(5);
        hdr.SetMaxTransmitPower(0);

        TestHeaderSerialization(hdr);

        Buffer buf;
        buf.AddAtStart(hdr.GetSerializedSize());
        hdr.Serialize(buf.Begin());

        LinkMeasurementRequestHeader deserialized;
        deserialized.Deserialize(buf.Begin());

        NS_TEST_EXPECT_MSG_EQ(deserialized.GetMaxTransmitPower(), 0, "Zero max tx power");
    }

    // Test 5: Default construction round-trip
    {
        LinkMeasurementRequestHeader hdr;
        TestHeaderSerialization(hdr);
    }

    // Test 6: GetSerializedSize returns 3
    {
        LinkMeasurementRequestHeader hdr;
        NS_TEST_EXPECT_MSG_EQ(hdr.GetSerializedSize(), 3, "Serialized size is 3 bytes");
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
    AddTestCase(new NeighborReportSubelementsTest, TestCase::Duration::QUICK);
    AddTestCase(new LinkMeasurementRequestTest, TestCase::Duration::QUICK);
}

static WifiRrmInfoElemsTestSuite g_wifiRrmInfoElemsTestSuite; ///< the test suite
