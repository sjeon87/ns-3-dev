/*
 * Copyright (c) 2026 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Ayush Nigam <ash12521198@gmail.com>
 *          S B L Prateek <sblprateek@gmail.com>
 *          A R Sharan Kumar <arsharankumar99@gmail.com>
 *          Yashwanth R <ryashwanth990@gmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */

// This program simulates the following topology:
//
//              1000 Mbps                                      1000 Mbps
//  Sender_0 --------------+                            +-------------- Receiver_0
//     ...         2ms     |        20 Mbps, 6ms        |      2ms          ...
//  Sender_N --------------+ R1 ----------------------- R2 +-------------- Receiver_N
//
// The R1-R2 link is the bottleneck, giving a base round trip time of 20 ms. Flow i runs from
// Sender_i to Receiver_i on port 50001 + i.
//
// The default is inter-LEDBAT++ fairness: four LEDBAT++ flows over 100 seconds, joining ten
// seconds apart. They should end up sharing the bottleneck closely, though not exactly: each flow
// measures its base delay while the flows already present are queuing, so the earliest flow keeps
// the smallest share. What must not happen is a latecomer starving the flows that arrived first.
//
// To see the scavenging behaviour instead, run one LEDBAT++ flow for 100 seconds and introduce a
// CUBIC flow between 40 and 80 seconds. LEDBAT++ should yield the bottleneck to CUBIC, and take it
// back once CUBIC leaves:
//
//   --ccaNames=TcpLedbatPp,TcpCubic --startTimes=0.1,40 --stopTimes=100,80
//
// Nothing in this program is specific to LEDBAT++. 'ccaNames' lists the congestion control of each
// flow and creates one flow per entry, so any mixture can be placed on the bottleneck, while
// 'startTimes' and 'stopTimes' give each flow its own lifetime. The link rates, the two delays and
// the queue disc size are options as well.
//
// This program creates a new directory called 'ledbatpp-results' in the ns-3 root directory, and
// a timestamped sub-directory inside it for every run. That sub-directory holds a 'Traces'
// sub-directory, and a 'Pcap' sub-directory if pcap generation is enabled.
//
// (1) The 'Traces' sub-directory contains, for every flow i:
//     * Node_i/<CongestionControl>/cwnd.dat     congestion window, in segments
//     * Node_i/<CongestionControl>/inflight.dat bytes in flight, in kilobytes
//     * Node_i/throughput.dat                   sender side throughput, in Mbit/s
//     and, for the bottleneck link:
//     * queueSize.dat                           queue length, in packets
//     * sojournTime.dat                         queue sojourn time, in milliseconds
//
// (2) The 'Pcap' sub-directory contains one PCAP file per point-to-point interface, named
//     tcp-ledbat-pp-<node>-<interface>.pcap. With N flows the nodes are numbered Sender_0 to
//     Sender_N-1, then Receiver_0 to Receiver_N-1, then R1 and R2, and R1 and R2 each carry one
//     interface per flow plus the bottleneck one. A single flow therefore produces six files:
//     * tcp-ledbat-pp-0-0.pcap for the interface on Sender_0
//     * tcp-ledbat-pp-1-0.pcap for the interface on Receiver_0
//     * tcp-ledbat-pp-2-0.pcap for the first interface on R1
//     * tcp-ledbat-pp-2-1.pcap for the second interface on R1
//     * tcp-ledbat-pp-3-0.pcap for the first interface on R2
//     * tcp-ledbat-pp-3-1.pcap for the second interface on R2
//     and N flows produce 4 * N + 2 files in total.
//
// This program does not draw the traces. A plotting script is provided in the following
// repository, if needed:
// https://github.com/sblprateek/ledbatpp-plot-script
//
// LEDBAT++ periodically enters a slowdown, freezing the congestion window at two segments for two
// round trip times so that the base delay can be re-measured. The congestion window traces output
// by this program therefore show periodic drops, and the sojourn time trace settles near the 60 ms
// target delay between them. A single flow empties the bottleneck queue during its slowdown; when
// several flows share the bottleneck their slowdowns do not coincide, so the queue stays occupied.

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"

