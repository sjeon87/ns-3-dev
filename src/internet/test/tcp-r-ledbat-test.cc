/*
 * Copyright (c) 2026 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:  Jayesh Akot <akotjayesh@gmail.com>
 *          S B L Prateek <sblprateek@gmail.com>
 *          A R Sharan Kumar <arsharankumar99@gmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */

#include "ns3/internet-stack-helper.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/tcp-header.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/tcp-linux-reno.h"
#include "ns3/tcp-option-ts.h"
#include "ns3/tcp-prr-recovery.h"
#include "ns3/tcp-r-ledbat.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/test.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpRLedbatTest");

/**
 * @brief Build a received TCP data segment with a TcpOptionTS timestamp.
 * @param payloadBytes Payload size in bytes.
 * @param tsVal        TCP timestamp value.
 * @param seqNo        Sequence number.
 * @return             The constructed packet.
 */
static Ptr<Packet>
MakeSeg(uint32_t payloadBytes, uint32_t tsVal, uint32_t seqNo)
{
    TcpHeader hdr;
    hdr.SetFlags(TcpHeader::ACK);
    hdr.SetSequenceNumber(SequenceNumber32(seqNo));
    hdr.SetAckNumber(SequenceNumber32(1));
    hdr.SetWindowSize(65535);

    Ptr<TcpOptionTS> ts = CreateObject<TcpOptionTS>();
    ts->SetTimestamp(tsVal);
    ts->SetEcho(0);
    hdr.AppendOption(ts);

    Ptr<Packet> pkt = Create<Packet>(payloadBytes);
    pkt->AddHeader(hdr);
    return pkt;
}

/**
 * @brief Create a TcpRLedbat socket on a fresh node.
 * @param segSize  Segment size in bytes.
 * @param targetMs Target queuing delay in milliseconds.
 * @param doSS     Slow-start mode.
 * @param noiseLen NoiseFilterLen.
 * @return         Configured TcpRLedbat socket.
 */
static Ptr<TcpRLedbat>
CreateRLedbatSocket(uint32_t segSize,
                    uint32_t targetMs,
                    TcpRLedbat::SlowStartType doSS,
                    uint32_t noiseLen)
{
    Ptr<Node> node = CreateObject<Node>();
    InternetStackHelper inet;
    inet.Install(node);

    Ptr<TcpL4Protocol> tcp4 = node->GetObject<TcpL4Protocol>();
    NS_ASSERT_MSG(tcp4, "No TcpL4Protocol on node");

    Ptr<Socket> raw = tcp4->CreateSocket(TcpRLedbat::GetTypeId(),
                                         TcpLinuxReno::GetTypeId(),
                                         TcpPrrRecovery::GetTypeId());

    Ptr<TcpRLedbat> sock = DynamicCast<TcpRLedbat>(raw);
    NS_ASSERT_MSG(sock, "DynamicCast<TcpRLedbat> failed");

    sock->SetAttribute("SegmentSize", UintegerValue(segSize));
    sock->SetAttribute("TargetDelay", TimeValue(MilliSeconds(targetMs)));
    sock->SetAttribute("NoiseFilterLen", UintegerValue(noiseLen));
    sock->SetDoSs(doSS);
    return sock;
}

/**
 * @brief Schedule a segment delivery at an absolute simulation time.
 *
 * All segments are scheduled before a single Simulator::Run() call.
 * Per-segment Run() calls allow TcpSocketBase timer events to bleed across
 * boundaries and corrupt Simulator::Now(), invalidating OWD measurements.
 *
 * @param sock         Target socket.
 * @param payloadBytes Payload size in bytes.
 * @param tsVal        TCP timestamp value.
 * @param seqNo        Sequence number.
 * @param atMs         Absolute injection time in milliseconds.
 */
static void
ScheduleInject(Ptr<TcpRLedbat> sock,
               uint32_t payloadBytes,
               uint32_t tsVal,
               uint32_t seqNo,
               uint32_t atMs)
{
    Ptr<Packet> pkt = MakeSeg(payloadBytes, tsVal, seqNo);
    TcpHeader hdr;
    pkt->PeekHeader(hdr);
    Simulator::Schedule(MilliSeconds(atMs), &TcpRLedbat::ReceivedData, sock, pkt, hdr);
}

/**
 * @ingroup internet-test
 *
 * @brief RLWND should grow by one segment per ACK during slow start.
 *
 * All segments use tsVal=1 so base delay == current delay and queuingDelay=0.
 * RLWND: 500 (init) -> 1000 -> 1500 -> 2000 -> 2500 -> 3000
 */
