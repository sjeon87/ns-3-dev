/*
 * Copyright (c) 2026 GPRT, UFPE
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Maria Eduarda Veras <eduarda.martins@gprt.ufpe.br>
 *          Eduardo Freitas <eduardo.freitas@gprt.ufpe.br>
 *          Djamel Fawzi Hadj Sadok <jamel@gprt.ufpe.br>
 */

#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/dualpi2-queue-disc.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/uinteger.h"

using namespace ns3;

// ---------------------------------------------------------------------------
// ECN codepoint constants
// ---------------------------------------------------------------------------
static const uint8_t ECN_NOT_ECT = 0x00;
static const uint8_t ECN_ECT0 = 0x02;
static const uint8_t ECN_ECT1 = 0x01;
static const uint8_t ECN_CE = 0x03;

// ---------------------------------------------------------------------------
// Test item
// ---------------------------------------------------------------------------

/**
 * @ingroup traffic-control-test
 * @brief QueueDiscItem that reports a configurable IP DS field (ECN bits).
 *
 * DualPi2 classifies packets via QueueItem::GetUint8Value(IP_DSFIELD).
 * This helper overrides that method so tests can inject any ECN codepoint
 * without building a real IP packet.
 *
 * ECN encoding (low 2 bits):
 *   0x00 Not-ECT | 0x02 ECT(0) -> Classic queue
 *   0x01 ECT(1)       -> L4S queue
 */
class DualPi2TestItem : public QueueDiscItem
{
  public:
    DualPi2TestItem(Ptr<Packet> p, const Address& addr, uint8_t dsField)
        : QueueDiscItem(p, addr, 0),
          m_dsField(dsField)
    {
    }

    DualPi2TestItem() = delete;
    DualPi2TestItem(const DualPi2TestItem&) = delete;
    DualPi2TestItem& operator=(const DualPi2TestItem&) = delete;

    void AddHeader() override
    {
    }

    bool Mark() override
    {
        if ((m_dsField & 0x03) != ECN_NOT_ECT)
        {
            m_dsField = (m_dsField & 0xFC) | ECN_CE;
            return true;
        }
        return false;
    }

    bool GetUint8Value(Uint8Values field, uint8_t& value) const override
    {
        if (field == QueueItem::IP_DSFIELD)
        {
            value = m_dsField;
            return true;
        }
        return false;
    }

  private:
    uint8_t m_dsField;
};

// ---------------------------------------------------------------------------
// Test case
// ---------------------------------------------------------------------------

/**
 * @ingroup traffic-control-test
 * @brief DualPi2 queue disc test case.
 */
class DualPi2QueueDiscTestCase : public TestCase
{
  public:
    DualPi2QueueDiscTestCase();
    void DoRun() override;

  private:
    /**
     * Enqueue @p nPkt packets of @p size bytes with ECN codepoint @p dsField.
     */
    void Enqueue(Ptr<DualPi2QueueDisc> queue, uint32_t size, uint32_t nPkt, uint8_t dsField);

    /**
     * Create a DualPi2QueueDisc with the given MTU already set.
     * MTU must be provided explicitly because unit tests have no NetDevice.
     */
    Ptr<DualPi2QueueDisc> CreateQueue(uint32_t mtu = 1500);

    /**
     * Trace sink for the Classic probability (p_C).
     * @param oldValue previous value (unused)
     * @param newValue updated value
     */
    void TraceProbC(double oldValue, double newValue);

    /**
     * Trace sink for the L4S probability (p_L).
     * @param oldValue previous value (unused)
     * @param newValue updated value
     */
    void TraceProbL(double oldValue, double newValue);

    double m_probC{0.0}; //!< Last observed Classic probability (p_C)
    double m_probL{0.0}; //!< Last observed L4S probability (p_L)
};

void
DualPi2QueueDiscTestCase::TraceProbC(double, double newValue)
{
    m_probC = newValue;
}

void
DualPi2QueueDiscTestCase::TraceProbL(double, double newValue)
{
    m_probL = newValue;
}

DualPi2QueueDiscTestCase::DualPi2QueueDiscTestCase()
    : TestCase("Sanity check on the DualPi2 queue disc implementation")
{
}

