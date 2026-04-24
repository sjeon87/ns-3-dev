/*
 * Copyright (c) 2018-20 NITK Surathkal
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Aarti Nandagiri <aarti.nandagiri@gmail.com>
 *          Vivek Jain <jain.vivek.anand@gmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */

// This program simulates a generalized dumbbell topology with multiple senders and receivers:
//
//  Sender-1 ----\                /---- Receiver-1
//  Sender-2 -----\              /------ Receiver-2
//  Sender-3 ------Router-1-----Router-2------ Receiver-3
//  ...            (Bottleneck)              ...
//  Sender-n -----/              \------ Receiver-n
//    (1000 Mbps)  (10 Mbps)   (1000 Mbps)
//     (5ms)       (10ms)       (5ms)
//
// Parameters:
// - tcpTypeId: TCP variant to use (default: TcpBbr)
// - nLeaf: Number of sender/receiver leaf pairs (default: 1)
// - bottleneckBw: Bottleneck link bandwidth (default: 10Mbps)
// - bottleneckDelay: Bottleneck link delay (default: 10ms)
// - edgeBw: Edge link bandwidth (default: 1000Mbps)
// - edgeDelay: Edge link delay (default: 5ms)
//
// This program runs by default for 100 seconds and creates a new directory
// called 'bbr-results' in the ns-3 root directory. The program creates one
// sub-directory called 'pcap' in 'bbr-results' directory (if pcap generation
// is enabled) and multiple .dat files.
//
// Output files:
// (1) 'pcap' sub-directory contains PCAP files from all links (if enabled)
// (2) cwnd_X.dat files contain congestion window trace for each sender node
// (3) throughput_X.dat files contain throughput trace for each sender node
// (4) queueSize.dat file contains queue length trace from the bottleneck link
//
// Supported TCP variants: TcpNewReno, TcpReno, TcpTahoe, TcpWestwood, TcpBbr,
// TcpYeah, TcpIllinois, TcpScalable, TcpVegas, TcpBic, and others.

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;

std::string dir;

// Structure to track per-sender throughput data
struct ThroughputTracker
{
    uint64_t prevBytes = 0;
    Time prevTime = Seconds(0);
};

std::vector<ThroughputTracker> throughputTrackers;

// Calculate throughput for a specific sender
static void
TraceThroughputPerSender(Ptr<FlowMonitor> monitor, uint32_t senderId)
{
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
    Time curTime = Now();
    
    uint32_t flowId = senderId + 1;
    
    if (stats.find(flowId) != stats.end())
    {
        uint64_t currentBytes = stats[flowId].rxBytes;
        std::ofstream thr(dir + "/throughput_" + std::to_string(flowId) + ".dat", 
                         std::ios::out | std::ios::app);

        if (curTime.GetSeconds() > throughputTrackers[senderId].prevTime.GetSeconds())
        {
            double throughputMbps = 8 * (currentBytes - throughputTrackers[senderId].prevBytes) /
                                   (1000000.0 * (curTime.GetSeconds() - 
                                    throughputTrackers[senderId].prevTime.GetSeconds()));
            thr << curTime.GetSeconds() << " " << throughputMbps << std::endl;
        }

        throughputTrackers[senderId].prevBytes = currentBytes;
        throughputTrackers[senderId].prevTime = curTime;
        thr.close();
    }
    
    Simulator::Schedule(Seconds(0.2), &TraceThroughputPerSender, monitor, senderId);
}
// Check the queue size on bottleneck link
void
CheckQueueSize(Ptr<QueueDisc> qd)
{
    uint32_t qsize = qd->GetCurrentSize().GetValue();
    Simulator::Schedule(Seconds(0.2), &CheckQueueSize, qd);
    std::ofstream q(dir + "/queueSize.dat", std::ios::out | std::ios::app);
    q << Simulator::Now().GetSeconds() << " " << qsize << std::endl;
    q.close();
}

// Trace congestion window
static void
CwndTracer(Ptr<OutputStreamWrapper> stream, uint32_t oldval, uint32_t newval)
{
    *stream->GetStream() << Simulator::Now().GetSeconds() << " " << newval / 1448.0 << std::endl;
}

