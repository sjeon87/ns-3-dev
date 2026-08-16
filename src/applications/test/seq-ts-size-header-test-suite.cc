/*
 * Copyright (c) 2025 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/application-container.h"
#include "ns3/application-helper.h"
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/inet-socket-address.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/log.h"
#include "ns3/node-container.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/seq-ts-size-header.h"
#include "ns3/simple-net-device-helper.h"
#include "ns3/sink-application.h"
#include "ns3/source-application.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/traced-callback.h"
#include "ns3/uinteger.h"

#include <string>
#include <tuple>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SeqTsSizeTest");

/**
 * @ingroup applications-test
 * @ingroup tests
 *
 * This test checks that the sequence number is sent and received in sequence
 * despite the sending application having to pause and restart its sending
 * due to a temporarily full transmit buffer.
 */
class SeqTsSizeTestCase : public TestCase
{
  public:
    /// Test parameters
    struct Params
    {
        ObjectFactory sourceAppFactory;     //!< Source application factory
        std::string sinkAppName;            //!< Sink application name
        std::string protocol;               //!< Protocol to use
        uint32_t tcpSegmentSize{0};         //!< TCP Segment Size
        bool useRandomPayload{false};       //!< Flag to use random payload, all zeroes if false
        bool enableTxSeqTsSizeHeader{true}; //!< Enable SeqTsSizeHeader on Tx
        bool enableRxSeqTsSizeHeader{true}; //!< Enable SeqTsSizeHeader on Rx
        uint32_t expectedPayloadSize{0};    //!< Payload size in bytes
    };

    /**
     * @brief Constructor
     * @param params the test parameters
     */
    explicit SeqTsSizeTestCase(const Params& params);
    ~SeqTsSizeTestCase() override = default;

  private:
    void DoSetup() override;
    void DoRun() override;

    /**
     * Record a packet successfully sent
     * @param p the packet
     */
    void SendTx(Ptr<const Packet> p);

    /**
     * Record a packet successfully sent with SeqTsSizeHeader
     * @param p the packet
     * @param from source address
     * @param to destination address
     * @param header the SeqTsSizeHeader
     */
    void SendTxWithSeqTsSize(Ptr<const Packet> p,
                             const Address& from,
                             const Address& to,
                             const SeqTsSizeHeader& header);

    /**
     * Record a packet successfully sent with SeqTsHeader
     * @param p the packet
     * @param from source address
     * @param to destination address
     * @param header the SeqTsHeader
     */
    void SendTxWithSeqTs(Ptr<const Packet> p,
                         const Address& from,
                         const Address& to,
                         const SeqTsHeader& header);

    /**
     * Record a packet successfully received
     * @param p the packet
     * @param from source address
     */
    void ReceiveRx(Ptr<const Packet> p, const Address& from);

    /**
     * Record a packet successfully received with SeqTsSizeHeader
     * @param p the packet
     * @param from source address
     * @param to destination address
     * @param header the SeqTsSizeHeader
     */
    void ReceiveRxWithSeqTsSize(Ptr<const Packet> p,
                                const Address& from,
                                const Address& to,
                                const SeqTsSizeHeader& header);
    /**
     * Record a packet successfully received with SeqTsHeader
     * @param p the packet
     * @param from source address
     * @param to destination address
     * @param header the SeqTsHeader
     */
    void ReceiveRxWithSeqTs(Ptr<const Packet> p,
                            const Address& from,
                            const Address& to,
                            const SeqTsHeader& header);

    Params m_params;                       //!< Test parameters
    bool m_expectTxSeqTsSizeHeader{false}; //!< Whether to expect SeqTsSizeHeader on Tx
    bool m_expectRxSeqTsSizeHeader{false}; //!< Whether to expect SeqTsSizeHeader on Rx
    bool m_expectTxSeqTsHeader{false};     //!< Whether to expect SeqTsHeader on Tx
    bool m_expectRxSeqTsHeader{false};     //!< Whether to expect SeqTsHeader on Rx

