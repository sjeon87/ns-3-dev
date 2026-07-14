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

#include "tcp-general-test.h"

#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/tcp-ledbat-pp.h"
#include "ns3/tcp-socket-state.h"
#include "ns3/test.h"

#include <algorithm>

NS_LOG_COMPONENT_DEFINE("TcpLedbatPpTestSuite");

namespace ns3
{

/**
 * @ingroup internet-test
 *
 * @brief Validates the Slow Start Phase of LEDBAT++ algorithm.
 */
class TcpLedbatPpSlowStartTest : public TestCase
{
  public:
    /**
     * @brief Constructor
     *
     * @param cWnd congestion window
     * @param segmentSize segment size
     * @param rtt RTT
     * @param name Name of the test
     */
    TcpLedbatPpSlowStartTest(uint32_t cWnd,
                             uint32_t segmentSize,
                             Time rtt,
                             const std::string& name);

  private:
    void DoRun() override;

    /**
     * @brief Sets initial congestion window, segment size, RTT, and pushes
     * initial RTT sample to base and noise filters.
     */
    void SetupTest();

    /**
     * @brief Ensures that when queue delay is below threshold, cwnd increases
     * according to LEDBAT++ gain-based formula.
     */
    void CheckInitialSlowStartGrowth();

    /**
     * @brief Ensures that when queue delay exceeds 3/4 of target delay,
     * initial slow start is disabled and congestion avoidance is applied.
     */
    void CheckInitialSlowStartExitToCA();

    /**
     * @brief Ensures that LEDBAT++ re-enters slow start via slowdown logic
     * and follows correct growth formula.
     */
    void CheckFurtherSlowStart();

    /**
     * @brief Ensures that when cwnd exceeds ssthresh or delay conditions are met,
     * algorithm switches to congestion avoidance behavior.
     */
    void CheckFurtherSlowStartExit();

