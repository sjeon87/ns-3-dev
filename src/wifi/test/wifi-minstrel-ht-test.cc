/*
 * Copyright (c) 2023 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/ap-wifi-mac.h"
#include "ns3/config.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/frame-exchange-manager.h"
#include "ns3/log.h"
#include "ns3/minstrel-ht-wifi-manager.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/wifi-net-device.h"
#include "ns3/yans-wifi-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiMinstrelHtTest");

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief MinstrelHt retry chain test
 */
class RetryChainTest : public TestCase
{
  public:
    /**
     * Constructor
     *
     * @param rtsThreshold The value used to determine if RTS/CTS is in use
     * @param maxMpdus the number of MPDUs to send; if greater than 0 use A-MPDU
     * @param sampling flag to signal that MinstrelHt is expected to be sampling
     * @param dropTimeRss vector that carries the RSS and Time for them to be set
     */
    RetryChainTest(int rtsThreshold,
                   int maxMpdus,
                   bool sampling,
                   std::vector<std::pair<Time, int>> dropTimeRss);
    void DoRun() override;

  private:
    void CheckResults(int rtsThreshold, int maxMpdus, bool sampling);
    void DoTeardown() override;
    void SetRss(Ptr<FixedRssLossModel> rssModel, double rss);
    // Callbacks
    void RetryChainChange(std::string context, RetryChainInfo retryChain);
    void SampleRateChange(std::string context, WifiTxVector sampleRate);
    void TxCallback(std::string context,
                    WifiConstPsduMap psdus,
                    WifiTxVector txVector,
                    double txPowerW);
    void Transmit(std::string context, Ptr<const Packet> p, double power);

    int m_rtsThreshold;
    int m_maxMpdus;  // Number of MPDUs inside A-MPDU
    Time m_dropTime; // Sets the time the drop in SNR occurs
    bool m_sampling; // Flag set when MinstrelHt is expected to use a sample rate as first Tx
    WifiTxVector m_sampleRate; // Rate used by MinstrelHt to sample

    std::vector<std::pair<Time, int>> m_dropTimeRss; // Time and RSS value to set
    std::vector<Time> m_segmentTime; // Used to calculate how much time the retry chain takes

    // Provide definition of less to be able to use WifiTxVector as key to map
    struct cmp
    {
        bool operator()(const WifiTxVector& c1, const WifiTxVector& c2) const
        {
            return (
                c1.GetMode().GetDataRate(c1.GetChannelWidth(), c1.GetGuardInterval(), c1.GetNss()) <
                c2.GetMode().GetDataRate(c2.GetChannelWidth(), c2.GetGuardInterval(), c2.GetNss()));
        }
    };

    std::map<WifiTxVector, int, cmp>
        m_expectedResults; // Expected results extracted from statistics in MinstrelHt
    std::map<WifiTxVector, int, cmp> m_results; // Observed results
    bool m_rssSet{false};
};

RetryChainTest::RetryChainTest(int rtsThreshold,
                               int maxMpdus,
                               bool sampling,
                               std::vector<std::pair<Time, int>> dropTimeRss)
    : TestCase("Check the correctness of the MinstrelHt retry chain"),
      m_rtsThreshold(rtsThreshold),
      m_maxMpdus(maxMpdus),
      m_sampling(sampling),
      m_dropTimeRss(dropTimeRss)
{
}

void
RetryChainTest::DoTeardown()
{
}

