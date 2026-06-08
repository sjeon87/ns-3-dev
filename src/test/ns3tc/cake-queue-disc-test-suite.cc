/*
 * Copyright (c) 2026 Shivang Upadhyay
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Unit test for CAKE queue disc host isolation (Algorithm 2).
 *
 * Verifies that in cake_dst mode the DRR quantum logic produces
 * the per-destination fairness ratios from Figure 3 of the CAKE paper.
 * The queue disc is exercised in isolation — no network topology,
 * time frozen at zero throughout.
 */

#include "ns3/cake-queue-disc.h"
#include "ns3/config.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-queue-disc-item.h"
#include "ns3/queue-size.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/udp-header.h"
#include "ns3/uinteger.h"

using namespace ns3;

/**
 * @ingroup system-tests-tc
 *
 * This class tests that cake_dst mode assigns DRR quanta so that
 * each destination host gets an equal total share of dequeues.
 */
class CakeQueueDiscDstIsolationRatio : public TestCase
{
  public:
    CakeQueueDiscDstIsolationRatio();
    ~CakeQueueDiscDstIsolationRatio() override;

  private:
    void DoRun() override;

    /**
     * Enqueue one 256-byte UDP packet for the given flow.
     * @param queue  The CAKE queue disc.
     * @param ipHdr  IPv4 header for this flow.
     * @param udpHdr UDP header for this flow.
     */
    void AddPacket(Ptr<CakeQueueDisc> queue, Ipv4Header ipHdr, UdpHeader udpHdr);
};

CakeQueueDiscDstIsolationRatio::CakeQueueDiscDstIsolationRatio()
    : TestCase("Test cake_dst host isolation dequeue ratios (Algorithm 2)")
{
}

CakeQueueDiscDstIsolationRatio::~CakeQueueDiscDstIsolationRatio()
{
}

void
CakeQueueDiscDstIsolationRatio::AddPacket(Ptr<CakeQueueDisc> queue,
                                          Ipv4Header ipHdr,
                                          UdpHeader udpHdr)
{
    // UDP header prepended so the queue hashes on the full 5-tuple
    Ptr<Packet> p = Create<Packet>(256);
    p->AddHeader(udpHdr);
    Address dest;
    Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem>(p, dest, 0, ipHdr);
    queue->Enqueue(item);
}

