/*
 * Copyright (c) 2008 INRIA
 *                  2013 University of New Brunswick
 *                  2014 Universitat Autònoma de Barcelona
 *                  2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *           Dizhi Zhou <dizhi.zhou@gmail.com>
 *           Gerard Garcia <ggarcia@deic.uab.cat>
 *           Rubén Martínez <rmartinez@deic.uab.cat>
 *           Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ns3/bundle-block.h"
#include "ns3/bundle-protocol-flags.h"
#include "ns3/bundle.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/test.h"

using namespace ns3;

/**
 * @ingroup dtn-test
 * @ingroup tests
 *
 * @brief Unit tests for Bundle Implementation
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
    Ptr<PrimaryBlock> pb = CreateObject<PrimaryBlock>();
    PrimaryBlockHeader& pbb = pb->GetHeader();

    pbb.SetVersion(7);
    uint32_t procFlags = (1 << PBB_PROC_FLAGS::NO_FRAGMENT) | (1 << PBB_PROC_FLAGS::REQ_APP_ACK);
    pbb.SetProcFlags(procFlags);
    pbb.SetCrcType(1);
    pbb.SetCreationTime(Seconds(10));
    pbb.SetLifetime(Seconds(3600));
    pbb.SetSequenceNumber(42);
    pbb.SetDestinationEID("dtn:node1");
    pbb.SetSourceEID("dtn:node0");
    pbb.SetReportToEID("dtn:none");

    Ptr<Bundle> b = CreateObject<Bundle>();
    b->AddBlock(pb);

    NS_TEST_ASSERT_MSG_EQ(b->GetPrimaryBlock()->GetHeader().GetVersion(),
                          7,
                          "PrimaryBlockHeader Version mismatch");
    NS_TEST_ASSERT_MSG_EQ(b->GetPrimaryBlock()->GetHeader().GetProcFlags(),
                          procFlags,
                          "PrimaryBlockHeader ProcFlags mismatch");
    NS_TEST_ASSERT_MSG_EQ(b->GetPrimaryBlock()->GetHeader().GetCreationTime(),
                          Seconds(10),
                          "PrimaryBlockHeader CreationTime mismatch");
    NS_TEST_ASSERT_MSG_EQ(b->GetPrimaryBlock()->GetHeader().GetLifetime(),
                          Seconds(3600),
                          "PrimaryBlockHeader Lifetime mismatch");
    NS_TEST_ASSERT_MSG_EQ(b->GetPrimaryBlock()->GetHeader().GetSequenceNumber(),
                          42,
                          "PrimaryBlockHeader SequenceNumber mismatch");
    NS_TEST_ASSERT_MSG_EQ(b->GetDestinationEID(),
                          "dtn:node1",
                          "Bundle Destination EID mapping mismatch");

    Ptr<Packet> payload = Create<Packet>(12);
    Ptr<Bundle> bundle = CreateObject<Bundle>();

    Ptr<PayloadBlock> payloadBlock = CreateObject<PayloadBlock>();

    payloadBlock->GetHeader().SetBlockNumber(2);
    payloadBlock->GetHeader().SetCrcType(1);

    payloadBlock->SetPayload(payload);
    bundle->AddBlock(payloadBlock);

    Ptr<Packet> retrievedPayload = bundle->GetPayloadBlock()->GetPayload();

    NS_TEST_ASSERT_MSG_EQ(payload->GetSize(), retrievedPayload->GetSize(), "Payload mismatch");
    NS_TEST_ASSERT_MSG_EQ(bundle->GetPayloadBlock()->GetHeader().GetBlockNumber(),
                          2,
                          "Payload Block Number mismatch");
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
