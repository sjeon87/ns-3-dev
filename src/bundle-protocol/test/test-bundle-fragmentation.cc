/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ns3/base-routing-engine.h"
#include "ns3/bundle-agent.h"
#include "ns3/bundle-protocol-flags.h"
#include "ns3/bundle.h"
#include "ns3/generic-convergence-layer-adapter.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/uinteger.h"

using namespace ns3;

/**
 * @ingroup dtn-test
 * @ingroup tests
 *
 * @brief Unit tests for Bundle::Fragment
 */
class BundleFragmentTestCase : public TestCase
{
  public:
    BundleFragmentTestCase();
    ~BundleFragmentTestCase() override;
    void DoRun() override;
};

BundleFragmentTestCase::BundleFragmentTestCase()
    : TestCase("Bundle::Fragment splits payload into fragment bundles")
{
}

BundleFragmentTestCase::~BundleFragmentTestCase()
{
}

void
BundleFragmentTestCase::DoRun()
{
    const uint32_t payloadSize = 250;
    const uint32_t maxFragmentSize = 100;

    uint8_t data[payloadSize];
    for (uint32_t i = 0; i < payloadSize; ++i)
    {
        data[i] = static_cast<uint8_t>(i % 256);
    }

    PrimaryBlockHeader primaryHeader;
    primaryHeader.SetVersion(7);
    primaryHeader.SetCrcType(1);
    primaryHeader.SetCreationTime(Seconds(10));
    primaryHeader.SetLifetime(Seconds(3600));
    primaryHeader.SetSequenceNumber(1);
    primaryHeader.SetDestinationEID("dtn:node1");
    primaryHeader.SetSourceEID("dtn:node0");
    primaryHeader.SetReportToEID("dtn:none");

    Ptr<PrimaryBlock> primaryBlock = CreateObject<PrimaryBlock>();
    primaryBlock->GetHeader() = primaryHeader;

    PayloadBlockHeader payloadHeader;
    payloadHeader.SetBlockType(1);
    payloadHeader.SetBlockNumber(1);
    payloadHeader.SetBlockLength(payloadSize);

    Ptr<PayloadBlock> payloadBlock = CreateObject<PayloadBlock>();
    payloadBlock->GetHeader() = payloadHeader;
    payloadBlock->SetPayload(Create<Packet>(data, payloadSize));

    Ptr<Bundle> original = CreateObject<Bundle>();
    original->AddBlock(primaryBlock);
    original->AddBlock(payloadBlock);

    std::vector<Ptr<Bundle>> fragments = Bundle::Fragment(original, maxFragmentSize);

    NS_TEST_ASSERT_MSG_EQ(fragments.size(), 3, "Expected ceil(250/100) = 3 fragments");

    uint32_t expectedOffset = 0;
    Ptr<Packet> reconstructed = Create<Packet>();
    for (const auto& fragment : fragments)
    {
        const PrimaryBlockHeader& h = fragment->GetPrimaryBlock()->GetHeader();
        NS_TEST_ASSERT_MSG_EQ(h.GetProcFlags() & (1 << PBB_PROC_FLAGS::IS_FRG),
                              static_cast<uint32_t>(1 << PBB_PROC_FLAGS::IS_FRG),
                              "Fragment must have IS_FRG set");
        NS_TEST_ASSERT_MSG_EQ(h.GetFragmentOffset(), expectedOffset, "Fragment offset mismatch");
        NS_TEST_ASSERT_MSG_EQ(h.GetTotalAppDataLength(), payloadSize, "Total ADU length mismatch");
        NS_TEST_ASSERT_MSG_EQ(h.GetDestinationEID(),
                              "dtn:node1",
                              "Fragment must retain original destination EID");

        Ptr<Packet> chunk = fragment->GetPayloadBlock()->GetPayload();
        expectedOffset += chunk->GetSize();
        reconstructed->AddAtEnd(chunk);
    }

    NS_TEST_ASSERT_MSG_EQ(reconstructed->GetSize(),
                          payloadSize,
                          "Reassembled payload size mismatch");

    uint8_t recovered[payloadSize];
    reconstructed->CopyData(recovered, payloadSize);
    for (uint32_t i = 0; i < payloadSize; ++i)
    {
        NS_TEST_ASSERT_MSG_EQ(static_cast<uint32_t>(recovered[i]),
                              static_cast<uint32_t>(data[i]),
                              "Reassembled payload content mismatch at byte " << i);
    }
}

