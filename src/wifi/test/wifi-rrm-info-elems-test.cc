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
}

static WifiRrmInfoElemsTestSuite g_wifiRrmInfoElemsTestSuite; ///< the test suite
