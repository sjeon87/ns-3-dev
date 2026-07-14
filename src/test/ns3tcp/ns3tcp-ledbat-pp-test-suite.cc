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

// Behavioural regression tests for LEDBAT++, on the topology of tcp-ledbat-pp-example.cc:
//
//           1 Gbps            20 Mbps           1 Gbps
//  Sender_i ---------- R1 ---------------- R2 ---------- Receiver_i
//             2ms               6ms                2ms
//
// The base round trip time is 20 ms. The tcp-ledbat-pp-test unit tests check the per-ACK
// arithmetic and the phase transitions; these tests check the properties that arithmetic is
// supposed to deliver, over real sockets sharing a real bottleneck:
//
// 1. Yields to standard TCP, and recovers. A LEDBAT++ flow gives way to a CUBIC flow that joins
//    the bottleneck, and takes the capacity back once CUBIC leaves. This is the low priority
//    contract of the draft, and the single most important property to hold.
// 2. Inter-LEDBAT++ fairness. Four LEDBAT++ flows joining at different times share the bottleneck,
//    rather than the latecomers starving the flows that arrived first.
// 3. Bounded delay when alone. A single LEDBAT++ flow holds the queuing delay near the 60 ms
//    target delay while keeping the bottleneck busy.
// 4. Small buffer fallback. When the buffer cannot hold the target delay, the queuing delay is
//    bounded by the buffer rather than by the target, and the flow keeps the link busy.
// 5. The slowdown drains the queue. Section 4.4 reduces the congestion window to two packets so
//    that the bottleneck empties and the base delay can be re-measured. Between slowdowns the
//    queue stays backlogged; during one it empties.
//
// Throughput is measured from the bytes each PacketSink receives over a window, once the flows
// have settled, so that the slow start transient does not bias the result.

#include "ns3/bulk-send-helper.h"
#include "ns3/config.h"
#include "ns3/inet-socket-address.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/log.h"
#include "ns3/node-container.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/queue-disc.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/test.h"
#include "ns3/traffic-control-helper.h"
#include "ns3/uinteger.h"

#include <algorithm>
#include <limits>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Ns3TcpLedbatPpTest");

namespace
{

/// Segment size used by every flow, in bytes.
constexpr uint32_t SEGMENT_SIZE = 1448;
/// Capacity of the bottleneck link.
constexpr auto BOTTLENECK_RATE = "20Mbps";
/// How often the bottleneck queue occupancy is sampled.
const Time QUEUE_SAMPLE_INTERVAL = MilliSeconds(10);

} // namespace

/**
 * @ingroup system-tests-tcp
 *
 * @brief Shared bottleneck topology, and the throughput, sojourn time and queue occupancy
 *        accounting used by the LEDBAT++ behaviour tests.
 */
class Ns3TcpLedbatPpBaseTestCase : public TestCase
{
  public:
    /**
     * Constructor.
     *
     * @param name Name of the test case.
     */
    Ns3TcpLedbatPpBaseTestCase(std::string name);

  protected:
    /**
     * Build the dumbbell and install one bulk send flow per entry of cca.
     *
     * @param cca Congestion control of each flow, without the "ns3::" prefix.
     * @param start When each flow starts sending.
     * @param stop When each flow stops sending.
     * @param queueDiscSize Size of the bottleneck queue disc, e.g. "1000p".
     */
    void SetupFlows(const std::vector<std::string>& cca,
                    const std::vector<Time>& start,
                    const std::vector<Time>& stop,
                    const std::string& queueDiscSize);

    /**
     * Declare a window over which throughput, sojourn time and queue occupancy are measured.
     * Must be called before Simulator::Run().
     *
     * @param start Beginning of the window.
     * @param end End of the window.
     * @return The index of the window.
     */
    uint32_t AddWindow(Time start, Time end);

    /**
     * Throughput of one flow over a window.
     *
     * @param window Index returned by AddWindow.
     * @param flow Index of the flow.
     * @return The throughput, in Mbit/s.
     */
    double GetThroughput(uint32_t window, uint32_t flow) const;

    /**
     * Throughput of every flow over a window, summed.
     *
     * @param window Index returned by AddWindow.
     * @return The throughput, in Mbit/s.
     */
    double GetTotalThroughput(uint32_t window) const;

    /**
     * Share of the throughput over a window taken by one flow.
     *
     * @param window Index returned by AddWindow.
     * @param flow Index of the flow.
     * @return The share, between 0 and 1.
     */
    double GetShare(uint32_t window, uint32_t flow) const;

