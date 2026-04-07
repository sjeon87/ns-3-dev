/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ns3/bundle.h"
#include "ns3/inet-socket-address.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address.h"
#include "ns3/node-container.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/tcp-convergence-layer-adapter.h"

using namespace ns3;

/**
 * @ingroup dtn-test
 * @ingroup tests
 *
 * @brief Unit tests for the TCP Convergence Layer Adapter
 */
class TcpBundleClaTestCase : public TestCase
{
  public:
    TcpBundleClaTestCase();
    ~TcpBundleClaTestCase() override;
    void DoRun() override;

    uint32_t ReceiveBundleCallback(Ptr<Bundle> bundle);

  private:
    uint32_t m_receivedBundles;
};

TcpBundleClaTestCase::TcpBundleClaTestCase()
    : TestCase("TcpBundleCla Implementation"),
      m_receivedBundles(0)
{
}

TcpBundleClaTestCase::~TcpBundleClaTestCase()
{
}

uint32_t
TcpBundleClaTestCase::ReceiveBundleCallback(Ptr<Bundle> bundle)
{
    m_receivedBundles++;
    return 0;
}

void
TcpBundleClaTestCase::DoRun()
{
    m_receivedBundles = 0;

    NodeContainer nodes;
    nodes.Create(1);
    Ptr<Node> node = nodes.Get(0);

    InternetStackHelper internet;
    internet.Install(nodes);

    Ipv4Address loopback("127.0.0.1");
    Address addrA = InetSocketAddress(loopback, 1001);
    Address addrB = InetSocketAddress(loopback, 1002);

    Ptr<TcpBundleCla> claA = CreateObject<TcpBundleCla>();
    Ptr<TcpBundleCla> claB = CreateObject<TcpBundleCla>();

    claA->Setup(node, addrA, addrB);
    NS_TEST_ASSERT_MSG_EQ(claA->IsUp(), true, "CLA A failed to start");

    claB->Setup(node, addrB, addrA);
    NS_TEST_ASSERT_MSG_EQ(claB->IsUp(), true, "CLA B failed to start");

    claB->SetRxCallback(MakeCallback(&TcpBundleClaTestCase::ReceiveBundleCallback, this));

    Ptr<Packet> packet = Create<Packet>(100);

    Simulator::Schedule(Seconds(1.0), &TcpBundleCla::Send, claA, packet);

    Simulator::Stop(Seconds(2.0));
    Simulator::Run();
    Simulator::Destroy();

    NS_TEST_ASSERT_MSG_EQ(m_receivedBundles, 1, "CLA B did not receive the bundle successfully");
}

/**
 * @ingroup dtn-test
 * @ingroup tests
 *
 * @brief TcpBundleCla Test Suite
 */
class TcpBundleClaTestSuite : public TestSuite
{
  public:
    TcpBundleClaTestSuite()
        : TestSuite("tcp-bundle-cla", Type::UNIT)
    {
        AddTestCase(new TcpBundleClaTestCase(), TestCase::Duration::QUICK);
    }
};

static TcpBundleClaTestSuite g_tcpBundleClaTestSuite; //!< Static variable for test initialization
