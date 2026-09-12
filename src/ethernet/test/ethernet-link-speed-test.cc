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
 * @brief Test Ethernet link speed configuration.
 *
 * Verifies that selecting different Ethernet link types configures
 * the correct data rate, transmission time and packet reception time.
 */
class EthernetLinkSpeedTestCase : public TestCase
{
  public:
    EthernetLinkSpeedTestCase();
    ~EthernetLinkSpeedTestCase() override;

  private:
    /**
     * @brief Execute the Ethernet link speed test.
     */
    void DoRun() override;

    /**
     * @brief Verify packet reception time for a given Ethernet link type.
     *
     * Creates a point-to-point Ethernet topology, transmits a packet, and
     * verifies that the measured reception time matches the expected frame
     * transmission time for the configured data rate.
     *
     * @param type The Ethernet link type to configure.
     * @param packet The packet to transmit.
     */
    void VerifyReceptionTime(EthernetLinkType type, Ptr<Packet> packet);

    /**
     * @brief Trace callback for transmission begin.
     * @param packet The transmitted packet.
     */
    void TxBeginTrace(Ptr<const Packet> packet);

    /**
     * @brief Trace callback for reception completion.
     *
     * Records the completion time of the first received packet.
     *
     * @param packet The received packet.
     */
    void RxEndTrace(Ptr<const Packet> packet);

    Time m_txStart; //!< Time when transmission begins
    Time m_rxEnd;   //!< Time when reception of first packet completes
};

EthernetLinkSpeedTestCase::EthernetLinkSpeedTestCase()
    : TestCase("Ethernet link speed test")
{
}

EthernetLinkSpeedTestCase::~EthernetLinkSpeedTestCase()
{
}

void
EthernetLinkSpeedTestCase::VerifyReceptionTime(EthernetLinkType type, Ptr<Packet> packet)
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

    devA->SetMaxSupportedLinkType(type);
    devB->SetMaxSupportedLinkType(type);

    // Reset timestamps before each transmission.
    m_txStart = Time();
    m_rxEnd = Time();

    // Record the transmission start and reception completion times.
    devA->GetPhy()->TraceConnectWithoutContext(
        "PhyTxBegin",
        MakeCallback(&EthernetLinkSpeedTestCase::TxBeginTrace, this));

    devB->GetPhy()->TraceConnectWithoutContext(
        "PhyRxEnd",
        MakeCallback(&EthernetLinkSpeedTestCase::RxEndTrace, this));

    // Ethernet frame overhead (header, preamble, SFD and FCS).
    constexpr uint32_t ETH_HEADER_SIZE = 14;
    constexpr uint32_t PREAMBLE_SFD_SIZE = 8;
    constexpr uint32_t FCS_SIZE = 4;

    // Compute the expected reception time for the complete Ethernet frame.
    uint32_t frameSize = packet->GetSize() + ETH_HEADER_SIZE + PREAMBLE_SFD_SIZE + FCS_SIZE;

    Time expectedRxTime = devA->GetDataRate().CalculateBytesTxTime(frameSize) + channel->GetDelay();

    Simulator::Schedule(Seconds(1.0),
                        &EthernetNetDevice::Send,
                        devA,
                        packet,
                        devB->GetAddress(),
                        0x0800);

    Simulator::Stop(Seconds(2.0));
    Simulator::Run();

    // Compare the measured reception time with the expected value.
    Time actualRxTime = m_rxEnd - m_txStart;

    NS_LOG_UNCOND("Actual RxTime   = " << actualRxTime.GetNanoSeconds());
    NS_LOG_UNCOND("Expected RxTime = " << expectedRxTime.GetNanoSeconds());

    NS_TEST_ASSERT_MSG_EQ(actualRxTime.GetNanoSeconds(),
                          expectedRxTime.GetNanoSeconds(),
                          "Reception time mismatch for link type: " << (uint32_t)type);

    Simulator::Destroy();
}

void
EthernetLinkSpeedTestCase::TxBeginTrace(Ptr<const Packet>)
{
    m_txStart = Simulator::Now();
}

void
EthernetLinkSpeedTestCase::RxEndTrace(Ptr<const Packet>)
{
    // Record only the first packet reception time.
    if (m_rxEnd.IsZero())
    {
        m_rxEnd = Simulator::Now();
    }
}

void
EthernetLinkSpeedTestCase::DoRun()
{
    Ptr<EthernetNetDevice> dev = CreateObject<EthernetNetDevice>();
    Ptr<EthernetChannel> channel = CreateObject<EthernetChannel>();

    dev->Attach(channel);
    Ptr<Packet> packet = Create<Packet>(1500);

    // Ethernet link types to verify.
    std::vector<EthernetLinkType> types = {EthernetLinkType::BASE10_T,
                                           EthernetLinkType::BASE100_TX,
                                           EthernetLinkType::BASE1000_T,
                                           EthernetLinkType::G10_T};

    for (const auto& type : types)
    {
        dev->SetMaxSupportedLinkType(type);

        NS_LOG_UNCOND("\n== Link type " << (uint32_t)type << " ==");

        uint64_t rate = dev->GetDataRate().GetBitRate();

        NS_LOG_UNCOND("DataRate: " << rate);

        // Compute the expected transmission time for frame.
        constexpr uint32_t PREAMBLE_SFD_SIZE = 8;
        uint32_t expectedBits = (packet->GetSize() + PREAMBLE_SFD_SIZE) * 8;

        Time expectedTxTime = Seconds(static_cast<double>(expectedBits) / rate);

        // Verify that the PHY reports the correct transmission time.
        NS_TEST_ASSERT_MSG_EQ(dev->GetPhy()->CalculateTxTime(packet).GetNanoSeconds(),
                              expectedTxTime.GetNanoSeconds(),
                              "TxTime mismatch for link type: " << (uint32_t)type);

        NS_LOG_UNCOND("TxTime matched for link type: " << (uint32_t)type);

        // Verify that a transmitted packet is received after the expected
        // frame transmission time for the configured PHY.
        VerifyReceptionTime(type, packet);
    }
}

/**
 * @ingroup ethernet-test
 * @brief Ethernet link speed test suite.
 */
class EthernetLinkSpeedTestSuite : public TestSuite
{
  public:
    EthernetLinkSpeedTestSuite();
};

EthernetLinkSpeedTestSuite::EthernetLinkSpeedTestSuite()
    : TestSuite("ethernet-link-speed-test", Type::UNIT)
{
    AddTestCase(new EthernetLinkSpeedTestCase, TestCase::Duration::QUICK);
}

static EthernetLinkSpeedTestSuite g_ethernetLinkSpeedTestSuite; //!< The testsuite
