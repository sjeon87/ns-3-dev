/*
 * Copyright (c) 2026 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Wenying Dai          <daiwenying927@gmail.com>
 *          Deepak Kumaraswamy   <deepak.kumaraswamy@gmail.com>
 *          Mohit P. Tahiliani   <tahiliani@nitk.edu.in>
 *          Prathapa Hasitha     <prathapahasitha@gmail.com>
 */

// This program simulates the following topology:
//
//           1000 Mbps, 2ms           1 Mbps, 5ms          1000 Mbps, 2ms
//  n0 (Sender 0) ---------- R1 -------------------- R2 ---------- n4 (Receiver 0)
//                           |                        |
//  n1 (Sender 1) -----------+                        +----------- n5 (Receiver 1)
//           1000 Mbps, 2ms                                1000 Mbps, 2ms
//
// n0 sends to n4, n1 sends to n5. Both flows share the bottleneck R1-R2.
//
// ECN++ (UseEcnPlusPlus attribute) is enabled on all TCP sockets by default.
// A CoDel AQM with ECN is installed on the bottleneck link. The program
// measures the congestion window of both senders and writes results to the
// output directory.
//
// Usage:
//   ./ns3 run tcp-ecnpp-example
//   ./ns3 run "tcp-ecnpp-example --useEcnPlusPlus=false" (Classic ECN instead)
//
// Output files are written to 'ecnpp-results/<timestamp>/':
//   cwnd0.dat   - congestion window trace for n0
//   cwnd1.dat   - congestion window trace for n1
//   throughput.dat - sender-side throughput
//   queueSize.dat  - bottleneck queue occupancy
//   pcap/       - optional PCAP captures

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"

#include <filesystem>

using namespace ns3;
using namespace ns3::SystemPath;

NS_LOG_COMPONENT_DEFINE("TcpEcnPpExample");

std::string dir;          //!< Output directory path
std::ofstream throughput; //!< Throughput output stream
std::ofstream queueSize;  //!< Queue size output stream

uint32_t prevTxBytes0{0}; //!< Previously transmitted bytes for n0 throughput calculation
uint32_t prevTxBytes1{0}; //!< Previously transmitted bytes for n1 throughput calculation
Time prevTime;            //!< Previous time for throughput calculation

/**
 * Traces the throughput of both flows using the flow monitor.
 *
 * @param monitor Pointer to the FlowMonitor instance.
 */
static void
TraceThroughput(Ptr<FlowMonitor> monitor)
{
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
    Time curTime = Now();
    double elapsed = (curTime - prevTime).ToDouble(Time::US);

    if (!stats.empty())
    {
        auto it = stats.begin();
        throughput << curTime.GetSeconds() << "s "
                   << 8.0 * (it->second.txBytes - prevTxBytes0) / elapsed << " Mbps (flow 0)"
                   << std::endl;
        prevTxBytes0 = it->second.txBytes;
    }
    if (stats.size() >= 2)
    {
        auto it = std::next(stats.begin());
        throughput << curTime.GetSeconds() << "s "
                   << 8.0 * (it->second.txBytes - prevTxBytes1) / elapsed << " Mbps (flow 1)"
                   << std::endl;
        prevTxBytes1 = it->second.txBytes;
    }

    prevTime = curTime;
    Simulator::Schedule(Seconds(0.2), &TraceThroughput, monitor);
}

/**
 * Periodically samples and records bottleneck queue occupancy.
 *
 * @param qd Pointer to the queue discipline on the bottleneck link.
 */
void
CheckQueueSize(Ptr<QueueDisc> qd)
{
    queueSize << Simulator::Now().GetSeconds() << " " << qd->GetCurrentSize().GetValue()
              << std::endl;
    Simulator::Schedule(Seconds(0.2), &CheckQueueSize, qd);
}

/**
 * Writes a congestion window sample to the given output stream.
 *
 * @param stream Output stream wrapper to write to.
 * @param oldval Previous congestion window value (bytes).
 * @param newval New congestion window value (bytes).
 */
