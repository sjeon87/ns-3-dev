/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Devansh Gupta <devanshg308@gmail.com>
 */

#include "ns3/header-serialization-test.h"
#include "ns3/ht-capabilities.h"
#include "ns3/ht-operation.h"
#include "ns3/log.h"
#include "ns3/measurement-request-element.h"
#include "ns3/neighbor-report-element.h"
#include "ns3/rm-enabled-capabilities.h"
#include "ns3/tpc-report-element.h"
#include "ns3/vht-capabilities.h"
#include "ns3/vht-operation.h"

#include <sstream>
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
        NS_TEST_ASSERT_MSG_EQ(cc->c1, 'U', "Country char 1");
        NS_TEST_ASSERT_MSG_EQ(cc->c2, 'S', "Country char 2");
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
        NS_TEST_ASSERT_MSG_EQ(pref->preference, 200, "Candidate preference value");
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
        NS_TEST_ASSERT_MSG_EQ(vs->data.size(), 4, "Vendor specific size");
        NS_TEST_ASSERT_MSG_EQ(vs->data[0], 0xAA, "Vendor byte 0");
        NS_TEST_ASSERT_MSG_EQ(vs->data[3], 0xDD, "Vendor byte 3");
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
        NS_TEST_ASSERT_MSG_EQ(cc->c1, 'G', "Country c1 all-together");
        NS_TEST_ASSERT_MSG_EQ(cc->c2, 'B', "Country c2 all-together");

        auto pref = deserialized.GetCandidatePreference();
        NS_TEST_ASSERT_MSG_EQ(pref.has_value(), true, "Pref survives all-together");
        NS_TEST_ASSERT_MSG_EQ(pref->preference, 150, "Pref value all-together");

        auto term = deserialized.GetBssTerminationDuration();
        NS_TEST_ASSERT_MSG_EQ(term.has_value(), true, "Term survives all-together");
        NS_TEST_ASSERT_MSG_EQ(term->terminationTsf, 999999, "Term TSF all-together");
        NS_TEST_ASSERT_MSG_EQ(term->duration, 120, "Term duration all-together");

        auto vs = deserialized.GetVendorSpecificData();
        NS_TEST_ASSERT_MSG_EQ(vs.has_value(), true, "VS survives all-together");
        NS_TEST_ASSERT_MSG_EQ(vs->data.size(), 2, "VS size all-together");
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
        it.WriteU8(IE_NEIGHBOR_REPORT);
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
        NS_TEST_ASSERT_MSG_EQ(pref->preference, 42, "Candidate pref value after unknown skip");
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
        NS_TEST_ASSERT_MSG_EQ(nre2.GetCandidatePreference()->preference,
                              255,
                              "Max candidate preference");

        // Empty vendor specific data
        NeighborReportElement nre3;
        nre3.SetBssid(Mac48Address("00:11:22:33:44:55"));
        nre3.SetOperatingClass(81);
        nre3.SetChannelNumber(6);
        nre3.SetPhyType(7);
        nre3.SetVendorSpecificData({});
        TestHeaderSerialization(nre3);
        NS_TEST_ASSERT_MSG_EQ(nre3.GetVendorSpecificData()->data.empty(),
                              true,
                              "Empty vendor specific");

        // Large vendor specific data
        NeighborReportElement nre4;
        nre4.SetBssid(Mac48Address("00:11:22:33:44:55"));
        nre4.SetOperatingClass(81);
        nre4.SetChannelNumber(6);
        nre4.SetPhyType(7);
        std::vector<uint8_t> largeData(200, 0x42);
        nre4.SetVendorSpecificData(largeData);
        TestHeaderSerialization(nre4);
        NS_TEST_ASSERT_MSG_EQ(nre4.GetVendorSpecificData()->data.size(),
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
        it.WriteU8(IE_NEIGHBOR_REPORT);
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
 * @brief Test serialization and deserialization of the RM Enabled Capabilities element
 * (IEEE 802.11-2024 Section 9.4.2.43, IE 70)
 */
class RmEnabledCapabilitiesTest : public HeaderSerializationTestCase
{
  public:
    RmEnabledCapabilitiesTest();

  private:
    void DoRun() override;
};

RmEnabledCapabilitiesTest::RmEnabledCapabilitiesTest()
    : HeaderSerializationTestCase(
          "Check serialization and deserialization of RM Enabled Capabilities element")
{
}

void
RmEnabledCapabilitiesTest::DoRun()
{
    // Test 1: Default construction round-trip
    {
        RmEnabledCapabilities elem;
        TestHeaderSerialization(elem);
    }

    // Test 2: GetSerializedSize returns 7 (IE ID 1 + Length 1 + 5 payload)
    {
        RmEnabledCapabilities elem;
        NS_TEST_EXPECT_MSG_EQ(elem.GetSerializedSize(), 7, "Serialized size is 7 bytes");
    }

    // Test 3: Individual boolean setters/getters -- each bit in isolation
    {
        struct BoolCase
        {
            const char* name;
            void (RmEnabledCapabilities::*setter)(bool);
            bool (RmEnabledCapabilities::*getter)() const;
        };

        std::vector<BoolCase> cases = {
            {"LinkMeasurement",
             &RmEnabledCapabilities::SetLinkMeasurement,
             &RmEnabledCapabilities::GetLinkMeasurement},
            {"NeighborReport",
             &RmEnabledCapabilities::SetNeighborReport,
             &RmEnabledCapabilities::GetNeighborReport},
            {"ParallelMeasurements",
             &RmEnabledCapabilities::SetParallelMeasurements,
             &RmEnabledCapabilities::GetParallelMeasurements},
            {"RepeatedMeasurements",
             &RmEnabledCapabilities::SetRepeatedMeasurements,
             &RmEnabledCapabilities::GetRepeatedMeasurements},
            {"BeaconPassiveMeasurement",
             &RmEnabledCapabilities::SetBeaconPassiveMeasurement,
             &RmEnabledCapabilities::GetBeaconPassiveMeasurement},
            {"BeaconActiveMeasurement",
             &RmEnabledCapabilities::SetBeaconActiveMeasurement,
             &RmEnabledCapabilities::GetBeaconActiveMeasurement},
            {"BeaconTableMeasurement",
             &RmEnabledCapabilities::SetBeaconTableMeasurement,
             &RmEnabledCapabilities::GetBeaconTableMeasurement},
            {"BeaconMeasurementReportingConditions",
             &RmEnabledCapabilities::SetBeaconMeasurementReportingConditions,
             &RmEnabledCapabilities::GetBeaconMeasurementReportingConditions},
            {"FrameMeasurement",
             &RmEnabledCapabilities::SetFrameMeasurement,
             &RmEnabledCapabilities::GetFrameMeasurement},
            {"ChannelLoadMeasurement",
             &RmEnabledCapabilities::SetChannelLoadMeasurement,
             &RmEnabledCapabilities::GetChannelLoadMeasurement},
            {"NoiseHistogramMeasurement",
             &RmEnabledCapabilities::SetNoiseHistogramMeasurement,
             &RmEnabledCapabilities::GetNoiseHistogramMeasurement},
            {"StatisticsMeasurement",
             &RmEnabledCapabilities::SetStatisticsMeasurement,
             &RmEnabledCapabilities::GetStatisticsMeasurement},
            {"LciMeasurement",
             &RmEnabledCapabilities::SetLciMeasurement,
             &RmEnabledCapabilities::GetLciMeasurement},
            {"LciAzimuth",
             &RmEnabledCapabilities::SetLciAzimuth,
             &RmEnabledCapabilities::GetLciAzimuth},
            {"TransmitStreamCategoryMeasurement",
             &RmEnabledCapabilities::SetTransmitStreamCategoryMeasurement,
             &RmEnabledCapabilities::GetTransmitStreamCategoryMeasurement},
            {"TriggeredTransmitStreamCategoryMeasurement",
             &RmEnabledCapabilities::SetTriggeredTransmitStreamCategoryMeasurement,
             &RmEnabledCapabilities::GetTriggeredTransmitStreamCategoryMeasurement},
            {"ApChannelReport",
             &RmEnabledCapabilities::SetApChannelReport,
             &RmEnabledCapabilities::GetApChannelReport},
            {"RmMib", &RmEnabledCapabilities::SetRmMib, &RmEnabledCapabilities::GetRmMib},
            {"MeasurementPilotTransmissionInformation",
             &RmEnabledCapabilities::SetMeasurementPilotTransmissionInformation,
             &RmEnabledCapabilities::GetMeasurementPilotTransmissionInformation},
            {"NeighborReportTsfOffset",
             &RmEnabledCapabilities::SetNeighborReportTsfOffset,
             &RmEnabledCapabilities::GetNeighborReportTsfOffset},
            {"RcpiMeasurement",
             &RmEnabledCapabilities::SetRcpiMeasurement,
             &RmEnabledCapabilities::GetRcpiMeasurement},
            {"RsniMeasurement",
             &RmEnabledCapabilities::SetRsniMeasurement,
             &RmEnabledCapabilities::GetRsniMeasurement},
            {"BssAverageAccessDelay",
             &RmEnabledCapabilities::SetBssAverageAccessDelay,
             &RmEnabledCapabilities::GetBssAverageAccessDelay},
            {"BssAvailableAdmissionCapacity",
             &RmEnabledCapabilities::SetBssAvailableAdmissionCapacity,
             &RmEnabledCapabilities::GetBssAvailableAdmissionCapacity},
            {"Antenna", &RmEnabledCapabilities::SetAntenna, &RmEnabledCapabilities::GetAntenna},
            {"FtmRangeReport",
             &RmEnabledCapabilities::SetFtmRangeReport,
             &RmEnabledCapabilities::GetFtmRangeReport},
            {"CivicLocationMeasurement",
             &RmEnabledCapabilities::SetCivicLocationMeasurement,
             &RmEnabledCapabilities::GetCivicLocationMeasurement},
        };

        for (const auto& tc : cases)
        {
            RmEnabledCapabilities elem;
            (elem.*tc.setter)(true);
            NS_TEST_EXPECT_MSG_EQ((elem.*tc.getter)(), true, tc.name << " getter after set");

            (elem.*tc.setter)(false);
            NS_TEST_EXPECT_MSG_EQ((elem.*tc.getter)(), false, tc.name << " getter after clear");
        }
    }

    // Test 4: Setting one bit does not affect another
    {
        RmEnabledCapabilities elem;
        elem.SetLinkMeasurement(true);
        elem.SetNeighborReport(true);

        NS_TEST_EXPECT_MSG_EQ(elem.GetLinkMeasurement(), true, "LinkMeasurement stays set");
        NS_TEST_EXPECT_MSG_EQ(elem.GetNeighborReport(), true, "NeighborReport stays set");
        NS_TEST_EXPECT_MSG_EQ(elem.GetParallelMeasurements(), false, "ParallelMeasurements unset");
        NS_TEST_EXPECT_MSG_EQ(elem.GetBeaconPassiveMeasurement(),
                              false,
                              "BeaconPassive unaffected");

        elem.SetLinkMeasurement(false);
        NS_TEST_EXPECT_MSG_EQ(elem.GetLinkMeasurement(), false, "LinkMeasurement cleared");
        NS_TEST_EXPECT_MSG_EQ(elem.GetNeighborReport(), true, "NeighborReport unaffected");
    }

    // Test 5: 3-bit field accessors -- all values 0-7
    {
        for (uint8_t val = 0; val <= 7; val++)
        {
            RmEnabledCapabilities elem;
            elem.SetOperatingChannelMaxMeasurementDuration(val);
            NS_TEST_EXPECT_MSG_EQ(elem.GetOperatingChannelMaxMeasurementDuration(),
                                  val,
                                  "OperatingChannelMaxMeasurementDuration=" << +val);
        }
        for (uint8_t val = 0; val <= 7; val++)
        {
            RmEnabledCapabilities elem;
            elem.SetNonoperatingChannelMaxMeasurementDuration(val);
            NS_TEST_EXPECT_MSG_EQ(elem.GetNonoperatingChannelMaxMeasurementDuration(),
                                  val,
                                  "NonoperatingChannelMaxMeasurementDuration=" << +val);
        }
        for (uint8_t val = 0; val <= 7; val++)
        {
            RmEnabledCapabilities elem;
            elem.SetMeasurementPilotCapability(val);
            NS_TEST_EXPECT_MSG_EQ(elem.GetMeasurementPilotCapability(),
                                  val,
                                  "MeasurementPilotCapability=" << +val);
        }
    }

    // Test 6: 3-bit fields are independent of each other and of boolean bits
    {
        RmEnabledCapabilities elem;
        elem.SetOperatingChannelMaxMeasurementDuration(5);
        elem.SetNonoperatingChannelMaxMeasurementDuration(3);
        elem.SetMeasurementPilotCapability(7);

        NS_TEST_EXPECT_MSG_EQ(elem.GetOperatingChannelMaxMeasurementDuration(),
                              5,
                              "OperatingChannel after setting all three");
        NS_TEST_EXPECT_MSG_EQ(elem.GetNonoperatingChannelMaxMeasurementDuration(),
                              3,
                              "NonoperatingChannel after setting all three");
        NS_TEST_EXPECT_MSG_EQ(elem.GetMeasurementPilotCapability(),
                              7,
                              "MeasurementPilot after setting all three");

        // Boolean bits adjacent to the 3-bit fields should be unaffected
        NS_TEST_EXPECT_MSG_EQ(elem.GetApChannelReport(), false, "ApChannelReport unaffected");
        NS_TEST_EXPECT_MSG_EQ(elem.GetRmMib(), false, "RmMib unaffected");
        NS_TEST_EXPECT_MSG_EQ(elem.GetMeasurementPilotTransmissionInformation(),
                              false,
                              "MeasurementPilotTransmissionInfo unaffected");
    }

    // Test 7: All capabilities set -- serialize, deserialize, verify all survive
    {
        RmEnabledCapabilities elem;
        elem.SetLinkMeasurement(true);
        elem.SetNeighborReport(true);
        elem.SetParallelMeasurements(true);
        elem.SetRepeatedMeasurements(true);
        elem.SetBeaconPassiveMeasurement(true);
        elem.SetBeaconActiveMeasurement(true);
        elem.SetBeaconTableMeasurement(true);
        elem.SetBeaconMeasurementReportingConditions(true);
        elem.SetFrameMeasurement(true);
        elem.SetChannelLoadMeasurement(true);
        elem.SetNoiseHistogramMeasurement(true);
        elem.SetStatisticsMeasurement(true);
        elem.SetLciMeasurement(true);
        elem.SetLciAzimuth(true);
        elem.SetTransmitStreamCategoryMeasurement(true);
        elem.SetTriggeredTransmitStreamCategoryMeasurement(true);
        elem.SetApChannelReport(true);
        elem.SetRmMib(true);
        elem.SetOperatingChannelMaxMeasurementDuration(7);
        elem.SetNonoperatingChannelMaxMeasurementDuration(7);
        elem.SetMeasurementPilotCapability(7);
        elem.SetMeasurementPilotTransmissionInformation(true);
        elem.SetNeighborReportTsfOffset(true);
        elem.SetRcpiMeasurement(true);
        elem.SetRsniMeasurement(true);
        elem.SetBssAverageAccessDelay(true);
        elem.SetBssAvailableAdmissionCapacity(true);
        elem.SetAntenna(true);
        elem.SetFtmRangeReport(true);
        elem.SetCivicLocationMeasurement(true);

        TestHeaderSerialization(elem);

        Buffer buf;
        buf.AddAtStart(elem.GetSerializedSize());
        elem.Serialize(buf.Begin());

        RmEnabledCapabilities deserialized;
        deserialized.Deserialize(buf.Begin());

        NS_TEST_EXPECT_MSG_EQ(deserialized.GetLinkMeasurement(), true, "LinkMeasurement survives");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetNeighborReport(), true, "NeighborReport survives");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetParallelMeasurements(),
                              true,
                              "ParallelMeasurements survives");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetRepeatedMeasurements(),
                              true,
                              "RepeatedMeasurements survives");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetBeaconPassiveMeasurement(),
                              true,
                              "BeaconPassive survives");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetBeaconActiveMeasurement(),
                              true,
                              "BeaconActive survives");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetBeaconTableMeasurement(),
                              true,
                              "BeaconTable survives");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetBeaconMeasurementReportingConditions(),
                              true,
                              "BeaconReportingConditions survives");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetFrameMeasurement(),
                              true,
                              "FrameMeasurement survives");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetChannelLoadMeasurement(),
                              true,
                              "ChannelLoad survives");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetNoiseHistogramMeasurement(),
                              true,
                              "NoiseHistogram survives");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetStatisticsMeasurement(), true, "Statistics survives");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetLciMeasurement(), true, "LCI survives");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetLciAzimuth(), true, "LciAzimuth survives");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetTransmitStreamCategoryMeasurement(),
                              true,
                              "TransmitStreamCategory survives");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetTriggeredTransmitStreamCategoryMeasurement(),
                              true,
                              "TriggeredTransmitStreamCategory survives");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetApChannelReport(), true, "ApChannelReport survives");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetRmMib(), true, "RmMib survives");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetOperatingChannelMaxMeasurementDuration(),
                              7,
                              "OperatingChannelMax survives");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetNonoperatingChannelMaxMeasurementDuration(),
                              7,
                              "NonoperatingChannelMax survives");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetMeasurementPilotCapability(),
                              7,
                              "MeasurementPilotCapability survives");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetMeasurementPilotTransmissionInformation(),
                              true,
                              "MeasurementPilotTransmissionInfo survives");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetNeighborReportTsfOffset(),
                              true,
                              "NeighborReportTsfOffset survives");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetRcpiMeasurement(), true, "Rcpi survives");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetRsniMeasurement(), true, "Rsni survives");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetBssAverageAccessDelay(),
                              true,
                              "BssAverageAccessDelay survives");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetBssAvailableAdmissionCapacity(),
                              true,
                              "BssAvailableAdmissionCapacity survives");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetAntenna(), true, "Antenna survives");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetFtmRangeReport(), true, "FtmRangeReport survives");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetCivicLocationMeasurement(),
                              true,
                              "CivicLocation survives");
    }

    // Test 8: Reserved bits (36-39) stay zero after round-trip
    {
        // Build a buffer with bits 36-39 set in the 5th payload byte
        // IE ID (1) + Length (1) + 5 payload bytes = 7 total
        Buffer buf;
        buf.AddAtStart(7);
        Buffer::Iterator it = buf.Begin();
        it.WriteU8(IE_RM_ENABLED_CAPACITIES);
        it.WriteU8(5); // Length
        it.WriteU8(0xFF);
        it.WriteU8(0xFF);
        it.WriteU8(0xFF);
        it.WriteU8(0xFF);
        it.WriteU8(0xFF); // bits 32-39: bits 36-39 are reserved

        RmEnabledCapabilities deserialized;
        deserialized.Deserialize(buf.Begin());

        // Re-serialize and verify reserved bits (36-39) of byte 4 are zeroed
        Buffer out;
        out.AddAtStart(deserialized.GetSerializedSize());
        deserialized.Serialize(out.Begin());

        Buffer::Iterator outIt = out.Begin();
        outIt.Next(6); // skip IE ID, Length, bytes 0-3
        uint8_t byte4 = outIt.ReadU8();
        NS_TEST_EXPECT_MSG_EQ((byte4 >> 4), 0, "Reserved bits 36-39 are zero");
    }

    // Test 9: Print output smoke test
    {
        RmEnabledCapabilities elem;
        elem.SetLinkMeasurement(true);
        elem.SetNeighborReport(true);

        std::ostringstream oss;
        elem.Print(oss);
        NS_TEST_EXPECT_MSG_EQ(oss.str().empty(), false, "Print output is non-empty");
    }
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test serialization and deserialization of the Measurement Request element
 * (IEEE 802.11-2024 Section 9.4.2.19, IE 38)
 */
