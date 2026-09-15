/*
 * Copyright (c) 2026 Somaiya Vidyavihar University, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manmita Das <das.manmita12@gmail.com>
 */

/**
 * Ethernet Switch Ping example using EthernetNetDevice end hosts and a switch.
 *
 * Topology:
 *
 *   host0 --- switch port0
 *                |
 *              Switch
 *                |
 *   host1 --- switch port1
 *
 * Each host is attached to its own EthernetChannel. The switch forwards frames
 * between ports using learned MAC addresses, and floods unknown destinations so
 * that ARP and Ping work.
 */

#include "ns3/core-module.h"
#include "ns3/ethernet-helper.h"
#include "ns3/ethernet-switch-helper.h"
#include "ns3/ethernet-switch.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"
#include "ns3/mac48-address.h"
#include "ns3/network-module.h"
#include "ns3/ping-helper.h"

using namespace ns3;
using namespace ns3::ethernet;

NS_LOG_COMPONENT_DEFINE("EthernetSwitchPingExample");

int
main(int argc, char* argv[])
{
    bool verbose = false;
    uint32_t count = 5;
    Time interval = Seconds(1);

    CommandLine cmd(__FILE__);
    cmd.AddValue("verbose", "Enable Ethernet and Ping logging", verbose);
    cmd.AddValue("count", "Number of ping packets", count);
    cmd.AddValue("interval", "Interval between ping packets", interval);
    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnableAll(LogLevel(LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));
        LogComponentEnable("EthernetSwitch", LOG_LEVEL_INFO);
        LogComponentEnable("EthernetSwitchPort", LOG_LEVEL_INFO);
        LogComponentEnable("EthernetNetDevice", LOG_LEVEL_INFO);
        LogComponentEnable("EthernetMac", LOG_LEVEL_INFO);
        LogComponentEnable("EthernetPhy", LOG_LEVEL_INFO);
        LogComponentEnable("Ping", LOG_LEVEL_INFO);
    }

    NodeContainer hosts;
    hosts.Create(2);

    NodeContainer ethernetSwitch;
    ethernetSwitch.Create(1);

    EthernetSwitchHelper switchHelper;
    NetDeviceContainer devices = switchHelper.Install(hosts);
    switchHelper.Install(ethernetSwitch.Get(0), devices);

    EthernetHelper ethernet;
    ethernet.EnablePcapAll("ethernet-switch-ping", true);

    InternetStackHelper internet;
    internet.Install(hosts);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    PingHelper ping(interfaces.GetAddress(1));
    ping.SetAttribute("Count", UintegerValue(count));
    ping.SetAttribute("Interval", TimeValue(interval));

    ApplicationContainer apps = ping.Install(hosts.Get(0));
    apps.Start(Seconds(1.0));
    apps.Stop(Seconds(20.0));

    Simulator::Stop(Seconds(20.0));

    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