class TcpRLedbatSlowStartTest : public TestCase
{
  public:
    /**
     * @brief Constructor.
     * @param segSize       Segment size in bytes.
     * @param numSegs       Number of segments to inject.
     * @param expectedRlwnd Expected RLWND after all segments.
     * @param name          Test name.
     */
    TcpRLedbatSlowStartTest(uint32_t segSize,
                            uint32_t numSegs,
                            uint32_t expectedRlwnd,
                            const std::string& name)
        : TestCase(name),
          m_segSize(segSize),
          m_numSegs(numSegs),
          m_expectedRlwnd(expectedRlwnd)
    {
    }

  private:
    void DoRun() override;

    uint32_t m_segSize;       //!< Segment size in bytes.
    uint32_t m_numSegs;       //!< Number of segments to inject.
    uint32_t m_expectedRlwnd; //!< Expected final RLWND.
};

void
TcpRLedbatSlowStartTest::DoRun()
{
    Ptr<TcpRLedbat> sock = CreateRLedbatSocket(m_segSize, 100, TcpRLedbat::DO_SLOWSTART, 4);

    uint32_t seq = 1000;
    for (uint32_t i = 0; i < m_numSegs; ++i)
    {
        ScheduleInject(sock, m_segSize, 1, seq, 100 + i * 10);
        seq += m_segSize;
    }

    Simulator::Stop(MilliSeconds(100 + (m_numSegs - 1) * 10 + 1));
    Simulator::Run();

    NS_TEST_ASSERT_MSG_EQ(sock->GetRLWND(),
                          m_expectedRlwnd,
                          "RLWND did not reach expected value after slow-start");
    Simulator::Destroy();
}

/**
 * @ingroup internet-test
 *
 * @brief RLWND should increase when OWD is below the target delay.
 *
 * OWD=5ms, target=100ms: queuingDelay=0, offTarget=1.0 every step.
 * delta_i = gain * offTarget * ackedBytes * segSize / RLWND_i
 *         = 1.0  * 1.0       * 500        * 500     / RLWND_i
 * RLWND: 500 -> 1000 -> 1250 -> 1450 -> 1622 -> 1776
 */
class TcpRLedbatCaIncreaseTest : public TestCase
{
  public:
    /**
     * @brief Constructor.
     * @param segSize       Segment size in bytes.
     * @param targetMs      Target queuing delay in milliseconds.
     * @param owd           One-way delay in milliseconds.
     * @param numSegs       Number of segments to inject.
     * @param expectedRlwnd Expected RLWND after all segments.
     * @param name          Test name.
     */
    TcpRLedbatCaIncreaseTest(uint32_t segSize,
                             uint32_t targetMs,
                             uint32_t owd,
                             uint32_t numSegs,
                             uint32_t expectedRlwnd,
                             const std::string& name)
        : TestCase(name),
          m_segSize(segSize),
          m_targetMs(targetMs),
          m_owd(owd),
          m_numSegs(numSegs),
          m_expectedRlwnd(expectedRlwnd)
    {
    }

  private:
    void DoRun() override;

    uint32_t m_segSize;       //!< Segment size in bytes.
    uint32_t m_targetMs;      //!< Target queuing delay in milliseconds.
    uint32_t m_owd;           //!< One-way delay in milliseconds.
    uint32_t m_numSegs;       //!< Number of segments to inject.
    uint32_t m_expectedRlwnd; //!< Expected final RLWND.
};

void
TcpRLedbatCaIncreaseTest::DoRun()
{
    Ptr<TcpRLedbat> sock =
        CreateRLedbatSocket(m_segSize, m_targetMs, TcpRLedbat::DO_NOT_SLOWSTART, 1);

    uint32_t seq = 1000;
    for (uint32_t i = 0; i < m_numSegs; ++i)
    {
        uint32_t simMs = 200 + i * 10;
        ScheduleInject(sock, m_segSize, simMs - m_owd, seq, simMs);
        seq += m_segSize;
    }

    Simulator::Stop(MilliSeconds(200 + (m_numSegs - 1) * 10 + 1));
    Simulator::Run();

    NS_TEST_ASSERT_MSG_EQ(sock->GetRLWND(),
                          m_expectedRlwnd,
                          "RLWND did not reach expected value (CA increase)");
    Simulator::Destroy();
}

