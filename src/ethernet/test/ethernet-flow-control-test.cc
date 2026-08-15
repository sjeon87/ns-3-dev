/*
 * Copyright (c) 2026 Somaiya Vidyavihar University, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manmita Das <das.manmita12@gmail.com>
 */

#include "ns3/drop-tail-queue.h"
#include "ns3/ethernet-channel.h"
#include "ns3/ethernet-header.h"
#include "ns3/ethernet-mac.h"
#include "ns3/ethernet-net-device.h"
#include "ns3/ethernet-phy.h"
#include "ns3/log.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/test.h"

#include <algorithm>
#include <vector>

using namespace ns3;
using namespace ns3::ethernet;

/**
 * @ingroup ethernet-test
 * @ingroup tests
 *
 * @brief Test that an IEEE 802.3x PAUSE frame suspends and resumes transmission.
 *
 * A device with a transmit backlog is sent a PAUSE frame carrying a known number
 * of pause quanta. The test verifies that the device stops transmitting for the
 * corresponding interval and then drains the rest of its backlog.
 */
class EthernetPauseFrameTestCase : public TestCase
{
  public:
    EthernetPauseFrameTestCase();
    ~EthernetPauseFrameTestCase() override;

  private:
    void DoRun() override;

    /**
     * @brief Record the start of a transmission.
     * @param packet The transmitted frame.
     */
    void TxBeginTrace(Ptr<const Packet> packet);

    std::vector<Time> m_txTimes; //!< Start time of every frame sent by the paused device
};

EthernetPauseFrameTestCase::EthernetPauseFrameTestCase()
    : TestCase("PAUSE frame suspends and resumes transmission")
{
}

EthernetPauseFrameTestCase::~EthernetPauseFrameTestCase()
{
}

void
EthernetPauseFrameTestCase::TxBeginTrace(Ptr<const Packet>)
{
    m_txTimes.push_back(Simulator::Now());
}

void
EthernetPauseFrameTestCase::DoRun()
{
    // Pause quanta requested by the PAUSE frame, and the backlog queued up for
    // transmission before the PAUSE is sent.
    const uint16_t pauseQuanta = 1000;
    const uint32_t nFrames = 60;
    const uint32_t payloadSize = 1500;

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

    // Pin the link type, so that the test does not depend on the default.
    devA->SetMaxSupportedLinkType(EthernetLinkType::G10_T);
    devB->SetMaxSupportedLinkType(EthernetLinkType::G10_T);

    // One pause quantum is 512 bit times on the link.
    Time pauseDuration = Seconds(512.0 * pauseQuanta / devA->GetDataRate().GetBitRate());

    // Time taken by a full frame, including header, FCS, preamble and SFD.
    Time frameTime = devA->GetDataRate().CalculateBytesTxTime(payloadSize + 14 + 4 + 8);

    devA->GetPhy()->TraceConnectWithoutContext(
        "PhyTxBegin",
        MakeCallback(&EthernetPauseFrameTestCase::TxBeginTrace, this));

    Time burstStart = MilliSeconds(1);

    for (uint32_t i = 0; i < nFrames; ++i)
    {
        Simulator::Schedule(burstStart,
                            &EthernetNetDevice::Send,
                            devA,
                            Create<Packet>(payloadSize),
                            devB->GetAddress(),
                            0x0800);
    }

    // Pause a few frames into the burst, so that it is interrupted rather than
    // delayed as a whole.
    Simulator::Schedule(burstStart + frameTime * 4,
                        &EthernetMac::SendPauseFrame,
                        devB->GetMac(),
                        pauseQuanta);

    Simulator::Stop(burstStart + pauseDuration + frameTime * nFrames * 2);
    Simulator::Run();

    NS_TEST_ASSERT_MSG_EQ(m_txTimes.size(),
                          nFrames,
                          "Every queued frame should eventually be transmitted");

    Time largestGap;
    for (std::size_t i = 1; i < m_txTimes.size(); ++i)
    {
        largestGap = std::max(largestGap, m_txTimes[i] - m_txTimes[i - 1]);
    }

    NS_TEST_ASSERT_MSG_GT_OR_EQ(largestGap,
                                pauseDuration,
                                "Transmission should be suspended for the pause interval");

    // The PAUSE only takes effect once the frame already on the wire completes,
    // so the observed gap may exceed the pause interval by up to one frame, but
    // no more than that.
    NS_TEST_ASSERT_MSG_LT(largestGap,
                          pauseDuration + frameTime * 2,
                          "Transmission should resume as soon as the pause interval expires");

    Simulator::Destroy();
}

