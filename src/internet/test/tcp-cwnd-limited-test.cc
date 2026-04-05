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
 * @brief Validate sticky cwnd-limited tracking within a usage window.
 */
class TcpCwndLimitedStickyTest : public TestCase
{
  public:
    /**
     * @brief Constructor.
     * @param name test name
     */
    explicit TcpCwndLimitedStickyTest(const std::string& name);

  private:
    void DoRun() override;
};

TcpCwndLimitedStickyTest::TcpCwndLimitedStickyTest(const std::string& name)
    : TestCase(name)
{
}

void
TcpCwndLimitedStickyTest::DoRun()
{
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

    // Stay in the same usage window; a non-limited sample must not clear the sticky signal.
    tcb->UpdateCwndUsage(SequenceNumber32(2000), SequenceNumber32(12000), false, 4000);
    NS_TEST_ASSERT_MSG_EQ(tcb->m_isCwndLimited,
                          true,
                          "Cwnd-limited signal should remain sticky in the same window");
    NS_TEST_ASSERT_MSG_EQ(tcb->m_maxBytesInFlight,
                          9000,
                          "Max in-flight bytes should remain unchanged in the same window");
    NS_TEST_ASSERT_MSG_EQ(tcb->m_cwndUsageSeq,
                          cwndUsageEnd,
                          "Usage window sequence should remain unchanged in the same window");

    // New usage window begins once SND.UNA reaches the previously stored usage sequence.
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
    Ptr<TcpSocketState> tcb = CreateObject<TcpSocketState>();
    tcb->m_segmentSize = 1000;
    tcb->m_isCwndLimited = false;
    tcb->m_maxBytesInFlight = 4000;
    tcb->m_cWnd = 7000;
    tcb->m_ssThresh = 20000; // slow start

    NS_TEST_ASSERT_MSG_EQ(tcb->IsCwndLimited(),
                          true,
                          "Slow start should be cwnd-limited if cwnd < 2 * maxBytesInFlight");

    tcb->m_cWnd = 8000;
    NS_TEST_ASSERT_MSG_EQ(tcb->IsCwndLimited(),
                          false,
                          "Slow start fallback should stop once cwnd reaches 2 * maxBytesInFlight");

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
    // Reno congestion avoidance should suppress growth when not cwnd-limited.
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

    renoState->m_isCwndLimited = true;
    reno->IncreaseWindow(renoState, 1);
    NS_TEST_ASSERT_MSG_EQ(renoState->m_cWnd.Get(),
                          2000u,
                          "Reno should grow cwnd when flow is cwnd-limited");

    // Cubic should rely on IsCwndLimited(), not only on the sticky flag.
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
        AddTestCase(new TcpCwndLimitedStickyTest("cwnd-limited sticky usage window behavior"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpCwndLimitedSlowStartTest("cwnd-limited slow-start fallback behavior"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpCwndLimitedCongAvoidGateTest(
                        "cwnd-limited congestion control growth suppression behavior"),
                    TestCase::Duration::QUICK);
    }
};

static TcpCwndLimitedTestSuite g_tcpCwndLimitedTestSuite;
