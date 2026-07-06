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
    Ptr<Packet> p1 = Create<Packet>();
    p1->AddHeader(pbb);

    PrimaryBlockHeader pbb2;
    uint32_t bytes1 = p1->RemoveHeader(pbb2);

    NS_TEST_ASSERT_MSG_EQ(bytes1,
                          pbb.GetSerializedSize(),
                          "PrimaryBlockHeader serialized size mismatch");
    NS_TEST_ASSERT_MSG_EQ(pbb2.GetVersion(), 7, "PrimaryBlockHeader Version mismatch");
    NS_TEST_ASSERT_MSG_EQ(pbb2.GetProcFlags(), procFlags, "PrimaryBlockHeader ProcFlags mismatch");
    NS_TEST_ASSERT_MSG_EQ(pbb2.GetCrcType(), 1, "PrimaryBlockHeader CrcType mismatch");
    NS_TEST_ASSERT_MSG_EQ(pbb2.GetCreationTime(),
                          Seconds(10),
                          "PrimaryBlockHeader CreationTime mismatch");
    NS_TEST_ASSERT_MSG_EQ(pbb2.GetLifetime(),
                          Seconds(3600),
                          "PrimaryBlockHeader Lifetime mismatch");
    NS_TEST_ASSERT_MSG_EQ(pbb2.GetSequenceNumber(),
                          42,
                          "PrimaryBlockHeader SequenceNumber mismatch");
    NS_TEST_ASSERT_MSG_EQ(pbb2.GetDestinationEID(),
                          "dtn:node1",
                          "PrimaryBlockHeader Destination EID mismatch");
    NS_TEST_ASSERT_MSG_EQ(pbb2.GetSourceEID(),
                          "dtn:node0",
                          "PrimaryBlockHeader Source EID mismatch");
    NS_TEST_ASSERT_MSG_EQ(pbb2.GetReportToEID(),
                          "dtn:none",
                          "PrimaryBlockHeader ReportTo EID mismatch");

    PayloadBlockHeader plb;
    plb.SetBlockType(1);
    plb.SetBlockNumber(2); // New for BPv7
    plb.SetProcFlags(0x02);
    plb.SetCrcType(1); // New for BPv7
    plb.SetBlockLength(2048);

    Ptr<Packet> p2 = Create<Packet>();
    p2->AddHeader(plb);

    PayloadBlockHeader plb2;
    uint32_t bytes2 = p2->RemoveHeader(plb2);

    NS_TEST_ASSERT_MSG_EQ(bytes2,
                          plb.GetSerializedSize(),
                          "PayloadBlockHeader serialized size mismatch");
    NS_TEST_ASSERT_MSG_EQ(plb2.GetBlockType(), 1, "PayloadBlockHeader BlockType mismatch");
    NS_TEST_ASSERT_MSG_EQ(plb2.GetBlockNumber(), 2, "PayloadBlockHeader BlockNumber mismatch");
    NS_TEST_ASSERT_MSG_EQ(plb2.GetProcFlags(), 0x02, "PayloadBlockHeader ProcFlags mismatch");
    NS_TEST_ASSERT_MSG_EQ(plb2.GetCrcType(), 1, "PayloadBlockHeader CrcType mismatch");
    NS_TEST_ASSERT_MSG_EQ(plb2.GetBlockLength(), 2048, "PayloadBlockHeader BlockLength mismatch");

    BundleStatusReport bsr;
    bsr.SetStatusFlags(0x05);
    bsr.SetReasonCode(0x01);
    bsr.SetSourceEID("dtn:node0");
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
    NS_TEST_ASSERT_MSG_EQ(bsr2.GetSourceEID(),
                          "dtn:node0",
                          "BundleStatusReport SourceEID mismatch");
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
 * @brief Unit tests for the PrimaryBlockHeader's fragment fields (RFC 9171, Section 5.9)
 */
