/*
 * Copyright (c) 2016 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Wenying Dai         <daiwenying927@gmail.com>
 *          N Nagabhushanam     <thechosentwins2005@gmail.com>
 *          Namburi Yaswanth    <yaswanthnamburi1010@gmail.com>
 *          Chinta Gnana Jyothi <gnanajyothi06@gmail.com>
 *          Prathapa Hasitha    <prathapahasitha@gmail.com>
 *          Mohit P. Tahiliani  <tahiliani@nitk.edu.in>
 *          Deepak Kumaraswamy  <deepak.kumaraswamy@gmail.com>
 *
 */
#include "../model/ipv4-end-point.h"
#include "../model/ipv6-end-point.h"
#include "tcp-error-model.h"
#include "tcp-general-test.h"

#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-interface-address.h"
#include "ns3/ipv4-route.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4.h"
#include "ns3/ipv6-route.h"
#include "ns3/ipv6-routing-protocol.h"
#include "ns3/ipv6.h"
#include "ns3/log.h"
#include "ns3/loopback-net-device.h"
#include "ns3/node.h"
#include "ns3/pointer.h"
#include "ns3/rtt-estimator.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/tcp-rx-buffer.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/tcp-tx-buffer.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TcpEcnPpTest");

// ECN++ test scenario identifiers
/**
 * @ingroup internet-test
 * @ingroup tests
 * @enum TcpEcnPpCase
 * @brief ECN++ test scenario identifiers used by TcpEcnPpTest.
 *
 * The enumeration covers negotiation-mode combinations, SYN/ACK marking and
 * response, FIN/RST behavior, and per-packet marking tests (SYN, Window Probe,
 * retransmissions, and ACK).
 */
enum TcpEcnPpCase
{
    // Negotiation-mode combinations
    EcnPpSender_NoEcnReceiver = 1,      //!< Sender ECN++ vs Receiver No ECN
    EcnPpSender_ClassicEcnReceiver = 2, //!< Sender ECN++ vs Receiver Classic ECN
    NoEcnSender_EcnPpReceiver = 3,      //!< Sender No ECN vs Receiver ECN++
    ClassicEcnSender_EcnPpReceiver = 4, //!< Sender Classic ECN vs Receiver ECN++
    EcnPpSender_EcnPpReceiver = 5,      //!< Sender ECN++ vs Receiver ECN++

    // SYN/ACK behavior
    SYNACK_CE_CLASSIC_SENDER = 6, //!< Classic sender, ECN++ receiver, CE on SYN/ACK
    SYNACK_CE_ECNPP_BOTH = 7,     //!< ECN++ sender and receiver, CE on SYN/ACK
    SYNACK_ECT_MARKING = 10,      //!< SYN-ACK ECT marking test
    SYNACK_CE_RESPONSE = 11,      //!< SYN-ACK CE response test
    SYNACK_FALLBACK = 12,         //!< SYN-ACK ECN fallback test

    // FIN and RST behavior
    FIN_MARKING = 8,      //!< FIN packet ECT marking test
    FIN_CE_RESPONSE = 9,  //!< FIN packet CE response test (ignore CE)
    RST_MARKING = 16,     //!< RST packet ECT marking test
    RST_CE_RESPONSE = 17, //!< RST packet CE response test (ignore CE)

    // Per-packet marking tests
    SYN_ECT_MARKING = 13,      //!< SYN packet ECT marking test
    WINDOW_PROBE_MARKING = 14, //!< Window Probe packet marking test
    RETX_MARKING = 15,         //!< Retransmission packet marking test
    ACK_MARKING = 18           //!< ACK packet ECT marking test
};

/**
 * @ingroup internet-test
 * @ingroup tests
 *
 * @brief A TCP socket which sends certain data packets with CE flags.
 */
class TcpEcnPpPacketBase : public TcpSocketMsgBase
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    uint32_t m_dataPacketSent; //!< Number of packets sent
    uint8_t m_testcase;        //!< Test Case type

    TcpEcnPpPacketBase()
        : TcpSocketMsgBase()
    {
        m_dataPacketSent = 0;
        m_controlPacketSent = 0;
    }

    /**
     * @brief Trigger transmission of a TCP RST packet.
     *
     * This helper function is used in ECN++ test cases to
     * explicitly generate a TCP reset from the socket.
     */
    void TriggerRst();

    /**
     * @brief Copy constructor.
     *
     * @param other The packet instance to copy from.
     */
    TcpEcnPpPacketBase(const TcpEcnPpPacketBase& other)
        : TcpSocketMsgBase(other)
    {
    }

    /**
     * @brief Identifies the socket role in ECN++ tests.
     */
    enum SocketWho
    {
        SENDER,  //!< Sender node
        RECEIVER //!< Receiver node
    };

    /**
     * @brief Configure the ECN++ test case and socket role.
     *
     * This method sets the ECN++ test case identifier along with
     * the role of the socket (sender or receiver). The behavior
     * of the socket during the test (e.g., CE marking, control
     * packet handling) depends on both the test case and the
     * socket role.
     *
     * @param testCase Test case type.
     * @param who Role of the socket in the test (SENDER or RECEIVER).
     */
    void SetTestCase(uint8_t testCase, SocketWho who);

  protected:
    uint32_t SendDataPacket(SequenceNumber32 seq, uint32_t maxSize, bool withAck) override;
    void ReTxTimeout() override;
    Ptr<TcpSocketBase> Fork() override;

    /**
     * @brief Mark a packet with the CE (Congestion Experienced) codepoint.
     *
     * Applies CE marking to the provided packet as required
     * by the ECN++ test case.
     *
     * @param p Packet to be marked with CE.
     */
    void SetCE(Ptr<Packet> p);

  private:
    uint32_t m_controlPacketSent; //!< Number of control packets sent
    SocketWho m_who;              //!< Role of this socket in the test (SENDER or RECEIVER)
};