#include <algorithm>

using namespace ns3;
using namespace ns3::SystemPath;

NS_LOG_COMPONENT_DEFINE("TcpLedbatppExample");

std::ofstream queueSize;
std::ofstream sojournTime;

// Segment size, and the unit in which the congestion window is reported
constexpr uint32_t SEGMENT_SIZE = 1448;

// Flow i listens on BASE_PORT + i, which is how a FlowMonitor record is matched back to its flow
constexpr uint16_t BASE_PORT = 50001;

/**
 * Per flow throughput accounting, with one entry per flow.
 */
struct Throughput
{
    std::vector<std::ofstream> files; //!< Where each flow's throughput is written
    std::vector<uint32_t> prevBytes;  //!< Bytes sent as of the previous sample
    std::vector<Time> prevTime;       //!< When the previous sample was taken
};

static void
TraceThroughput(Ptr<FlowMonitor> monitor, Ptr<Ipv4FlowClassifier> classifier, Throughput* tput)
{
    Time curTime = Now();

    for (auto& kv : monitor->GetFlowStats())
    {
        // FlowMonitor also records the reverse (acknowledgement) flows, and it only records a
        // flow once it has sent its first packet. Match on the destination port rather than on
        // the flow identifier, which shifts as the staggered flows come up.
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(kv.first);
        if (t.destinationPort < BASE_PORT || t.destinationPort >= BASE_PORT + tput->files.size())
        {
            continue;
        }
        uint32_t i = t.destinationPort - BASE_PORT;

        // Convert (curTime - prevTime) to microseconds so that throughput is in bits per
        // microsecond (which is equivalent to Mbps)
        tput->files[i] << curTime.GetSeconds() << "s "
                       << 8 * (kv.second.txBytes - tput->prevBytes[i]) /
                              ((curTime - tput->prevTime[i]).ToDouble(Time::US))
                       << " Mbps" << std::endl;
        tput->prevTime[i] = curTime;
        tput->prevBytes[i] = kv.second.txBytes;
    }
    Simulator::Schedule(Seconds(0.2), &TraceThroughput, monitor, classifier, tput);
}

void
CheckQueueSize(Ptr<QueueDisc> qd)
{
    uint32_t qsize = qd->GetCurrentSize().GetValue();
    Simulator::Schedule(Seconds(0.2), &CheckQueueSize, qd);
    queueSize << Simulator::Now().GetSeconds() << " " << qsize << std::endl;
}

// The congestion window is recorded in segments rather than bytes
static void
SegmentsTracer(Ptr<OutputStreamWrapper> stream, uint32_t oldval, uint32_t newval)
{
    *stream->GetStream() << Simulator::Now().GetSeconds() << " " << newval / SEGMENT_SIZE
                         << std::endl;
}

// Bytes in flight are recorded in kilobytes, which does not depend on the segment size
static void
KilobytesTracer(Ptr<OutputStreamWrapper> stream, uint32_t oldval, uint32_t newval)
{
    *stream->GetStream() << Simulator::Now().GetSeconds() << " " << newval / 1024.0 << std::endl;
}

void
TraceSegments(uint32_t nodeId, uint32_t socketId, std::string traceSource, std::string filename)
{
    AsciiTraceHelper ascii;
    Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream(filename);
    Config::ConnectWithoutContext("/NodeList/" + std::to_string(nodeId) +
                                      "/$ns3::TcpL4Protocol/SocketList/" +
                                      std::to_string(socketId) + "/" + traceSource,
                                  MakeBoundCallback(&SegmentsTracer, stream));
}

void
TraceKilobytes(uint32_t nodeId, uint32_t socketId, std::string traceSource, std::string filename)
{
    AsciiTraceHelper ascii;
    Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream(filename);
    Config::ConnectWithoutContext("/NodeList/" + std::to_string(nodeId) +
                                      "/$ns3::TcpL4Protocol/SocketList/" +
                                      std::to_string(socketId) + "/" + traceSource,
                                  MakeBoundCallback(&KilobytesTracer, stream));
}

