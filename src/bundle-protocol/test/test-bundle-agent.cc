/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ns3/test.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"

#include "ns3/bundle.h"
#include "ns3/generic-convergence-layer-adapter.h"
#include "ns3/bundle-agent.h"

using namespace ns3;

class MockBundleCla : public BundleCla
{
public:
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("ns3::MockBundleCla")
                                .SetParent<BundleCla>()
                                .AddConstructor<MockBundleCla>();
        return tid;
    }

    MockBundleCla() : m_sentCount(0) {}

    void Send(Ptr<Packet> packet) override
    {
        m_sentCount++;
        m_lastPacketSize = packet->GetSize();
    }

    bool IsUp() const override { return true; }

    uint32_t GetSentCount() const { return m_sentCount; }
    uint32_t GetLastPacketSize() const { return m_lastPacketSize; }

private:
    uint32_t m_sentCount;
    uint32_t m_lastPacketSize;
};

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

    void LocalReceiveCallback(Ptr<Bundle> bundle);

private:
    uint32_t m_locallyReceivedCount;
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
}

void
BundleAgentTestCase::DoRun()
{
    m_locallyReceivedCount = 0;

    Ptr<BundleAgent> agent = CreateObject<BundleAgent>();
    agent->SetLocalEID("dtn:nodeA");
    agent->SetReceiveCallback(MakeCallback(&BundleAgentTestCase::LocalReceiveCallback, this));

    uint8_t dummyPayload[] = { 0xDE, 0xAD, 0xBE, 0xEF };
    uint32_t payloadSize = sizeof(dummyPayload);

    agent->TransmitBundle("dtn:nodeA", "none", dummyPayload, payloadSize, Seconds(3600), 0);
    NS_TEST_ASSERT_MSG_EQ(m_locallyReceivedCount, 1, "Agent failed to deliver bundle to itself");

    Ptr<MockBundleCla> claB = CreateObject<MockBundleCla>();
    agent->RegisterCla("dtn:nodeB", claB);
    
    agent->TransmitBundle("dtn:nodeB", "none", dummyPayload, payloadSize, Seconds(3600), 0);
    NS_TEST_ASSERT_MSG_EQ(claB->GetSentCount(), 1, "Agent failed to forward to registered CLA");
    NS_TEST_ASSERT_MSG_EQ(agent->GetStorageEngineSize(), 0, "Storage should be empty after direct forward");


    agent->TransmitBundle("dtn:nodeC", "none", dummyPayload, payloadSize, Seconds(3600), 0);
    NS_TEST_ASSERT_MSG_EQ(agent->GetStorageEngineSize() > 0, true, "Bundle should be held in storage for missing CLA");


    Ptr<MockBundleCla> claC = CreateObject<MockBundleCla>();
    agent->RegisterCla("dtn:nodeC", claC);
    
    NS_TEST_ASSERT_MSG_EQ(claC->GetSentCount(), 1, "Agent failed to process backlog upon CLA registration");
    NS_TEST_ASSERT_MSG_EQ(agent->GetStorageEngineSize(), 0, "Storage should be empty after backlog processed");


    agent->TransmitBundle("dtn:nodeD", "none", dummyPayload, payloadSize, Seconds(1.0), 0);
    NS_TEST_ASSERT_MSG_EQ(agent->GetStorageEngineSize() > 0, true, "Short-lived bundle should be in storage");


    Simulator::Stop(Seconds(2.0));
    Simulator::Run();


    NS_TEST_ASSERT_MSG_EQ(agent->GetStorageEngineSize(), 0, "Expired bundle was not deleted from storage engine");

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