/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ns3/bundle-header.h"
#include "ns3/bundle-protocol-flags.h"
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
class BundleHeaderTestCase : public TestCase
{
  public:
    BundleHeaderTestCase();
    ~BundleHeaderTestCase() override;
    void DoRun() override;
};

BundleHeaderTestCase::BundleHeaderTestCase()
    : TestCase("Bundle Header Serialization and Deserialization")
{
}

BundleHeaderTestCase::~BundleHeaderTestCase()
{
}

void
BundleHeaderTestCase::DoRun()
{
    PrimaryBlockHeader pbb;
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

    Ptr<Packet> p1 = Create<Packet>();
    p1->AddHeader(pbb);

    PrimaryBlockHeader pbb2;
    uint32_t bytes1 = p1->RemoveHeader(pbb2);

    NS_TEST_ASSERT_MSG_EQ(bytes1,
                          pbb.GetSerializedSize(),
                          "PrimaryBlockHeader serialized size mismatch");
    NS_TEST_ASSERT_MSG_EQ(pbb2.GetVersion(), 6, "PrimaryBlockHeader Version mismatch");
    NS_TEST_ASSERT_MSG_EQ(pbb2.GetProcFlags(), procFlags, "PrimaryBlockHeader ProcFlags mismatch");
    NS_TEST_ASSERT_MSG_EQ(pbb2.GetCreationTime(),
                          Seconds(10),
                          "PrimaryBlockHeader CreationTime mismatch");
    NS_TEST_ASSERT_MSG_EQ(pbb2.GetTTL(), Seconds(3600), "PrimaryBlockHeader TTL mismatch");
    NS_TEST_ASSERT_MSG_EQ(pbb2.GetSequenceNumber(),
                          42,
                          "PrimaryBlockHeader SequenceNumber mismatch");
    NS_TEST_ASSERT_MSG_EQ(pbb2.GetDictionary(),
                          pbb.GetDictionary(),
                          "PrimaryBlockHeader Dictionary mismatch");

    PayloadBlockHeader plb;
    plb.SetBlockType(1);
    plb.SetProcFlags(0x02);
    plb.SetBlockLength(2048);

    Ptr<Packet> p2 = Create<Packet>();
    p2->AddHeader(plb);

    PayloadBlockHeader plb2;
    uint32_t bytes2 = p2->RemoveHeader(plb2);

    NS_TEST_ASSERT_MSG_EQ(bytes2,
                          plb.GetSerializedSize(),
                          "PayloadBlockHeader serialized size mismatch");
    NS_TEST_ASSERT_MSG_EQ(plb2.GetBlockType(), 1, "PayloadBlockHeader BlockType mismatch");
    NS_TEST_ASSERT_MSG_EQ(plb2.GetProcFlags(), 0x02, "PayloadBlockHeader ProcFlags mismatch");
    NS_TEST_ASSERT_MSG_EQ(plb2.GetBlockLength(), 2048, "PayloadBlockHeader BlockLength mismatch");

    BundleStatusReport bsr;
    bsr.SetStatusFlags(0x05);
    bsr.SetReasonCode(0x01);
    bsr.SetFragmentOffset(0);
    bsr.SetBundleReceiptTime(Seconds(15));
    bsr.SetCreationTime(Seconds(5));
    bsr.SetSequenceNumber(100);

    Ptr<Packet> p3 = Create<Packet>();
    p3->AddHeader(bsr);

    BundleStatusReport bsr2;
    uint32_t bytes3 = p3->RemoveHeader(bsr2);

    NS_TEST_ASSERT_MSG_EQ(bytes3,
                          bsr.GetSerializedSize(),
                          "BundleStatusReport serialized size mismatch");
    NS_TEST_ASSERT_MSG_EQ(bsr2.GetStatusFlags(), 0x05, "BundleStatusReport StatusFlags mismatch");
    NS_TEST_ASSERT_MSG_EQ(bsr2.GetReasonCode(), 0x01, "BundleStatusReport ReasonCode mismatch");
    NS_TEST_ASSERT_MSG_EQ(bsr2.GetBundleReceiptTime(),
                          Seconds(15),
                          "BundleStatusReport ReceiptTime mismatch");
    NS_TEST_ASSERT_MSG_EQ(bsr2.GetCreationTime(),
                          Seconds(5),
                          "BundleStatusReport CreationTime mismatch");
    NS_TEST_ASSERT_MSG_EQ(bsr2.GetSequenceNumber(),
                          100,
                          "BundleStatusReport SequenceNumber mismatch");
}

/**
 * @ingroup dtn-test
 * @ingroup tests
 *
 * @brief Bundle Header Test Suite
 */
class BundleHeaderTestSuite : public TestSuite
{
  public:
    BundleHeaderTestSuite()
        : TestSuite("bundle-header", Type::UNIT)
    {
        AddTestCase(new BundleHeaderTestCase(), TestCase::Duration::QUICK);
    }
};

static BundleHeaderTestSuite g_bundleHeaderTestSuite; //!< Static variable for test initialization
