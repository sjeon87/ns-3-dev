/*
 * Copyright (c) 2020 Tom Henderson (tomh@tomh.org)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */

#include "ns3/application-container.h"
#include "ns3/boolean.h"
#include "ns3/bulk-send-application.h"
#include "ns3/bulk-send-helper.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/ipv6-address-helper.h"
#include "ns3/ipv6-address.h"
#include "ns3/ipv6-interface-container.h"
#include "ns3/log.h"
#include "ns3/node-container.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/pointer.h"
#include "ns3/simple-net-device-helper.h"
#include "ns3/string.h"
#include "ns3/tcp-bbr.h"
#include "ns3/tcp-congestion-ops.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/tcp-prr-recovery.h"
#include "ns3/tcp-recovery-ops.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/test.h"
#include "ns3/traced-callback.h"
#include "ns3/uinteger.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("BulkSendApplicationTest");

/**
 * @ingroup applications-test
 * @ingroup tests
 *
 * Basic test, checks that the right quantity of packets are sent and received.
 */
class BulkSendBasicTestCase : public TestCase
{
  public:
    BulkSendBasicTestCase();
    ~BulkSendBasicTestCase() override;

  private:
    void DoRun() override;
    /**
     * Record a packet successfully sent
     * @param p the packet
     */
    void SendTx(Ptr<const Packet> p);
    /**
     * Record a packet successfully received
     * @param p the packet
     * @param addr the sender's address
     */
    void ReceiveRx(Ptr<const Packet> p, const Address& addr);
    uint64_t m_sent{0};     //!< number of bytes sent
    uint64_t m_received{0}; //!< number of bytes received
};

BulkSendBasicTestCase::BulkSendBasicTestCase()
    : TestCase("Check a basic 300KB transfer")
{
}

BulkSendBasicTestCase::~BulkSendBasicTestCase()
{
}

void
BulkSendBasicTestCase::SendTx(Ptr<const Packet> p)
{
    m_sent += p->GetSize();
}

void
BulkSendBasicTestCase::ReceiveRx(Ptr<const Packet> p, const Address& addr)
{
    m_received += p->GetSize();
}

void
BulkSendBasicTestCase::DoRun()
{
    Ptr<Node> sender = CreateObject<Node>();
    Ptr<Node> receiver = CreateObject<Node>();
    NodeContainer nodes;
    nodes.Add(sender);
    nodes.Add(receiver);
    SimpleNetDeviceHelper simpleHelper;
    simpleHelper.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    simpleHelper.SetChannelAttribute("Delay", StringValue("10ms"));
    NetDeviceContainer devices;
    devices = simpleHelper.Install(nodes);
    InternetStackHelper internet;
    internet.Install(nodes);
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i = ipv4.Assign(devices);
    uint16_t port = 9;
    BulkSendHelper sourceHelper("ns3::TcpSocketFactory", InetSocketAddress(i.GetAddress(1), port));
    sourceHelper.SetAttribute("MaxBytes", UintegerValue(300000));
    ApplicationContainer sourceApp = sourceHelper.Install(nodes.Get(0));
    sourceApp.Start(Seconds(0));
    sourceApp.Stop(Seconds(10));
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApp = sinkHelper.Install(nodes.Get(1));
    sinkApp.Start(Seconds(0));
    sinkApp.Stop(Seconds(10));

    Ptr<BulkSendApplication> source = DynamicCast<BulkSendApplication>(sourceApp.Get(0));
    Ptr<PacketSink> sink = DynamicCast<PacketSink>(sinkApp.Get(0));

    source->TraceConnectWithoutContext("Tx", MakeCallback(&BulkSendBasicTestCase::SendTx, this));
    sink->TraceConnectWithoutContext("Rx", MakeCallback(&BulkSendBasicTestCase::ReceiveRx, this));

    Simulator::Run();
    Simulator::Destroy();

    NS_TEST_ASSERT_MSG_EQ(m_sent, 300000, "Sent the full 300000 bytes");
    NS_TEST_ASSERT_MSG_EQ(m_received, 300000, "Received the full 300000 bytes");
}