    /**
     * Jain's fairness index over a window, which is 1 when every flow receives an equal share.
     *
     * @param window Index returned by AddWindow.
     * @return The index, between 1/n and 1.
     */
    double GetJainIndex(uint32_t window) const;

    /**
     * Mean sojourn time of the bottleneck queue over a window.
     *
     * @param window Index returned by AddWindow.
     * @return The mean sojourn time.
     */
    Time GetMeanSojourn(uint32_t window) const;

    /**
     * Smallest bottleneck queue occupancy observed over a window.
     *
     * @param window Index returned by AddWindow.
     * @return The occupancy, in packets.
     */
    uint32_t GetMinQueue(uint32_t window) const;

    /**
     * Mean bottleneck queue occupancy over a window.
     *
     * @param window Index returned by AddWindow.
     * @return The occupancy, in packets.
     */
    double GetMeanQueue(uint32_t window) const;

  private:
    /// What is recorded over one measurement window.
    struct Window
    {
        Time start;                    //!< Beginning of the window
        Time end;                      //!< End of the window
        bool open{false};              //!< Whether samples are being accumulated
        std::vector<uint64_t> rxOpen;  //!< Bytes received by each sink when the window opened
        std::vector<uint64_t> rxClose; //!< Bytes received by each sink when the window closed
        Time sojournSum;               //!< Sum of the sojourn samples taken in the window
        uint64_t sojournCount{0};      //!< Number of sojourn samples taken in the window
        uint32_t minQueue{std::numeric_limits<uint32_t>::max()}; //!< Smallest occupancy seen
        uint64_t queueSum{0};                                    //!< Sum of the occupancy samples
        uint64_t queueCount{0};                                  //!< Number of occupancy samples
    };

    /**
     * Record the bytes received so far by every sink, at the start of a window.
     *
     * @param window Index of the window.
     */
    void OpenWindow(uint32_t window);

    /**
     * Record the bytes received so far by every sink, at the end of a window.
     *
     * @param window Index of the window.
     */
    void CloseWindow(uint32_t window);

    /**
     * Accumulate one sojourn time sample into every open window.
     *
     * @param sojourn The sojourn time of a packet leaving the bottleneck queue.
     */
    void SojournCb(Time sojourn);

    /// Accumulate one queue occupancy sample into every open window, and reschedule.
    void SampleQueue();

    std::vector<Ptr<PacketSink>> m_sinks; //!< One sink per flow
    std::vector<Window> m_windows;        //!< Measurement windows, in the order declared
    Ptr<QueueDisc> m_bottleneckQd;        //!< Queue disc on R1's bottleneck device
};

Ns3TcpLedbatPpBaseTestCase::Ns3TcpLedbatPpBaseTestCase(std::string name)
    : TestCase(name)
{
}