NS_OBJECT_ENSURE_REGISTERED(TcpEcnPpPacketBase);

TypeId
TcpEcnPpPacketBase::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TcpEcnPpPacketBase")
                            .SetParent<TcpSocketMsgBase>()
                            .SetGroupName("Internet")
                            .AddConstructor<TcpEcnPpPacketBase>();
    return tid;
}

void
TcpEcnPpPacketBase::ReTxTimeout()
{
    TcpSocketBase::ReTxTimeout();
}

void
TcpEcnPpPacketBase::TriggerRst()
{
    SendRST();
}

void
TcpEcnPpPacketBase::SetTestCase(uint8_t testCase, SocketWho who)
{
    m_testcase = testCase;
    m_who = who;
}

uint32_t
TcpEcnPpPacketBase::SendDataPacket(SequenceNumber32 seq, uint32_t maxSize, bool withAck)
{
    if (m_testcase == WINDOW_PROBE_MARKING && m_dataPacketSent == 10)
    {
        TcpSocketBase::PersistTimeout();
    }
    m_dataPacketSent++;
    return TcpSocketBase::SendDataPacket(seq, maxSize, false);
}

Ptr<TcpSocketBase>
TcpEcnPpPacketBase::Fork()
{
    return CopyObject<TcpEcnPpPacketBase>(this);
}

void
TcpEcnPpPacketBase::SetCE(Ptr<Packet> p)
{
    SocketIpTosTag ipTosTag;
    ipTosTag.SetTos(MarkEcnCe(0));
    p->ReplacePacketTag(ipTosTag);

    SocketIpv6TclassTag ipTclassTag;
    ipTclassTag.SetTclass(MarkEcnCe(0));
    p->ReplacePacketTag(ipTclassTag);
}

/**
 * @ingroup internet-test
 * @ingroup tests
 *
 * @brief checks if ECT (in IP header), CWR and ECE (in TCP header) bits are set correctly in
 * different packet types.
 *
 * This test suite will run five combinations of EcnPp to different Ecn Modes.
 * case 1: SENDER EcnPp       RECEIVER NoEcn
 * case 2: SENDER EcnPp       RECEIVER ClassicEcn
 * case 3: SENDER NoEcn       RECEIVER EcnPp
 * case 4: SENDER ClassicEcn  RECEIVER EcnPp
 * case 5: SENDER EcnPp       RECEIVER EcnPp
 * The first five cases are trying to test the following things:
 * 1. ECT, CWR and ECE setting correctness in SYN, SYN/ACK and ACK in negotiation phases
 * 2. ECT setting correctness for W probe, FIN packet and RST packet
 * 3. Congestion response when W probe suffers congestion, if we treat W probe as a special data
 * packet
 *
 * To test the congestion response for SYN/ACK packet, case 6 and 7 are constructed.
 * case 6: SENDER ClassicEcn  RECEIVER EcnPp
 * case 7: SENDER EcnPp       RECEIVER EcnPp
 * case 6 should ignore the CE mark in SYN/ACK when the sender receives this packet.
 * case 7 will feed back this information (ECE) from sender to receiver and corresponding process.
 *
 * The following test cases cover marking and CE response details:
 * case 8: FIN packet ECT marking.
 * case 9: FIN packet CE response (CE is ignored).
 * case 10: SYN-ACK ECT marking.
 * case 11: SYN-ACK CE response.
 * case 12: SYN-ACK ECN fallback mechanism.
 * case 13: SYN packet ECT marking.
 * case 14: Window Probe packet marking.
 * case 15: Retransmission ECN marking (retransmits should preserve/propagate marking).
 * case 16: RST packet ECT marking.
 * case 17: RST packet CE response (CE is ignored).
 * case 18: ACK packet marking.
 *
 */
class TcpEcnPpTest : public TcpGeneralTest
{
  public:
    /**
     * @brief Constructor
     *
     * @param testcase test case type
     * @param desc Description about the ECN capabilities of sender and receiver
     */
    TcpEcnPpTest(uint32_t testcase, const std::string& desc);

  protected:
    void Rx(const Ptr<const Packet> p, const TcpHeader& h, SocketWho who) override;
    void Tx(const Ptr<const Packet> p, const TcpHeader& h, SocketWho who) override;
    Ptr<TcpSocketMsgBase> CreateSenderSocket(Ptr<Node> node) override;
    Ptr<TcpSocketMsgBase> CreateReceiverSocket(Ptr<Node> node) override;
    void ConfigureProperties() override;

    /**
     * @brief Callback invoked when a packet is dropped.
     *
     * This method is used to observe packet drops during the test and
     * inspect ECN-related fields carried in the IPv4 and TCP headers.
     *
     * @param ipH The IPv4 header of the dropped packet
     * @param tcpH The TCP header of the dropped packet
     * @param p The dropped packet
     */
    void PktDropped(const Ipv4Header& ipH, const TcpHeader& tcpH, Ptr<const Packet> p);
    Ptr<ErrorModel> CreateSenderErrorModel() override;
    Ptr<ErrorModel> CreateReceiverErrorModel() override;

    /**
     * @brief Trigger transmission of a TCP RST packet from the sender.
     *
     * Used in test cases that verify ECN behavior for RST packets
     * initiated by the sender.
     */
    void SendRstPacketSender();

    /**
     * @brief Trigger transmission of a TCP RST packet from the receiver.
     *
     * Used in test cases that verify ECN behavior for RST packets
     * initiated by the receiver.
     */
    void SendRstPacketReceiver();