    /// Information about a transmitted or received packet
    struct PacketInfo
    {
        uint64_t size{0}; //!< Packet size
        Time timestamp;   //!< Packet timestamp
    };

    std::vector<PacketInfo> m_sent;         //!< transmitted packets
    std::vector<PacketInfo> m_received;     //!< received packets
    uint64_t m_sentSeqTsSizeHeaders{0};     //!< number of SeqTsSizeHeaders sent
    uint64_t m_receivedSeqTsSizeHeaders{0}; //!< number of SeqTsSizeHeaders received
    uint64_t m_sentSeqTsHeaders{0};         //!< number of SeqTsHeaders sent
    uint64_t m_receivedSeqTsHeaders{0};     //!< number of SeqTsHeaders received
    uint64_t m_seqTxCounter{0};             //!< Counter for Sequences on Tx
    uint64_t m_seqRxCounter{0};             //!< Counter for Sequences on Rx
};

SeqTsSizeTestCase::SeqTsSizeTestCase(const Params& params)
    : TestCase(
          "Check SeqTsSizeHeader between " + params.sourceAppFactory.GetTypeId().GetName() +
          " and " + params.sinkAppName + " using protocol " + params.protocol +
          (params.protocol == "ns3::TcpSocketFactory"
               ? (" with Segment Size " + std::to_string(params.tcpSegmentSize))
               : "") +
          " with SeqTsSizeHeader " + (params.enableTxSeqTsSizeHeader ? "enabled" : "disabled") +
          " on Tx and " + (params.enableRxSeqTsSizeHeader ? "enabled" : "disabled") +
          " on Rx (random payload " + (params.useRandomPayload ? "enabled" : "disabled") + ")"),
      m_params{params},
      m_expectTxSeqTsSizeHeader{m_params.enableTxSeqTsSizeHeader &&
                                (m_params.protocol == "ns3::TcpSocketFactory")},
      m_expectRxSeqTsSizeHeader{m_expectTxSeqTsSizeHeader && m_params.enableRxSeqTsSizeHeader},
      m_expectTxSeqTsHeader{m_params.enableTxSeqTsSizeHeader &&
                            (m_params.protocol == "ns3::UdpSocketFactory")},
      m_expectRxSeqTsHeader{m_expectTxSeqTsHeader && m_params.enableRxSeqTsSizeHeader}
{
}

void
SeqTsSizeTestCase::SendTx(Ptr<const Packet> p)
{
    const auto size = p->GetSize();
    const auto now = Simulator::Now();
    NS_LOG_FUNCTION(this << p << size);
    m_sent.emplace_back(size, now);
    if (m_expectTxSeqTsSizeHeader)
    {
        SeqTsSizeHeader hdr;
        if (size < hdr.GetSerializedSize())
        {
            m_expectTxSeqTsSizeHeader = false;
            m_expectRxSeqTsSizeHeader = false;
            return;
        }
        p->PeekHeader(hdr);
        NS_TEST_EXPECT_MSG_EQ(hdr.GetSize(),
                              size,
                              "Size in SeqTsSizeHeader should match packet size in sent packet");
        NS_TEST_EXPECT_MSG_EQ(
            hdr.GetSeq(),
            m_sent.size() - 1,
            "Sequence number in SeqTsSizeHeader should match expected sequence number");
        NS_TEST_EXPECT_MSG_EQ(
            hdr.GetTs(),
            now,
            "Timestamp in SeqTsSizeHeader should match time at which packet is sent");
    }
    if (m_expectTxSeqTsHeader)
    {
        SeqTsHeader hdr;
        if (p->GetSize() < hdr.GetSerializedSize())
        {
            m_expectTxSeqTsHeader = false;
            m_expectRxSeqTsHeader = false;
            return;
        }
        p->PeekHeader(hdr);
        NS_TEST_EXPECT_MSG_EQ(
            hdr.GetSeq(),
            m_sent.size() - 1,
            "Sequence number in SeqTsSizeHeader should match expected sequence number");
        NS_TEST_EXPECT_MSG_EQ(
            hdr.GetTs(),
            now,
            "Timestamp in SeqTsSizeHeader should match time at which packet is sent");
    }
}

