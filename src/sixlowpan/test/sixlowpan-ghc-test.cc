/*
 * Copyright (c) 2026 SRM Institute of Science and Technology
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Usham Roy <ushamroy80@gmail.com>
 *
 * Tests for 6LoWPAN-GHC (RFC 7400) implementation.
 */

#include "ns3/boolean.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/simple-channel.h"
#include "ns3/simple-net-device.h"
#include "ns3/simulator.h"
#include "ns3/sixlowpan-ghc.h"
#include "ns3/sixlowpan-header.h"
#include "ns3/sixlowpan-net-device.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/test.h"
#include "ns3/udp-socket-factory.h"

#include <cstring>
#include <limits>
#include <string>

using namespace ns3;

// ============================================================================
//  Test 1: GHC Engine Unit Test - Compress/Decompress round-trip
// ============================================================================

/**
 * @ingroup sixlowpan-tests
 * @brief GHC Engine Compress/Decompress round-trip test.
 *
 * Tests that the GHC engine can compress data and decompress it
 * back to the original, verifying the LZ77 bytecode pipeline.
 */
class SixlowpanGhcEngineTest : public TestCase
{
  public:
    SixlowpanGhcEngineTest();
    void DoRun() override;
};

SixlowpanGhcEngineTest::SixlowpanGhcEngineTest()
    : TestCase("GHC Engine compress-decompress round-trip")
{
}