  private:
    uint32_t m_testcase;         //!< ECN++ test case identifier
    uint32_t m_senderSent;       //!< Number of packets sent by sender
    uint32_t m_senderReceived;   //!< Number of packets received by sender
    uint32_t m_receiverSent;     //!< Number of packets sent by receiver
    uint32_t m_receiverReceived; //!< Number of packets received by receiver
    uint32_t m_prevEcnState;     //!< Track prev ECN state
    uint32_t m_isHandShakeDone;  //!< Is handshake done
    bool m_dataPacketDropped;    //!< Bool for dropping pkt
    uint32_t m_seqToDrop;        //!< Seq no used to drop pkt
    uint16_t m_synAckCount;      //!< Counter for SYN-ACK packets in SYNACK_FALLBACK test
};

TcpEcnPpTest::TcpEcnPpTest(uint32_t testcase, const std::string& desc)
    : TcpGeneralTest(desc),
      m_testcase(testcase),
      m_senderSent(0),
      m_senderReceived(0),
      m_receiverSent(0),
      m_receiverReceived(0)
{
    m_dataPacketDropped = false;
    m_seqToDrop = 501;
    m_prevEcnState = 0;
    m_isHandShakeDone = 0;
    m_synAckCount = 0;
}

void
TcpEcnPpTest::ConfigureProperties()
{
    TcpGeneralTest::ConfigureProperties();

    SetUseEcn(SENDER, TcpSocketState::Off);
    SetUseEcn(RECEIVER, TcpSocketState::Off);

    if (m_testcase == EcnPpSender_NoEcnReceiver || m_testcase == EcnPpSender_ClassicEcnReceiver ||
        m_testcase == EcnPpSender_EcnPpReceiver || m_testcase >= SYNACK_CE_ECNPP_BOTH)
    {
        SetUseEcn(SENDER, TcpSocketState::On);
        GetTcb(SENDER)->m_useEcnPlusPlus = true;
    }
    else if (m_testcase == ClassicEcnSender_EcnPpReceiver || m_testcase == SYNACK_CE_CLASSIC_SENDER)
    {
        SetUseEcn(SENDER, TcpSocketState::On);
        GetTcb(SENDER)->m_ecnMode = TcpSocketState::ClassicEcn;
    }

    if (m_testcase == NoEcnSender_EcnPpReceiver || m_testcase == ClassicEcnSender_EcnPpReceiver ||
        m_testcase == EcnPpSender_EcnPpReceiver || m_testcase == SYNACK_CE_CLASSIC_SENDER ||
        m_testcase >= SYNACK_CE_ECNPP_BOTH)
    {
        SetUseEcn(RECEIVER, TcpSocketState::On);
        GetTcb(RECEIVER)->m_useEcnPlusPlus = true;
    }
    else if (m_testcase == EcnPpSender_ClassicEcnReceiver)
    {
        SetUseEcn(RECEIVER, TcpSocketState::On);
        GetTcb(RECEIVER)->m_ecnMode = TcpSocketState::ClassicEcn;
    }

    if (m_testcase == RST_MARKING)
    {
        Simulator::Schedule(Seconds(1.25), &TcpEcnPpTest::SendRstPacketSender, this);
    }
    else if (m_testcase == RST_CE_RESPONSE)
    {
        Simulator::Schedule(Seconds(1.25), &TcpEcnPpTest::SendRstPacketReceiver, this);
    }
}

void
TcpEcnPpTest::SendRstPacketSender()
{
    Ptr<TcpEcnPpPacketBase> senderSocket = DynamicCast<TcpEcnPpPacketBase>(GetSenderSocket());
    if (senderSocket)
    {
        senderSocket->TriggerRst();
    }
    else
    {
        NS_LOG_ERROR("Sender socket not found — cannot send RST");
    }
}

void
TcpEcnPpTest::SendRstPacketReceiver()
{
    Ptr<TcpEcnPpPacketBase> receiverSocket = DynamicCast<TcpEcnPpPacketBase>(GetReceiverSocket());
    if (receiverSocket)
    {
        receiverSocket->TriggerRst();
    }
    else
    {
        NS_LOG_ERROR("Receiver socket not found — cannot send RST");
    }
}

Ptr<TcpSocketMsgBase>
TcpEcnPpTest::CreateSenderSocket(Ptr<Node> node)
{
    std::cout << "Running Test Case " << m_testcase << std::endl;
    Ptr<TcpEcnPpPacketBase> socket = DynamicCast<TcpEcnPpPacketBase>(
        CreateSocket(node, TcpEcnPpPacketBase::GetTypeId(), m_congControlTypeId));
    socket->SetTestCase(m_testcase, TcpEcnPpPacketBase::SENDER);
    return socket;
}

Ptr<TcpSocketMsgBase>
TcpEcnPpTest::CreateReceiverSocket(Ptr<Node> node)
{
    Ptr<TcpEcnPpPacketBase> socket = DynamicCast<TcpEcnPpPacketBase>(
        CreateSocket(node, TcpEcnPpPacketBase::GetTypeId(), m_congControlTypeId));
    socket->SetTestCase(m_testcase, TcpEcnPpPacketBase::RECEIVER);
    return socket;
}

Ptr<ErrorModel>
TcpEcnPpTest::CreateSenderErrorModel()
{
    if (m_testcase == SYNACK_FALLBACK)
    {
        // Drop the first two incoming SYN-ACKs at the sender
        Ptr<TcpFlagErrorModel> em = CreateObject<TcpFlagErrorModel>();
        em->SetFlagToKill(static_cast<TcpHeader::Flags_t>(TcpHeader::SYN | TcpHeader::ACK));
        em->SetKillRepeat(2);
        return em;
    }
    return nullptr;
}