class MeasurementRequestElementTest : public HeaderSerializationTestCase
{
  public:
    MeasurementRequestElementTest();

  private:
    void DoRun() override;
};

MeasurementRequestElementTest::MeasurementRequestElementTest()
    : HeaderSerializationTestCase(
          "Check serialization and deserialization of Measurement Request elements")
{
}

void
MeasurementRequestElementTest::DoRun()
{
    using BasicBody = MeasurementRequestElement::BasicRequestBody;
    using ChannelLoadBody = MeasurementRequestElement::ChannelLoadRequestBody;
    using NoiseHistogramBody = MeasurementRequestElement::NoiseHistogramRequestBody;
    using BeaconBody = MeasurementRequestElement::BeaconRequestBody;
    using FrameBody = MeasurementRequestElement::FrameRequestBody;
    using StaStatsBody = MeasurementRequestElement::StaStatisticsRequestBody;
    using LciBody = MeasurementRequestElement::LciRequestBody;
    using TxStreamBody = MeasurementRequestElement::TransmitStreamRequestBody;
    using MulticastBody = MeasurementRequestElement::MulticastDiagnosticsRequestBody;
    using LocCivicBody = MeasurementRequestElement::LocationCivicRequestBody;
    using LocIdBody = MeasurementRequestElement::LocationIdentifierRequestBody;
    using DirChQBody = MeasurementRequestElement::DirectionalChannelQualityRequestBody;
    using DirMeasBody = MeasurementRequestElement::DirectionalMeasurementRequestBody;
    using DirStatsBody = MeasurementRequestElement::DirectionalStatisticsRequestBody;
    using FtmBody = MeasurementRequestElement::FtmRangeRequestBody;
    using PauseBody = MeasurementRequestElement::MeasurementPauseRequestBody;
    using MeasurementType = MeasurementRequestElement::MeasurementType;

    // IE header (2) + token (1) + mode (1) + type (1) = 5 fixed bytes

    // Test 1: Size assertions per measurement type (no optional subelements)
    {
        // Type 0 -- Basic: Channel(1) + StartTime(8) + Duration(2) = 11
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(1);
            elem.SetMeasurementType(MeasurementType::BASIC);
            elem.SetBody(BasicBody{6, 0, 200});
            NS_TEST_EXPECT_MSG_EQ(elem.GetSerializedSize(), 16, "Basic size");
        }

        // Type 1 -- CCA: Channel(1) + StartTime(8) + Duration(2) = 11
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(1);
            elem.SetMeasurementType(MeasurementType::CCA);
            elem.SetBody(BasicBody{6, 0, 200});
            NS_TEST_EXPECT_MSG_EQ(elem.GetSerializedSize(), 16, "CCA size");
        }

        // Type 2 -- RPI Histogram: Channel(1) + StartTime(8) + Duration(2) = 11
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(1);
            elem.SetMeasurementType(MeasurementType::RPI_HISTOGRAM);
            elem.SetBody(BasicBody{6, 0, 200});
            NS_TEST_EXPECT_MSG_EQ(elem.GetSerializedSize(), 16, "RPI Histogram size");
        }

        // Type 3 -- Channel Load: OpClass(1) + Channel(1) + RandInterval(2) + Duration(2) = 6
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(1);
            elem.SetMeasurementType(MeasurementType::CHANNEL_LOAD);
            ChannelLoadBody body;
            body.operatingClass = 81;
            body.channelNumber = 6;
            body.randomizationInterval = 100;
            body.measurementDuration = 200;
            elem.SetBody(body);
            NS_TEST_EXPECT_MSG_EQ(elem.GetSerializedSize(), 11, "Channel Load size");
        }

        // Type 4 -- Noise Histogram: same layout as Channel Load = 6
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(1);
            elem.SetMeasurementType(MeasurementType::NOISE_HISTOGRAM);
            NoiseHistogramBody body;
            body.operatingClass = 81;
            body.channelNumber = 6;
            body.randomizationInterval = 100;
            body.measurementDuration = 200;
            elem.SetBody(body);
            NS_TEST_EXPECT_MSG_EQ(elem.GetSerializedSize(), 11, "Noise Histogram size");
        }

        // Type 5 -- Beacon: OpClass(1) + Channel(1) + RandInterval(2) + Duration(2) + Mode(1) +
        //           BSSID(6) = 13
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(1);
            elem.SetMeasurementType(MeasurementType::BEACON);
            BeaconBody body;
            body.operatingClass = 81;
            body.channelNumber = 6;
            body.randomizationInterval = 100;
            body.measurementDuration = 200;
            body.measurementMode = 0;
            body.bssid = Mac48Address("ff:ff:ff:ff:ff:ff");
            elem.SetBody(body);
            NS_TEST_EXPECT_MSG_EQ(elem.GetSerializedSize(), 18, "Beacon size");
        }

        // Type 6 -- Frame: OpClass(1) + Channel(1) + RandInterval(2) + Duration(2) +
        //           FrameReqType(1) + MAC(6) = 13
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(1);
            elem.SetMeasurementType(MeasurementType::FRAME);
            FrameBody body;
            body.operatingClass = 81;
            body.channelNumber = 6;
            body.randomizationInterval = 100;
            body.measurementDuration = 200;
            body.frameRequestType = 1;
            body.macAddress = Mac48Address("00:11:22:33:44:55");
            elem.SetBody(body);
            NS_TEST_EXPECT_MSG_EQ(elem.GetSerializedSize(), 18, "Frame size");
        }

        // Type 7 -- STA Statistics: PeerMAC(6) + RandInterval(2) + Duration(2) + GroupID(1) = 11
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(1);
            elem.SetMeasurementType(MeasurementType::STA_STATISTICS);
            StaStatsBody body;
            body.peerMacAddress = Mac48Address("00:11:22:33:44:55");
            body.randomizationInterval = 100;
            body.measurementDuration = 200;
            body.groupIdentity = 0;
            elem.SetBody(body);
            NS_TEST_EXPECT_MSG_EQ(elem.GetSerializedSize(), 16, "STA Statistics size");
        }

        // Type 8 -- LCI: LocationSubject(1) = 1
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(1);
            elem.SetMeasurementType(MeasurementType::LCI);
            LciBody body;
            body.locationSubject = 0;
            elem.SetBody(body);
            NS_TEST_EXPECT_MSG_EQ(elem.GetSerializedSize(), 6, "LCI size");
        }

        // Type 9 -- Transmit Stream: RandInterval(2) + Duration(2) + PeerSTA(6) + TID(1) +
        //           Bin0Range(1) = 12
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(1);
            elem.SetMeasurementType(MeasurementType::TRANSMIT_STREAM);
            TxStreamBody body;
            body.randomizationInterval = 100;
            body.measurementDuration = 200;
            body.peerStaAddress = Mac48Address("00:11:22:33:44:55");
            body.trafficIdentifier = 0;
            body.bin0Range = 10;
            elem.SetBody(body);
            NS_TEST_EXPECT_MSG_EQ(elem.GetSerializedSize(), 17, "Transmit Stream size");
        }

        // Type 10 -- Multicast Diagnostics: RandInterval(2) + Duration(2) + GroupMAC(6) = 10
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(1);
            elem.SetMeasurementType(MeasurementType::MULTICAST_DIAGNOSTICS);
            MulticastBody body;
            body.randomizationInterval = 100;
            body.measurementDuration = 200;
            body.groupMacAddress = Mac48Address("01:00:5e:00:00:01");
            elem.SetBody(body);
            NS_TEST_EXPECT_MSG_EQ(elem.GetSerializedSize(), 15, "Multicast Diagnostics size");
        }

        // Type 11 -- Location Civic
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(1);
            elem.SetMeasurementType(MeasurementType::LOCATION_CIVIC);
            elem.SetBody(LocCivicBody{});
            NS_TEST_EXPECT_MSG_EQ(elem.GetSerializedSize(), 10, "Location Civic size");
        }

        // Type 12 -- Location Identifier
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(1);
            elem.SetMeasurementType(MeasurementType::LOCATION_IDENTIFIER);
            elem.SetBody(LocIdBody{});
            NS_TEST_EXPECT_MSG_EQ(elem.GetSerializedSize(), 9, "Location Identifier size");
        }

        // Type 13 -- Directional Channel Quality
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(1);
            elem.SetMeasurementType(MeasurementType::DIRECTIONAL_CHANNEL_QUALITY);
            DirChQBody body;
            body.operatingClass = 1;
            body.channelNumber = 1;
            body.aid = 1;
            body.measurementDuration = 100;
            body.numberOfTimeBlocks = 1;
            elem.SetBody(body);
            NS_TEST_EXPECT_MSG_EQ(elem.GetSerializedSize(), 21, "Directional ChQ size");
        }

        // Type 14 -- Directional Measurement
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(1);
            elem.SetMeasurementType(MeasurementType::DIRECTIONAL_MEASUREMENT);
            DirMeasBody body;
            body.operatingClass = 1;
            body.channelNumber = 1;
            body.measurementDurationPerDirection = 100;
            elem.SetBody(body);
            NS_TEST_EXPECT_MSG_EQ(elem.GetSerializedSize(), 18, "Directional Measurement size");
        }

        // Type 15 -- Directional Statistics
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(1);
            elem.SetMeasurementType(MeasurementType::DIRECTIONAL_STATISTICS);
            DirStatsBody body;
            body.operatingClass = 1;
            body.channelNumber = 1;
            body.measurementDurationPerDirection = 100;
            elem.SetBody(body);
            NS_TEST_EXPECT_MSG_EQ(elem.GetSerializedSize(), 19, "Directional Statistics size");
        }

        // Type 16 -- FTM Range: RandInterval(2) + MinAPCount(1) = 3
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(1);
            elem.SetMeasurementType(MeasurementType::FTM_RANGE);
            FtmBody body;
            body.randomizationInterval = 100;
            body.minimumApCount = 1;
            elem.SetBody(body);
            NS_TEST_EXPECT_MSG_EQ(elem.GetSerializedSize(), 8, "FTM Range size");
        }

        // Type 255 -- Measurement Pause: PauseTime(2) = 2
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(1);
            elem.SetMeasurementType(MeasurementType::MEASUREMENT_PAUSE);
            PauseBody body;
            body.pauseTime = 100;
            elem.SetBody(body);
            NS_TEST_EXPECT_MSG_EQ(elem.GetSerializedSize(), 7, "Measurement Pause size");
        }
    }

    // Test 2: Round-trip serialization for each type with populated fields
    {
        // Basic
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(1);
            elem.SetMeasurementType(MeasurementType::BASIC);
            elem.SetBody(BasicBody{11, 123456789, 1000});
            TestHeaderSerialization(elem);
        }

        // CCA
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(2);
            elem.SetMeasurementType(MeasurementType::CCA);
            elem.SetBody(BasicBody{36, 0, 500});
            TestHeaderSerialization(elem);
        }

        // RPI Histogram
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(3);
            elem.SetMeasurementType(MeasurementType::RPI_HISTOGRAM);
            elem.SetBody(BasicBody{1, 999999, 2000});
            TestHeaderSerialization(elem);
        }

        // Channel Load
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(10);
            elem.SetMeasurementType(MeasurementType::CHANNEL_LOAD);
            ChannelLoadBody body;
            body.operatingClass = 81;
            body.channelNumber = 6;
            body.randomizationInterval = 500;
            body.measurementDuration = 1000;
            elem.SetBody(body);
            TestHeaderSerialization(elem);
        }

        // Noise Histogram
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(20);
            elem.SetMeasurementType(MeasurementType::NOISE_HISTOGRAM);
            NoiseHistogramBody body;
            body.operatingClass = 115;
            body.channelNumber = 36;
            body.randomizationInterval = 200;
            body.measurementDuration = 500;
            elem.SetBody(body);
            TestHeaderSerialization(elem);
        }

        // Beacon
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(30);
            elem.SetMeasurementType(MeasurementType::BEACON);
            BeaconBody body;
            body.operatingClass = 81;
            body.channelNumber = 6;
            body.randomizationInterval = 100;
            body.measurementDuration = 200;
            body.measurementMode = 1;
            body.bssid = Mac48Address("aa:bb:cc:dd:ee:ff");
            elem.SetBody(body);
            TestHeaderSerialization(elem);
        }

        // Frame
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(40);
            elem.SetMeasurementType(MeasurementType::FRAME);
            FrameBody body;
            body.operatingClass = 81;
            body.channelNumber = 11;
            body.randomizationInterval = 300;
            body.measurementDuration = 400;
            body.frameRequestType = 1;
            body.macAddress = Mac48Address("00:11:22:33:44:55");
            elem.SetBody(body);
            TestHeaderSerialization(elem);
        }

        // STA Statistics
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(50);
            elem.SetMeasurementType(MeasurementType::STA_STATISTICS);
            StaStatsBody body;
            body.peerMacAddress = Mac48Address("aa:bb:cc:dd:ee:ff");
            body.randomizationInterval = 100;
            body.measurementDuration = 600;
            body.groupIdentity = 10;
            elem.SetBody(body);
            TestHeaderSerialization(elem);
        }

        // LCI
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(60);
            elem.SetMeasurementType(MeasurementType::LCI);
            LciBody body;
            body.locationSubject = 1;
            elem.SetBody(body);
            TestHeaderSerialization(elem);
        }

        // Transmit Stream
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(70);
            elem.SetMeasurementType(MeasurementType::TRANSMIT_STREAM);
            TxStreamBody body;
            body.randomizationInterval = 50;
            body.measurementDuration = 100;
            body.peerStaAddress = Mac48Address("00:11:22:33:44:55");
            body.trafficIdentifier = 4;
            body.bin0Range = 20;
            elem.SetBody(body);
            TestHeaderSerialization(elem);
        }

        // Multicast Diagnostics
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(80);
            elem.SetMeasurementType(MeasurementType::MULTICAST_DIAGNOSTICS);
            MulticastBody body;
            body.randomizationInterval = 100;
            body.measurementDuration = 200;
            body.groupMacAddress = Mac48Address("01:00:5e:00:00:01");
            elem.SetBody(body);
            TestHeaderSerialization(elem);
        }

        // Location Civic
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(90);
            elem.SetMeasurementType(MeasurementType::LOCATION_CIVIC);
            LocCivicBody body;
            body.locationSubject = 1;
            body.locationServiceIntervalUnits = 1;
            body.locationServiceInterval = 60;
            elem.SetBody(body);
            TestHeaderSerialization(elem);
        }

        // Location Identifier
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(100);
            elem.SetMeasurementType(MeasurementType::LOCATION_IDENTIFIER);
            LocIdBody body;
            body.locationServiceIntervalUnits = 2;
            body.locationServiceInterval = 24;
            elem.SetBody(body);
            TestHeaderSerialization(elem);
        }

        // Directional Channel Quality
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(110);
            elem.SetMeasurementType(MeasurementType::DIRECTIONAL_CHANNEL_QUALITY);
            DirChQBody body;
            body.operatingClass = 1;
            body.channelNumber = 1;
            body.aid = 5;
            body.measurementMethod = 1;
            body.measurementStartTime = 1000;
            body.measurementDuration = 200;
            body.numberOfTimeBlocks = 4;
            elem.SetBody(body);
            TestHeaderSerialization(elem);
        }

        // Directional Measurement
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(120);
            elem.SetMeasurementType(MeasurementType::DIRECTIONAL_MEASUREMENT);
            DirMeasBody body;
            body.operatingClass = 1;
            body.channelNumber = 1;
            body.measurementDurationPerDirection = 100;
            body.measurementMethodAndAntennaConfiguration = 0x09;
            elem.SetBody(body);
            TestHeaderSerialization(elem);
        }

        // Directional Statistics
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(130);
            elem.SetMeasurementType(MeasurementType::DIRECTIONAL_STATISTICS);
            DirStatsBody body;
            body.operatingClass = 1;
            body.channelNumber = 1;
            body.measurementDurationPerDirection = 100;
            body.measurementMethod = 2;
            body.directionalStatisticsBitmap = 0x0F;
            elem.SetBody(body);
            TestHeaderSerialization(elem);
        }

        // FTM Range
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(140);
            elem.SetMeasurementType(MeasurementType::FTM_RANGE);
            FtmBody body;
            body.randomizationInterval = 100;
            body.minimumApCount = 3;
            elem.SetBody(body);
            TestHeaderSerialization(elem);
        }

        // Measurement Pause
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(150);
            elem.SetMeasurementType(MeasurementType::MEASUREMENT_PAUSE);
            PauseBody body;
            body.pauseTime = 5000;
            elem.SetBody(body);
            TestHeaderSerialization(elem);
        }
    }

    // Test 3: Field value assertions after deserialization for representative types
    {
        // Basic
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(1);
            elem.SetMeasurementType(MeasurementType::BASIC);
            elem.SetBody(BasicBody{11, 123456789, 1000});

            Buffer buf;
            buf.AddAtStart(elem.GetSerializedSize());
            elem.Serialize(buf.Begin());

            MeasurementRequestElement deserialized;
            deserialized.Deserialize(buf.Begin());

            NS_TEST_EXPECT_MSG_EQ(deserialized.GetMeasurementToken(), 1, "Basic token");
            NS_TEST_EXPECT_MSG_EQ(deserialized.GetMeasurementType(),
                                  MeasurementType::BASIC,
                                  "Basic type");
            auto& b = deserialized.GetBody<BasicBody>();
            NS_TEST_EXPECT_MSG_EQ(b.channelNumber, 11, "Basic channel");
            NS_TEST_EXPECT_MSG_EQ(b.measurementStartTime, 123456789, "Basic start time");
            NS_TEST_EXPECT_MSG_EQ(b.measurementDuration, 1000, "Basic duration");
        }

        // CCA
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(2);
            elem.SetMeasurementType(MeasurementType::CCA);
            elem.SetBody(BasicBody{36, 0, 500});

            Buffer buf;
            buf.AddAtStart(elem.GetSerializedSize());
            elem.Serialize(buf.Begin());

            MeasurementRequestElement deserialized;
            deserialized.Deserialize(buf.Begin());

            NS_TEST_EXPECT_MSG_EQ(deserialized.GetMeasurementType(),
                                  MeasurementType::CCA,
                                  "CCA type");
            auto& b = deserialized.GetBody<BasicBody>();
            NS_TEST_EXPECT_MSG_EQ(b.channelNumber, 36, "CCA channel");
            NS_TEST_EXPECT_MSG_EQ(b.measurementStartTime, 0, "CCA start time");
            NS_TEST_EXPECT_MSG_EQ(b.measurementDuration, 500, "CCA duration");
        }

        // RPI Histogram
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(3);
            elem.SetMeasurementType(MeasurementType::RPI_HISTOGRAM);
            elem.SetBody(BasicBody{1, 999999, 2000});

            Buffer buf;
            buf.AddAtStart(elem.GetSerializedSize());
            elem.Serialize(buf.Begin());

            MeasurementRequestElement deserialized;
            deserialized.Deserialize(buf.Begin());

            NS_TEST_EXPECT_MSG_EQ(deserialized.GetMeasurementType(),
                                  MeasurementType::RPI_HISTOGRAM,
                                  "RPI type");
            auto& b = deserialized.GetBody<BasicBody>();
            NS_TEST_EXPECT_MSG_EQ(b.channelNumber, 1, "RPI channel");
            NS_TEST_EXPECT_MSG_EQ(b.measurementStartTime, 999999, "RPI start time");
            NS_TEST_EXPECT_MSG_EQ(b.measurementDuration, 2000, "RPI duration");
        }

        // Channel Load
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(42);
            elem.SetMeasurementType(MeasurementType::CHANNEL_LOAD);
            ChannelLoadBody body;
            body.operatingClass = 115;
            body.channelNumber = 36;
            body.randomizationInterval = 1234;
            body.measurementDuration = 5678;
            elem.SetBody(body);

            Buffer buf;
            buf.AddAtStart(elem.GetSerializedSize());
            elem.Serialize(buf.Begin());

            MeasurementRequestElement deserialized;
            deserialized.Deserialize(buf.Begin());

            NS_TEST_EXPECT_MSG_EQ(deserialized.GetMeasurementToken(), 42, "CL token");
            NS_TEST_EXPECT_MSG_EQ(deserialized.GetMeasurementType(),
                                  MeasurementType::CHANNEL_LOAD,
                                  "CL type");
            auto& b = deserialized.GetBody<ChannelLoadBody>();
            NS_TEST_EXPECT_MSG_EQ(b.operatingClass, 115, "CL opclass");
            NS_TEST_EXPECT_MSG_EQ(b.channelNumber, 36, "CL channel");
            NS_TEST_EXPECT_MSG_EQ(b.randomizationInterval, 1234, "CL rand interval");
            NS_TEST_EXPECT_MSG_EQ(b.measurementDuration, 5678, "CL duration");
        }

        // Beacon
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(7);
            elem.SetMeasurementType(MeasurementType::BEACON);
            BeaconBody body;
            body.operatingClass = 81;
            body.channelNumber = 0;
            body.randomizationInterval = 100;
            body.measurementDuration = 200;
            body.measurementMode = 2;
            body.bssid = Mac48Address("ff:ff:ff:ff:ff:ff");
            elem.SetBody(body);

            Buffer buf;
            buf.AddAtStart(elem.GetSerializedSize());
            elem.Serialize(buf.Begin());

            MeasurementRequestElement deserialized;
            deserialized.Deserialize(buf.Begin());

            auto& b = deserialized.GetBody<BeaconBody>();
            NS_TEST_EXPECT_MSG_EQ(b.measurementMode, 2, "Beacon mode");
            NS_TEST_EXPECT_MSG_EQ(b.bssid,
                                  Mac48Address("ff:ff:ff:ff:ff:ff"),
                                  "Beacon wildcard BSSID");
        }

        // LCI
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(99);
            elem.SetMeasurementType(MeasurementType::LCI);
            LciBody body;
            body.locationSubject = 2;
            elem.SetBody(body);

            Buffer buf;
            buf.AddAtStart(elem.GetSerializedSize());
            elem.Serialize(buf.Begin());

            MeasurementRequestElement deserialized;
            deserialized.Deserialize(buf.Begin());

            auto& b = deserialized.GetBody<LciBody>();
            NS_TEST_EXPECT_MSG_EQ(b.locationSubject, 2, "LCI location subject");
        }

        // STA Statistics
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(55);
            elem.SetMeasurementType(MeasurementType::STA_STATISTICS);
            StaStatsBody body;
            body.peerMacAddress = Mac48Address("aa:bb:cc:dd:ee:ff");
            body.randomizationInterval = 300;
            body.measurementDuration = 600;
            body.groupIdentity = 16;
            elem.SetBody(body);

            Buffer buf;
            buf.AddAtStart(elem.GetSerializedSize());
            elem.Serialize(buf.Begin());

            MeasurementRequestElement deserialized;
            deserialized.Deserialize(buf.Begin());

            auto& b = deserialized.GetBody<StaStatsBody>();
            NS_TEST_EXPECT_MSG_EQ(b.peerMacAddress,
                                  Mac48Address("aa:bb:cc:dd:ee:ff"),
                                  "STA Stats peer MAC");
            NS_TEST_EXPECT_MSG_EQ(b.groupIdentity, 16, "STA Stats group ID");
        }

        // Transmit Stream
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(77);
            elem.SetMeasurementType(MeasurementType::TRANSMIT_STREAM);
            TxStreamBody body;
            body.randomizationInterval = 50;
            body.measurementDuration = 100;
            body.peerStaAddress = Mac48Address("00:11:22:33:44:55");
            body.trafficIdentifier = 7;
            body.bin0Range = 30;
            elem.SetBody(body);

            Buffer buf;
            buf.AddAtStart(elem.GetSerializedSize());
            elem.Serialize(buf.Begin());

            MeasurementRequestElement deserialized;
            deserialized.Deserialize(buf.Begin());

            auto& b = deserialized.GetBody<TxStreamBody>();
            NS_TEST_EXPECT_MSG_EQ(b.peerStaAddress,
                                  Mac48Address("00:11:22:33:44:55"),
                                  "TxStream peer STA");
            NS_TEST_EXPECT_MSG_EQ(b.trafficIdentifier, 7, "TxStream TID");
            NS_TEST_EXPECT_MSG_EQ(b.bin0Range, 30, "TxStream bin0 range");
        }
    }

    // Test 4: Default-constructed element round-trip
    {
        MeasurementRequestElement elem;
        TestHeaderSerialization(elem);
    }

    // Test 5: Edge values
    {
        // Max token (255)
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(255);
            elem.SetMeasurementType(MeasurementType::CHANNEL_LOAD);
            ChannelLoadBody body;
            body.operatingClass = 81;
            body.channelNumber = 6;
            body.randomizationInterval = 100;
            body.measurementDuration = 200;
            elem.SetBody(body);

            Buffer buf;
            buf.AddAtStart(elem.GetSerializedSize());
            elem.Serialize(buf.Begin());

            MeasurementRequestElement deserialized;
            deserialized.Deserialize(buf.Begin());

            NS_TEST_EXPECT_MSG_EQ(deserialized.GetMeasurementToken(), 255, "Max token");
        }

        // Max randomization interval (0xFFFF) and max duration (0xFFFF)
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(1);
            elem.SetMeasurementType(MeasurementType::CHANNEL_LOAD);
            ChannelLoadBody body;
            body.operatingClass = 81;
            body.channelNumber = 6;
            body.randomizationInterval = 0xFFFF;
            body.measurementDuration = 0xFFFF;
            elem.SetBody(body);

            Buffer buf;
            buf.AddAtStart(elem.GetSerializedSize());
            elem.Serialize(buf.Begin());

            MeasurementRequestElement deserialized;
            deserialized.Deserialize(buf.Begin());

            auto& b = deserialized.GetBody<ChannelLoadBody>();
            NS_TEST_EXPECT_MSG_EQ(b.randomizationInterval, 0xFFFF, "Max rand interval");
            NS_TEST_EXPECT_MSG_EQ(b.measurementDuration, 0xFFFF, "Max duration");
        }

        // Wildcard BSSID in Beacon request
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(1);
            elem.SetMeasurementType(MeasurementType::BEACON);
            BeaconBody body;
            body.operatingClass = 81;
            body.channelNumber = 255;
            body.bssid = Mac48Address("ff:ff:ff:ff:ff:ff");
            elem.SetBody(body);
            TestHeaderSerialization(elem);
        }

        // Measurement mode = 0 (all bits clear)
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(1);
            elem.SetMeasurementRequestMode(0);
            elem.SetMeasurementType(MeasurementType::CHANNEL_LOAD);
            ChannelLoadBody body;
            body.operatingClass = 81;
            body.channelNumber = 6;
            elem.SetBody(body);

            Buffer buf;
            buf.AddAtStart(elem.GetSerializedSize());
            elem.Serialize(buf.Begin());

            MeasurementRequestElement deserialized;
            deserialized.Deserialize(buf.Begin());

            NS_TEST_EXPECT_MSG_EQ(deserialized.GetMeasurementRequestMode(),
                                  0,
                                  "Mode zero round-trip");
        }
    }
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test Measurement Request Mode bitmap accessors
 * (IEEE 802.11-2024 Figure 9-242)
 */
