/*
 * @file flow-monitor-derived-stats-test.cc
 * @ingroup tests
 * @brief Unit tests for FlowStats derived stats methods.
*/

#include "ns3/flow-monitor.h"
#include "ns3/nstime.h"
#include "ns3/test.h"

using namespace ns3;

/**
 * @ingroup flow-monitor
 * @ingroup tests
 *
 * @brief Test FlowStats::GetMeanDelay and GetMeanJitter
 */
class FlowStatsDelayJitterTestCase : public TestCase
{
  public:
    FlowStatsDelayJitterTestCase();
    void DoRun() override;
};

FlowStatsDelayJitterTestCase::FlowStatsDelayJitterTestCase()
    : TestCase("FlowStats GetMeanDelay and GetMeanJitter")
{
}

void
FlowStatsDelayJitterTestCase::DoRun()
{
    FlowMonitor::FlowStats stats{};
    stats.rxPackets = 0;

    // Edge case: no packets received
    NS_TEST_EXPECT_MSG_EQ(stats.GetMeanDelay(),
                          Seconds(0),
                          "Mean delay should be zero when no packets received");
    NS_TEST_EXPECT_MSG_EQ(stats.GetMeanJitter(),
                          Seconds(0),
                          "Mean jitter should be zero when no packets received");

    // Edge case: single packet (jitter needs at least 2)
    stats.rxPackets = 1;
    stats.delaySum = Seconds(0.5);
    stats.jitterSum = Seconds(0);
    NS_TEST_EXPECT_MSG_EQ(stats.GetMeanDelay(),
                          Seconds(0.5),
                          "Mean delay should be 0.5s for one packet");
    NS_TEST_EXPECT_MSG_EQ(stats.GetMeanJitter(),
                          Seconds(0),
                          "Mean jitter should be zero with only one packet");

    // Normal case: 10 packets
    stats.rxPackets = 10;
    stats.delaySum = Seconds(1.0);
    stats.jitterSum = Seconds(0.9); // 9 jitter samples (rxPackets - 1)
    NS_TEST_ASSERT_MSG_EQ_TOL(stats.GetMeanDelay().GetSeconds(),
                              0.1,
                              1e-9,
                              "Mean delay should be 0.1s");
    NS_TEST_ASSERT_MSG_EQ_TOL(stats.GetMeanJitter().GetSeconds(),
                              0.1,
                              1e-9,
                              "Mean jitter should be 0.1s (0.9 / 9)");
}

/**
 * @ingroup flow-monitor
 * @ingroup tests
 *
 * @brief Test FlowStats::GetRxThroughput
 */
class FlowStatsThroughputTestCase : public TestCase
{
  public:
    FlowStatsThroughputTestCase();
    void DoRun() override;
};

FlowStatsThroughputTestCase::FlowStatsThroughputTestCase()
    : TestCase("FlowStats GetRxThroughput and GetTxOfferedLoad")
{
}

