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
#include "ns3/bundle.h"
#include "ns3/inet-socket-address.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address.h"
#include "ns3/node-container.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/udp-convergence-layer-adapter.h"

using namespace ns3;

/**
 * @ingroup dtn-test
 * @ingroup tests
 *
 * @brief Unit tests for the UDP Convergence Layer Adapter
 */
class UdpBundleClaTestCase : public TestCase
{
  public:
    UdpBundleClaTestCase();
    ~UdpBundleClaTestCase() override;
    void DoRun() override;

    /**
     * @brief Callback invoked when a bundle is received.
     * @param bundle The received bundle.
     * @return 0 on success.
     */
    uint32_t ReceiveBundleCallback(Ptr<Bundle> bundle);

  private:
    uint32_t m_receivedBundles; //!< The number of received bundles
};

UdpBundleClaTestCase::UdpBundleClaTestCase()
    : TestCase("UdpBundleCla Implementation"),
      m_receivedBundles(0)
{
}

UdpBundleClaTestCase::~UdpBundleClaTestCase()
{
}

uint32_t
UdpBundleClaTestCase::ReceiveBundleCallback(Ptr<Bundle> bundle)
{
    m_receivedBundles++;
    return 0;
}

void
UdpBundleClaTestCase::DoRun()
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

    Ptr<UdpBundleCla> claA = CreateObject<UdpBundleCla>();
    Ptr<UdpBundleCla> claB = CreateObject<UdpBundleCla>();

    claA->Setup(node, addrA, addrB);
    NS_TEST_ASSERT_MSG_EQ(claA->IsUp(), true, "CLA A failed to start");

    claB->Setup(node, addrB, addrA);
    NS_TEST_ASSERT_MSG_EQ(claB->IsUp(), true, "CLA B failed to start");

    claB->SetRxCallback(MakeCallback(&UdpBundleClaTestCase::ReceiveBundleCallback, this));

    Ptr<Bundle> bundle = CreateObject<Bundle>();

    Ptr<PrimaryBlock> pb = CreateObject<PrimaryBlock>();
    pb->GetHeader().SetVersion(7);
    pb->GetHeader().SetDestinationEID("dtn:nodeB");
    pb->GetHeader().SetSourceEID("dtn:nodeA");
    pb->GetHeader().SetReportToEID("dtn:none");
    bundle->AddBlock(pb);

    Ptr<PayloadBlock> pl = CreateObject<PayloadBlock>();
    pl->GetHeader().SetBlockNumber(2);
    pl->GetHeader().SetCrcType(1);
    pl->SetPayload(Create<Packet>(100));
    bundle->AddBlock(pl);
    Ptr<Packet> packetToSend = bundle->Serialize();

    Simulator::Schedule(Seconds(1.0), &UdpBundleCla::Send, claA, packetToSend, 0);

    Simulator::Stop(Seconds(2.0));
    Simulator::Run();
    Simulator::Destroy();

    NS_TEST_ASSERT_MSG_EQ(m_receivedBundles, 1, "CLA B did not receive the bundle successfully");
}

/**
 * @ingroup dtn-test
 * @ingroup tests
 *
 * @brief UdpBundleCla Test Suite
 */
class UdpBundleClaTestSuite : public TestSuite
{
  public:
    UdpBundleClaTestSuite()
        : TestSuite("udp-bundle-cla", Type::UNIT)
    {
        AddTestCase(new UdpBundleClaTestCase(), TestCase::Duration::QUICK);
    }
};

static UdpBundleClaTestSuite g_udpBundleClaTestSuite; //!< Static variable for test initialization