void
Ns3TcpLedbatPpBaseTestCase::SetupFlows(const std::vector<std::string>& cca,
                                       const std::vector<Time>& start,
                                       const std::vector<Time>& stop,
                                       const std::string& queueDiscSize)
{
    NS_ABORT_MSG_IF(cca.size() != start.size() || cca.size() != stop.size(),
                    "One start time and one stop time is needed per flow");
    uint32_t flows = cca.size();
    Time lastStop = *std::max_element(stop.begin(), stop.end());

    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(4194304));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(6291456));
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(10));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(2));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(SEGMENT_SIZE));

    // Keep the buffering in the queue disc rather than in the device, so that the queue disc size
    // alone decides how much queuing delay the bottleneck can hold
    Config::SetDefault("ns3::DropTailQueue<Packet>::MaxSize", QueueSizeValue(QueueSize("1p")));
    Config::SetDefault("ns3::FifoQueueDisc::MaxSize", QueueSizeValue(QueueSize(queueDiscSize)));

    NodeContainer sender;
    NodeContainer receiver;
    NodeContainer routers;
    sender.Create(flows);
    receiver.Create(flows);
    routers.Create(2);

    PointToPointHelper bottleneckLink;
    bottleneckLink.SetDeviceAttribute("DataRate", StringValue(BOTTLENECK_RATE));
    bottleneckLink.SetChannelAttribute("Delay", TimeValue(MilliSeconds(6)));

    PointToPointHelper edgeLink;
    edgeLink.SetDeviceAttribute("DataRate", StringValue("1000Mbps"));
    edgeLink.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));

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
    tch.SetRootQueueDisc("ns3::FifoQueueDisc");
    tch.SetQueueLimits("ns3::DynamicQueueLimits", "HoldTime", StringValue("1000ms"));
    for (uint32_t i = 0; i < flows; i++)
    {
        tch.Install(senderEdges[i]);
        tch.Install(receiverEdges[i]);
    }

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.0.0.0", "255.255.255.0");
    ipv4.Assign(r1r2);
    ipv4.NewNetwork();
    std::vector<Ipv4InterfaceContainer> receiverInterface(flows);
    for (uint32_t i = 0; i < flows; i++)
    {
        ipv4.NewNetwork();
        ipv4.Assign(senderEdges[i]);
        ipv4.NewNetwork();
        receiverInterface[i] = ipv4.Assign(receiverEdges[i]);
    }
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    for (uint32_t i = 0; i < flows; i++)
    {
        Config::Set("/NodeList/" + std::to_string(sender.Get(i)->GetId()) +
                        "/$ns3::TcpL4Protocol/SocketType",
                    TypeIdValue(TypeId::LookupByName("ns3::" + cca[i])));

        uint16_t port = 50001 + i;
        BulkSendHelper source("ns3::TcpSocketFactory",
                              InetSocketAddress(receiverInterface[i].GetAddress(1), port));
        source.SetAttribute("MaxBytes", UintegerValue(0));
        ApplicationContainer sourceApps = source.Install(sender.Get(i));
        sourceApps.Start(start[i]);
        sourceApps.Stop(stop[i]);

        // The receiver outlives every sender, so that a flow which stops early closes cleanly
        PacketSinkHelper sink("ns3::TcpSocketFactory",
                              InetSocketAddress(Ipv4Address::GetAny(), port));
        ApplicationContainer sinkApps = sink.Install(receiver.Get(i));
        sinkApps.Start(Seconds(0));
        sinkApps.Stop(lastStop);
        m_sinks.push_back(DynamicCast<PacketSink>(sinkApps.Get(0)));
    }

    // Reinstall the queue disc on R1's bottleneck device, so that it can be traced and sampled
    tch.Uninstall(r1r2.Get(0));
    m_bottleneckQd = tch.Install(r1r2.Get(0)).Get(0);
    m_bottleneckQd->TraceConnectWithoutContext(
        "SojournTime",
        MakeCallback(&Ns3TcpLedbatPpBaseTestCase::SojournCb, this));
    Simulator::ScheduleNow(&Ns3TcpLedbatPpBaseTestCase::SampleQueue, this);
}

uint32_t
Ns3TcpLedbatPpBaseTestCase::AddWindow(Time start, Time end)
{
    uint32_t index = m_windows.size();
    Window w;
    w.start = start;
    w.end = end;
    m_windows.push_back(w);
    Simulator::Schedule(start, &Ns3TcpLedbatPpBaseTestCase::OpenWindow, this, index);
    Simulator::Schedule(end, &Ns3TcpLedbatPpBaseTestCase::CloseWindow, this, index);
    return index;
}

void
Ns3TcpLedbatPpBaseTestCase::OpenWindow(uint32_t window)
{
    Window& w = m_windows[window];
    for (const auto& sink : m_sinks)
    {
        w.rxOpen.push_back(sink->GetTotalRx());
    }
    w.open = true;
}

void
Ns3TcpLedbatPpBaseTestCase::CloseWindow(uint32_t window)
{
    Window& w = m_windows[window];
    for (const auto& sink : m_sinks)
    {
        w.rxClose.push_back(sink->GetTotalRx());
    }
    w.open = false;
}

void
Ns3TcpLedbatPpBaseTestCase::SojournCb(Time sojourn)
{
    for (auto& w : m_windows)
    {
        if (w.open)
        {
            w.sojournSum += sojourn;
            w.sojournCount++;
        }
    }
}

void
Ns3TcpLedbatPpBaseTestCase::SampleQueue()
{
    uint32_t packets = m_bottleneckQd->GetCurrentSize().GetValue();
    for (auto& w : m_windows)
    {
        if (w.open)
        {
            w.minQueue = std::min(w.minQueue, packets);
            w.queueSum += packets;
            w.queueCount++;
        }
    }
    Simulator::Schedule(QUEUE_SAMPLE_INTERVAL, &Ns3TcpLedbatPpBaseTestCase::SampleQueue, this);
}

