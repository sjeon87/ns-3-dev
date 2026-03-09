/*
 * Copyright (c) 2026 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Jayesh Akot <akotjayesh@gmail.com>
 *          S B L Prateek <sblprateek@gmail.com>
 *          A R Sharan Kumar <arsharankumar99@gmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */

// This program simulates two competing TCP flows sharing a bottleneck link
// to evaluate rLEDBAT (RFC 9840) coexistence with a standard TCP variant.
//
// Topology:
//
//            1000 Mbps, 5ms                          1000 Mbps, 5ms
//  Sender1 -----------------+                   +---------------- Receiver1
//                           R1 -- 10Mbps,10ms -- R2
//  Sender2 -----------------+                   +---------------- Receiver2
//            1000 Mbps, 5ms                          1000 Mbps, 5ms
//
// Flow 1 (Sender1 -> Receiver1, port 50001):
//   rLEDBAT receiver (RFC 9840): TcpL4Protocol::SocketImplType is set to
//   TcpRLedbat on Receiver1 via Config::Set after InternetStackHelper::Install.
//   Receiver1's accepted sockets advertise RLWND derived from TCP
//   Timestamp-based OWD measurements; all other nodes use TcpSocketBase.
//   Active: t=0.1s to t=stopTime (default 350s).
//
// Flow 2 (Sender2 -> Receiver2, port 50002):
//   Standard TCP (TcpCubic by default) on both endpoints.
//   Active: t=flow2Start (default 50s) to t=flow2Stop (default 300s).
//
// This program runs by default for 350 seconds and creates a new directory
// called 'r-ledbat-results' in the ns-3 root directory. The program creates
// one sub-directory called 'pcap' in 'r-ledbat-results' directory (if pcap
// generation is enabled) and eight .dat files:
//
// (1) 'pcap' sub-directory contains pcap traces captured on all interfaces:
//     * r-ledbat-0-0.pcap  for the interface on Sender1
//     * r-ledbat-1-0.pcap  for the interface on Sender2
//     * r-ledbat-2-0.pcap  for the interface on Receiver1
//     * r-ledbat-3-0.pcap  for the interface on Receiver2
//     * r-ledbat-4-0.pcap  for the first interface on R1  (edge, towards Sender1)
//     * r-ledbat-4-1.pcap  for the second interface on R1 (edge, towards Sender2)
//     * r-ledbat-4-2.pcap  for the third interface on R1  (bottleneck egress)
//     * r-ledbat-5-0.pcap  for the first interface on R2  (bottleneck ingress)
//     * r-ledbat-5-1.pcap  for the second interface on R2 (edge, towards Receiver1)
//     * r-ledbat-5-2.pcap  for the third interface on R2  (edge, towards Receiver2)
// (2) flow1-cwnd.dat          contains congestion window trace for Sender1 (segments)
// (3) flow1-bytesInFlight.dat contains bytes in flight trace for Sender1 (segments)
// (4) flow2-cwnd.dat          contains congestion window trace for Sender2 (segments)
// (5) flow2-bytesInFlight.dat contains bytes in flight trace for Sender2 (segments)
// (6) flow1-throughput.dat    contains sender-side throughput for Flow 1 (Mbit/s)
// (7) flow2-throughput.dat    contains sender-side throughput for Flow 2 (Mbit/s)
// (8) queueSize.dat           contains bottleneck queue occupancy (packets)
// (9) sojournTime.dat         contains per-packet sojourn time at bottleneck (ms)

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;
using namespace ns3::SystemPath;

std::string dir;
std::ofstream flow1Throughput;
std::ofstream flow2Throughput;
std::ofstream queueSize;
std::ofstream sojournTime;

uint32_t g_flow1PrevBytes = 0;
uint32_t g_flow2PrevBytes = 0;
Time g_flow1PrevTime;
Time g_flow2PrevTime;

