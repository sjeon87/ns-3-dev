/*
 * Copyright (c) 2017 Trinity College Dublin
 * Copyright (c) 2025-26 NITK Surathkal (Porting to ns-3)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Rohit P. Tahiliani <rohit.tahil@gmail.com>
 *
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/double.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/pi-square-queue-disc.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/traffic-control-module.h"
#include "ns3/uinteger.h"

using namespace ns3;

/**
 * @ingroup traffic-control-test
 *
 * @brief PiSquare Queue Disc Test Item
 */
class PiSquareQueueDiscTestItem : public QueueDiscItem
{
  public:
    /**
     * Constructor
     *
     * @param p the packet
     * @param addr the address
     * @param protocol the L3 protocol number
     */
    PiSquareQueueDiscTestItem(Ptr<Packet> p, const Address& addr, uint16_t protocol);

    // Delete default constructor, copy constructor and assignment operator to avoid misuse
    PiSquareQueueDiscTestItem() = delete;
    PiSquareQueueDiscTestItem(const PiSquareQueueDiscTestItem&) = delete;
    PiSquareQueueDiscTestItem& operator=(const PiSquareQueueDiscTestItem&) = delete;

    void AddHeader() override;
    bool Mark() override;
};

PiSquareQueueDiscTestItem::PiSquareQueueDiscTestItem(Ptr<Packet> p,
                                                     const Address& addr,
                                                     uint16_t protocol)
    : QueueDiscItem(p, addr, protocol)
{
}

void
PiSquareQueueDiscTestItem::AddHeader()
{
}

bool
PiSquareQueueDiscTestItem::Mark()
{
    return false;
}

/**
 * @ingroup traffic-control-test
 *
 * @brief Test 1: basic enqueue/dequeue sanity test
 */
class PiSquareQueueDiscEnqueueDequeueTest : public TestCase
{
  public:
    PiSquareQueueDiscEnqueueDequeueTest();
    void DoRun() override;

  private:
    /**
     * Enqueue function
     * @param queue the queue disc
     * @param size the size
     * @param nPkt the number of packets
     */
    void Enqueue(Ptr<PiSquareQueueDisc> queue, uint32_t size, uint32_t nPkt);
    /**
     * Enqueue with delay function
     * @param queue the queue disc
     * @param size the size
     * @param nPkt the number of packets
     */
    void EnqueueWithDelay(Ptr<PiSquareQueueDisc> queue, uint32_t size, uint32_t nPkt);
    /**
     * Dequeue function
     * @param queue the queue disc
     * @param nPkt the number of packets
     */
    void Dequeue(Ptr<PiSquareQueueDisc> queue, uint32_t nPkt);
    /**
     * Dequeue with delay function
     * @param queue the queue disc
     * @param delay the delay
     * @param nPkt the number of packets
     */
    void DequeueWithDelay(Ptr<PiSquareQueueDisc> queue, double delay, uint32_t nPkt);
    /**
     * Run test function
     * @param mode the test mode
     */
    void RunPiSquareEnqueueDequeueTest(QueueSizeUnit mode);
};

PiSquareQueueDiscEnqueueDequeueTest::PiSquareQueueDiscEnqueueDequeueTest()
    : TestCase("Basic enqueue and dequeue operations, and attribute setting")
{
}