double
Ns3TcpLedbatPpBaseTestCase::GetThroughput(uint32_t window, uint32_t flow) const
{
    const Window& w = m_windows[window];
    NS_ABORT_MSG_IF(w.rxClose.size() <= flow, "The measurement window never closed");
    double bytes = w.rxClose[flow] - w.rxOpen[flow];
    return bytes * 8.0 / (w.end - w.start).GetSeconds() / 1e6;
}

double
Ns3TcpLedbatPpBaseTestCase::GetTotalThroughput(uint32_t window) const
{
    double total = 0;
    for (uint32_t i = 0; i < m_sinks.size(); i++)
    {
        total += GetThroughput(window, i);
    }
    return total;
}

double
Ns3TcpLedbatPpBaseTestCase::GetShare(uint32_t window, uint32_t flow) const
{
    double total = GetTotalThroughput(window);
    return (total > 0) ? GetThroughput(window, flow) / total : 0.0;
}

double
Ns3TcpLedbatPpBaseTestCase::GetJainIndex(uint32_t window) const
{
    double sum = 0;
    double sumOfSquares = 0;
    for (uint32_t i = 0; i < m_sinks.size(); i++)
    {
        double x = GetThroughput(window, i);
        sum += x;
        sumOfSquares += x * x;
    }
    return (sumOfSquares > 0) ? (sum * sum) / (m_sinks.size() * sumOfSquares) : 0.0;
}

Time
Ns3TcpLedbatPpBaseTestCase::GetMeanSojourn(uint32_t window) const
{
    const Window& w = m_windows[window];
    return (w.sojournCount > 0) ? w.sojournSum / w.sojournCount : Time(0);
}

uint32_t
Ns3TcpLedbatPpBaseTestCase::GetMinQueue(uint32_t window) const
{
    return m_windows[window].minQueue;
}

double
Ns3TcpLedbatPpBaseTestCase::GetMeanQueue(uint32_t window) const
{
    const Window& w = m_windows[window];
    return (w.queueCount > 0) ? static_cast<double>(w.queueSum) / w.queueCount : 0.0;
}

/**
 * @ingroup system-tests-tcp
 *
 * @brief LEDBAT++ yields the bottleneck to a CUBIC flow, and takes it back when CUBIC leaves.
 */
class Ns3TcpLedbatPpYieldTestCase : public Ns3TcpLedbatPpBaseTestCase
{
  public:
    Ns3TcpLedbatPpYieldTestCase();

  private:
    void DoRun() override;
};

Ns3TcpLedbatPpYieldTestCase::Ns3TcpLedbatPpYieldTestCase()
    : Ns3TcpLedbatPpBaseTestCase("LEDBAT++ yields to CUBIC and recovers")
{
}

void
Ns3TcpLedbatPpYieldTestCase::DoRun()
{
    SetupFlows({"TcpLedbatPp", "TcpCubic"},
               {Seconds(0.1), Seconds(15)},
               {Seconds(60), Seconds(35)},
               "1000p");
    uint32_t shared = AddWindow(Seconds(25), Seconds(34));
    uint32_t alone = AddWindow(Seconds(45), Seconds(59));

    Simulator::Stop(Seconds(61));
    Simulator::Run();

    double share = GetShare(shared, 0);
    double recovered = GetThroughput(alone, 0);
    NS_LOG_INFO("shared: ledbat=" << GetThroughput(shared, 0)
                                  << " cubic=" << GetThroughput(shared, 1) << " share=" << share
                                  << "; alone: ledbat=" << recovered);

    NS_TEST_ASSERT_MSG_LT(share,
                          0.20,
                          "LEDBAT++ should yield most of the bottleneck to CUBIC, but took "
                              << share * 100 << "% of it");
    NS_TEST_ASSERT_MSG_GT(GetTotalThroughput(shared),
                          15.0,
                          "The bottleneck should stay busy while both flows are present");
    NS_TEST_ASSERT_MSG_GT(recovered,
                          15.0,
                          "LEDBAT++ should take the bottleneck back once CUBIC leaves, but only "
                          "reached "
                              << recovered << " Mbit/s");
    Simulator::Destroy();
}

/**
 * @ingroup system-tests-tcp
 *
 * @brief Four LEDBAT++ flows joining at different times share the bottleneck.
 */
class Ns3TcpLedbatPpFairnessTestCase : public Ns3TcpLedbatPpBaseTestCase
{
  public:
    Ns3TcpLedbatPpFairnessTestCase();

  private:
    void DoRun() override;
};

