/*
 * Copyright (c) 2026 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Ayush Nigam <ash12521198@gmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */

#include "ns3/reorder-queue.h"
#include "ns3/string.h"
#include "ns3/test.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ReorderQueueTest");

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * ReorderQueue unit tests.
 */
class ReorderQueueTestCase : public TestCase
{
  public:
    ReorderQueueTestCase();
    void DoRun() override;
};

ReorderQueueTestCase::ReorderQueueTestCase()
    : TestCase("Sanity check on the drop tail queue implementation")
{
}

void
ReorderQueueTestCase::DoRun()
{
    Ptr<ReorderQueue<Packet>> queue = CreateObject<ReorderQueue<Packet>>();

    /* Setting ReorderDepth to 3 intentionally retains one packet in the queue. The next three
     * packets are dequeued first, after which the retained packet is released to induce controlled
     * reordering.
     */
    queue->SetAttribute("ReorderDepth", UintegerValue(3));

    Ptr<Packet> p1;
    Ptr<Packet> p2;
    Ptr<Packet> p3;
    Ptr<Packet> p4;
    Ptr<Packet> p5;
    Ptr<Packet> p6;
    Ptr<Packet> p7;
    Ptr<Packet> p8;
    p1 = Create<Packet>();
    p2 = Create<Packet>();
    p3 = Create<Packet>();
    p4 = Create<Packet>();
    p5 = Create<Packet>();
    p6 = Create<Packet>();
    p7 = Create<Packet>();
    p8 = Create<Packet>();

    // Enqueue 8 packets
    NS_TEST_EXPECT_MSG_EQ(queue->GetSize(), 0, "There should be no packets in there");
    queue->Enqueue(p1);
    NS_TEST_EXPECT_MSG_EQ(queue->GetSize(), 1, "There should be one packet in there");
    queue->Enqueue(p2);
    NS_TEST_EXPECT_MSG_EQ(queue->GetSize(), 2, "There should be two packets in there");
    queue->Enqueue(p3);
    NS_TEST_EXPECT_MSG_EQ(queue->GetSize(), 3, "There should be three packets in there");
    queue->Enqueue(p4);
    NS_TEST_EXPECT_MSG_EQ(queue->GetSize(), 4, "There should be four packets in there");
    queue->Enqueue(p5);
    NS_TEST_EXPECT_MSG_EQ(queue->GetSize(), 5, "There should be five packet in there");
    queue->Enqueue(p6);
    NS_TEST_EXPECT_MSG_EQ(queue->GetSize(), 6, "There should be six packets in there");
    queue->Enqueue(p7);
    NS_TEST_EXPECT_MSG_EQ(queue->GetSize(), 7, "There should be seven packets in there");
    queue->Enqueue(p8);
    NS_TEST_EXPECT_MSG_EQ(queue->GetSize(), 8, "There should be eight packets in there");

    Ptr<Packet> packet;

    queue->Dequeue();
    queue->Dequeue();
    packet = queue->Dequeue();
    NS_TEST_EXPECT_MSG_EQ(packet->GetUid(), p3->GetUid(), "was this the third packet ?");

    /* Since InSequenceLength is set to 3 packets, one packet is retained after every three
     * in-sequence dequeues to introduce controlled reordering. On the fourth Dequeue() call, the
     * fifth packet is dequeued because the fourth packet has been intentionally retained.
     */
    packet = queue->Dequeue();
    NS_TEST_EXPECT_MSG_EQ(packet->GetUid(), p5->GetUid(), "Was this the fifth packet ?");

    queue->Dequeue();
    queue->Dequeue();
    packet = queue->Dequeue();

    /* The fourth packet is intentionally held back for three dequeue operations. Hence, it will be
     * returned on the 7th call to Dequeue().
     */
    NS_TEST_EXPECT_MSG_EQ(packet->GetUid(), p4->GetUid(), "Was this the fourth packet ?");

    packet = queue->Dequeue();
    NS_TEST_EXPECT_MSG_EQ(packet->GetUid(), p8->GetUid(), "Was this the eighth packet ?");
}

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * @brief Reorder Queue TestSuite
 */
class ReorderQueueTestSuite : public TestSuite
{
  public:
    ReorderQueueTestSuite()
        : TestSuite("reorder-queue", Type::UNIT)
    {
        AddTestCase(new ReorderQueueTestCase(), TestCase::Duration::QUICK);
    }
};

static ReorderQueueTestSuite g_reorderQueueTestSuite; //!< Static variable for test initialization