class PrimaryBlockFragmentHeaderTestCase : public TestCase
{
  public:
    PrimaryBlockFragmentHeaderTestCase();
    ~PrimaryBlockFragmentHeaderTestCase() override;
    void DoRun() override;
};

PrimaryBlockFragmentHeaderTestCase::PrimaryBlockFragmentHeaderTestCase()
    : TestCase("PrimaryBlockHeader Fragment Field Serialization")
{
}

PrimaryBlockFragmentHeaderTestCase::~PrimaryBlockFragmentHeaderTestCase()
{
}

void
PrimaryBlockFragmentHeaderTestCase::DoRun()
{
    PrimaryBlockHeader frag;
    frag.SetVersion(7);
    frag.SetProcFlags(1 << PBB_PROC_FLAGS::IS_FRG);
    frag.SetCrcType(1);
    frag.SetCreationTime(Seconds(10));
    frag.SetLifetime(Seconds(3600));
    frag.SetSequenceNumber(7);
    frag.SetDestinationEID("dtn:node1");
    frag.SetSourceEID("dtn:node0");
    frag.SetReportToEID("dtn:none");
    frag.SetFragmentOffset(512);
    frag.SetTotalAppDataLength(2048);

    Ptr<Packet> p1 = Create<Packet>();
    p1->AddHeader(frag);

    PrimaryBlockHeader frag2;
    uint32_t bytes1 = p1->RemoveHeader(frag2);

    NS_TEST_ASSERT_MSG_EQ(bytes1,
                          frag.GetSerializedSize(),
                          "Fragment PrimaryBlockHeader serialized size mismatch");
    NS_TEST_ASSERT_MSG_EQ(frag2.GetFragmentOffset(), 512, "Fragment offset mismatch");
    NS_TEST_ASSERT_MSG_EQ(frag2.GetTotalAppDataLength(), 2048, "Total ADU length mismatch");
    NS_TEST_ASSERT_MSG_EQ(frag2.GetProcFlags() & (1 << PBB_PROC_FLAGS::IS_FRG),
                          static_cast<uint32_t>(1 << PBB_PROC_FLAGS::IS_FRG),
                          "IS_FRG flag not preserved");

    PrimaryBlockHeader nonFrag;
    nonFrag.SetVersion(7);
    nonFrag.SetProcFlags(0);
    nonFrag.SetCrcType(1);
    nonFrag.SetCreationTime(Seconds(10));
    nonFrag.SetLifetime(Seconds(3600));
    nonFrag.SetSequenceNumber(7);
    nonFrag.SetDestinationEID("dtn:node1");
    nonFrag.SetSourceEID("dtn:node0");
    nonFrag.SetReportToEID("dtn:none");
    nonFrag.SetFragmentOffset(999); // ignore this
    nonFrag.SetTotalAppDataLength(999);

    Ptr<Packet> p2 = Create<Packet>();
    p2->AddHeader(nonFrag);

    PrimaryBlockHeader nonFrag2;
    uint32_t bytes2 = p2->RemoveHeader(nonFrag2);

    NS_TEST_ASSERT_MSG_EQ(bytes2,
                          nonFrag.GetSerializedSize(),
                          "Non-fragment PrimaryBlockHeader serialized size mismatch");
    NS_TEST_ASSERT_MSG_EQ(nonFrag2.GetFragmentOffset(),
                          0,
                          "Fragment offset must be zero when IS_FRG is unset");
    NS_TEST_ASSERT_MSG_EQ(nonFrag2.GetTotalAppDataLength(),
                          0,
                          "Total ADU length must be zero when IS_FRG is unset");
    NS_TEST_ASSERT_MSG_LT(bytes2, bytes1, "Non-fragment header should be smaller on the wire");
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
        AddTestCase(new PrimaryBlockFragmentHeaderTestCase(), TestCase::Duration::QUICK);
    }
};

static BundleHeaderTestSuite g_bundleHeaderTestSuite; //!< Static variable for test initialization
