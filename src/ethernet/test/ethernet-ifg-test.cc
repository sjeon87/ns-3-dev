/*
 * Copyright (c) 2026 Somaiya Vidyavihar University, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manmita Das <das.manmita12@gmail.com>
 */

#include "ns3/ethernet-channel.h"
#include "ns3/ethernet-net-device.h"
#include "ns3/ethernet-phy.h"
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
 * @brief Test Ethernet inter-frame gap.
 *
 * Verifies that queued packets are transmitted only after
 * the configured inter-frame gap expires.
 */
class EthernetInterframeGapTestCase : public TestCase
{
  public:
    EthernetInterframeGapTestCase();
    ~EthernetInterframeGapTestCase() override;

  private:
    /**
     * @brief Run the test case.
     */
    void DoRun() override;

    /**
     * @brief Trace callback for transmission begin.
     * @param packet The transmitted packet.
     */
    void TxBeginTrace(Ptr<const Packet> packet);

    /**
     * @brief Trace callback for transmission end.
     * @param packet The transmitted packet.
     */
    void TxEndTrace(Ptr<const Packet> packet);

    Time m_firstTxEnd;     //!< Time when the first transmission ends
    Time m_secondTxBegin;  //!< Time when the second transmission begins
    uint32_t m_txCount{0}; //!< Transmission count
};

EthernetInterframeGapTestCase::EthernetInterframeGapTestCase()
    : TestCase("Ethernet inter-frame gap test")
{
}

EthernetInterframeGapTestCase::~EthernetInterframeGapTestCase()
{
}

void
EthernetInterframeGapTestCase::TxBeginTrace(Ptr<const Packet>)
{
    ++m_txCount;

    if (m_txCount == 2)
    {
        m_secondTxBegin = Simulator::Now();
    }
}

void
EthernetInterframeGapTestCase::TxEndTrace(Ptr<const Packet>)
{
    if (m_firstTxEnd.IsZero())
    {
        m_firstTxEnd = Simulator::Now();
    }
}

void
EthernetInterframeGapTestCase::DoRun()
{
    // Verify the inter-frame gap for each supported Ethernet link type.
    std::vector<EthernetLinkType> modes = {EthernetLinkType::BASE10_T,
                                           EthernetLinkType::BASE100_TX,
                                           EthernetLinkType::BASE1000_T,
                                           EthernetLinkType::G10_T};

    for (auto mode : modes)
    {
        NS_LOG_UNCOND("\n=== Testing mode " << (uint32_t)mode << " ===");
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

        devA->SetMaxSupportedLinkType(mode);
        devB->SetMaxSupportedLinkType(mode);

        // Compute the expected inter-frame gap for the negotiated data rate.
        DataRate rate = devA->GetDataRate();
        Time expectedIfg = rate.CalculateBytesTxTime(96 / 8);

        // reset state
        m_txCount = 0;
        m_firstTxEnd = Time();
        m_secondTxBegin = Time();

        // Record the end of the first transmission and the beginning of the second.
        devA->GetPhy()->TraceConnectWithoutContext(
            "PhyTxBegin",
            MakeCallback(&EthernetInterframeGapTestCase::TxBeginTrace, this));

        devA->GetPhy()->TraceConnectWithoutContext(
            "PhyTxEnd",
            MakeCallback(&EthernetInterframeGapTestCase::TxEndTrace, this));

        Ptr<Packet> pkt1 = Create<Packet>(1000);
        Ptr<Packet> pkt2 = Create<Packet>(100);

        // First packet
        Simulator::Schedule(Seconds(1.0),
                            &EthernetNetDevice::Send,
                            devA,
                            pkt1,
                            devB->GetAddress(),
                            0x0800);

        // Second packet
        Simulator::Schedule(Seconds(1.0),
                            &EthernetNetDevice::Send,
                            devA,
                            pkt2,
                            devB->GetAddress(),
                            0x0800);

        Simulator::Stop(Seconds(3.0));
        Simulator::Run();

        // Verify that the measured inter-frame gap matches the expected value.
        int64_t actual = (m_secondTxBegin - m_firstTxEnd).GetNanoSeconds();
        int64_t expected = expectedIfg.GetNanoSeconds();

        NS_LOG_UNCOND("Actual IFG   = " << actual);
        NS_LOG_UNCOND("Expected IFG = " << expected);

        NS_TEST_ASSERT_MSG_EQ_TOL(actual, expected, expected * 0.05, "IFG mismatch for PHY mode");

        Simulator::Destroy();
    }
}

/**
 * @ingroup ethernet-test
 * @brief Ethernet inter-frame gap test suite.
 */
class EthernetInterframeGapTestSuite : public TestSuite
{
  public:
    EthernetInterframeGapTestSuite();
};

EthernetInterframeGapTestSuite::EthernetInterframeGapTestSuite()
    : TestSuite("ethernet-ifg-test", Type::UNIT)
{
    AddTestCase(new EthernetInterframeGapTestCase, TestCase::Duration::QUICK);
}

static EthernetInterframeGapTestSuite g_ethernetInterframeGapTestSuite; //!< The testsuite