void
SeqTsSizeTestCase::SendTxWithSeqTsSize(Ptr<const Packet> p,
                                       const Address& from,
                                       const Address& to,
                                       const SeqTsSizeHeader& header)
{
    NS_LOG_FUNCTION(this << p << from << to << header);
    NS_TEST_EXPECT_MSG_EQ(
        header.GetSize(),
        p->GetSize() + header.GetSerializedSize(),
        "SeqTsSizeHeader should not be added in sent packet when using trace with header");
    NS_TEST_EXPECT_MSG_EQ(header.GetSeq(), m_seqTxCounter++, "Incorrect sequence number");
    NS_TEST_EXPECT_MSG_EQ(header.GetTs(), Simulator::Now(), "Incorrect timestamp");
    m_sentSeqTsSizeHeaders++;
}

void
SeqTsSizeTestCase::SendTxWithSeqTs(Ptr<const Packet> p,
                                   const Address& from,
                                   const Address& to,
                                   const SeqTsHeader& header)
{
    NS_LOG_FUNCTION(this << p << from << to << header);
    NS_TEST_EXPECT_MSG_EQ(
        p->GetSize() + header.GetSerializedSize(),
        m_params.expectedPayloadSize,
        "SeqTsHeader should not be added in sent packet when using trace with header");
    NS_TEST_EXPECT_MSG_EQ(header.GetSeq(), m_seqTxCounter++, "Incorrect sequence number");
    NS_TEST_EXPECT_MSG_GT_OR_EQ(header.GetTs(), Simulator::Now(), "Incorrect timestamp");
    m_sentSeqTsHeaders++;
}

void
SeqTsSizeTestCase::ReceiveRx(Ptr<const Packet> p, const Address& from)
{
    const auto size = p->GetSize();
    NS_LOG_FUNCTION(this << p << size);
    m_received.emplace_back(size, Simulator::Now());
}

void
SeqTsSizeTestCase::ReceiveRxWithSeqTsSize(Ptr<const Packet> p,
                                          const Address& from,
                                          const Address& to,
                                          const SeqTsSizeHeader& header)
{
    NS_LOG_FUNCTION(this << p << from << to << header);
    NS_TEST_EXPECT_MSG_EQ(
        header.GetSize(),
        p->GetSize() + header.GetSerializedSize(),
        "SeqTsSizeHeader should not be present in received packet when using trace with header");
    NS_TEST_EXPECT_MSG_EQ(header.GetSeq(), m_seqRxCounter++, "Incorrect sequence number");
    NS_TEST_EXPECT_MSG_EQ(header.GetSize(),
                          m_sent.at(header.GetSeq()).size,
                          "Incorrect size information");
    NS_TEST_EXPECT_MSG_EQ(header.GetTs(),
                          m_sent.at(header.GetSeq()).timestamp,
                          "Incorrect timestamp");
    m_receivedSeqTsSizeHeaders++;
}

void
SeqTsSizeTestCase::ReceiveRxWithSeqTs(Ptr<const Packet> p,
                                      const Address& from,
                                      const Address& to,
                                      const SeqTsHeader& header)
{
    NS_LOG_FUNCTION(this << p << from << to << header);
    NS_TEST_EXPECT_MSG_EQ(
        p->GetSize() + header.GetSerializedSize(),
        m_params.expectedPayloadSize,
        "SeqTsHeader should not be present in received packet when using trace with header");
    NS_TEST_EXPECT_MSG_EQ(header.GetSeq(), m_seqRxCounter++, "Incorrect sequence number");
    NS_TEST_EXPECT_MSG_EQ(header.GetTs(),
                          m_sent.at(header.GetSeq()).timestamp,
                          "Incorrect timestamp");
    m_receivedSeqTsHeaders++;
}