// Calculate throughput for both flows
static void
TraceThroughput(Ptr<FlowMonitor> monitor, Ptr<Ipv4FlowClassifier> classifier)
{
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
    Time curTime = Now();

    for (auto& kv : stats)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(kv.first);

        if (t.destinationPort == 50001)
        {
            // Convert (curTime - prevTime) to microseconds so that throughput is in
            // bits per microsecond (which is equivalent to Mbps)
            flow1Throughput << curTime.GetSeconds() << "s "
                            << 8.0 * (kv.second.txBytes - g_flow1PrevBytes) /
                                   (curTime - g_flow1PrevTime).ToDouble(Time::US)
                            << " Mbps" << std::endl;
            g_flow1PrevTime = curTime;
            g_flow1PrevBytes = kv.second.txBytes;
        }
        else if (t.destinationPort == 50002)
        {
            // Convert (curTime - prevTime) to microseconds so that throughput is in
            // bits per microsecond (which is equivalent to Mbps)
            flow2Throughput << curTime.GetSeconds() << "s "
                            << 8.0 * (kv.second.txBytes - g_flow2PrevBytes) /
                                   (curTime - g_flow2PrevTime).ToDouble(Time::US)
                            << " Mbps" << std::endl;
            g_flow2PrevTime = curTime;
            g_flow2PrevBytes = kv.second.txBytes;
        }
    }

    Simulator::Schedule(Seconds(0.2), &TraceThroughput, monitor, classifier);
}

// Check the queue size
void
CheckQueueSize(Ptr<QueueDisc> qd)
{
    uint32_t qsize = qd->GetCurrentSize().GetValue();
    Simulator::Schedule(Seconds(0.2), &CheckQueueSize, qd);
    queueSize << Simulator::Now().GetSeconds() << " " << qsize << std::endl;
}

// Trace sojourn time
static void
SojournTimeTracer(Time sojourn)
{
    sojournTime << Simulator::Now().GetSeconds() << " " << sojourn.GetMilliSeconds() << std::endl;
}

// Connect the sojourn time trace source
void
TraceSojournTime(Ptr<QueueDisc> qd)
{
    qd->TraceConnectWithoutContext("SojournTime", MakeCallback(&SojournTimeTracer));
}

// Trace congestion window
static void
CwndTracer(Ptr<OutputStreamWrapper> stream, uint32_t oldval, uint32_t newval)
{
    *stream->GetStream() << Simulator::Now().GetSeconds() << " " << newval / 1448.0 << std::endl;
}

void
TraceCwnd(uint32_t nodeId, uint32_t socketId, std::string filename)
{
    AsciiTraceHelper ascii;
    Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream(filename);
    Config::ConnectWithoutContext("/NodeList/" + std::to_string(nodeId) +
                                      "/$ns3::TcpL4Protocol/SocketList/" +
                                      std::to_string(socketId) + "/CongestionWindow",
                                  MakeBoundCallback(&CwndTracer, stream));
}

// Trace bytes in flight
static void
BytesInFlightTracer(Ptr<OutputStreamWrapper> stream, uint32_t oldval, uint32_t newval)
{
    *stream->GetStream() << Simulator::Now().GetSeconds() << " " << newval / 1448.0 << std::endl;
}

void
TraceBytesInFlight(uint32_t nodeId, uint32_t socketId, std::string filename)
{
    AsciiTraceHelper ascii;
    Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream(filename);
    Config::ConnectWithoutContext("/NodeList/" + std::to_string(nodeId) +
                                      "/$ns3::TcpL4Protocol/SocketList/" +
                                      std::to_string(socketId) + "/BytesInFlight",
                                  MakeBoundCallback(&BytesInFlightTracer, stream));
}