void
RetryChainTest::CheckResults(int rtsThreshold, int maxMpdus, bool sampling)
{
    // int totalRetransmissions = 0;
    if (!m_sampling)
    {
        for (const auto& it : m_expectedResults)
        {
            NS_LOG_INFO("Rate expected: " << it.first.GetMode() << " Retry count: " << it.second);
        }
        for (const auto& it : m_results)
        {
            NS_LOG_INFO("Rate used: " << it.first.GetMode() << " Retry count: " << it.second);
            if (m_expectedResults.find(it.first) == m_expectedResults.end())
            {
                NS_TEST_ASSERT_MSG_EQ(1, 0, "Used an unexpected rate during retry chain");
            }
            else
            {
                NS_TEST_ASSERT_MSG_EQ(
                    it.second,
                    m_expectedResults[it.first],
                    "Retried a rate an unexpected amount of times during retry chain");
                // totalRetransmissions += it.second;
            }
        }
    }
    else
    {
        m_expectedResults[m_sampleRate] = 1;
        for (const auto& it : m_expectedResults)
        {
            NS_LOG_INFO("Rate expected: " << it.first.GetMode() << " Retry count: " << it.second);
        }
        for (const auto& it : m_results)
        {
            NS_LOG_INFO("Rate used: " << it.first.GetMode() << " Retry count: " << it.second);
            if (m_expectedResults.find(it.first) == m_expectedResults.end())
            {
                NS_TEST_ASSERT_MSG_EQ(1, 0, "Used an unexpected rate during retry chain");
            }
            else
            {
                NS_TEST_ASSERT_MSG_EQ(
                    it.second,
                    m_expectedResults[it.first],
                    "Retried a rate an unexpected amount of times during retry chain");
                // totalRetransmissions += it.second;
            }
        }
    }

    // // Check if retransmissions occurred within segment of 26 ms
    // Time segmentSize = m_segmentTime[m_segmentTime.size() - 1] - m_segmentTime[0];
    // NS_TEST_ASSERT_MSG_LT(segmentSize,
    //                       MilliSeconds(26),
    //                       "Full retry chain takes longer than 26 ms");

    // // Check if the total amount of retransmissions is equal to the amount allowed
    // NS_TEST_ASSERT_MSG_EQ(totalRetransmissions, 8, "Obtained expected m_results");
}

/**
 * Report Retry Chain changed.
 *
 * @param retryChain structure to carry rates used in retry chain and their max allowed
 * retransmission count
 */
void
RetryChainTest::RetryChainChange(std::string context, RetryChainInfo retryChain)
{
    if (!m_rssSet)
    {
        m_expectedResults.clear();

        if (!m_sampling)
        {
            NS_LOG_DEBUG("Best rate: " << retryChain.m_maxTp.GetMode() << " # of retries "
                                       << retryChain.m_maxTpCount
                                       << "; Best rate2: " << retryChain.m_maxTp2.GetMode()
                                       << " # of retries " << retryChain.m_maxTp2Count
                                       << "; Best prob: " << retryChain.m_maxProb.GetMode()
                                       << " # of retries " << retryChain.m_maxProbCount);
            m_expectedResults[retryChain.m_maxTp] += retryChain.m_maxTpCount;
            m_expectedResults[retryChain.m_maxTp2] += retryChain.m_maxTp2Count;
            m_expectedResults[retryChain.m_maxProb] += retryChain.m_maxProbCount;
        }
        // When sampling the expected retry chain follows: Sample rate, Best TP and Best Prob
        else
        {
            NS_LOG_DEBUG("Best rate: " << retryChain.m_maxTp.GetMode() << " # of retries "
                                       << retryChain.m_maxTpCountSampling
                                       << "; Best rate2: " << retryChain.m_maxTp2.GetMode()
                                       << " # of retries " << retryChain.m_maxTp2Count
                                       << "; Best prob: " << retryChain.m_maxProb.GetMode()
                                       << " # of retries " << retryChain.m_maxProbCount);
            m_expectedResults[retryChain.m_maxTp] += retryChain.m_maxTpCountSampling;
            m_expectedResults[retryChain.m_maxProb] += retryChain.m_maxProbCount;
        }
    }
}

/**
 * Report Rate changed.
 *
 * @param oldVal Old value.
 * @param newVal New value.
 */
void
RetryChainTest::SampleRateChange(std::string context, WifiTxVector sampleRate)
{
    NS_LOG_DEBUG("Sample rate: " << sampleRate.GetMode());
    m_sampleRate = sampleRate;
}

/**
 *  Set the receive signal strength
 */
void
RetryChainTest::SetRss(Ptr<FixedRssLossModel> rssModel, double rss)
{
    NS_LOG_DEBUG("Changed RSS to " << rss);
    rssModel->SetRss(rss);
    m_rssSet = true;
}

/**
 * Callback when a frame is transmitted.
 * @param rxErrorModel the post reception error model on the receiver
 * @param context the context
 * @param psduMap the PSDU map
 * @param txVector the TX vector
 * @param txPowerW the tx power in Watts
 */
void
RetryChainTest::TxCallback(std::string context,
                           WifiConstPsduMap psdus,
                           WifiTxVector txVector,
                           double txPowerW)
{
    if (psdus.begin()->second->GetSize() >= 1000)
    {
        if (m_rssSet)
        {
            m_results[txVector] += 1;
            NS_LOG_INFO("Transmission with WifiMode " << txVector.GetMode());
        }
    }
}

/**
 * Callback invoked when PHY transmits a packet
 * @param context the context
 * @param p the packet
 * @param power the tx power
 */
