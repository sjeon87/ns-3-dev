/*
 * Copyright (c) 2026 GPRT
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Maria Eduarda Veras <eduarda.martins@gprt.ufpe.br>
 *          Eduardo Freitas <eduardo.freitas@gprt.ufpe.br>
 *          Djamel Fawzi Hadj Sadok <jamel@gprt.ufpe.br>
 *
 * This work has been performed within the framework of the FAPESP Engineering
 * Research Center (ERC) Program under FAPESP grant agreement #2021/00199-8
 * (SMARTNESS).
 */

// This example demonstrates the DualPI2 AQM sharing a single bottleneck between
// a scalable L4S flow (DCTCP, steered into the L4S queue) and a Classic flow
// (TCP Cubic, served by the Classic queue). It shows how DualPI2 keeps the L4S
// queuing delay low while preserving approximate rate fairness between the two
// congestion controls.
//
// Topology:
//
//   senderL4S   (n0) ---+                      +--- receiverL4S   (n3)
//                       n2 --- bottleneck --- n5
//   senderCubic (n1) ---+                      +--- receiverCubic (n4)
//
// The access links are 1 Gbps; only the n2->n5 bottleneck is rate limited and
// carries the DualPI2 queue disc. Half of the base RTT is configured as the
// bottleneck one-way delay.
//
// The example writes a few trace files (per-flow congestion window and
// throughput, plus per-queue sojourn time) to the output directory and prints
// the number of marks and drops observed at the bottleneck.

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"

#include <fstream>
#include <iomanip>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Dualpi2Example");

namespace
{

uint32_t g_bytesReceivedL4s = 0;   //!< Bytes received by the L4S sink in the current interval
uint32_t g_bytesReceivedCubic = 0; //!< Bytes received by the Cubic sink in the current interval
uint32_t g_marks = 0;              //!< Total CE marks at the bottleneck queue disc
uint32_t g_drops = 0;              //!< Total drops at the bottleneck queue disc
uint32_t g_segmentSize = 1448;     //!< TCP segment size in bytes

/**
 * Trace a congestion window change, logging it in segments.
 * @param stream the output stream
 * @param oldCwnd the previous congestion window (bytes)
 * @param newCwnd the new congestion window (bytes)
 */
void
TraceCwnd(std::ofstream* stream, uint32_t oldCwnd, uint32_t newCwnd)
{
    *stream << Simulator::Now().GetSeconds() << " " << static_cast<double>(newCwnd) / g_segmentSize
            << std::endl;
}

/**
 * Trace a queue sojourn time, logging it in milliseconds.
 * @param stream the output stream
 * @param sojourn the measured sojourn time
 */
void
TraceSojourn(std::ofstream* stream, Time sojourn)
{
    *stream << Simulator::Now().GetSeconds() << " " << sojourn.GetSeconds() * 1000 << std::endl;
}

/// Accumulate bytes received by the L4S sink.
void
RxL4s(Ptr<const Packet> p, const Address&)
{
    g_bytesReceivedL4s += p->GetSize();
}

/// Accumulate bytes received by the Cubic sink.
void
RxCubic(Ptr<const Packet> p, const Address&)
{
    g_bytesReceivedCubic += p->GetSize();
}

/// Count a CE mark at the bottleneck queue disc.
void
MarkTrace(Ptr<const QueueDiscItem>, const char*)
{
    g_marks++;
}

/// Count a drop at the bottleneck queue disc.
void
DropTrace(Ptr<const QueueDiscItem>)
{
    g_drops++;
}

/**
 * Sample and reset the per-flow throughput counters.
 * @param stream the output stream
 * @param interval the sampling interval
 */
void
SampleThroughput(std::ofstream* stream, Time interval)
{
    double l4s = g_bytesReceivedL4s * 8.0 / interval.GetSeconds() / 1e6;
    double cubic = g_bytesReceivedCubic * 8.0 / interval.GetSeconds() / 1e6;
    *stream << Simulator::Now().GetSeconds() << " " << l4s << " " << cubic << std::endl;
    g_bytesReceivedL4s = 0;
    g_bytesReceivedCubic = 0;
    Simulator::Schedule(interval, &SampleThroughput, stream, interval);
}

/**
 * Connect a congestion window trace to the first socket of a node.
 * Scheduled after the socket has been created.
 * @param stream the output stream
 * @param nodeId the node whose socket is traced
 */
void
ConnectCwnd(std::ofstream* stream, uint32_t nodeId)
{
    std::ostringstream path;
    path << "/NodeList/" << nodeId << "/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow";
    Config::ConnectWithoutContext(path.str(), MakeBoundCallback(&TraceCwnd, stream));
}

/**
 * Connect a packet-sink Rx trace to a node.
 * @param nodeId the node whose sink is traced
 * @param cb the callback to invoke on each received packet
 */
void
ConnectRx(uint32_t nodeId, void (*cb)(Ptr<const Packet>, const Address&))
{
    std::ostringstream path;
    path << "/NodeList/" << nodeId << "/ApplicationList/*/$ns3::PacketSink/Rx";
    Config::ConnectWithoutContext(path.str(), MakeCallback(cb));
}

} // namespace