int
main(int argc, char* argv[])
{
    // Naming the output directory using local system time
    time_t rawtime;
    struct tm* timeinfo;
    char buffer[80];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, sizeof(buffer), "%d-%m-%Y-%I-%M-%S", timeinfo);
    std::string currentTime(buffer);

    std::string tcpTypeId1 = "TcpCubic";
    std::string tcpTypeId2 = "TcpCubic";
    std::string queueDisc = "FifoQueueDisc";
    uint32_t delAckCount = 2;
    bool bql = true;
    bool enablePcap = false;
    Time stopTime = Seconds(350);
    Time flow2Start = Seconds(50);
    Time flow2Stop = Seconds(300);

    CommandLine cmd(__FILE__);
    cmd.AddValue("tcpTypeId1", "TCP variant for Sender1: TcpNewReno, TcpCubic", tcpTypeId1);
    cmd.AddValue("tcpTypeId2", "TCP variant for Sender2: TcpNewReno, TcpCubic", tcpTypeId2);
    cmd.AddValue("delAckCount", "Delayed ACK count", delAckCount);
    cmd.AddValue("enablePcap", "Enable/Disable pcap file generation", enablePcap);
    cmd.AddValue("stopTime",
                 "Stop time for applications / simulation time will be stopTime + 1",
                 stopTime);
    cmd.AddValue("flow2Start", "Start time for Flow 2", flow2Start);
    cmd.AddValue("flow2Stop", "Stop time for Flow 2", flow2Stop);
    cmd.Parse(argc, argv);

    queueDisc = std::string("ns3::") + queueDisc;

    // The maximum send buffer size is set to 4194304 bytes (4MB) and the
    // maximum receive buffer size is set to 6291456 bytes (6MB) in the Linux
    // kernel. The same buffer sizes are used as default in this example.
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(4194304));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(6291456));
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(10));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(delAckCount));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));
    Config::SetDefault("ns3::DropTailQueue<Packet>::MaxSize", QueueSizeValue(QueueSize("1p")));
    Config::SetDefault(queueDisc + "::MaxSize", QueueSizeValue(QueueSize("100p")));

    NodeContainer sender1;
    NodeContainer sender2;
    NodeContainer receiver1;
    NodeContainer receiver2;
    NodeContainer routers;
    sender1.Create(1);
    sender2.Create(1);
    receiver1.Create(1);
    receiver2.Create(1);
    routers.Create(2);

    // Create the point-to-point link helpers
    PointToPointHelper bottleneckLink;
    bottleneckLink.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    bottleneckLink.SetChannelAttribute("Delay", StringValue("10ms"));

    PointToPointHelper edgeLink;
    edgeLink.SetDeviceAttribute("DataRate", StringValue("1000Mbps"));
    edgeLink.SetChannelAttribute("Delay", StringValue("5ms"));

    // Create NetDevice containers
    NetDeviceContainer sender1Edge = edgeLink.Install(sender1.Get(0), routers.Get(0));
    NetDeviceContainer sender2Edge = edgeLink.Install(sender2.Get(0), routers.Get(0));
    NetDeviceContainer r1r2 = bottleneckLink.Install(routers.Get(0), routers.Get(1));
    NetDeviceContainer receiver1Edge = edgeLink.Install(routers.Get(1), receiver1.Get(0));
    NetDeviceContainer receiver2Edge = edgeLink.Install(routers.Get(1), receiver2.Get(0));

    // Install Stack
    InternetStackHelper internet;
    internet.Install(sender1);
    internet.Install(sender2);
    internet.Install(receiver1);
    internet.Install(receiver2);
    internet.Install(routers);

    // Set socket type per receiver after InternetStack is installed.
    // NodeList indices are assigned in Create() call order:
    //   NodeList/0 = sender1, NodeList/1 = sender2,
    //   NodeList/2 = receiver1 (TcpRLedbat), NodeList/3 = receiver2 (TcpSocketBase),
    //   NodeList/4 = R1, NodeList/5 = R2
    Config::Set("/NodeList/0/$ns3::TcpL4Protocol/SocketType",
                TypeIdValue(TypeId::LookupByName("ns3::" + tcpTypeId1)));
    Config::Set("/NodeList/1/$ns3::TcpL4Protocol/SocketType",
                TypeIdValue(TypeId::LookupByName("ns3::" + tcpTypeId2)));
    Config::Set("/NodeList/2/$ns3::TcpL4Protocol/SocketImplType",
                TypeIdValue(TcpRLedbat::GetTypeId()));
    Config::Set("/NodeList/3/$ns3::TcpL4Protocol/SocketImplType",
                TypeIdValue(TcpSocketBase::GetTypeId()));

    // Configure the root queue discipline
    TrafficControlHelper tch;
    tch.SetRootQueueDisc(queueDisc);
    if (bql)
    {
        tch.SetQueueLimits("ns3::DynamicQueueLimits", "HoldTime", StringValue("1000ms"));
    }
    tch.Install(sender1Edge);
    tch.Install(sender2Edge);
    tch.Install(receiver1Edge);
    tch.Install(receiver2Edge);

    // Install queue discipline on the bottleneck device separately so its
    // QueueDiscContainer is available for tracing without reinstallation.
    TrafficControlHelper tchBottleneck;
    tchBottleneck.SetRootQueueDisc(queueDisc);
    if (bql)
    {
        tchBottleneck.SetQueueLimits("ns3::DynamicQueueLimits", "HoldTime", StringValue("1000ms"));
    }
    QueueDiscContainer qdBottleneck = tchBottleneck.Install(r1r2.Get(0));

    // Assign IP addresses
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer i1i2 = ipv4.Assign(r1r2);

    ipv4.NewNetwork();
    Ipv4InterfaceContainer iSender1 = ipv4.Assign(sender1Edge);

    ipv4.NewNetwork();
    Ipv4InterfaceContainer iSender2 = ipv4.Assign(sender2Edge);

    ipv4.NewNetwork();
    Ipv4InterfaceContainer iReceiver1 = ipv4.Assign(receiver1Edge);

    ipv4.NewNetwork();
    Ipv4InterfaceContainer iReceiver2 = ipv4.Assign(receiver2Edge);

    // Populate routing tables
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Create a new directory to store the output of the program
    dir = "r-ledbat-results/" + currentTime + "/";
    MakeDirectories(dir);

    // Open files for writing throughput traces and queue size
    flow1Throughput.open(dir + "flow1-throughput.dat", std::ios::out);
    flow2Throughput.open(dir + "flow2-throughput.dat", std::ios::out);
    queueSize.open(dir + "queueSize.dat", std::ios::out);
    sojournTime.open(dir + "sojournTime.dat", std::ios::out);

    NS_ASSERT_MSG(flow1Throughput.is_open(), "Flow 1 throughput file was not opened correctly");
    NS_ASSERT_MSG(flow2Throughput.is_open(), "Flow 2 throughput file was not opened correctly");
    NS_ASSERT_MSG(queueSize.is_open(), "Queue size file was not opened correctly");
    NS_ASSERT_MSG(sojournTime.is_open(), "Sojourn time file was not opened correctly");

    // Install application on Sender1 (Flow 1: rLEDBAT)
    BulkSendHelper source1("ns3::TcpSocketFactory",
                           InetSocketAddress(iReceiver1.GetAddress(1), 50001));
    source1.SetAttribute("MaxBytes", UintegerValue(0));
    ApplicationContainer sourceApps1 = source1.Install(sender1.Get(0));
    sourceApps1.Start(Seconds(0.1));
    // Hook trace source after application starts
    Simulator::Schedule(Seconds(0.1) + MilliSeconds(1), &TraceCwnd, 0, 0, dir + "flow1-cwnd.dat");
    Simulator::Schedule(Seconds(0.1) + MilliSeconds(1),
                        &TraceBytesInFlight,
                        0,
                        0,
                        dir + "flow1-bytesInFlight.dat");
    sourceApps1.Stop(stopTime);

    // Install application on Receiver1
    PacketSinkHelper sink1("ns3::TcpSocketFactory",
                           InetSocketAddress(Ipv4Address::GetAny(), 50001));
    ApplicationContainer sinkApps1 = sink1.Install(receiver1.Get(0));
    sinkApps1.Start(Seconds(0));
    sinkApps1.Stop(stopTime);

    // Install application on Sender2 (Flow 2: TcpCubic)
    BulkSendHelper source2("ns3::TcpSocketFactory",
                           InetSocketAddress(iReceiver2.GetAddress(1), 50002));
    source2.SetAttribute("MaxBytes", UintegerValue(0));
    ApplicationContainer sourceApps2 = source2.Install(sender2.Get(0));
    sourceApps2.Start(flow2Start);
    // Hook trace source after application starts
    Simulator::Schedule(flow2Start + MilliSeconds(1), &TraceCwnd, 1, 0, dir + "flow2-cwnd.dat");
    Simulator::Schedule(flow2Start + MilliSeconds(1),
                        &TraceBytesInFlight,
                        1,
                        0,
                        dir + "flow2-bytesInFlight.dat");
    sourceApps2.Stop(flow2Stop);

    // Install application on Receiver2
    PacketSinkHelper sink2("ns3::TcpSocketFactory",
                           InetSocketAddress(Ipv4Address::GetAny(), 50002));
    ApplicationContainer sinkApps2 = sink2.Install(receiver2.Get(0));
    sinkApps2.Start(flow2Start);
    sinkApps2.Stop(flow2Stop);

    // Trace the queue occupancy on the bottleneck link
    Simulator::ScheduleNow(&CheckQueueSize, qdBottleneck.Get(0));
    Simulator::ScheduleNow(&TraceSojournTime, qdBottleneck.Get(0));

    // Generate PCAP traces if it is enabled
    if (enablePcap)
    {
        MakeDirectories(dir + "pcap/");
        bottleneckLink.EnablePcapAll(dir + "pcap/r-ledbat", true);
    }

    // Check for dropped packets using Flow Monitor
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());

    g_flow1PrevTime = Seconds(0);
    g_flow2PrevTime = Seconds(0);

    Simulator::Schedule(Seconds(0 + 0.000001), &TraceThroughput, monitor, classifier);

    Simulator::Stop(stopTime + TimeStep(1));
    Simulator::Run();
    Simulator::Destroy();

    flow1Throughput.close();
    flow2Throughput.close();
    queueSize.close();
    sojournTime.close();

    return 0;
}
