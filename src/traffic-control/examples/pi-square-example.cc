/*
 * Copyright (c) 2017 Trinity College Dublin
 * Copyright (c) 2025-26 NITK Surathkal (Porting to ns-3)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Rohit P. Tahiliani <rohit.tahil@gmail.com>
 *
 */

// This program simulates the following topology:
//
//           1000 Mbps           10Mbps           1000 Mbps
// Senders -------------- R1 -------------- R2 -------------- Receivers
//              5ms               10ms               5ms
//
// The link between R1 and R2 is a bottleneck link with 1 Mbps. All other
// links are 100 Mbps.
//
// This program runs by default for 18 seconds and creates a new directory
// called 'pi-square-example' in the ns-3 root directory. The program creates
// one .dat file containing sojourn time over time.
//
// (1) qdelay.dat file contains sojourn time trace for queuedisc at R1

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"

#include <iomanip>
#include <iostream>
#include <map>

using namespace ns3;

std::ofstream sojournTime;

static void
TraceSojourn(Time newValue)
{
    sojournTime << Simulator::Now().GetSeconds() << " " << newValue.GetMilliSeconds() << std::endl;
}

int
main(int argc, char* argv[])
{
    uint32_t nLeaf = 10;
    bool modeBytes = false;
    uint32_t queueDiscLimitPackets = 1000;
    uint32_t pktSize = 512;
    std::string appDataRate = "10Mbps";
    std::string queueDisc = "ns3::PiSquareQueueDisc";
    uint16_t port = 5001;
    std::string bottleNeckLinkBw = "10Mbps";
    std::string bottleNeckLinkDelay = "10ms";

    CommandLine cmd;
    cmd.AddValue("nLeaf", "Number of left and right side leaf nodes", nLeaf);
    cmd.AddValue("queueDiscLimitPackets",
                 "Max Packets allowed in the queue disc",
                 queueDiscLimitPackets);
    cmd.AddValue("appPktSize", "Set OnOff App Packet Size", pktSize);
    cmd.AddValue("appDataRate", "Set OnOff App DataRate", appDataRate);
    cmd.AddValue("modeBytes", "Set Queue disc mode to Packets <false> or bytes <true>", modeBytes);

    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::OnOffApplication::PacketSize", UintegerValue(pktSize));
    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue(appDataRate));
    Config::SetDefault(queueDisc + "::MeanPktSize", UintegerValue(pktSize));

    if (!modeBytes)
    {
        Config::SetDefault(
            queueDisc + "::MaxSize",
            QueueSizeValue(QueueSize(QueueSizeUnit::PACKETS, queueDiscLimitPackets)));
    }
    else
    {
        Config::SetDefault(
            queueDisc + "::MaxSize",
            QueueSizeValue(QueueSize(QueueSizeUnit::BYTES, queueDiscLimitPackets * pktSize)));
    }

    std::string dir = "pi-square-example";
    ns3::SystemPath::MakeDirectories(dir);
    sojournTime.open(dir + "/qdelay.dat");

    // Create the point-to-point link helpers
    PointToPointHelper bottleNeckLink;
    bottleNeckLink.SetDeviceAttribute("DataRate", StringValue(bottleNeckLinkBw));
    bottleNeckLink.SetChannelAttribute("Delay", StringValue(bottleNeckLinkDelay));

    PointToPointHelper pointToPointLeaf;
    pointToPointLeaf.SetDeviceAttribute("DataRate", StringValue("1000Mbps"));
    pointToPointLeaf.SetChannelAttribute("Delay", StringValue("5ms"));

    PointToPointDumbbellHelper d(nLeaf, pointToPointLeaf, nLeaf, pointToPointLeaf, bottleNeckLink);

    // Install Stack
    InternetStackHelper stack;
    for (uint32_t i = 0; i < d.LeftCount(); ++i)
    {
        stack.Install(d.GetLeft(i));
    }
    for (uint32_t i = 0; i < d.RightCount(); ++i)
    {
        stack.Install(d.GetRight(i));
    }

    stack.Install(d.GetLeft());
    stack.Install(d.GetRight());
    TrafficControlHelper tchBottleneck;
    QueueDiscContainer queueDiscs;
    tchBottleneck.SetRootQueueDisc(queueDisc);
    tchBottleneck.Install(d.GetLeft()->GetDevice(0));
    queueDiscs = tchBottleneck.Install(d.GetRight()->GetDevice(0));

    Ptr<QueueDisc> queueDiscPtr = queueDiscs.Get(0);
    queueDiscPtr->TraceConnectWithoutContext("SojournTime", MakeCallback(&TraceSojourn));

    // Assign IP Addresses
    d.AssignIpv4Addresses(Ipv4AddressHelper("10.1.1.0", "255.255.255.0"),
                          Ipv4AddressHelper("10.2.1.0", "255.255.255.0"),
                          Ipv4AddressHelper("10.3.1.0", "255.255.255.0"));

    // Install on/off app on all right side nodes
    OnOffHelper clientHelper("ns3::TcpSocketFactory", Address());
    clientHelper.SetAttribute("OnTime", StringValue("ns3::UniformRandomVariable[Min=0.|Max=1.]"));
    clientHelper.SetAttribute("OffTime", StringValue("ns3::UniformRandomVariable[Min=0.|Max=1.]"));
    Address sinkLocalAddress(InetSocketAddress(Ipv4Address::GetAny(), port));
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", sinkLocalAddress);
    ApplicationContainer sinkApps;
    for (uint32_t i = 0; i < d.LeftCount(); ++i)
    {
        sinkApps.Add(packetSinkHelper.Install(d.GetLeft(i)));
    }
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(18.0));

    ApplicationContainer clientApps;
    for (uint32_t i = 0; i < d.RightCount(); ++i)
    {
        // Create an on/off app sending packets to the left side
        AddressValue remoteAddress(InetSocketAddress(d.GetLeftIpv4Address(i), port));
        clientHelper.SetAttribute("Remote", remoteAddress);
        clientApps.Add(clientHelper.Install(d.GetRight(i)));
    }
    clientApps.Start(Seconds(0.1)); // Start 1 second after sink
    clientApps.Stop(Seconds(15.0)); // Stop before the sink

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Stop(Seconds(18.0));

    std::cout << "Running the simulation" << std::endl;
    Simulator::Run();

    QueueDisc::Stats st = queueDiscs.Get(0)->GetStats();
    auto forced = st.GetNDroppedPackets(PiSquareQueueDisc::FORCED_DROP);
    auto unforced = st.GetNDroppedPackets(PiSquareQueueDisc::UNFORCED_DROP);

    if (forced != 0 || unforced == 0)
    {
        std::cout << "There should be some unforced drops, but no forced drops" << std::endl;
        exit(1);
    }

    std::cout << "*** Stats from the bottleneck queue disc ***" << std::endl;

    std::cout << st << std::endl;

    std::cout << "Destroying the simulation" << std::endl;
    Simulator::Destroy();
    sojournTime.close();
    return 0;
}