void
CakeQueueDiscDstIsolationRatio::DoRun()
{
    Ptr<CakeQueueDisc> queueDisc = CreateObjectWithAttributes<CakeQueueDisc>("IsolationMode",
                                                                             UintegerValue(2),
                                                                             "Perturbation",
                                                                             UintegerValue(1));
    queueDisc->Initialize();

    // Six flows reproducing the Figure 3 host topology.
    struct FlowDef
    {
        const char* srcIp;
        const char* dstIp;
        uint16_t sport;
        uint16_t dport;
        const char* label;
    };

    FlowDef flows[] = {
        {"10.0.0.1", "10.0.1.1", 1001, 2001, "A->A"},
        {"10.0.0.1", "10.0.1.2", 1001, 2002, "A->B"},
        {"10.0.0.1", "10.0.1.3", 1001, 2003, "A->C1"},
        {"10.0.0.1", "10.0.1.3", 1002, 2003, "A->C2"},
        {"10.0.0.2", "10.0.1.3", 1003, 2003, "B->C"},
        {"10.0.0.2", "10.0.1.4", 1003, 2004, "B->D"},
    };

    const uint32_t NUM_FLOWS = 6;
    const uint32_t PACKETS_PER_FLOW = 1500;

    Ipv4Header ipHdrs[NUM_FLOWS];
    UdpHeader udpHdrs[NUM_FLOWS];

    for (uint32_t i = 0; i < NUM_FLOWS; i++)
    {
        ipHdrs[i].SetSource(Ipv4Address(flows[i].srcIp));
        ipHdrs[i].SetDestination(Ipv4Address(flows[i].dstIp));
        ipHdrs[i].SetProtocol(17); // UDP
        ipHdrs[i].SetPayloadSize(256 + 8);

        udpHdrs[i].SetSourcePort(flows[i].sport);
        udpHdrs[i].SetDestinationPort(flows[i].dport);
    }

    // Enqueue packets interleaved
    for (uint32_t pkt = 0; pkt < PACKETS_PER_FLOW; pkt++)
    {
        for (uint32_t f = 0; f < NUM_FLOWS; f++)
        {
            AddPacket(queueDisc, ipHdrs[f], udpHdrs[f]);
        }
    }

    NS_TEST_ASSERT_MSG_EQ(queueDisc->GetNPackets(),
                          NUM_FLOWS * PACKETS_PER_FLOW,
                          "unexpected number of packets in the queue disc");

    // Discard the first 600 dequeues (transient state)
    const uint32_t WARMUP = 600;
    for (uint32_t i = 0; i < WARMUP; i++)
    {
        Ptr<QueueDiscItem> pkt = queueDisc->Dequeue();
        NS_TEST_ASSERT_MSG_NE(pkt,
                              nullptr,
                              "queue unexpectedly empty during warm-up at iteration " << i);
        if (pkt == nullptr)
        {
            Simulator::Destroy();
            return;
        }
    }

    // Measure exactly the next 3,000 dequeues
    const uint32_t MEASURE = 3000;
    uint32_t counts[NUM_FLOWS] = {0};

    for (uint32_t i = 0; i < MEASURE; i++)
    {
        Ptr<QueueDiscItem> item = queueDisc->Dequeue();
        NS_TEST_ASSERT_MSG_NE(item,
                              nullptr,
                              "queue unexpectedly empty during measurement at iteration " << i);
        if (item == nullptr)
        {
            break;
        }

        Ptr<Ipv4QueueDiscItem> ip4item = DynamicCast<Ipv4QueueDiscItem>(item);
        NS_TEST_ASSERT_MSG_NE(ip4item, nullptr, "dequeued item is not an Ipv4QueueDiscItem");
        if (ip4item == nullptr)
        {
            continue;
        }

        Ipv4Header ipHdr = ip4item->GetHeader();
        Ipv4Address src = ipHdr.GetSource();
        Ipv4Address dst = ipHdr.GetDestination();

        UdpHeader udpHdr;
        ip4item->GetPacket()->PeekHeader(udpHdr);
        uint16_t sport = udpHdr.GetSourcePort();

        bool matched = false;
        for (uint32_t f = 0; f < NUM_FLOWS; f++)
        {
            if (src == Ipv4Address(flows[f].srcIp) && dst == Ipv4Address(flows[f].dstIp) &&
                sport == flows[f].sport)
            {
                counts[f]++;
                matched = true;
                break;
            }
        }
        NS_TEST_ASSERT_MSG_EQ(matched, true, "dequeued packet did not match any known flow");
    }

    // Expected steady-state 3:3:1:1:1:3 ratio tracking
    const double TOLERANCE = 0.20;
    double expected_single = MEASURE * 3.0 / 12.0; // 750 packets
    double expected_triple = MEASURE * 1.0 / 12.0; // 250 packets

    for (uint32_t f = 0; f < NUM_FLOWS; f++)
    {
        double expected = (f == 2 || f == 3 || f == 4) ? expected_triple : expected_single;
        double lo = expected * (1.0 - TOLERANCE);
        double hi = expected * (1.0 + TOLERANCE);

        NS_TEST_ASSERT_MSG_GT(static_cast<double>(counts[f]),
                              lo,
                              "Flow " + std::string(flows[f].label) +
                                  " dequeue count below expected range");
        NS_TEST_ASSERT_MSG_LT(static_cast<double>(counts[f]),
                              hi,
                              "Flow " + std::string(flows[f].label) +
                                  " dequeue count above expected range");
    }

    Simulator::Destroy();
}

/**
 * @ingroup system-tests-tc
 *
 * CAKE queue disc test suite.
 */
class CakeQueueDiscTestSuite : public TestSuite
{
  public:
    CakeQueueDiscTestSuite();
};

CakeQueueDiscTestSuite::CakeQueueDiscTestSuite()
    : TestSuite("cake-queue-disc", Type::UNIT)
{
    AddTestCase(new CakeQueueDiscDstIsolationRatio, TestCase::Duration::QUICK);
}

/// Instance Allocation
static CakeQueueDiscTestSuite g_cakeQueueDiscTestSuite;