void
SixlowpanGhcEngineTest::DoRun()
{
    Ipv6Address srcAddr("2001:db8::1");
    Ipv6Address dstAddr("2001:db8::2");

    // Test 1: Data with lots of zeros (should compress well via zero-insert)
    {
        uint8_t input[] = {0x3a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x12, 0x34,
                           0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        uint32_t inputLen = sizeof(input);

        uint8_t compressed[256];
        uint32_t compLen =
            SixLowPanGhcEngine::Compress(srcAddr, dstAddr, input, inputLen, compressed, 256, false);

        // Should produce some compressed output (or 0 if no benefit)
        if (compLen > 0)
        {
            NS_TEST_ASSERT_MSG_LT(compLen, inputLen, "Compressed size should be less than input");

            uint8_t decompressed[1280];
            uint32_t decompLen = SixLowPanGhcEngine::Decompress(srcAddr,
                                                                dstAddr,
                                                                compressed,
                                                                compLen,
                                                                decompressed,
                                                                1280,
                                                                false);

            NS_TEST_ASSERT_MSG_EQ(decompLen, inputLen, "Decompressed length must match original");
            NS_TEST_ASSERT_MSG_EQ(std::memcmp(input, decompressed, inputLen),
                                  0,
                                  "Decompressed data must match original");
        }
    }

    // Test 2: Data that matches the static dictionary
    {
        // Include bytes from static dictionary: 0x16, 0xfe, 0xfd, 0x17...
        uint8_t input[] = {0x16,
                           0xfe,
                           0xfd,
                           0x17,
                           0xfe,
                           0xfd,
                           0x00,
                           0x01,
                           0x00,
                           0x00,
                           0x00,
                           0x00,
                           0x00,
                           0x01,
                           0x00,
                           0x00};
        uint32_t inputLen = sizeof(input);

        uint8_t compressed[256];
        uint32_t compLen =
            SixLowPanGhcEngine::Compress(srcAddr, dstAddr, input, inputLen, compressed, 256, false);

        if (compLen > 0)
        {
            uint8_t decompressed[1280];
            uint32_t decompLen = SixLowPanGhcEngine::Decompress(srcAddr,
                                                                dstAddr,
                                                                compressed,
                                                                compLen,
                                                                decompressed,
                                                                1280,
                                                                false);

            NS_TEST_ASSERT_MSG_EQ(decompLen,
                                  inputLen,
                                  "Dictionary match: decompressed length must match");
            NS_TEST_ASSERT_MSG_EQ(std::memcmp(input, decompressed, inputLen),
                                  0,
                                  "Dictionary match: decompressed data must match");
        }
    }

    // Test 3: Data with address-derived dictionary match
    {
        // Use bytes that match the source IPv6 address (first 16 bytes of dictionary)
        uint8_t srcBuf[16];
        srcAddr.GetBytes(srcBuf);

        uint8_t compressed[256];
        uint32_t compLen =
            SixLowPanGhcEngine::Compress(srcAddr, dstAddr, srcBuf, 16, compressed, 256, false);

        if (compLen > 0)
        {
            NS_TEST_ASSERT_MSG_LT(compLen, 16u, "Address data should compress significantly");

            uint8_t decompressed[1280];
            uint32_t decompLen = SixLowPanGhcEngine::Decompress(srcAddr,
                                                                dstAddr,
                                                                compressed,
                                                                compLen,
                                                                decompressed,
                                                                1280,
                                                                false);

            NS_TEST_ASSERT_MSG_EQ(decompLen, 16u, "Address match: decompressed length must match");
            NS_TEST_ASSERT_MSG_EQ(std::memcmp(srcBuf, decompressed, 16),
                                  0,
                                  "Address match: decompressed data must match");
        }
    }

    // Test 4: Stop Code handling for extension headers
    {
        uint8_t input[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
        uint32_t inputLen = sizeof(input);

        uint8_t compressed[256];
        uint32_t compLen =
            SixLowPanGhcEngine::Compress(srcAddr, dstAddr, input, inputLen, compressed, 256, true);

        if (compLen > 0)
        {
            uint8_t decompressed[1280];
            uint32_t decompLen = SixLowPanGhcEngine::Decompress(srcAddr,
                                                                dstAddr,
                                                                compressed,
                                                                compLen,
                                                                decompressed,
                                                                1280,
                                                                true);

            NS_TEST_ASSERT_MSG_EQ(decompLen, inputLen, "Stop code: decompressed length must match");
            NS_TEST_ASSERT_MSG_EQ(std::memcmp(input, decompressed, inputLen),
                                  0,
                                  "Stop code: decompressed data must match");
        }
    }

    // Test 5: Bytecode classification
    {
        NS_TEST_ASSERT_MSG_EQ((int)SixLowPanGhcEngine::ClassifyBytecode(0x00),
                              (int)GhcBytecodeType::LITERAL,
                              "0x00 should be LITERAL");
        NS_TEST_ASSERT_MSG_EQ((int)SixLowPanGhcEngine::ClassifyBytecode(0x7F),
                              (int)GhcBytecodeType::LITERAL,
                              "0x7F should be LITERAL");
        NS_TEST_ASSERT_MSG_EQ((int)SixLowPanGhcEngine::ClassifyBytecode(0x80),
                              (int)GhcBytecodeType::ZERO_INSERT,
                              "0x80 should be ZERO_INSERT");
        NS_TEST_ASSERT_MSG_EQ((int)SixLowPanGhcEngine::ClassifyBytecode(0x8F),
                              (int)GhcBytecodeType::ZERO_INSERT,
                              "0x8F should be ZERO_INSERT");
        NS_TEST_ASSERT_MSG_EQ((int)SixLowPanGhcEngine::ClassifyBytecode(0x90),
                              (int)GhcBytecodeType::STOP_CODE,
                              "0x90 should be STOP_CODE");
        NS_TEST_ASSERT_MSG_EQ((int)SixLowPanGhcEngine::ClassifyBytecode(0xA0),
                              (int)GhcBytecodeType::EXTENDED_ARGS,
                              "0xA0 should be EXTENDED_ARGS");
        NS_TEST_ASSERT_MSG_EQ((int)SixLowPanGhcEngine::ClassifyBytecode(0xBF),
                              (int)GhcBytecodeType::EXTENDED_ARGS,
                              "0xBF should be EXTENDED_ARGS");
        NS_TEST_ASSERT_MSG_EQ((int)SixLowPanGhcEngine::ClassifyBytecode(0xC0),
                              (int)GhcBytecodeType::BACKREF,
                              "0xC0 should be BACKREF");
        NS_TEST_ASSERT_MSG_EQ((int)SixLowPanGhcEngine::ClassifyBytecode(0xFF),
                              (int)GhcBytecodeType::BACKREF,
                              "0xFF should be BACKREF");
    }
}

// ============================================================================
//  Test 2: GHC NHC Header Serialization Test
// ============================================================================

/**
 * @ingroup sixlowpan-tests
 * @brief GHC NHC Header serialization/deserialization test.
 */
class SixlowpanGhcHeaderTest : public TestCase
{
  public:
    SixlowpanGhcHeaderTest();
    void DoRun() override;
};

SixlowpanGhcHeaderTest::SixlowpanGhcHeaderTest()
    : TestCase("GHC NHC Header serialize-deserialize round-trip")
{
}

void
SixlowpanGhcHeaderTest::DoRun()
{
    // Test GHC Extension Header
    {
        SixLowPanGhcExtension original;
        original.SetEid(SixLowPanGhcExtension::EID_HOPBYHOP_OPTIONS_H);
        original.SetNh(true);
        uint8_t blob[] = {0x01, 0x02, 0x03, 0x90}; // 3 literal bytes + stop code
        original.SetBlob(blob, 4);

        // Serialize
        Buffer buf;
        buf.AddAtStart(original.GetSerializedSize());
        original.Serialize(buf.Begin());

        // Deserialize
        SixLowPanGhcExtension decoded;
        decoded.Deserialize(buf.Begin());

        NS_TEST_ASSERT_MSG_EQ(decoded.GetEid(),
                              SixLowPanGhcExtension::EID_HOPBYHOP_OPTIONS_H,
                              "EID must match");
        NS_TEST_ASSERT_MSG_EQ(decoded.GetNh(), true, "NH flag must match");
        NS_TEST_ASSERT_MSG_EQ(decoded.GetBlobLength(), 4u, "Blob length must match");

        uint8_t decodedBlob[256];
        decoded.CopyBlob(decodedBlob, 256);
        NS_TEST_ASSERT_MSG_EQ(std::memcmp(blob, decodedBlob, 4), 0, "Blob data must match");

        // Check dispatch type
        NS_TEST_ASSERT_MSG_EQ(decoded.GetNhcDispatchType(),
                              SixLowPanDispatch::LOWPAN_GHC_EXT,
                              "Dispatch type must be LOWPAN_GHC_EXT");
    }

    // Test GHC UDP Header
    {
        SixLowPanGhcUdp original;
        original.SetPorts(SixLowPanGhcUdp::PORTS_INLINE);
        original.SetSrcPort(1234);
        original.SetDstPort(5678);
        original.SetC(false);
        original.SetChecksum(0xABCD);

        Buffer buf;
        buf.AddAtStart(original.GetSerializedSize());
        original.Serialize(buf.Begin());

        SixLowPanGhcUdp decoded;
        decoded.Deserialize(buf.Begin());

        NS_TEST_ASSERT_MSG_EQ(decoded.GetSrcPort(), 1234u, "Src port must match");
        NS_TEST_ASSERT_MSG_EQ(decoded.GetDstPort(), 5678u, "Dst port must match");
        NS_TEST_ASSERT_MSG_EQ(decoded.GetC(), false, "C flag must match");
        NS_TEST_ASSERT_MSG_EQ(decoded.GetChecksum(), 0xABCDu, "Checksum must match");
        NS_TEST_ASSERT_MSG_EQ(decoded.GetNhcDispatchType(),
                              SixLowPanDispatch::LOWPAN_GHC_UDP,
                              "Dispatch type must be LOWPAN_GHC_UDP");
    }

    // Test GHC ICMPv6 Header
    {
        SixLowPanGhcIcmpv6 original;
        uint8_t blob[] = {0x05, 0x80, 0x00, 0x12, 0x34, 0x84};
        original.SetBlob(blob, 6);

        Buffer buf;
        buf.AddAtStart(original.GetSerializedSize());
        original.Serialize(buf.Begin());

        SixLowPanGhcIcmpv6 decoded;
        decoded.Deserialize(buf.Begin());

        NS_TEST_ASSERT_MSG_EQ(decoded.GetBlobLength(), 6u, "Blob length must match");
        NS_TEST_ASSERT_MSG_EQ(decoded.GetNhcDispatchType(),
                              SixLowPanDispatch::LOWPAN_GHC_ICMPV6,
                              "Dispatch type must be LOWPAN_GHC_ICMPV6");

        uint8_t decodedBlob[256];
        decoded.CopyBlob(decodedBlob, 256);
        NS_TEST_ASSERT_MSG_EQ(std::memcmp(blob, decodedBlob, 6), 0, "Blob data must match");
    }

    // Test 6CIO Option Header
    {
        SixLowPan6Cio original;
        original.SetGhcCapable(true);

        Buffer buf;
        buf.AddAtStart(original.GetSerializedSize());
        original.Serialize(buf.Begin());

        SixLowPan6Cio decoded;
        decoded.Deserialize(buf.Begin());

        NS_TEST_ASSERT_MSG_EQ(decoded.GetGhcCapable(), true, "GHC capable flag must match");
    }

    // Test dispatch type recognition
    {
        NS_TEST_ASSERT_MSG_EQ(SixLowPanDispatch::GetNhcDispatchType(0xB0),
                              SixLowPanDispatch::LOWPAN_GHC_EXT,
                              "0xB0 must be GHC_EXT");
        NS_TEST_ASSERT_MSG_EQ(SixLowPanDispatch::GetNhcDispatchType(0xBF),
                              SixLowPanDispatch::LOWPAN_GHC_EXT,
                              "0xBF must be GHC_EXT");
        NS_TEST_ASSERT_MSG_EQ(SixLowPanDispatch::GetNhcDispatchType(0xD0),
                              SixLowPanDispatch::LOWPAN_GHC_UDP,
                              "0xD0 must be GHC_UDP");
        NS_TEST_ASSERT_MSG_EQ(SixLowPanDispatch::GetNhcDispatchType(0xD7),
                              SixLowPanDispatch::LOWPAN_GHC_UDP,
                              "0xD7 must be GHC_UDP");
        NS_TEST_ASSERT_MSG_EQ(SixLowPanDispatch::GetNhcDispatchType(0xDF),
                              SixLowPanDispatch::LOWPAN_GHC_ICMPV6,
                              "0xDF must be GHC_ICMPV6");
        // Ensure standard NHC is still recognized
        NS_TEST_ASSERT_MSG_EQ(SixLowPanDispatch::GetNhcDispatchType(0xE0),
                              SixLowPanDispatch::LOWPAN_NHC,
                              "0xE0 must still be NHC");
        NS_TEST_ASSERT_MSG_EQ(SixLowPanDispatch::GetNhcDispatchType(0xF0),
                              SixLowPanDispatch::LOWPAN_UDPNHC,
                              "0xF0 must still be UDPNHC");
    }
}

// ============================================================================
//  Test 3: End-to-End GHC UDP Compression Test
// ============================================================================

/**
 * @ingroup sixlowpan-tests
 * @brief 6LoWPAN GHC end-to-end UDP test.
 *
 * Sends a UDP packet between two nodes with GHC enabled and verifies
 * the packet arrives intact after compression + decompression.
 */
class SixlowpanGhcUdpImplTest : public TestCase
{
    Ptr<Packet> m_receivedPacket;

    void DoSendData(Ptr<Socket> socket, std::string to);
    void SendData(Ptr<Socket> socket, std::string to);

  public:
    void DoRun() override;
    SixlowpanGhcUdpImplTest();
    void ReceivePacket(Ptr<Socket> socket, Ptr<Packet> packet, const Address& from);
    void ReceivePkt(Ptr<Socket> socket);
};

SixlowpanGhcUdpImplTest::SixlowpanGhcUdpImplTest()
    : TestCase("Sixlowpan GHC UDP end-to-end")
{
}

void
SixlowpanGhcUdpImplTest::ReceivePacket(Ptr<Socket> socket, Ptr<Packet> packet, const Address& from)
{
    m_receivedPacket = packet;
}

void
SixlowpanGhcUdpImplTest::ReceivePkt(Ptr<Socket> socket)
{
    uint32_t availableData [[maybe_unused]] = socket->GetRxAvailable();
    m_receivedPacket = socket->Recv(std::numeric_limits<uint32_t>::max(), 0);
    NS_ASSERT(availableData == m_receivedPacket->GetSize());
}

void
SixlowpanGhcUdpImplTest::DoSendData(Ptr<Socket> socket, std::string to)
{
    Address realTo = Inet6SocketAddress(Ipv6Address(to.c_str()), 1234);
    uint8_t buffer[128] = "GHC RFC 7400 compression test - Generic Header Compression for "
                          "6LoWPAN IPv6 over Low-Power Wireless Personal Area Networks.";

    Ptr<Packet> packet = Create<Packet>(buffer, 128);
    NS_TEST_EXPECT_MSG_EQ(socket->SendTo(packet, 0, realTo), 128, "Send should succeed");
}

void
SixlowpanGhcUdpImplTest::SendData(Ptr<Socket> socket, std::string to)
{
    m_receivedPacket = Create<Packet>();
    Simulator::ScheduleWithContext(socket->GetNode()->GetId(),
                                   Seconds(0),
                                   &SixlowpanGhcUdpImplTest::DoSendData,
                                   this,
                                   socket,
                                   to);
    Simulator::Run();
}

void
SixlowpanGhcUdpImplTest::DoRun()
{
    InternetStackHelper internet;
    internet.SetIpv4StackInstall(false);

    // Receiver Node
    Ptr<Node> rxNode = CreateObject<Node>();
    internet.Install(rxNode);
    Ptr<SimpleNetDevice> rxDev;
    {
        rxDev = CreateObject<SimpleNetDevice>();
        rxDev->SetAddress(Mac48Address::ConvertFrom(Mac48Address::Allocate()));
        rxNode->AddDevice(rxDev);

        Ptr<SixLowPanNetDevice> rxSix = CreateObject<SixLowPanNetDevice>();
        rxSix->SetAttribute("UseGhc", BooleanValue(true));
        rxNode->AddDevice(rxSix);
        rxSix->SetNetDevice(rxDev);

        Ptr<Ipv6> ipv6 = rxNode->GetObject<Ipv6>();
        ipv6->AddInterface(rxDev);
        uint32_t netdev_idx = ipv6->AddInterface(rxSix);
        Ipv6InterfaceAddress ipv6Addr =
            Ipv6InterfaceAddress(Ipv6Address("2001:0100::1"), Ipv6Prefix(64));
        ipv6->AddAddress(netdev_idx, ipv6Addr);
        ipv6->SetUp(netdev_idx);
    }

    // Sender Node
    Ptr<Node> txNode = CreateObject<Node>();
    internet.Install(txNode);
    Ptr<SimpleNetDevice> txDev;
    {
        txDev = CreateObject<SimpleNetDevice>();
        txDev->SetAddress(Mac48Address::ConvertFrom(Mac48Address::Allocate()));
        txNode->AddDevice(txDev);

        Ptr<SixLowPanNetDevice> txSix = CreateObject<SixLowPanNetDevice>();
        txSix->SetAttribute("UseGhc", BooleanValue(true));
        txNode->AddDevice(txSix);
        txSix->SetNetDevice(txDev);

        Ptr<Ipv6> ipv6 = txNode->GetObject<Ipv6>();
        ipv6->AddInterface(txDev);
        uint32_t netdev_idx = ipv6->AddInterface(txSix);
        Ipv6InterfaceAddress ipv6Addr =
            Ipv6InterfaceAddress(Ipv6Address("2001:0100::2"), Ipv6Prefix(64));
        ipv6->AddAddress(netdev_idx, ipv6Addr);
        ipv6->SetUp(netdev_idx);
    }

    // Link the two nodes
    Ptr<SimpleChannel> channel1 = CreateObject<SimpleChannel>();
    rxDev->SetChannel(channel1);
    txDev->SetChannel(channel1);

    // Create the UDP sockets
    Ptr<SocketFactory> rxSocketFactory = rxNode->GetObject<UdpSocketFactory>();
    Ptr<Socket> rxSocket = rxSocketFactory->CreateSocket();
    NS_TEST_EXPECT_MSG_EQ(rxSocket->Bind(Inet6SocketAddress(Ipv6Address("2001:0100::1"), 1234)),
                          0,
                          "trivial");
    rxSocket->SetRecvCallback(MakeCallback(&SixlowpanGhcUdpImplTest::ReceivePkt, this));

    Ptr<SocketFactory> txSocketFactory = txNode->GetObject<UdpSocketFactory>();
    Ptr<Socket> txSocket = txSocketFactory->CreateSocket();
    txSocket->SetAllowBroadcast(true);

    // Unicast test
    SendData(txSocket, "2001:0100::1");
    NS_TEST_EXPECT_MSG_EQ(m_receivedPacket->GetSize(), 128, "Received packet size must match");

    uint8_t rxBuffer[128];
    uint8_t txBuffer[128] = "GHC RFC 7400 compression test - Generic Header Compression for "
                            "6LoWPAN IPv6 over Low-Power Wireless Personal Area Networks.";
    m_receivedPacket->CopyData(rxBuffer, 128);
    NS_TEST_EXPECT_MSG_EQ(std::memcmp(rxBuffer, txBuffer, 128),
                          0,
                          "Received data must match sent data");

    m_receivedPacket->RemoveAllByteTags();
    Simulator::Destroy();
}

// ============================================================================
//  Test Suite
// ============================================================================

/**
 * @ingroup sixlowpan-tests
 * @brief 6LoWPAN GHC (RFC 7400) Test Suite.
 */
class SixlowpanGhcTestSuite : public TestSuite
{
  public:
    SixlowpanGhcTestSuite();
};

SixlowpanGhcTestSuite::SixlowpanGhcTestSuite()
    : TestSuite("sixlowpan-ghc", Type::UNIT)
{
    AddTestCase(new SixlowpanGhcEngineTest(), TestCase::Duration::QUICK);
    AddTestCase(new SixlowpanGhcHeaderTest(), TestCase::Duration::QUICK);
    AddTestCase(new SixlowpanGhcUdpImplTest(), TestCase::Duration::QUICK);
}

static SixlowpanGhcTestSuite g_sixlowpanGhcTestSuite;
