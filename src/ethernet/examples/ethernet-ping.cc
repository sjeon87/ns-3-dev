/*
 * Copyright (c) 2026 Somaiya Vidyavihar University, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manmita Das <das.manmita12@gmail.com>
 */

/**
 * Network Topology
 *
 * +--------+                   +--------+
 * | Node 1 |-------------------| Node 2 |
 * +--------+   Ethernet Link   +--------+
 *
 *
 * This example demonstrates communication between two nodes over Ethernet Link.
 * The example can be used to validate basic Ethernet connectivity, packet
 * transmission and reception, link delay modeling, and packet handling under
 * normal and error conditions.
 *
 */

#include "ns3/core-module.h"
#include "ns3/ethernet-channel.h"
#include "ns3/ethernet-helper.h"
#include "ns3/ethernet-net-device.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ping-helper.h"
#include "ns3/queue.h"
#include "ns3/trace-helper.h"

using namespace ns3;
using namespace ns3::ethernet;

NS_LOG_COMPONENT_DEFINE("EthernetPingExample");

int
main(int argc, char* argv[])
{
    bool verbose = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("verbose", "turn on log components", verbose);
    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnableAll(LogLevel(LOG_PREFIX_TIME | LOG_PREFIX_FUNC | LOG_PREFIX_NODE));
        LogComponentEnable("EthernetNetDevice", LOG_LEVEL_INFO);
        LogComponentEnable("EthernetChannel", LOG_LEVEL_INFO);
        LogComponentEnable("Ping", LOG_LEVEL_INFO);
    }

    // We will create 2 Nodes
    NodeContainer nodes;
    nodes.Create(2);

    EthernetHelper ethernet;

    NetDeviceContainer devices = ethernet.Install(nodes);

    ethernet.EnablePcapAll("ethernet-ping", true);

    InternetStackHelper internet;
    internet.Install(nodes);

    NS_LOG_INFO("Assign IP Addresses.");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    PingHelper ping(interfaces.GetAddress(1, 0));
    ping.SetAttribute("Count", UintegerValue(5));
    ApplicationContainer apps = ping.Install(nodes.Get(0));

    apps.Start(Seconds(1.0));
    apps.Stop(Seconds(10.0));

    Simulator::Stop(Seconds(10.0));

    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
