/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ns3/bundle-block.h"
#include "ns3/bundle.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/test.h"

using namespace ns3;

/**
 * @ingroup dtn-test
 * @ingroup tests
 *
 * @brief Unit tests for Bundle Protocol Headers
 */
class BundleTestCase : public TestCase
{
  public:
    BundleTestCase();
    ~BundleTestCase() override;
    void DoRun() override;
};

BundleTestCase::BundleTestCase()
    : TestCase("Bundle Implementation")
{
}

BundleTestCase::~BundleTestCase()
{
}

void
BundleTestCase::DoRun()
{
    // Testing primary block and header attachment
    Ptr<PrimaryBlock> pb = CreateObject<PrimaryBlock>();
    PrimaryBlockHeader& pbb = pb->GetHeader();

    pbb.SetVersion(6);
    uint32_t procFlags = (1 << PBB_PROC_FLAGS::NO_FRAGMENT) | (1 << PBB_PROC_FLAGS::SINGLETON);
    pbb.SetProcFlags(procFlags);
    pbb.SetCreationTime(Seconds(10));
    pbb.SetTTL(Seconds(3600));
    pbb.SetSequenceNumber(42);
    pbb.SetDestinationEID("dtn", "node1");
    pbb.SetSourceEID("dtn", "node0");
    pbb.SetReportToEID("dtn", "none");
    pbb.SetCustodianEID("dtn", "none");

    Ptr<Bundle> b = CreateObject<Bundle>();
    b->AddBlock(pb);

    NS_TEST_ASSERT_MSG_EQ(b->GetPrimaryBlock()->GetHeader().GetVersion(),
                          6,
                          "PrimaryBlockHeader Version mismatch");
    NS_TEST_ASSERT_MSG_EQ(b->GetPrimaryBlock()->GetHeader().GetProcFlags(),
                          procFlags,
                          "PrimaryBlockHeader ProcFlags mismatch");
    NS_TEST_ASSERT_MSG_EQ(b->GetPrimaryBlock()->GetHeader().GetCreationTime(),
                          Seconds(10),
                          "PrimaryBlockHeader CreationTime mismatch");
    NS_TEST_ASSERT_MSG_EQ(b->GetPrimaryBlock()->GetHeader().GetTTL(),
                          Seconds(3600),
                          "PrimaryBlockHeader TTL mismatch");
    NS_TEST_ASSERT_MSG_EQ(b->GetPrimaryBlock()->GetHeader().GetSequenceNumber(),
                          42,
                          "PrimaryBlockHeader SequenceNumber mismatch");

    // Testing payload attachment

    Ptr<Packet> payload = Create<Packet>(12);
    Ptr<Bundle> bundle = CreateObject<Bundle>();

    Ptr<PayloadBlock> payloadBlock = CreateObject<PayloadBlock>();
    payloadBlock->SetPayload(payload);
    bundle->AddBlock(payloadBlock);

    Ptr<Packet> retrievedPayload = bundle->GetPayloadBlock()->GetPayload();

    NS_TEST_ASSERT_MSG_EQ(payload->GetSize(), retrievedPayload->GetSize(), "Payload mismatch");
}

/**
 * @ingroup dtn-test
 * @ingroup tests
 *
 * @brief Bundle Test Suite
 */
class BundleTestSuite : public TestSuite
{
  public:
    BundleTestSuite()
        : TestSuite("bundle", Type::UNIT)
    {
        AddTestCase(new BundleTestCase(), TestCase::Duration::QUICK);
    }
};

static BundleTestSuite g_bundleTestSuite; //!< Static variable for test initialization
