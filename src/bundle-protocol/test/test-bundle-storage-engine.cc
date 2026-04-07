/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ns3/bundle-block.h" // Added for PrimaryBlock and PayloadBlock
#include "ns3/bundle-storage-engine.h"
#include "ns3/bundle.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/test.h"

using namespace ns3;

/**
 * @ingroup dtn-test
 * @ingroup tests
 *
 * @brief Unit tests for Bundle Storage
 */
class BundleStorageEngineTestCase : public TestCase
{
  public:
    BundleStorageEngineTestCase();
    ~BundleStorageEngineTestCase() override;
    void DoRun() override;
};

BundleStorageEngineTestCase::BundleStorageEngineTestCase()
    : TestCase("BundleStorageEngineTestCase Implementation")
{
}

BundleStorageEngineTestCase::~BundleStorageEngineTestCase()
{
}

void
BundleStorageEngineTestCase::DoRun()
{
    Ptr<BundleStorageEngine> storageEngine = CreateObject<BundleStorageEngine>();

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

    Ptr<Bundle> b1 = CreateObject<Bundle>();

    Ptr<PrimaryBlock> pb1 = CreateObject<PrimaryBlock>();
    pb1->GetHeader() = pbb;
    b1->AddBlock(pb1);

    Ptr<PayloadBlock> pl1 = CreateObject<PayloadBlock>();
    Ptr<Packet> payload = Create<Packet>(12);
    pl1->SetPayload(payload);
    b1->AddBlock(pl1);

    uint32_t b1Size = b1->GetTotalSize();

    storageEngine->SetTotalSize(b1Size * 2);
    NS_TEST_ASSERT_MSG_EQ(storageEngine->GetTotalSize(),
                          b1Size * 2,
                          "BundleEngine total size not updated");

    uint32_t handle1 = storageEngine->StoreBundle(b1);
    NS_TEST_ASSERT_MSG_EQ(storageEngine->HasBundle(handle1),
                          true,
                          "Bundle 1 not inserted correctly");
    NS_TEST_ASSERT_MSG_EQ(storageEngine->GetCurrentSize(),
                          b1Size,
                          "BundleEngine size not incremented");

    Ptr<Bundle> retrieved = storageEngine->RetrieveBundle(handle1);
    NS_TEST_ASSERT_MSG_EQ(retrieved->GetTotalSize(),
                          b1->GetTotalSize(),
                          "Retrieved bundle does not match stored bundle");

    NS_TEST_ASSERT_MSG_EQ(storageEngine->RetrieveBundle(999),
                          nullptr,
                          "RetrieveBundle should return null for invalid handle");

    Ptr<Bundle> b2 = CreateObject<Bundle>();

    Ptr<PrimaryBlock> pb2 = CreateObject<PrimaryBlock>();
    pb2->GetHeader() = pbb;
    b2->AddBlock(pb2);

    Ptr<PayloadBlock> pl2 = CreateObject<PayloadBlock>();
    pl2->SetPayload(Create<Packet>(12));
    b2->AddBlock(pl2);

    uint32_t handle2 = storageEngine->StoreBundle(b2);
    NS_TEST_ASSERT_MSG_EQ(storageEngine->HasBundle(handle2), true, "Bundle 2 should be inserted");
    NS_TEST_ASSERT_MSG_EQ(storageEngine->GetCurrentSize(),
                          b1Size * 2,
                          "BundleEngine size not updated for second bundle");

    Ptr<Bundle> b3 = CreateObject<Bundle>();

    Ptr<PrimaryBlock> pb3 = CreateObject<PrimaryBlock>();
    pb3->GetHeader() = pbb;
    b3->AddBlock(pb3);

    Ptr<PayloadBlock> pl3 = CreateObject<PayloadBlock>();
    pl3->SetPayload(Create<Packet>(12));
    b3->AddBlock(pl3);

    uint32_t handle3 = storageEngine->StoreBundle(b3);
    NS_TEST_ASSERT_MSG_EQ(handle3, 0, "Bundle 3 should be rejected due to capacity limits");
    NS_TEST_ASSERT_MSG_EQ(storageEngine->GetCurrentSize(),
                          b1Size * 2,
                          "BundleEngine size should not change on rejection");

    std::string destEid = b1->GetDestinationEID();
    std::vector<uint32_t> handles = storageEngine->GetHandlesForDestination(destEid);

    NS_TEST_ASSERT_MSG_EQ(handles.size(), 2, "Should find 2 bundles for the given destination");
    NS_TEST_ASSERT_MSG_EQ(handles[0], handle1, "First handle mismatch");
    NS_TEST_ASSERT_MSG_EQ(handles[1], handle2, "Second handle mismatch");

    std::vector<uint32_t> emptyHandles =
        storageEngine->GetHandlesForDestination("dtn:unknown-node");
    NS_TEST_ASSERT_MSG_EQ(emptyHandles.size(), 0, "Should find 0 bundles for unknown destination");

    std::vector<Ptr<Bundle>> expired = storageEngine->GetExpiredBundles();
    NS_TEST_ASSERT_MSG_EQ(expired.size(), 0, "Bundles should not be expired yet");

    NS_TEST_ASSERT_MSG_EQ(storageEngine->DeleteBundle(handle1), 0, "Bundle 1 failed to delete");
    NS_TEST_ASSERT_MSG_EQ(storageEngine->HasBundle(handle1), false, "Bundle 1 still exists in map");
    NS_TEST_ASSERT_MSG_EQ(storageEngine->GetCurrentSize(),
                          b1Size,
                          "BundleEngine size not decremented correctly");

    NS_TEST_ASSERT_MSG_EQ(storageEngine->DeleteBundle(999),
                          1,
                          "DeleteBundle should return 1 for invalid handle");

    storageEngine->DeleteBundle(handle2);
    NS_TEST_ASSERT_MSG_EQ(storageEngine->GetCurrentSize(),
                          0,
                          "BundleEngine size should be 0 after clearing");
}

/**
 * @ingroup dtn-test
 * @ingroup tests
 *
 * @brief BundleStorageEngineTestCase Test Suite
 */
class BundleStorageEngineTestSuite : public TestSuite
{
  public:
    BundleStorageEngineTestSuite()
        : TestSuite("bundle-storage-engine", Type::UNIT)
    {
        AddTestCase(new BundleStorageEngineTestCase(), TestCase::Duration::QUICK);
    }
};

static BundleStorageEngineTestSuite
    g_bundleStorageEngineTestSuite; //!< Static variable for test initialization
