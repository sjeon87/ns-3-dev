/*
 * Copyright (c) 2026 Shivam Kumar
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Shivam Kumar <shivamkumar.scarage@gmail.com>
 */

// This example demonstrates the bufferbloat problem and how Active Queue
// Management (AQM) solves it.
//
// Bufferbloat occurs when large network buffers cause excessive queuing
// delay.  When a TCP flow saturates a bottleneck link with a large
// DropTail (PfifoFast) queue, packets queue up rather than being dropped
// early.  TCP congestion control does not react because no loss signal is
// generated, and RTT grows by hundreds of milliseconds -- degrading the
// experience for latency-sensitive traffic such as VoIP and gaming.
//
// FQ-CoDel (Fair Queuing with Controlled Delay) solves this by detecting
// incipient congestion via sojourn time and signalling the sender early.
// Under the same traffic load, RTT stays close to the propagation delay.
//
// Network topology
//
//                    10.1.1.0/24            10.1.2.0/24
//   n0 -------------------------------- n1 -------------------------------- n2
//       access link: 100 Mbps, 1 ms         bottleneck: bandwidth, delay
//                                           device queue: 1 packet
//                                           queue disc: queueDisc, queueSize
//
// n0 runs a BulkSendApplication (TCP) to n2 to saturate the bottleneck.
// n0 also runs Ping to n2 to measure RTT under load.
// n2 runs a PacketSink to receive the TCP traffic.
//
// Usage examples:
//
//   ./ns3 run "bufferbloat-example --queueDisc=PfifoFast"
//     -> Expect RTT to grow to several hundred ms (bufferbloat)
//
//   ./ns3 run "bufferbloat-example --queueDisc=FqCoDel"
//     -> Expect RTT to stay close to the base RTT (~42 ms)
//
// The per-packet RTT measurements are written to "bufferbloat-rtt.dat"
// (two columns: time_s rtt_ms).  To plot:
//
//   gnuplot -e "set xlabel 'Time (s)'; set ylabel 'RTT (ms)'; \
//     plot 'bufferbloat-rtt.dat' with lines title 'Ping RTT'"

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("BufferbloatExample");

/**
 * Callback invoked on each Ping RTT sample.
 *
 * Writes the simulation time and measured RTT to the output stream.
 *
 * @param stream The output stream wrapper for the RTT data file.
 * @param seq The ICMP sequence number (unused).
 * @param rtt The measured round-trip time.
 */
static void
PingRttTrace(Ptr<OutputStreamWrapper> stream, uint16_t seq [[maybe_unused]], Time rtt)
{
    *stream->GetStream() << Simulator::Now().GetSeconds() << " " << rtt.ToDouble(Time::MS)
                         << std::endl;
}

/**
 * Callback invoked once when Ping application stops.
 *
 * Prints the aggregate RTT statistics (min/avg/max/mdev) and packet loss
 * to standard output.
 *
 * @param report The Ping summary report.
 */
static void
PingReportTrace(const Ping::PingReport& report)
{
    std::cout << "\n--- Ping statistics ---" << std::endl;
    std::cout << report.m_transmitted << " packets transmitted, " << report.m_received
              << " received, " << report.m_loss << "% packet loss" << std::endl;
    std::cout << "rtt min/avg/max/mdev = " << report.m_rttMin << "/" << report.m_rttAvg << "/"
              << report.m_rttMax << "/" << report.m_rttMdev << " ms" << std::endl;
}