void
RetryChainTest::Transmit(std::string context, Ptr<const Packet> p, double power)
{
    WifiMacHeader hdr;
    p->PeekHeader(hdr);

    if (m_rssSet)
    {
        if (hdr.IsQosData())
        {
            m_segmentTime.emplace_back(Simulator::Now());
        }
    }
}

void
RetryChainTest::DoRun()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 100;
    NS_LOG_DEBUG("RTS threshold " << m_rtsThreshold << " Mpdus " << m_maxMpdus
                                  << " Sampling: " << m_sampling);

    NodeContainer wifiStaNode;
    wifiStaNode.Create(1);

    NodeContainer wifiApNode;
    wifiApNode.Create(1);

    YansWifiPhyHelper wifiPhy;

    // This is one parameter that matters when using FixedRssLossModel
    // set it to zero; otherwise, gain will be added
    wifiPhy.Set("RxGain", DoubleValue(0));

    // Force bandwidth to reduce sample space of MinstrelHt
    wifiPhy.Set("ChannelSettings", StringValue("{36, 20, BAND_5GHZ, 0}"));

    wifiPhy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ax);
    // LogComponentEnable("MinstrelHtWifiManager", LOG_LEVEL_ALL);

    Ptr<YansWifiChannel> wifiChannel = CreateObject<YansWifiChannel>();

    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    wifiChannel->SetPropagationDelayModel(delayModel);

    // Using a fixed RSS model overrides the RSS regardless of node position or TX power.
    Ptr<FixedRssLossModel> rssLossModel = CreateObject<FixedRssLossModel>();
    rssLossModel->SetRss(-55);
    wifiChannel->SetPropagationLossModel(rssLossModel);
    wifiPhy.SetChannel(wifiChannel);

    // Set MinstrelHt as rate control and enable or disable RTS/CTS.
    wifi.SetRemoteStationManager("ns3::MinstrelHtWifiManager",
                                 "RtsCtsThreshold",
                                 UintegerValue(m_rtsThreshold));

    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));

    NetDeviceContainer staDevices;
    staDevices = wifi.Install(wifiPhy, mac, wifiStaNode);

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid), "BeaconGeneration", BooleanValue(true));

    NetDeviceContainer apDevices;
    apDevices = wifi.Install(wifiPhy, mac, wifiApNode);

    // Assign fixed streams to random variables in use
    wifi.AssignStreams(apDevices, streamNumber);
    wifi.AssignStreams(staDevices, streamNumber);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    // Doesn't have any effect
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNode);

    Ptr<WifiNetDevice> ap_device = DynamicCast<WifiNetDevice>(apDevices.Get(0));
    Ptr<WifiNetDevice> sta_device = DynamicCast<WifiNetDevice>(staDevices.Get(0));

    // wifiPhy.EnablePcap("sta", sta_device);
    // wifiPhy.EnablePcap("ap", ap_device);

    // Configure AP Aggregation

    ap_device->GetMac()->SetAttribute("BE_MaxAmpduSize", UintegerValue(m_maxMpdus * (1200 + 50)));
    ap_device->GetMac()->SetAttribute("BK_MaxAmpduSize", UintegerValue(m_maxMpdus * (1200 + 50)));
    ap_device->GetMac()->SetAttribute("VO_MaxAmpduSize", UintegerValue(m_maxMpdus * (1200 + 50)));
    ap_device->GetMac()->SetAttribute("VI_MaxAmpduSize", UintegerValue(m_maxMpdus * (1200 + 50)));

    // Configure STA Aggregation

    sta_device->GetMac()->SetAttribute("BE_MaxAmpduSize", UintegerValue(m_maxMpdus * (1200 + 50)));
    sta_device->GetMac()->SetAttribute("BK_MaxAmpduSize", UintegerValue(m_maxMpdus * (1200 + 50)));
    sta_device->GetMac()->SetAttribute("VO_MaxAmpduSize", UintegerValue(m_maxMpdus * (1200 + 50)));
    sta_device->GetMac()->SetAttribute("VI_MaxAmpduSize", UintegerValue(m_maxMpdus * (1200 + 50)));

    // Connect Callbacks
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/RemoteStationManager/"
                    "$ns3::MinstrelHtWifiManager/RetryChain",
                    MakeCallback(&RetryChainTest::RetryChainChange, this));

    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/RemoteStationManager/"
                    "$ns3::MinstrelHtWifiManager/SampleRate",
                    MakeCallback(&RetryChainTest::SampleRateChange, this));

    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/PhyTxPsduBegin",
                    MakeCallback(&RetryChainTest::TxCallback, this));

    Config::Connect("/NodeList/0/DeviceList/*/Phy/PhyTxBegin",
                    MakeCallback(&RetryChainTest::Transmit, this));

    PacketSocketAddress socket;
    socket.SetSingleDevice(sta_device->GetIfIndex());
    socket.SetPhysicalAddress(ap_device->GetAddress());
    socket.SetProtocol(1);

    // give packet socket powers to nodes.
    PacketSocketHelper packetSocket;
    packetSocket.Install(wifiStaNode);
    packetSocket.Install(wifiApNode);

    // Set the initial RSS drop time
    m_dropTime = m_dropTimeRss[0].first;

    // Send 500 packets to build statistics
    Ptr<PacketSocketClient> client1 = CreateObject<PacketSocketClient>();
    client1->SetAttribute("PacketSize", UintegerValue(1200));
    client1->SetAttribute("MaxPackets", UintegerValue(500));
    client1->SetAttribute("Interval", TimeValue(MicroSeconds(100)));
    client1->SetRemote(socket);
    wifiStaNode.Get(0)->AddApplication(client1);
    client1->SetStartTime(Seconds(1));
    client1->SetStopTime(m_dropTime);

    // Send one MPDU or a A-MPDU after building statistics
    Ptr<PacketSocketClient> client2 = CreateObject<PacketSocketClient>();
    client2->SetAttribute("PacketSize", UintegerValue(1200));
    if (m_maxMpdus == 0)
    {
        client2->SetAttribute("MaxPackets", UintegerValue(1));
    }
    else
    {
        client2->SetAttribute("MaxPackets", UintegerValue(m_maxMpdus));
    }
    client2->SetAttribute("Interval", TimeValue(MicroSeconds(1)));
    client2->SetRemote(socket);
    wifiStaNode.Get(0)->AddApplication(client2);
    client2->SetStartTime(m_dropTime + Seconds(0.01));

    for (const auto& it : m_dropTimeRss)
    {
        // Lower RSS to force retranmissions and, if required, increase it after
        // to successfully send BAR
        Simulator::Schedule(it.first, &RetryChainTest::SetRss, this, rssLossModel, it.second);
    }

    Simulator::Stop(m_dropTime + Seconds(0.2));
    Simulator::Run();
    CheckResults(m_rtsThreshold, m_maxMpdus, m_sampling);
    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief MinstrelHt Test Suite
 */