Ptr<ErrorModel>
TcpEcnPpTest::CreateReceiverErrorModel()
{
    if (m_testcase == RETX_MARKING)
    {
        Ptr<TcpSeqErrorModel> errorModel = CreateObject<TcpSeqErrorModel>();
        errorModel->AddSeqToKill(SequenceNumber32(m_seqToDrop));
        errorModel->SetDropCallback(MakeCallback(&TcpEcnPpTest::PktDropped, this));
        return errorModel;
    }
    return nullptr;
}

void
TcpEcnPpTest::PktDropped(const Ipv4Header& ipH, const TcpHeader& tcpH, Ptr<const Packet> p)
{
    NS_LOG_DEBUG("Dropped!..." << tcpH);
}

void
TcpEcnPpTest::Rx(const Ptr<const Packet> p, const TcpHeader& h, SocketWho who)
{
    NS_LOG_FUNCTION(this << m_testcase << who);

    uint8_t flags = h.GetFlags();

    if (who == RECEIVER)
    {
        m_receiverReceived++;
        NS_LOG_DEBUG("Packet size is: " << p->GetSize() << " at packet: " << m_receiverReceived);
        if (m_receiverReceived == 1) // SYN for negotiation test in TCP header
        {
            NS_TEST_ASSERT_MSG_NE((flags & TcpHeader::SYN),
                                  0,
                                  "SYN should be received as first message at the receiver");
            if (m_testcase == EcnPpSender_NoEcnReceiver ||
                m_testcase == EcnPpSender_ClassicEcnReceiver ||
                m_testcase == ClassicEcnSender_EcnPpReceiver ||
                m_testcase == EcnPpSender_EcnPpReceiver)
            {
                NS_TEST_ASSERT_MSG_NE((flags & TcpHeader::ECE) && (flags & TcpHeader::CWR),
                                      0,
                                      "The flags ECE + CWR should be set in the TCP header of SYN "
                                      "at receiver when sender is ECN Capable");
            }
            else if (m_testcase == NoEcnSender_EcnPpReceiver)
            {
                NS_TEST_ASSERT_MSG_EQ((flags & TcpHeader::ECE) || (flags & TcpHeader::CWR),
                                      0,
                                      "The flags ECE + CWR should not be set in the TCP header of "
                                      "SYN at receiver when sender is not ECN Capable");
            }
        }

        if (m_receiverReceived == 2 &&
            m_testcase <= SYNACK_CE_ECNPP_BOTH) // ACK for negotiation test in TCP header
        {
            NS_TEST_ASSERT_MSG_NE((flags & TcpHeader::ACK),
                                  0,
                                  "ACK should be received as second message at receiver");
            if (m_testcase == EcnPpSender_NoEcnReceiver ||
                m_testcase == EcnPpSender_ClassicEcnReceiver ||
                m_testcase == NoEcnSender_EcnPpReceiver ||
                m_testcase == ClassicEcnSender_EcnPpReceiver ||
                m_testcase == EcnPpSender_EcnPpReceiver)
            {
                NS_TEST_ASSERT_MSG_EQ((flags & TcpHeader::ECE),
                                      0,
                                      "ECE should not be set if the SYN/ACK not CE in any cases");
            }

            // test if ACK with ECE in TCP header when received SYN/ACK with CE mark
            if (m_testcase == SYNACK_CE_CLASSIC_SENDER)
            {
                NS_TEST_ASSERT_MSG_EQ((flags & TcpHeader::ECE),
                                      0,
                                      "ECE should not be set if the sender is classicECN when "
                                      "received SYN/ACK with CE mark");
            }
            else if (m_testcase == SYNACK_CE_ECNPP_BOTH)
            {
                NS_TEST_ASSERT_MSG_NE(
                    (flags & TcpHeader::ECE),
                    0,
                    "ECE should be set if the sender is EcnPp when received SYN/ACK with CE mark");
            }
        }

        if (h.GetFlags() & TcpHeader::CWR) // just for debug to find out which data packet carried
                                           // CWR because of previous ECE
        {
            NS_LOG_DEBUG("CWR at: " << m_receiverReceived);
        }

        if (p->GetSize() == 1 &&
            (m_testcase == EcnPpSender_NoEcnReceiver || m_testcase == NoEcnSender_EcnPpReceiver))
        {
            NS_TEST_ASSERT_MSG_EQ((flags & TcpHeader::CWR),
                                  0,
                                  "The flag ECE should not be set in the TCP header even if W "
                                  "probe get congestion");
        }
        if (flags == TcpHeader::FIN || flags == (TcpHeader::FIN | TcpHeader::ACK))
        {
            if (m_testcase == FIN_CE_RESPONSE)
            {
                // Now the state of ECN shouldn't change as ECN++ ignores CE of FIN pkt
                uint32_t currentState = GetTcb(SENDER)->m_ecnState;
                NS_TEST_ASSERT_MSG_EQ(
                    currentState,
                    m_prevEcnState,
                    "State of ECN shouldn't change as ECN++ ignores CE of FIN pkt");
            }
        }
        if (flags == TcpHeader::RST && m_testcase == RST_CE_RESPONSE)
        {
            uint32_t currentState = GetTcb(SENDER)->m_ecnState;
            NS_TEST_ASSERT_MSG_EQ(currentState,
                                  m_prevEcnState,
                                  "State of ECN shouldn't change as ECN++ ignores CE of RST pkt");
        }

        if (m_isHandShakeDone == 0)
        {
            bool isPureAck = ((flags & TcpHeader::ACK) &&
                              !(flags & (TcpHeader::SYN | TcpHeader::FIN | TcpHeader::RST)));
            if (isPureAck)
            {
                if (m_testcase == SYNACK_CE_RESPONSE)
                {
                    NS_TEST_ASSERT_MSG_EQ(
                        flags & TcpHeader::ECE,
                        TcpHeader::ECE,
                        "ECE flag should be set in pure ACK after handshake due to CE");
                }
                m_isHandShakeDone = 1;
            }
        }
    }

    if (who == SENDER)
    {
        m_senderReceived++;
        if (m_senderReceived == 1) // SYN/ACK for negotiation test in TCP header
        {
            NS_TEST_ASSERT_MSG_NE((flags & TcpHeader::SYN) && (flags & TcpHeader::ACK),
                                  0,
                                  "SYN+ACK received as first message at sender");
            if (m_testcase == EcnPpSender_ClassicEcnReceiver ||
                m_testcase == ClassicEcnSender_EcnPpReceiver ||
                m_testcase == EcnPpSender_EcnPpReceiver || m_testcase == SYNACK_CE_CLASSIC_SENDER ||
                m_testcase == SYNACK_CE_ECNPP_BOTH)
            {
                NS_TEST_ASSERT_MSG_NE((h.GetFlags() & TcpHeader::ECE),
                                      0,
                                      "The flag ECE should be set in the TCP header of SYN/ACK at "
                                      "sender when both receiver and sender are ECN Capable");
            }
            else if (m_testcase == EcnPpSender_NoEcnReceiver ||
                     m_testcase == NoEcnSender_EcnPpReceiver)
            {
                NS_TEST_ASSERT_MSG_EQ(
                    (flags & TcpHeader::ECE),
                    0,
                    "The flag ECE should not be set in the TCP header of SYN/ACK at sender when "
                    "either receiver or sender are not ECN Capable");
            }
        }

        if (h.GetAckNumber() == SequenceNumber32(3002)) // ACK for W probe
        {
            if (m_testcase == EcnPpSender_NoEcnReceiver || m_testcase == NoEcnSender_EcnPpReceiver)
            {
                NS_TEST_ASSERT_MSG_EQ((h.GetFlags() & TcpHeader::ECE),
                                      0,
                                      "The flag ECE should not be set in the TCP header even if W "
                                      "probe get congestion");
            }

            if (m_testcase == EcnPpSender_ClassicEcnReceiver ||
                m_testcase == EcnPpSender_EcnPpReceiver)
            {
                NS_TEST_ASSERT_MSG_EQ(
                    (flags & TcpHeader::CWR),
                    0,
                    "The flag ECE should be set in the TCP header if W probe get congestion");
            }
        }
    }
}