    uint32_t m_cWnd;             //!< cWnd
    uint32_t m_segmentSize;      //!< Segment size
    Time m_rtt;                  //!< RTT
    Ptr<TcpSocketState> m_state; //!< TCP Socket State
    Ptr<TcpLedbatPp> m_cong;     //!< LEDBAT++ Congestion control instance
};

TcpLedbatPpSlowStartTest::TcpLedbatPpSlowStartTest(uint32_t cWnd,
                                                   uint32_t segmentSize,
                                                   Time rtt,
                                                   const std::string& name)
    : TestCase(name),
      m_cWnd(cWnd),
      m_segmentSize(segmentSize),
      m_rtt(rtt)
{
}

void
TcpLedbatPpSlowStartTest::DoRun()
{
    SetupTest();
    CheckInitialSlowStartGrowth();
    Simulator::Schedule(Seconds(0.2),
                        &TcpLedbatPpSlowStartTest::CheckInitialSlowStartExitToCA,
                        this);
    Simulator::Schedule(Seconds(11.0), &TcpLedbatPpSlowStartTest::CheckFurtherSlowStart, this);
    Simulator::Schedule(Seconds(30.0), &TcpLedbatPpSlowStartTest::CheckFurtherSlowStartExit, this);
    Simulator::Run();
    Simulator::Destroy();
}

void
TcpLedbatPpSlowStartTest::SetupTest()
{
    m_state = CreateObject<TcpSocketState>();
    m_state->m_segmentSize = m_segmentSize;
    m_state->m_cWnd = m_cWnd;
    m_state->m_ssThresh = 10 * m_cWnd;
    m_state->m_lastRtt = m_rtt;
    m_cong = CreateObject<TcpLedbatPp>();

    // The suite runs this test with MSS = 1448 bytes, cWnd = 14480 bytes (10 segments) and
    // RTT = 10 ms. LEDBAT++ leaves its target delay at the default of 60 ms.
    //
    // A single-sample noise filter makes the current delay equal to the latest RTT sample, so the
    // checks below can set the delays directly, without any smoothing in between.
    m_cong->SetAttribute("NoiseFilterLen", UintegerValue(1));

    // Seed the base delay history and the noise filter with one RTT sample
    m_cong->PktsAcked(m_state, 1, m_rtt);
}

void
TcpLedbatPpSlowStartTest::CheckInitialSlowStartGrowth()
{
    // With the queue delay below the exit threshold, the connection should stay
    // in the initial slow start and grow the window there.
    m_cong->m_state = TcpLedbatPp::INITIAL_SLOW_START;
    m_state->m_lastRtt = MilliSeconds(10);
    m_cong->PktsAcked(m_state, 1, MilliSeconds(10)); // Base delay = 10ms
    m_state->m_lastRtt = MilliSeconds(20);
    m_cong->PktsAcked(m_state, 1, MilliSeconds(20)); // Current delay = 20ms
    m_state->m_lastRtt = m_rtt;

    // queue delay = current delay - base delay = 20 - 10 = 10 ms.
    // The initial slow start is left once the queue delay passes 3/4 of the target, that is
    // 3/4 * 60 = 45 ms. Here 10 ms < 45 ms, so the connection stays in the initial slow start.
    uint32_t segmentsAcked = 3;
    m_cong->IncreaseWindow(m_state, segmentsAcked);

    // Slow start growth, from Section 4.3 of the draft:
    //   GAIN = 1 / min(16, ceil(2 * target / base delay)) = 1 / min(16, ceil(120 / 10)) = 1 / 12
    //   cWnd += segmentsAcked * MSS * GAIN = 3 * 1448 / 12 = 362 bytes
    //   cWnd  = 14480 + 362 = 14842 bytes
    NS_TEST_ASSERT_MSG_EQ(m_state->m_cWnd.Get(),
                          14842,
                          "Initial slow start should increase by gain*MSS");
}

void
TcpLedbatPpSlowStartTest::CheckInitialSlowStartExitToCA()
{
    // Start from the initial slow start with a known window
    m_cong->m_state = TcpLedbatPp::INITIAL_SLOW_START;
    m_state->m_cWnd = m_cWnd;
    m_state->m_ssThresh = 10 * m_cWnd;

    // Base delay = 10ms; drive the current delay above 3/4 of the target
    m_state->m_lastRtt = MilliSeconds(10);
    m_cong->PktsAcked(m_state, 1, MilliSeconds(10)); // Base delay = 10ms
    m_state->m_lastRtt = MilliSeconds(60);
    m_cong->PktsAcked(m_state, 1, MilliSeconds(60)); // Current delay = 60ms
    m_state->m_lastRtt = m_rtt;

    // queue delay = current delay - base delay = 60 - 10 = 50 ms, which is above the
    // 3/4 * 60 = 45 ms exit threshold, so the initial slow start ends here
    uint32_t segmentsAcked = 3;
    m_cong->IncreaseWindow(m_state, segmentsAcked);

    NS_TEST_ASSERT_MSG_EQ(m_cong->m_state,
                          TcpLedbatPp::CONGESTION_AVOIDANCE,
                          "Must exit initial slow start into congestion avoidance");

    // The same ACK is then handled by congestion avoidance. The queue delay, 50 ms, is still below
    // the 60 ms target, so the additive increase of Section 4.2 applies:
    //   GAIN = 1 / min(16, ceil(2 * 60 / 10)) = 1 / 12
    //   cWnd += GAIN * MSS * MSS * segmentsAcked / cWnd
    //         = 1448 * 1448 * 3 / (12 * 14480) = 36 bytes
    //   cWnd  = 14480 + 36 = 14516 bytes
    NS_TEST_ASSERT_MSG_EQ(m_state->m_cWnd.Get(),
                          14516,
                          "After exit from slow start, congestion avoidance must be applied");
}

void
TcpLedbatPpSlowStartTest::CheckFurtherSlowStart()
{
    // During a slowdown, once the freeze has elapsed and cwnd is still below
    // ssthresh, LEDBAT++ ramps the window using the slow start growth formula.
    m_cong->m_state = TcpLedbatPp::SLOWDOWN;
    m_cong->m_freeze = Seconds(0); // freeze already elapsed
    m_state->m_cWnd = m_cWnd;
    m_state->m_ssThresh = 10 * m_cWnd; // keep cwnd below ssthresh during the ramp

    m_state->m_lastRtt = MilliSeconds(10);
    m_cong->PktsAcked(m_state, 1, MilliSeconds(10)); // Base delay = 10ms
    m_state->m_lastRtt = MilliSeconds(20);
    m_cong->PktsAcked(m_state, 1, MilliSeconds(20)); // Current delay = 20ms
    m_state->m_lastRtt = m_rtt;

    uint32_t segmentsAcked = 3;
    m_cong->IncreaseWindow(m_state, segmentsAcked);

    // The ramp back up out of a slowdown uses the same slow start growth as the initial slow
    // start, so with base delay = 10 ms the arithmetic is that of CheckInitialSlowStartGrowth:
    //   GAIN = 1 / min(16, ceil(2 * 60 / 10)) = 1 / 12
    //   cWnd += segmentsAcked * MSS * GAIN = 3 * 1448 / 12 = 362 bytes
    //   cWnd  = 14480 + 362 = 14842 bytes
    NS_TEST_ASSERT_MSG_EQ(m_state->m_cWnd.Get(),
                          14842,
                          "Slowdown ramp must use the slow start growth formula");
}

void
TcpLedbatPpSlowStartTest::CheckFurtherSlowStartExit()
{
    // During a slowdown, once cwnd reaches ssthresh, LEDBAT++ leaves the
    // slowdown, records its duration and schedules the next slowdown.
    m_cong->m_state = TcpLedbatPp::SLOWDOWN;
    m_cong->m_freeze = Seconds(0);                 // freeze already elapsed
    m_cong->m_slowdownEntry = MilliSeconds(29000); // slowdown began at t = 29s
    m_state->m_cWnd = m_cWnd;
    m_state->m_ssThresh = m_cWnd; // cwnd >= ssthresh triggers the exit

    m_state->m_lastRtt = MilliSeconds(10);
    m_cong->PktsAcked(m_state, 1, MilliSeconds(10)); // Base delay = 10ms
    m_state->m_lastRtt = MilliSeconds(20);
    m_cong->PktsAcked(m_state, 1, MilliSeconds(20)); // Current delay = 20ms
    m_state->m_lastRtt = m_rtt;

    uint32_t segmentsAcked = 3;
    m_cong->IncreaseWindow(m_state, segmentsAcked);

    // cWnd, 14480 bytes, has reached ssThresh, so the slowdown ends. This check is scheduled at
    // t = 30 s and the slowdown was made to begin at t = 29 s, so Section 4.4 gives:
    //   duration      = exit - entry            = 30000 - 29000 = 1000 ms
    //   next slowdown = exit + 9 * duration     = 30000 + 9000  = 39000 ms
    NS_TEST_ASSERT_MSG_EQ(m_cong->m_state,
                          TcpLedbatPp::CONGESTION_AVOIDANCE,
                          "Must leave the slowdown once cwnd reaches ssthresh");
    NS_TEST_ASSERT_MSG_EQ(m_cong->m_duration,
                          MilliSeconds(1000),
                          "Slowdown duration must be the entry-to-exit time");
    NS_TEST_ASSERT_MSG_EQ(m_cong->m_nextSdStart,
                          MilliSeconds(39000),
                          "Next slowdown must be scheduled at exit + 9 * duration");
}

/**
 * @ingroup internet-test
 *
 * @brief Validates the Congestion Avoidance Phase of LEDBAT++ algorithm.
 */
class TcpLedbatPpCongestionAvoidanceTest : public TestCase
{
  public:
    /**
     * @brief Constructor
     *
     * @param cWnd congestion window
     * @param segmentSize segment size
     * @param segmentsAcked segments acknowledged
     * @param rtt RTT
     * @param name Name of the test
     */
    TcpLedbatPpCongestionAvoidanceTest(uint32_t cWnd,
                                       uint32_t segmentSize,
                                       uint32_t segmentsAcked,
                                       Time rtt,
                                       const std::string& name)
        : TestCase(name),
          m_cWnd(cWnd),
          m_segmentSize(segmentSize),
          m_segmentsAcked(segmentsAcked),
          m_rtt(rtt)
    {
    }

