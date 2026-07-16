/*
 * Copyright (c) 2026 Hamburg University of Applied Sciences
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Christoph Busch <christoph.busch@haw-hamburg.de>
 */

#include "ns3/mesh-helper.h"
#include "ns3/mesh-peering-close-header.h"
#include "ns3/mesh-peering-confirm-header.h"
#include "ns3/mesh-peering-open-header.h"
#include "ns3/mgt-action-headers.h"
#include "ns3/mobility-helper.h"
#include "ns3/peer-management-protocol.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/test.h"
#include "ns3/yans-wifi-helper.h"

#include <tuple>
#include <vector>

using namespace ns3;
using namespace dot11s;

NS_LOG_COMPONENT_DEFINE("MeshPeeringFrameTestSuite");

const Time BEACON_INTERVAL = Seconds(1);     ///< beacon interval
const Time BEACON_START = MilliSeconds(100); ///< window for the random beacon start
const Time SIMULATION_TIME = Seconds(1);     ///< simulation duration
const auto NODE_COUNT = 2;                   ///< number of mesh nodes
const auto MESH_STACK = "ns3::Dot11sStack";  ///< mesh stack installer type
const auto CHANNEL_WIDTH = 0;                ///< 0 == default: pick the smallest channel available
const auto PEERING_CLOSE_TIME = MilliSeconds(500); ///< when node 0 cancels its peer links

/**
 * @ingroup dot11s-test
 * @brief Test vector for a mesh peering frame test case
 */
struct MeshPeeringFrameTestVector
{
    std::string testName;      ///< name of the test case
    WifiPhyBand phyBand;       ///< wifi phy band
    WifiStandard wifiStandard; ///< wifi standard
};

/**
 * @ingroup dot11s-test
 * @ingroup tests
 * @brief Mesh peering frame tests
 *
 * Tests the format of mesh peering frames
 */
class MeshPeeringFrameTest : public TestCase
{
  public:
    /**
     * Constructor
     *
     * @param testVec the test vector for this test case
     */
    MeshPeeringFrameTest(MeshPeeringFrameTestVector testVec);

  private:
    /**
     * Create the two-node mesh setup and connect the phy tx traces
     */
    void Setup();

    /**
     * Collects phy tx traces
     *
     * @param address address of the sender
     * @param packet the sent mpdu
     * @param txPower tx power in watts
     */
    void CollectTxTrace(Address address, Ptr<const Packet> packet, double txPower);

    /**
     * Validate mesh peering frame format
     *
     * @param txTrace a tuple containing the sender address and mpdu
     */
    void ValidateMeshPeering(const std::tuple<const Mac48Address, const Ptr<Packet>>& txTrace);

    /**
     * Validate the format of a mesh peering open frame
     *
     * @param sender the transmitter address (addr2) of the frame
     * @param receiver the receiver address (addr1) of the frame
     * @param packet the frame with the wifi mac and action headers removed
     */
    void ValidateMeshPeeringOpen(const Mac48Address sender,
                                 const Mac48Address receiver,
                                 const Ptr<Packet> packet);

    /**
     * Validate the format of a mesh peering confirm frame
     *
     * @param sender the transmitter address (addr2) of the frame
     * @param receiver the receiver address (addr1) of the frame
     * @param packet the frame with the wifi mac and action headers removed
     */
    void ValidateMeshPeeringConfirm(const Mac48Address sender,
                                    const Mac48Address receiver,
                                    const Ptr<Packet> packet);

    /**
     * Validate the format of a mesh peering close frame
     *
     * @param sender the transmitter address (addr2) of the frame
     * @param receiver the receiver address (addr1) of the frame
     * @param packet the frame with the wifi mac and action headers removed
     */
    void ValidateMeshPeeringClose(const Mac48Address sender,
                                  const Mac48Address receiver,
                                  const Ptr<Packet> packet);

    /**
     * Cancel peer links of a mesh point device to trigger peering close frames
     *
     * @param meshPointDevice the mesh point device whose peer links are canceled
     */
    void CancelPeerLinks(const Ptr<MeshPointDevice> meshPointDevice);

    void DoSetup() override;
    void DoTeardown() override;
    void DoRun() override;

    /**
     * Number of frames sent per station and frame type
     */
    struct FrameCounts
    {
        uint16_t beaconsSent = 0;  ///< beacon frames
        uint16_t opensSent = 0;    ///< mesh peering open frames
        uint16_t confirmsSent = 0; ///< mesh peering confirm frames
        uint16_t closesSent = 0;   ///< mesh peering close frames
    };

