/*
 * Copyright (c) 2009 Drexel University
 *
 * SPDX-License-Identifier: GPL-2.0-only and NIST-Software
 *
 * Author: Tom Wambold <tom5760@gmail.com>
 */

#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/manet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"

using namespace ns3;
using namespace manet;

NS_LOG_COMPONENT_DEFINE("NhdpExample");

NetDeviceContainer CreateAdhocNetwork(NodeContainer c, Ssid ssid);

int
main(int argc, char* argv[])
{
    bool verbose{true};
    bool useIpv6{false};
    Time startTime{Seconds(1)};
    Time stopTime{Seconds(10)};

    CommandLine cmd;
    cmd.AddValue("verbose", "turn on log components", verbose);
    cmd.AddValue("ipv6", "run NHDP over IPv6 (link-local plus a ULA) instead of IPv4", useIpv6);
    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnableAll(
            LogLevel(LOG_PREFIX_FUNC | LOG_PREFIX_LEVEL | LOG_PREFIX_TIME | LOG_PREFIX_NODE));
        LogComponentEnable("NhdpClient", LOG_LEVEL_ALL);
    }
    // Create node index zero but do not use it; this allows the subsequent
    // node IDs to align with the last octet of the IP address, for help
    // in correlating IP addresses to nodes
    Ptr<Node> unusedNode [[maybe_unused]] = CreateObject<Node>();

    NS_LOG_INFO("Creating nodes...");
    NodeContainer nodes;
    nodes.Create(2);

    NS_LOG_INFO("Creating mobility model...");
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    auto devices = CreateAdhocNetwork(nodes, Ssid("nhdp-example"));

    NS_LOG_INFO("Installing internet stack...");
    InternetStackHelper internet;
    internet.Install(nodes);

    NhdpHelper nhdpHelper;
    if (useIpv6)
    {
        // Assign a routing Unique Local Address (RFC 4193) to each node; the stack
        // auto-configures a link-local address as well, so NHDP advertises both.  Only the
        // top 8 bits (fd00::/8) follow the RFC 4193 ULA allocation; a pseudo-randomly
        // derived 40-bit Global ID (RFC 4193 Sec. 3.2.1) is not used, for simplicity.
        Ipv6AddressHelper ipv6;
        ipv6.SetBase(Ipv6Address("fd00::"), Ipv6Prefix(64));
        auto ipInterfaces = ipv6.Assign(devices);
        nhdpHelper.SetAttribute("AddressMode", EnumValue(manet::AddressMode::IPV6));
    }
    else
    {
        Ipv4AddressHelper ipv4;
        ipv4.SetBase("7.0.0.0", "255.255.255.255"); // Will assign 7.0.0.1 first
        auto ipInterfaces = ipv4.Assign(devices);
    }

    NS_LOG_INFO("Installing applications...");
    ApplicationContainer apps = nhdpHelper.Install(nodes);

    /*
    NS_LOG_INFO ("Adding interfaces to NHDP...");
    for (NodeContainer::Iterator iter = nodes.Begin ();
        iter != nodes.End ();
        iter++)
      {
        Ptr<NhdpClient> nhdp = DynamicCast<NhdpClient> ((*iter)->GetApplication (0));
        nhdp->AddLocalInterface(1, true);
      }
    */

    YansWifiPhyHelper phy;
    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    phy.EnablePcap("nhdp-example", devices);

    NS_LOG_INFO("Starting simulation...");
    apps.Start(startTime);
    apps.Stop(stopTime);

    Simulator::Stop(stopTime + Seconds(1));
    Simulator::Run();
    Simulator::Destroy();
}

NetDeviceContainer
CreateAdhocNetwork(NodeContainer c, Ssid ssid)
{
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211a);
    WifiMacHelper wifiMac;
    YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    wifiPhy.SetChannel(wifiChannel.Create());
    wifiMac.SetType("ns3::AdhocWifiMac");
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("OfdmRate54Mbps"));
    return wifi.Install(wifiPhy, wifiMac, c);
}
