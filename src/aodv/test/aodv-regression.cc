/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Pavel Boyko <boyko@iitp.ru>
 */

#include "aodv-regression.h"

#include "bug-772.h"

#include "ns3/abort.h"
#include "ns3/aodv-helper.h"
#include "ns3/aodv-packet.h"
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/icmpv4.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-header.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/pcap-file.h"
#include "ns3/pcap-test.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/udp-header.h"
#include "ns3/uinteger.h"
#include "ns3/yans-wifi-helper.h"

#include <sstream>

using namespace ns3;

/**
 * @ingroup aodv-test
 *
 * @brief AODV regression test suite
 */
class AodvRegressionTestSuite : public TestSuite
{
  public:
    AodvRegressionTestSuite()
        : TestSuite("routing-aodv-regression", Type::SYSTEM)
    {
        SetDataDir(NS_TEST_SOURCEDIR);
        // General RREQ-RREP-RRER test case
        AddTestCase(new ChainRegressionTest("aodv-chain-regression-test"),
                    TestCase::Duration::QUICK);
        // \bugid{606} test case, should crash if bug is not fixed
        AddTestCase(new ChainRegressionTest("bug-606-test", Seconds(10), 3, Seconds(1)),
                    TestCase::Duration::QUICK);
        // \bugid{772} UDP test case
        AddTestCase(new Bug772ChainTest("udp-chain-test", "ns3::UdpSocketFactory", Seconds(3), 10),
                    TestCase::Duration::QUICK);
    }
} g_aodvRegressionTestSuite; ///< the test suite

/**
 * @ingroup aodv-test
 *
 * @brief Chain Regression Test
 */
ChainRegressionTest::ChainRegressionTest(const char* const prefix,
                                         Time t,
                                         uint32_t size,
                                         Time arpAliveTimeout)
    : TestCase("AODV chain regression test"),
      m_nodes(nullptr),
      m_prefix(prefix),
      m_time(t),
      m_size(size),
      m_step(120),
      m_arpAliveTimeout(arpAliveTimeout),
      m_seq(0)
{
}

ChainRegressionTest::~ChainRegressionTest()
{
    delete m_nodes;
}

void
ChainRegressionTest::SendPing()
{
    if (Simulator::Now() >= m_time)
    {
        return;
    }

    Ptr<Packet> p = Create<Packet>();
    Icmpv4Echo echo;
    echo.SetSequenceNumber(m_seq);
    m_seq++;
    echo.SetIdentifier(0);

    Ptr<Packet> dataPacket = Create<Packet>(56);
    echo.SetData(dataPacket);
    p->AddHeader(echo);
    Icmpv4Header header;
    header.SetType(Icmpv4Header::ICMPV4_ECHO);
    header.SetCode(0);
    if (Node::ChecksumEnabled())
    {
        header.EnableChecksum();
    }
    p->AddHeader(header);
    m_socket->Send(p, 0);
    Simulator::Schedule(Seconds(1), &ChainRegressionTest::SendPing, this);
}

void
ChainRegressionTest::DoRun()
{
    RngSeedManager::SetSeed(12345);
    RngSeedManager::SetRun(7);
    Config::SetDefault("ns3::ArpCache::AliveTimeout", TimeValue(m_arpAliveTimeout));

    CreateNodes();
    CreateDevices();

    // At m_time / 3 move central node away and see what will happen
    Ptr<Node> node = m_nodes->Get(m_size / 2);
    Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();
    Simulator::Schedule(Time(m_time / 3), &MobilityModel::SetPosition, mob, Vector(1e5, 1e5, 1e5));

    Config::ConnectWithoutContext("/NodeList/*/$ns3::Ipv4L3Protocol/Tx",
                                  MakeCallback(&ChainRegressionTest::TxPkt, this));

    Simulator::Stop(m_time);
    Simulator::Run();
    Simulator::Destroy();

    CheckResults();

    delete m_nodes, m_nodes = nullptr;
}

void
ChainRegressionTest::CreateNodes()
{
    m_nodes = new NodeContainer;
    m_nodes->Create(m_size);
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(m_step),
                                  "DeltaY",
                                  DoubleValue(0),
                                  "GridWidth",
                                  UintegerValue(m_size),
                                  "LayoutType",
                                  StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(*m_nodes);
}