/**
 * @ingroup internet-test
 *
 * @brief RLWND should decrease when OWD exceeds the target delay.
 *
 * Phase 1 - owdLow=5ms, 8 segments: RLWND grows 500 -> 2168.
 * Phase 2 - owdHigh=200ms, 6 segments:
 *   queuingDelay = 195ms > target = 100ms, offTarget = -0.95
 *   delta_i = -0.95 * 500 * 500 / RLWND_i
 *   RLWND: 2168 -> 2059 -> 1944 -> 1822 -> 1692 -> 1552 -> 1399
 */
class TcpRLedbatCaDecreaseTest : public TestCase
{
  public:
    /**
     * @brief Constructor.
     * @param segSize          Segment size in bytes.
     * @param targetMs         Target queuing delay in milliseconds.
     * @param owdLow           OWD for the increase phase in milliseconds.
     * @param numSegsIncrease  Number of segments in the increase phase.
     * @param owdHigh          OWD for the decrease phase in milliseconds.
     * @param numSegsDecrease  Number of segments in the decrease phase.
     * @param expectedFinal    Expected RLWND after the decrease phase.
     * @param name             Test name.
     */
    TcpRLedbatCaDecreaseTest(uint32_t segSize,
                             uint32_t targetMs,
                             uint32_t owdLow,
                             uint32_t numSegsIncrease,
                             uint32_t owdHigh,
                             uint32_t numSegsDecrease,
                             uint32_t expectedFinal,
                             const std::string& name)
        : TestCase(name),
          m_segSize(segSize),
          m_targetMs(targetMs),
          m_owdLow(owdLow),
          m_numSegsIncrease(numSegsIncrease),
          m_owdHigh(owdHigh),
          m_numSegsDecrease(numSegsDecrease),
          m_expectedFinal(expectedFinal)
    {
    }

  private:
    void DoRun() override;

    uint32_t m_segSize;         //!< Segment size in bytes.
    uint32_t m_targetMs;        //!< Target queuing delay in milliseconds.
    uint32_t m_owdLow;          //!< OWD for the increase phase in milliseconds.
    uint32_t m_numSegsIncrease; //!< Number of segments in the increase phase.
    uint32_t m_owdHigh;         //!< OWD for the decrease phase in milliseconds.
    uint32_t m_numSegsDecrease; //!< Number of segments in the decrease phase.
    uint32_t m_expectedFinal;   //!< Expected final RLWND.
};

void
TcpRLedbatCaDecreaseTest::DoRun()
{
    Ptr<TcpRLedbat> sock =
        CreateRLedbatSocket(m_segSize, m_targetMs, TcpRLedbat::DO_NOT_SLOWSTART, 1);

    uint32_t seq = 1000;

    for (uint32_t i = 0; i < m_numSegsIncrease; ++i)
    {
        uint32_t simMs = 200 + i * 20;
        ScheduleInject(sock, m_segSize, simMs - m_owdLow, seq, simMs);
        seq += m_segSize;
    }
    for (uint32_t i = 0; i < m_numSegsDecrease; ++i)
    {
        uint32_t simMs = 400 + i * 20;
        ScheduleInject(sock, m_segSize, simMs - m_owdHigh, seq, simMs);
        seq += m_segSize;
    }

    Simulator::Stop(MilliSeconds(400 + (m_numSegsDecrease - 1) * 20 + 1));
    Simulator::Run();

    NS_TEST_ASSERT_MSG_EQ(sock->GetRLWND(),
                          m_expectedFinal,
                          "RLWND after CA decrease does not match");
    Simulator::Destroy();
}

/**
 * @ingroup internet-test
 *
 * @brief RLWND should be clamped to minRlwnd x segSize when a decrease
 * would push it below the minimum window.
 *
 * Seg1 (OWD=5ms):   queuingDelay=0, delta=500 -> RLWND: 500 -> 1000
 * Seg2 (OWD=200ms): queuingDelay=195ms, offTarget=-0.95,
 *   delta = -0.95 * 500 * 500 / 1000 = -237.5 -> pendingReduction=237
 *   RLWND = 1000 - 237 = 763 < minRlwnd x segSize (1000) -> clamped to 1000
 */