    std::vector<Mac48Address> m_addresses;                   ///< device mac addresses
    std::map<const Mac48Address, FrameCounts> m_frameCounts; ///< frame counts per station

    MeshPeeringFrameTestVector m_testVector; ///< the test vector for this test case
    std::vector<std::tuple<const Mac48Address, const Ptr<Packet>>>
        m_txTraces; ///< the collected phy tx traces (sender address, mpdu)
};

MeshPeeringFrameTest::MeshPeeringFrameTest(MeshPeeringFrameTestVector testVector)
    : TestCase(testVector.testName),
      m_testVector(testVector)
{
}

void
MeshPeeringFrameTest::Setup()
{
    // create wifi channel
    YansWifiChannelHelper wifiChannelHelper = YansWifiChannelHelper::Default();
    Ptr<YansWifiChannel> wifiChannel = wifiChannelHelper.Create();
    YansWifiPhyHelper wifiPhyHelper;
    wifiPhyHelper.SetChannel(wifiChannel);
    WifiPhy::ChannelSettingsValue channelSettings;
    channelSettings.Set(WifiPhy::ChannelSegments{{0, CHANNEL_WIDTH, m_testVector.phyBand, 0}});
    wifiPhyHelper.Set("ChannelSettings", channelSettings);

    // configure the mesh stack
    MeshHelper meshHelper = MeshHelper::Default();
    meshHelper.SetStandard(m_testVector.wifiStandard);
    meshHelper.SetRemoteStationManager("ns3::IdealWifiManager");
    meshHelper.SetMacType("BeaconGeneration",
                          BooleanValue(true),
                          "BeaconInterval",
                          TimeValue(BEACON_INTERVAL),
                          "RandomStart",
                          TimeValue(BEACON_START));
    meshHelper.SetStackInstaller(MESH_STACK);
    meshHelper.SetSpreadInterfaceChannels(MeshHelper::ZERO_CHANNEL);

    // install wifi/mesh stack on nodes
    NodeContainer nodes(NODE_COUNT);
    NetDeviceContainer netDevices = meshHelper.Install(wifiPhyHelper, nodes);
    meshHelper.AssignStreams(netDevices, 0);

    const auto positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));
    MobilityHelper mobility;
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    for (uint32_t netDeviceIdx = 0; netDeviceIdx < netDevices.GetN(); netDeviceIdx++)
    {
        const auto meshPointDevice = DynamicCast<MeshPointDevice>(netDevices.Get(netDeviceIdx));

        // schedule close event on first node to test peering close frames
        if (netDeviceIdx == 0)
        {
            Simulator::Schedule(PEERING_CLOSE_TIME,
                                &MeshPeeringFrameTest::CancelPeerLinks,
                                this,
                                meshPointDevice);
        }

        for (const auto& netDevice : meshPointDevice->GetInterfaces())
        {
            const auto wifiNetDevice = DynamicCast<WifiNetDevice>(netDevice);

            // add address to list
            const auto address = Mac48Address::ConvertFrom(wifiNetDevice->GetAddress());
            m_addresses.push_back(address);

            // connect traces
            for (const auto& phy : wifiNetDevice->GetPhys())
            {
                phy->TraceConnectWithoutContext(
                    "PhyTxBegin",
                    MakeCallback(&MeshPeeringFrameTest::CollectTxTrace, this).Bind(address));
            }
        }
    }
}

void
MeshPeeringFrameTest::CollectTxTrace(Address address, Ptr<const Packet> packet, double txPower)
{
    m_txTraces.emplace_back(Mac48Address::ConvertFrom(address), packet->Copy());
}

void
MeshPeeringFrameTest::DoSetup()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    Setup();
}

void
MeshPeeringFrameTest::CancelPeerLinks(const Ptr<MeshPointDevice> meshPointDevice)
{
    const auto pmp = meshPointDevice->GetObject<PeerManagementProtocol>();
    for (const auto& peerLink : pmp->GetPeerLinks())
    {
        peerLink->MLMECancelPeerLink(REASON11S_PEERING_CANCELLED);
    }
}