void
PiSquareQueueDiscEnqueueDequeueTest::RunPiSquareEnqueueDequeueTest(QueueSizeUnit mode)
{
    uint32_t pktSize = 0;

    // 1 for packets; pktSize for bytes
    uint32_t modeSize = 1;

    uint32_t qSize = 300;
    Ptr<PiSquareQueueDisc> queue = CreateObject<PiSquareQueueDisc>();

    // test 1: simple enqueue/dequeue with defaults, no drops
    Address dest;

    if (mode == QueueSizeUnit::BYTES)
    {
        // pktSize should be same as MeanPktSize to avoid performance gap between byte and packet
        // mode
        pktSize = 1000;
        modeSize = pktSize;
        qSize = qSize * modeSize;
    }

    NS_TEST_ASSERT_MSG_EQ(
        queue->SetAttributeFailSafe("MaxSize", QueueSizeValue(QueueSize(mode, qSize))),
        true,
        "Verify that we can actually set the attribute QueueLimit");

    Ptr<Packet> p1;
    Ptr<Packet> p2;
    Ptr<Packet> p3;
    Ptr<Packet> p4;
    Ptr<Packet> p5;
    Ptr<Packet> p6;
    Ptr<Packet> p7;
    Ptr<Packet> p8;
    p1 = Create<Packet>(pktSize);
    p2 = Create<Packet>(pktSize);
    p3 = Create<Packet>(pktSize);
    p4 = Create<Packet>(pktSize);
    p5 = Create<Packet>(pktSize);
    p6 = Create<Packet>(pktSize);
    p7 = Create<Packet>(pktSize);
    p8 = Create<Packet>(pktSize);

    queue->Initialize();
    NS_TEST_ASSERT_MSG_EQ(queue->GetCurrentSize().GetValue(),
                          0 * modeSize,
                          "There should be no packets in there");
    queue->Enqueue(Create<PiSquareQueueDiscTestItem>(p1, dest, 0));
    NS_TEST_ASSERT_MSG_EQ(queue->GetCurrentSize().GetValue(),
                          1 * modeSize,
                          "There should be one packet in there");
    queue->Enqueue(Create<PiSquareQueueDiscTestItem>(p2, dest, 0));
    NS_TEST_ASSERT_MSG_EQ(queue->GetCurrentSize().GetValue(),
                          2 * modeSize,
                          "There should be two packets in there");
    queue->Enqueue(Create<PiSquareQueueDiscTestItem>(p3, dest, 0));
    queue->Enqueue(Create<PiSquareQueueDiscTestItem>(p4, dest, 0));
    queue->Enqueue(Create<PiSquareQueueDiscTestItem>(p5, dest, 0));
    queue->Enqueue(Create<PiSquareQueueDiscTestItem>(p6, dest, 0));
    queue->Enqueue(Create<PiSquareQueueDiscTestItem>(p7, dest, 0));
    queue->Enqueue(Create<PiSquareQueueDiscTestItem>(p8, dest, 0));
    NS_TEST_ASSERT_MSG_EQ(queue->GetCurrentSize().GetValue(),
                          8 * modeSize,
                          "There should be eight packets in there");

    Ptr<QueueDiscItem> item;
    item = queue->Dequeue();
    NS_TEST_ASSERT_MSG_NE(item, nullptr, "I want to remove the first packet");
    NS_TEST_ASSERT_MSG_EQ(queue->GetCurrentSize().GetValue(),
                          7 * modeSize,
                          "There should be seven packets in there");
    NS_TEST_ASSERT_MSG_EQ(item->GetPacket()->GetUid(), p1->GetUid(), "was this the first packet ?");

    item = queue->Dequeue();
    NS_TEST_ASSERT_MSG_NE(item, nullptr, "I want to remove the second packet");
    NS_TEST_ASSERT_MSG_EQ(queue->GetCurrentSize().GetValue(),
                          6 * modeSize,
                          "There should be six packet in there");
    NS_TEST_ASSERT_MSG_EQ(item->GetPacket()->GetUid(),
                          p2->GetUid(),
                          "Was this the second packet ?");

    item = queue->Dequeue();
    NS_TEST_ASSERT_MSG_NE(item, nullptr, "I want to remove the third packet");
    NS_TEST_ASSERT_MSG_EQ(queue->GetCurrentSize().GetValue(),
                          5 * modeSize,
                          "There should be five packets in there");
    NS_TEST_ASSERT_MSG_EQ(item->GetPacket()->GetUid(), p3->GetUid(), "Was this the third packet ?");

    item = queue->Dequeue();
    item = queue->Dequeue();
    item = queue->Dequeue();
    item = queue->Dequeue();
    item = queue->Dequeue();

    item = queue->Dequeue();
    NS_TEST_ASSERT_MSG_EQ(item, nullptr, "There are really no packets in there");

    // test 2: more data with defaults, unforced drops but no forced drops
    queue = CreateObject<PiSquareQueueDisc>();
    pktSize = 1000; // pktSize != 0 because DequeueThreshold always works in bytes
    NS_TEST_ASSERT_MSG_EQ(
        queue->SetAttributeFailSafe("MaxSize", QueueSizeValue(QueueSize(mode, qSize))),
        true,
        "Verify that we can actually set the attribute MaxSize");
    NS_TEST_ASSERT_MSG_EQ(queue->SetAttributeFailSafe("A", DoubleValue(0.625)),
                          true,
                          "Verify that we can actually set the attribute A");
    NS_TEST_ASSERT_MSG_EQ(queue->SetAttributeFailSafe("B", DoubleValue(6.25)),
                          true,
                          "Verify that we can actually set the attribute B");
    NS_TEST_ASSERT_MSG_EQ(queue->SetAttributeFailSafe("Tupdate", TimeValue(Seconds(0.03))),
                          true,
                          "Verify that we can actually set the attribute Tupdate");
    NS_TEST_ASSERT_MSG_EQ(queue->SetAttributeFailSafe("Supdate", TimeValue(Seconds(0.0))),
                          true,
                          "Verify that we can actually set the attribute Supdate");
    NS_TEST_ASSERT_MSG_EQ(queue->SetAttributeFailSafe("DequeueThreshold", UintegerValue(10000)),
                          true,
                          "Verify that we can actually set the attribute DequeueThreshold");
    NS_TEST_ASSERT_MSG_EQ(
        queue->SetAttributeFailSafe("QueueDelayReference", TimeValue(Seconds(0.02))),
        true,
        "Verify that we can actually set the attribute QueueDelayReference");
    queue->Initialize();
    EnqueueWithDelay(queue, pktSize, 400);
    DequeueWithDelay(queue, 0.012, 400);
    Simulator::Stop(Seconds(8.0));
    Simulator::Run();
    QueueDisc::Stats st = queue->GetStats();
    uint32_t test2 = st.GetNDroppedPackets(PiSquareQueueDisc::UNFORCED_DROP);
    NS_TEST_EXPECT_MSG_NE(test2, 0, "There should some unforced drops");
    NS_TEST_ASSERT_MSG_EQ(st.GetNDroppedPackets(PiSquareQueueDisc::FORCED_DROP),
                          0,
                          "There should zero forced drops");

    // test 3: same as test 2, but with higher QueueDelayReference
    queue = CreateObject<PiSquareQueueDisc>();
    NS_TEST_ASSERT_MSG_EQ(
        queue->SetAttributeFailSafe("MaxSize", QueueSizeValue(QueueSize(mode, qSize))),
        true,
        "Verify that we can actually set the attribute MaxSize");
    NS_TEST_ASSERT_MSG_EQ(queue->SetAttributeFailSafe("A", DoubleValue(0.125)),
                          true,
                          "Verify that we can actually set the attribute A");
    NS_TEST_ASSERT_MSG_EQ(queue->SetAttributeFailSafe("B", DoubleValue(1.25)),
                          true,
                          "Verify that we can actually set the attribute B");
    NS_TEST_ASSERT_MSG_EQ(queue->SetAttributeFailSafe("Tupdate", TimeValue(Seconds(0.03))),
                          true,
                          "Verify that we can actually set the attribute Tupdate");
    NS_TEST_ASSERT_MSG_EQ(queue->SetAttributeFailSafe("Supdate", TimeValue(Seconds(0.0))),
                          true,
                          "Verify that we can actually set the attribute Supdate");
    NS_TEST_ASSERT_MSG_EQ(queue->SetAttributeFailSafe("DequeueThreshold", UintegerValue(10000)),
                          true,
                          "Verify that we can actually set the attribute DequeueThreshold");
    NS_TEST_ASSERT_MSG_EQ(
        queue->SetAttributeFailSafe("QueueDelayReference", TimeValue(Seconds(0.08))),
        true,
        "Verify that we can actually set the attribute QueueDelayReference");
    queue->Initialize();
    EnqueueWithDelay(queue, pktSize, 400);
    DequeueWithDelay(queue, 0.012, 400);
    Simulator::Stop(Seconds(8.0));
    Simulator::Run();
    st = StaticCast<PiSquareQueueDisc>(queue)->GetStats();
    uint32_t test3 = st.GetNDroppedPackets(PiSquareQueueDisc::UNFORCED_DROP);
    NS_TEST_EXPECT_MSG_LT(test3, test2, "Test 3 should have less unforced drops than test 2");
    NS_TEST_ASSERT_MSG_EQ(st.GetNDroppedPackets(PiSquareQueueDisc::FORCED_DROP),
                          0,
                          "There should zero forced drops");

    // test 4: same as test 2, but with lesser dequeue rate
    queue = CreateObject<PiSquareQueueDisc>();
    NS_TEST_ASSERT_MSG_EQ(
        queue->SetAttributeFailSafe("MaxSize", QueueSizeValue(QueueSize(mode, qSize))),
        true,
        "Verify that we can actually set the attribute MaxSize");
    NS_TEST_ASSERT_MSG_EQ(queue->SetAttributeFailSafe("A", DoubleValue(0.125)),
                          true,
                          "Verify that we can actually set the attribute A");
    NS_TEST_ASSERT_MSG_EQ(queue->SetAttributeFailSafe("B", DoubleValue(1.25)),
                          true,
                          "Verify that we can actually set the attribute B");
    NS_TEST_ASSERT_MSG_EQ(queue->SetAttributeFailSafe("Tupdate", TimeValue(Seconds(0.03))),
                          true,
                          "Verify that we can actually set the attribute Tupdate");
    NS_TEST_ASSERT_MSG_EQ(queue->SetAttributeFailSafe("Supdate", TimeValue(Seconds(0.0))),
                          true,
                          "Verify that we can actually set the attribute Supdate");
    NS_TEST_ASSERT_MSG_EQ(queue->SetAttributeFailSafe("DequeueThreshold", UintegerValue(10000)),
                          true,
                          "Verify that we can actually set the attribute DequeueThreshold");
    NS_TEST_ASSERT_MSG_EQ(
        queue->SetAttributeFailSafe("QueueDelayReference", TimeValue(Seconds(0.02))),
        true,
        "Verify that we can actually set the attribute QueueDelayReference");
    queue->Initialize();
    EnqueueWithDelay(queue, pktSize, 400);
    DequeueWithDelay(queue, 0.015, 400); // delay between two successive dequeue events is increased
    Simulator::Stop(Seconds(8.0));
    Simulator::Run();
    st = StaticCast<PiSquareQueueDisc>(queue)->GetStats();
    uint32_t test4 = st.GetNDroppedPackets(PiSquareQueueDisc::UNFORCED_DROP);
    NS_TEST_EXPECT_MSG_GT(test4, test2, "Test 4 should have more unforced drops than test 2");
    NS_TEST_ASSERT_MSG_EQ(st.GetNDroppedPackets(PiSquareQueueDisc::FORCED_DROP),
                          0,
                          "There should zero forced drops");
}