class MeasurementRequestModeTest : public TestCase
{
  public:
    MeasurementRequestModeTest();

  private:
    void DoRun() override;
};

MeasurementRequestModeTest::MeasurementRequestModeTest()
    : TestCase("Check Measurement Request Mode bitmap accessors")
{
}

void
MeasurementRequestModeTest::DoRun()
{
    using MeasurementType = MeasurementRequestElement::MeasurementType;

    // Test 1: Individual bit setters produce correct raw value
    {
        MeasurementRequestElement elem;

        elem.SetMeasurementRequestMode(0);
        elem.SetParallel(true);
        NS_TEST_EXPECT_MSG_EQ(elem.GetMeasurementRequestMode() & 0x01, 1, "Parallel B0");

        elem.SetMeasurementRequestMode(0);
        elem.SetEnable(true);
        NS_TEST_EXPECT_MSG_EQ(elem.GetMeasurementRequestMode() & 0x02, 2, "Enable B1");

        elem.SetMeasurementRequestMode(0);
        elem.SetRequest(true);
        NS_TEST_EXPECT_MSG_EQ(elem.GetMeasurementRequestMode() & 0x04, 4, "Request B2");

        elem.SetMeasurementRequestMode(0);
        elem.SetReport(true);
        NS_TEST_EXPECT_MSG_EQ(elem.GetMeasurementRequestMode() & 0x08, 8, "Report B3");

        elem.SetMeasurementRequestMode(0);
        elem.SetDurationMandatory(true);
        NS_TEST_EXPECT_MSG_EQ(elem.GetMeasurementRequestMode() & 0x10,
                              0x10,
                              "DurationMandatory B4");
    }

    // Test 2: Each bit in isolation produces correct raw value
    {
        MeasurementRequestElement elem;

        elem.SetMeasurementRequestMode(0);
        elem.SetParallel(true);
        NS_TEST_EXPECT_MSG_EQ(elem.GetMeasurementRequestMode(), 0x01, "Only Parallel set");

        elem.SetMeasurementRequestMode(0);
        elem.SetEnable(true);
        NS_TEST_EXPECT_MSG_EQ(elem.GetMeasurementRequestMode(), 0x02, "Only Enable set");

        elem.SetMeasurementRequestMode(0);
        elem.SetRequest(true);
        NS_TEST_EXPECT_MSG_EQ(elem.GetMeasurementRequestMode(), 0x04, "Only Request set");

        elem.SetMeasurementRequestMode(0);
        elem.SetReport(true);
        NS_TEST_EXPECT_MSG_EQ(elem.GetMeasurementRequestMode(), 0x08, "Only Report set");

        elem.SetMeasurementRequestMode(0);
        elem.SetDurationMandatory(true);
        NS_TEST_EXPECT_MSG_EQ(elem.GetMeasurementRequestMode(), 0x10, "Only DurationMandatory set");
    }

    // Test 3: Raw value to individual getters
    {
        MeasurementRequestElement elem;
        elem.SetMeasurementRequestMode(0x1F); // all 5 bits set

        NS_TEST_EXPECT_MSG_EQ(elem.GetParallel(), true, "Parallel from 0x1F");
        NS_TEST_EXPECT_MSG_EQ(elem.GetEnable(), true, "Enable from 0x1F");
        NS_TEST_EXPECT_MSG_EQ(elem.GetRequest(), true, "Request from 0x1F");
        NS_TEST_EXPECT_MSG_EQ(elem.GetReport(), true, "Report from 0x1F");
        NS_TEST_EXPECT_MSG_EQ(elem.GetDurationMandatory(), true, "DurationMandatory from 0x1F");

        elem.SetMeasurementRequestMode(0x00);
        NS_TEST_EXPECT_MSG_EQ(elem.GetParallel(), false, "Parallel from 0x00");
        NS_TEST_EXPECT_MSG_EQ(elem.GetEnable(), false, "Enable from 0x00");
        NS_TEST_EXPECT_MSG_EQ(elem.GetRequest(), false, "Request from 0x00");
        NS_TEST_EXPECT_MSG_EQ(elem.GetReport(), false, "Report from 0x00");
        NS_TEST_EXPECT_MSG_EQ(elem.GetDurationMandatory(), false, "DurationMandatory from 0x00");
    }

    // Test 4: Reserved bits (B5-B7) are zeroed
    {
        MeasurementRequestElement elem;
        elem.SetMeasurementRequestMode(0xFF); // set all bits including reserved
        // Reserved bits should be masked out
        NS_TEST_EXPECT_MSG_EQ(elem.GetMeasurementRequestMode() & 0xE0,
                              0,
                              "Reserved bits B5-B7 zeroed");
    }

    // Test 5: Combined mode values matching Table 9-135 semantics
    {
        // Enable=0: standard measurement request (Request and Report are reserved)
        {
            MeasurementRequestElement elem;
            elem.SetEnable(false);
            NS_TEST_EXPECT_MSG_EQ(elem.GetEnable(), false, "Enable=0 standard request");
        }

        // Enable=1, Req=0, Rep=0: stop requests/reports
        {
            MeasurementRequestElement elem;
            elem.SetEnable(true);
            elem.SetRequest(false);
            elem.SetReport(false);
            NS_TEST_EXPECT_MSG_EQ(elem.GetMeasurementRequestMode() & 0x0E,
                                  0x02,
                                  "Enable=1 Req=0 Rep=0");
        }

        // Enable=1, Req=1, Rep=0: accept requests, stop reports
        {
            MeasurementRequestElement elem;
            elem.SetEnable(true);
            elem.SetRequest(true);
            elem.SetReport(false);
            NS_TEST_EXPECT_MSG_EQ(elem.GetMeasurementRequestMode() & 0x0E,
                                  0x06,
                                  "Enable=1 Req=1 Rep=0");
        }

        // Enable=1, Req=0, Rep=1: stop requests, accept reports
        {
            MeasurementRequestElement elem;
            elem.SetEnable(true);
            elem.SetRequest(false);
            elem.SetReport(true);
            NS_TEST_EXPECT_MSG_EQ(elem.GetMeasurementRequestMode() & 0x0E,
                                  0x0A,
                                  "Enable=1 Req=0 Rep=1");
        }

        // Enable=1, Req=1, Rep=1: accept both
        {
            MeasurementRequestElement elem;
            elem.SetEnable(true);
            elem.SetRequest(true);
            elem.SetReport(true);
            NS_TEST_EXPECT_MSG_EQ(elem.GetMeasurementRequestMode() & 0x0E,
                                  0x0E,
                                  "Enable=1 Req=1 Rep=1");
        }
    }

    // Test 6: Mode bitmap survives serialization round-trip
    {
        MeasurementRequestElement elem;
        elem.SetMeasurementToken(1);
        elem.SetMeasurementType(MeasurementType::CHANNEL_LOAD);
        MeasurementRequestElement::ChannelLoadRequestBody body;
        body.operatingClass = 81;
        body.channelNumber = 6;
        body.randomizationInterval = 100;
        body.measurementDuration = 200;
        elem.SetBody(body);
        elem.SetParallel(true);
        elem.SetDurationMandatory(true);

        Buffer buf;
        buf.AddAtStart(elem.GetSerializedSize());
        elem.Serialize(buf.Begin());

        MeasurementRequestElement deserialized;
        deserialized.Deserialize(buf.Begin());

        NS_TEST_EXPECT_MSG_EQ(deserialized.GetParallel(), true, "Parallel survives serde");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetEnable(), false, "Enable false survives serde");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetRequest(), false, "Request false survives serde");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetReport(), false, "Report false survives serde");
        NS_TEST_EXPECT_MSG_EQ(deserialized.GetDurationMandatory(),
                              true,
                              "DurationMandatory survives serde");
    }
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test optional subelements of the Measurement Request element
 * (IEEE 802.11-2024 Section 9.4.2.19)
 */