void
MeshPeeringFrameTest::ValidateMeshPeering(
    const std::tuple<const Mac48Address, const Ptr<Packet>>& txTrace)
{
    const auto address = std::get<const Mac48Address>(txTrace);
    const auto packet = std::get<const Ptr<Packet>>(txTrace);

    // parse wifi header from mpdu
    WifiMacHeader header;
    packet->RemoveHeader(header);

    // count beacons and ignore
    if (header.IsBeacon())
    {
        m_frameCounts[address].beaconsSent++;
        return;
    }

    // ignore other frames
    if (!header.IsAction())
    {
        return;
    }

    // must be a self-protected action frame
    NS_TEST_ASSERT_MSG_EQ(header.GetAddr1().IsGroup(),
                          false,
                          "addr1 (RA) must be a unicast address");
    NS_TEST_ASSERT_MSG_EQ(header.GetAddr2(),
                          address,
                          "addr2 (TA) must be equal to the sender address");
    WifiActionHeader actionHeader;
    packet->RemoveHeader(actionHeader);
    NS_TEST_ASSERT_MSG_EQ(actionHeader.GetCategory(),
                          WifiActionHeader::SELF_PROTECTED,
                          "must be a self protected action frame");

    // must be peering open/confirm/close
    const auto action = actionHeader.GetAction();
    NS_TEST_ASSERT_MSG_EQ(action.selfProtectedAction == WifiActionHeader::PEER_LINK_OPEN ||
                              action.selfProtectedAction == WifiActionHeader::PEER_LINK_CONFIRM ||
                              action.selfProtectedAction == WifiActionHeader::PEER_LINK_CLOSE,
                          true,
                          "frame is not a peering open/confirm/close frame");

    if (action.selfProtectedAction == WifiActionHeader::PEER_LINK_OPEN)
    {
        ValidateMeshPeeringOpen(header.GetAddr2(), header.GetAddr1(), packet);
    }
    else if (action.selfProtectedAction == WifiActionHeader::PEER_LINK_CONFIRM)
    {
        ValidateMeshPeeringConfirm(header.GetAddr2(), header.GetAddr1(), packet);
    }
    else if (action.selfProtectedAction == WifiActionHeader::PEER_LINK_CLOSE)
    {
        ValidateMeshPeeringClose(header.GetAddr2(), header.GetAddr1(), packet);
    }
}

void
MeshPeeringFrameTest::ValidateMeshPeeringOpen(const Mac48Address sender,
                                              const Mac48Address receiver,
                                              const Ptr<Packet> packet)
{
    // the receiver is either known by its beacon or from an open frame
    NS_TEST_ASSERT_MSG_EQ(m_frameCounts[receiver].beaconsSent == 1 ||
                              m_frameCounts[receiver].opensSent == 1,
                          true,
                          "open receiver must be known via beacon or open");
    NS_TEST_ASSERT_MSG_EQ(m_frameCounts[sender].opensSent,
                          0,
                          "must be the first open sent by this station");
    m_frameCounts[sender].opensSent++;

    // expected field/elements (802.11-2020, 9.6.15.2.2):
    // - Capability
    // - Supported Rates
    // - Extended Supported Rates (optional, not tested)
    // - Mesh ID
    // - Mesh Configuration
    // - Mesh Peering Management

    MeshPeeringOpenHeader header;
    packet->RemoveHeader(header);

    // capability information
    const auto capabilities = header.m_capability;
    NS_TEST_EXPECT_MSG_EQ(capabilities.IsEss(), false, "ess flag should be false");
    NS_TEST_EXPECT_MSG_EQ(capabilities.IsIbss(), false, "ibss flag should be false");
    // note: the qos flag should be set, but CapabilityInformation does not implement it

    // supported rates
    NS_TEST_ASSERT_MSG_EQ(header.Get<SupportedRates>().has_value(),
                          true,
                          "peering frame should contain supported rates element");

    // mesh id
    NS_TEST_ASSERT_MSG_EQ(header.Get<IeMeshId>().has_value(),
                          true,
                          "peering frame should contain the mesh id");

    // mesh configuration
    NS_TEST_ASSERT_MSG_EQ(header.Get<IeConfiguration>().has_value(),
                          true,
                          "peering frame should contain the mesh configuration element");

    // mesh peering management
    NS_TEST_ASSERT_MSG_EQ(header.Get<IeMeshPeeringManagement>().has_value(),
                          true,
                          "peering frame should contain the mesh peering management ie");
}