void
PiSquareQueueDiscEnqueueDequeueTest::Enqueue(Ptr<PiSquareQueueDisc> queue,
                                             uint32_t size,
                                             uint32_t nPkt)
{
    Address dest;
    for (uint32_t i = 0; i < nPkt; i++)
    {
        queue->Enqueue(Create<PiSquareQueueDiscTestItem>(Create<Packet>(size), dest, 0));
    }
}

void
PiSquareQueueDiscEnqueueDequeueTest::EnqueueWithDelay(Ptr<PiSquareQueueDisc> queue,
                                                      uint32_t size,
                                                      uint32_t nPkt)
{
    double delay = 0.01; // enqueue packets with delay
    for (uint32_t i = 0; i < nPkt; i++)
    {
        Simulator::Schedule(Time(Seconds((i + 1) * delay)),
                            &PiSquareQueueDiscEnqueueDequeueTest::Enqueue,
                            this,
                            queue,
                            size,
                            1);
    }
}

void
PiSquareQueueDiscEnqueueDequeueTest::Dequeue(Ptr<PiSquareQueueDisc> queue, uint32_t nPkt)
{
    for (uint32_t i = 0; i < nPkt; i++)
    {
        Ptr<QueueDiscItem> item = queue->Dequeue();
    }
}

void
PiSquareQueueDiscEnqueueDequeueTest::DequeueWithDelay(Ptr<PiSquareQueueDisc> queue,
                                                      double delay,
                                                      uint32_t nPkt)
{
    for (uint32_t i = 0; i < nPkt; i++)
    {
        Simulator::Schedule(Time(Seconds((i + 1) * delay)),
                            &PiSquareQueueDiscEnqueueDequeueTest::Dequeue,
                            this,
                            queue,
                            1);
    }
}