int
main(int argc, char* argv[])
{
    Time simulationStopTime = Seconds(20);
    Time baseRtt = MilliSeconds(50);
    Time throughputInterval = MilliSeconds(200);
    DataRate bottleneckRate("40Mbps");
    std::string l4sThreshold = "1ms";
    std::string outputDir = ".";

    CommandLine cmd(__FILE__);
    cmd.AddValue("baseRtt", "Base round-trip time", baseRtt);
    cmd.AddValue("bottleneckRate", "Data rate of the bottleneck link", bottleneckRate);
    cmd.AddValue("l4sThreshold", "DualPI2 L4S step marking threshold", l4sThreshold);
    cmd.AddValue("stopTime", "Simulation stop time", simulationStopTime);
    cmd.AddValue("outputDir", "Directory for the output trace files", outputDir);
    cmd.Parse(argc, argv);

    // The L4S flow uses DCTCP (with ECT(1)); the Classic flow uses Cubic.
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(g_segmentSize));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(8 << 20));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(8 << 20));
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(10));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(1));
    Config::SetDefault("ns3::TcpSocketState::EnablePacing", BooleanValue(true));
    Config::SetDefault("ns3::TcpSocketBase::UseEcn", StringValue("On"));
    Config::SetDefault("ns3::TcpDctcp::UseEct0", BooleanValue(false));

    Time oneWayDelay = baseRtt / 2;

    // Topology: two senders (n0, n1), left router (n2), right router (n5),
    // two receivers (n3, n4).
    NodeContainer nodes;
    nodes.Create(6);

    InternetStackHelper stack;
    stack.Install(nodes);

    // Per-node congestion control: DCTCP for the L4S endpoints, Cubic for the
    // Classic endpoints.
    Config::Set("/NodeList/0/$ns3::TcpL4Protocol/SocketType", TypeIdValue(TcpDctcp::GetTypeId()));
    Config::Set("/NodeList/3/$ns3::TcpL4Protocol/SocketType", TypeIdValue(TcpDctcp::GetTypeId()));
    Config::Set("/NodeList/1/$ns3::TcpL4Protocol/SocketType", TypeIdValue(TcpCubic::GetTypeId()));
    Config::Set("/NodeList/4/$ns3::TcpL4Protocol/SocketType", TypeIdValue(TcpCubic::GetTypeId()));

    PointToPointHelper p2p;
    p2p.SetQueue("ns3::DropTailQueue", "MaxSize", QueueSizeValue(QueueSize("3p")));
    p2p.SetDeviceAttribute("DataRate", DataRateValue(DataRate("1Gbps")));
    p2p.SetChannelAttribute("Delay", TimeValue(MicroSeconds(1)));

    NetDeviceContainer devL4sToRouter = p2p.Install(nodes.Get(0), nodes.Get(2));
    NetDeviceContainer devCubicToRouter = p2p.Install(nodes.Get(1), nodes.Get(2));
    NetDeviceContainer devRouterToL4s = p2p.Install(nodes.Get(5), nodes.Get(3));
    NetDeviceContainer devRouterToCubic = p2p.Install(nodes.Get(5), nodes.Get(4));

    // Bottleneck link n2 -> n5, rate limited and carrying the DualPI2 queue.
    p2p.SetChannelAttribute("Delay", TimeValue(oneWayDelay));
    p2p.SetDeviceAttribute("DataRate", DataRateValue(bottleneckRate));
    NetDeviceContainer devBottleneck = p2p.Install(nodes.Get(2), nodes.Get(5));

    // DualPI2 on the bottleneck; simple pfifo elsewhere.
    TrafficControlHelper tchDualPi2;
    tchDualPi2.SetRootQueueDisc("ns3::DualPi2QueueDisc",
                                "L4SMarkThreshold",
                                TimeValue(Time(l4sThreshold)));
    tchDualPi2.Install(devBottleneck.Get(0));

    TrafficControlHelper tchPfifo;
    tchPfifo.SetRootQueueDisc("ns3::PfifoFastQueueDisc");
    tchPfifo.Install(devBottleneck.Get(1));
    tchPfifo.Install(devL4sToRouter);
    tchPfifo.Install(devCubicToRouter);
    tchPfifo.Install(devRouterToL4s);
    tchPfifo.Install(devRouterToCubic);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    ipv4.Assign(devL4sToRouter);
    ipv4.SetBase("10.1.2.0", "255.255.255.0");
    ipv4.Assign(devCubicToRouter);
    ipv4.SetBase("10.1.3.0", "255.255.255.0");
    ipv4.Assign(devBottleneck);
    ipv4.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer ifL4sRcv = ipv4.Assign(devRouterToL4s);
    ipv4.SetBase("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer ifCubicRcv = ipv4.Assign(devRouterToCubic);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // L4S flow: n0 -> n3.
    uint16_t portL4s = 5000;
    BulkSendHelper l4sSender("ns3::TcpSocketFactory",
                             InetSocketAddress(ifL4sRcv.GetAddress(1), portL4s));
    l4sSender.SetAttribute("MaxBytes", UintegerValue(0));
    ApplicationContainer l4sSenderApp = l4sSender.Install(nodes.Get(0));
    l4sSenderApp.Start(MilliSeconds(1));
    l4sSenderApp.Stop(simulationStopTime - Seconds(1));

    PacketSinkHelper l4sSink("ns3::TcpSocketFactory",
                             InetSocketAddress(Ipv4Address::GetAny(), portL4s));
    ApplicationContainer l4sSinkApp = l4sSink.Install(nodes.Get(3));
    l4sSinkApp.Start(MilliSeconds(1));
    l4sSinkApp.Stop(simulationStopTime);

    // Classic flow: n1 -> n4.
    uint16_t portCubic = 5001;
    BulkSendHelper cubicSender("ns3::TcpSocketFactory",
                               InetSocketAddress(ifCubicRcv.GetAddress(1), portCubic));
    cubicSender.SetAttribute("MaxBytes", UintegerValue(0));
    ApplicationContainer cubicSenderApp = cubicSender.Install(nodes.Get(1));
    cubicSenderApp.Start(MilliSeconds(1));
    cubicSenderApp.Stop(simulationStopTime - Seconds(1));

    PacketSinkHelper cubicSink("ns3::TcpSocketFactory",
                               InetSocketAddress(Ipv4Address::GetAny(), portCubic));
    ApplicationContainer cubicSinkApp = cubicSink.Install(nodes.Get(4));
    cubicSinkApp.Start(MilliSeconds(1));
    cubicSinkApp.Stop(simulationStopTime);

    // Trace files.
    std::ofstream l4sCwndFile(outputDir + "/dualpi2-l4s-cwnd.dat");
    std::ofstream cubicCwndFile(outputDir + "/dualpi2-cubic-cwnd.dat");
    std::ofstream throughputFile(outputDir + "/dualpi2-throughput.dat");
    std::ofstream classicSojournFile(outputDir + "/dualpi2-classic-sojourn.dat");
    std::ofstream l4sSojournFile(outputDir + "/dualpi2-l4s-sojourn.dat");

    Ptr<TrafficControlLayer> tc = nodes.Get(2)->GetObject<TrafficControlLayer>();
    Ptr<QueueDisc> qd = tc->GetRootQueueDiscOnDevice(devBottleneck.Get(0));
    qd->TraceConnectWithoutContext("Mark", MakeCallback(&MarkTrace));
    qd->TraceConnectWithoutContext("Drop", MakeCallback(&DropTrace));
    qd->TraceConnectWithoutContext("ClassicSojournTime",
                                   MakeBoundCallback(&TraceSojourn, &classicSojournFile));
    qd->TraceConnectWithoutContext("L4sSojournTime",
                                   MakeBoundCallback(&TraceSojourn, &l4sSojournFile));

    // Sockets exist only after the senders start, so connect the per-socket
    // traces slightly afterwards.
    Simulator::Schedule(MilliSeconds(1) + TimeStep(1), &ConnectCwnd, &l4sCwndFile, 0);
    Simulator::Schedule(MilliSeconds(1) + TimeStep(1), &ConnectCwnd, &cubicCwndFile, 1);
    Simulator::Schedule(MilliSeconds(1) + TimeStep(1), &ConnectRx, 3, &RxL4s);
    Simulator::Schedule(MilliSeconds(1) + TimeStep(1), &ConnectRx, 4, &RxCubic);
    Simulator::Schedule(throughputInterval, &SampleThroughput, &throughputFile, throughputInterval);

    std::cout << "DualPI2 example: L4S (DCTCP) + Classic (Cubic) over a " << bottleneckRate
              << " bottleneck, base RTT " << baseRtt.As(Time::MS) << std::endl;

    Simulator::Stop(simulationStopTime);
    Simulator::Run();

    auto l4sTotal = DynamicCast<PacketSink>(l4sSinkApp.Get(0))->GetTotalRx() * 8.0 / 1e6;
    auto cubicTotal = DynamicCast<PacketSink>(cubicSinkApp.Get(0))->GetTotalRx() * 8.0 / 1e6;
    double seconds = simulationStopTime.GetSeconds();
    std::cout << "L4S goodput:     " << l4sTotal / seconds << " Mbps" << std::endl
              << "Classic goodput: " << cubicTotal / seconds << " Mbps" << std::endl
              << "Marks: " << g_marks << "  Drops: " << g_drops << std::endl;

    Simulator::Destroy();
    return 0;
}