/**
 * @ingroup dtn-test
 * @ingroup tests
 *
 * @brief A mock CLA that captures every serialized packet it is asked to send.
 */
class CapturingBundleCla : public BundleCla
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("ns3::CapturingBundleCla")
                                .SetParent<BundleCla>()
                                .AddConstructor<CapturingBundleCla>();
        return tid;
    }

    void Send(Ptr<Packet> packet, uint32_t bundleHandle) override
    {
        m_packets.push_back(packet->Copy());
        if (!m_txResultCb.IsNull() && bundleHandle != 0)
        {
            m_txResultCb(bundleHandle, true);
        }
    }

    bool IsUp() const override
    {
        return true;
    }

    /**
     * @brief Get every packet sent through this mock CLA, in send order.
     * @return the captured packets
     */
    const std::vector<Ptr<Packet>>& GetPackets() const
    {
        return m_packets;
    }

  private:
    std::vector<Ptr<Packet>> m_packets; //!< Packets captured via Send()
};

/**
 * @ingroup dtn-test
 * @ingroup tests
 *
 * @brief A mock Routing Engine that always routes directly to the bundle's destination.
 */
class DirectRoutingEngine : public BaseRoutingEngine
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("ns3::DirectRoutingEngine")
                                .SetParent<BaseRoutingEngine>()
                                .AddConstructor<DirectRoutingEngine>();
        return tid;
    }

    std::string GetNextHop(Ptr<Bundle> bundle, const std::string& currEID) override
    {
        return bundle->GetDestinationEID();
    }

    void InitializeMap(const std::vector<std::string>& eidList) override
    {
    }

    void AddContact(const std::string& fromEID,
                    const std::string& toEID,
                    uint32_t dataRate) override
    {
    }

    void RemoveContact(const std::string& fromEID, const std::string& toEID) override
    {
    }
};

NS_OBJECT_ENSURE_REGISTERED(CapturingBundleCla);
NS_OBJECT_ENSURE_REGISTERED(DirectRoutingEngine);

/**
 * @ingroup dtn-test
 * @ingroup tests
 *
 * @brief End-to-end tests for BundleAgent fragmentation and reassembly
 */
class BundleAgentFragmentationTestCase : public TestCase
{
  public:
    BundleAgentFragmentationTestCase();
    ~BundleAgentFragmentationTestCase() override;
    void DoRun() override;

    /**
     * @brief Callback triggered when the receiver agent delivers a bundle locally.
     * @param bundle The bundle that was received.
     */
    void LocalReceiveCallback(Ptr<Bundle> bundle);

  private:
    uint32_t m_receivedCount;  ///< Number of bundles delivered to the receive callback
    Ptr<Packet> m_lastPayload; ///< Payload of the most recently delivered bundle
};

BundleAgentFragmentationTestCase::BundleAgentFragmentationTestCase()
    : TestCase("BundleAgent Fragmentation and Reassembly"),
      m_receivedCount(0)
{
}

BundleAgentFragmentationTestCase::~BundleAgentFragmentationTestCase()
{
}

void
BundleAgentFragmentationTestCase::LocalReceiveCallback(Ptr<Bundle> bundle)
{
    m_receivedCount++;
    m_lastPayload = bundle->GetPayloadBlock()->GetPayload();
}

