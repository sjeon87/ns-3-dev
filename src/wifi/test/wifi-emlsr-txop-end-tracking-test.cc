/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Davide Magrin <davide@magr.in>
 */

#include "ns3/ap-wifi-mac.h"
#include "ns3/attribute-container.h"
#include "ns3/boolean.h"
#include "ns3/eht-frame-exchange-manager.h"
#include "ns3/mac48-address.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/node-container.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/uinteger.h"
#include "ns3/wifi-mac-helper.h"
#include "ns3/wifi-mac-queue-scheduler.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-static-setup-helper.h"
#include "ns3/wifi-utils.h"

#include <optional>

using namespace ns3;

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test that a new EMLSR TXOP finalizes the tracking of the previous TXOP.
 *
 * With a non-zero TXOP limit, the AP keeps a pending TXOP end event for a short window
 * after each frame exchange, waiting to see whether the TXOP holder continues its TXOP.
 * Frames belonging to the tracked TXOP (a frame sent by the TXOP holder, a frame carried
 * in a TB PPDU) must leave the event pending; a TXOP-opening frame from a different
 * EMLSR client means the tracked TXOP is over: the pending event must be finalized (the
 * previous holder switches back to listening operation, so the blocking of its other
 * links ends) and the start-of-TXOP processing must run for the new holder (its other
 * links become blocked).
 *
 * The AP MLD (two links) is set up statically with two EMLSR clients. The received
 * frames are passed directly to EhtFrameExchangeManager::CheckEmlsrClientStartingTxop()
 * and the pending TXOP end event is scheduled directly (this test is a friend of
 * EhtFrameExchangeManager); the checks then inspect the per-link queue masks of the two
 * clients at the AP MLD.
 */
class EmlsrStaleTxopEndTest : public TestCase
{
  public:
    EmlsrStaleTxopEndTest();

  private:
    void DoSetup() override;
    void DoRun() override;

    /// Drive the sequence: client 1 starts a TXOP that terminates while its TXOP end
    /// event is still pending; frames belonging to that TXOP do not finalize the event;
    /// a TXOP-opening frame from client 2 does
    void CheckTxopTakeover();

    /**
     * Create an RTS header sent by the given station to the AP.
     *
     * @param sender the link MAC address of the sender
     * @return the RTS header
     */
    WifiMacHeader GetRtsFrom(Mac48Address sender) const;

    /**
     * Check whether transmissions to the given client are blocked for the given reason
     * on the given link of the AP MLD.
     *
     * @param mldAddress the MLD address of the client
     * @param linkId the ID of the given link
     * @param reason the given block reason
     * @return whether transmissions are blocked
     */
    bool IsBlocked(Mac48Address mldAddress, uint8_t linkId, WifiQueueBlockedReason reason) const;

    /**
     * Create a WifiNetDevice with two links operating on the given spectrum channels.
     *
     * @param isAp whether the device is the AP MLD
     * @param channels the spectrum channel of each link
     * @return the created device
     */
    Ptr<WifiNetDevice> GetWifiNetDevice(
        bool isAp,
        const std::vector<Ptr<MultiModelSpectrumChannel>>& channels);

    Ptr<WifiNetDevice> m_apDev;                   ///< AP MLD device
    std::vector<Ptr<WifiNetDevice>> m_clientDevs; ///< EMLSR client devices
};

EmlsrStaleTxopEndTest::EmlsrStaleTxopEndTest()
    : TestCase("Check that a new EMLSR TXOP finalizes the tracking of the previous TXOP")
{
}

Ptr<WifiNetDevice>
EmlsrStaleTxopEndTest::GetWifiNetDevice(bool isAp,
                                        const std::vector<Ptr<MultiModelSpectrumChannel>>& channels)
{
    NodeContainer node(1);

    SpectrumWifiPhyHelper phy(2);
    phy.Set(0, "ChannelSettings", StringValue("{42, 80, BAND_5GHZ, 0}"));
    phy.Set(1, "ChannelSettings", StringValue("{23, 80, BAND_6GHZ, 0}"));
    phy.AddPhyToFreqRangeMapping(0, WIFI_SPECTRUM_5_GHZ);
    phy.AddPhyToFreqRangeMapping(1, WIFI_SPECTRUM_6_GHZ);
    phy.AddChannel(channels.at(0), WIFI_SPECTRUM_5_GHZ);
    phy.AddChannel(channels.at(1), WIFI_SPECTRUM_6_GHZ);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211be);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("HeMcs3"),
                                 "ControlMode",
                                 StringValue("OfdmRate24Mbps"));
    wifi.ConfigEhtOptions("EmlsrActivated", BooleanValue(true));

    WifiMacHelper mac;
    Ssid ssid("emlsr-txop-end-test");
    if (isAp)
    {
        mac.SetType("ns3::ApWifiMac",
                    "Ssid",
                    SsidValue(ssid),
                    "BeaconGeneration",
                    BooleanValue(false));
    }
    else
    {
        mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
        mac.SetEmlsrManager("ns3::DefaultEmlsrManager",
                            "EmlsrLinkSet",
                            AttributeContainerValue<UintegerValue>(std::set<uint8_t>{0, 1}),
                            "EmlsrTransitionDelay",
                            TimeValue(MicroSeconds(64)));
    }

    auto devices = wifi.Install(phy, mac, node);
    return DynamicCast<WifiNetDevice>(devices.Get(0));
}

