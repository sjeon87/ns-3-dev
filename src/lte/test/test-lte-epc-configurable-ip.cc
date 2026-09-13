/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Usham Roy <ushamroy80@gmail.com>
 */

#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/inet-socket-address.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/log.h"
#include "ns3/lte-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/point-to-point-epc-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/udp-echo-helper.h"
#include "ns3/uinteger.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LteEpcConfigurableIpTest");

/**
 * @ingroup lte-test
 *
 * @brief Test that LTE EPC works correctly with non-default (user-configured)
 * IP addresses for all backhaul links.
 *
 * This test exercises the configurable IP address attributes added to
 * PointToPointEpcHelper and NoBackhaulEpcHelper. It sets all network base
 * addresses to non-default values and verifies end-to-end data transfer
 * (both uplink and downlink) still works correctly.
 */
class LteEpcConfigurableIpTestCase : public TestCase
{
  public:
    LteEpcConfigurableIpTestCase();
    ~LteEpcConfigurableIpTestCase() override;

  private:
    void DoRun() override;
};

LteEpcConfigurableIpTestCase::LteEpcConfigurableIpTestCase()
    : TestCase("LTE EPC end-to-end data with non-default IP addresses")
{
}

LteEpcConfigurableIpTestCase::~LteEpcConfigurableIpTestCase()
{
}

