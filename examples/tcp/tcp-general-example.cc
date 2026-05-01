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
 *
 * Modified by: Santhosh Balaji G <santhoshbalajig.231cs224@nitk.edu.in>
 *              Sai Nishnath Rao  <tugenasainishnathrao.231cs260@nitk.edu.in>
 *              Dattatreya M      <manepallidattatretyalaxminarasimha.231cs231@nitk.edu.in>
 *              Aravind G         <gurugubelliaravind.231cs124@nitk.edu.in>
 *              karthikeya S V    <svkarthikeya.231cs150@nitk.edu.in>
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
// - tcpTypeId: TCP variant to use (default: TcpCubic)
// - nLeaf: Number of sender/receiver leaf pairs (default: 1)
// - bottleneckBw: Bottleneck link bandwidth (default: 10Mbps)
// - bottleneckDelay: Bottleneck link delay (default: 10ms)
// - edgeBw: Edge link bandwidth (default: 1000Mbps)
// - edgeDelay: Edge link delay (default: 5ms)
// - delAckCount: Delayed ACK count (default: 2)
// - QueueDisc: Queue discipline to use on bottleneck link (default: FifoQueueDisc)
// - queueSize: Queue size for the bottleneck link (default: 100 packets)
//
// This program runs by default for 100 seconds and creates a new directory
// called 'TcpCubic-results' in the ns-3 root directory. The program creates one
// sub-directory called 'pcap' in 'TcpCubic-results' directory (if pcap generation
// is enabled) and multiple .dat files.
//
// Output files:
// (1) 'pcap' sub-directory contains PCAP files from all links (if enabled)
// (2) cwnd_X.dat files contain congestion window trace for each sender node
// (3) throughput_X.dat files contain throughput trace for each sender node
// (4) queueSize.dat file contains queue length trace from the bottleneck link
//
// Supported TCP variants: TcpNewReno, TcpReno, TcpTahoe, TcpBbr, TcpBic, and others.

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
   FlowId flowId = 0;          
   Ipv4Address senderIp;       
};

std::vector<ThroughputTracker> throughputTrackers;


static void
BuildFlowIdMap(Ptr<FlowMonitor> monitor,
                   Ptr<Ipv4FlowClassifier> classifier)
{
   monitor->CheckForLostPackets();
   for (auto& flow : monitor->GetFlowStats())
   {
       Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);
       for (uint32_t i = 0; i < throughputTrackers.size(); ++i)
       {
           if (t.sourceAddress == throughputTrackers[i].senderIp)
           {
               throughputTrackers[i].flowId = flow.first;
               break;
           }
       }
   }
}


static void
TraceThroughputPerSender(Ptr<FlowMonitor> monitor, uint32_t senderId)
{
   auto stats = monitor->GetFlowStats();
   FlowId fid = throughputTrackers[senderId].flowId;
   uint64_t currentBytes = stats[fid].rxBytes;
    Time curTime = Now();
    std::ofstream thr(dir + "/throughput" + std::to_string(senderId + 1) + ".dat", std::ios::out | std::ios::app);
    thr << curTime << " "
        << 8 * (currentBytes - throughputTrackers[senderId].prevBytes) /
               (1000 * 1000 * (curTime.GetSeconds() - throughputTrackers[senderId].prevTime.GetSeconds()))
        << std::endl;
    throughputTrackers[senderId].prevTime = curTime;
    throughputTrackers[senderId].prevBytes = currentBytes;
    thr.close();
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
    Config::SetDefault(queueDisc + "::MaxSize", QueueSizeValue(QueueSize("100p")));


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
        throughputTrackers[i].senderIp = dumbbell.GetLeft(i)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
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

    TrafficControlHelper tch;
    tch.SetRootQueueDisc(queueDisc,"MaxSize", QueueSizeValue(QueueSize("100p")));
        if (bql)
    {
        tch.SetQueueLimits("ns3::DynamicQueueLimits", "HoldTime", StringValue("1000ms"));
    }

    dumbbell.InstallBottleneckQueueDisc(tch);

    Ptr<QueueDisc> qd = dumbbell.GetBottleneckQueueDisc();
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


    // Get classifier
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());


    // Build flow ID map for throughput tracking
    Simulator::Schedule(Seconds(0.1) + MilliSeconds(500), &BuildFlowIdMap, monitor, classifier);


    for (uint32_t i = 0; i < nLeaf; ++i)
    {
        Simulator::Schedule(Seconds(0.2), &TraceThroughputPerSender, monitor, i);
    }

    Simulator::Stop(stopTime + TimeStep(1));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
