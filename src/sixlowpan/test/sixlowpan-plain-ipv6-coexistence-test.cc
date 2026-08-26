/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Test for SixLowPanNetDevice EnablePlainIpv6 coexistence (issue #1360).
 */
#include "ns3/boolean.h"
#include "ns3/csma-channel.h"
#include "ns3/csma-net-device.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/iana-ieee802-numbers.h"
#include "ns3/ipv6-header.h"
#include "ns3/log.h"
#include "ns3/mac48-address.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/sixlowpan-net-device.h"
#include "ns3/test.h"
#include "ns3/uinteger.h"

using namespace ns3;

/**
 * @ingroup sixlowpan-tests
 *
 * @brief SixLowPanNetDevice / plain IPv6 coexistence test (issue #1360).
 *
 * Verifies that when EnablePlainIpv6 is set, a SixLowPanNetDevice sitting
 * on a real CsmaNetDevice/CsmaChannel sends below-threshold packets as
 * genuine plain IPv6 (EtherType 0x86DD), observable by a peer on the same
 * shared link that only understands plain IPv6 -- and that with the
 * attribute left at its default (false), legacy behavior (6LoWPAN-framed
 * uncompressed IPv6, EtherType 0xA0ED) is unchanged.
 */
class SixLowPanPlainIpv6CoexistenceTest : public TestCase
{
    bool m_loWpanHandlerFired{false};  //!< Whether the LoWPAN (0xA0ED) handler fired.
    bool m_ipv6HandlerFired{false};    //!< Whether the plain IPv6 (0x86DD) handler fired.
    Ptr<Packet> m_receivedPacket;      //!< Packet captured by whichever handler fired.

    /**
     * Protocol handler registered on the peer node for LoWPAN EtherType.
     *
     * @param device The receiving device.
     * @param packet The received packet.
     * @param protocol The protocol number.
     * @param source The source address.
     * @param destination The destination address.
     * @param packetType The packet type.
     */
    void ReceiveLoWPan(Ptr<NetDevice> device,
                       Ptr<const Packet> packet,
                       uint16_t protocol,
                       const Address& source,
                       const Address& destination,
                       NetDevice::PacketType packetType);
    /**
     * Protocol handler registered on the peer node for plain IPv6 EtherType.
     *
     * @param device The receiving device.
     * @param packet The received packet.
     * @param protocol The protocol number.
     * @param source The source address.
     * @param destination The destination address.
     * @param packetType The packet type.
     */
    void ReceivePlainIpv6(Ptr<NetDevice> device,
                         Ptr<const Packet> packet,
                         uint16_t protocol,
                         const Address& source,
                         const Address& destination,
                         NetDevice::PacketType packetType);

    /**
     * Build the two-node CSMA topology, send one below-threshold packet
     * from the SixLowPanNetDevice side, and record which EtherType the
     * peer observed it under.
     *
     * @param enablePlainIpv6 Value for the EnablePlainIpv6 attribute.
     */
    void SendOnePacket(bool enablePlainIpv6);

  public:
    SixLowPanPlainIpv6CoexistenceTest();
    void DoRun() override;
};

SixLowPanPlainIpv6CoexistenceTest::SixLowPanPlainIpv6CoexistenceTest()
    : TestCase("SixLowPanNetDevice plain IPv6 coexistence over a shared CSMA link (issue #1360)")
{
}

void
SixLowPanPlainIpv6CoexistenceTest::ReceiveLoWPan(Ptr<NetDevice> device,
                                                 Ptr<const Packet> packet,
                                                 uint16_t protocol,
                                                 const Address& source,
                                                 const Address& destination,
                                                 NetDevice::PacketType packetType)
{
    m_loWpanHandlerFired = true;
    m_receivedPacket = packet->Copy();
}

void
SixLowPanPlainIpv6CoexistenceTest::ReceivePlainIpv6(Ptr<NetDevice> device,
                                                    Ptr<const Packet> packet,
                                                    uint16_t protocol,
                                                    const Address& source,
                                                    const Address& destination,
                                                    NetDevice::PacketType packetType)
{
    m_ipv6HandlerFired = true;
    m_receivedPacket = packet->Copy();
}

void
SixLowPanPlainIpv6CoexistenceTest::SendOnePacket(bool enablePlainIpv6)
{
    m_loWpanHandlerFired = false;
    m_ipv6HandlerFired = false;
    m_receivedPacket = nullptr;

    // Node A: SixLowPanNetDevice over a real CsmaNetDevice.
    auto nodeA = CreateObject<Node>();
    auto csmaA = CreateObject<CsmaNetDevice>();
    csmaA->SetAddress(Mac48Address("00:00:00:00:00:01"));
    csmaA->SetQueue(CreateObject<DropTailQueue<Packet>>());
    nodeA->AddDevice(csmaA);

    auto six = CreateObject<SixLowPanNetDevice>();
    // Force the below-threshold fallback branch regardless of payload size.
    six->SetAttribute("CompressionThreshold", UintegerValue(1500));
    six->SetAttribute("EnablePlainIpv6", BooleanValue(enablePlainIpv6));
    nodeA->AddDevice(six);
    six->SetNetDevice(csmaA);

    // Node B: plain peer on the same segment, with raw handlers for both
    // possible EtherTypes -- exactly what a real coexistence scenario
    // needs to discriminate between 6LoWPAN and plain IPv6 traffic.
    auto nodeB = CreateObject<Node>();
    auto csmaB = CreateObject<CsmaNetDevice>();
    csmaB->SetAddress(Mac48Address("00:00:00:00:00:02"));
    csmaB->SetQueue(CreateObject<DropTailQueue<Packet>>());
    nodeB->AddDevice(csmaB);
    nodeB->RegisterProtocolHandler(
        MakeCallback(&SixLowPanPlainIpv6CoexistenceTest::ReceiveLoWPan, this),
        iana::ieee802numbers::LoWPAN,
        csmaB,
        false);
    nodeB->RegisterProtocolHandler(
        MakeCallback(&SixLowPanPlainIpv6CoexistenceTest::ReceivePlainIpv6, this),
        iana::ieee802numbers::IPV6,
        csmaB,
        false);

    // Join both devices on one shared CSMA channel -- the actual "same
    // Ethernet segment" scenario from issue #1360.
    auto channel = CreateObject<CsmaChannel>();
    csmaA->Attach(channel);
    csmaB->Attach(channel);

    Ptr<Packet> pkt = Create<Packet>(4);
    Ipv6Header ipHdr;
    ipHdr.SetSource(Ipv6Address("2001:1::1"));
    ipHdr.SetDestination(Ipv6Address("2001:1::2"));
    ipHdr.SetHopLimit(64);
    ipHdr.SetPayloadLength(4);
    ipHdr.SetNextHeader(0xff);
    pkt->AddHeader(ipHdr);

    six->Send(pkt, csmaB->GetAddress(), 0);

    Simulator::Run();
    Simulator::Destroy();
}

void
SixLowPanPlainIpv6CoexistenceTest::DoRun()
{
    // Default attribute value: legacy behavior must be unchanged -- the
    // below-threshold packet still goes out 6LoWPAN-framed (uncompressed),
    // under the LoWPAN EtherType. A plain-IPv6-only peer never sees it.
    SendOnePacket(false);
    NS_TEST_EXPECT_MSG_EQ(m_loWpanHandlerFired,
                          true,
                          "Expected legacy LoWPAN EtherType framing when EnablePlainIpv6=false");
    NS_TEST_EXPECT_MSG_EQ(m_ipv6HandlerFired,
                          false,
                          "Plain IPv6 handler must not fire when EnablePlainIpv6=false");

    // With EnablePlainIpv6 set, the same below-threshold packet must go
    // out as genuine plain IPv6, observable by a peer that only
    // understands plain IPv6 -- this is the actual coexistence fix.
    SendOnePacket(true);
    NS_TEST_EXPECT_MSG_EQ(m_ipv6HandlerFired,
                          true,
                          "Expected plain IPv6 EtherType framing when EnablePlainIpv6=true");
    NS_TEST_EXPECT_MSG_EQ(m_loWpanHandlerFired,
                          false,
                          "LoWPAN handler must not fire when EnablePlainIpv6=true");

    NS_TEST_ASSERT_MSG_NE(m_receivedPacket, nullptr, "Expected a packet to be received");
    Ipv6Header received;
    m_receivedPacket->RemoveHeader(received);
    NS_TEST_EXPECT_MSG_EQ(received.GetSource(),
                          Ipv6Address("2001:1::1"),
                          "Expected the original, unwrapped IPv6 source address");
    NS_TEST_EXPECT_MSG_EQ(received.GetDestination(),
                          Ipv6Address("2001:1::2"),
                          "Expected the original, unwrapped IPv6 destination address");
}

/**
 * @ingroup sixlowpan-tests
 *
 * @brief SixLowPanNetDevice plain IPv6 coexistence TestSuite.
 */
class SixLowPanPlainIpv6CoexistenceTestSuite : public TestSuite
{
  public:
    SixLowPanPlainIpv6CoexistenceTestSuite();
};

SixLowPanPlainIpv6CoexistenceTestSuite::SixLowPanPlainIpv6CoexistenceTestSuite()
    : TestSuite("sixlowpan-plain-ipv6-coexistence", Type::UNIT)
{
    AddTestCase(new SixLowPanPlainIpv6CoexistenceTest(), TestCase::Duration::QUICK);
}

static SixLowPanPlainIpv6CoexistenceTestSuite g_sixLowPanPlainIpv6CoexistenceTestSuite;