static void
CwndTracer(Ptr<OutputStreamWrapper> stream, uint32_t oldval [[maybe_unused]], uint32_t newval)
{
    *stream->GetStream() << Simulator::Now().GetSeconds() << " " << newval / 1448.0 << std::endl;
}

/**
 * Connects the congestion window trace for the given socket to a .dat file.
 *
 * @param filename Output file name (relative to dir).
 * @param nodeId   Node list index of the sender.
 * @param socketId Socket list index on that node.
 */
void
TraceCwnd(const std::string& filename, uint32_t nodeId, uint32_t socketId)
{
    AsciiTraceHelper ascii;
    Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream(dir + "/" + filename);
    Config::ConnectWithoutContext("/NodeList/" + std::to_string(nodeId) +
                                      "/$ns3::TcpL4Protocol/SocketList/" +
                                      std::to_string(socketId) + "/CongestionWindow",
                                  MakeBoundCallback(&CwndTracer, stream));
}

int
main(int argc, char* argv[])
{
    // Build a timestamped output directory name
    time_t rawtime;
    struct tm* timeinfo;
    char buffer[80];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, sizeof(buffer), "%d-%m-%Y-%I-%M-%S", timeinfo);
    std::string currentTime(buffer);

    bool useEcnPlusPlus = true; //!< true = EcnPlusPlus mode; false = Classic ECN
    bool enablePcap = false;    //!< Enable PCAP capture
    Time stopTime = Seconds(100);

    CommandLine cmd(__FILE__);
    cmd.AddValue("useEcnPlusPlus",
                 "Use ECN++ (EcnPlusPlus) mode; set to false for Classic ECN",
                 useEcnPlusPlus);
    cmd.AddValue("enablePcap", "Enable PCAP file generation", enablePcap);
    cmd.AddValue("stopTime", "Simulation stop time", stopTime);
    cmd.Parse(argc, argv);

    // TCP socket defaults
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpCubic"));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(4194304));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(6291456));
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(10));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(2));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));
    Config::SetDefault("ns3::DropTailQueue<Packet>::MaxSize", QueueSizeValue(QueueSize("1p")));

    // ECN configuration:
    // UseEcnPlusPlus enables ECT marking of TCP control packets (SYN/ACK, Window
    // Probe, FIN, RST, retransmissions). It also enables ECN automatically.
    // For Classic ECN we enable UseEcn explicitly.
    Config::SetDefault("ns3::TcpSocketBase::UseEcn", EnumValue(TcpSocketState::On));
    if (useEcnPlusPlus)
    {
        Config::SetDefault("ns3::TcpSocketBase::UseEcnPlusPlus", BooleanValue(true));
    }

    // CoDel AQM on the bottleneck with ECN marking enabled
    Config::SetDefault("ns3::CoDelQueueDisc::MaxSize", QueueSizeValue(QueueSize("1000p")));
    Config::SetDefault("ns3::CoDelQueueDisc::Interval", TimeValue(MilliSeconds(100)));
    Config::SetDefault("ns3::CoDelQueueDisc::Target", TimeValue(MilliSeconds(5)));
    Config::SetDefault("ns3::CoDelQueueDisc::UseEcn", BooleanValue(true));

    // Create nodes
    NodeContainer senders;
    senders.Create(2); // n0, n1
    NodeContainer receivers;
    receivers.Create(2); // n4, n5
    NodeContainer routers;
    routers.Create(2); // R1 (n2), R2 (n3)

    // Link helpers
    PointToPointHelper edgeLink;
    edgeLink.SetDeviceAttribute("DataRate", StringValue("1000Mbps"));
    edgeLink.SetChannelAttribute("Delay", StringValue("2ms"));

    PointToPointHelper bottleneckLink;
    bottleneckLink.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
    bottleneckLink.SetChannelAttribute("Delay", StringValue("5ms"));

    // Install links
    NetDeviceContainer devN0R1 = edgeLink.Install(senders.Get(0), routers.Get(0));
    NetDeviceContainer devN1R1 = edgeLink.Install(senders.Get(1), routers.Get(0));
    NetDeviceContainer devR1R2 = bottleneckLink.Install(routers.Get(0), routers.Get(1));
    NetDeviceContainer devR2N4 = edgeLink.Install(routers.Get(1), receivers.Get(0));
    NetDeviceContainer devR2N5 = edgeLink.Install(routers.Get(1), receivers.Get(1));

    // Install internet stack
    InternetStackHelper internet;
    internet.Install(senders);
    internet.Install(receivers);
    internet.Install(routers);

    // Install CoDel on the bottleneck link at R1
    TrafficControlHelper tch;
    tch.SetRootQueueDisc("ns3::CoDelQueueDisc");
    tch.SetQueueLimits("ns3::DynamicQueueLimits", "HoldTime", StringValue("1000ms"));
    QueueDiscContainer bottleneckQd = tch.Install(devR1R2.Get(0)); // R1 egress toward R2

    // Assign IP addresses
    Ipv4AddressHelper ipv4;

    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i0r1 = ipv4.Assign(devN0R1);

    ipv4.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer i1r1 = ipv4.Assign(devN1R1);

    ipv4.SetBase("10.1.3.0", "255.255.255.0");
    ipv4.Assign(devR1R2);

    ipv4.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer ir2n4 = ipv4.Assign(devR2N4);

    ipv4.SetBase("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer ir2n5 = ipv4.Assign(devR2N5);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Applications: BulkSend on senders, PacketSink on receivers
    uint16_t port0 = 50000;
    uint16_t port1 = 50001;

    BulkSendHelper source0("ns3::TcpSocketFactory", InetSocketAddress(ir2n4.GetAddress(1), port0));
    source0.SetAttribute("MaxBytes", UintegerValue(0));
    ApplicationContainer sourceApp0 = source0.Install(senders.Get(0));
    sourceApp0.Start(Seconds(0.1));
    sourceApp0.Stop(stopTime);
    Simulator::Schedule(Seconds(0.1) + MilliSeconds(1), &TraceCwnd, "cwnd0.dat", 0, 0);

    BulkSendHelper source1("ns3::TcpSocketFactory", InetSocketAddress(ir2n5.GetAddress(1), port1));
    source1.SetAttribute("MaxBytes", UintegerValue(0));
    ApplicationContainer sourceApp1 = source1.Install(senders.Get(1));
    sourceApp1.Start(Seconds(0.1));
    sourceApp1.Stop(stopTime);
    Simulator::Schedule(Seconds(0.1) + MilliSeconds(1), &TraceCwnd, "cwnd1.dat", 1, 0);

    PacketSinkHelper sink0("ns3::TcpSocketFactory",
                           InetSocketAddress(Ipv4Address::GetAny(), port0));
    sink0.Install(receivers.Get(0)).Start(Seconds(0.0));

    PacketSinkHelper sink1("ns3::TcpSocketFactory",
                           InetSocketAddress(Ipv4Address::GetAny(), port1));
    sink1.Install(receivers.Get(1)).Start(Seconds(0.0));

    // Output directory
    dir = "ecnpp-results/" + currentTime;
    MakeDirectories(dir);

    // Queue size trace on bottleneck
    Simulator::ScheduleNow(&CheckQueueSize, bottleneckQd.Get(0));

    // PCAP
    if (enablePcap)
    {
        MakeDirectories(dir + "/pcap");
        bottleneckLink.EnablePcapAll(dir + "/pcap/ecnpp", true);
    }

    // Open output files
    throughput.open(dir + "/throughput.dat", std::ios::out);
    queueSize.open(dir + "/queueSize.dat", std::ios::out);
    NS_ASSERT_MSG(throughput.is_open(), "Throughput file could not be opened");
    NS_ASSERT_MSG(queueSize.is_open(), "Queue size file could not be opened");

    // Flow monitor for throughput
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    prevTime = Seconds(0.1);
    Simulator::Schedule(Seconds(0.1), &TraceThroughput, monitor);

    NS_LOG_INFO("Running ECN++ example simulation...");
    Simulator::Stop(stopTime + TimeStep(1));
    Simulator::Run();
    Simulator::Destroy();

    throughput.close();
    queueSize.close();

    NS_LOG_INFO("Done. Results written to: " << dir);
    return 0;
}