void
DualPi2QueueDiscTestCase::Enqueue(Ptr<DualPi2QueueDisc> queue,
                                  uint32_t size,
                                  uint32_t nPkt,
                                  uint8_t dsField)
{
    Address dest;
    for (uint32_t i = 0; i < nPkt; i++)
    {
        queue->Enqueue(Create<DualPi2TestItem>(Create<Packet>(size), dest, dsField));
    }
}

Ptr<DualPi2QueueDisc>
DualPi2QueueDiscTestCase::CreateQueue(uint32_t mtu)
{
    Ptr<DualPi2QueueDisc> queue = CreateObject<DualPi2QueueDisc>();
    NS_ABORT_MSG_IF(!queue->SetAttributeFailSafe("Mtu", UintegerValue(mtu)),
                    "Could not set Mtu attribute");
    return queue;
}

void
DualPi2QueueDiscTestCase::DoRun()
{
    Address dest;

    // -----------------------------------------------------------------------
    // Test 1: Packet classification
    //
    // Not-ECT and ECT(0) must be enqueued into the Classic queue (index 0).
    // ECT(1) must be enqueued into the L4S queue (index 1).
    // -----------------------------------------------------------------------
    {
        Ptr<DualPi2QueueDisc> queue = CreateQueue();
        queue->Initialize();

        queue->Enqueue(Create<DualPi2TestItem>(Create<Packet>(100), dest, ECN_NOT_ECT));
        NS_TEST_ASSERT_MSG_EQ(queue->GetInternalQueue(0)->GetNPackets(),
                              1,
                              "Not-ECT must go to Classic queue");
        NS_TEST_ASSERT_MSG_EQ(queue->GetInternalQueue(1)->GetNPackets(),
                              0,
                              "L4S queue must be empty after Not-ECT");

        queue->Enqueue(Create<DualPi2TestItem>(Create<Packet>(100), dest, ECN_ECT0));
        NS_TEST_ASSERT_MSG_EQ(queue->GetInternalQueue(0)->GetNPackets(),
                              2,
                              "ECT(0) must go to Classic queue");
        NS_TEST_ASSERT_MSG_EQ(queue->GetInternalQueue(1)->GetNPackets(),
                              0,
                              "L4S queue must still be empty after ECT(0)");

        queue->Enqueue(Create<DualPi2TestItem>(Create<Packet>(100), dest, ECN_ECT1));
        NS_TEST_ASSERT_MSG_EQ(queue->GetInternalQueue(1)->GetNPackets(),
                              1,
                              "ECT(1) must go to L4S queue");
        NS_TEST_ASSERT_MSG_EQ(queue->GetInternalQueue(0)->GetNPackets(),
                              2,
                              "Classic queue must be unchanged after ECT(1)");

        Simulator::Destroy();
    }

    // -----------------------------------------------------------------------
    // Test 2: StepTag controls Step AQM CE marking
    //
    // A packet enqueued below MinQLenStep must NOT be CE-marked even if its
    // sojourn time later exceeds L4SMarkThreshold.
    // A packet enqueued at or above MinQLenStep MUST be CE-marked when the
    // sojourn condition is also met.
    // -----------------------------------------------------------------------
    {
        Ptr<DualPi2QueueDisc> queue = CreateQueue();
        NS_TEST_ASSERT_MSG_EQ(queue->SetAttributeFailSafe("MinQLenStep", UintegerValue(2)),
                              true,
                              "Could not set MinQLenStep");
        NS_TEST_ASSERT_MSG_EQ(
            queue->SetAttributeFailSafe("L4SMarkThreshold", TimeValue(MilliSeconds(5))),
            true,
            "Could not set L4SMarkThreshold");
        queue->Initialize();

        // p1: L4S queue empty (0 < 2) -> applyStep = false
        queue->Enqueue(Create<DualPi2TestItem>(Create<Packet>(100), dest, ECN_ECT1));
        // p2, p3: bring queue to 2 packets
        queue->Enqueue(Create<DualPi2TestItem>(Create<Packet>(100), dest, ECN_ECT1));
        queue->Enqueue(Create<DualPi2TestItem>(Create<Packet>(100), dest, ECN_ECT1));
        // p4: L4S queue has 3 packets (>= 2) -> applyStep = true
        queue->Enqueue(Create<DualPi2TestItem>(Create<Packet>(100), dest, ECN_ECT1));

        // Advance time so sojourn exceeds L4SMarkThreshold for all packets
        Simulator::Stop(MilliSeconds(50));
        Simulator::Run();

        // Dequeue p1: applyStep=false -> no mark
        queue->Dequeue();
        NS_TEST_ASSERT_MSG_EQ(
            queue->GetStats().GetNMarkedPackets(DualPi2QueueDisc::PROBABILISTIC_L4S_MARK),
            0,
            "p1 must NOT be CE-marked (applyStep was false)");

        queue->Dequeue(); // p2
        queue->Dequeue(); // p3

        uint64_t marksBefore = queue->GetStats().GetNMarkedPackets(DualPi2QueueDisc::STEP_L4S_MARK);

        // Dequeue p4: applyStep=true and sojourn > threshold -> CE mark
        queue->Dequeue();
        NS_TEST_ASSERT_MSG_EQ(
            queue->GetStats().GetNMarkedPackets(DualPi2QueueDisc::STEP_L4S_MARK),
            marksBefore + 1,
            "p4 must be CE-marked by StepAqm (applyStep=true, sojourn > threshold)");

        Simulator::Destroy();
    }

    // -----------------------------------------------------------------------
    // Test 3: Step AQM - sojourn time threshold
    //
    // StepInPackets=false: packet below sojourn threshold is not marked;
    // packet above sojourn threshold is marked.
    // -----------------------------------------------------------------------
    {
        Ptr<DualPi2QueueDisc> queue = CreateQueue();
        NS_TEST_ASSERT_MSG_EQ(queue->SetAttributeFailSafe("StepInPackets", BooleanValue(false)),
                              true,
                              "Could not set StepInPackets");
        NS_TEST_ASSERT_MSG_EQ(queue->SetAttributeFailSafe("MinQLenStep", UintegerValue(0)),
                              true,
                              "Could not set MinQLenStep");
        NS_TEST_ASSERT_MSG_EQ(
            queue->SetAttributeFailSafe("L4SMarkThreshold", TimeValue(MilliSeconds(5))),
            true,
            "Could not set L4SMarkThreshold");
        queue->Initialize();

        // Enqueue and dequeue immediately: sojourn < threshold: no mark
        queue->Enqueue(Create<DualPi2TestItem>(Create<Packet>(100), dest, ECN_ECT1));
        queue->Dequeue();
        NS_TEST_ASSERT_MSG_EQ(queue->GetStats().GetNMarkedPackets(DualPi2QueueDisc::STEP_L4S_MARK),
                              0,
                              "Below sojourn threshold must NOT be marked");

        // Enqueue and let sojourn exceed threshold
        queue->Enqueue(Create<DualPi2TestItem>(Create<Packet>(100), dest, ECN_ECT1));
        Simulator::Stop(MilliSeconds(50));
        Simulator::Run();
        queue->Dequeue();
        NS_TEST_ASSERT_MSG_EQ(queue->GetStats().GetNMarkedPackets(DualPi2QueueDisc::STEP_L4S_MARK),
                              1,
                              "Above sojourn threshold must be marked");

        Simulator::Destroy();
    }

    // -----------------------------------------------------------------------
    // Test 4: WRR scheduler respects ClassicWeight
    //
    // With ClassicWeight=10, the Classic queue must be served ~10% of the
    // time over 100 dequeue operations.
    // -----------------------------------------------------------------------
    {
        Ptr<DualPi2QueueDisc> queue = CreateQueue(100);
        NS_TEST_ASSERT_MSG_EQ(queue->SetAttributeFailSafe("ClassicWeight", UintegerValue(10)),
                              true,
                              "Could not set ClassicWeight");
        queue->Initialize();

        const uint32_t N = 100;
        for (uint32_t i = 0; i < N; i++)
        {
            queue->Enqueue(Create<DualPi2TestItem>(Create<Packet>(100), dest, ECN_NOT_ECT));
            queue->Enqueue(Create<DualPi2TestItem>(Create<Packet>(100), dest, ECN_ECT1));
        }

        uint32_t classicCount = 0;
        for (uint32_t i = 0; i < N; i++)
        {
            Ptr<QueueDiscItem> item = queue->Dequeue();
            if (item)
            {
                uint8_t ds = 0;
                item->GetUint8Value(QueueItem::IP_DSFIELD, ds);
                if ((ds & 0x3) == ECN_NOT_ECT || (ds & 0x3) == ECN_ECT0)
                {
                    classicCount++;
                }
            }
        }

        double fraction = static_cast<double>(classicCount) / N;
        NS_TEST_ASSERT_MSG_EQ_TOL(fraction, 0.10, 0.01, "Classic fraction must be ~10%");

        Simulator::Destroy();
    }

    // -----------------------------------------------------------------------
    // Test 5: Queue limit triggers FORCED_DROP
    //
    // When the queue is full, the next enqueue must fail and increment
    // FORCED_DROP; the queue size must not change.
    // -----------------------------------------------------------------------
    {
        const uint32_t mtu = 1500;
        const uint32_t limit = 3 * mtu;

        Ptr<DualPi2QueueDisc> queue = CreateQueue(mtu);
        NS_TEST_ASSERT_MSG_EQ(queue->SetAttributeFailSafe("QueueLimit", UintegerValue(limit)),
                              true,
                              "Could not set QueueLimit");
        queue->Initialize();

        Enqueue(queue, mtu, 3, ECN_NOT_ECT);
        uint32_t sizeBefore = queue->GetQueueSize();

        bool ok = queue->Enqueue(Create<DualPi2TestItem>(Create<Packet>(mtu), dest, ECN_NOT_ECT));
        NS_TEST_ASSERT_MSG_EQ(ok, false, "Enqueue beyond limit must fail");
        NS_TEST_ASSERT_MSG_EQ(queue->GetStats().GetNDroppedPackets(DualPi2QueueDisc::FORCED_DROP),
                              1,
                              "FORCED_DROP counter must be incremented");
        NS_TEST_ASSERT_MSG_EQ(queue->GetQueueSize(),
                              sizeBefore,
                              "Queue size must not change after FORCED_DROP");

        Simulator::Destroy();
    }

    // -----------------------------------------------------------------------
    // Test 6: OVERLOAD_DROP for Classic and L4S packets
    //
    // Overload fires when baseProb * k > 1.0.  With DropOverload=true,
    // Classic ECT(0) packets are dropped instead of marked.  L4S ECT(1)
    // packets are dropped when the RNG draw falls within m_pC.
    //
    // To reach overload deterministically: fill only the L4S queue without
    // dequeuing so sojourn time grows, then advance 500 ms so DualPi2Update
    // converges (beta=3.2 adds ~0.051 per 16 ms tick; overload fires when
    // baseProb > 0.5 with k=2, i.e. after ~10 ticks / ~160 ms).
    // -----------------------------------------------------------------------
    {
        // Sub-case A: Classic overload drop
        {
            RngSeedManager::SetSeed(1);
            RngSeedManager::SetRun(1);

            Ptr<DualPi2QueueDisc> queue = CreateQueue();
            queue->AssignStreams(1);
            NS_TEST_ASSERT_MSG_EQ(queue->SetAttributeFailSafe("DropOverload", BooleanValue(true)),
                                  true,
                                  "Could not set DropOverload");
            queue->Initialize();

            Enqueue(queue, 100, 60, ECN_ECT0); // Enqueue Classic packets to build sojourn time and
                                               // trigger overload drops

            Simulator::Stop(MilliSeconds(500));
            Simulator::Run();

            bool gotDrop = false;
            for (int i = 0; i < 10 && !gotDrop; i++)
            {
                uint64_t before =
                    queue->GetStats().GetNDroppedPackets(DualPi2QueueDisc::OVERLOAD_DROP);
                queue->Dequeue();
                uint64_t after =
                    queue->GetStats().GetNDroppedPackets(DualPi2QueueDisc::OVERLOAD_DROP);
                gotDrop = (after > before);
            }

            NS_TEST_ASSERT_MSG_EQ(gotDrop,
                                  true,
                                  "Overload: at least one Classic packet must be OVERLOAD_DROPped");

            Simulator::Destroy();
        }

        // Sub-case B: L4S overload drop
        {
            RngSeedManager::SetSeed(1);
            RngSeedManager::SetRun(1);

            Ptr<DualPi2QueueDisc> queue = CreateQueue();
            queue->AssignStreams(1);
            NS_TEST_ASSERT_MSG_EQ(queue->SetAttributeFailSafe("DropOverload", BooleanValue(true)),
                                  true,
                                  "Could not set DropOverload");
            queue->Initialize();

            Enqueue(queue, 100, 60, ECN_ECT1);

            Simulator::Stop(MilliSeconds(500));
            Simulator::Run();

            bool gotDrop = false;
            for (int i = 0; i < 10 && !gotDrop; i++)
            {
                uint64_t before =
                    queue->GetStats().GetNDroppedPackets(DualPi2QueueDisc::OVERLOAD_DROP);
                queue->Dequeue();
                uint64_t after =
                    queue->GetStats().GetNDroppedPackets(DualPi2QueueDisc::OVERLOAD_DROP);
                gotDrop = (after > before);
            }

            NS_TEST_ASSERT_MSG_EQ(gotDrop,
                                  true,
                                  "Overload: at least one L4S packet must be OVERLOAD_DROPped");

            Simulator::Destroy();
        }
    }

    // -----------------------------------------------------------------------
    // Test 7: DropEarly controls when OVERLOAD_DROP occurs
    //
    // Under overload conditions (baseProb * k > 1.0), MustDrop() returns true.
    // If DropEarly is true, packets are dropped inside DoEnqueue().
    // If DropEarly is false, packets are enqueued successfully and dropped in DoDequeue().
    // -----------------------------------------------------------------------
    {
        // Sub-case A: DropEarly = true (Drop at Enqueue)
        {
            RngSeedManager::SetSeed(1);
            RngSeedManager::SetRun(1);

            Ptr<DualPi2QueueDisc> queue = CreateQueue();
            queue->AssignStreams(1);
            NS_TEST_ASSERT_MSG_EQ(queue->SetAttributeFailSafe("DropEarly", BooleanValue(true)),
                                  true,
                                  "Could not set DropEarly");
            queue->Initialize();

            // Build up delay in the L4S queue to force the overload state
            Enqueue(queue, 100, 50, ECN_ECT1);
            Simulator::Stop(Seconds(1.0));
            Simulator::Run();

            uint32_t sizeBefore = queue->GetQueueSize();
            uint64_t dropsBefore =
                queue->GetStats().GetNDroppedPackets(DualPi2QueueDisc::OVERLOAD_DROP);

            // With DropEarly=true, this Classic packet must be rejected immediately
            bool ok =
                queue->Enqueue(Create<DualPi2TestItem>(Create<Packet>(100), dest, ECN_NOT_ECT));

            NS_TEST_ASSERT_MSG_EQ(ok,
                                  false,
                                  "Enqueue must fail when DropEarly is true during overload");
            NS_TEST_ASSERT_MSG_EQ(queue->GetQueueSize(), sizeBefore, "Queue size must not change");
            NS_TEST_ASSERT_MSG_EQ(
                queue->GetStats().GetNDroppedPackets(DualPi2QueueDisc::OVERLOAD_DROP),
                dropsBefore + 1,
                "OVERLOAD_DROP must increment at enqueue");

            Simulator::Destroy();
        }

        // Sub-case B: DropEarly = false (Drop at Dequeue)
        {
            RngSeedManager::SetSeed(1);
            RngSeedManager::SetRun(1);

            Ptr<DualPi2QueueDisc> queue = CreateQueue();
            queue->AssignStreams(1);
            NS_TEST_ASSERT_MSG_EQ(queue->SetAttributeFailSafe("DropEarly", BooleanValue(false)),
                                  true,
                                  "Could not set DropEarly");
            queue->Initialize();

            // Build up delay in the L4S queue to force the overload state
            Enqueue(queue, 100, 50, ECN_ECT1);
            Simulator::Stop(Seconds(1.0));
            Simulator::Run();

            uint32_t sizeBefore = queue->GetQueueSize();
            uint64_t dropsBefore =
                queue->GetStats().GetNDroppedPackets(DualPi2QueueDisc::OVERLOAD_DROP);

            // With DropEarly=false, this packet must be enqueued successfully despite the overload
            bool ok =
                queue->Enqueue(Create<DualPi2TestItem>(Create<Packet>(100), dest, ECN_NOT_ECT));

            NS_TEST_ASSERT_MSG_EQ(
                ok,
                true,
                "Enqueue must succeed when DropEarly is false even during overload");
            NS_TEST_ASSERT_MSG_EQ(queue->GetQueueSize(),
                                  sizeBefore + 100,
                                  "Queue size must increase by 100 bytes");
            NS_TEST_ASSERT_MSG_EQ(
                queue->GetStats().GetNDroppedPackets(DualPi2QueueDisc::OVERLOAD_DROP),
                dropsBefore,
                "OVERLOAD_DROP must NOT increment at enqueue");

            // The drop must occur now, during the dequeue attempt
            queue->Dequeue();

            NS_TEST_ASSERT_MSG_GT(
                queue->GetStats().GetNDroppedPackets(DualPi2QueueDisc::OVERLOAD_DROP),
                dropsBefore,
                "OVERLOAD_DROP must increment during dequeue");

            Simulator::Destroy();
        }
    }

    // -----------------------------------------------------------------------
    // Test 8: PI2 probability values
    //
    // With no traffic the PI2 controller holds the probability at zero.  When a
    // burst arrives, curQ grows by one Tupdate (16 ms) per update and baseProb
    // ramps deterministically (alpha=0.15625, beta=3.195312, target=15 ms):
    //
    //   t=16 ms: baseProb = 0.0513  (p_L = 0.103)
    //   t=32 ms: baseProb = 0.1051  (p_L = 0.210)
    //   t=48 ms: baseProb = 0.1613  (p_L = 0.323, p_C = 0.026)
    //
    // The run stops just after the third update (t=48 ms) and the model's own
    // p_C/p_L (read from the ProbC/ProbL trace sources) must match those
    // closed-form values.  p_L reaching about 30% confirms the coupling
    // p_L = baseProb * k, and p_C = baseProb^2.
    // -----------------------------------------------------------------------
    {
        const uint32_t pktSize = 100;
        const uint32_t burst = 100;

        // Initialize the trace mirrors: m_probC/m_probL are the test's copies of
        // p_C/p_L, updated only through the ProbC/ProbL callbacks (not by
        // Initialize), so clear them before reading the idle value below.
        m_probC = 0.0;
        m_probL = 0.0;

        Ptr<DualPi2QueueDisc> queue = CreateQueue(pktSize);
        queue->Initialize();
        queue->TraceConnectWithoutContext(
            "ProbC",
            MakeCallback(&DualPi2QueueDiscTestCase::TraceProbC, this));
        queue->TraceConnectWithoutContext(
            "ProbL",
            MakeCallback(&DualPi2QueueDiscTestCase::TraceProbL, this));

        // No traffic yet: the controller starts with zero probability.
        NS_TEST_ASSERT_MSG_EQ(m_probL, 0.0, "Idle: p_L must start at 0");

        // A burst arrives; run just past the third update (t=48 ms).
        Enqueue(queue, pktSize, burst, ECN_ECT1);
        Simulator::Stop(MilliSeconds(50));
        Simulator::Run();

        NS_TEST_ASSERT_MSG_EQ_TOL(m_probL,
                                  0.3227,
                                  0.001,
                                  "p_L must reach ~0.32 (baseProb * k) after three updates");
        NS_TEST_ASSERT_MSG_EQ_TOL(m_probC,
                                  0.0260,
                                  0.001,
                                  "p_C must equal baseProb^2 after three updates");

        Simulator::Destroy();
    }

    // -----------------------------------------------------------------------
    // Test 9: Mark() applies the probability (marking rate matches p_C / p_L)
    //
    // Test 8 checks that the controller computes the right p_C/p_L; this test
    // checks that MustDrop() actually marks packets at those rates.  A backlog
    // run for 100 ms drives baseProb to about 0.35 (p_C ~ 0.12, p_L ~ 0.69),
    // still non-overload.  Time is then frozen so p_C/p_L stay constant, N
    // packets are dequeued, and the measured fraction of marked packets must
    // match the probability the model computed (read from the ProbC/ProbL
    // trace sources).  No OVERLOAD_DROP may occur.
    //
    // The MTU matches the packet size so the "2 * MTU" floor in MustDrop()
    // means "at least 2 packets".  Since MustDrop() runs after the scheduler
    // has removed the current packet, N + 2 packets are enqueued to keep the
    // queue above that floor during all N dequeues.
    // -----------------------------------------------------------------------
    {
        const uint32_t N = 1000;
        const uint32_t pktSize = 100;

        // Sub-case A: Classic marking rate matches p_C
        {
            RngSeedManager::SetSeed(1);
            RngSeedManager::SetRun(1);

            m_probC = 0.0;
            m_probL = 0.0;

            Ptr<DualPi2QueueDisc> queue = CreateQueue(pktSize);
            queue->AssignStreams(1);
            queue->Initialize();
            queue->TraceConnectWithoutContext(
                "ProbC",
                MakeCallback(&DualPi2QueueDiscTestCase::TraceProbC, this));
            queue->TraceConnectWithoutContext(
                "ProbL",
                MakeCallback(&DualPi2QueueDiscTestCase::TraceProbL, this));

            // Ramp baseProb into the non-overload region, then freeze time.
            Enqueue(queue, pktSize, N + 2, ECN_ECT0);
            Simulator::Stop(MilliSeconds(100));
            Simulator::Run();

            NS_TEST_ASSERT_MSG_GT(m_probC, 0.0, "p_C must be positive after the ramp");
            NS_TEST_ASSERT_MSG_LT(m_probL, 1.0, "Must stay in the non-overload region (p_L <= 1)");

            uint64_t before =
                queue->GetStats().GetNMarkedPackets(DualPi2QueueDisc::PROBABILISTIC_CLASSIC_MARK);
            for (uint32_t i = 0; i < N; i++)
            {
                queue->Dequeue();
            }
            uint64_t marks =
                queue->GetStats().GetNMarkedPackets(DualPi2QueueDisc::PROBABILISTIC_CLASSIC_MARK) -
                before;

            double rate = static_cast<double>(marks) / N;
            NS_TEST_ASSERT_MSG_EQ_TOL(rate,
                                      m_probC,
                                      0.03,
                                      "Classic mark rate must match the computed p_C");
            NS_TEST_ASSERT_MSG_EQ(
                queue->GetStats().GetNDroppedPackets(DualPi2QueueDisc::OVERLOAD_DROP),
                0,
                "No overload drops must occur in the non-overload regime");

            Simulator::Destroy();
        }

        // Sub-case B: L4S marking rate matches p_L
        {
            RngSeedManager::SetSeed(1);
            RngSeedManager::SetRun(1);

            m_probC = 0.0;
            m_probL = 0.0;

            Ptr<DualPi2QueueDisc> queue = CreateQueue(pktSize);
            queue->AssignStreams(1);
            queue->Initialize();
            queue->TraceConnectWithoutContext(
                "ProbC",
                MakeCallback(&DualPi2QueueDiscTestCase::TraceProbC, this));
            queue->TraceConnectWithoutContext(
                "ProbL",
                MakeCallback(&DualPi2QueueDiscTestCase::TraceProbL, this));

            // Ramp baseProb into the non-overload region, then freeze time.
            Enqueue(queue, pktSize, N + 2, ECN_ECT1);
            Simulator::Stop(MilliSeconds(100));
            Simulator::Run();

            NS_TEST_ASSERT_MSG_GT(m_probL, 0.0, "p_L must be positive after the ramp");
            NS_TEST_ASSERT_MSG_LT(m_probL, 1.0, "Must stay in the non-overload region (p_L <= 1)");

            uint64_t before =
                queue->GetStats().GetNMarkedPackets(DualPi2QueueDisc::PROBABILISTIC_L4S_MARK);
            for (uint32_t i = 0; i < N; i++)
            {
                queue->Dequeue();
            }
            uint64_t marks =
                queue->GetStats().GetNMarkedPackets(DualPi2QueueDisc::PROBABILISTIC_L4S_MARK) -
                before;

            double rate = static_cast<double>(marks) / N;
            NS_TEST_ASSERT_MSG_EQ_TOL(rate,
                                      m_probL,
                                      0.05,
                                      "L4S mark rate must match the computed p_L");
            NS_TEST_ASSERT_MSG_EQ(
                queue->GetStats().GetNDroppedPackets(DualPi2QueueDisc::OVERLOAD_DROP),
                0,
                "No overload drops must occur in the non-overload regime");

            Simulator::Destroy();
        }
    }
}

// ---------------------------------------------------------------------------
// Test Suite registration
// ---------------------------------------------------------------------------

/**
 * @ingroup traffic-control-test
 * @brief DualPi2 queue disc test suite.
 */
static class DualPi2QueueDiscTestSuite : public TestSuite
{
  public:
    DualPi2QueueDiscTestSuite()
        : TestSuite("dualpi2-queue-disc", Type::UNIT)
    {
        AddTestCase(new DualPi2QueueDiscTestCase(), TestCase::Duration::QUICK);
    }
} g_dualPi2QueueDiscTestSuite;