/**
 * @ingroup applications-test
 * @ingroup tests
 *
 * This test checks that the sequence number is sent and received in sequence
 * despite the sending application having to pause and restart its sending
 * due to a temporarily full transmit buffer.
 */
class BulkSendSeqTsSizeTestCase : public TestCase
{
  public:
    BulkSendSeqTsSizeTestCase();
    ~BulkSendSeqTsSizeTestCase() override;

  private:
    void DoRun() override;
    /**
     * Record a packet successfully sent
     * @param p the packet
     * @param from source address
     * @param to destination address
     * @param header the SeqTsSizeHeader
     */
    void SendTx(Ptr<const Packet> p,
                const Address& from,
                const Address& to,
                const SeqTsSizeHeader& header);
    /**
     * Record a packet successfully received
     * @param p the packet
     * @param from source address
     * @param to destination address
     * @param header the SeqTsSizeHeader
     */
    void ReceiveRx(Ptr<const Packet> p,
                   const Address& from,
                   const Address& to,
                   const SeqTsSizeHeader& header);
    uint64_t m_sent{0};          //!< number of bytes sent
    uint64_t m_received{0};      //!< number of bytes received
    uint64_t m_seqTxCounter{0};  //!< Counter for Sequences on Tx
    uint64_t m_seqRxCounter{0};  //!< Counter for Sequences on Rx
    Time m_lastTxTs{Seconds(0)}; //!< Last recorded timestamp on Tx
    Time m_lastRxTs{Seconds(0)}; //!< Last recorded timestamp on Rx
};

BulkSendSeqTsSizeTestCase::BulkSendSeqTsSizeTestCase()
    : TestCase("Check a 300KB transfer with SeqTsSize header enabled")
{
}

BulkSendSeqTsSizeTestCase::~BulkSendSeqTsSizeTestCase()
{
}

void
BulkSendSeqTsSizeTestCase::SendTx(Ptr<const Packet> p,
                                  const Address& from,
                                  const Address& to,
                                  const SeqTsSizeHeader& header)
{
    // The header is not serialized onto the packet in this trace
    m_sent += p->GetSize() + header.GetSerializedSize();
    NS_TEST_ASSERT_MSG_EQ(header.GetSeq(), m_seqTxCounter, "Missing sequence number");
    m_seqTxCounter++;
    NS_TEST_ASSERT_MSG_GT_OR_EQ(header.GetTs(), m_lastTxTs, "Timestamp less than last time");
    m_lastTxTs = header.GetTs();
}

void
BulkSendSeqTsSizeTestCase::ReceiveRx(Ptr<const Packet> p,
                                     const Address& from,
                                     const Address& to,
                                     const SeqTsSizeHeader& header)
{
    // The header is not serialized onto the packet in this trace
    m_received += p->GetSize() + header.GetSerializedSize();
    NS_TEST_ASSERT_MSG_EQ(header.GetSeq(), m_seqRxCounter, "Missing sequence number");
    m_seqRxCounter++;
    NS_TEST_ASSERT_MSG_GT_OR_EQ(header.GetTs(), m_lastRxTs, "Timestamp less than last time");
    m_lastRxTs = header.GetTs();
}