/**
 * @ingroup ethernet-test
 * @ingroup tests
 *
 * @brief Test that receive queue congestion generates a PAUSE frame.
 *
 * A device whose receive queue is congested is expected to emit a PAUSE
 * frame towards its peer.
 */
class EthernetCongestionTestCase : public TestCase
{
  public:
    EthernetCongestionTestCase();
    ~EthernetCongestionTestCase() override;

  private:
    void DoRun() override;

    /**
     * @brief Count PAUSE frames transmitted by the congested device.
     * @param packet The transmitted frame.
     */
    void TxBeginTrace(Ptr<const Packet> packet);

    uint32_t m_pauseFrames{0}; //!< PAUSE frames sent by the congested device
};

EthernetCongestionTestCase::EthernetCongestionTestCase()
    : TestCase("Receive queue congestion generates a PAUSE frame")
{
}

EthernetCongestionTestCase::~EthernetCongestionTestCase()
{
}

void
EthernetCongestionTestCase::TxBeginTrace(Ptr<const Packet> packet)
{
    EthernetHeader header(false);
    Ptr<Packet> copy = packet->Copy();
    copy->RemoveHeader(header);

    if (header.GetLengthType() == EthernetMac::PAUSE_LENGTH_TYPE)
    {
        ++m_pauseFrames;
    }
}

void
EthernetCongestionTestCase::DoRun()
{
    const uint32_t payloadSize = 1000;

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

    // Pin the link type so the test does not depend on the default.
    devA->SetMaxSupportedLinkType(EthernetLinkType::G10_T);
    devB->SetMaxSupportedLinkType(EthernetLinkType::G10_T);

    /*
     * ShouldSendPauseFrame() sends PAUSE when the queue is >= 80% full.
     * With a 2-packet queue, two packets make it 100% full.
     */
    Ptr<DropTailQueue<Packet>> rxQueue = CreateObject<DropTailQueue<Packet>>();

    rxQueue->SetMaxSize(QueueSize("2p"));
    devA->GetMac()->SetRxQueue(rxQueue);

    rxQueue->Enqueue(Create<Packet>(payloadSize));
    rxQueue->Enqueue(Create<Packet>(payloadSize));

    NS_TEST_ASSERT_MSG_EQ(rxQueue->GetNPackets(),
                          2u,
                          "A's RX queue should be full before the test frame arrives");

    // Count PAUSE frames transmitted by A.
    devA->GetPhy()->TraceConnectWithoutContext(
        "PhyTxBegin",
        MakeCallback(&EthernetCongestionTestCase::TxBeginTrace, this));

    devB->Send(Create<Packet>(payloadSize), devA->GetAddress(), 0x0800);

    Simulator::Stop(MilliSeconds(1));
    Simulator::Run();

    NS_TEST_ASSERT_MSG_GT_OR_EQ(m_pauseFrames,
                                1u,
                                "A congested receiver should emit at least one PAUSE frame");

    Simulator::Destroy();
}

/**
 * @ingroup ethernet-test
 * @brief Ethernet flow control test suite.
 */
class EthernetFlowControlTestSuite : public TestSuite
{
  public:
    EthernetFlowControlTestSuite();
};

EthernetFlowControlTestSuite::EthernetFlowControlTestSuite()
    : TestSuite("ethernet-flow-control-test", Type::UNIT)
{
    AddTestCase(new EthernetPauseFrameTestCase, TestCase::Duration::QUICK);
    AddTestCase(new EthernetCongestionTestCase, TestCase::Duration::QUICK);
}

static EthernetFlowControlTestSuite g_EthernetFlowControlTestSuite; //!< The testsuite