static void
TraceSojourn(Time newValue)
{
    sojournTime << Simulator::Now().GetSeconds() << " " << newValue.GetMilliSeconds() << std::endl;
}

// Parse a comma separated list of seconds, such as "0.1,10.1". An empty string yields an empty
// vector, and the caller falls back to its own default.
static std::vector<Time>
ParseTimes(const std::string& csv)
{
    std::vector<Time> times;
    if (csv.empty())
    {
        return times;
    }
    for (const auto& field : SplitString(csv, ","))
    {
        double seconds;
        std::istringstream iss(field);
        iss >> seconds;
        NS_ABORT_MSG_IF(iss.fail() || !iss.eof(), "'" << field << "' is not a number of seconds");
        times.push_back(Seconds(seconds));
    }
    return times;
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

    // Edit these to change the queue discipline or the byte queue limits
    std::string queueDisc = "FifoQueueDisc";
    bool bql = true;

    uint32_t delAckCount = 2;
    bool enablePcap = false;
    Time stopTime = Seconds(100);
    std::string bottleneckBandwidth = "20Mbps";
    std::string edgeBandwidth = "1000Mbps";
    Time bottleneckDelay = MilliSeconds(6);
    Time edgeDelay = MilliSeconds(2);
    std::string queueDiscSize = "1000p";
    std::string startTimesCsv;
    std::string stopTimesCsv;
    std::string ccaNamesCsv = "TcpLedbatPp,TcpLedbatPp,TcpLedbatPp,TcpLedbatPp";

    CommandLine cmd(__FILE__);
    cmd.AddValue("ccaNames",
                 "Congestion control of each flow, e.g. TcpLedbatPp,TcpCubic,TcpBbr. One flow is "
                 "created per entry",
                 ccaNamesCsv);
    cmd.AddValue("delAckCount", "Delayed ACK count", delAckCount);
    cmd.AddValue("enablePcap", "Enable/Disable pcap file generation", enablePcap);
    cmd.AddValue("stopTime", "Stop time for the applications", stopTime);
    cmd.AddValue("bottleneckBandwidth", "Data rate of the R1-R2 link", bottleneckBandwidth);
    cmd.AddValue("bottleneckDelay", "One way delay of the R1-R2 link", bottleneckDelay);
    cmd.AddValue("edgeBandwidth", "Data rate of each edge link", edgeBandwidth);
    cmd.AddValue("edgeDelay", "One way delay of each edge link", edgeDelay);
    cmd.AddValue("queueDiscSize", "Size of the bottleneck queue disc, e.g. 900p", queueDiscSize);
    cmd.AddValue("startTimes",
                 "Per flow start times in seconds, e.g. 0.1,10.1,20.1. Defaults to ten seconds "
                 "apart",
                 startTimesCsv);
    cmd.AddValue("stopTimes",
                 "Per flow stop times in seconds, e.g. 100,100,50. Defaults to stopTime for every "
                 "flow",
                 stopTimesCsv);
    cmd.Parse(argc, argv);

    NS_ABORT_MSG_IF(ccaNamesCsv.find_first_not_of(" \t") == std::string::npos,
                    "At least one flow must be configured");

    std::vector<std::string> flowCca = SplitString(ccaNamesCsv, ",");
    TypeId ccaTid;
    for (auto& name : flowCca)
    {
        // SplitString keeps any blanks around an entry, which the TypeId lookup would reject
        size_t first = name.find_first_not_of(" \t");
        size_t last = name.find_last_not_of(" \t");
        name = (first == std::string::npos) ? "" : name.substr(first, last - first + 1);
        NS_ABORT_MSG_IF(name.empty(), "ccaNames has an empty entry: '" << ccaNamesCsv << "'");
        NS_ABORT_MSG_IF(!TypeId::LookupByNameFailSafe("ns3::" + name, &ccaTid),
                        "Unknown congestion control '"
                            << name
                            << "'. Valid examples: TcpLedbatPp, TcpLinuxReno, TcpCubic, TcpBbr.");
    }

    uint32_t flows = flowCca.size();
    std::vector<Time> startTimes = ParseTimes(startTimesCsv);
    std::vector<Time> stopTimes = ParseTimes(stopTimesCsv);
    auto checkCount = [flows](const std::vector<Time>& times, const std::string& what) {
        NS_ABORT_MSG_IF(!times.empty() && times.size() != flows,
                        what << " lists " << times.size() << " entries but there are " << flows
                             << " flows");
    };
    checkCount(startTimes, "startTimes");
    checkCount(stopTimes, "stopTimes");

    // The maximum send buffer size is set to 4194304 bytes (4MB) and the
    // maximum receive buffer size is set to 6291456 bytes (6MB) in the Linux
    // kernel. The same buffer sizes are used as default in this example.
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(4194304));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(6291456));
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(10));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(delAckCount));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(SEGMENT_SIZE));
    Config::SetDefault("ns3::DropTailQueue<Packet>::MaxSize", QueueSizeValue(QueueSize("1p")));
    queueDisc = "ns3::" + queueDisc;
    Config::SetDefault(queueDisc + "::MaxSize", QueueSizeValue(QueueSize(queueDiscSize)));

    NodeContainer sender;
    NodeContainer receiver;
    NodeContainer routers;

    sender.Create(flows);
    receiver.Create(flows);
    routers.Create(2);

    PointToPointHelper bottleneckLink;
    bottleneckLink.SetDeviceAttribute("DataRate", StringValue(bottleneckBandwidth));
    bottleneckLink.SetChannelAttribute("Delay", TimeValue(bottleneckDelay));

    PointToPointHelper edgeLink;
    edgeLink.SetDeviceAttribute("DataRate", StringValue(edgeBandwidth));
    edgeLink.SetChannelAttribute("Delay", TimeValue(edgeDelay));

    std::vector<NetDeviceContainer> senderEdges(flows);
    std::vector<NetDeviceContainer> receiverEdges(flows);

    for (uint32_t i = 0; i < flows; i++)
    {
        senderEdges[i] = edgeLink.Install(sender.Get(i), routers.Get(0));
        receiverEdges[i] = edgeLink.Install(routers.Get(1), receiver.Get(i));
    }
    NetDeviceContainer r1r2 = bottleneckLink.Install(routers.Get(0), routers.Get(1));

    InternetStackHelper internet;
    internet.Install(sender);
    internet.Install(receiver);
    internet.Install(routers);

    TrafficControlHelper tch;
    tch.SetRootQueueDisc(queueDisc);

    if (bql)
    {
        tch.SetQueueLimits("ns3::DynamicQueueLimits", "HoldTime", StringValue("1000ms"));
    }

    for (uint32_t i = 0; i < flows; i++)
    {
        tch.Install(senderEdges[i]);
        tch.Install(receiverEdges[i]);
    }

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.0.0.0", "255.255.255.0");
    ipv4.Assign(r1r2);
    ipv4.NewNetwork();

    // Only the receiver addresses are needed later, to point each sender at its own receiver
    std::vector<Ipv4InterfaceContainer> receiverInterface(flows);
    for (uint32_t i = 0; i < flows; i++)
    {
        ipv4.NewNetwork();
        ipv4.Assign(senderEdges[i]);
        ipv4.NewNetwork();
        receiverInterface[i] = ipv4.Assign(receiverEdges[i]);
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // MakeDirectories creates the parents, so the per-flow directories below build the whole tree
    std::string results = "ledbatpp-results/" + currentTime + "/";
    std::string dir = results + "Traces/";

    // The simulation runs until the last flow has stopped, which is stopTime unless stopTimes
    // pushes a flow out further
    Time lastStop =
        stopTimes.empty() ? stopTime : *std::max_element(stopTimes.begin(), stopTimes.end());

    for (uint32_t i = 0; i < flows; i++)
    {
        const std::string& name = flowCca[i];
        std::string flowDir = dir + "Node_" + std::to_string(i) + "/" + name + "/";
        MakeDirectories(flowDir);

        Config::Set("/NodeList/" + std::to_string(sender.Get(i)->GetId()) +
                        "/$ns3::TcpL4Protocol/SocketType",
                    TypeIdValue(TypeId::LookupByName("ns3::" + name)));

        uint16_t port = BASE_PORT + i;
        // Unless startTimes says otherwise, flows start ten seconds apart, so that a flow joining
        // a bottleneck that is already busy can be observed
        Time start = startTimes.empty() ? Seconds(0.1 + 10 * i) : startTimes[i];
        Time stop = stopTimes.empty() ? stopTime : stopTimes[i];
        NS_ABORT_MSG_IF(start >= stop,
                        "Flow " << i << " starts at " << start.As(Time::S) << " but stops at "
                                << stop.As(Time::S));

        BulkSendHelper source("ns3::TcpSocketFactory",
                              InetSocketAddress(receiverInterface[i].GetAddress(1), port));
        source.SetAttribute("MaxBytes", UintegerValue(0));
        ApplicationContainer sourceApps = source.Install(sender.Get(i));
        sourceApps.Start(start);
        sourceApps.Stop(stop);

        // The receiver listens from the outset and outlives its sender, so that a flow which
        // stops early can still close cleanly
        PacketSinkHelper sink("ns3::TcpSocketFactory",
                              InetSocketAddress(Ipv4Address::GetAny(), port));
        ApplicationContainer sinkApps = sink.Install(receiver.Get(i));
        sinkApps.Start(Seconds(0));
        sinkApps.Stop(lastStop);

        // The socket does not exist until the application starts, so hook the traces just after
        Time hook = start + MilliSeconds(1);
        Simulator::Schedule(hook, &TraceSegments, i, 0, "CongestionWindow", flowDir + "cwnd.dat");
        Simulator::Schedule(hook, &TraceKilobytes, i, 0, "BytesInFlight", flowDir + "inflight.dat");
    }

    // Reinstall the queue disc on R1's bottleneck device so that its queue can be traced
    tch.Uninstall(r1r2.Get(0));
    QueueDiscContainer qd = tch.Install(r1r2.Get(0));
    Simulator::ScheduleNow(&CheckQueueSize, qd.Get(0));
    qd.Get(0)->TraceConnectWithoutContext("SojournTime", MakeCallback(&TraceSojourn));

    // EnablePcapAll already covers every point-to-point device, so it is called once
    if (enablePcap)
    {
        std::string pcapDir = results + "Pcap/";
        MakeDirectories(pcapDir);
        bottleneckLink.EnablePcapAll(pcapDir + "tcp-ledbat-pp", true);
    }

    Throughput tput{std::vector<std::ofstream>(flows),
                    std::vector<uint32_t>(flows, 0),
                    std::vector<Time>(flows)};
    for (uint32_t i = 0; i < flows; i++)
    {
        tput.files[i].open(dir + "Node_" + std::to_string(i) + "/throughput.dat", std::ios::out);
        NS_ASSERT_MSG(tput.files[i].is_open(), "Throughput file was not opened correctly");
    }
    queueSize.open(dir + "queueSize.dat", std::ios::out);
    NS_ASSERT_MSG(queueSize.is_open(), "Queue size file was not opened correctly");
    sojournTime.open(dir + "sojournTime.dat", std::ios::out);
    NS_ASSERT_MSG(sojournTime.is_open(), "Sojourn time file was not opened correctly");

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    Simulator::Schedule(Seconds(0.000001), &TraceThroughput, monitor, classifier, &tput);

    Simulator::Stop(lastStop + TimeStep(1));
    Simulator::Run();
    Simulator::Destroy();

    queueSize.close();
    sojournTime.close();

    return 0;
}