void
BulkSendSeqTsSizeTestCase::DoRun()
{
    Ptr<Node> sender = CreateObject<Node>();
    Ptr<Node> receiver = CreateObject<Node>();
    NodeContainer nodes;
    nodes.Add(sender);
    nodes.Add(receiver);
    SimpleNetDeviceHelper simpleHelper;
    simpleHelper.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    simpleHelper.SetChannelAttribute("Delay", StringValue("10ms"));
    NetDeviceContainer devices;
    devices = simpleHelper.Install(nodes);
    InternetStackHelper internet;
    internet.Install(nodes);
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i = ipv4.Assign(devices);
    uint16_t port = 9;
    BulkSendHelper sourceHelper("ns3::TcpSocketFactory", InetSocketAddress(i.GetAddress(1), port));
    sourceHelper.SetAttribute("MaxBytes", UintegerValue(300000));
    sourceHelper.SetAttribute("EnableSeqTsSizeHeader", BooleanValue(true));
    ApplicationContainer sourceApp = sourceHelper.Install(nodes.Get(0));
    sourceApp.Start(Seconds(0));
    sourceApp.Stop(Seconds(10));
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), port));
    sinkHelper.SetAttribute("EnableSeqTsSizeHeader", BooleanValue(true));
    ApplicationContainer sinkApp = sinkHelper.Install(nodes.Get(1));
    sinkApp.Start(Seconds(0));
    sinkApp.Stop(Seconds(10));

    Ptr<BulkSendApplication> source = DynamicCast<BulkSendApplication>(sourceApp.Get(0));
    Ptr<PacketSink> sink = DynamicCast<PacketSink>(sinkApp.Get(0));

    source->TraceConnectWithoutContext("TxWithSeqTsSize",
                                       MakeCallback(&BulkSendSeqTsSizeTestCase::SendTx, this));
    sink->TraceConnectWithoutContext("RxWithSeqTsSize",
                                     MakeCallback(&BulkSendSeqTsSizeTestCase::ReceiveRx, this));

    Simulator::Run();
    Simulator::Destroy();

    NS_TEST_ASSERT_MSG_EQ(m_sent, 300000, "Sent the full 300000 bytes");
    NS_TEST_ASSERT_MSG_EQ(m_received, 300000, "Received the full 300000 bytes");
}

/**
 * @ingroup applications-test
 * @ingroup tests
 *
 * @brief Tests that BulkSendApplication and PacketSink can be given custom
 * sockets via the SetSocket() / SetPrimarySocket() / SetDualStackSocket()
 * API, and that the provided socket's congestion control and recovery algorithms
 * are preserved, and that TcpL4Protocol's default settings are not used.
 *
 * Two BulkSendApplication flows run simultaneously on the same sender node,
 * using different TCP congestion control algorithms.
 *
 * The test demonstrates two ways a caller can create a socket with a specific
 * congestion control algorithm before passing it into an application.
 *
 * There are three cases to test, each of which uses a different possible
 * configuration of PacketSink:
 *
 * Case LOCAL_UNSET: the Local attribute is unset on PacketSink, causing the
 * sink to open both an IPv4 and an IPv6 listening socket (dual-stack mode).
 * One PacketSink is used with two custom sockets, set using the
 * SetPrimarySocket() and SetDualStackSocket() methods.
 *
 * Case INET_SOCKET_ADDR: two separate PacketSink objects, one bound via
 * InetSocketAddress(explicit IPv4 address, explicit port) and one via
 * Inet6SocketAddress(explicit IPv6 address, explicit port), are used.
 * Each sink uses SetPrimarySocket() only; the dual-stack socket is unused
 * when the address is configured on the sink application.
 *
 * Case INET_SOCKET_ADDR_ANY: similar to the previous case except that
 * InetSocketAddress(any IPv4 address, explicit port) and
 * Inet6SocketAddress(any IPv6 address, explicit port) are instead used.
 */
class BulkSendCustomSocketTestCase : public TestCase
{
  public:
    /// Configuration of the Local attribute for PacketSink
    enum class LocalConfig
    {
        LOCAL_UNSET,          ///< "Local" attribute unset (one sink, two listening sockets)
        INET_SOCKET_ADDR,     ///< InetSocketAddress (explicit IPv4) and Inet6SocketAddress(explicit
                              ///< IPv6) combination with explicit port
        INET_SOCKET_ADDR_ANY, ///< InetSocketAddress(any IPv4) and Inet6SocketAddress(any IPv6)
                              ///< combination with explicit port
    };

    /**
     * Constructor
     * @param config the local address configuration to test
     */
    BulkSendCustomSocketTestCase(LocalConfig config);