void
SeqTsSizeTestCase::DoSetup()
{
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(m_params.tcpSegmentSize));

    Packet::EnableChecking();

    const uint16_t port{9};
    const auto duration{Seconds(1)};
    const auto offset{Seconds(1)};

    auto sender = CreateObject<Node>();
    auto receiver = CreateObject<Node>();

    NodeContainer nodes;
    nodes.Add(sender);
    nodes.Add(receiver);

    SimpleNetDeviceHelper simpleHelper;
    simpleHelper.SetNetDevicePointToPointMode(true);
    auto devices = simpleHelper.Install(nodes);

    InternetStackHelper internet;
    internet.Install(nodes);

    Ipv4AddressHelper ipv4Helper;
    ipv4Helper.SetBase("10.1.1.0", "255.255.255.0");
    auto interfaces = ipv4Helper.Assign(devices);

    ApplicationHelper sourceHelper(m_params.sourceAppFactory);
    auto remoteAddress = InetSocketAddress(interfaces.GetAddress(1), port);
    sourceHelper.SetAttribute("Remote", AddressValue(remoteAddress));
    {
        TypeId::AttributeInformation sourceProtocolInfo;
        if (m_params.sourceAppFactory.GetTypeId().LookupAttributeByName("Protocol",
                                                                        &sourceProtocolInfo,
                                                                        true))
        {
            sourceHelper.SetAttribute("Protocol", StringValue(m_params.protocol));
        }
    }
    sourceHelper.SetAttribute("RandomPayload", BooleanValue(m_params.useRandomPayload));
    sourceHelper.SetAttribute("EnableSeqTsSizeHeader",
                              BooleanValue(m_params.enableTxSeqTsSizeHeader));
    auto sourceApp = sourceHelper.Install(nodes.Get(0));
    sourceApp.Start(offset);
    sourceApp.Stop(duration + offset);

    ApplicationHelper sinkHelper(m_params.sinkAppName);
    {
        const auto sinkTid = TypeId::LookupByName(m_params.sinkAppName);
        TypeId::AttributeInformation sinkProtocolInfo;
        if (sinkTid.LookupAttributeByName("Protocol", &sinkProtocolInfo, true))
        {
            sinkHelper.SetAttribute("Protocol", StringValue(m_params.protocol));
        }
    }
    sinkHelper.SetAttribute("Port", UintegerValue(port));
    sinkHelper.SetAttribute("EnableSeqTsSizeHeader",
                            BooleanValue(m_params.enableRxSeqTsSizeHeader));
    auto sinkApp = sinkHelper.Install(nodes.Get(1));
    sinkApp.Start(Seconds(0));
    sinkApp.Stop(duration + offset);

    auto source = DynamicCast<SourceApplication>(sourceApp.Get(0));
    auto sink = DynamicCast<SinkApplication>(sinkApp.Get(0));

    source->TraceConnectWithoutContext("Tx", MakeCallback(&SeqTsSizeTestCase::SendTx, this));
    source->TraceConnectWithoutContext("TxWithSeqTsSize",
                                       MakeCallback(&SeqTsSizeTestCase::SendTxWithSeqTsSize, this));
    source->TraceConnectWithoutContext("TxWithSeqTs",
                                       MakeCallback(&SeqTsSizeTestCase::SendTxWithSeqTs, this));

    sink->TraceConnectWithoutContext("Rx", MakeCallback(&SeqTsSizeTestCase::ReceiveRx, this));
    sink->TraceConnectWithoutContext(
        "RxWithSeqTsSize",
        MakeCallback(&SeqTsSizeTestCase::ReceiveRxWithSeqTsSize, this));
    sink->TraceConnectWithoutContext("RxWithSeqTs",
                                     MakeCallback(&SeqTsSizeTestCase::ReceiveRxWithSeqTs, this));
}