void
EmlsrStaleTxopEndTest::DoSetup()
{
    std::vector<Ptr<MultiModelSpectrumChannel>> channels{CreateObject<MultiModelSpectrumChannel>(),
                                                         CreateObject<MultiModelSpectrumChannel>()};

    m_apDev = GetWifiNetDevice(true, channels);
    m_clientDevs.push_back(GetWifiNetDevice(false, channels));
    m_clientDevs.push_back(GetWifiNetDevice(false, channels));

    for (const auto& clientDev : m_clientDevs)
    {
        WifiStaticSetupHelper::SetStaticAssociation(m_apDev, clientDev);
        WifiStaticSetupHelper::SetStaticEmlsr(m_apDev, clientDev);
    }
}

WifiMacHeader
EmlsrStaleTxopEndTest::GetRtsFrom(Mac48Address sender) const
{
    WifiMacHeader rts(WIFI_MAC_CTL_RTS);
    rts.SetAddr1(m_apDev->GetMac()->GetFrameExchangeManager(0)->GetAddress());
    rts.SetAddr2(sender);
    rts.SetDsNotFrom();
    rts.SetDsNotTo();
    rts.SetDuration(MicroSeconds(100));
    return rts;
}

bool
EmlsrStaleTxopEndTest::IsBlocked(Mac48Address mldAddress,
                                 uint8_t linkId,
                                 WifiQueueBlockedReason reason) const
{
    const auto queueId = MakeWifiUnicastQueueId(WIFI_QOSDATA_QUEUE, mldAddress, 0);
    const auto mask =
        m_apDev->GetMac()->GetMacQueueScheduler()->GetQueueLinkMask(AC_BE, queueId, linkId);
    // a missing mask means the queue was never blocked
    return mask.has_value() && mask->test(static_cast<std::size_t>(reason));
}