  private:
    void DoRun() override;
    /**
     * Trace packet successfully sent
     * @param p the packet
     */
    void SendTx(Ptr<const Packet> p);
    /**
     * Trace packet successfully received
     * @param p the packet
     * @param addr the sender's address
     */
    void ReceiveRx(Ptr<const Packet> p, const Address& addr);
    /**
     * Configure the sink application(s) based on m_config and store them in
     * m_sink1 and m_sink2.  For LOCAL_UNSET, m_sink1 is the single dual-stack
     * sink and m_sink2 is null.  For all other cases, m_sink1 is the IPv4 sink
     * and m_sink2 is the IPv6 sink.
     * @param nodes the node container (nodes.Get(1) is the receiver)
     * @param sinkSock1 custom socket for the IPv4 (or primary) sink
     * @param sinkSock2 custom socket for the IPv6 (or dual-stack) sink
     * @param port the port to listen on
     * @param i4 IPv4 interface container
     * @param i6 IPv6 interface container
     */
    void ConfigureSinks(NodeContainer nodes,
                        Ptr<Socket> sinkSock1,
                        Ptr<Socket> sinkSock2,
                        uint16_t port,
                        Ipv4InterfaceContainer i4,
                        Ipv6InterfaceContainer i6);
    /**
     * Translate the LocalConfig enum values to strings
     * @param config the LocalConfig value
     * @return test name string
     */
    static std::string GetTestName(LocalConfig config);

    LocalConfig m_config;    //!< LocalConfig for this test case
    Ptr<PacketSink> m_sink1; //!< Primary (or dual-stack) sink
    Ptr<PacketSink> m_sink2; //!< Secondary IPv6 sink (null for LOCAL_UNSET)
    uint32_t m_sent{0};      //!< number of bytes sent (both flows combined)
    uint32_t m_received{0};  //!< number of bytes received (both flows combined)
};

BulkSendCustomSocketTestCase::BulkSendCustomSocketTestCase(LocalConfig config)
    : TestCase(GetTestName(config)),
      m_config(config)
{
}

void
BulkSendCustomSocketTestCase::SendTx(Ptr<const Packet> p)
{
    NS_LOG_DEBUG("SendTx received " << p->GetSize());
    m_sent += p->GetSize();
}

void
BulkSendCustomSocketTestCase::ReceiveRx(Ptr<const Packet> p, const Address& addr)
{
    NS_LOG_DEBUG("ReceiveRx received " << p->GetSize());
    m_received += p->GetSize();
}

std::string
BulkSendCustomSocketTestCase::GetTestName(LocalConfig config)
{
    switch (config)
    {
    case LocalConfig::LOCAL_UNSET:
        return "Check custom socket with dual-stack sink ('Local' attribute unset)";
    case LocalConfig::INET_SOCKET_ADDR:
        return "Check custom socket with InetSocketAddress/Inet6SocketAddress explicit address "
               "sinks";
    case LocalConfig::INET_SOCKET_ADDR_ANY:
        return "Check custom socket with InetSocketAddress/Inet6SocketAddress any-address sinks";
    default:
        return "Unknown LocalConfig";
    }
}