Ns3TcpLedbatPpFairnessTestCase::Ns3TcpLedbatPpFairnessTestCase()
    : Ns3TcpLedbatPpBaseTestCase("Inter-LEDBAT++ fairness")
{
}

void
Ns3TcpLedbatPpFairnessTestCase::DoRun()
{
    // Each flow measures its base delay while the flows already present are queuing, so the shares
    // converge closely rather than exactly. What must not happen is the latecomer advantage of
    // LEDBAT, where a flow arriving late starves the flows that were there first.
    SetupFlows({"TcpLedbatPp", "TcpLedbatPp", "TcpLedbatPp", "TcpLedbatPp"},
               {Seconds(0.1), Seconds(10), Seconds(20), Seconds(30)},
               {Seconds(100), Seconds(100), Seconds(100), Seconds(100)},
               "1000p");
    uint32_t steady = AddWindow(Seconds(70), Seconds(99));

    Simulator::Stop(Seconds(101));
    Simulator::Run();

    double jain = GetJainIndex(steady);
    NS_LOG_INFO("flows=" << GetThroughput(steady, 0) << "," << GetThroughput(steady, 1) << ","
                         << GetThroughput(steady, 2) << "," << GetThroughput(steady, 3)
                         << " jain=" << jain);

    NS_TEST_ASSERT_MSG_GT(jain,
                          0.85,
                          "The LEDBAT++ flows should share the bottleneck, but Jain's index was "
                          "only "
                              << jain);
    for (uint32_t i = 0; i < 4; i++)
    {
        NS_TEST_ASSERT_MSG_GT(GetShare(steady, i),
                              0.08,
                              "Flow " << i << " was starved, taking only "
                                      << GetShare(steady, i) * 100 << "% of the bottleneck");
    }
    NS_TEST_ASSERT_MSG_GT(GetTotalThroughput(steady), 18.0, "The bottleneck should stay busy");
    Simulator::Destroy();
}

/**
 * @ingroup system-tests-tcp
 *
 * @brief A lone LEDBAT++ flow holds the queuing delay near the target and keeps the link busy.
 */
class Ns3TcpLedbatPpBoundedDelayTestCase : public Ns3TcpLedbatPpBaseTestCase
{
  public:
    Ns3TcpLedbatPpBoundedDelayTestCase();

  private:
    void DoRun() override;
};

Ns3TcpLedbatPpBoundedDelayTestCase::Ns3TcpLedbatPpBoundedDelayTestCase()
    : Ns3TcpLedbatPpBaseTestCase("LEDBAT++ bounded delay when alone")
{
}

void
Ns3TcpLedbatPpBoundedDelayTestCase::DoRun()
{
    // 1000 packets hold about 580 ms at 20 Mbps, far more than the 60 ms target, so the target is
    // what bounds the delay rather than the buffer
    SetupFlows({"TcpLedbatPp"}, {Seconds(0.1)}, {Seconds(40)}, "1000p");
    uint32_t steady = AddWindow(Seconds(25), Seconds(39));

    Simulator::Stop(Seconds(41));
    Simulator::Run();

    Time sojourn = GetMeanSojourn(steady);
    double throughput = GetThroughput(steady, 0);
    NS_LOG_INFO("sojourn=" << sojourn.As(Time::MS) << " throughput=" << throughput);

    NS_TEST_ASSERT_MSG_LT(sojourn.GetMilliSeconds(),
                          90,
                          "The queuing delay should stay near the 60 ms target");
    NS_TEST_ASSERT_MSG_GT(sojourn.GetMilliSeconds(),
                          30,
                          "LEDBAT++ should build a queue up to the target, not run empty");
    NS_TEST_ASSERT_MSG_GT(throughput, 15.0, "A lone LEDBAT++ flow should keep the link busy");
    Simulator::Destroy();
}

/**
 * @ingroup system-tests-tcp
 *
 * @brief With a buffer too small to hold the target delay, the delay is bounded by the buffer and
 *        the flow keeps the link busy rather than starving.
 */
class Ns3TcpLedbatPpSmallBufferTestCase : public Ns3TcpLedbatPpBaseTestCase
{
  public:
    Ns3TcpLedbatPpSmallBufferTestCase();

  private:
    void DoRun() override;
};

Ns3TcpLedbatPpSmallBufferTestCase::Ns3TcpLedbatPpSmallBufferTestCase()
    : Ns3TcpLedbatPpBaseTestCase("LEDBAT++ small buffer fallback")
{
}

