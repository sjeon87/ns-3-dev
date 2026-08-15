/*
 * Copyright (c) 2026 Somaiya Vidyavihar University, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manmita Das <das.manmita12@gmail.com>
 */

#include "ns3/ethernet-channel.h"
#include "ns3/ethernet-mac.h"
#include "ns3/ethernet-net-device.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/test.h"

using namespace ns3;
using namespace ns3::ethernet;

/**
 * @ingroup ethernet-test
 * @ingroup tests
 *
 * @brief Test full-duplex Ethernet operation.
 *
 * Verifies that two EthernetNetDevices connected by the same
 * EthernetChannel can transmit simultaneously in opposite
 * directions and successfully receive each other's packets.
 */
class EthernetFullDuplexTestCase : public TestCase
{
  public:
    EthernetFullDuplexTestCase();
    ~EthernetFullDuplexTestCase() override;

  private:
    /**
     * @brief Run the test case.
     */
    void DoRun() override;

    /**
     * @brief Receive callback for node A.
     *
     * Records the reception time and increments the receive counter.
     *
     * @param device Receiving network device.
     * @param packet Received packet.
     * @param protocol Ethernet protocol number.
     * @param sender Sender address.
     * @return Always returns true.
     */
    bool ReceiveAtA(Ptr<NetDevice> device,
                    Ptr<const Packet> packet,
                    uint16_t protocol,
                    const Address& sender);

    /**
     * @brief Receive callback for node B.
     *
     * Records the reception time and increments the receive counter.
     *
     * @param device Receiving network device.
     * @param packet Received packet.
     * @param protocol Ethernet protocol number.
     * @param sender Sender address.
     * @return Always returns true.
     */
    bool ReceiveAtB(Ptr<NetDevice> device,
                    Ptr<const Packet> packet,
                    uint16_t protocol,
                    const Address& sender);

    /**
     * @brief Trace callback for transmission at Node A.
     * @param packet The transmitted packet.
     */
    void TxTraceA(Ptr<const Packet> packet);

    /**
     * @brief Trace callback for transmission at Node B.
     * @param packet The transmitted packet.
     */
    void TxTraceB(Ptr<const Packet> packet);

    Time m_txTimeA; //!< Transmission time for Node A
    Time m_txTimeB; //!< Transmission time for Node B
    Time m_rxTimeA; //!< Reception time for Node A
    Time m_rxTimeB; //!< Reception time for Node B

    uint32_t m_rxA{0}; //!< Receive count for Node A
    uint32_t m_rxB{0}; //!< Receive count for Node B
};

EthernetFullDuplexTestCase::EthernetFullDuplexTestCase()
    : TestCase("Full-duplex Ethernet transmission test")
{
}

EthernetFullDuplexTestCase::~EthernetFullDuplexTestCase()
{
}

bool
EthernetFullDuplexTestCase::ReceiveAtA(Ptr<NetDevice>, Ptr<const Packet>, uint16_t, const Address&)
{
    // Record the reception time and count packets received at node A.
    m_rxTimeA = Simulator::Now();
    m_rxA++;
    return true;
}

bool
EthernetFullDuplexTestCase::ReceiveAtB(Ptr<NetDevice>, Ptr<const Packet>, uint16_t, const Address&)
{
    // Record the reception time and count packets received at node B.
    m_rxTimeB = Simulator::Now();
    m_rxB++;
    return true;
}

void
EthernetFullDuplexTestCase::TxTraceA(Ptr<const Packet>)
{
    m_txTimeA = Simulator::Now();
}

void
EthernetFullDuplexTestCase::TxTraceB(Ptr<const Packet>)
{
    m_txTimeB = Simulator::Now();
}

void
EthernetFullDuplexTestCase::DoRun()
{
    Ptr<Node> nodeA = CreateObject<Node>();
    Ptr<Node> nodeB = CreateObject<Node>();

    Ptr<EthernetNetDevice> devA = CreateObject<EthernetNetDevice>();
    Ptr<EthernetNetDevice> devB = CreateObject<EthernetNetDevice>();

    Ptr<EthernetChannel> channel = CreateObject<EthernetChannel>();

    devA->Attach(channel);
    devB->Attach(channel);

    devA->SetAddress(Mac48Address::Allocate());
    devB->SetAddress(Mac48Address::Allocate());

    nodeA->AddDevice(devA);
    nodeB->AddDevice(devB);

    devA->SetReceiveCallback(MakeCallback(&EthernetFullDuplexTestCase::ReceiveAtA, this));

    devB->SetReceiveCallback(MakeCallback(&EthernetFullDuplexTestCase::ReceiveAtB, this));

    // MacTx is a trace source of the MAC sublayer, not of the netdevice.
    devA->GetMac()->TraceConnectWithoutContext(
        "MacTx",
        MakeCallback(&EthernetFullDuplexTestCase::TxTraceA, this));

    devB->GetMac()->TraceConnectWithoutContext(
        "MacTx",
        MakeCallback(&EthernetFullDuplexTestCase::TxTraceB, this));

    // Create packets to be transmitted simultaneously in opposite directions.
    Ptr<Packet> pktA = Create<Packet>(100);
    Ptr<Packet> pktB = Create<Packet>(100);

    // Schedule simultaneous transmissions from both nodes.
    Simulator::Schedule(Seconds(1.0),
                        &EthernetNetDevice::Send,
                        devA,
                        pktA,
                        devB->GetAddress(),
                        0x0800);

    Simulator::Schedule(Seconds(1.0),
                        &EthernetNetDevice::Send,
                        devB,
                        pktB,
                        devA->GetAddress(),
                        0x0800);

    Simulator::Stop(Seconds(2.0));
    Simulator::Run();

    // Verify that both nodes successfully receive one packet.
    NS_TEST_ASSERT_MSG_EQ(
        m_rxA,
        1,
        "Node A should receive exactly one packet during simultaneous transmission");

    NS_TEST_ASSERT_MSG_EQ(
        m_rxB,
        1,
        "Node B should receive exactly one packet during simultaneous transmission");

    // Verify that neither transmission was deferred because of the other one,
    // which is what distinguishes full-duplex from half-duplex operation.
    NS_TEST_ASSERT_MSG_EQ(m_txTimeA,
                          Seconds(1.0),
                          "Node A should transmit as soon as it is asked to");

    NS_TEST_ASSERT_MSG_EQ(m_txTimeB, m_txTimeA, "Both nodes should transmit at the same instant");

    NS_TEST_ASSERT_MSG_EQ(m_rxTimeA, m_rxTimeB, "Both nodes should receive at the same instant");

    NS_TEST_ASSERT_MSG_GT(m_rxTimeA,
                          m_txTimeA,
                          "Reception should complete after transmission has started");

    Simulator::Destroy();
}

/**
 * @ingroup ethernet-test
 * @brief Ethernet full-duplex test suite.
 */
class EthernetFullDuplexTestSuite : public TestSuite
{
  public:
    EthernetFullDuplexTestSuite();
};

EthernetFullDuplexTestSuite::EthernetFullDuplexTestSuite()
    : TestSuite("ethernet-full-duplex-test", Type::UNIT)
{
    AddTestCase(new EthernetFullDuplexTestCase, TestCase::Duration::QUICK);
}

static EthernetFullDuplexTestSuite g_ethernetFullDuplexTestSuite; //!< The testsuite