class TcpRLedbatMinRlwndTest : public TestCase
{
  public:
    /**
     * @brief Constructor.
     * @param segSize       Segment size in bytes.
     * @param targetMs      Target queuing delay in milliseconds.
     * @param owdLow        OWD of the increase segment in milliseconds.
     * @param owdHigh       OWD of the decrease segment in milliseconds.
     * @param expectedFinal Expected RLWND after clamping.
     * @param name          Test name.
     */
    TcpRLedbatMinRlwndTest(uint32_t segSize,
                           uint32_t targetMs,
                           uint32_t owdLow,
                           uint32_t owdHigh,
                           uint32_t expectedFinal,
                           const std::string& name)
        : TestCase(name),
          m_segSize(segSize),
          m_targetMs(targetMs),
          m_owdLow(owdLow),
          m_owdHigh(owdHigh),
          m_expectedFinal(expectedFinal)
    {
    }

  private:
    void DoRun() override;

    uint32_t m_segSize;       //!< Segment size in bytes.
    uint32_t m_targetMs;      //!< Target queuing delay in milliseconds.
    uint32_t m_owdLow;        //!< OWD of the increase segment in milliseconds.
    uint32_t m_owdHigh;       //!< OWD of the decrease segment in milliseconds.
    uint32_t m_expectedFinal; //!< Expected final RLWND.
};

void
TcpRLedbatMinRlwndTest::DoRun()
{
    Ptr<TcpRLedbat> sock =
        CreateRLedbatSocket(m_segSize, m_targetMs, TcpRLedbat::DO_NOT_SLOWSTART, 1);

    uint32_t seq = 1000;
    ScheduleInject(sock, m_segSize, 200 - m_owdLow, seq, 200);
    seq += m_segSize;
    ScheduleInject(sock, m_segSize, 220 - m_owdHigh, seq, 220);

    Simulator::Stop(MilliSeconds(221));
    Simulator::Run();

    NS_TEST_ASSERT_MSG_EQ(sock->GetRLWND(),
                          m_expectedFinal,
                          "RLWND should be clamped to minRlwnd x segSize");
    Simulator::Destroy();
}

/**
 * @ingroup internet-test
 *
 * @brief RLWND should be halved and immediately drained upon retransmission
 * detection.
 *
 * DetectRetransmission fires when SEG.SEQ < RCV.HGH AND TSV.SEQ > TSV.HGH
 * (RFC 9840 section 4.3).
 *
 * Seg1 (seq=1000, tsval=195, simMs=200): rcvHgh=1500, tsvHgh=195, RLWND=1000
 * Seg2 (seq=1500, tsval=205, simMs=210): rcvHgh=2000, tsvHgh=205, RLWND=1250
 * Retx (seq=1000, tsval=515, simMs=520): OWD=5ms (no uint32_t wrap)
 *   ssthresh = max(1250/2, 2x500) = max(625, 1000) = 1000
 *   pendingReduction = 1250 - 1000 = 250
 *   DecreaseWindow(500): 500 >= 250 -> RLWND = 1250 - 250 = 1000
 */
class TcpRLedbatRetransmissionTest : public TestCase
{
  public:
    /**
     * @brief Constructor.
     * @param expectedRlwnd Expected RLWND after retransmission handling.
     * @param name          Test name.
     */
    TcpRLedbatRetransmissionTest(uint32_t expectedRlwnd, const std::string& name)
        : TestCase(name),
          m_expectedRlwnd(expectedRlwnd)
    {
    }

  private:
    void DoRun() override;

    uint32_t m_expectedRlwnd; //!< Expected RLWND after retransmission handling.
};

void
TcpRLedbatRetransmissionTest::DoRun()
{
    Ptr<TcpRLedbat> sock = CreateRLedbatSocket(500, 100, TcpRLedbat::DO_NOT_SLOWSTART, 1);

    ScheduleInject(sock, 500, 195, 1000, 200); // Seg1: rcvHgh = 1500, tsvHgh = 195
    ScheduleInject(sock, 500, 205, 1500, 210); // Seg2: rcvHgh = 2000, tsvHgh = 205, RLWND = 1250
    ScheduleInject(sock, 500, 515, 1000, 520); // Retx: seq < rcvHgh, tsval > tsvHgh

    Simulator::Stop(MilliSeconds(521));
    Simulator::Run();

    NS_TEST_ASSERT_MSG_EQ(sock->GetRLWND(),
                          m_expectedRlwnd,
                          "RLWND after retransmission detection does not match");
    Simulator::Destroy();
}