void
Ns3TcpLedbatPpSmallBufferTestCase::DoRun()
{
    // 20 packets of 1448 bytes at 20 Mbps hold about 12 ms, well under the 60 ms target, so the
    // target can never be reached and the flow becomes loss driven
    SetupFlows({"TcpLedbatPp"}, {Seconds(0.1)}, {Seconds(30)}, "20p");
    uint32_t steady = AddWindow(Seconds(15), Seconds(29));

    Simulator::Stop(Seconds(31));
    Simulator::Run();

    Time sojourn = GetMeanSojourn(steady);
    double throughput = GetThroughput(steady, 0);
    NS_LOG_INFO("sojourn=" << sojourn.As(Time::MS) << " throughput=" << throughput);

    NS_TEST_ASSERT_MSG_LT(sojourn.GetMilliSeconds(),
                          20,
                          "The buffer, not the target delay, should bound the queuing delay");
    NS_TEST_ASSERT_MSG_GT(throughput,
                          12.0,
                          "LEDBAT++ should keep the link busy when it cannot reach the target");
    Simulator::Destroy();
}

/**
 * @ingroup system-tests-tcp
 *
 * @brief The periodic slowdown of Section 4.4 drains the bottleneck queue.
 */
class Ns3TcpLedbatPpSlowdownDrainsTestCase : public Ns3TcpLedbatPpBaseTestCase
{
  public:
    Ns3TcpLedbatPpSlowdownDrainsTestCase();

  private:
    void DoRun() override;
};

Ns3TcpLedbatPpSlowdownDrainsTestCase::Ns3TcpLedbatPpSlowdownDrainsTestCase()
    : Ns3TcpLedbatPpBaseTestCase("LEDBAT++ slowdown drains the queue")
{
}

void
Ns3TcpLedbatPpSlowdownDrainsTestCase::DoRun()
{
    // A lone flow on this path slows down at roughly 0.8, 10.4, 20.2 and 30.0 seconds. The first
    // window falls between two slowdowns, where the queue must stay backlogged at the target. The
    // second window contains a slowdown, where the queue must empty.
    SetupFlows({"TcpLedbatPp"}, {Seconds(0.1)}, {Seconds(25)}, "1000p");
    uint32_t between = AddWindow(Seconds(12), Seconds(19));
    uint32_t during = AddWindow(Seconds(19.8), Seconds(22));

    Simulator::Stop(Seconds(26));
    Simulator::Run();

    NS_LOG_INFO("between: min=" << GetMinQueue(between) << " mean=" << GetMeanQueue(between)
                                << "; during: min=" << GetMinQueue(during)
                                << " mean=" << GetMeanQueue(during));

    NS_TEST_ASSERT_MSG_GT(GetMeanQueue(between),
                          50.0,
                          "Between slowdowns the queue should stay backlogged at the target");
    NS_TEST_ASSERT_MSG_GT(GetMinQueue(between),
                          20,
                          "Between slowdowns the queue should never empty");
    NS_TEST_ASSERT_MSG_LT(GetMinQueue(during),
                          5,
                          "The slowdown should drain the bottleneck queue, so that the base delay "
                          "can be re-measured");
    Simulator::Destroy();
}

/**
 * @ingroup system-tests-tcp
 *
 * @brief LEDBAT++ behaviour test suite.
 */
class Ns3TcpLedbatPpTestSuite : public TestSuite
{
  public:
    Ns3TcpLedbatPpTestSuite();
};

Ns3TcpLedbatPpTestSuite::Ns3TcpLedbatPpTestSuite()
    : TestSuite("ns3-tcp-ledbat-pp", Type::SYSTEM)
{
    AddTestCase(new Ns3TcpLedbatPpYieldTestCase, TestCase::Duration::EXTENSIVE);
    AddTestCase(new Ns3TcpLedbatPpFairnessTestCase, TestCase::Duration::EXTENSIVE);
    AddTestCase(new Ns3TcpLedbatPpBoundedDelayTestCase, TestCase::Duration::EXTENSIVE);
    AddTestCase(new Ns3TcpLedbatPpSmallBufferTestCase, TestCase::Duration::EXTENSIVE);
    AddTestCase(new Ns3TcpLedbatPpSlowdownDrainsTestCase, TestCase::Duration::EXTENSIVE);
}

/// Static variable for test initialization
static Ns3TcpLedbatPpTestSuite g_ns3TcpLedbatPpTestSuite;