void
LteEpcConfigurableIpTestCase::DoRun()
{
    NS_LOG_FUNCTION(this);
    Config::Reset();

    Config::SetDefault("ns3::LteSpectrumPhy::CtrlErrorModelEnabled", BooleanValue(false));
    Config::SetDefault("ns3::LteSpectrumPhy::DataErrorModelEnabled", BooleanValue(false));
    Config::SetDefault("ns3::LteHelper::UseIdealRrc", BooleanValue(true));

    // Set non-default IP addresses for all configurable attributes
    Config::SetDefault("ns3::PointToPointEpcHelper::S1uNetworkAddress",
                       Ipv4AddressValue(Ipv4Address("172.16.0.0")));
    Config::SetDefault("ns3::PointToPointEpcHelper::S1apNetworkAddress",
                       Ipv4AddressValue(Ipv4Address("172.17.0.0")));
    Config::SetDefault("ns3::NoBackhaulEpcHelper::X2NetworkAddress",
                       Ipv4AddressValue(Ipv4Address("172.18.0.0")));
    Config::SetDefault("ns3::NoBackhaulEpcHelper::S11NetworkAddress",
                       Ipv4AddressValue(Ipv4Address("172.19.0.0")));
    Config::SetDefault("ns3::NoBackhaulEpcHelper::S5NetworkAddress",
                       Ipv4AddressValue(Ipv4Address("172.20.0.0")));
    Config::SetDefault("ns3::NoBackhaulEpcHelper::UeNetworkAddress",
                       Ipv4AddressValue(Ipv4Address("10.0.0.0")));
    Config::SetDefault("ns3::NoBackhaulEpcHelper::UeNetworkMask",
                       Ipv4MaskValue(Ipv4Mask("255.0.0.0")));

    Config::SetDefault("ns3::RadioBearerStatsCalculator::DlPdcpOutputFilename",
                       StringValue(CreateTempDirFilename("DlPdcpStats.txt")));
    Config::SetDefault("ns3::RadioBearerStatsCalculator::UlPdcpOutputFilename",
                       StringValue(CreateTempDirFilename("UlPdcpStats.txt")));

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);

    lteHelper->SetAttribute("PathlossModel", StringValue("ns3::FriisPropagationLossModel"));

    // allow jumbo frames on the S1-U link
    epcHelper->SetAttribute("S1uLinkMtu", UintegerValue(30000));

    Ptr<Node> pgw = epcHelper->GetPgwNode();

    // Create a single RemoteHost
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    // Create the internet link
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(30000));
    p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);

    // setup default gateway for the remote host
    // route to the non-default UE network (10.0.0.0/8 instead of default 7.0.0.0/8)
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("10.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

    // Create one eNB and one UE
    NodeContainer enbNodes;
    enbNodes.Create(1);
    NodeContainer ueNodes;
    ueNodes.Create(1);

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(enbNodes);
    mobility.Install(ueNodes);

    NetDeviceContainer enbDevs = lteHelper->InstallEnbDevice(enbNodes);
    NetDeviceContainer ueDevs = lteHelper->InstallUeDevice(ueNodes);

    // Install the IP stack on the UE
    InternetStackHelper ueInternet;
    ueInternet.Install(ueNodes);

    // Assign IP address to UE
    Ipv4InterfaceContainer ueIpIface =
        epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueDevs.Get(0)));

    // Verify the UE got an address in the non-default 10.0.0.0/8 range
    Ipv4Address ueAddr = ueIpIface.GetAddress(0);
    NS_TEST_ASSERT_MSG_EQ((ueAddr.Get() & 0xFF000000),
                          (Ipv4Address("10.0.0.0").Get() & 0xFF000000),
                          "UE address " << ueAddr << " should be in the 10.0.0.0/8 range");

    // Set default route for the UE
    Ptr<Ipv4StaticRouting> ueStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(ueNodes.Get(0)->GetObject<Ipv4>());
    ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);

    // Attach UE to eNB
    lteHelper->Attach(ueDevs.Get(0), enbDevs.Get(0));

    // Setup downlink application
    uint16_t dlPort = 2000;
    PacketSinkHelper dlPacketSinkHelper("ns3::UdpSocketFactory",
                                        InetSocketAddress(Ipv4Address::GetAny(), dlPort));
    ApplicationContainer dlSinkApps = dlPacketSinkHelper.Install(ueNodes.Get(0));
    dlSinkApps.Start(Seconds(0.04));

    UdpEchoClientHelper dlClient(ueIpIface.GetAddress(0), dlPort);
    dlClient.SetAttribute("MaxPackets", UintegerValue(5));
    dlClient.SetAttribute("Interval", TimeValue(Seconds(0.01)));
    dlClient.SetAttribute("PacketSize", UintegerValue(100));
    ApplicationContainer dlClientApps = dlClient.Install(remoteHost);
    dlClientApps.Start(Seconds(0.04));

    // Setup uplink application
    uint16_t ulPort = 3000;
    PacketSinkHelper ulPacketSinkHelper("ns3::UdpSocketFactory",
                                        InetSocketAddress(Ipv4Address::GetAny(), ulPort));
    ApplicationContainer ulSinkApps = ulPacketSinkHelper.Install(remoteHost);
    ulSinkApps.Start(Seconds(0.8));

    UdpEchoClientHelper ulClient(remoteHostAddr, ulPort);
    ulClient.SetAttribute("MaxPackets", UintegerValue(5));
    ulClient.SetAttribute("Interval", TimeValue(Seconds(0.01)));
    ulClient.SetAttribute("PacketSize", UintegerValue(100));
    ApplicationContainer ulClientApps = ulClient.Install(ueNodes.Get(0));
    ulClientApps.Start(Seconds(0.8));

    // Activate a dedicated bearer
    EpsBearer epsBearer(EpsBearer::NGBR_VOICE_VIDEO_GAMING);
    Ptr<EpcTft> tft = Create<EpcTft>();
    EpcTft::PacketFilter dlpf;
    dlpf.localPortStart = dlPort;
    dlpf.localPortEnd = dlPort;
    tft->Add(dlpf);
    EpcTft::PacketFilter ulpf;
    ulpf.remotePortStart = ulPort;
    ulpf.remotePortEnd = ulPort;
    tft->Add(ulpf);
    lteHelper->ActivateDedicatedEpsBearer(ueDevs.Get(0), epsBearer, tft);

    Config::Set(
        "/NodeList/*/DeviceList/*/LteEnbRrc/UeMap/*/RadioBearerMap/*/LteRlc/MaxTxBufferSize",
        UintegerValue(2 * 1024 * 1024));
    Config::Set("/NodeList/*/DeviceList/*/LteUeRrc/RadioBearerMap/*/LteRlc/MaxTxBufferSize",
                UintegerValue(2 * 1024 * 1024));

    double statsStartTime = 0.040;
    double statsDuration = 2.0;

    lteHelper->EnablePdcpTraces();
    lteHelper->GetPdcpStats()->SetAttribute("StartTime", TimeValue(Seconds(statsStartTime)));
    lteHelper->GetPdcpStats()->SetAttribute("EpochDuration", TimeValue(Seconds(statsDuration)));

    Simulator::Stop(Seconds(statsStartTime + statsDuration - 0.0001));
    Simulator::Run();

    // Verify end-to-end data transfer worked
    uint32_t expectedPkts = 5;
    uint32_t expectedBytes = 5 * 100;

    // IMSI=1, LCID=4 (first dedicated bearer)
    uint32_t txPktsDl = lteHelper->GetPdcpStats()->GetDlTxPackets(1, 4);
    uint32_t rxPktsDl = lteHelper->GetPdcpStats()->GetDlRxPackets(1, 4);
    uint32_t txPktsUl = lteHelper->GetPdcpStats()->GetUlTxPackets(1, 4);
    uint32_t rxPktsUl = lteHelper->GetPdcpStats()->GetUlRxPackets(1, 4);

    Ptr<PacketSink> dlSink = dlSinkApps.Get(0)->GetObject<PacketSink>();
    Ptr<PacketSink> ulSink = ulSinkApps.Get(0)->GetObject<PacketSink>();

    NS_TEST_ASSERT_MSG_EQ(txPktsDl,
                          expectedPkts,
                          "wrong TX PDCP packets in downlink with non-default IP addresses");
    NS_TEST_ASSERT_MSG_EQ(rxPktsDl,
                          expectedPkts,
                          "wrong RX PDCP packets in downlink with non-default IP addresses");
    NS_TEST_ASSERT_MSG_EQ(txPktsUl,
                          expectedPkts,
                          "wrong TX PDCP packets in uplink with non-default IP addresses");
    NS_TEST_ASSERT_MSG_EQ(rxPktsUl,
                          expectedPkts,
                          "wrong RX PDCP packets in uplink with non-default IP addresses");
    NS_TEST_ASSERT_MSG_EQ(dlSink->GetTotalRx(),
                          expectedBytes,
                          "wrong total received bytes in downlink with non-default IP addresses");
    NS_TEST_ASSERT_MSG_EQ(ulSink->GetTotalRx(),
                          expectedBytes,
                          "wrong total received bytes in uplink with non-default IP addresses");

    Simulator::Destroy();
}

/**
 * @ingroup lte-test
 *
 * @brief Test suite for configurable EPC IP addresses
 */
class LteEpcConfigurableIpTestSuite : public TestSuite
{
  public:
    LteEpcConfigurableIpTestSuite();

} g_lteEpcConfigurableIpTestSuite; ///< the test suite

LteEpcConfigurableIpTestSuite::LteEpcConfigurableIpTestSuite()
    : TestSuite("lte-epc-configurable-ip", Type::SYSTEM)
{
    AddTestCase(new LteEpcConfigurableIpTestCase(), TestCase::Duration::QUICK);
}