void
TcpEcnPpTest::Tx(const Ptr<const Packet> p, const TcpHeader& h, SocketWho who)
{
    NS_LOG_FUNCTION(this << m_testcase << who);
    SocketIpTosTag ipTosTag;
    bool found = p->PeekPacketTag(ipTosTag);
    uint16_t ipTos = 0;
    if (found)
    {
        ipTos = static_cast<uint16_t>(ipTosTag.GetTos() & 0x3);
    }
    uint8_t flags = h.GetFlags();
    if (who == SENDER)
    {
        m_senderSent++;
        if (m_senderSent == 1) // SYN for negotiation test in IP header
        {
            if (m_testcase == EcnPpSender_NoEcnReceiver ||
                m_testcase == EcnPpSender_ClassicEcnReceiver ||
                m_testcase == NoEcnSender_EcnPpReceiver ||
                m_testcase == ClassicEcnSender_EcnPpReceiver ||
                m_testcase == EcnPpSender_EcnPpReceiver || m_testcase == SYNACK_CE_CLASSIC_SENDER ||
                m_testcase == SYNACK_CE_ECNPP_BOTH)
            {
                NS_TEST_ASSERT_MSG_EQ(ipTos, 0x0, "IP TOS should not have ECT set in SYN");
            }
        }

        if (m_senderSent == 2) // ACK for negotiation test in IP header
        {
            if (m_testcase == EcnPpSender_NoEcnReceiver ||
                m_testcase == EcnPpSender_ClassicEcnReceiver ||
                m_testcase == NoEcnSender_EcnPpReceiver ||
                m_testcase == ClassicEcnSender_EcnPpReceiver ||
                m_testcase == EcnPpSender_EcnPpReceiver || m_testcase == SYNACK_CE_CLASSIC_SENDER ||
                m_testcase == SYNACK_CE_ECNPP_BOTH)
            {
                NS_TEST_ASSERT_MSG_EQ(ipTos, 0x0, "IP TOS should not have ECT set in pure ACK");
            }
        }

        if (p->GetSize() == 1) // W probe
        {
            NS_LOG_DEBUG("W probe being triggered " << m_senderSent);
            if (m_testcase == EcnPpSender_NoEcnReceiver || m_testcase == NoEcnSender_EcnPpReceiver)
            {
                NS_TEST_ASSERT_MSG_EQ(
                    ipTos,
                    0x0,
                    "IP TOS should not have ECT set in W probe if the sender is NoEcn");
            }
            if (m_testcase == EcnPpSender_ClassicEcnReceiver ||
                m_testcase == EcnPpSender_EcnPpReceiver)
            {
                NS_TEST_ASSERT_MSG_EQ(
                    ipTos,
                    0x2,
                    "IP TOS should have ECT set in W probe if the sender is ClassicEcn/EcnPp");

                SocketIpTosTag tosTag;
                uint8_t ceTos = (ipTos & 0xfc) | 0x03;
                tosTag.SetTos(ceTos);
                if (found)
                {
                    Ptr<Packet> mod = const_cast<Packet*>(PeekPointer(p));
                    mod->ReplacePacketTag(tosTag);
                }
                else
                {
                    p->AddPacketTag(tosTag);
                }
            }
        }

        if (flags & TcpHeader::FIN) // FIN
        {
            NS_LOG_DEBUG("Send out FIN packet");
            if (m_testcase == EcnPpSender_NoEcnReceiver || m_testcase == NoEcnSender_EcnPpReceiver)
            {
                NS_TEST_ASSERT_MSG_EQ(
                    ipTos,
                    0x0,
                    "IP TOS should not have ECT set in FIN if the sender is not EcnPp");
            }
            if (m_testcase == EcnPpSender_ClassicEcnReceiver ||
                m_testcase == EcnPpSender_EcnPpReceiver) // ECN++ will set ECT in FIN
            {
                NS_TEST_ASSERT_MSG_EQ(ipTos,
                                      0x2,
                                      "IP TOS should have ECT set in FIN if the sender is EcnPp");
            }
        }

        if (p->GetSize() == 1 && m_testcase == WINDOW_PROBE_MARKING)
        {
            NS_TEST_ASSERT_MSG_NE(ipTos, 0x0, "IP TOS should be set in Window Probe");
        }

        // Per packet tests
        if (m_testcase == FIN_MARKING)
        {
            if (flags == TcpHeader::FIN || flags == (TcpHeader::FIN | TcpHeader::ACK))
            {
                NS_TEST_ASSERT_MSG_EQ(ipTos, 0x2, "IP TOS should have ECT set for FIN pkts");
            }
        }
        // FIN or RST response
        if (((flags == TcpHeader::FIN || flags == (TcpHeader::FIN | TcpHeader::ACK)) &&
             m_testcase == FIN_CE_RESPONSE) ||
            (flags == TcpHeader::RST && m_testcase == RST_CE_RESPONSE))
        {
            SocketIpTosTag tosTag;
            uint8_t ceTos = (ipTos & 0xfc) | 0x03;
            tosTag.SetTos(ceTos);
            if (found)
            {
                Ptr<Packet> mod = const_cast<Packet*>(PeekPointer(p));
                mod->ReplacePacketTag(tosTag);
            }
            else
            {
                p->AddPacketTag(tosTag);
            }
            m_prevEcnState = GetTcb(SENDER)->m_ecnState;
        }
        if (m_testcase == SYN_ECT_MARKING)
        {
            if ((flags & (TcpHeader::SYN)) == (TcpHeader::SYN))
            {
                NS_TEST_ASSERT_MSG_EQ(ipTos, 0x0, "IP TOS shouldn't have ECT set for SYN packets");
            }
        }
        if (flags == TcpHeader::RST && m_testcase == RST_MARKING)
        {
            NS_TEST_ASSERT_MSG_EQ(ipTos, 0x2, "IP TOS should have ECT set for RST packets");
        }
    }

    if (who == RECEIVER)
    {
        m_receiverSent++;
        if (m_receiverSent == 1) // SYN/ACK for negotiation test
        {
            if (m_testcase == ClassicEcnSender_EcnPpReceiver ||
                m_testcase == EcnPpSender_EcnPpReceiver || m_testcase == SYNACK_CE_CLASSIC_SENDER ||
                m_testcase == SYNACK_CE_ECNPP_BOTH)
            {
                NS_TEST_ASSERT_MSG_EQ(ipTos, 0x2, "IP TOS should have ECT set in SYN/ACK");
            }
            else if (m_testcase == EcnPpSender_NoEcnReceiver ||
                     m_testcase == EcnPpSender_ClassicEcnReceiver ||
                     m_testcase == NoEcnSender_EcnPpReceiver)
            {
                NS_TEST_ASSERT_MSG_EQ(ipTos, 0x0, "IP TOS should not have ECT set in SYN/ACK");
            }
        }
        // CE mark in SYN/ACK
        if ((m_testcase == SYNACK_CE_CLASSIC_SENDER || m_testcase == SYNACK_CE_ECNPP_BOTH) &&
            flags == (TcpHeader::SYN | TcpHeader::ACK | TcpHeader::ECE))
        {
            SocketIpTosTag tosTag;
            uint8_t ceTos = (ipTos & 0xfc) | 0x03;
            tosTag.SetTos(ceTos);
            if (found)
            {
                Ptr<Packet> mod = const_cast<Packet*>(PeekPointer(p));
                mod->ReplacePacketTag(tosTag);
            }
            else
            {
                p->AddPacketTag(tosTag);
            }
        }

        if (m_testcase < SYNACK_CE_CLASSIC_SENDER && (m_receiverSent < 6 && m_receiverSent > 3))
        {
            NS_LOG_DEBUG("set Window size = 0, trying to trigger W probe");
            auto header = const_cast<TcpHeader*>(&h);
            header->SetWindowSize(0);
        }

        // RECEIVER only sends pure acks in this case
        if (flags == TcpHeader::ACK && m_testcase == ACK_MARKING)
        {
            NS_TEST_ASSERT_MSG_EQ(ipTos, 0x0, "IP TOS should not be set in pure ACKs");
        }

        else if ((flags & (TcpHeader::SYN | TcpHeader::ACK)) == (TcpHeader::SYN | TcpHeader::ACK))
        {
            if (m_testcase == SYNACK_ECT_MARKING)
            {
                // SYN-ACK marking test: SYN-ACK should have ECT set
                NS_TEST_ASSERT_MSG_EQ(ipTos, 0x2, "IP TOS should have ECT set for SYN-ACK packets");
            }
            else if (m_testcase == SYNACK_CE_RESPONSE)
            {
                GetTcb(SENDER)->m_ecnState = TcpSocketState::ECN_CE_RCVD;
            }
            else if (m_testcase == SYNACK_FALLBACK)
            {
                m_synAckCount++;
                if (m_synAckCount == 3)
                {
                    NS_TEST_ASSERT_MSG_EQ(ipTos,
                                          0x0,
                                          "3rd SYN-ACK should have ECN fallback (ECT not set)");
                }
            }
        }

        if (m_testcase == WINDOW_PROBE_MARKING && p->GetSize() == 1)
        {
            NS_TEST_ASSERT_MSG_NE(ipTos, 0x0, "IP TOS should be set in Window Probe");
        }

        if (m_testcase == RETX_MARKING)
        {
            // after dropping data packet
            if (m_dataPacketDropped && (h.GetSequenceNumber() == SequenceNumber32(m_seqToDrop)))
            {
                NS_TEST_ASSERT_MSG_NE(ipTos, 0x0, "IP TOS should be set in retransmitted pkt also");
            }
            // after dropping data packet
            if (h.GetSequenceNumber() == SequenceNumber32(m_seqToDrop))
            {
                m_dataPacketDropped = true;
            }
        }
    }

    std::ostringstream flagsStream;
    SocketIpTosTag i;
    bool f = p->PeekPacketTag(i);
    uint16_t tos = 0;
    if (f)
    {
        tos = static_cast<uint16_t>(i.GetTos());
    }
    if (flags & TcpHeader::SYN)
    {
        flagsStream << " SYN";
    }
    if (flags & TcpHeader::FIN)
    {
        flagsStream << " FIN";
    }
    if (flags & TcpHeader::RST)
    {
        flagsStream << " RST";
    }
    if (flags & TcpHeader::ACK)
    {
        flagsStream << " ACK";
    }
    if (flags & TcpHeader::ECE)
    {
        flagsStream << " ECE";
    }
    if (flags & TcpHeader::CWR)
    {
        flagsStream << " CWR";
    }

    std::string whoStr = (who == SENDER ? "SENDER" : "RECEIVER");

    std::cout << "[" << whoStr << "]"
              << "\tFlags:" << (flagsStream.str().empty() ? " (data)" : flagsStream.str())
              << "\tSeq=" << h.GetSequenceNumber() << "\tAck=" << h.GetAckNumber()
              << "\tSize=" << p->GetSize() << "\tTos=" << (tos & 0x03) << std::endl;
}