void
BulkSendCustomSocketTestCase::ConfigureSinks(NodeContainer nodes,
                                             Ptr<Socket> sinkSock1,
                                             Ptr<Socket> sinkSock2,
                                             uint16_t port,
                                             Ipv4InterfaceContainer i4,
                                             Ipv6InterfaceContainer i6)
{
    switch (m_config)
    {
    case LocalConfig::LOCAL_UNSET: {
        auto sink = CreateObject<PacketSink>();
        sink->SetAttribute("Port", UintegerValue(port));
        sink->SetPrimarySocket(sinkSock1);
        sink->SetDualStackSocket(sinkSock2);
        nodes.Get(1)->AddApplication(sink);
        m_sink1 = sink;
        m_sink2 = nullptr;
        break;
    }
    case LocalConfig::INET_SOCKET_ADDR: {
        auto sink1 = CreateObject<PacketSink>();
        sink1->SetAttribute("Local", AddressValue(InetSocketAddress(i4.GetAddress(1), port)));
        sink1->SetPrimarySocket(sinkSock1);
        nodes.Get(1)->AddApplication(sink1);

        auto sink2 = CreateObject<PacketSink>();
        sink2->SetAttribute("Local", AddressValue(Inet6SocketAddress(i6.GetAddress(1, 1), port)));
        sink2->SetPrimarySocket(sinkSock2);
        nodes.Get(1)->AddApplication(sink2);

        m_sink1 = sink1;
        m_sink2 = sink2;
        break;
    }
    case LocalConfig::INET_SOCKET_ADDR_ANY: {
        auto sink1 = CreateObject<PacketSink>();
        sink1->SetAttribute("Local", AddressValue(InetSocketAddress(Ipv4Address::GetAny(), port)));
        sink1->SetPrimarySocket(sinkSock1);
        nodes.Get(1)->AddApplication(sink1);

        auto sink2 = CreateObject<PacketSink>();
        sink2->SetAttribute("Local", AddressValue(Inet6SocketAddress(Ipv6Address::GetAny(), port)));
        sink2->SetPrimarySocket(sinkSock2);
        nodes.Get(1)->AddApplication(sink2);

        m_sink1 = sink1;
        m_sink2 = sink2;
        break;
    }
    }
}