void
EmlsrStaleTxopEndTest::CheckTxopTakeover()
{
    auto fem = DynamicCast<EhtFrameExchangeManager>(m_apDev->GetMac()->GetFrameExchangeManager(0));
    NS_TEST_ASSERT_MSG_NE(fem, nullptr, "Expected an EHT frame exchange manager");

    const auto client1Addr = m_clientDevs.at(0)->GetMac()->GetFrameExchangeManager(0)->GetAddress();
    const auto client2Addr = m_clientDevs.at(1)->GetMac()->GetFrameExchangeManager(0)->GetAddress();
    const auto client1Mld = m_clientDevs.at(0)->GetMac()->GetAddress();
    const auto client2Mld = m_clientDevs.at(1)->GetMac()->GetAddress();
    const auto txVector =
        m_apDev->GetRemoteStationManager(0)->GetRtsTxVector(client1Addr, MHz_u{20});

    // client 1 starts an UL TXOP on link 0: its other EMLSR link gets blocked
    NS_TEST_ASSERT_MSG_EQ(fem->CheckEmlsrClientStartingTxop(GetRtsFrom(client1Addr), txVector),
                          true,
                          "The frame from client 1 must start a TXOP");
    NS_TEST_ASSERT_MSG_EQ(IsBlocked(client1Mld, 1, WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK),
                          true,
                          "Transmissions to client 1 on link 1 must be blocked during its TXOP");

    // the TXOP end event stays pending after each frame exchange of the TXOP
    fem->m_ongoingTxopEnd = Simulator::Schedule(MilliSeconds(1),
                                                &EhtFrameExchangeManager::TxopEnd,
                                                fem,
                                                std::optional<Mac48Address>(client1Addr));

    // frames belonging to the tracked TXOP do not finalize the pending event: a frame
    // sent by the TXOP holder continues its TXOP...
    fem->m_txopHolder = client1Addr;
    NS_TEST_ASSERT_MSG_EQ(fem->CheckEmlsrClientStartingTxop(GetRtsFrom(client1Addr), txVector),
                          false,
                          "A frame from the TXOP holder must not start a new TXOP");
    NS_TEST_ASSERT_MSG_EQ(fem->m_ongoingTxopEnd.IsPending(),
                          true,
                          "A frame from the TXOP holder must not finalize the TXOP end event");

    // ... and a frame carried in a TB PPDU is a solicited response, whatever the sender
    auto tbTxVector = txVector;
    tbTxVector.SetPreambleType(WIFI_PREAMBLE_HE_TB);
    WifiMacHeader qosData(WIFI_MAC_QOSDATA);
    qosData.SetAddr1(m_apDev->GetMac()->GetFrameExchangeManager(0)->GetAddress());
    qosData.SetAddr2(client2Addr);
    qosData.SetDsTo();
    NS_TEST_ASSERT_MSG_EQ(fem->CheckEmlsrClientStartingTxop(qosData, tbTxVector),
                          false,
                          "A frame carried in a TB PPDU must not start a new TXOP");
    NS_TEST_ASSERT_MSG_EQ(fem->m_ongoingTxopEnd.IsPending(),
                          true,
                          "A frame carried in a TB PPDU must not finalize the TXOP end event");

    // client 1's TXOP has terminated, but the AP MLD does not know it yet; the saved TXOP
    // holder has been cleared (the intra-BSS NAV is not set by frames addressed to the AP)
    fem->m_txopHolder.reset();

    // a TXOP-opening frame from client 2 is received in the tracking window: the previous
    // TXOP must be finalized and the new TXOP must be processed
    NS_TEST_ASSERT_MSG_EQ(fem->CheckEmlsrClientStartingTxop(GetRtsFrom(client2Addr), txVector),
                          true,
                          "The frame from client 2 must start a TXOP");
    NS_TEST_ASSERT_MSG_EQ(fem->m_ongoingTxopEnd.IsPending(),
                          false,
                          "The TXOP end event of the previous TXOP must have been finalized");

    // start-of-TXOP processing for the new holder
    NS_TEST_ASSERT_MSG_EQ(IsBlocked(client2Mld, 1, WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK),
                          true,
                          "Transmissions to client 2 on link 1 must be blocked during its TXOP");
    NS_TEST_ASSERT_MSG_EQ(IsBlocked(client2Mld, 0, WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK),
                          false,
                          "Transmissions to client 2 on link 0 must not be blocked");

    // the previous holder is switched back to listening operation: the blocking of its
    // other links ends and the transition delay starts
    for (uint8_t linkId = 0; linkId < 2; ++linkId)
    {
        NS_TEST_ASSERT_MSG_EQ(
            IsBlocked(client1Mld, linkId, WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK),
            false,
            "Client 1 must not be blocked as using another link after its TXOP ended (link "
                << +linkId << ")");
        NS_TEST_ASSERT_MSG_EQ(
            IsBlocked(client1Mld, linkId, WifiQueueBlockedReason::WAITING_EMLSR_TRANSITION_DELAY),
            true,
            "Client 1 must be waiting for the transition delay after its TXOP ended (link "
                << +linkId << ")");
    }

    // after the 64 us transition delay, client 1 is fully unblocked while client 2's TXOP
    // blocking persists
    Simulator::Schedule(MicroSeconds(65), [=, this]() {
        for (uint8_t linkId = 0; linkId < 2; ++linkId)
        {
            NS_TEST_ASSERT_MSG_EQ(
                IsBlocked(client1Mld,
                          linkId,
                          WifiQueueBlockedReason::WAITING_EMLSR_TRANSITION_DELAY),
                false,
                "Client 1 must not be blocked after the transition delay elapsed (link " << +linkId
                                                                                         << ")");
        }
        NS_TEST_ASSERT_MSG_EQ(
            IsBlocked(client2Mld, 1, WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK),
            true,
            "Transmissions to client 2 on link 1 must still be blocked");
    });
}

void
EmlsrStaleTxopEndTest::DoRun()
{
    Simulator::Schedule(MilliSeconds(1), &EmlsrStaleTxopEndTest::CheckTxopTakeover, this);

    Simulator::Stop(MilliSeconds(3));
    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief wifi EMLSR TXOP end tracking Test Suite
 */
class WifiEmlsrTxopEndTrackingTestSuite : public TestSuite
{
  public:
    WifiEmlsrTxopEndTrackingTestSuite();
};

WifiEmlsrTxopEndTrackingTestSuite::WifiEmlsrTxopEndTrackingTestSuite()
    : TestSuite("wifi-emlsr-txop-end-tracking", Type::UNIT)
{
    AddTestCase(new EmlsrStaleTxopEndTest, TestCase::Duration::QUICK);
}

static WifiEmlsrTxopEndTrackingTestSuite g_wifiEmlsrTxopEndTrackingTestSuite; ///< the test suite