/**
 * @ingroup internet-test
 * @ingroup tests
 *
 * @brief TCP ECN++ TestSuite
 */
class TcpEcnPpNegotiationSuite : public TestSuite
{
  public:
    /**
     * @brief Test suite for ECN++ negotiation scenarios.
     *
     * This suite tests various combinations of sender and receiver ECN capabilities.
     */
    TcpEcnPpNegotiationSuite()
        : TestSuite("tcp-ecnpp-negotiation", Type::UNIT)
    {
        AddTestCase(
            new TcpEcnPpTest(EcnPpSender_NoEcnReceiver,
                             "Negotiation: ECN++ capable sender and ECN incapable receiver"),
            TestCase::Duration::QUICK);
        AddTestCase(
            new TcpEcnPpTest(EcnPpSender_ClassicEcnReceiver,
                             "Negotiation: ECN++ capable sender and ClassicECN capable receiver"),
            TestCase::Duration::QUICK);
        AddTestCase(
            new TcpEcnPpTest(NoEcnSender_EcnPpReceiver,
                             "Negotiation: ECN incapable sender and ECN++ capable receiver"),
            TestCase::Duration::QUICK);
        AddTestCase(
            new TcpEcnPpTest(ClassicEcnSender_EcnPpReceiver,
                             "Negotiation: ClassicECN capable sender and ECN++ capable receiver"),
            TestCase::Duration::QUICK);
        AddTestCase(
            new TcpEcnPpTest(EcnPpSender_EcnPpReceiver,
                             "Negotiation: ECN++ capable sender and ECN++ capable receiver"),
            TestCase::Duration::QUICK);
    }
};