void
BulkSendCustomSocketTestCase::DoRun()
{
    // Network topology: two nodes connected by a SimpleNetDevice link
    Ptr<Node> sender = CreateObject<Node>();
    Ptr<Node> receiver = CreateObject<Node>();
    NodeContainer nodes;
    nodes.Add(sender);
    nodes.Add(receiver);
    SimpleNetDeviceHelper simpleHelper;
    simpleHelper.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    simpleHelper.SetChannelAttribute("Delay", StringValue("1ms"));
    NetDeviceContainer devices = simpleHelper.Install(nodes);
    InternetStackHelper internet;
    internet.Install(nodes);
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i4 = ipv4.Assign(devices);
    Ipv6AddressHelper ipv6;
    ipv6.SetBase(Ipv6Address("2001:1::"), Ipv6Prefix(64));
    Ipv6InterfaceContainer i6 = ipv6.Assign(devices);

    // Method 1: create a default TCP socket via Socket::CreateSocket(), then
    // override the congestion ops algorithm via SetCongestionControlAlgorithm().  The
    // recovery algorithm remains the node default (TcpPrrRecovery).
    Ptr<Socket> sock1 = Socket::CreateSocket(sender, TcpSocketFactory::GetTypeId());
    DynamicCast<TcpSocketBase>(sock1)->SetCongestionControlAlgorithm(CreateObject<TcpBbr>());

    // Method 2: obtain TcpL4Protocol from the node and call CreateSocket() with
    // both a congestion ops TypeId and a recovery TypeId, enabling a non-default recovery
    // algorithm (TcpClassicRecovery instead of the node-default TcpPrrRecovery).
    auto tcpTx = sender->GetObject<TcpL4Protocol>();
    Ptr<Socket> sock2 =
        tcpTx->CreateSocket(TcpNewReno::GetTypeId(), TcpClassicRecovery::GetTypeId());

    // Sink sockets: created via TcpL4Protocol with specific TcpCongestionOps algorithms.
    auto tcpRx = receiver->GetObject<TcpL4Protocol>();
    Ptr<Socket> sinkSock1 = tcpRx->CreateSocket(TcpNewReno::GetTypeId()); // IPv4 listener
    Ptr<Socket> sinkSock2 = tcpRx->CreateSocket(TcpBbr::GetTypeId());     // IPv6 listener

    uint16_t port = 9;

    // Sender application 1: BulkSend over IPv4 using TcpBbr (socket method 1)
    auto app1 = CreateObject<BulkSendApplication>();
    app1->SetAttribute("Remote", AddressValue(InetSocketAddress(i4.GetAddress(1), port)));
    app1->SetAttribute("MaxBytes", UintegerValue(5000));
    app1->SetSocket(sock1);
    nodes.Get(0)->AddApplication(app1);

    // Sender application 2: BulkSend over IPv6 using TcpNewReno (socket method 2)
    auto app2 = CreateObject<BulkSendApplication>();
    app2->SetAttribute("Remote", AddressValue(Inet6SocketAddress(i6.GetAddress(1, 1), port)));
    app2->SetAttribute("MaxBytes", UintegerValue(5000));
    app2->SetSocket(sock2);
    nodes.Get(0)->AddApplication(app2);

    // Configure sink(s) based on m_config
    ConfigureSinks(nodes, sinkSock1, sinkSock2, port, i4, i6);

    // Configure start and stop times
    app1->SetStartTime(Seconds(0));
    app1->SetStopTime(Seconds(5));
    app2->SetStartTime(Seconds(0));
    app2->SetStopTime(Seconds(5));
    m_sink1->SetStartTime(Seconds(0));
    m_sink1->SetStopTime(Seconds(5));
    if (m_sink2)
    {
        m_sink2->SetStartTime(Seconds(0));
        m_sink2->SetStopTime(Seconds(5));
    }

    app1->TraceConnectWithoutContext("Tx",
                                     MakeCallback(&BulkSendCustomSocketTestCase::SendTx, this));
    app2->TraceConnectWithoutContext("Tx",
                                     MakeCallback(&BulkSendCustomSocketTestCase::SendTx, this));
    m_sink1->TraceConnectWithoutContext(
        "Rx",
        MakeCallback(&BulkSendCustomSocketTestCase::ReceiveRx, this));
    if (m_sink2)
    {
        m_sink2->TraceConnectWithoutContext(
            "Rx",
            MakeCallback(&BulkSendCustomSocketTestCase::ReceiveRx, this));
    }

    Simulator::Stop(Seconds(6));
    Simulator::Run();

    auto tcpSock1 = DynamicCast<TcpSocketBase>(app1->GetSocket());
    auto tcpSock2 = DynamicCast<TcpSocketBase>(app2->GetSocket());

    NS_TEST_ASSERT_MSG_EQ(m_sent, 10000, "Sent 5000 bytes per flow (2 flows)");
    NS_TEST_ASSERT_MSG_EQ(m_received, 10000, "Received 5000 bytes per flow (2 flows)");

    NS_TEST_ASSERT_MSG_NE(tcpSock1, nullptr, "Sender socket 1 must be a TcpSocketBase");
    NS_TEST_ASSERT_MSG_NE(tcpSock2, nullptr, "Sender socket 2 must be a TcpSocketBase");

    PointerValue pv;

    tcpSock1->GetAttribute("CongestionOps", pv);
    NS_TEST_ASSERT_MSG_EQ(pv.Get<TcpCongestionOps>()->GetInstanceTypeId(),
                          TcpBbr::GetTypeId(),
                          "Sender socket 1 must use TcpBbr");

    tcpSock1->GetAttribute("RecoveryOps", pv);
    NS_TEST_ASSERT_MSG_EQ(pv.Get<TcpRecoveryOps>()->GetInstanceTypeId(),
                          TcpPrrRecovery::GetTypeId(),
                          "Sender socket 1 must use TcpPrrRecovery (node default)");

    tcpSock2->GetAttribute("CongestionOps", pv);
    NS_TEST_ASSERT_MSG_EQ(pv.Get<TcpCongestionOps>()->GetInstanceTypeId(),
                          TcpNewReno::GetTypeId(),
                          "Sender socket 2 must use TcpNewReno");

    tcpSock2->GetAttribute("RecoveryOps", pv);
    NS_TEST_ASSERT_MSG_EQ(pv.Get<TcpRecoveryOps>()->GetInstanceTypeId(),
                          TcpClassicRecovery::GetTypeId(),
                          "Sender socket 2 must use TcpClassicRecovery");

    if (m_config == LocalConfig::LOCAL_UNSET)
    {
        auto sinkTcpSock1 = DynamicCast<TcpSocketBase>(m_sink1->GetPrimarySocket());
        auto sinkTcpSock2 = DynamicCast<TcpSocketBase>(m_sink1->GetDualStackSocket());

        NS_TEST_ASSERT_MSG_NE(sinkTcpSock1, nullptr, "Sink primary socket must be a TcpSocketBase");
        NS_TEST_ASSERT_MSG_NE(sinkTcpSock2,
                              nullptr,
                              "Sink dual-stack socket must be a TcpSocketBase");

        sinkTcpSock1->GetAttribute("CongestionOps", pv);
        NS_TEST_ASSERT_MSG_EQ(pv.Get<TcpCongestionOps>()->GetInstanceTypeId(),
                              TcpNewReno::GetTypeId(),
                              "Sink primary socket must use TcpNewReno");

        sinkTcpSock2->GetAttribute("CongestionOps", pv);
        NS_TEST_ASSERT_MSG_EQ(pv.Get<TcpCongestionOps>()->GetInstanceTypeId(),
                              TcpBbr::GetTypeId(),
                              "Sink dual-stack socket must use TcpBbr");
    }
    else
    {
        auto sinkTcpSock1 = DynamicCast<TcpSocketBase>(m_sink1->GetPrimarySocket());
        // Here, note the change from the previous case-- GetDualStackSocket() is not used
        auto sinkTcpSock2 = DynamicCast<TcpSocketBase>(m_sink2->GetPrimarySocket());

        NS_TEST_ASSERT_MSG_NE(sinkTcpSock1,
                              nullptr,
                              "Sink 1 primary socket must be a TcpSocketBase");
        NS_TEST_ASSERT_MSG_NE(sinkTcpSock2,
                              nullptr,
                              "Sink 2 primary socket must be a TcpSocketBase");

        NS_TEST_ASSERT_MSG_EQ(m_sink1->GetDualStackSocket(),
                              nullptr,
                              "Sink 1 dual-stack socket must be null");
        NS_TEST_ASSERT_MSG_EQ(m_sink2->GetDualStackSocket(),
                              nullptr,
                              "Sink 2 dual-stack socket must be null");

        sinkTcpSock1->GetAttribute("CongestionOps", pv);
        NS_TEST_ASSERT_MSG_EQ(pv.Get<TcpCongestionOps>()->GetInstanceTypeId(),
                              TcpNewReno::GetTypeId(),
                              "Sink 1 primary socket must use TcpNewReno");

        sinkTcpSock2->GetAttribute("CongestionOps", pv);
        NS_TEST_ASSERT_MSG_EQ(pv.Get<TcpCongestionOps>()->GetInstanceTypeId(),
                              TcpBbr::GetTypeId(),
                              "Sink 2 primary socket must use TcpBbr");
    }

    Simulator::Destroy();
}

/**
 * @ingroup applications-test
 * @ingroup tests
 *
 * @brief BulkSend TestSuite
 */
class BulkSendTestSuite : public TestSuite
{
  public:
    BulkSendTestSuite();
};

BulkSendTestSuite::BulkSendTestSuite()
    : TestSuite("applications-bulk-send", Type::UNIT)
{
    AddTestCase(new BulkSendBasicTestCase, TestCase::Duration::QUICK);
    AddTestCase(new BulkSendSeqTsSizeTestCase, TestCase::Duration::QUICK);
    AddTestCase(
        new BulkSendCustomSocketTestCase(BulkSendCustomSocketTestCase::LocalConfig::LOCAL_UNSET),
        TestCase::Duration::QUICK);
    AddTestCase(new BulkSendCustomSocketTestCase(
                    BulkSendCustomSocketTestCase::LocalConfig::INET_SOCKET_ADDR),
                TestCase::Duration::QUICK);
    AddTestCase(new BulkSendCustomSocketTestCase(
                    BulkSendCustomSocketTestCase::LocalConfig::INET_SOCKET_ADDR_ANY),
                TestCase::Duration::QUICK);
}

static BulkSendTestSuite g_bulkSendTestSuite; //!< Static variable for test initialization