void
BundleAgentFragmentationTestCase::DoRun()
{
    const uint32_t payloadSize = 130;
    const uint32_t mtu = 50;

    uint8_t data[payloadSize];
    for (uint32_t i = 0; i < payloadSize; ++i)
    {
        data[i] = static_cast<uint8_t>(i % 256);
    }

    Ptr<BundleAgent> sender = CreateObject<BundleAgent>();
    sender->SetLocalEID("dtn:nodeA");
    sender->SetAttribute("FragmentationMtu", UintegerValue(mtu));
    Ptr<DirectRoutingEngine> routing = CreateObject<DirectRoutingEngine>();
    sender->SetContactGraph(routing);

    Ptr<CapturingBundleCla> cla = CreateObject<CapturingBundleCla>();
    sender->RegisterCla("dtn:nodeB", cla);

    sender->TransmitBundle("dtn:nodeB", "dtn:none", data, payloadSize, Seconds(3600), 0);

    NS_TEST_ASSERT_MSG_EQ(cla->GetPackets().size(),
                          3,
                          "Expected ceil(130/50) = 3 fragments to be sent");

    Ptr<BundleAgent> receiver = CreateObject<BundleAgent>();
    receiver->SetLocalEID("dtn:nodeB");
    receiver->SetReceiveCallback(
        MakeCallback(&BundleAgentFragmentationTestCase::LocalReceiveCallback, this));

    std::vector<Ptr<Bundle>> received;
    for (const auto& packet : cla->GetPackets())
    {
        Ptr<Bundle> bundle = CreateObject<Bundle>();
        bundle->Deserialize(packet);
        received.push_back(bundle);
    }

    receiver->RecvBundle(received[2]);
    NS_TEST_ASSERT_MSG_EQ(m_receivedCount, 0, "Must not deliver on a partial fragment set");

    receiver->RecvBundle(received[0]);
    NS_TEST_ASSERT_MSG_EQ(m_receivedCount, 0, "Must not deliver with a gap in the fragment set");

    receiver->RecvBundle(received[1]);
    NS_TEST_ASSERT_MSG_EQ(m_receivedCount, 1, "Must deliver exactly once when set is complete");

    NS_TEST_ASSERT_MSG_EQ(m_lastPayload->GetSize(),
                          payloadSize,
                          "Reassembled payload size mismatch");
    uint8_t recovered[payloadSize];
    m_lastPayload->CopyData(recovered, payloadSize);
    for (uint32_t i = 0; i < payloadSize; ++i)
    {
        NS_TEST_ASSERT_MSG_EQ(static_cast<uint32_t>(recovered[i]),
                              static_cast<uint32_t>(data[i]),
                              "Reassembled payload content mismatch at byte " << i);
    }

    Ptr<BundleAgent> guardedSender = CreateObject<BundleAgent>();
    guardedSender->SetLocalEID("dtn:nodeC");
    guardedSender->SetAttribute("FragmentationMtu", UintegerValue(mtu));
    Ptr<DirectRoutingEngine> routing2 = CreateObject<DirectRoutingEngine>();
    guardedSender->SetContactGraph(routing2);

    Ptr<CapturingBundleCla> cla2 = CreateObject<CapturingBundleCla>();
    guardedSender->RegisterCla("dtn:nodeD", cla2);

    guardedSender->TransmitBundle("dtn:nodeD",
                                  "dtn:none",
                                  data,
                                  payloadSize,
                                  Seconds(3600),
                                  1 << PBB_PROC_FLAGS::NO_FRAGMENT);

    NS_TEST_ASSERT_MSG_EQ(cla2->GetPackets().size(),
                          1,
                          "NO_FRAGMENT bundle must be sent as a single bundle");

    Simulator::Destroy();
}

/**
 * @ingroup dtn-test
 * @ingroup tests
 *
 * @brief Bundle Fragmentation Test Suite
 */
class BundleFragmentationTestSuite : public TestSuite
{
  public:
    BundleFragmentationTestSuite()
        : TestSuite("bundle-fragmentation", Type::UNIT)
    {
        AddTestCase(new BundleFragmentTestCase(), TestCase::Duration::QUICK);
        AddTestCase(new BundleAgentFragmentationTestCase(), TestCase::Duration::QUICK);
    }
};

static BundleFragmentationTestSuite
    g_bundleFragmentationTestSuite; //!< Static variable for test initialization