void
ChainRegressionTest::CreateDevices()
{
    // 1. Setup WiFi
    int64_t streamsUsed = 0;
    int64_t totalStreamsUsed = 0;
    int64_t streamNumber = 0;
    int64_t streamIncrement = 1000;
    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");
    YansWifiPhyHelper wifiPhy;
    wifiPhy.DisablePreambleDetectionModel();
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    Ptr<YansWifiChannel> chan = wifiChannel.Create();
    wifiPhy.SetChannel(chan);

    // This test suite output was originally based on YansErrorRateModel
    wifiPhy.SetErrorRateModel("ns3::YansErrorRateModel");
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211a);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("OfdmRate6Mbps"),
                                 "RtsCtsThreshold",
                                 StringValue("2200"));
    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, *m_nodes);

    // Assign fixed stream numbers to wifi and channel random variables
    constexpr int expectedWifiStreamsPerDevice = 4;
    streamsUsed = WifiHelper::AssignStreams(devices, streamNumber);
    totalStreamsUsed += streamsUsed;
    streamNumber += streamIncrement;
    NS_TEST_ASSERT_MSG_EQ(streamsUsed,
                          (devices.GetN() * expectedWifiStreamsPerDevice),
                          "Stream assignment mismatch");
    streamsUsed = wifiChannel.AssignStreams(chan, streamNumber);
    totalStreamsUsed += streamsUsed;
    streamNumber += streamIncrement;
    // Assign 0 streams per channel for this configuration
    NS_TEST_ASSERT_MSG_EQ(streamsUsed, 0, "No WifiChannel stream usage expected");

    // 2. Setup TCP/IP & AODV
    AodvHelper aodv; // Use default parameters here
    InternetStackHelper internetStack;
    internetStack.SetIpv6StackInstall(false);
    internetStack.SetRoutingHelper(aodv);
    internetStack.Install(*m_nodes);
    streamsUsed = internetStack.AssignStreams(*m_nodes, streamNumber);
    totalStreamsUsed += streamsUsed;
    streamNumber += streamIncrement;
    constexpr int expectedArpL3ProtocolStreams = 1;
    constexpr int expectedInternetStreamsPerDevice = expectedArpL3ProtocolStreams;
    // InternetStack uses numDevices * expectedInternetStreamsPerDevice more streams
    NS_TEST_ASSERT_MSG_EQ(streamsUsed,
                          (devices.GetN() * expectedInternetStreamsPerDevice),
                          "Stream assignment mismatch");

    constexpr int expectedAodvStreamsPerDevice = 1;
    streamsUsed = aodv.AssignStreams(*m_nodes, streamNumber);
    totalStreamsUsed += streamsUsed;
    streamNumber += streamIncrement;
    // AODV uses numDevices * expectedAodvStreamsPerDevice
    NS_TEST_ASSERT_MSG_EQ(
        totalStreamsUsed,
        (devices.GetN() * (expectedWifiStreamsPerDevice + expectedInternetStreamsPerDevice +
                           expectedAodvStreamsPerDevice)),
        "Stream assignment mismatch");

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // 3. Setup ping
    m_socket =
        Socket::CreateSocket(m_nodes->Get(0), TypeId::LookupByName("ns3::Ipv4RawSocketFactory"));
    m_socket->SetAttribute("Protocol", UintegerValue(1)); // icmp
    InetSocketAddress src = InetSocketAddress(Ipv4Address::GetAny(), 0);
    m_socket->Bind(src);
    InetSocketAddress dst = InetSocketAddress(interfaces.GetAddress(m_size - 1), 0);
    m_socket->Connect(dst);

    SendPing();
}

void
ChainRegressionTest::CheckResults()
{
    if (m_size == 5)
    {
        NS_TEST_ASSERT_MSG_EQ(m_rreqCount, 14, "Routing broken: Expected exactly 14 RREQs");
        NS_TEST_ASSERT_MSG_EQ(m_rrepCount, 48, "Routing broken: Expected exactly 48 RREPs");
    }
    else if (m_size == 3)
    {
        NS_TEST_ASSERT_MSG_EQ(m_rreqCount, 6, "Routing broken: Expected exactly 6 RREQs");
        NS_TEST_ASSERT_MSG_EQ(m_rrepCount, 30, "Routing broken: Expected exactly 30 RREPs");
    }
}

void
ChainRegressionTest::TxPkt(Ptr<const Packet> packet, Ptr<Ipv4> ipv4, uint32_t interface)
{
    Ptr<Packet> p = packet->Copy();

    Ipv4Header ipv4Header;
    p->RemoveHeader(ipv4Header);

    if (ipv4Header.GetProtocol() == 17)
    {
        UdpHeader udpHeader;
        p->RemoveHeader(udpHeader);

        if (udpHeader.GetDestinationPort() == 654)
        {
            aodv::TypeHeader tHeader;
            p->PeekHeader(tHeader);

            if (tHeader.IsValid())
            {
                if (tHeader.Get() == aodv::AODVTYPE_RREQ)
                {
                    m_rreqCount++;
                }
                else if (tHeader.Get() == aodv::AODVTYPE_RREP)
                {
                    m_rrepCount++;
                }
            }
        }
    }
}
