/*
 * Copyright (c) 2026 Shivang Upadhyay
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Validation script for CAKE Figure 3 (Host Isolation).
 * Based on: arXiv:1804.07617 (Høiland-Jørgensen et al.)
 *
 * Topology:
 *   srcA, srcB --> router0 --> router1 --> dstA, dstB, dstC, dstD
 *
 * Flows (6 total):
 *   srcA -> dstA  (port 5001)
 *   srcA -> dstB  (port 5002)
 *   srcA -> dstC  (port 5003)
 *   srcA -> dstC  (port 5004)
 *   srcB -> dstC  (port 5005)
 *   srcB -> dstD  (port 5006)
 *
 * Run once per mode:
 *   --mode=cake         (no host isolation)
 *   --mode=cake_dst     (destination host isolation)
 *   --mode=cake_src     (source host isolation)
 *   --mode=cake_triple  (triple isolation)
 *   --mode=fq_codel
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"

#include <fstream>
#include <iomanip>
#include <map>

using namespace ns3;

std::map<uint16_t, Ptr<PacketSink>> globalSinks;
std::map<uint16_t, uint64_t> bytesAt10s;

void
RecordMidpointBytes()
{
    for (const auto& pair : globalSinks)
    {
        bytesAt10s[pair.first] = pair.second->GetTotalRx();
    }
}

NS_LOG_COMPONENT_DEFINE("CakeFig3");

int
main(int argc, char* argv[])
{
    std::string mode = "cake";
    std::string outputFile = "cake-fig3-results.dat";
    Time simStop = Seconds(60.0);
    uint32_t segSize = 1448;
    std::string bottleneckRate = "10Mbps";

    CommandLine cmd(__FILE__);
    cmd.AddValue("mode", "Mode: cake, cake_dst, cake_src, cake_triple, fq_codel", mode);
    cmd.AddValue("output", "Output file", outputFile);
    cmd.AddValue("simStop", "Simulation stop time", simStop);
    cmd.AddValue("rate", "Bottleneck and CAKE shaping rate", bottleneckRate);
    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(segSize));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1 << 20));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1 << 20));

    NodeContainer srcNodes;
    srcNodes.Create(2); // srcA=0, srcB=1

    NodeContainer routers;
    routers.Create(2); // router0=0, router1=1

    NodeContainer dstNodes;
    dstNodes.Create(4); // dstA=0, dstB=1, dstC=2, dstD=3

    PointToPointHelper accessHelper;
    accessHelper.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Mbps")));
    accessHelper.SetChannelAttribute("Delay", TimeValue(MilliSeconds(1)));
    accessHelper.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", StringValue("100p"));

    PointToPointHelper bottleneckHelper;
    bottleneckHelper.SetDeviceAttribute("DataRate", DataRateValue(DataRate(bottleneckRate)));
    bottleneckHelper.SetChannelAttribute("Delay", TimeValue(MilliSeconds(23)));
    bottleneckHelper.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", StringValue("100p"));

    std::vector<NetDeviceContainer> srcDevs(2);
    for (uint32_t i = 0; i < 2; ++i)
    {
        srcDevs[i] = accessHelper.Install(srcNodes.Get(i), routers.Get(0));
    }

    NetDeviceContainer bottleneckDevs = bottleneckHelper.Install(routers.Get(0), routers.Get(1));

    std::vector<NetDeviceContainer> dstDevs(4);
    for (uint32_t i = 0; i < 4; ++i)
    {
        dstDevs[i] = accessHelper.Install(routers.Get(1), dstNodes.Get(i));
    }

    InternetStackHelper internet;
    internet.Install(srcNodes);
    internet.Install(routers);
    internet.Install(dstNodes);

    TrafficControlHelper tch;
    if (mode == "fq_codel")
    {
        tch.SetRootQueueDisc("ns3::FqCoDelQueueDisc");
    }
    else
    {
        uint32_t IsolationMode = 0;
        if (mode == "cake_src")
        {
            IsolationMode = 1;
        }
        else if (mode == "cake_dst")
        {
            IsolationMode = 2;
        }
        else if (mode == "cake_triple")
        {
            IsolationMode = 3;
        }
        tch.SetRootQueueDisc("ns3::CakeQueueDisc",
                             "Bandwidth",
                             DataRateValue(DataRate(bottleneckRate)),
                             "DiffServMode",
                             UintegerValue(0),
                             "IsolationMode",
                             UintegerValue(IsolationMode),
                             "Overhead",
                             UintegerValue(38));
    }
    tch.Install(bottleneckDevs.Get(0));

    Ipv4AddressHelper ipv4;

    for (uint32_t i = 0; i < 2; ++i)
    {
        std::ostringstream base;
        base << "10.1." << (i + 1) << ".0";
        ipv4.SetBase(base.str().c_str(), "255.255.255.252");
        ipv4.Assign(srcDevs[i]);
    }

    ipv4.SetBase("10.2.0.0", "255.255.255.252");
    ipv4.Assign(bottleneckDevs);

    std::vector<Ipv4InterfaceContainer> dstIfaces(4);
    for (uint32_t i = 0; i < 4; ++i)
    {
        std::ostringstream base;
        base << "10.3." << (i + 1) << ".0";
        ipv4.SetBase(base.str().c_str(), "255.255.255.252");
        dstIfaces[i] = ipv4.Assign(dstDevs[i]);
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Flow spec: {srcNode, dstNode, port}
    struct FlowSpec
    {
        uint32_t src;
        uint32_t dst;
        uint16_t port;
    };

    const std::vector<FlowSpec> flows = {
        {0, 0, 5001}, // srcA -> dstA
        {0, 1, 5002}, // srcA -> dstB
        {0, 2, 5003}, // srcA -> dstC (flow 1)
        {0, 2, 5004}, // srcA -> dstC (flow 2)
        {1, 2, 5005}, // srcB -> dstC
        {1, 3, 5006}, // srcB -> dstD
    };

    for (const auto& fs : flows)
    {
        Ipv4Address dstAddr = dstIfaces[fs.dst].GetAddress(1);

        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",
                                    InetSocketAddress(Ipv4Address::GetAny(), fs.port));
        ApplicationContainer sinkApp = sinkHelper.Install(dstNodes.Get(fs.dst));
        sinkApp.Start(Seconds(0.0));
        sinkApp.Stop(simStop);

        globalSinks[fs.port] = DynamicCast<PacketSink>(sinkApp.Get(0));

        BulkSendHelper bulkHelper("ns3::TcpSocketFactory", InetSocketAddress(dstAddr, fs.port));
        bulkHelper.SetAttribute("MaxBytes", UintegerValue(0));
        bulkHelper.SetAttribute("SendSize", UintegerValue(segSize));
        ApplicationContainer srcApp = bulkHelper.Install(srcNodes.Get(fs.src));
        srcApp.Start(Seconds(1.0));
        srcApp.Stop(simStop);
    }

    Simulator::Schedule(Seconds(10.0), &RecordMidpointBytes);
    Simulator::Stop(simStop);
    Simulator::Run();

    // Calculate steady-state goodput (from 10s to 60s)
    std::map<uint16_t, double> goodputByPort;
    double steadyStateDuration = simStop.GetSeconds() - 10.0;

    for (const auto& fs : flows)
    {
        uint64_t totalBytes = globalSinks[fs.port]->GetTotalRx();
        uint64_t steadyBytes = totalBytes - bytesAt10s[fs.port];

        double goodput = (steadyBytes * 8.0) / (steadyStateDuration * 1e6);
        goodputByPort[fs.port] = goodput;
    }

    std::ifstream check(outputFile);
    bool needHeader = !check.good() || check.peek() == std::ifstream::traits_type::eof();
    check.close();

    std::ofstream out(outputFile, std::ios::app);
    if (needHeader)
    {
        out << "# mode\tA->A\tA->B\tA->C(1)\tA->C(2)\tB->C\tB->D\n";
    }
    out << mode;
    for (const auto& fs : flows)
    {
        auto it = goodputByPort.find(fs.port);
        double g = (it != goodputByPort.end()) ? it->second : 0.0;
        out << "\t" << std::fixed << std::setprecision(4) << g;
    }
    out << "\n";
    out.close();

    NS_LOG_INFO("Results appended to " << outputFile);
    Simulator::Destroy();
    return 0;
}
