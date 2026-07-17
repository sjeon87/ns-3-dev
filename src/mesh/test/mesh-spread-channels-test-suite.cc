/*
 * Copyright (c) 2026 Hamburg University of Applied Sciences
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Christoph Busch <christoph.busch@haw-hamburg.de>
 */

#include "ns3/mesh-helper.h"
#include "ns3/mesh-point-device.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy.h"
#include "ns3/yans-wifi-helper.h"

#include <string>
#include <vector>

using namespace ns3;

const auto MESH_STACK = "ns3::Dot11sStack"; ///< mesh stack installer type

/**
 * @ingroup mesh-test
 * @brief Test vector for mesh channel assignment test
 */
struct MeshSpreadChannelsTestVector
{
    std::string testName;                  ///< name of the test case
    WifiStandard standard;                 ///< wifi standard
    uint32_t nInterfaces;                  ///< number of interfaces per mesh point
    MeshHelper::ChannelPolicy policy;      ///< tested channel policy
    std::string channelSettings;           ///< PHY channel settings; empty = default
    std::vector<uint8_t> expectedChannels; ///< expected channel number per interface
};

/**
 * @ingroup mesh-test
 * @ingroup tests
 * @brief Mesh channel assignment test
 *
 * Tests the channel assignment in MeshHelper::Install.
 */
class MeshSpreadChannelsTest : public TestCase
{
  public:
    /**
     * Constructor
     *
     * @param testVec the test vector for this test case
     */
    MeshSpreadChannelsTest(MeshSpreadChannelsTestVector testVec)
        : TestCase(testVec.testName),
          m_testVector(testVec)
    {
    }

  private:
    void DoRun() override;

    MeshSpreadChannelsTestVector m_testVector; ///< test vector for current test case
};

void
MeshSpreadChannelsTest::DoRun()
{
    // create nodes
    NodeContainer nodes;
    nodes.Create(1);

    // phy & channel setup
    YansWifiPhyHelper wifiPhy;
    auto wifiChannelHelper = YansWifiChannelHelper::Default();
    Ptr<YansWifiChannel> chan = wifiChannelHelper.Create();
    wifiPhy.SetChannel(chan);

    // apply channel settings, if set
    if (!m_testVector.channelSettings.empty())
    {
        wifiPhy.Set("ChannelSettings", StringValue(m_testVector.channelSettings));
    }

    // install mesh stack
    MeshHelper mesh = MeshHelper::Default();
    mesh.SetStackInstaller(MESH_STACK);
    mesh.SetStandard(m_testVector.standard);
    mesh.SetNumberOfInterfaces(m_testVector.nInterfaces);
    mesh.SetSpreadInterfaceChannels(m_testVector.policy);
    NetDeviceContainer meshDevices = mesh.Install(wifiPhy, nodes);

    // compare channel assignment
    Ptr<MeshPointDevice> mp = DynamicCast<MeshPointDevice>(meshDevices.Get(0));
    std::vector<Ptr<NetDevice>> interfaces = mp->GetInterfaces();
    for (std::size_t i = 0; i < interfaces.size(); ++i)
    {
        Ptr<WifiNetDevice> iface = DynamicCast<WifiNetDevice>(interfaces[i]);
        const auto channelNumber = iface->GetPhy()->GetChannelNumber();
        const auto expectedChannel = m_testVector.expectedChannels[i];
        NS_TEST_EXPECT_MSG_EQ(+channelNumber, +expectedChannel, "wrong channel on interface " << i);
    }

    Simulator::Destroy();
}

/**
 * @ingroup mesh-test
 * @ingroup tests
 * @brief Mesh channel assignment test suite
 */
class MeshSpreadChannelsTestSuite : public TestSuite
{
  public:
    MeshSpreadChannelsTestSuite();
};

MeshSpreadChannelsTestSuite::MeshSpreadChannelsTestSuite()
    : TestSuite("devices-mesh-spread-channels", Type::UNIT)
{
    std::vector<MeshSpreadChannelsTestVector> testVectors{
        {
            .testName = "802.11a, 5 GHz, three interfaces, non-overlapping channels",
            .standard = WIFI_STANDARD_80211a,
            .nInterfaces = 3,
            .policy = MeshHelper::SPREAD_CHANNELS,
            .channelSettings = "",
            .expectedChannels = {36, 40, 44},
        },
        {
            .testName = "802.11g, 2.4 GHz, four interfaces, non-overlapping channels",
            .standard = WIFI_STANDARD_80211g,
            .nInterfaces = 4,
            .policy = MeshHelper::SPREAD_CHANNELS,
            .channelSettings = "",
            .expectedChannels = {1, 5, 9, 13},
        },
        {
            .testName = "802.11b, 2.4 GHz, three interfaces, non-overlapping channels",
            .standard = WIFI_STANDARD_80211b,
            .nInterfaces = 3,
            .policy = MeshHelper::SPREAD_CHANNELS,
            .channelSettings = "",
            .expectedChannels = {1, 6, 11},
        },
        {
            .testName = "SPREAD_CHANNELS does not change single interface",
            .standard = WIFI_STANDARD_80211a,
            .nInterfaces = 1,
            .policy = MeshHelper::SPREAD_CHANNELS,
            .channelSettings = "{44, 20, BAND_5GHZ, 0}",
            .expectedChannels = {44},
        },
        {
            .testName = "ZERO_CHANNEL does not change configured channel",
            .standard = WIFI_STANDARD_80211a,
            .nInterfaces = 2,
            .policy = MeshHelper::ZERO_CHANNEL,
            .channelSettings = "{40, 20, BAND_5GHZ, 0}",
            .expectedChannels = {40, 40},
        },
        {
            .testName = "2.4 GHz works with default channel settings",
            .standard = WIFI_STANDARD_80211g,
            .nInterfaces = 1,
            .policy = MeshHelper::ZERO_CHANNEL,
            .channelSettings = "",
            .expectedChannels = {1},
        },
    };
    for (const auto& testVector : testVectors)
    {
        AddTestCase(new MeshSpreadChannelsTest(testVector), TestCase::Duration::QUICK);
    }
}

static MeshSpreadChannelsTestSuite g_meshSpreadChannelsTestSuite; ///< the test suite