void
PiSquareQueueDiscEnqueueDequeueTest::DoRun()
{
    RunPiSquareEnqueueDequeueTest(QueueSizeUnit::PACKETS);
    RunPiSquareEnqueueDequeueTest(QueueSizeUnit::BYTES);
    Simulator::Destroy();
}

/**
 * @ingroup traffic-control-test
 *
 * @brief Test 2: Runs a dumbbell topology with multiple TCP and UDP flows over
 * a bottleneck link. The test monitors sojourn time at the queue discipline and
 * verifies that queue delay remains within predefined limits.
 */
class PiSquareQueueDiscDumbbellTest : public TestCase
{
  public:
    PiSquareQueueDiscDumbbellTest();
    void DoRun() override;

  private:
    Time m_tracePeakDelay;        ///< peak queue delay trace
    Time m_traceSteadyStateDelay; ///< steady state queue delay trace

    /**
     * Sojourn Trace callback
     * @param peakTimeLimit the maximum time by which the peak must occur
     * @param sojourn the sojourn time
     */
    void TraceSojourn(Time peakTimeLimit, Time sojourn);
    /**
     * Create and run dumbbell experiment
     *
     * @param nTcp number of tcp flows
     * @param nUdp number of udp flows
     * @param expectedPeakDelay expected peak delay
     * @param expectedSteadyStateDelay expected steady state delay
     *
     */
    void RunDumbbellExperiment(uint32_t nTcp,
                               uint32_t nUdp,
                               Time expectedPeakDelay,
                               Time expectedSteadyStateDelay);
};

