/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ns3/test.h"
#include "ns3/packet.h"
#include "ns3/nstime.h"

#include "ns3/bundle.h"

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

    // Testing header attachment

    PrimaryBlockHeader pbb;
    pbb.SetVersion(6);
    uint32_t procFlags = (1 << PBB_PROC_FLAGS::NO_FRAGMENT) | 
                         (1 << PBB_PROC_FLAGS::SINGLETON);
    pbb.SetProcFlags(procFlags);
    pbb.SetCreationTime(Seconds(10));
    pbb.SetTTL(Seconds(3600));
    pbb.SetSequenceNumber(42);
    pbb.SetDestinationEID("dtn", "node1");
    pbb.SetSourceEID("dtn", "node0");
    pbb.SetReportToEID("dtn", "none");
    pbb.SetCustodianEID("dtn", "none");

    Bundle b;
    b.SetPrimaryHeader(pbb);

    NS_TEST_ASSERT_MSG_EQ(b.GetPrimaryHeader().GetVersion(), 6, "PrimaryBlockHeader Version mismatch");
    NS_TEST_ASSERT_MSG_EQ(b.GetPrimaryHeader().GetProcFlags(), procFlags, "PrimaryBlockHeader ProcFlags mismatch");
    NS_TEST_ASSERT_MSG_EQ(b.GetPrimaryHeader().GetCreationTime(), Seconds(10), "PrimaryBlockHeader CreationTime mismatch");
    NS_TEST_ASSERT_MSG_EQ(b.GetPrimaryHeader().GetTTL(), Seconds(3600), "PrimaryBlockHeader TTL mismatch");
    NS_TEST_ASSERT_MSG_EQ(b.GetPrimaryHeader().GetSequenceNumber(), 42, "PrimaryBlockHeader SequenceNumber mismatch");

    // Testing payload attachment

    Ptr<Packet> payload = Create<Packet>(12);
    Ptr<Bundle> bundle = CreateObject<Bundle>();
    bundle->SetPayload(payload);
    Ptr<Packet> deserializedPayload = bundle->GetPayload();

    NS_TEST_ASSERT_MSG_EQ(payload->GetSize(), deserializedPayload->GetSize(), "Payload mismatch");
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