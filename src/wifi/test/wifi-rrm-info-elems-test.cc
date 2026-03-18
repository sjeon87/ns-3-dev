/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Devansh Gupta <devanshg308@gmail.com>
 */

#include "ns3/header-serialization-test.h"
#include "ns3/ht-capabilities.h"
#include "ns3/ht-operation.h"
#include "ns3/link-measurement.h"
#include "ns3/log.h"
#include "ns3/neighbor-report-element.h"
#include "ns3/tpc-report-element.h"
#include "ns3/vht-capabilities.h"
#include "ns3/vht-operation.h"

#include <array>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiRrmInfoElemsTest");

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test serialization and deserialization of the Neighbor Report element
 * (IEEE 802.11-2024 Section 9.4.2.35)
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
 * (IEEE 802.11-2024 Figure 9-417)
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
        nre.SetMobilityDomain(true);
        NS_TEST_EXPECT_MSG_EQ(nre.GetBssidInfo(), (1 << 10), "Mobility Domain bit");

        nre.SetBssidInfo(0);
        nre.SetHighThroughput(true);
        NS_TEST_EXPECT_MSG_EQ(nre.GetBssidInfo(), (1 << 11), "High Throughput bit");

        nre.SetBssidInfo(0);
        nre.SetVeryHighThroughput(true);
        NS_TEST_EXPECT_MSG_EQ(nre.GetBssidInfo(), (1 << 12), "Very High Throughput bit");

        nre.SetBssidInfo(0);
        nre.SetFtm(true);
        NS_TEST_EXPECT_MSG_EQ(nre.GetBssidInfo(), (1 << 13), "FTM bit");

        nre.SetBssidInfo(0);
        nre.SetHighEfficiency(true);
        NS_TEST_EXPECT_MSG_EQ(nre.GetBssidInfo(), (1 << 14), "High Efficiency bit");

        nre.SetBssidInfo(0);
        nre.SetErBss(true);
        NS_TEST_EXPECT_MSG_EQ(nre.GetBssidInfo(), (1 << 15), "ER BSS bit");

        nre.SetBssidInfo(0);
        nre.SetColocatedAp(true);
        NS_TEST_EXPECT_MSG_EQ(nre.GetBssidInfo(), (1 << 16), "Colocated AP bit");

        nre.SetBssidInfo(0);
        nre.SetUnsolicitedProbeResponsesActive(true);
        NS_TEST_EXPECT_MSG_EQ(nre.GetBssidInfo(),
                              (1 << 17),
                              "Unsolicited Probe Responses Active bit");

        nre.SetBssidInfo(0);
        nre.SetMemberOfEssWith2gOr5gColocatedAp(true);
        NS_TEST_EXPECT_MSG_EQ(nre.GetBssidInfo(),
                              (1 << 18),
                              "Member of ESS with 2.4/5 GHz Colocated AP bit");

        nre.SetBssidInfo(0);
        nre.SetOctSupportedWithReportingAp(true);
        NS_TEST_EXPECT_MSG_EQ(nre.GetBssidInfo(), (1 << 19), "OCT Supported bit");

        nre.SetBssidInfo(0);
        nre.SetColocatedWith6gAp(true);
        NS_TEST_EXPECT_MSG_EQ(nre.GetBssidInfo(), (1 << 20), "Colocated with 6 GHz AP bit");

        nre.SetBssidInfo(0);
        nre.SetDmgPositioning(true);
        NS_TEST_EXPECT_MSG_EQ(nre.GetBssidInfo(), (1 << 22), "DMG Positioning bit");
    }

    // Test 2: Raw value to individual getters
    {
        NeighborReportElement nre;
        // Bits 0-7, 10-20, 22 all set (skip reserved B8-B9, B21)
        nre.SetBssidInfo(0x005FFCFF);

        NS_TEST_EXPECT_MSG_EQ(nre.GetApReachability(), 3, "AP Reachability from raw");
        NS_TEST_EXPECT_MSG_EQ(nre.GetSecurity(), true, "Security from raw");
        NS_TEST_EXPECT_MSG_EQ(nre.GetKeyScope(), true, "Key Scope from raw");
        NS_TEST_EXPECT_MSG_EQ(nre.GetSpectrumManagement(), true, "Spectrum Management from raw");
        NS_TEST_EXPECT_MSG_EQ(nre.GetQos(), true, "QoS from raw");
        NS_TEST_EXPECT_MSG_EQ(nre.GetApsd(), true, "APSD from raw");
        NS_TEST_EXPECT_MSG_EQ(nre.GetRadioMeasurement(), true, "Radio Measurement from raw");
        NS_TEST_EXPECT_MSG_EQ(nre.GetMobilityDomain(), true, "Mobility Domain from raw");
        NS_TEST_EXPECT_MSG_EQ(nre.GetHighThroughput(), true, "High Throughput from raw");
        NS_TEST_EXPECT_MSG_EQ(nre.GetVeryHighThroughput(), true, "VHT from raw");
        NS_TEST_EXPECT_MSG_EQ(nre.GetFtm(), true, "FTM from raw");
        NS_TEST_EXPECT_MSG_EQ(nre.GetHighEfficiency(), true, "HE from raw");
        NS_TEST_EXPECT_MSG_EQ(nre.GetErBss(), true, "ER BSS from raw");
        NS_TEST_EXPECT_MSG_EQ(nre.GetColocatedAp(), true, "Colocated AP from raw");
        NS_TEST_EXPECT_MSG_EQ(nre.GetUnsolicitedProbeResponsesActive(),
                              true,
                              "Unsolicited Probe Responses from raw");
        NS_TEST_EXPECT_MSG_EQ(nre.GetMemberOfEssWith2gOr5gColocatedAp(),
                              true,
                              "Member of ESS from raw");
        NS_TEST_EXPECT_MSG_EQ(nre.GetOctSupportedWithReportingAp(), true, "OCT Supported from raw");
        NS_TEST_EXPECT_MSG_EQ(nre.GetColocatedWith6gAp(), true, "Colocated 6 GHz from raw");
        NS_TEST_EXPECT_MSG_EQ(nre.GetDmgPositioning(), true, "DMG Positioning from raw");
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
        nre.SetBssidInfo(0x005FFCFF);

        nre.SetSecurity(false);
        NS_TEST_EXPECT_MSG_EQ(nre.GetSecurity(), false, "Security cleared");
        NS_TEST_EXPECT_MSG_EQ(nre.GetKeyScope(), true, "Key Scope unaffected");

        nre.SetVeryHighThroughput(false);
        NS_TEST_EXPECT_MSG_EQ(nre.GetVeryHighThroughput(), false, "VHT cleared");
        NS_TEST_EXPECT_MSG_EQ(nre.GetColocatedAp(), true, "Colocated AP unaffected");
    }

    // Test 5: Setting fields preserves reserved bits (B8-B9, B21, B23-B31)
    {
        NeighborReportElement nre;
        nre.SetBssidInfo(0xFF800300); // reserved bits set
        nre.SetSecurity(true);
        NS_TEST_EXPECT_MSG_EQ(nre.GetBssidInfo(),
                              (0xFF800300 | (1 << 2)),
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
        nre.SetHighThroughput(true);
        nre.SetVeryHighThroughput(true);
        nre.SetFtm(true);
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
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetHighThroughput(), true, "HT survives serde");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetVeryHighThroughput(), true, "VHT survives serde");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetFtm(), true, "FTM survives serde");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetKeyScope(), false, "Key Scope false survives serde");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetHighEfficiency(), false, "HE false survives serde");
    }
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test optional subelements of the Neighbor Report element
 * (IEEE 802.11-2024 Table 9-212)
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

    // Test 9: Bearing subelement round-trip
    {
        NeighborReportElement nre;
        nre.SetBssid(Mac48Address("00:11:22:33:44:55"));
        nre.SetOperatingClass(81);
        nre.SetChannelNumber(6);
        nre.SetPhyType(7);
        nre.SetBearing(180, 0x41200000, -5); // 180 deg, 10.0m (IEEE 754), -5m height

        TestHeaderSerialization(nre);

        auto bearing = nre.GetBearing();
        NS_TEST_ASSERT_MSG_EQ(bearing.has_value(), true, "Bearing should be present");
        NS_TEST_ASSERT_MSG_EQ(bearing->bearing, 180, "Bearing degrees");
        NS_TEST_ASSERT_MSG_EQ(bearing->distance, 0x41200000, "Distance raw float");
        NS_TEST_ASSERT_MSG_EQ(bearing->relativeHeight, -5, "Relative height");
    }

    // Test 10: Bearing edge values
    {
        NeighborReportElement nre;
        nre.SetBssid(Mac48Address("00:11:22:33:44:55"));
        nre.SetOperatingClass(81);
        nre.SetChannelNumber(6);
        nre.SetPhyType(7);
        nre.SetBearing(359, 0xFFFFFFFF, INT16_MAX);

        TestHeaderSerialization(nre);

        auto bearing = nre.GetBearing();
        NS_TEST_ASSERT_MSG_EQ(bearing->bearing, 359, "Max bearing degrees");
        NS_TEST_ASSERT_MSG_EQ(bearing->distance, 0xFFFFFFFF, "Max distance");
        NS_TEST_ASSERT_MSG_EQ(bearing->relativeHeight, INT16_MAX, "Max relative height");
    }

    // Test 11: Bearing with INT16_MIN relative height
    {
        NeighborReportElement nre;
        nre.SetBssid(Mac48Address("00:11:22:33:44:55"));
        nre.SetOperatingClass(81);
        nre.SetChannelNumber(6);
        nre.SetPhyType(7);
        nre.SetBearing(0, 0, INT16_MIN);

        TestHeaderSerialization(nre);

        auto bearing = nre.GetBearing();
        NS_TEST_ASSERT_MSG_EQ(bearing->relativeHeight, INT16_MIN, "Min relative height");
    }

    // Test 12: Bearing with zero relative height
    {
        NeighborReportElement nre;
        nre.SetBssid(Mac48Address("00:11:22:33:44:55"));
        nre.SetOperatingClass(81);
        nre.SetChannelNumber(6);
        nre.SetPhyType(7);
        nre.SetBearing(0, 0, 0);

        TestHeaderSerialization(nre);

        auto bearing = nre.GetBearing();
        NS_TEST_ASSERT_MSG_EQ(bearing->relativeHeight, 0, "Zero relative height");
    }

    // Test 13: Wide Bandwidth Channel subelement round-trip
    {
        NeighborReportElement nre;
        nre.SetBssid(Mac48Address("00:11:22:33:44:55"));
        nre.SetOperatingClass(81);
        nre.SetChannelNumber(6);
        nre.SetPhyType(7);
        nre.SetWideBandwidthChannel(1, 42, 0);

        TestHeaderSerialization(nre);

        auto wbc = nre.GetWideBandwidthChannel();
        NS_TEST_ASSERT_MSG_EQ(wbc.has_value(), true, "WBC should be present");
        NS_TEST_ASSERT_MSG_EQ(wbc->channelWidth, 1, "Channel width");
        NS_TEST_ASSERT_MSG_EQ(wbc->centerFreqSegment0, 42, "Center freq seg 0");
        NS_TEST_ASSERT_MSG_EQ(wbc->centerFreqSegment1, 0, "Center freq seg 1");
    }

    // Test 14: Wide Bandwidth Channel edge values
    {
        NeighborReportElement nre;
        nre.SetBssid(Mac48Address("00:11:22:33:44:55"));
        nre.SetOperatingClass(81);
        nre.SetChannelNumber(6);
        nre.SetPhyType(7);
        nre.SetWideBandwidthChannel(255, 255, 255);

        TestHeaderSerialization(nre);

        auto wbc = nre.GetWideBandwidthChannel();
        NS_TEST_ASSERT_MSG_EQ(wbc->channelWidth, 255, "Max channel width");
        NS_TEST_ASSERT_MSG_EQ(wbc->centerFreqSegment0, 255, "Max center freq seg 0");
        NS_TEST_ASSERT_MSG_EQ(wbc->centerFreqSegment1, 255, "Max center freq seg 1");
    }

    // Test 15: Bearing + Wide Bandwidth Channel together
    {
        NeighborReportElement nre;
        nre.SetBssid(Mac48Address("aa:bb:cc:dd:ee:ff"));
        nre.SetOperatingClass(115);
        nre.SetChannelNumber(36);
        nre.SetPhyType(8);
        nre.SetBearing(90, 0x41A00000, 3);
        nre.SetWideBandwidthChannel(1, 42, 50);

        TestHeaderSerialization(nre);

        auto bearing = nre.GetBearing();
        NS_TEST_ASSERT_MSG_EQ(bearing.has_value(), true, "Bearing present with WBC");
        auto wbc = nre.GetWideBandwidthChannel();
        NS_TEST_ASSERT_MSG_EQ(wbc.has_value(), true, "WBC present with Bearing");
    }

    // Test 16: HT Capabilities subelement round-trip
    {
        NeighborReportElement nre;
        nre.SetBssid(Mac48Address("00:11:22:33:44:55"));
        nre.SetOperatingClass(81);
        nre.SetChannelNumber(6);
        nre.SetPhyType(7);

        HtCapabilities htCap;
        htCap.SetLdpc(1);
        htCap.SetSupportedChannelWidth(1);
        nre.SetHtCapabilities(htCap);

        TestHeaderSerialization(nre);

        auto result = nre.GetHtCapabilities();
        NS_TEST_ASSERT_MSG_EQ(result.has_value(), true, "HT Capabilities should be present");
        NS_TEST_ASSERT_MSG_EQ(result->GetLdpc(), 1, "HT LDPC round-trip");
        NS_TEST_ASSERT_MSG_EQ(result->GetSupportedChannelWidth(), 1, "HT channel width round-trip");
    }

    // Test 17: HT Operation subelement round-trip
    {
        NeighborReportElement nre;
        nre.SetBssid(Mac48Address("00:11:22:33:44:55"));
        nre.SetOperatingClass(81);
        nre.SetChannelNumber(6);
        nre.SetPhyType(7);

        HtOperation htOp;
        htOp.SetPrimaryChannel(6);
        htOp.SetSecondaryChannelOffset(1);
        nre.SetHtOperation(htOp);

        TestHeaderSerialization(nre);

        auto result = nre.GetHtOperation();
        NS_TEST_ASSERT_MSG_EQ(result.has_value(), true, "HT Operation should be present");
        NS_TEST_ASSERT_MSG_EQ(result->GetPrimaryChannel(), 6, "HT primary channel round-trip");
    }

    // Test 18: VHT Capabilities subelement round-trip
    {
        NeighborReportElement nre;
        nre.SetBssid(Mac48Address("00:11:22:33:44:55"));
        nre.SetOperatingClass(81);
        nre.SetChannelNumber(6);
        nre.SetPhyType(7);

        VhtCapabilities vhtCap;
        vhtCap.SetMaxMpduLength(7991);
        vhtCap.SetRxLdpc(1);
        nre.SetVhtCapabilities(vhtCap);

        TestHeaderSerialization(nre);

        auto result = nre.GetVhtCapabilities();
        NS_TEST_ASSERT_MSG_EQ(result.has_value(), true, "VHT Capabilities should be present");
        NS_TEST_ASSERT_MSG_EQ(result->GetRxLdpc(), 1, "VHT LDPC round-trip");
    }

    // Test 19: VHT Operation subelement round-trip
    {
        NeighborReportElement nre;
        nre.SetBssid(Mac48Address("00:11:22:33:44:55"));
        nre.SetOperatingClass(81);
        nre.SetChannelNumber(6);
        nre.SetPhyType(7);

        VhtOperation vhtOp;
        vhtOp.SetChannelWidth(1);
        vhtOp.SetChannelCenterFrequencySegment0(42);
        vhtOp.SetChannelCenterFrequencySegment1(0);
        nre.SetVhtOperation(vhtOp);

        TestHeaderSerialization(nre);

        auto result = nre.GetVhtOperation();
        NS_TEST_ASSERT_MSG_EQ(result.has_value(), true, "VHT Operation should be present");
        NS_TEST_ASSERT_MSG_EQ(result->GetChannelWidth(), 1, "VHT channel width round-trip");
        NS_TEST_ASSERT_MSG_EQ(result->GetChannelCenterFrequencySegment0(),
                              42,
                              "VHT seg0 round-trip");
    }

    // Test 20: All Tier 2 subelements together with Tier 1
    {
        NeighborReportElement nre;
        nre.SetBssid(Mac48Address("aa:bb:cc:dd:ee:ff"));
        nre.SetOperatingClass(115);
        nre.SetChannelNumber(36);
        nre.SetPhyType(8);
        nre.SetTsfInformation(500, 200);
        nre.SetBearing(90, 0x41A00000, 3);
        nre.SetWideBandwidthChannel(1, 42, 50);

        HtCapabilities htCap;
        htCap.SetLdpc(1);
        nre.SetHtCapabilities(htCap);

        HtOperation htOp;
        htOp.SetPrimaryChannel(36);
        nre.SetHtOperation(htOp);

        VhtCapabilities vhtCap;
        vhtCap.SetRxLdpc(1);
        nre.SetVhtCapabilities(vhtCap);

        VhtOperation vhtOp;
        vhtOp.SetChannelWidth(1);
        nre.SetVhtOperation(vhtOp);

        TestHeaderSerialization(nre);

        // Verify all survive round-trip
        Buffer buf;
        buf.AddAtStart(nre.GetSerializedSize());
        nre.Serialize(buf.Begin());

        NeighborReportElement deserialized;
        deserialized.Deserialize(buf.Begin());

        NS_TEST_ASSERT_MSG_EQ(deserialized.GetTsfInformation().has_value(),
                              true,
                              "TSF survives combined");
        NS_TEST_ASSERT_MSG_EQ(deserialized.GetBearing().has_value(),
                              true,
                              "Bearing survives combined");
        NS_TEST_ASSERT_MSG_EQ(deserialized.GetWideBandwidthChannel().has_value(),
                              true,
                              "WBC survives combined");
        NS_TEST_ASSERT_MSG_EQ(deserialized.GetHtCapabilities().has_value(),
                              true,
                              "HT Cap survives combined");
        NS_TEST_ASSERT_MSG_EQ(deserialized.GetHtOperation().has_value(),
                              true,
                              "HT Op survives combined");
        NS_TEST_ASSERT_MSG_EQ(deserialized.GetVhtCapabilities().has_value(),
                              true,
                              "VHT Cap survives combined");
        NS_TEST_ASSERT_MSG_EQ(deserialized.GetVhtOperation().has_value(),
                              true,
                              "VHT Op survives combined");
    }

    // Test 21: HT/VHT absent by default
    {
        NeighborReportElement nre;
        nre.SetBssid(Mac48Address("00:11:22:33:44:55"));
        nre.SetOperatingClass(81);
        nre.SetChannelNumber(6);
        nre.SetPhyType(7);

        NS_TEST_ASSERT_MSG_EQ(nre.GetHtCapabilities().has_value(),
                              false,
                              "HT Cap absent by default");
        NS_TEST_ASSERT_MSG_EQ(nre.GetHtOperation().has_value(), false, "HT Op absent by default");
        NS_TEST_ASSERT_MSG_EQ(nre.GetVhtCapabilities().has_value(),
                              false,
                              "VHT Cap absent by default");
        NS_TEST_ASSERT_MSG_EQ(nre.GetVhtOperation().has_value(), false, "VHT Op absent by default");
    }

    // Test 22: Edge cases
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

    // Test 23: Out-of-order IE-format subelements are deserialized correctly
    {
        // Serialize HT Capabilities and VHT Operation independently to get their raw bytes
        HtCapabilities htCap;
        htCap.SetLdpc(1);
        htCap.SetSupportedChannelWidth(1);

        VhtOperation vhtOp;
        vhtOp.SetChannelWidth(1);
        vhtOp.SetChannelCenterFrequencySegment0(42);

        // Serialize each IE to get raw bytes
        uint16_t htCapSize = htCap.GetSerializedSize(); // 28
        uint16_t vhtOpSize = vhtOp.GetSerializedSize(); // 7

        Buffer htCapBuf;
        htCapBuf.AddAtStart(htCapSize);
        htCap.Serialize(htCapBuf.Begin());

        Buffer vhtOpBuf;
        vhtOpBuf.AddAtStart(vhtOpSize);
        vhtOp.Serialize(vhtOpBuf.Begin());

        // Build manual buffer: fixed fields (13) + VHT Op (7) + HT Cap (28) = 48
        // This is out-of-order: VHT Operation (ID 192) comes before HT Capabilities (ID 45)
        uint16_t fixedFieldsSize = 13;
        uint16_t totalInfoFieldSize = fixedFieldsSize + vhtOpSize + htCapSize;
        uint16_t totalSize = 2 + totalInfoFieldSize; // ElemID + Length + info field

        Buffer manualBuf;
        manualBuf.AddAtStart(totalSize);
        Buffer::Iterator it = manualBuf.Begin();

        // Element header
        it.WriteU8(52); // IE_NEIGHBOR_REPORT
        it.WriteU8(static_cast<uint8_t>(totalInfoFieldSize));

        // Fixed fields: BSSID + BSSIDInfo + OpClass + Channel + PhyType
        // BSSID 00:11:22:33:44:55
        it.WriteU8(0x00);
        it.WriteU8(0x11);
        it.WriteU8(0x22);
        it.WriteU8(0x33);
        it.WriteU8(0x44);
        it.WriteU8(0x55);
        it.WriteU32(0); // BSSIDInfo
        it.WriteU8(81); // Operating class
        it.WriteU8(6);  // Channel
        it.WriteU8(7);  // PhyType

        // VHT Operation first (out of order -- ID 192 before ID 45)
        Buffer::Iterator vhtIt = vhtOpBuf.Begin();
        for (uint16_t j = 0; j < vhtOpSize; j++)
        {
            it.WriteU8(vhtIt.ReadU8());
        }

        // HT Capabilities second
        Buffer::Iterator htIt = htCapBuf.Begin();
        for (uint16_t j = 0; j < htCapSize; j++)
        {
            it.WriteU8(htIt.ReadU8());
        }

        // Deserialize
        NeighborReportElement deserialized;
        deserialized.Deserialize(manualBuf.Begin());

        NS_TEST_ASSERT_MSG_EQ(deserialized.GetBssid(),
                              Mac48Address("00:11:22:33:44:55"),
                              "BSSID after out-of-order IEs");
        auto htResult = deserialized.GetHtCapabilities();
        NS_TEST_ASSERT_MSG_EQ(htResult.has_value(),
                              true,
                              "HT Cap present after out-of-order deser");
        NS_TEST_ASSERT_MSG_EQ(htResult->GetLdpc(), 1, "HT LDPC after out-of-order deser");
        NS_TEST_ASSERT_MSG_EQ(htResult->GetSupportedChannelWidth(),
                              1,
                              "HT channel width after out-of-order deser");
        auto vhtResult = deserialized.GetVhtOperation();
        NS_TEST_ASSERT_MSG_EQ(vhtResult.has_value(),
                              true,
                              "VHT Op present after out-of-order deser");
        NS_TEST_ASSERT_MSG_EQ(vhtResult->GetChannelWidth(),
                              1,
                              "VHT channel width after out-of-order deser");
        NS_TEST_ASSERT_MSG_EQ(vhtResult->GetChannelCenterFrequencySegment0(),
                              42,
                              "VHT seg0 after out-of-order deser");
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
 * @brief Test serialization and deserialization of the TPC Report element
 * (IEEE 802.11-2024 Section 9.4.2.15, IE 35)
 */
class TpcReportElementTest : public HeaderSerializationTestCase
{
  public:
    TpcReportElementTest();

  private:
    void DoRun() override;
};

TpcReportElementTest::TpcReportElementTest()
    : HeaderSerializationTestCase("Check serialization and deserialization of TPC Report element")
{
}

void
TpcReportElementTest::DoRun()
{
    // Test 1: Basic round-trip with positive values
    {
        TpcReportElement elem;
        elem.SetTransmitPower(20);
        elem.SetLinkMargin(10);

        TestHeaderSerialization(elem);

        Buffer buf;
        buf.AddAtStart(elem.GetSerializedSize());
        elem.Serialize(buf.Begin());

        TpcReportElement deserialized;
        deserialized.Deserialize(buf.Begin());

        NS_TEST_EXPECT_MSG_EQ(deserialized.GetTransmitPower(), 20, "Transmit power round-trip");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetLinkMargin(), 10, "Link margin round-trip");
    }

    // Test 2: Negative Transmit Power
    {
        TpcReportElement elem;
        elem.SetTransmitPower(-10);
        elem.SetLinkMargin(5);

        TestHeaderSerialization(elem);

        Buffer buf;
        buf.AddAtStart(elem.GetSerializedSize());
        elem.Serialize(buf.Begin());

        TpcReportElement deserialized;
        deserialized.Deserialize(buf.Begin());

        NS_TEST_EXPECT_MSG_EQ(deserialized.GetTransmitPower(),
                              -10,
                              "Negative transmit power round-trip");
    }

    // Test 3: Negative Link Margin
    {
        TpcReportElement elem;
        elem.SetTransmitPower(15);
        elem.SetLinkMargin(-8);

        TestHeaderSerialization(elem);

        Buffer buf;
        buf.AddAtStart(elem.GetSerializedSize());
        elem.Serialize(buf.Begin());

        TpcReportElement deserialized;
        deserialized.Deserialize(buf.Begin());

        NS_TEST_EXPECT_MSG_EQ(deserialized.GetLinkMargin(), -8, "Negative link margin round-trip");
    }

    // Test 4: INT8_MIN and INT8_MAX edge cases
    {
        TpcReportElement elem;
        elem.SetTransmitPower(INT8_MAX);
        elem.SetLinkMargin(INT8_MIN);

        TestHeaderSerialization(elem);

        Buffer buf;
        buf.AddAtStart(elem.GetSerializedSize());
        elem.Serialize(buf.Begin());

        TpcReportElement deserialized;
        deserialized.Deserialize(buf.Begin());

        NS_TEST_EXPECT_MSG_EQ(deserialized.GetTransmitPower(), INT8_MAX, "INT8_MAX transmit power");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetLinkMargin(), INT8_MIN, "INT8_MIN link margin");
    }

    // Test 5: Both values zero
    {
        TpcReportElement elem;
        elem.SetTransmitPower(0);
        elem.SetLinkMargin(0);

        TestHeaderSerialization(elem);

        Buffer buf;
        buf.AddAtStart(elem.GetSerializedSize());
        elem.Serialize(buf.Begin());

        TpcReportElement deserialized;
        deserialized.Deserialize(buf.Begin());

        NS_TEST_EXPECT_MSG_EQ(deserialized.GetTransmitPower(), 0, "Zero transmit power");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetLinkMargin(), 0, "Zero link margin");
    }

    // Test 6: Default construction round-trip
    {
        TpcReportElement elem;
        TestHeaderSerialization(elem);
    }

    // Test 7: GetSerializedSize returns 4 (IE header 2 + payload 2)
    {
        TpcReportElement elem;
        NS_TEST_EXPECT_MSG_EQ(elem.GetSerializedSize(), 4, "Serialized size is 4 bytes");
    }
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test serialization and deserialization of the Link Measurement Report header
 * (IEEE 802.11-2024 Section 9.6.6.5)
 */