class WifiMinstrelHtTestSuite : public TestSuite
{
  public:
    WifiMinstrelHtTestSuite();
};

WifiMinstrelHtTestSuite::WifiMinstrelHtTestSuite()
    : TestSuite("wifi-minstrel-ht", Type::UNIT)
{
    std::vector<std::pair<Time, int>> rssDropTimes;

    // Case 1:  S-MPDU, no RTS/CTS, not sampling
    // Force S-MPDU to be retransmitted
    rssDropTimes.emplace_back(Seconds(1.2), -90);
    // Allow BAR to be received
    rssDropTimes.emplace_back(Seconds(1.25140), -55);
    AddTestCase(new RetryChainTest(655555, 0, false, rssDropTimes), TestCase::Duration::QUICK);
    // Result: Correct Behavior; No Additional Retransmissions
    rssDropTimes.clear();

    // Case 2:  S-MPDU, no RTS/CTS, sampling
    // Force S-MPDU to be retransmitted
    rssDropTimes.emplace_back(Seconds(1.04853), -90);
    // // After this test fails
    // // Allow BAR to be received
    // rssDropTimes.emplace_back(Seconds(1.08152), -55);
    // // Seems to try another transmissions and doesn't update long retry; Force S-MPDU to be
    // // retransmitted
    // rssDropTimes.emplace_back(Seconds(1.08173), -90);
    AddTestCase(new RetryChainTest(655555, 0, true, rssDropTimes), TestCase::Duration::QUICK);
    rssDropTimes.clear();
    // // Result:
    // // Unexpected transmissions and subsequent retransmissions

    // The remaining test cases of MR 1590 (A-MPDU and RTS/CTS configurations)
    // rely on hand-tuned receive signal strength toggle times and on the retry
    // chain rework proposed by that MR (which accounts for RTS/CTS in the
    // computation of the allowed retransmissions), hence they are not included
    // here.
}

static WifiMinstrelHtTestSuite g_minstrelHtTestSuite; ///< the test suite