void
MeshPeeringFrameTest::ValidateMeshPeeringConfirm(const Mac48Address sender,
                                                 const Mac48Address receiver,
                                                 const Ptr<Packet> packet)
{
    NS_TEST_ASSERT_MSG_EQ(m_frameCounts[receiver].opensSent,
                          1,
                          "receiver must have sent a peering open first");
    m_frameCounts[sender].confirmsSent++;

    // expected field/elements (802.11-2020, 9.6.15.3.2):
    // - Capability
    // - AID
    // - Supported Rates
    // - Extended Supported Rates (optional, not tested)
    // - Mesh ID
    // - Mesh Configuration
    // - Mesh Peering Management

    MeshPeeringConfirmHeader header;
    packet->RemoveHeader(header);

    // capability information
    const auto capabilities = header.m_capability;
    NS_TEST_EXPECT_MSG_EQ(capabilities.IsEss(), false, "ess flag should be false");
    NS_TEST_EXPECT_MSG_EQ(capabilities.IsIbss(), false, "ibss flag should be false");
    // note: the qos flag should be set, but CapabilityInformation does not implement it

    // association id (should be zero, since both nodes are each other's first peer)
    const auto associationId = header.m_aid;
    NS_TEST_EXPECT_MSG_EQ(associationId, 0, "associationId should be zero");

    // supported rates
    NS_TEST_ASSERT_MSG_EQ(header.Get<SupportedRates>().has_value(),
                          true,
                          "peering frame should contain supported rates element");

    // mesh id
    NS_TEST_ASSERT_MSG_EQ(header.Get<IeMeshId>().has_value(),
                          true,
                          "peering frame should contain the mesh id");

    // mesh configuration
    NS_TEST_ASSERT_MSG_EQ(header.Get<IeConfiguration>().has_value(),
                          true,
                          "peering frame should contain the mesh configuration element");

    // mesh peering management
    NS_TEST_ASSERT_MSG_EQ(header.Get<IeMeshPeeringManagement>().has_value(),
                          true,
                          "peering frame should contain the mesh peering management ie");
}

void
MeshPeeringFrameTest::ValidateMeshPeeringClose(const Mac48Address sender,
                                               const Mac48Address receiver,
                                               const Ptr<Packet> packet)
{
    // a close requires an established link (receiver must have confirmed before)
    NS_TEST_ASSERT_MSG_EQ(m_frameCounts[receiver].confirmsSent,
                          1,
                          "close receiver must have confirmed the peer link first");
    m_frameCounts[sender].closesSent++;

    // expected field/elements (802.11-2020, 9.6.15.4.2):
    // - Mesh ID
    // - Mesh Peering Management

    MeshPeeringCloseHeader header;
    packet->RemoveHeader(header);

    // mesh id
    NS_TEST_ASSERT_MSG_EQ(header.Get<IeMeshId>().has_value(),
                          true,
                          "peering frame should contain the mesh id");

    // mesh peering management
    NS_TEST_ASSERT_MSG_EQ(header.Get<IeMeshPeeringManagement>().has_value(),
                          true,
                          "peering frame should contain the mesh peering management ie");
}

void
MeshPeeringFrameTest::DoRun()
{
    Simulator::Stop(SIMULATION_TIME);
    Simulator::Run();
    Simulator::Destroy();

    // validate peering frames
    for (const auto& txTrace : m_txTraces)
    {
        ValidateMeshPeering(txTrace);
    }

    // check that every station sent exactly one frame of each type: beacon, peering open, peering
    // confirm and close
    for (const auto& address : m_addresses)
    {
        NS_TEST_EXPECT_MSG_EQ(m_frameCounts[address].beaconsSent,
                              1,
                              "every station should send exactly one beacon");
        NS_TEST_EXPECT_MSG_EQ(m_frameCounts[address].opensSent,
                              1,
                              "every station should send exactly one peering open frame");
        NS_TEST_EXPECT_MSG_EQ(m_frameCounts[address].confirmsSent,
                              1,
                              "every station should send exactly one peering confirm frame");
        NS_TEST_EXPECT_MSG_EQ(m_frameCounts[address].closesSent,
                              1,
                              "every station should send exactly one peering close frame");
    }
}

void
MeshPeeringFrameTest::DoTeardown()
{
    m_txTraces.clear();
    m_frameCounts.clear();
    m_addresses.clear();
}

/**
 * @ingroup dot11s-test
 * @ingroup tests
 *
 * @brief mesh peering frame test suite
 */
class MeshPeeringFrameTestSuite : public TestSuite
{
  public:
    MeshPeeringFrameTestSuite();
};

MeshPeeringFrameTestSuite::MeshPeeringFrameTestSuite()
    : TestSuite("devices-mesh-dot11s-peering", Type::SYSTEM)
{
    std::vector<MeshPeeringFrameTestVector> testVectors{
        {
            .testName = "5 GHz 802.11a",
            .phyBand = WIFI_PHY_BAND_5GHZ,
            .wifiStandard = WIFI_STANDARD_80211a,
        },
    };

    for (const auto& testVector : testVectors)
    {
        AddTestCase(new MeshPeeringFrameTest(testVector), TestCase::Duration::QUICK);
    }
}

static MeshPeeringFrameTestSuite g_meshPeeringFrameTestSuite; ///< the test suite