void
TraceCwnd(Ptr<Node> node, uint32_t socketId, uint32_t senderId)
{
    AsciiTraceHelper ascii;
    uint32_t nodeId = node->GetId();
    Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream(dir + "/cwnd_" + std::to_string(senderId + 1) + ".dat");
    Config::ConnectWithoutContext("/NodeList/" + std::to_string(nodeId) +
                                      "/$ns3::TcpL4Protocol/SocketList/" +
                                      std::to_string(socketId) + "/CongestionWindow",
                                  MakeBoundCallback(&CwndTracer, stream));
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

    std::string tcpTypeId = "TcpCubic";
    std::string queueDisc = "FifoQueueDisc";
    uint32_t delAckCount = 2;
    bool bql = true;
    bool enablePcap = true;
    Time stopTime = Seconds(100);
    
    // Dumbbell topology parameters
    uint32_t nLeaf = 1;  // Number of sender/receiver leaf pairs
    std::string bottleneckBw = "10Mbps";
    std::string bottleneckDelay = "10ms";
    std::string edgeBw = "1000Mbps";
    std::string edgeDelay = "5ms";

    CommandLine cmd(__FILE__);
    cmd.AddValue("tcpTypeId", "Transport protocol to use (e.g., TcpNewReno, TcpBbr, TcpCubic, etc.)", tcpTypeId);
    cmd.AddValue("delAckCount", "Delayed ACK count", delAckCount);
    cmd.AddValue("enablePcap", "Enable/Disable pcap file generation", enablePcap);
    cmd.AddValue("stopTime",
                 "Stop time for applications / simulation time will be stopTime + 1",
                 stopTime);
    cmd.AddValue("nLeaf", "Number of sender/receiver leaf pairs", nLeaf);
    cmd.AddValue("bottleneckBw", "Bottleneck link bandwidth", bottleneckBw);
    cmd.AddValue("bottleneckDelay", "Bottleneck link one-way delay", bottleneckDelay);
    cmd.AddValue("edgeBw", "Edge link bandwidth", edgeBw);
    cmd.AddValue("edgeDelay", "Edge link one-way delay", edgeDelay);
    cmd.Parse(argc, argv);

    queueDisc = std::string("ns3::") + queueDisc;

    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::" + tcpTypeId));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(4194304));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(6291456));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(delAckCount));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));
    Config::SetDefault("ns3::DropTailQueue<Packet>::MaxSize", QueueSizeValue(QueueSize("1p")));
    Config::SetDefault(queueDisc + "::MaxSize", QueueSizeValue(QueueSize("150p")));

    // Configure queue discipline globally BEFORE creating topology
    // This way dumbbell devices will use our configured queue disc from the start
    TrafficControlHelper tch;
    tch.SetRootQueueDisc(queueDisc);
    if (bql)
    {
        tch.SetQueueLimits("ns3::DynamicQueueLimits", "HoldTime", StringValue("1000ms"));
    }

    // Create the dumbbell topology using PointToPointDumbbellHelper
    PointToPointHelper bottleneckLink;
    bottleneckLink.SetDeviceAttribute("DataRate", StringValue(bottleneckBw));
    bottleneckLink.SetChannelAttribute("Delay", StringValue(bottleneckDelay));

    PointToPointHelper edgeLink;
    edgeLink.SetDeviceAttribute("DataRate", StringValue(edgeBw));
    edgeLink.SetChannelAttribute("Delay", StringValue(edgeDelay));

    // Initialize throughput trackers for each sender
    throughputTrackers.resize(nLeaf);

    // Create dumbbell topology with nLeaf senders and nLeaf receivers
    PointToPointDumbbellHelper dumbbell(nLeaf, edgeLink, nLeaf, edgeLink, bottleneckLink);

    // Install Internet Stack on all nodes
    InternetStackHelper internet;
    dumbbell.InstallStack(internet);

    // Assign IP addresses
    dumbbell.AssignIpv4Addresses(Ipv4AddressHelper("10.1.1.0", "255.255.255.0"),    // left leaves
                                  Ipv4AddressHelper("10.2.1.0", "255.255.255.0"),    // right leaves
                                  Ipv4AddressHelper("10.10.1.0", "255.255.255.0")); // routers

    // Populate routing tables
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Select sender side port
    uint16_t port = 50001;

    // Install applications on all sender/receiver pairs
    ApplicationContainer sourceApps;
    ApplicationContainer sinkApps;

    for (uint32_t i = 0; i < nLeaf; ++i)
    {
        // Get receiver IP address
        Ipv4Address receiverIp = dumbbell.GetRightIpv4Address(i);

        // Install BulkSendHelper (sender side application)
        BulkSendHelper source("ns3::TcpSocketFactory", 
                             InetSocketAddress(receiverIp, port + i));
        source.SetAttribute("MaxBytes", UintegerValue(0));
        ApplicationContainer tempSource = source.Install(dumbbell.GetLeft(i));
        tempSource.Start(Seconds(0.1));
        tempSource.Stop(stopTime);
        sourceApps.Add(tempSource);

        // Schedule trace connection for each sender
        Ptr<Node> senderNode = dumbbell.GetLeft(i);
        Simulator::Schedule(Seconds(0.1) + MilliSeconds(1), &TraceCwnd, senderNode, 0, i);

        // Install PacketSinkHelper (receiver side application)
        PacketSinkHelper sink("ns3::TcpSocketFactory", 
                             InetSocketAddress(Ipv4Address::GetAny(), port + i));
        ApplicationContainer tempSink = sink.Install(dumbbell.GetRight(i));
        tempSink.Start(Seconds(0.0));
        tempSink.Stop(stopTime);
        sinkApps.Add(tempSink);
    }

    // Create a new directory to store the output of the program
    dir = tcpTypeId + "-results/" + currentTime + "/";
    std::string dirToSave = "mkdir -p " + dir;
    if (system(dirToSave.c_str()) == -1)
    {
        exit(1);
    }

    // Trace the queue occupancy on the bottleneck link
    // The bottleneck device is at index nLeaf on the left router (device connecting to right router)
    Ptr<NetDevice> bottleneckDevice = dumbbell.GetLeft()->GetDevice(0);
    
    // Get the queue disc that was installed by TrafficControlHelper during dumbbell creation
    Ptr<QueueDisc> qd = bottleneckDevice->GetNode()->GetObject<TrafficControlLayer>()
                            ->GetRootQueueDiscOnDevice(bottleneckDevice);
    
    if (qd)
    {
        Simulator::ScheduleNow(&CheckQueueSize, qd);
    }

    // Generate PCAP traces if it is enabled
    if (enablePcap)
    {
        if (system((dirToSave + "/pcap/").c_str()) == -1)
        {
            exit(1);
        }
        bottleneckLink.EnablePcapAll(dir + "/pcap/bottleneck", true);
    }

    // Check for dropped packets using Flow Monitor
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    
    // Schedule throughput tracing for each sender
    for (uint32_t i = 0; i < nLeaf; ++i)
    {
        Simulator::Schedule(Seconds(0.2), &TraceThroughputPerSender, monitor, i);
    }


    Simulator::Stop(stopTime + TimeStep(1));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