  private:
    void DoRun() override;

    uint32_t m_cWnd;             //!< cWnd
    uint32_t m_segmentSize;      //!< Segment size
    uint32_t m_segmentsAcked;    //!< Segments acknowledged
    Time m_rtt;                  //!< RTT
    Ptr<TcpSocketState> m_state; //!< TCP Socket State
    Ptr<TcpLedbatPp> m_cong;     //!< LEDBAT++ Congestion control instance
};

void
TcpLedbatPpCongestionAvoidanceTest::DoRun()
{
    m_state = CreateObject<TcpSocketState>();
    m_state->m_cWnd = m_cWnd;
    m_state->m_segmentSize = m_segmentSize;

    m_cong = CreateObject<TcpLedbatPp>();

    // One sample per filter makes the base delay the smaller of the two RTT samples below, and
    // the current delay the latest, so the checks exercise the formulas without any smoothing
    m_cong->SetAttribute("BaseHistoryLen", UintegerValue(1));
    m_cong->SetAttribute("NoiseFilterLen", UintegerValue(1));

    // Seed the filters with two RTT samples
    m_state->m_lastRtt = MilliSeconds(20);
    m_cong->PktsAcked(m_state, m_segmentsAcked, MilliSeconds(20)); // Base delay = 20
    m_state->m_lastRtt = MilliSeconds(30);
    m_cong->PktsAcked(m_state, m_segmentsAcked, MilliSeconds(30)); // Current delay = 30
    m_state->m_lastRtt = m_rtt;

    m_cong->m_state = TcpLedbatPp::CONGESTION_AVOIDANCE;

    // Case A: the queue delay is below the target, so the window grows by the additive increase.
    //
    //   queue delay = current delay - base delay = 30 - 20 = 10 ms, below the 60 ms target
    //   GAIN = 1 / min(16, ceil(2 * target / base delay)) = 1 / min(16, ceil(120 / 20)) = 1 / 6
    //   cWnd += GAIN * MSS * MSS * segmentsAcked / cWnd
    //         = 1448 * 1448 * 1 / (6 * 14480) = 24 bytes
    //   cWnd  = 14480 + 24 = 14504 bytes
    m_cong->IncreaseWindow(m_state, m_segmentsAcked);

    NS_TEST_ASSERT_MSG_EQ(m_state->m_cWnd.Get(),
                          14504,
                          "LEDBAT++ cwnd incorrect for LOW delay (queue_delay < target)");

    // Case B: a 200 ms RTT sample drives the queue delay above the target, so the window shrinks
    // by the multiplicative decrease of Section 4.2.
    m_state->m_lastRtt = MilliSeconds(200);
    m_cong->PktsAcked(m_state, m_segmentsAcked, MilliSeconds(200));
    m_state->m_lastRtt = m_rtt;

    //   queue delay = 200 - 20 = 180 ms, which is three times the 60 ms target
    //   excess      = queue delay / target - 1 = 180 / 60 - 1 = 2
    //   cWndDelta   = (GAIN * MSS - Constant * cWnd * excess) * segmentsAcked * MSS / cWnd
    //               = (1448 / 6 - 1 * 14504 * 2) * 1 * 1448 / 14504 = -2872 bytes
    //
    // The draft never lets the window fall by more than half per RTT. Spread over the ACKs that
    // arrive in one RTT, that is half a segment per acked segment:
    //
    //   cWndDelta   = max(-2872, -0.5 * segmentsAcked * MSS) = max(-2872, -724) = -724 bytes
    //   cWnd        = 14504 - 724 = 13780 bytes
    m_cong->IncreaseWindow(m_state, m_segmentsAcked);

    NS_TEST_ASSERT_MSG_EQ(m_state->m_cWnd.Get(),
                          13780,
                          "LEDBAT++ cwnd incorrect for HIGH delay (queue_delay > target)");

    // Case C: the queue delay is still 180 ms, but the window is now small enough that the
    // decrease would take it under the MinCwnd floor of two segments.
    //
    //   cWnd        = 3 * MSS = 4344 bytes, and three segments are acknowledged
    //   cWndDelta   = (1448 / 6 - 1 * 4344 * 2) * 3 * 1448 / 4344 = -8447 bytes
    //   cWndDelta   = max(-8447, -0.5 * 3 * 1448) = -2172 bytes
    //   cWnd        = 4344 - 2172 = 2172 bytes, which is below the floor
    //   cWnd        = max(2172, MinCwnd * MSS) = max(2172, 2896) = 2896 bytes
    m_state->m_cWnd = 3 * m_segmentSize;
    m_cong->IncreaseWindow(m_state, 3);

    NS_TEST_ASSERT_MSG_EQ(m_state->m_cWnd.Get(),
                          2 * m_segmentSize,
                          "LEDBAT++ cwnd must never fall below the MinCwnd floor");

    // The same floor applies to the slow start threshold after a congestion signal, which is
    // halved but never taken below MinCwnd segments
    m_state->m_cWnd = 2 * m_segmentSize;

    NS_TEST_ASSERT_MSG_EQ(m_cong->GetSsThresh(m_state, 0),
                          2 * m_segmentSize,
                          "LEDBAT++ ssThresh must never fall below the MinCwnd floor");

    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup internet-test
 *
 * @brief Validates the SlowDown Phase of LEDBAT++ algorithm.
 */
class TcpLedbatPpSlowdownTest : public TestCase
{
  public:
    /**
     * @brief Constructor
     *
     * @param cWnd congestion window
     * @param segmentSize segment size
     * @param rtt RTT
     * @param name Name of the test
     */
    TcpLedbatPpSlowdownTest(uint32_t cWnd, uint32_t segmentSize, Time rtt, const std::string& name);

  private:
    void DoRun() override;

    /**
     * @brief Initializes TCP state and LEDBAT++ instance for slowdown testing. Sets initial values
     * and pushes RTT sample for delay estimation.
     */
    void SetupTest();

    /**
     * @brief Validates entry into slowdown phase and freeze behavior. Ensures cwnd is reduced to
     * 2*MSS and freeze timing is correctly set.
     */
    void CheckStartFreeze();

    /**
     * @brief Validates cwnd behavior during freeze period. Ensures cwnd remains constant (frozen)
     * during slowdown freeze duration.
     */
    void CheckDuringFreeze();

    /**
     * @brief Validates exit from slowdown phase and rescheduling logic.
     */
    void CheckEndFreezeAndReschedule();

    /**
     * @brief Validates second slowdown cycle behavior.
     */
    void CheckSecondFreeze();

    uint32_t m_cWnd;             //!< cWnd
    uint32_t m_segmentSize;      //!< Segment size
    Time m_rtt;                  //!< RTT
    Ptr<TcpSocketState> m_state; //!< TCP Socket State
    Ptr<TcpLedbatPp> m_cong;     //!< LEDBAT++ Congestion control instance
};

TcpLedbatPpSlowdownTest::TcpLedbatPpSlowdownTest(uint32_t cWnd,
                                                 uint32_t segmentSize,
                                                 Time rtt,
                                                 const std::string& name)
    : TestCase(name),
      m_cWnd(cWnd),
      m_segmentSize(segmentSize),
      m_rtt(rtt)
{
}

void
TcpLedbatPpSlowdownTest::DoRun()
{
    Simulator::Schedule(Seconds(0.0), &TcpLedbatPpSlowdownTest::SetupTest, this);

    // freeze start
    Simulator::Schedule(Seconds(1.0), &TcpLedbatPpSlowdownTest::CheckStartFreeze, this);

    // freeze window (1.000s to 1.020s)
    Simulator::Schedule(Seconds(1.010), &TcpLedbatPpSlowdownTest::CheckDuringFreeze, this);

    // after freeze expires (as freeze ends at 1.020s)
    Simulator::Schedule(Seconds(1.050),
                        &TcpLedbatPpSlowdownTest::CheckEndFreezeAndReschedule,
                        this);

    // freezes again after m_nextSdStart (as m_nextSdStart = 1.500s)
    Simulator::Schedule(Seconds(1.530), &TcpLedbatPpSlowdownTest::CheckSecondFreeze, this);

    Simulator::Run();
    Simulator::Destroy();
}

void
TcpLedbatPpSlowdownTest::SetupTest()
{
    m_state = CreateObject<TcpSocketState>();
    m_state->m_segmentSize = m_segmentSize;
    m_state->m_cWnd = m_cWnd;
    m_state->m_ssThresh = m_cWnd * 2;
    m_state->m_lastRtt = m_rtt;

    m_cong = CreateObject<TcpLedbatPp>();
    m_cong->PktsAcked(m_state, 1, m_rtt);
}

void
TcpLedbatPpSlowdownTest::CheckStartFreeze()
{
    // This check is scheduled at t = 1000 ms, and the next slowdown is made due at that instant,
    // so this ACK must start one. MSS is 1448 bytes and the RTT is 10 ms.
    m_cong->m_state = TcpLedbatPp::CONGESTION_AVOIDANCE;
    m_cong->m_nextSdStart = MilliSeconds(1000);

    m_cong->IncreaseWindow(m_state, 1);

    // Section 4.4 freezes the window at two segments for two round trips:
    //   entry  = now                 = 1000 ms
    //   freeze = entry + 2 * RTT     = 1000 + 20 = 1020 ms
    //   cWnd   = 2 * MSS             = 2 * 1448  = 2896 bytes
    NS_TEST_ASSERT_MSG_EQ(m_state->m_cWnd.Get(),
                          2896,
                          "At Slowdown start, cWnd must freeze to 2*MSS");

    NS_TEST_ASSERT_MSG_EQ(m_cong->m_state, TcpLedbatPp::SLOWDOWN, "Must be in the SLOWDOWN phase");

    NS_TEST_ASSERT_MSG_EQ(m_cong->m_slowdownEntry,
                          MilliSeconds(1000),
                          "Slowdown entry time calculated incorrectly");

    NS_TEST_ASSERT_MSG_EQ(m_cong->m_freeze,
                          MilliSeconds(1020),
                          "Freeze time calculated incorrectly");
}

void
TcpLedbatPpSlowdownTest::CheckDuringFreeze()
{
    // This check is scheduled at t = 1010 ms, inside the freeze that runs until 1020 ms. The
    // window is deliberately inflated to 69 segments first, to show that the slowdown pins it
    // back to 2 * MSS = 2896 bytes however large it was.
    m_state->m_cWnd = 69 * m_segmentSize;
    m_cong->IncreaseWindow(m_state, 1);

    NS_TEST_ASSERT_MSG_EQ(m_state->m_cWnd.Get(),
                          2896,
                          "cWnd should remain frozen inside the freeze duration");
}

void
TcpLedbatPpSlowdownTest::CheckEndFreezeAndReschedule()
{
    // This check is scheduled at t = 1050 ms, after the freeze ended at 1020 ms. The window,
    // 69 segments, is above ssThresh, 10 segments, so the ramp is over and the slowdown ends.
    m_state->m_cWnd = 69 * m_segmentSize;
    m_state->m_ssThresh = 10 * m_segmentSize;

    m_cong->IncreaseWindow(m_state, 1);

    // The slowdown began at 1000 ms, so Section 4.4 gives:
    //   duration      = exit - entry        = 1050 - 1000 = 50 ms
    //   next slowdown = exit + 9 * duration = 1050 + 450  = 1500 ms

    NS_TEST_ASSERT_MSG_EQ(m_cong->m_state,
                          TcpLedbatPp::CONGESTION_AVOIDANCE,
                          "Must leave the SLOWDOWN phase after the freeze");

    NS_TEST_ASSERT_MSG_EQ(m_cong->m_duration,
                          MilliSeconds(50),
                          "m_duration calculated incorrectly");

    NS_TEST_ASSERT_MSG_EQ(m_cong->m_nextSdStart,
                          MilliSeconds(1500),
                          "m_nextSdStart calculated incorrectly");

    NS_TEST_ASSERT_MSG_GT(m_state->m_cWnd.Get(),
                          2896,
                          "cWnd should increase (CA) after freeze expires");
}

void
TcpLedbatPpSlowdownTest::CheckSecondFreeze()
{
    // This check is scheduled at t = 1530 ms, past the 1500 ms that the previous check computed
    // as the time of the next slowdown, so a second slowdown must begin:
    //   entry  = now             = 1530 ms
    //   freeze = entry + 2 * RTT = 1530 + 20 = 1550 ms
    //   cWnd   = 2 * MSS         = 2896 bytes
    m_state->m_cWnd = 50 * m_segmentSize;
    m_state->m_ssThresh = 10 * m_segmentSize;

    m_cong->IncreaseWindow(m_state, 1);

    NS_TEST_ASSERT_MSG_EQ(m_cong->m_state,
                          TcpLedbatPp::SLOWDOWN,
                          "At 1530ms: Should be in slowdown mode again (second cycle)");

    NS_TEST_ASSERT_MSG_EQ(m_state->m_cWnd.Get(),
                          2896,
                          "At 1530ms: cWnd should be frozen to 2*MSS for second slowdown");

    NS_TEST_ASSERT_MSG_EQ(m_cong->m_slowdownEntry,
                          MilliSeconds(1530),
                          "Second slowdown entry time should be current time");

    NS_TEST_ASSERT_MSG_EQ(m_cong->m_freeze,
                          MilliSeconds(1550),
                          "Second freeze time should be set to entry + 2*RTT");
}

/**
 * @ingroup internet-test
 *
 * @brief Runs a LEDBAT++ flow over a real sender and receiver.
 *
 * The three tests above drive IncreaseWindow() by hand and check the arithmetic.
 * This one puts a LEDBAT++ socket at each end of a SimpleChannel and lets the ACK
 * clock run, so that the periodic slowdown of Section 4.4 of the draft is exercised
 * over real packets rather than on paper: the window freezes at two segments and
 * then ramps back up.
 *
 * The MinCwnd floor of Section 4.2 is not reached here, because the slowdown holds
 * the window at exactly two segments rather than driving it below them. The floor
 * itself is checked by TcpLedbatPpCongestionAvoidanceTest.
 *
 * The sender opens with a congestion window already above ssThresh, which makes
 * LEDBAT++ leave the initial slow start on the first ACK and schedule its first
 * slowdown two RTTs later, comfortably inside the transfer.
 */
class TcpLedbatPpClosedLoopTest : public TcpGeneralTest
{
  public:
    /**
     * @brief Constructor
     *
     * @param segmentSize segment size
     * @param packets number of packets the sending application queues
     * @param name Name of the test
     */
    TcpLedbatPpClosedLoopTest(uint32_t segmentSize, uint32_t packets, const std::string& name);

  protected:
    void ConfigureEnvironment() override;
    void ConfigureProperties() override;
    void CWndTrace(uint32_t oldValue, uint32_t newValue) override;
    void FinalChecks() override;

  private:
    uint32_t m_segmentSize;       //!< Segment size
    uint32_t m_packets;           //!< Packets the sending application queues
    uint32_t m_minCwnd;           //!< Smallest cWnd seen once the flow is running
    uint32_t m_cwndAfterSlowdown; //!< Largest cWnd seen after the first freeze
    bool m_sawSlowdownFreeze;     //!< Whether cWnd was ever frozen at 2 segments
};

TcpLedbatPpClosedLoopTest::TcpLedbatPpClosedLoopTest(uint32_t segmentSize,
                                                     uint32_t packets,
                                                     const std::string& name)
    : TcpGeneralTest(name),
      m_segmentSize(segmentSize),
      m_packets(packets),
      m_minCwnd(UINT32_MAX),
      m_cwndAfterSlowdown(0),
      m_sawSlowdownFreeze(false)
{
}

void
TcpLedbatPpClosedLoopTest::ConfigureEnvironment()
{
    TcpGeneralTest::ConfigureEnvironment();
    SetAppPktCount(m_packets);
    SetAppPktSize(m_segmentSize);
    SetPropagationDelay(MilliSeconds(50));
    SetTransmitStart(Seconds(0));
    SetCongestionControl(TcpLedbatPp::GetTypeId());
}

void
TcpLedbatPpClosedLoopTest::ConfigureProperties()
{
    TcpGeneralTest::ConfigureProperties();
    SetSegmentSize(SENDER, m_segmentSize);
    SetSegmentSize(RECEIVER, m_segmentSize);

    // Start above ssThresh so that the initial slow start is left on the first
    // ACK; the first slowdown is then due two RTTs later.
    SetInitialCwnd(SENDER, 10);
    SetInitialSsThresh(SENDER, 4 * m_segmentSize);
}

void
TcpLedbatPpClosedLoopTest::CWndTrace(uint32_t oldValue [[maybe_unused]], uint32_t newValue)
{
    if (newValue == 0)
    {
        return;
    }

    m_minCwnd = std::min(m_minCwnd, newValue);

    if (newValue == 2 * m_segmentSize)
    {
        m_sawSlowdownFreeze = true;
    }
    else if (m_sawSlowdownFreeze)
    {
        m_cwndAfterSlowdown = std::max(m_cwndAfterSlowdown, newValue);
    }
}

void
TcpLedbatPpClosedLoopTest::FinalChecks()
{
    NS_TEST_ASSERT_MSG_EQ(m_sawSlowdownFreeze,
                          true,
                          "A slowdown must occur and freeze cWnd at 2 segments");

    NS_TEST_ASSERT_MSG_GT_OR_EQ(m_minCwnd,
                                2 * m_segmentSize,
                                "cWnd must never drop below two segments during the transfer");

    NS_TEST_ASSERT_MSG_GT(m_cwndAfterSlowdown,
                          2 * m_segmentSize,
                          "cWnd must ramp back up once the slowdown ends");
}

/**
 * @ingroup internet-test
 *
 * @brief TCP LedbatPp TestSuite
 */
class TcpLedbatPpTestSuite : public TestSuite
{
  public:
    TcpLedbatPpTestSuite()
        : TestSuite("tcp-ledbat-pp-test", TestSuite::Type::UNIT)
    {
        AddTestCase(new TcpLedbatPpSlowStartTest(14480, 1448, MilliSeconds(10), "SlowStart-Test"),
                    TestCase::Duration::QUICK);

        AddTestCase(new TcpLedbatPpCongestionAvoidanceTest(14480,
                                                           1448,
                                                           1,
                                                           MilliSeconds(10),
                                                           "CongestionAvoidance-Test"),
                    TestCase::Duration::QUICK);

        AddTestCase(new TcpLedbatPpSlowdownTest(14480, 1448, MilliSeconds(10), "Slowdown-Test"),
                    TestCase::Duration::QUICK);

        AddTestCase(new TcpLedbatPpClosedLoopTest(500, 100, "ClosedLoop-Test"),
                    TestCase::Duration::QUICK);
    }
};

static TcpLedbatPpTestSuite g_tcpLedbatPpTestSuite; //!< static variable for test initialization

} // namespace ns3