void
FlowStatsThroughputTestCase::DoRun()
{
    FlowMonitor::FlowStats stats{};

    // Edge case: zero duration
    stats.rxBytes = 125000; // 1 Mbit
    NS_TEST_EXPECT_MSG_EQ(stats.GetRxThroughput(Seconds(0)),
                          0.0,
                          "Throughput should be 0 for zero duration");

    // Edge case: negative duration
    NS_TEST_EXPECT_MSG_EQ(stats.GetRxThroughput(Seconds(-1)),
                          0.0,
                          "Throughput should be 0 for negative duration");

    // Normal case: explicit duration
    // 125000 bytes * 8 = 1000000 bits; 1000000 / 1.0s = 1000000 bps = 1 Mbps
    NS_TEST_ASSERT_MSG_EQ_TOL(stats.GetRxThroughput(Seconds(1.0)),
                              1000000.0,
                              1e-6,
                              "Throughput should be 1 Mbps (1000000 bps)");

    // Inferred duration: no packets received
    stats.rxPackets = 0;
    NS_TEST_EXPECT_MSG_EQ(stats.GetRxThroughput(),
                          0.0,
                          "Throughput should be 0 when no packets received");

    // Inferred duration: with timestamps
    stats.rxPackets = 10;
    stats.rxBytes = 125000;
    stats.timeFirstTxPacket = Seconds(1.0);
    stats.timeLastRxPacket = Seconds(2.0);
    // duration = 2.0 - 1.0 = 1.0s; 125000 * 8 / 1.0 = 1000000 bps
    NS_TEST_ASSERT_MSG_EQ_TOL(stats.GetRxThroughput(),
                              1000000.0,
                              1e-6,
                              "Inferred throughput should be 1 Mbps");

    // TxOfferedLoad: explicit duration
    stats.txBytes = 250000; // 2 Mbit
    NS_TEST_ASSERT_MSG_EQ_TOL(stats.GetTxOfferedLoad(Seconds(1.0)),
                              2000000.0,
                              1e-6,
                              "TxOffered should be 2 Mbps");

    // TxOfferedLoad: zero duration
    NS_TEST_EXPECT_MSG_EQ(stats.GetTxOfferedLoad(Seconds(0)),
                          0.0,
                          "TxOffered should be 0 for zero duration");

    // TxOfferedLoad: inferred, no tx packets
    stats.txPackets = 0;
    NS_TEST_EXPECT_MSG_EQ(stats.GetTxOfferedLoad(),
                          0.0,
                          "TxOffered should be 0 when no packets transmitted");

    // TxOfferedLoad: inferred with timestamps
    stats.txPackets = 20;
    stats.txBytes = 250000;
    stats.timeFirstTxPacket = Seconds(1.0);
    stats.timeLastTxPacket = Seconds(2.0);
    // duration = 1.0s; 250000 * 8 / 1.0 = 2000000 bps
    NS_TEST_ASSERT_MSG_EQ_TOL(stats.GetTxOfferedLoad(),
                              2000000.0,
                              1e-6,
                              "Inferred TxOffered should be 2 Mbps");
}

/**
 * @ingroup flow-monitor
 * @ingroup tests
 *
 * @brief Test FlowStats::GetPacketLossRatio
 */
class FlowStatsLossRatioTestCase : public TestCase
{
  public:
    FlowStatsLossRatioTestCase();
    void DoRun() override;
};

FlowStatsLossRatioTestCase::FlowStatsLossRatioTestCase()
    : TestCase("FlowStats GetPacketLossRatio")
{
}

void
FlowStatsLossRatioTestCase::DoRun()
{
    FlowMonitor::FlowStats stats{};

    // Edge case: no packets transmitted
    stats.txPackets = 0;
    stats.rxPackets = 0;
    NS_TEST_EXPECT_MSG_EQ(stats.GetPacketLossRatio(),
                          0.0,
                          "Loss ratio should be 0 when no packets transmitted");

    // No loss
    stats.txPackets = 100;
    stats.rxPackets = 100;
    NS_TEST_ASSERT_MSG_EQ_TOL(stats.GetPacketLossRatio(),
                              0.0,
                              1e-9,
                              "Loss ratio should be 0 when all packets received");

    // 50% loss
    stats.txPackets = 100;
    stats.rxPackets = 50;
    NS_TEST_ASSERT_MSG_EQ_TOL(stats.GetPacketLossRatio(),
                              0.5,
                              1e-9,
                              "Loss ratio should be 0.5 for 50% loss");

    // 100% loss
    stats.txPackets = 100;
    stats.rxPackets = 0;
    NS_TEST_ASSERT_MSG_EQ_TOL(stats.GetPacketLossRatio(),
                              1.0,
                              1e-9,
                              "Loss ratio should be 1.0 for 100% loss");
}

/**
 * @ingroup flow-monitor
 * @ingroup tests
 *
 * @brief FlowStats derived stats test suite
 */
class FlowMonitorDerivedStatsTestSuite : public TestSuite
{
  public:
    FlowMonitorDerivedStatsTestSuite()
        : TestSuite("flow-monitor-derived-stats", Type::UNIT)
    {
        AddTestCase(new FlowStatsDelayJitterTestCase, TestCase::Duration::QUICK);
        AddTestCase(new FlowStatsThroughputTestCase, TestCase::Duration::QUICK);
        AddTestCase(new FlowStatsLossRatioTestCase, TestCase::Duration::QUICK);
    }
};

static FlowMonitorDerivedStatsTestSuite g_flowMonitorDerivedStatsTestSuite;
