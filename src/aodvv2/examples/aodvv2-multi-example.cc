/*
 * Copyright (c) 2024 University of Florence
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This is an example script for AODVv2 manet routing protocol.
 *
 * Authors: Francesco Todino <todinofrancesco97@gmail.com>
 *          Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "ns3/aodvv2-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/ping-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/yans-wifi-helper.h"

#include <cmath>
#include <iostream>

using namespace ns3;

/**
 * \defgroup aodvv2-examples AODVv2 Examples
 * \ingroup aodvv2
 * \ingroup examples
 */

/**
 * \ingroup aodvv2-examples
 * \ingroup examples
 * \brief Test script.
 *
 * This script creates a random network topology and then send some pings from random nodes:
 *
 *  Examples:
 *
 *  Node1 ------ Node2      Node1 ----- Node2
 *    |        /    |         |         |   \
 *    |       /     |         |         |    Node3
 *  Node3    /    Node4     Node4 ------+-----^
 *    |     /                 |          \
 *    |    /                  |          Node5
 *  Node5                   Node6
 *
 */
class Aodvv2MultiExample
{
  public:
    Aodvv2MultiExample();
    /**
     * \brief Configure script parameters
     * \param argc is the command line argument count
     * \param argv is the command line arguments
     * \return true on successful configuration
     */
    bool Configure(int argc, char** argv);
    /// Run simulation
    void Run();
    /**
     * Report results
     * \param os the output stream
     */
    void Report(std::ostream& os);

  private:
    // parameters
    /// Number of nodes
    uint32_t size;
    /// Distance between nodes, meters
    double step;
    /// Simulation time, seconds
    double totalTime;
    /// Write per-device PCAP traces if true
    bool pcap;
    /// Print routes if true
    bool printRoutes;

    // network
    /// nodes used in the example
    NodeContainer nodes;
    /// devices used in the example
    NetDeviceContainer devices;
    /// interfaces used in the example
    Ipv4InterfaceContainer interfaces;

  private:
    /// Create the nodes
    void CreateNodes();
    /// Create the devices
    void CreateDevices();
    /// Create the network
    void InstallInternetStack();
    /// Create the simulation applications
    void InstallApplications();
    /// Print nodes positions
    void PrintNodes();
};

int
main(int argc, char** argv)
{
    Aodvv2MultiExample test;
    if (!test.Configure(argc, argv))
    {
        NS_FATAL_ERROR("Configuration failed. Aborted.");
    }

    test.Run();
    test.Report(std::cout);
    return 0;
}

//-----------------------------------------------------------------------------
Aodvv2MultiExample::Aodvv2MultiExample()
    : size(10),
      step(50),
      totalTime(10),
      pcap(true),
      printRoutes(true)
{
}

bool
Aodvv2MultiExample::Configure(int argc, char** argv)
{
    // Enable AODVv2 logs by default. Comment this if too noisy
    // LogComponentEnable("Aodvv2RoutingProtocol", LOG_LEVEL_ALL);

    SeedManager::SetSeed(12345);
    CommandLine cmd(__FILE__);

    cmd.AddValue("pcap", "Write PCAP traces.", pcap);
    cmd.AddValue("printRoutes", "Print routing table dumps.", printRoutes);
    cmd.AddValue("size", "Number of nodes.", size);
    cmd.AddValue("time", "Simulation time, s.", totalTime);
    cmd.AddValue("step", "Grid step, m", step);

    cmd.Parse(argc, argv);
    return true;
}

void
Aodvv2MultiExample::Run()
{
    //  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue (1)); //
    //  enable rts cts all the time.
    CreateNodes();
    CreateDevices();
    InstallInternetStack();
    InstallApplications();

    // PrintNodes();

    std::cout << "Starting simulation for " << totalTime << " s ...\n";

    Simulator::Stop(Seconds(totalTime));
    Simulator::Run();
    Simulator::Destroy();
}

void
Aodvv2MultiExample::Report(std::ostream&)
{
}

void
Aodvv2MultiExample::CreateNodes()
{
    std::cout << "Creating " << (unsigned)size << " nodes with random positions " << step
              << " m apart.\n";
    nodes.Create(size);
    // Name nodes
    for (uint32_t i = 0; i < size; ++i)
    {
        std::ostringstream os;
        os << "node-" << i;
        Names::Add(os.str(), nodes.Get(i));
    }
    // Create random positions
    MobilityHelper mobility;
    Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
    mobility.SetPositionAllocator(
        "ns3::RandomRectanglePositionAllocator",
        "X",
        StringValue("ns3::UniformRandomVariable[Min=0.0|Max=" + std::to_string(step * (size / 2)) +
                    "]"),
        "Y",
        StringValue("ns3::UniformRandomVariable[Min=0.0|Max=" + std::to_string(step * (size / 2)) +
                    "]"));

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);
}

void
Aodvv2MultiExample::CreateDevices()
{
    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");
    YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    wifiPhy.SetChannel(wifiChannel.Create());
    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("OfdmRate6Mbps"),
                                 "RtsCtsThreshold",
                                 UintegerValue(0));
    devices = wifi.Install(wifiPhy, wifiMac, nodes);

    if (pcap)
    {
        wifiPhy.EnablePcapAll(std::string("aodvv2"));
    }
}

void
Aodvv2MultiExample::InstallInternetStack()
{
    Aodvv2Helper<Ipv4RoutingHelper> aodvv2;
    // you can configure AODVv2 attributes here using aodvv2.Set(name, value)
    InternetStackHelper stack;
    stack.SetRoutingHelper(aodvv2); // has effect on the next Install ()
    stack.Install(nodes);
    Ipv4AddressHelper address;
    address.SetBase("10.0.0.0", "255.0.0.0");
    interfaces = address.Assign(devices);

    if (printRoutes)
    {
        Ptr<OutputStreamWrapper> routingStream =
            Create<OutputStreamWrapper>("aodvv2.routes", std::ios::out);
        Ipv4RoutingHelper::PrintRoutingTableAllAt(Seconds(8), routingStream);
    }
}

void
Aodvv2MultiExample::InstallApplications()
{
    Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();

    for (uint32_t i = 0; i < size / 2; ++i)
    {
        uint32_t srcNodeIndex = rand->GetInteger(0, size - 1);
        uint32_t dstNodeIndex = rand->GetInteger(0, size - 1);

        while (dstNodeIndex == srcNodeIndex)
        {
            dstNodeIndex = rand->GetInteger(0, size - 1);
        }

        PingHelper ping(interfaces.GetAddress(dstNodeIndex));
        ping.SetAttribute("VerboseMode", EnumValue(Ping::VerboseMode::VERBOSE));

        ApplicationContainer p = ping.Install(nodes.Get(srcNodeIndex));
        p.Start(Seconds(0));
        p.Stop(Seconds(totalTime) - Seconds(0.001));
    }
}

void
Aodvv2MultiExample::PrintNodes()
{
    for (uint32_t i = 0; i < size; ++i)
    {
        Ptr<MobilityModel> mobility = nodes.Get(i)->GetObject<MobilityModel>();
        std::cout << "Node " << i << " at " << mobility->GetPosition() << std::endl;
    }
}