void
SeqTsSizeTestCase::DoRun()
{
    Simulator::Run();
    Simulator::Destroy();

    const uint64_t sentBytes =
        std::accumulate(m_sent.cbegin(),
                        m_sent.cend(),
                        uint64_t{0},
                        [](uint64_t sum, const PacketInfo& info) { return sum + info.size; });
    const uint64_t receivedBytes =
        std::accumulate(m_received.cbegin(),
                        m_received.cend(),
                        uint64_t{0},
                        [](uint64_t sum, const PacketInfo& info) { return sum + info.size; });
    NS_TEST_EXPECT_MSG_GT(sentBytes, 0, "No bytes were sent");
    NS_TEST_EXPECT_MSG_GT(receivedBytes, 0, "No bytes were received");
    NS_TEST_EXPECT_MSG_EQ(sentBytes, receivedBytes, "Sent and received byte counts differ");

    if (m_expectTxSeqTsSizeHeader)
    {
        NS_TEST_EXPECT_MSG_GT(m_sentSeqTsSizeHeaders, 0, "No SeqTsSizeHeaders were generated");
    }
    else
    {
        NS_TEST_EXPECT_MSG_EQ(m_sentSeqTsSizeHeaders,
                              0,
                              "SeqTsSizeHeaders were generated whereas they were not expected");
    }

    if (m_expectRxSeqTsSizeHeader)
    {
        NS_TEST_EXPECT_MSG_GT(m_receivedSeqTsSizeHeaders, 0, "No SeqTsSizeHeaders were processed");
    }
    else
    {
        NS_TEST_EXPECT_MSG_EQ(m_receivedSeqTsSizeHeaders,
                              0,
                              "SeqTsSizeHeaders were processed whereas they were not expected");
    }

    if (m_expectTxSeqTsSizeHeader && m_expectRxSeqTsSizeHeader)
    {
        NS_TEST_EXPECT_MSG_EQ(m_sentSeqTsSizeHeaders,
                              m_receivedSeqTsSizeHeaders,
                              "Sent and received SeqTsSizeHeader counts differ");
    }

    if (m_expectTxSeqTsHeader)
    {
        NS_TEST_EXPECT_MSG_GT(m_sentSeqTsHeaders, 0, "No SeqTsHeaders were generated");
    }
    else
    {
        NS_TEST_EXPECT_MSG_EQ(m_sentSeqTsHeaders,
                              0,
                              "SeqTsHeaders were generated whereas they were not expected");
    }

    if (m_expectRxSeqTsHeader)
    {
        NS_TEST_EXPECT_MSG_GT(m_receivedSeqTsHeaders, 0, "No SeqTsHeaders were processed");
    }
    else
    {
        NS_TEST_EXPECT_MSG_EQ(m_receivedSeqTsHeaders,
                              0,
                              "SeqTsHeaders were processed whereas they were not expected");
    }

    if (m_expectTxSeqTsHeader && m_expectRxSeqTsHeader)
    {
        NS_TEST_EXPECT_MSG_EQ(m_sentSeqTsHeaders,
                              m_receivedSeqTsHeaders,
                              "Sent and received SeqTsHeader counts differ");
    }
}

/**
 * @ingroup applications-test
 * @ingroup tests
 *
 * @brief SeqTsSizeHeader TestSuite
 */
class SeqTsSizeHeaderTestSuite : public TestSuite
{
  public:
    SeqTsSizeHeaderTestSuite();
};