class MeasurementRequestSubelementsTest : public HeaderSerializationTestCase
{
  public:
    MeasurementRequestSubelementsTest();

  private:
    void DoRun() override;
};

MeasurementRequestSubelementsTest::MeasurementRequestSubelementsTest()
    : HeaderSerializationTestCase(
          "Check serialization and deserialization of Measurement Request subelements")
{
}

void
MeasurementRequestSubelementsTest::DoRun()
{
    using ChannelLoadBody = MeasurementRequestElement::ChannelLoadRequestBody;
    using NoiseHistogramBody = MeasurementRequestElement::NoiseHistogramRequestBody;
    using BeaconBody = MeasurementRequestElement::BeaconRequestBody;
    using LciBody = MeasurementRequestElement::LciRequestBody;
    using FtmBody = MeasurementRequestElement::FtmRangeRequestBody;
    using ApChannelReport = MeasurementRequestElement::ApChannelReport;
    using MeasurementType = MeasurementRequestElement::MeasurementType;

    // Test 1: Channel Load with Reporting subelement (ID 1)
    {
        MeasurementRequestElement elem;
        elem.SetMeasurementToken(1);
        elem.SetMeasurementType(MeasurementType::CHANNEL_LOAD);
        ChannelLoadBody body;
        body.operatingClass = 81;
        body.channelNumber = 6;
        body.randomizationInterval = 100;
        body.measurementDuration = 200;
        body.channelLoadReporting = MeasurementRequestElement::ChannelLoadReporting{1, 128};
        elem.SetBody(body);

        TestHeaderSerialization(elem);

        Buffer buf;
        buf.AddAtStart(elem.GetSerializedSize());
        elem.Serialize(buf.Begin());

        MeasurementRequestElement deserialized;
        deserialized.Deserialize(buf.Begin());

        auto& b = deserialized.GetBody<ChannelLoadBody>();
        NS_TEST_ASSERT_MSG_EQ(b.channelLoadReporting.has_value(), true, "CL Reporting present");
        NS_TEST_ASSERT_MSG_EQ(b.channelLoadReporting->reportingCondition,
                              1,
                              "CL Reporting condition");
        NS_TEST_ASSERT_MSG_EQ(b.channelLoadReporting->channelLoadReferenceValue,
                              128,
                              "CL Reporting reference value");
    }

    // Test 2: Beacon with SSID subelement (ID 0)
    {
        MeasurementRequestElement elem;
        elem.SetMeasurementToken(2);
        elem.SetMeasurementType(MeasurementType::BEACON);
        BeaconBody body;
        body.operatingClass = 81;
        body.channelNumber = 6;
        body.randomizationInterval = 100;
        body.measurementDuration = 200;
        body.measurementMode = 1;
        body.bssid = Mac48Address("ff:ff:ff:ff:ff:ff");
        body.ssid = Ssid("TestNetwork");
        elem.SetBody(body);

        TestHeaderSerialization(elem);

        Buffer buf;
        buf.AddAtStart(elem.GetSerializedSize());
        elem.Serialize(buf.Begin());

        MeasurementRequestElement deserialized;
        deserialized.Deserialize(buf.Begin());

        auto& b = deserialized.GetBody<BeaconBody>();
        NS_TEST_ASSERT_MSG_EQ(b.ssid.has_value(), true, "Beacon SSID present");
        NS_TEST_ASSERT_MSG_EQ(b.ssid->IsEqual(Ssid("TestNetwork")), true, "Beacon SSID value");
    }

    // Test 3: Beacon with Reporting Detail subelement (ID 2)
    {
        MeasurementRequestElement elem;
        elem.SetMeasurementToken(3);
        elem.SetMeasurementType(MeasurementType::BEACON);
        BeaconBody body;
        body.operatingClass = 81;
        body.channelNumber = 6;
        body.randomizationInterval = 100;
        body.measurementDuration = 200;
        body.bssid = Mac48Address("ff:ff:ff:ff:ff:ff");
        body.reportingDetail = 2;
        elem.SetBody(body);

        TestHeaderSerialization(elem);

        Buffer buf;
        buf.AddAtStart(elem.GetSerializedSize());
        elem.Serialize(buf.Begin());

        MeasurementRequestElement deserialized;
        deserialized.Deserialize(buf.Begin());

        auto& b = deserialized.GetBody<BeaconBody>();
        NS_TEST_ASSERT_MSG_EQ(b.reportingDetail.has_value(), true, "Reporting Detail present");
        NS_TEST_ASSERT_MSG_EQ(*b.reportingDetail, 2, "Reporting Detail value");
    }

    // Test 4: Beacon with AP Channel Report subelement (ID 51)
    {
        MeasurementRequestElement elem;
        elem.SetMeasurementToken(4);
        elem.SetMeasurementType(MeasurementType::BEACON);
        BeaconBody body;
        body.operatingClass = 81;
        body.channelNumber = 6;
        body.randomizationInterval = 100;
        body.measurementDuration = 200;
        body.bssid = Mac48Address("ff:ff:ff:ff:ff:ff");
        body.apChannelReports.push_back(ApChannelReport{115, {36, 40, 44, 48}});
        elem.SetBody(body);

        TestHeaderSerialization(elem);

        Buffer buf;
        buf.AddAtStart(elem.GetSerializedSize());
        elem.Serialize(buf.Begin());

        MeasurementRequestElement deserialized;
        deserialized.Deserialize(buf.Begin());

        auto& b = deserialized.GetBody<BeaconBody>();
        NS_TEST_ASSERT_MSG_EQ(b.apChannelReports.empty(), false, "AP Channel Report present");
        NS_TEST_ASSERT_MSG_EQ(b.apChannelReports[0].operatingClass,
                              115,
                              "AP Channel Report opclass");
        NS_TEST_ASSERT_MSG_EQ(b.apChannelReports[0].channelList.size(),
                              4,
                              "AP Channel Report channel count");
    }

    // Test 5: LCI with Azimuth Request subelement (ID 1)
    {
        MeasurementRequestElement elem;
        elem.SetMeasurementToken(5);
        elem.SetMeasurementType(MeasurementType::LCI);
        LciBody body;
        body.locationSubject = 1;
        body.azimuthRequest = MeasurementRequestElement::AzimuthRequest{9, 1};
        elem.SetBody(body);

        TestHeaderSerialization(elem);

        Buffer buf;
        buf.AddAtStart(elem.GetSerializedSize());
        elem.Serialize(buf.Begin());

        MeasurementRequestElement deserialized;
        deserialized.Deserialize(buf.Begin());

        auto& b = deserialized.GetBody<LciBody>();
        NS_TEST_ASSERT_MSG_EQ(b.azimuthRequest.has_value(), true, "Azimuth Request present");
        NS_TEST_ASSERT_MSG_EQ(b.azimuthRequest->azimuthResolution, 9, "Azimuth resolution");
        NS_TEST_ASSERT_MSG_EQ(b.azimuthRequest->azimuthType, 1, "Azimuth type");
    }

    // Test 6: FTM Range with Neighbor Report subelement (ID 52)
    {
        MeasurementRequestElement elem;
        elem.SetMeasurementToken(6);
        elem.SetMeasurementType(MeasurementType::FTM_RANGE);
        FtmBody body;
        body.randomizationInterval = 100;
        body.minimumApCount = 1;
        NeighborReportElement nre;
        nre.SetBssid(Mac48Address("aa:bb:cc:dd:ee:ff"));
        nre.SetOperatingClass(115);
        nre.SetChannelNumber(36);
        nre.SetPhyType(8);
        body.neighborReports.push_back(nre);
        elem.SetBody(body);

        TestHeaderSerialization(elem);

        Buffer buf;
        buf.AddAtStart(elem.GetSerializedSize());
        elem.Serialize(buf.Begin());

        MeasurementRequestElement deserialized;
        deserialized.Deserialize(buf.Begin());

        auto& b = deserialized.GetBody<FtmBody>();
        NS_TEST_ASSERT_MSG_EQ(b.neighborReports.size(), 1, "FTM Range neighbor count");
        NS_TEST_ASSERT_MSG_EQ(b.neighborReports[0].GetBssid(),
                              Mac48Address("aa:bb:cc:dd:ee:ff"),
                              "FTM Range neighbor BSSID");
    }

    // Test 7: Multiple subelements on a single request
    {
        MeasurementRequestElement elem;
        elem.SetMeasurementToken(7);
        elem.SetMeasurementType(MeasurementType::BEACON);
        BeaconBody body;
        body.operatingClass = 81;
        body.channelNumber = 6;
        body.randomizationInterval = 100;
        body.measurementDuration = 200;
        body.measurementMode = 1;
        body.bssid = Mac48Address("ff:ff:ff:ff:ff:ff");
        body.ssid = Ssid("MultiTest");
        body.reportingDetail = 1;
        body.apChannelReports.push_back(ApChannelReport{81, {1, 6, 11}});
        elem.SetBody(body);

        TestHeaderSerialization(elem);

        Buffer buf;
        buf.AddAtStart(elem.GetSerializedSize());
        elem.Serialize(buf.Begin());

        MeasurementRequestElement deserialized;
        deserialized.Deserialize(buf.Begin());

        auto& b = deserialized.GetBody<BeaconBody>();
        NS_TEST_ASSERT_MSG_EQ(b.ssid.has_value(), true, "SSID survives multi-subelement");
        NS_TEST_ASSERT_MSG_EQ(b.reportingDetail.has_value(),
                              true,
                              "ReportingDetail survives multi-subelement");
        NS_TEST_ASSERT_MSG_EQ(b.apChannelReports.empty(),
                              false,
                              "AP Channel Report survives multi-subelement");
    }

    // Test 8: Size changes correctly when subelements added
    {
        MeasurementRequestElement base;
        base.SetMeasurementToken(1);
        base.SetMeasurementType(MeasurementType::CHANNEL_LOAD);
        ChannelLoadBody baseBody;
        baseBody.operatingClass = 81;
        baseBody.channelNumber = 6;
        baseBody.randomizationInterval = 100;
        baseBody.measurementDuration = 200;
        base.SetBody(baseBody);

        uint32_t baseSize = base.GetSerializedSize();

        MeasurementRequestElement withSubelem;
        withSubelem.SetMeasurementToken(1);
        withSubelem.SetMeasurementType(MeasurementType::CHANNEL_LOAD);
        ChannelLoadBody subBody;
        subBody.operatingClass = 81;
        subBody.channelNumber = 6;
        subBody.randomizationInterval = 100;
        subBody.measurementDuration = 200;
        subBody.channelLoadReporting = MeasurementRequestElement::ChannelLoadReporting{1, 128};
        withSubelem.SetBody(subBody);

        uint32_t withSubelemSize = withSubelem.GetSerializedSize();

        // Reporting subelement: ID(1) + Len(1) + Condition(1) + RefValue(1) = 4
        NS_TEST_EXPECT_MSG_EQ(withSubelemSize, baseSize + 4, "Size increases by subelement size");
    }

    // Test 9: Vendor Specific subelement (ID 221) round-trip on Channel Load type
    {
        MeasurementRequestElement elem;
        elem.SetMeasurementToken(9);
        elem.SetMeasurementType(MeasurementType::CHANNEL_LOAD);
        ChannelLoadBody body;
        body.operatingClass = 81;
        body.channelNumber = 6;
        body.randomizationInterval = 100;
        body.measurementDuration = 200;
        body.vendorSpecific = std::vector<uint8_t>{0xAA, 0xBB, 0xCC};
        elem.SetBody(body);

        TestHeaderSerialization(elem);

        Buffer buf;
        buf.AddAtStart(elem.GetSerializedSize());
        elem.Serialize(buf.Begin());

        MeasurementRequestElement deserialized;
        deserialized.Deserialize(buf.Begin());

        auto& b = deserialized.GetBody<ChannelLoadBody>();
        NS_TEST_ASSERT_MSG_EQ(b.vendorSpecific.has_value(), true, "Vendor Specific present");
        NS_TEST_ASSERT_MSG_EQ(b.vendorSpecific->size(), 3, "Vendor Specific size");
        NS_TEST_ASSERT_MSG_EQ((*b.vendorSpecific)[0], 0xAA, "VS byte 0");
        NS_TEST_ASSERT_MSG_EQ((*b.vendorSpecific)[2], 0xCC, "VS byte 2");
    }

    // Test 10: Noise Histogram with Reporting subelement (ID 1)
    {
        MeasurementRequestElement elem;
        elem.SetMeasurementToken(10);
        elem.SetMeasurementType(MeasurementType::NOISE_HISTOGRAM);
        NoiseHistogramBody body;
        body.operatingClass = 115;
        body.channelNumber = 36;
        body.randomizationInterval = 100;
        body.measurementDuration = 200;
        body.noiseHistogramReporting = MeasurementRequestElement::NoiseHistogramReporting{2, 200};
        elem.SetBody(body);

        TestHeaderSerialization(elem);

        Buffer buf;
        buf.AddAtStart(elem.GetSerializedSize());
        elem.Serialize(buf.Begin());

        MeasurementRequestElement deserialized;
        deserialized.Deserialize(buf.Begin());

        auto& b = deserialized.GetBody<NoiseHistogramBody>();
        NS_TEST_ASSERT_MSG_EQ(b.noiseHistogramReporting.has_value(), true, "NH Reporting present");
        NS_TEST_ASSERT_MSG_EQ(b.noiseHistogramReporting->reportingCondition,
                              2,
                              "NH Reporting condition");
        NS_TEST_ASSERT_MSG_EQ(b.noiseHistogramReporting->anpiReferenceValue,
                              200,
                              "NH Reporting ANPI ref");
    }

    // Test 11: Beacon with Beacon Reporting subelement (ID 1)
    {
        MeasurementRequestElement elem;
        elem.SetMeasurementToken(11);
        elem.SetMeasurementType(MeasurementType::BEACON);
        BeaconBody body;
        body.operatingClass = 81;
        body.channelNumber = 6;
        body.randomizationInterval = 100;
        body.measurementDuration = 200;
        body.bssid = Mac48Address("ff:ff:ff:ff:ff:ff");
        body.beaconReporting = MeasurementRequestElement::BeaconReporting{5, 180};
        elem.SetBody(body);

        TestHeaderSerialization(elem);

        Buffer buf;
        buf.AddAtStart(elem.GetSerializedSize());
        elem.Serialize(buf.Begin());

        MeasurementRequestElement deserialized;
        deserialized.Deserialize(buf.Begin());

        auto& b = deserialized.GetBody<BeaconBody>();
        NS_TEST_ASSERT_MSG_EQ(b.beaconReporting.has_value(), true, "Beacon Reporting present");
        NS_TEST_ASSERT_MSG_EQ(b.beaconReporting->reportingCondition,
                              5,
                              "Beacon Reporting condition");
        NS_TEST_ASSERT_MSG_EQ(b.beaconReporting->thresholdOffsetReference,
                              180,
                              "Beacon Reporting threshold");
    }

    // Test 12: Multiple AP Channel Report subelements
    {
        MeasurementRequestElement elem;
        elem.SetMeasurementToken(12);
        elem.SetMeasurementType(MeasurementType::BEACON);
        BeaconBody body;
        body.operatingClass = 81;
        body.channelNumber = 6;
        body.randomizationInterval = 100;
        body.measurementDuration = 200;
        body.bssid = Mac48Address("ff:ff:ff:ff:ff:ff");
        body.apChannelReports.push_back(ApChannelReport{81, {1, 6, 11}});
        body.apChannelReports.push_back(ApChannelReport{115, {36, 40, 44, 48}});
        body.apChannelReports.push_back(ApChannelReport{124, {149, 153, 157, 161, 165}});
        elem.SetBody(body);

        TestHeaderSerialization(elem);

        Buffer buf;
        buf.AddAtStart(elem.GetSerializedSize());
        elem.Serialize(buf.Begin());

        MeasurementRequestElement deserialized;
        deserialized.Deserialize(buf.Begin());

        auto& b = deserialized.GetBody<BeaconBody>();
        NS_TEST_ASSERT_MSG_EQ(b.apChannelReports.size(), 3, "3 AP Channel Reports");

        NS_TEST_ASSERT_MSG_EQ(b.apChannelReports[0].operatingClass, 81, "APChRep 0 opclass");
        NS_TEST_ASSERT_MSG_EQ(b.apChannelReports[0].channelList.size(), 3, "APChRep 0 count");
        NS_TEST_ASSERT_MSG_EQ(b.apChannelReports[0].channelList[0], 1, "APChRep 0 ch0");
        NS_TEST_ASSERT_MSG_EQ(b.apChannelReports[0].channelList[2], 11, "APChRep 0 ch2");

        NS_TEST_ASSERT_MSG_EQ(b.apChannelReports[1].operatingClass, 115, "APChRep 1 opclass");
        NS_TEST_ASSERT_MSG_EQ(b.apChannelReports[1].channelList.size(), 4, "APChRep 1 count");

        NS_TEST_ASSERT_MSG_EQ(b.apChannelReports[2].operatingClass, 124, "APChRep 2 opclass");
        NS_TEST_ASSERT_MSG_EQ(b.apChannelReports[2].channelList.size(), 5, "APChRep 2 count");
        NS_TEST_ASSERT_MSG_EQ(b.apChannelReports[2].channelList[4], 165, "APChRep 2 ch4");
    }

    // Test 13: Multiple FTM Range Neighbor Report subelements
    {
        MeasurementRequestElement elem;
        elem.SetMeasurementToken(13);
        elem.SetMeasurementType(MeasurementType::FTM_RANGE);
        FtmBody body;
        body.randomizationInterval = 100;
        body.minimumApCount = 3;

        NeighborReportElement nre1;
        nre1.SetBssid(Mac48Address("aa:bb:cc:dd:ee:01"));
        nre1.SetOperatingClass(115);
        nre1.SetChannelNumber(36);
        nre1.SetPhyType(8);
        body.neighborReports.push_back(nre1);

        NeighborReportElement nre2;
        nre2.SetBssid(Mac48Address("aa:bb:cc:dd:ee:02"));
        nre2.SetOperatingClass(115);
        nre2.SetChannelNumber(40);
        nre2.SetPhyType(8);
        body.neighborReports.push_back(nre2);

        NeighborReportElement nre3;
        nre3.SetBssid(Mac48Address("aa:bb:cc:dd:ee:03"));
        nre3.SetOperatingClass(81);
        nre3.SetChannelNumber(6);
        nre3.SetPhyType(7);
        body.neighborReports.push_back(nre3);

        elem.SetBody(body);

        TestHeaderSerialization(elem);

        Buffer buf;
        buf.AddAtStart(elem.GetSerializedSize());
        elem.Serialize(buf.Begin());

        MeasurementRequestElement deserialized;
        deserialized.Deserialize(buf.Begin());

        auto& b = deserialized.GetBody<FtmBody>();
        NS_TEST_ASSERT_MSG_EQ(b.neighborReports.size(), 3, "3 FTM Range neighbors");
        NS_TEST_ASSERT_MSG_EQ(b.neighborReports[0].GetBssid(),
                              Mac48Address("aa:bb:cc:dd:ee:01"),
                              "FTM NRE 0 BSSID");
        NS_TEST_ASSERT_MSG_EQ(b.neighborReports[1].GetBssid(),
                              Mac48Address("aa:bb:cc:dd:ee:02"),
                              "FTM NRE 1 BSSID");
        NS_TEST_ASSERT_MSG_EQ(b.neighborReports[2].GetBssid(),
                              Mac48Address("aa:bb:cc:dd:ee:03"),
                              "FTM NRE 2 BSSID");
        NS_TEST_ASSERT_MSG_EQ(b.neighborReports[2].GetChannelNumber(), 6, "FTM NRE 2 channel");
    }

    // Test 14: SetBody auto-syncs m_measurementType (validates Fix 1)
    {
        MeasurementRequestElement elem;
        elem.SetMeasurementToken(14);
        ChannelLoadBody body;
        body.operatingClass = 81;
        body.channelNumber = 6;
        body.randomizationInterval = 100;
        body.measurementDuration = 200;
        body.channelLoadReporting = MeasurementRequestElement::ChannelLoadReporting{1, 128};
        elem.SetBody(body);

        NS_TEST_EXPECT_MSG_EQ(elem.GetMeasurementType(),
                              MeasurementType::CHANNEL_LOAD,
                              "SetBody auto-sets type to 3");

        TestHeaderSerialization(elem);

        Buffer buf;
        buf.AddAtStart(elem.GetSerializedSize());
        elem.Serialize(buf.Begin());

        MeasurementRequestElement deserialized;
        deserialized.Deserialize(buf.Begin());

        NS_TEST_EXPECT_MSG_EQ(deserialized.GetMeasurementType(),
                              MeasurementType::CHANNEL_LOAD,
                              "Auto-synced type survives round-trip");
        auto& b = deserialized.GetBody<ChannelLoadBody>();
        NS_TEST_ASSERT_MSG_EQ(b.channelLoadReporting.has_value(),
                              true,
                              "CL Reporting present after auto-sync");
        NS_TEST_ASSERT_MSG_EQ(b.channelLoadReporting->channelLoadReferenceValue,
                              128,
                              "CL Reporting ref value after auto-sync");
    }

    // Test 15: SetBody auto-syncs for all non-Basic types
    {
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(1);
            elem.SetBody(NoiseHistogramBody{});
            NS_TEST_EXPECT_MSG_EQ(elem.GetMeasurementType(),
                                  MeasurementType::NOISE_HISTOGRAM,
                                  "SetBody auto-sets NH type");
            TestHeaderSerialization(elem);
        }
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(1);
            BeaconBody body;
            body.bssid = Mac48Address("ff:ff:ff:ff:ff:ff");
            elem.SetBody(body);
            NS_TEST_EXPECT_MSG_EQ(elem.GetMeasurementType(),
                                  MeasurementType::BEACON,
                                  "SetBody auto-sets Beacon type");
            TestHeaderSerialization(elem);
        }
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(1);
            LciBody body;
            body.locationSubject = 1;
            body.azimuthRequest = MeasurementRequestElement::AzimuthRequest{9, 1};
            elem.SetBody(body);
            NS_TEST_EXPECT_MSG_EQ(elem.GetMeasurementType(),
                                  MeasurementType::LCI,
                                  "SetBody auto-sets LCI type");
            TestHeaderSerialization(elem);
        }
        {
            MeasurementRequestElement elem;
            elem.SetMeasurementToken(1);
            FtmBody body;
            body.randomizationInterval = 100;
            body.minimumApCount = 1;
            NeighborReportElement nre;
            nre.SetBssid(Mac48Address("aa:bb:cc:dd:ee:ff"));
            nre.SetOperatingClass(115);
            nre.SetChannelNumber(36);
            nre.SetPhyType(8);
            body.neighborReports.push_back(nre);
            elem.SetBody(body);
            NS_TEST_EXPECT_MSG_EQ(elem.GetMeasurementType(),
                                  MeasurementType::FTM_RANGE,
                                  "SetBody auto-sets FTM type");
            TestHeaderSerialization(elem);
        }
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
    AddTestCase(new TpcReportElementTest, TestCase::Duration::QUICK);
    AddTestCase(new RmEnabledCapabilitiesTest, TestCase::Duration::QUICK);
    AddTestCase(new MeasurementRequestElementTest, TestCase::Duration::QUICK);
    AddTestCase(new MeasurementRequestModeTest, TestCase::Duration::QUICK);
    AddTestCase(new MeasurementRequestSubelementsTest, TestCase::Duration::QUICK);
}

static WifiRrmInfoElemsTestSuite g_wifiRrmInfoElemsTestSuite; ///< the test suite