/**
 * @ingroup internet-test
 *
 * @brief DecreaseWindow should drain RLWND across two ACKs when a single
 * ACK is insufficient to retire the full pendingReduction (partial-reduction
 * path).
 *
 * Phase 1 - OWD=5ms, 6 segments: RLWND grows 500 -> 1000 -> 1250 -> 1450
 *                                               -> 1622 -> 1776 -> 1916
 * Phase 2 - OWD=600ms (simMs=700, tsval=100):
 *   queuingDelay=595ms, offTarget=-4.95
 *   delta = -4.95 * 500 * 500 / 1916 = -645.9 -> pendingReduction=645
 *   ackedBytes(500) < pendingReduction(645) -> PARTIAL: RLWND=1416, pendingReduction=145
 * Phase 3 - drain (simMs=701, tsval=101):
 *   ackedBytes(500) >= pendingReduction(145) -> FULL: RLWND=1416-145=1271
 */
class TcpRLedbatMultiStepDrainTest : public TestCase
{
  public:
    /**
     * @brief Constructor.
     * @param expectedFinal Expected RLWND after the drain completes.
     * @param name          Test name.
     */
    TcpRLedbatMultiStepDrainTest(uint32_t expectedFinal, const std::string& name)
        : TestCase(name),
          m_expectedFinal(expectedFinal)
    {
    }

  private:
    void DoRun() override;

    uint32_t m_expectedFinal; //!< Expected RLWND after complete drain.
};

void
TcpRLedbatMultiStepDrainTest::DoRun()
{
    Ptr<TcpRLedbat> sock = CreateRLedbatSocket(500, 100, TcpRLedbat::DO_NOT_SLOWSTART, 1);

    uint32_t seq = 1000;

    for (uint32_t i = 0; i < 6; ++i)
    {
        uint32_t simMs = 200 + i * 10;
        ScheduleInject(sock, 500, simMs - 5, seq, simMs);
        seq += 500;
    }

    ScheduleInject(sock, 500, 100, seq, 700); // OWD = 600 ms -> partial drain
    seq += 500;
    ScheduleInject(sock, 500, 101, seq, 701); // Completes drain

    Simulator::Stop(MilliSeconds(702));
    Simulator::Run();

    NS_TEST_ASSERT_MSG_EQ(sock->GetRLWND(),
                          m_expectedFinal,
                          "RLWND after multi-step DecreaseWindow drain does not match");
    Simulator::Destroy();
}

/**
 * @ingroup internet-test
 *
 * @brief Unit test suite for TcpRLedbat (RFC 9840 receiver-side LEDBAT).
 *
 * Tests exercise the RLWND management algorithm by feeding synthetic segments
 * directly into TcpRLedbat::ReceivedData(). No full TCP connection is
 * established; this isolates rLEDBAT logic from the rest of the TCP stack.
 */
class TcpRLedbatTestSuite : public TestSuite
{
  public:
    TcpRLedbatTestSuite()
        : TestSuite("tcp-r-ledbat-test", Type::UNIT)
    {
        AddTestCase(new TcpRLedbatSlowStartTest(500,
                                                5,
                                                3000,
                                                "SlowStart: 5 x 500B segments"
                                                " grow RLWND from 500 to 3000"),
                    TestCase::Duration::QUICK);

        AddTestCase(new TcpRLedbatCaIncreaseTest(500,
                                                 100,
                                                 5,
                                                 5,
                                                 1776,
                                                 "CA increase: OWD=5ms < target=100ms,"
                                                 " RLWND grows to 1776"),
                    TestCase::Duration::QUICK);

        AddTestCase(new TcpRLedbatCaDecreaseTest(500,
                                                 100,
                                                 5,
                                                 8,
                                                 200,
                                                 6,
                                                 1399,
                                                 "CA decrease: OWD=200ms > target=100ms,"
                                                 " RLWND drains from 2168 to 1399"),
                    TestCase::Duration::QUICK);

        AddTestCase(new TcpRLedbatMinRlwndTest(500,
                                               100,
                                               5,
                                               200,
                                               2 * 500,
                                               "MinRlwnd: RLWND clamped to"
                                               " minRlwnd x segSize (1000)"),
                    TestCase::Duration::QUICK);

        AddTestCase(new TcpRLedbatRetransmissionTest(1000,
                                                     "Retransmission: ssthresh halved"
                                                     " and RLWND drained to 1000"),
                    TestCase::Duration::QUICK);

        AddTestCase(new TcpRLedbatMultiStepDrainTest(1271,
                                                     "MultiStepDrain: pendingReduction >"
                                                     " ackedBytes triggers partial DecreaseWindow"),
                    TestCase::Duration::QUICK);
    }
};

static TcpRLedbatTestSuite g_tcpRLedbatTestSuite; //!< Static variable for test registration.