int
main(int argc, char* argv[])
{
    std::string queueDiscType = "PfifoFast";
    std::string bandwidth = "10Mbps";
    std::string delay = "20ms";
    std::string queueSize = "1000p";
    double simulationTime = 30.0;

    CommandLine cmd(__FILE__);
    cmd.AddValue("queueDisc", "Bottleneck queue disc type: PfifoFast or FqCoDel", queueDiscType);
    cmd.AddValue("bandwidth", "Bottleneck link bandwidth", bandwidth);
    cmd.AddValue("delay", "Bottleneck link one-way delay", delay);
    cmd.AddValue("queueSize", "Queue disc capacity (e.g. 1000p or 1500000B)", queueSize);
    cmd.AddValue("simulationTime", "Total simulation time in seconds", simulationTime);
    cmd.Parse(argc, argv);

    NS_ABORT_MSG_IF(queueDiscType != "PfifoFast" && queueDiscType != "FqCoDel",
                    "--queueDisc must be PfifoFast or FqCoDel");
    NS_ABORT_MSG_IF(simulationTime <= 2.0, "--simulationTime must be greater than 2 seconds");

    // Enable ICMP checksum computation
    GlobalValue::Bind("ChecksumEnabled", BooleanValue(true));

    // TCP configuration
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1 << 21));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1 << 21));

    // --- Topology: 3 nodes ---
    NodeContainer nodes;
    nodes.Create(3);

    // Access link: n0 -- n1 (high bandwidth, low delay, no bottleneck)
    PointToPointHelper accessLink;
    accessLink.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    accessLink.SetChannelAttribute("Delay", StringValue("1ms"));

    // Bottleneck link: n1 -- n2 (configurable, device queue forced to 1 packet
    // so that all queuing happens in the traffic control layer)
    PointToPointHelper bottleneckLink;
    bottleneckLink.SetDeviceAttribute("DataRate", StringValue(bandwidth));
    bottleneckLink.SetChannelAttribute("Delay", StringValue(delay));
    bottleneckLink.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p"));

    // Install network stacks (IPv4 only)
    InternetStackHelper stack;
    stack.SetIpv6StackInstall(false);
    stack.Install(nodes);

    // Install links
    NetDeviceContainer accessDevices = accessLink.Install(nodes.Get(0), nodes.Get(1));
    NetDeviceContainer bottleneckDevices = bottleneckLink.Install(nodes.Get(1), nodes.Get(2));

    // Traffic control on the bottleneck (n1 egress toward n2)
    TrafficControlHelper tch;
    tch.SetRootQueueDisc("ns3::" + queueDiscType + "QueueDisc",
                         "MaxSize",
                         QueueSizeValue(QueueSize(queueSize)));
    QueueDiscContainer qdiscs = tch.Install(bottleneckDevices.Get(0));

    // Assign IP addresses
    Ipv4AddressHelper address;

    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer accessInterfaces = address.Assign(accessDevices);

    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer bottleneckInterfaces = address.Assign(bottleneckDevices);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Ipv4Address serverAddress = bottleneckInterfaces.GetAddress(1); // n2

    // --- Applications ---

    // TCP sink on n2 (server)
    uint16_t port = 9;
    Address sinkAddress(InetSocketAddress(Ipv4Address::GetAny(), port));
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", sinkAddress);
    sinkHelper.SetAttribute("Protocol", TypeIdValue(TcpSocketFactory::GetTypeId()));
    ApplicationContainer sinkApps = sinkHelper.Install(nodes.Get(2));
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(simulationTime));

    // BulkSend TCP source on n0 (client) -- saturates the bottleneck
    BulkSendHelper bulkSend("ns3::TcpSocketFactory", InetSocketAddress(serverAddress, port));
    bulkSend.SetAttribute("SendSize", UintegerValue(1448));
    ApplicationContainer sendApps = bulkSend.Install(nodes.Get(0));
    sendApps.Start(Seconds(0.5));
    sendApps.Stop(Seconds(simulationTime - 0.5));

    // Ping from n0 to n2 -- measures RTT under load
    PingHelper pingHelper(serverAddress);
    pingHelper.SetAttribute("Interval", TimeValue(MilliSeconds(100)));
    pingHelper.SetAttribute("Size", UintegerValue(56));
    pingHelper.SetAttribute("VerboseMode", EnumValue(Ping::VerboseMode::SILENT));
    ApplicationContainer pingApps = pingHelper.Install(nodes.Get(0));
    pingApps.Start(Seconds(0.1));
    pingApps.Stop(Seconds(simulationTime));

    // --- Connect trace sources ---

    // Per-packet RTT trace to output file
    AsciiTraceHelper ascii;
    Ptr<OutputStreamWrapper> rttStream = ascii.CreateFileStream("bufferbloat-rtt.dat");
    *rttStream->GetStream() << "# Time(s) RTT(ms)" << std::endl;
    *rttStream->GetStream() << "# QueueDisc: " << queueDiscType << "  Bandwidth: " << bandwidth
                            << "  Delay: " << delay << "  QueueSize: " << queueSize << std::endl;

    Ptr<Ping> pingApp = DynamicCast<Ping>(pingApps.Get(0));
    NS_ASSERT_MSG(pingApp, "Failed to retrieve Ping application");
    pingApp->TraceConnectWithoutContext("Rtt", MakeBoundCallback(&PingRttTrace, rttStream));
    pingApp->TraceConnectWithoutContext("Report", MakeCallback(&PingReportTrace));

    // --- Run simulation ---
    Simulator::Stop(Seconds(simulationTime + 1.0));
    Simulator::Run();

    // Print TCP goodput summary
    uint64_t totalRx = DynamicCast<PacketSink>(sinkApps.Get(0))->GetTotalRx();
    double goodputMbps = totalRx * 8.0 / (simulationTime * 1e6);
    std::cout << "\n--- TCP goodput ---" << std::endl;
    std::cout << "Total bytes received: " << totalRx << std::endl;
    std::cout << "Average goodput: " << goodputMbps << " Mbit/s" << std::endl;

    // Print queue disc statistics
    std::cout << "\n--- Queue disc statistics ---" << std::endl;
    std::cout << qdiscs.Get(0)->GetStats() << std::endl;

    Simulator::Destroy();
    return 0;
}
