/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */

#include "ns3/log.h"
#include "ns3/tcp-cubic.h"
#include "ns3/tcp-linux-reno.h"
#include "ns3/tcp-socket-state.h"
#include "ns3/test.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpCwndLimitedTestSuite");

/**
 * @ingroup internet-test
 *
 * @brief Validate per-window cwnd usage tracking.
 */
class TcpPerWindowCwndUsageTrackingTest : public TestCase
{
  public:
    /**
     * @brief Constructor.
     * @param name test name
     */
    explicit TcpPerWindowCwndUsageTrackingTest(const std::string& name);

  private:
    void DoRun() override;
};

TcpPerWindowCwndUsageTrackingTest::TcpPerWindowCwndUsageTrackingTest(const std::string& name)
    : TestCase(name)
{
}

void
TcpPerWindowCwndUsageTrackingTest::DoRun()
{
    // Scenario 1: First sample is cwnd-limited.
    // The flow is currently utilizing its full window, so we record this state.
    Ptr<TcpSocketState> tcb = CreateObject<TcpSocketState>();
    tcb->m_segmentSize = 1000;
    tcb->m_cWnd = 10000;
    tcb->m_ssThresh = 20000;

    SequenceNumber32 sndUna(1000);
    SequenceNumber32 cwndUsageEnd(11000);
    tcb->UpdateCwndUsage(sndUna, cwndUsageEnd, true, 9000);

    NS_TEST_ASSERT_MSG_EQ(tcb->m_isCwndLimited, true, "Expected cwnd-limited sample to be stored");
    NS_TEST_ASSERT_MSG_EQ(tcb->m_maxBytesInFlight,
                          9000,
                          "Max in-flight bytes should track first sample");
    NS_TEST_ASSERT_MSG_EQ(tcb->m_cwndUsageSeq,
                          cwndUsageEnd,
                          "Usage window sequence should follow sndNxt");

    // Scenario 2: Stay in the same usage window; a non-limited sample must not clear the persisted
    // cwnd-limited state. As long as SND.UNA hasn't advanced past the usage sequence, we are in the
    // same window. We want the cwnd-limited signal to persist within the usage window so that
    // growth isn't suppressed by transient fluctuations in in-flight bytes.
    tcb->UpdateCwndUsage(SequenceNumber32(2000), SequenceNumber32(12000), false, 4000);
    NS_TEST_ASSERT_MSG_EQ(tcb->m_isCwndLimited,
                          true,
                          "Cwnd-limited signal should persist in the same window");
    NS_TEST_ASSERT_MSG_EQ(tcb->m_maxBytesInFlight,
                          9000,
                          "Max in-flight bytes should remain unchanged in the same window");
    NS_TEST_ASSERT_MSG_EQ(tcb->m_cwndUsageSeq,
                          cwndUsageEnd,
                          "Usage window sequence should remain unchanged in the same window");

    // Scenario 3: New usage window begins once SND.UNA reaches the previously stored usage
    // sequence. Advancing SND.UNA signals that the previous usage window has been fully
    // acknowledged. This should clear the persisted state and start tracking from the new sample.
    tcb->UpdateCwndUsage(cwndUsageEnd, SequenceNumber32(12000), false, 4000);
    NS_TEST_ASSERT_MSG_EQ(tcb->m_isCwndLimited,
                          false,
                          "New usage window should refresh cwnd-limited signal");
    NS_TEST_ASSERT_MSG_EQ(tcb->m_maxBytesInFlight,
                          4000,
                          "New usage window should refresh max in-flight bytes");
    NS_TEST_ASSERT_MSG_EQ(tcb->m_cwndUsageSeq,
                          SequenceNumber32(12000),
                          "New usage window should refresh usage sequence");
}

/**
 * @ingroup internet-test
 *
 * @brief Validate slow-start fallback in IsCwndLimited().
 */
class TcpCwndLimitedSlowStartTest : public TestCase
{
  public:
    /**
     * @brief Constructor.
     * @param name test name
     */
    explicit TcpCwndLimitedSlowStartTest(const std::string& name);

  private:
    void DoRun() override;
};

TcpCwndLimitedSlowStartTest::TcpCwndLimitedSlowStartTest(const std::string& name)
    : TestCase(name)
{
}