class LinkMeasurementReportTest : public HeaderSerializationTestCase
{
  public:
    LinkMeasurementReportTest();

  private:
    void DoRun() override;
};

LinkMeasurementReportTest::LinkMeasurementReportTest()
    : HeaderSerializationTestCase(
          "Check serialization and deserialization of Link Measurement Report header")
{
}

void
LinkMeasurementReportTest::DoRun()
{
    // Test 1: Basic round-trip
    {
        LinkMeasurementReportHeader hdr;
        hdr.SetDialogToken(1);
        hdr.SetTpcTransmitPower(20);
        hdr.SetTpcLinkMargin(10);
        hdr.SetRxAntennaId(1);
        hdr.SetTxAntennaId(2);
        hdr.SetRcpi(110);
        hdr.SetRsni(50);

        TestHeaderSerialization(hdr);

        Buffer buf;
        buf.AddAtStart(hdr.GetSerializedSize());
        hdr.Serialize(buf.Begin());

        LinkMeasurementReportHeader deserialized;
        deserialized.Deserialize(buf.Begin());

        NS_TEST_EXPECT_MSG_EQ(deserialized.GetDialogToken(), 1, "Dialog token round-trip");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetTpcTransmitPower(), 20, "TPC tx power round-trip");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetTpcLinkMargin(), 10, "TPC link margin round-trip");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetRxAntennaId(), 1, "Rx antenna ID round-trip");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetTxAntennaId(), 2, "Tx antenna ID round-trip");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetRcpi(), 110, "RCPI round-trip");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetRsni(), 50, "RSNI round-trip");
    }

    // Test 2: Negative TPC power values
    {
        LinkMeasurementReportHeader hdr;
        hdr.SetDialogToken(42);
        hdr.SetTpcTransmitPower(-15);
        hdr.SetTpcLinkMargin(-3);
        hdr.SetRxAntennaId(0);
        hdr.SetTxAntennaId(0);
        hdr.SetRcpi(100);
        hdr.SetRsni(40);

        TestHeaderSerialization(hdr);

        Buffer buf;
        buf.AddAtStart(hdr.GetSerializedSize());
        hdr.Serialize(buf.Begin());

        LinkMeasurementReportHeader deserialized;
        deserialized.Deserialize(buf.Begin());

        NS_TEST_EXPECT_MSG_EQ(deserialized.GetTpcTransmitPower(),
                              -15,
                              "Negative TPC tx power round-trip");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetTpcLinkMargin(),
                              -3,
                              "Negative TPC link margin round-trip");
    }

    // Test 3: RCPI edge values
    {
        // RCPI = 0: P < -109.5 dBm
        LinkMeasurementReportHeader hdr;
        hdr.SetDialogToken(1);
        hdr.SetRcpi(0);
        TestHeaderSerialization(hdr);

        // RCPI = 220: P >= 0 dBm
        hdr.SetRcpi(220);
        TestHeaderSerialization(hdr);

        // RCPI = 255: measurement not available
        hdr.SetRcpi(255);
        TestHeaderSerialization(hdr);

        Buffer buf;
        buf.AddAtStart(hdr.GetSerializedSize());
        hdr.Serialize(buf.Begin());

        LinkMeasurementReportHeader deserialized;
        deserialized.Deserialize(buf.Begin());

        NS_TEST_EXPECT_MSG_EQ(deserialized.GetRcpi(), 255, "RCPI 255 round-trip");
    }

    // Test 4: RSNI edge values
    {
        LinkMeasurementReportHeader hdr;
        hdr.SetDialogToken(1);

        hdr.SetRsni(0);
        TestHeaderSerialization(hdr);

        hdr.SetRsni(255);
        TestHeaderSerialization(hdr);

        Buffer buf;
        buf.AddAtStart(hdr.GetSerializedSize());
        hdr.Serialize(buf.Begin());

        LinkMeasurementReportHeader deserialized;
        deserialized.Deserialize(buf.Begin());

        NS_TEST_EXPECT_MSG_EQ(deserialized.GetRsni(), 255, "RSNI 255 round-trip");
    }

    // Test 5: Antenna ID values
    {
        LinkMeasurementReportHeader hdr;
        hdr.SetDialogToken(1);
        hdr.SetRxAntennaId(255);
        hdr.SetTxAntennaId(128);

        TestHeaderSerialization(hdr);

        Buffer buf;
        buf.AddAtStart(hdr.GetSerializedSize());
        hdr.Serialize(buf.Begin());

        LinkMeasurementReportHeader deserialized;
        deserialized.Deserialize(buf.Begin());

        NS_TEST_EXPECT_MSG_EQ(deserialized.GetRxAntennaId(), 255, "Max Rx antenna ID");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetTxAntennaId(), 128, "Tx antenna ID 128");
    }

    // Test 6: Default construction round-trip
    {
        LinkMeasurementReportHeader hdr;
        TestHeaderSerialization(hdr);
    }

    // Test 7: GetSerializedSize returns 9
    {
        LinkMeasurementReportHeader hdr;
        NS_TEST_EXPECT_MSG_EQ(hdr.GetSerializedSize(), 9, "Serialized size is 9 bytes");
    }

    // Test 8: TpcReport element accessor
    {
        LinkMeasurementReportHeader hdr;
        TpcReportElement tpc;
        tpc.SetTransmitPower(-20);
        tpc.SetLinkMargin(15);
        hdr.SetTpcReport(tpc);

        NS_TEST_EXPECT_MSG_EQ(hdr.GetTpcTransmitPower(), -20, "TPC power via element setter");
        NS_TEST_EXPECT_MSG_EQ(hdr.GetTpcLinkMargin(), 15, "TPC margin via element setter");

        TestHeaderSerialization(hdr);
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
    AddTestCase(new TpcReportElementTest, TestCase::Duration::QUICK);
    AddTestCase(new LinkMeasurementReportTest, TestCase::Duration::QUICK);
}

static WifiRrmInfoElemsTestSuite g_wifiRrmInfoElemsTestSuite; ///< the test suite