/**
 * @ingroup internet-test
 * @ingroup tests
 * @brief ECN++ SYN/ACK TestSuite.
 */
class TcpEcnPpSynAckSuite : public TestSuite
{
  public:
    /**
     * @brief Test suite for SYN/ACK handling with ECN++.
     *
     * This suite validates marking and response behavior for SYN-ACK exchanges.
     */
    TcpEcnPpSynAckSuite()
        : TestSuite("tcp-ecnpp-synack", Type::UNIT)
    {
        AddTestCase(new TcpEcnPpTest(SYNACK_CE_CLASSIC_SENDER,
                                     "SYN+ACK CE: ClassicECN sender and ECN++ receiver"),
                    TestCase::Duration::QUICK);
        AddTestCase(
            new TcpEcnPpTest(SYNACK_CE_ECNPP_BOTH, "SYN+ACK CE: ECN++ sender and ECN++ receiver"),
            TestCase::Duration::QUICK);
        AddTestCase(new TcpEcnPpTest(SYNACK_ECT_MARKING, "SYN-ACK ECT marking"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpEcnPpTest(SYNACK_CE_RESPONSE, "SYN-ACK CE response"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpEcnPpTest(SYNACK_FALLBACK, "SYN-ACK ECN fallback"),
                    TestCase::Duration::QUICK);
    }
};

/**
 * @ingroup internet-test
 * @ingroup tests
 * @brief ECN++ FIN/RST TestSuite.
 */
class TcpEcnPpFinRstSuite : public TestSuite
{
  public:
    /**
     * @brief Test suite for ECN++ behavior for FIN and RST packets.
     *
     * This suite checks marking and CE response semantics for connection teardown and reset.
     */
    TcpEcnPpFinRstSuite()
        : TestSuite("tcp-ecnpp-finrst", Type::UNIT)
    {
        AddTestCase(new TcpEcnPpTest(FIN_MARKING, "FIN packet ECT marking"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpEcnPpTest(FIN_CE_RESPONSE, "FIN packet CE response"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpEcnPpTest(RST_MARKING, "RST packet ECT marking"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpEcnPpTest(RST_CE_RESPONSE, "RST packet CE response"),
                    TestCase::Duration::QUICK);
    }
};

/**
 * @ingroup internet-test
 * @ingroup tests
 * @brief ECN++ per-packet marking TestSuite. (SYN, Window Probe, RETX, ACK).
 */
class TcpEcnPpPerPacketSuite : public TestSuite
{
  public:
    /**
     * @brief Test suite for per-packet ECN++ marking tests.
     *
     * This suite exercises marking rules for individual packet types (SYN, Window Probe, RETX,
     * ACK).
     */
    TcpEcnPpPerPacketSuite()
        : TestSuite("tcp-ecnpp-perpacket", Type::UNIT)
    {
        AddTestCase(new TcpEcnPpTest(SYN_ECT_MARKING, "SYN packet ECT marking"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpEcnPpTest(WINDOW_PROBE_MARKING, "Window Probe packet marking"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpEcnPpTest(RETX_MARKING, "Retransmission packet marking"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpEcnPpTest(ACK_MARKING, "ACK packet marking"), TestCase::Duration::QUICK);
    }
};

static TcpEcnPpNegotiationSuite
    g_tcpEcnPpNegotiationSuite; //!< static var for ECN++ negotiation scenarios
static TcpEcnPpSynAckSuite
    g_tcpEcnPpSynAckSuite; //!< static var for SYN/ACK marking and response scenarios
static TcpEcnPpFinRstSuite
    g_tcpEcnPpFinRstSuite; //!< static var for FIN and RST marking and CE response
static TcpEcnPpPerPacketSuite g_tcpEcnPpPerPacketSuite; //!< static var for Per-packet marking tests
                                                        //!< (SYN, Window Probe, RETX, ACK)

/**
 * @ingroup internet-test
 * @ingroup tests
 *
 * @brief Aggregate ECN++ TestSuite.
 *
 * Registers all ECN++ test cases (negotiation, SYN/ACK, FIN/RST, SYN, Window Probe, RETX and ACK
 * marking) into a single suite for comprehensive testing of ECN++ behavior in TCP.
 */
class TcpEcnPpAllSuite : public TestSuite
{
  public:
    TcpEcnPpAllSuite()
        : TestSuite("tcp-ecnpp-test", Type::UNIT)
    {
        // Test cases for negotiation scenarios
        AddTestCase(new TcpEcnPpTest(EcnPpSender_NoEcnReceiver,
                                     "Negotiation: EcnPp sender vs NoEcn receiver"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpEcnPpTest(EcnPpSender_ClassicEcnReceiver,
                                     "Negotiation: EcnPp sender vs ClassicEcn receiver"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpEcnPpTest(NoEcnSender_EcnPpReceiver,
                                     "Negotiation: NoEcn sender vs EcnPp receiver"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpEcnPpTest(ClassicEcnSender_EcnPpReceiver,
                                     "Negotiation: ClassicEcn sender vs EcnPp receiver"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpEcnPpTest(EcnPpSender_EcnPpReceiver,
                                     "Negotiation: EcnPp sender vs EcnPp receiver"),
                    TestCase::Duration::QUICK);

        // Test cases for SYN/ACK related scenarios
        AddTestCase(new TcpEcnPpTest(SYNACK_CE_CLASSIC_SENDER,
                                     "SYN+ACK CE: ClassicECN sender & EcnPp receiver"),
                    TestCase::Duration::QUICK);
        AddTestCase(
            new TcpEcnPpTest(SYNACK_CE_ECNPP_BOTH, "SYN+ACK CE: EcnPp sender & EcnPp receiver"),
            TestCase::Duration::QUICK);
        AddTestCase(new TcpEcnPpTest(SYNACK_ECT_MARKING, "SYN-ACK ECT marking"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpEcnPpTest(SYNACK_CE_RESPONSE, "SYN-ACK CE response"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpEcnPpTest(SYNACK_FALLBACK, "SYN-ACK ECN fallback"),
                    TestCase::Duration::QUICK);

        // Test cases for FIN and RST related scenarios
        AddTestCase(new TcpEcnPpTest(FIN_MARKING, "FIN packet ECT marking"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpEcnPpTest(FIN_CE_RESPONSE, "FIN packet CE response"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpEcnPpTest(RST_MARKING, "RST packet ECT marking"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpEcnPpTest(RST_CE_RESPONSE, "RST packet CE response"),
                    TestCase::Duration::QUICK);

        // Test cases for per-packet marking of SYN, Window Probe, RETX, ACK.
        AddTestCase(new TcpEcnPpTest(SYN_ECT_MARKING, "SYN packet ECT marking"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpEcnPpTest(WINDOW_PROBE_MARKING, "Window Probe packet marking"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpEcnPpTest(RETX_MARKING, "Retransmission packet marking"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpEcnPpTest(ACK_MARKING, "ACK packet marking"), TestCase::Duration::QUICK);
    }
};

static TcpEcnPpAllSuite
    g_tcpEcnPpAllSuite; //!< static var for Aggregate ECN++ suite ("tcp-ecnpp-test")

} // namespace ns3