void
TcpCwndLimitedSlowStartTest::DoRun()
{
    // Scenario 1: Slow start fallback.
    // Flow is in slow start (ssThresh=20000 > cWnd=7000).
    // m_maxBytesInFlight=4000, so 2*maxBytesInFlight=8000.
    // With cWnd=7000 < 8000, IsCwndLimited() should return true via the
    // slow-start fallback, allowing cwnd growth to continue even if the
    // explicit per-window tracking flag is false.
    Ptr<TcpSocketState> tcb = CreateObject<TcpSocketState>();
    tcb->m_segmentSize = 1000;
    tcb->m_isCwndLimited = false;
    tcb->m_maxBytesInFlight = 4000;
    tcb->m_cWnd = 7000;
    tcb->m_ssThresh = 20000; // slow start

    NS_TEST_ASSERT_MSG_EQ(tcb->IsCwndLimited(),
                          true,
                          "Slow start should be cwnd-limited if cwnd < 2 * maxBytesInFlight");

    // Scenario 2: Slow start fallback should stop once cwnd reaches 2 * maxBytesInFlight.
    // This prevents the fallback from allowing growth indefinitely if the
    // flow is not actually limited.
    tcb->m_cWnd = 8000;
    NS_TEST_ASSERT_MSG_EQ(tcb->IsCwndLimited(),
                          false,
                          "Slow start fallback should stop once cwnd reaches 2 * maxBytesInFlight");

    // Scenario 3: Congestion avoidance should not use slow-start fallback.
    // Fallback is specifically a slow-start mechanism to ensure growth when
    // samples are sparse. In congestion avoidance, we require a strict
    // cwnd-limited measurement.
    tcb->m_ssThresh = 6000; // congestion avoidance
    NS_TEST_ASSERT_MSG_EQ(tcb->IsCwndLimited(),
                          false,
                          "Congestion avoidance should not use slow-start fallback");
}

/**
 * @ingroup internet-test
 *
 * @brief Validate cwnd growth suppression/allowance with IsCwndLimited().
 */
class TcpCwndLimitedCongAvoidGateTest : public TestCase
{
  public:
    /**
     * @brief Constructor.
     * @param name test name
     */
    explicit TcpCwndLimitedCongAvoidGateTest(const std::string& name);

  private:
    void DoRun() override;
};

TcpCwndLimitedCongAvoidGateTest::TcpCwndLimitedCongAvoidGateTest(const std::string& name)
    : TestCase(name)
{
}

void
TcpCwndLimitedCongAvoidGateTest::DoRun()
{
    // Scenario 1: Reno congestion avoidance should suppress growth when not cwnd-limited.
    // In congestion avoidance, we only increase cwnd if the flow is actually
    // limited by the window (to avoid over-aggressive growth).
    Ptr<TcpSocketState> renoState = CreateObject<TcpSocketState>();
    renoState->m_segmentSize = 1000;
    renoState->m_cWnd = 1000;
    renoState->m_ssThresh = 1000; // congestion avoidance
    renoState->m_isCwndLimited = false;
    renoState->m_maxBytesInFlight = 0;

    Ptr<TcpLinuxReno> reno = CreateObject<TcpLinuxReno>();
    reno->IncreaseWindow(renoState, 1);
    NS_TEST_ASSERT_MSG_EQ(renoState->m_cWnd.Get(),
                          1000u,
                          "Reno should not grow cwnd when flow is not cwnd-limited");

    // Scenario 2: Reno should grow cwnd when flow is cwnd-limited.
    renoState->m_isCwndLimited = true;
    reno->IncreaseWindow(renoState, 1);
    NS_TEST_ASSERT_MSG_EQ(renoState->m_cWnd.Get(),
                          2000u,
                          "Reno should grow cwnd when flow is cwnd-limited");

    // Cubic should rely on IsCwndLimited(), not only on the per-window tracking flag.
    // Cubic uses IsCwndLimited() to determine if it should enter the
    // window-growth phase. Without the fallback trigger (maxBytesInFlight=0),
    // it should be suppressed.
    Ptr<TcpSocketState> cubicState = CreateObject<TcpSocketState>();
    cubicState->m_segmentSize = 1000;
    cubicState->m_cWnd = 3000;
    cubicState->m_ssThresh = 10000; // slow start
    cubicState->m_isCwndLimited = false;
    cubicState->m_maxBytesInFlight = 0;

    Ptr<TcpCubic> cubic = CreateObject<TcpCubic>();
    cubic->IncreaseWindow(cubicState, 1);
    NS_TEST_ASSERT_MSG_EQ(cubicState->m_cWnd.Get(),
                          3000u,
                          "Cubic should suppress growth when IsCwndLimited() is false");

    // Scenario 4: Cubic should grow cwnd when IsCwndLimited() becomes true.
    // By setting maxBytesInFlight such that cWnd < 2 * maxBytesInFlight,
    // the slow-start fallback triggers, allowing growth.
    cubicState->m_maxBytesInFlight = 2000; // 3000 < 2 * 2000 => cwnd-limited in slow start
    cubic->IncreaseWindow(cubicState, 1);
    NS_TEST_ASSERT_MSG_EQ(cubicState->m_cWnd.Get(),
                          4000u,
                          "Cubic should grow cwnd when IsCwndLimited() becomes true");
}

/**
 * @ingroup internet-test
 *
 * @brief Test suite for cwnd-limited tracking.
 */
class TcpCwndLimitedTestSuite : public TestSuite
{
  public:
    TcpCwndLimitedTestSuite()
        : TestSuite("tcp-cwnd-limited-test", Type::UNIT)
    {
        AddTestCase(new TcpPerWindowCwndUsageTrackingTest(
                        "cwnd-limited per-window usage tracking behavior"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpCwndLimitedSlowStartTest("cwnd-limited slow-start fallback behavior"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpCwndLimitedCongAvoidGateTest(
                        "cwnd-limited congestion control growth suppression behavior"),
                    TestCase::Duration::QUICK);
    }
};

static TcpCwndLimitedTestSuite g_tcpCwndLimitedTestSuite;