SeqTsSizeHeaderTestSuite::SeqTsSizeHeaderTestSuite()
    : TestSuite("applications-seq-ts-size-header", Type::UNIT)
{
    const uint32_t numPackets{100};
    const uint32_t largePayloadSize{512}; // larger than the header size
    const uint32_t smallPayloadSize{10};  // smaller than the header size
    ObjectFactory onOffFactoryLarge("ns3::OnOffApplication",
                                    "OnTime",
                                    StringValue("ns3::ConstantRandomVariable[Constant=1.0]"),
                                    "OffTime",
                                    StringValue("ns3::ConstantRandomVariable[Constant=0.0]"),
                                    "PacketSize",
                                    UintegerValue(largePayloadSize),
                                    "MaxBytes",
                                    UintegerValue(numPackets * largePayloadSize));
    ObjectFactory onOffFactorySmall("ns3::OnOffApplication",
                                    "OnTime",
                                    StringValue("ns3::ConstantRandomVariable[Constant=1.0]"),
                                    "OffTime",
                                    StringValue("ns3::ConstantRandomVariable[Constant=0.0]"),
                                    "PacketSize",
                                    UintegerValue(smallPayloadSize),
                                    "MaxBytes",
                                    UintegerValue(numPackets * smallPayloadSize));
    ObjectFactory bulkSendFactoryLarge("ns3::BulkSendApplication",
                                       "SendSize",
                                       UintegerValue(largePayloadSize),
                                       "MaxBytes",
                                       UintegerValue(numPackets * largePayloadSize));
    ObjectFactory bulkSendFactorySmall("ns3::BulkSendApplication",
                                       "SendSize",
                                       UintegerValue(smallPayloadSize),
                                       "MaxBytes",
                                       UintegerValue(numPackets * smallPayloadSize));
    const uint32_t udpClientPacketSize{128};
    ObjectFactory udpClientFactory("ns3::UdpClient",
                                   "MaxPackets",
                                   UintegerValue(numPackets),
                                   "Interval",
                                   TimeValue(MilliSeconds(1)),
                                   "PacketSize",
                                   UintegerValue(udpClientPacketSize));

    for (const auto& [sourceAppFactory, payloadSize] :
         std::initializer_list<std::pair<ObjectFactory, uint32_t>>{
             {onOffFactoryLarge, largePayloadSize},
             {onOffFactorySmall, smallPayloadSize},
             {bulkSendFactoryLarge, largePayloadSize},
             {bulkSendFactorySmall, smallPayloadSize}})
    {
        for (const auto& sinkAppName : {"ns3::PacketSink"})
        {
            for (const auto& protocol :
                 {std::string("ns3::TcpSocketFactory"), std::string("ns3::UdpSocketFactory")})
            {
                if ((sourceAppFactory.GetTypeId().GetName() == "ns3::BulkSendApplication") &&
                    (protocol == "ns3::UdpSocketFactory"))
                {
                    // BulkSendApplication over UDP is not supported
                    continue;
                }
                for (const auto tcpSegmentSize :
                     (protocol == "ns3::TcpSocketFactory")
                         ? std::initializer_list<uint32_t>{payloadSize * 2,
                                                           payloadSize,
                                                           payloadSize / 2}
                         : std::initializer_list<uint32_t>{0})
                {
                    for (const auto randomPayload : {true, false})
                    {
                        for (const auto enableTxHdr : {true, false})
                        {
                            for (const auto enableRxHdr : {true, false})
                            {
                                AddTestCase(
                                    new SeqTsSizeTestCase({.sourceAppFactory = sourceAppFactory,
                                                           .sinkAppName = sinkAppName,
                                                           .protocol = protocol,
                                                           .tcpSegmentSize = tcpSegmentSize,
                                                           .useRandomPayload = randomPayload,
                                                           .enableTxSeqTsSizeHeader = enableTxHdr,
                                                           .enableRxSeqTsSizeHeader = enableRxHdr,
                                                           .expectedPayloadSize = payloadSize}),
                                    TestCase::Duration::QUICK);
                            }
                        }
                    }
                }
            }
        }
    }

    for (const auto randomPayload : {true, false})
    {
        for (const auto enableTxHdr : {true, false})
        {
            for (const auto enableRxHdr : {true, false})
            {
                AddTestCase(new SeqTsSizeTestCase({.sourceAppFactory = udpClientFactory,
                                                   .sinkAppName = "ns3::UdpServer",
                                                   .protocol = "ns3::UdpSocketFactory",
                                                   .useRandomPayload = randomPayload,
                                                   .enableTxSeqTsSizeHeader = enableTxHdr,
                                                   .enableRxSeqTsSizeHeader = enableRxHdr,
                                                   .expectedPayloadSize = udpClientPacketSize}),
                            TestCase::Duration::QUICK);
            }
        }
    }
}

static SeqTsSizeHeaderTestSuite
    g_seqTsSizeHeaderTestSuite; //!< Static variable for test initialization
