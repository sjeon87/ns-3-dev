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

#include "ns3/base-routing-engine.h"
#include "ns3/bundle-agent.h"
#include "ns3/bundle.h"
#include "ns3/generic-convergence-layer-adapter.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/test.h"

using namespace ns3;

/**
 * @ingroup dtn-test
 * @ingroup tests
 *
 * @brief A mock Convergence Layer Adapter for testing the BundleAgent.
 */
class MockBundleCla : public BundleCla
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId()
    {
        static TypeId tid =
            TypeId("ns3::MockBundleCla").SetParent<BundleCla>().AddConstructor<MockBundleCla>();
        return tid;
    }

    MockBundleCla()
        : m_sentCount(0)
    {
    }

    void Send(Ptr<Packet> packet, uint32_t bundleHandle) override
    {
        m_sentCount++;
        m_lastPacketSize = packet->GetSize();
        m_lastPacket = packet;

        // Fire the success callback so the BundleAgent knows to delete the bundle from storage
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
     * @brief Get the total number of packets sent through this mock CLA.
     * @return The number of sent packets.
     */
    uint32_t GetSentCount() const
    {
        return m_sentCount;
    }

    /**
     * @brief Get the size of the last packet sent.
     * @return The size of the last packet in bytes.
     */
    uint32_t GetLastPacketSize() const
    {
        return m_lastPacketSize;
    }

    /**
     * @brief Get the most recently sent packet.
     * @return The last serialized bundle packet handed to Send().
     */
    Ptr<Packet> GetLastPacket() const
    {
        return m_lastPacket;
    }

  private:
    uint32_t m_sentCount;      ///< Total number of packets sent
    uint32_t m_lastPacketSize; ///< Size of the most recently sent packet
    Ptr<Packet> m_lastPacket;  ///< The most recently sent packet
};

/**
 * @ingroup dtn-test
 * @ingroup tests
 *
 * @brief A mock Routing Engine for testing the BundleAgent.
 */
class MockRoutingEngine : public BaseRoutingEngine
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("ns3::MockRoutingEngine")
                                .SetParent<BaseRoutingEngine>()
                                .AddConstructor<MockRoutingEngine>();
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

NS_OBJECT_ENSURE_REGISTERED(MockRoutingEngine);

NS_OBJECT_ENSURE_REGISTERED(MockBundleCla);

/**
 * @ingroup dtn-test
 * @ingroup tests
 *
 * @brief Unit tests for the Bundle Agent core routing logic
 */
class BundleAgentTestCase : public TestCase
{
  public:
    BundleAgentTestCase();
    ~BundleAgentTestCase() override;
    void DoRun() override;

    /**
     * @brief Callback triggered when a bundle is received locally by the agent.
     * @param bundle The bundle that was received.
     */
    void LocalReceiveCallback(Ptr<Bundle> bundle);

  private:
    uint32_t m_locallyReceivedCount; ///< Counter for the number of locally received bundles
};

BundleAgentTestCase::BundleAgentTestCase()
    : TestCase("BundleAgent Routing and Storage Logic"),
      m_locallyReceivedCount(0)
{
}

BundleAgentTestCase::~BundleAgentTestCase()
{
}

void
BundleAgentTestCase::LocalReceiveCallback(Ptr<Bundle> bundle)
{
    m_locallyReceivedCount++;

    NS_TEST_ASSERT_MSG_NE(bundle->GetPrimaryBlock(), nullptr, "Primary block missing");
    NS_TEST_ASSERT_MSG_NE(bundle->GetPayloadBlock(), nullptr, "Payload block missing");
    NS_TEST_ASSERT_MSG_EQ(bundle->GetHopCount(),
                          0,
                          "Bundle delivered to self should not have traversed any hops");
}

void
BundleAgentTestCase::DoRun()
{
    m_locallyReceivedCount = 0;

    Ptr<BundleAgent> agent = CreateObject<BundleAgent>();
    agent->SetLocalEID("dtn:nodeA");
    Ptr<MockRoutingEngine> contactGraph = CreateObject<MockRoutingEngine>();
    agent->SetContactGraph(contactGraph);
    agent->SetReceiveCallback(MakeCallback(&BundleAgentTestCase::LocalReceiveCallback, this));

    uint8_t dummyPayload[] = {0xDE, 0xAD, 0xBE, 0xEF};
    uint32_t payloadSize = sizeof(dummyPayload);

    agent->TransmitBundle("dtn:nodeA", "dtn:none", dummyPayload, payloadSize, Seconds(3600), 0);
    NS_TEST_ASSERT_MSG_EQ(m_locallyReceivedCount, 1, "Agent failed to deliver bundle to itself");

    Ptr<MockBundleCla> claB = CreateObject<MockBundleCla>();
    agent->RegisterCla("dtn:nodeB", claB);

    agent->TransmitBundle("dtn:nodeB", "dtn:none", dummyPayload, payloadSize, Seconds(3600), 0);
    NS_TEST_ASSERT_MSG_EQ(claB->GetSentCount(), 1, "Agent failed to forward to registered CLA");
    NS_TEST_ASSERT_MSG_EQ(agent->GetStorageEngineSize(),
                          0,
                          "Storage should be empty after direct forward");

    Ptr<Bundle> sentToB = CreateObject<Bundle>();
    sentToB->Deserialize(claB->GetLastPacket());
    NS_TEST_ASSERT_MSG_EQ(sentToB->GetHopCount(),
                          1,
                          "Hop count should be incremented once bundle is forwarded");

    agent->TransmitBundle("dtn:nodeC", "dtn:none", dummyPayload, payloadSize, Seconds(3600), 0);
    NS_TEST_ASSERT_MSG_EQ(agent->GetStorageEngineSize() > 0,
                          true,
                          "Bundle should be held in storage for missing CLA");

    Ptr<MockBundleCla> claC = CreateObject<MockBundleCla>();
    agent->RegisterCla("dtn:nodeC", claC);

    NS_TEST_ASSERT_MSG_EQ(claC->GetSentCount(),
                          1,
                          "Agent failed to process backlog upon CLA registration");
    NS_TEST_ASSERT_MSG_EQ(agent->GetStorageEngineSize(),
                          0,
                          "Storage should be empty after backlog processed");

    agent->TransmitBundle("dtn:nodeD", "dtn:none", dummyPayload, payloadSize, Seconds(1.0), 0);
    NS_TEST_ASSERT_MSG_EQ(agent->GetStorageEngineSize() > 0,
                          true,
                          "Short-lived bundle should be in storage");

    Simulator::Stop(Seconds(2.0));
    Simulator::Run();

    NS_TEST_ASSERT_MSG_EQ(agent->GetStorageEngineSize(),
                          0,
                          "Expired bundle was not deleted from storage engine");

    Simulator::Destroy();
}

/**
 * @ingroup dtn-test
 * @ingroup tests
 *
 * @brief BundleAgent Test Suite
 */
class BundleAgentTestSuite : public TestSuite
{
  public:
    BundleAgentTestSuite()
        : TestSuite("bundle-agent", Type::UNIT)
    {
        AddTestCase(new BundleAgentTestCase(), TestCase::Duration::QUICK);
    }
};

static BundleAgentTestSuite g_bundleAgentTestSuite; //!< Static variable for test initialization