PiSquareQueueDiscDumbbellTest::PiSquareQueueDiscDumbbellTest()
    : TestCase("PiSquare dumbbell queue delay validation test")
{
}

void
PiSquareQueueDiscDumbbellTest::TraceSojourn(Time peakTimeLimit, Time sojourn)
{
    if (Simulator::Now().Compare(peakTimeLimit) == -1)
    {
        if (sojourn.Compare(m_tracePeakDelay) == 1)
        {
            m_tracePeakDelay = sojourn;
        }
    }
    else
    {
        if (sojourn.Compare(m_traceSteadyStateDelay) == 1)
        {
            m_traceSteadyStateDelay = sojourn;
        }
    }
}

void
PiSquareQueueDiscDumbbellTest::RunDumbbellExperiment(uint32_t nTcp,
                                                     uint32_t nUdp,
                                                     Time expectedPeakDelay,
                                                     Time expectedSteadyStateDelay)
{
    m_tracePeakDelay = Seconds(0);
    m_traceSteadyStateDelay = Seconds(0);

    uint32_t pktSize = 512;
    double stopTime = 30.0;
    std::string queueDisc = "ns3::PiSquareQueueDisc";

    Config::SetDefault("ns3::OnOffApplication::PacketSize", UintegerValue(pktSize));
    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue("10Mbps"));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpNewReno::GetTypeId()));
    Config::SetDefault(queueDisc + "::MaxSize",
                       QueueSizeValue(QueueSize(QueueSizeUnit::PACKETS, 1000)));
    Config::SetDefault(queueDisc + "::A", DoubleValue(0.3125));
    Config::SetDefault(queueDisc + "::B", DoubleValue(3.125));
    Config::SetDefault(queueDisc + "::Tupdate", TimeValue(Seconds(0.03)));
    Config::SetDefault(queueDisc + "::QueueDelayReference", TimeValue(Seconds(0.02)));

    // Dumbbell topology
    PointToPointHelper bottleneck;
    bottleneck.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    bottleneck.SetChannelAttribute("Delay", StringValue("38ms"));

    PointToPointHelper leaf;
    leaf.SetDeviceAttribute("DataRate", StringValue("1000Mbps"));
    leaf.SetChannelAttribute("Delay", StringValue("1ms"));

    PointToPointDumbbellHelper d(nTcp + nUdp, leaf, nTcp + nUdp, leaf, bottleneck);

    InternetStackHelper stack;
    stack.InstallAll();

    TrafficControlHelper tch;
    tch.SetRootQueueDisc(queueDisc);

    QueueDiscContainer qdiscs = tch.Install(d.GetRight()->GetDevice(0));

    Ptr<QueueDisc> q = qdiscs.Get(0);

    // Trace sojourn Time
    q->TraceConnectWithoutContext(
        "SojournTime",
        MakeCallback(&PiSquareQueueDiscDumbbellTest::TraceSojourn, this, Seconds(5)));

    d.AssignIpv4Addresses(Ipv4AddressHelper("10.1.1.0", "255.255.255.0"),
                          Ipv4AddressHelper("10.2.1.0", "255.255.255.0"),
                          Ipv4AddressHelper("10.3.1.0", "255.255.255.0"));

    // Install Applications
    uint16_t port = 5001;

    PacketSinkHelper sinkHelperTcp("ns3::TcpSocketFactory",
                                   InetSocketAddress(Ipv4Address::GetAny(), port));
    PacketSinkHelper sinkHelperUdp("ns3::UdpSocketFactory",
                                   InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinks;

    for (uint32_t i = 0; i < nTcp; ++i)
    {
        sinks.Add(sinkHelperTcp.Install(d.GetLeft(i)));
    }

    for (uint32_t i = nTcp; i < nTcp + nUdp; ++i)
    {
        sinks.Add(sinkHelperUdp.Install(d.GetLeft(i)));
    }

    sinks.Start(Seconds(0.0));
    sinks.Stop(Seconds(stopTime));

    OnOffHelper clientTcp("ns3::TcpSocketFactory", Address());
    clientTcp.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    clientTcp.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    OnOffHelper clientUdp("ns3::UdpSocketFactory", Address());
    clientUdp.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    clientUdp.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer clients;

    // Add TCP clients
    for (uint32_t i = 0; i < nTcp; ++i)
    {
        AddressValue remote(InetSocketAddress(d.GetLeftIpv4Address(i), port));
        clientTcp.SetAttribute("Remote", remote);
        clients.Add(clientTcp.Install(d.GetRight(i)));
    }

    // Add UDP clients
    for (uint32_t i = nTcp; i < nTcp + nUdp; ++i)
    {
        AddressValue remote(InetSocketAddress(d.GetLeftIpv4Address(i), port));
        clientUdp.SetAttribute("Remote", remote);
        clients.Add(clientUdp.Install(d.GetRight(i)));
    }

    clients.Start(Seconds(0.1));
    clients.Stop(Seconds(stopTime));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Stop(Seconds(stopTime));
    Simulator::Run();
    Simulator::Destroy();

    // Queue delay bound check
    NS_TEST_ASSERT_MSG_NE(m_tracePeakDelay.GetSeconds(),
                          0.0,
                          "Peak queue delay should be more than zero");
    NS_TEST_ASSERT_MSG_NE(m_traceSteadyStateDelay.GetSeconds(),
                          0.0,
                          "Steady state queue delay should be more than zero");
    NS_TEST_ASSERT_MSG_LT(m_tracePeakDelay.GetMilliSeconds(),
                          expectedPeakDelay.GetMilliSeconds(),
                          "Queue delay exceeded expected peak delay "
                              << expectedPeakDelay.GetMilliSeconds() << "ms");
    NS_TEST_ASSERT_MSG_LT(m_traceSteadyStateDelay.GetMilliSeconds(),
                          expectedSteadyStateDelay.GetMilliSeconds(),
                          "Queue delay exceeded expected steady state delay "
                              << expectedSteadyStateDelay.GetMilliSeconds() << "ms");

    // Drop behavior check
    QueueDisc::Stats st = q->GetStats();
    auto forced = st.GetNDroppedPackets(PieQueueDisc::FORCED_DROP);
    auto unforced = st.GetNDroppedPackets(PieQueueDisc::UNFORCED_DROP);

    NS_TEST_ASSERT_MSG_EQ(forced, 0, "Forced drops should be zero");

    NS_TEST_ASSERT_MSG_GT(unforced, 0, "Expected some unforced drops");
}

void
PiSquareQueueDiscDumbbellTest::DoRun()
{
    // Scenario 1: 5 TCP traffic
    RunDumbbellExperiment(5, 0, MilliSeconds(120), MilliSeconds(40));

    // Scenario 2: 50 TCP traffic
    RunDumbbellExperiment(50, 0, MilliSeconds(180), MilliSeconds(50));

    // Scenario 3: Mix TCP and UDP traffic flows
    RunDumbbellExperiment(5, 2, MilliSeconds(182), MilliSeconds(40));
}

/**
 * @ingroup traffic-control-test
 *
 * @brief PiSquare Queue Disc Test Suite
 */
static class PiSquareQueueDiscTestSuite : public TestSuite
{
  public:
    PiSquareQueueDiscTestSuite()
        : TestSuite("pi-square-queue-disc", Type::UNIT)
    {
        AddTestCase(new PiSquareQueueDiscEnqueueDequeueTest(), TestCase::Duration::QUICK);
        AddTestCase(new PiSquareQueueDiscDumbbellTest(), TestCase::Duration::QUICK);
    }
} g_piSquareQueueTestSuite; ///< the test suit
