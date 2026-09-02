/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author : Urval Kheni <kheniurval777@gmail.com>
 */

#include "tcp-general-test.h"

#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-end-point.h"
#include "ns3/log.h"
#include "ns3/tcp-header.h"
#include "ns3/tcp-option-ts.h"
#include "ns3/tcp-socket-state.h"
#include "ns3/tcp-tx-buffer.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpChallengeAckTestSuite");

// NOTE:
// This test suite validates RFC 5961 handling for pure ACK segments only,
// matching the current implementation scope. ACKs carrying payload
// (data+ACK, FIN|ACK) are not covered here.
//
// TODO:
// Extend coverage to the IPv6 path (ForwardUp6) and non-pure ACK cases.

/**
 * @brief TCP socket harness used by RFC 5961 tests.
 */
class TcpChallengeAckHarness : public TcpSocketMsgBase
{
  public:
    /**
     * @brief Get the TypeId for the harness.
     *
     * @return the TypeId
     */
    static TypeId GetTypeId();

    TcpChallengeAckHarness()
        : TcpSocketMsgBase()
    {
    }

    /**
     * @brief Copy constructor.
     *
     * @param other harness to copy
     */
    TcpChallengeAckHarness(const TcpChallengeAckHarness& other)
        : TcpSocketMsgBase(other)
    {
    }

    /**
     * @brief Inject a pure ACK into the socket receive path.
     *
     * @param ackNumber ACK number to advertise
     * @param advertisedWindow advertised receive window
     */
    void InjectPureAck(SequenceNumber32 ackNumber, uint16_t advertisedWindow)
    {
        Ptr<Packet> packet = Create<Packet>();
        TcpHeader header;
        header.SetFlags(TcpHeader::ACK);
        header.SetSequenceNumber(m_tcb->m_rxBuffer->NextRxSequence());
        header.SetAckNumber(ackNumber);
        header.SetWindowSize(advertisedWindow);
        if (m_endPoint != nullptr)
        {
            auto* ep = m_endPoint;
            NS_ASSERT(ep != nullptr);
            header.SetSourcePort(ep->GetPeerPort());
            header.SetDestinationPort(ep->GetLocalPort());
            packet->AddHeader(header);
            DoForwardUp(packet,
                        InetSocketAddress(ep->GetPeerAddress(), ep->GetPeerPort()),
                        InetSocketAddress(ep->GetLocalAddress(), ep->GetLocalPort()));
            return;
        }

        NS_FATAL_ERROR("IPv6 path not used in TcpChallengeAckHarness");
    }

    /**
     * @brief Inject a pure ACK carrying a TCP timestamp option.
     *
     * @param ackNumber ACK number to advertise
     * @param advertisedWindow advertised receive window
     * @param timestampValue TSval to place in the option
     * @param timestampEcho TSecr to place in the option
     */
    void InjectPureAckWithTimestamp(SequenceNumber32 ackNumber,
                                    uint16_t advertisedWindow,
                                    uint32_t timestampValue,
                                    uint32_t timestampEcho)
    {
        Ptr<Packet> packet = Create<Packet>();
        TcpHeader header;
        header.SetFlags(TcpHeader::ACK);
        header.SetSequenceNumber(m_tcb->m_rxBuffer->NextRxSequence());
        header.SetAckNumber(ackNumber);
        header.SetWindowSize(advertisedWindow);

        Ptr<TcpOptionTS> ts = CreateObject<TcpOptionTS>();
        ts->SetTimestamp(timestampValue);
        ts->SetEcho(timestampEcho);
        header.AppendOption(ts);

        if (m_endPoint != nullptr)
        {
            auto* ep = m_endPoint;
            NS_ASSERT(ep != nullptr);
            header.SetSourcePort(ep->GetPeerPort());
            header.SetDestinationPort(ep->GetLocalPort());
            packet->AddHeader(header);
            DoForwardUp(packet,
                        InetSocketAddress(ep->GetPeerAddress(), ep->GetPeerPort()),
                        InetSocketAddress(ep->GetLocalAddress(), ep->GetLocalPort()));
            return;
        }

        NS_FATAL_ERROR("IPv6 path not used in TcpChallengeAckHarness");
    }

    /**
     * @brief Get the maximum remembered send window.
     *
     * @return MAX.SND.WND tracked by the socket
     */
    uint32_t GetMaxSndWnd() const
    {
        return m_maxSndWnd;
    }

    /**
     * @brief Get the highest cumulative ACK tracked by the socket.
     *
     * @return the tracked ACK number
     */
    SequenceNumber32 GetTrackedHighRxAck() const
    {
        return GetHighRxAck();
    }

    /**
     * @brief Get the number of challenge ACKs sent by the socket.
     *
     * @return the challenge ACK count
     */
    uint32_t GetChallengeAckCount() const
    {
        return m_challengeAckCount;
    }

    /**
     * @brief Get the current retransmission timeout.
     *
     * @return the cached RTO
     */
    Time GetRto() const
    {
        return m_rto;
    }

    /**
     * @brief Get the current ECN state.
     *
     * @return the ECN state
     */
    TcpSocketState::EcnState_t GetEcnState() const
    {
        return m_tcb->m_ecnState;
    }

    /**
     * @brief Force the socket into a specific ECN state.
     *
     * @param ecnState state to install
     */
    void ForceEcnState(TcpSocketState::EcnState_t ecnState)
    {
        m_tcb->m_ecnState = ecnState;
    }

    /**
     * @brief Get the last received timestamp value.
     *
     * @return the stored TSval
     */
    uint32_t GetLastTimestampValue() const
    {
        return m_tcb->m_rcvTimestampValue;
    }

    /**
     * @brief Get the last received timestamp echo reply.
     *
     * @return the stored TSecr
     */
    uint32_t GetLastTimestampEcho() const
    {
        return m_tcb->m_rcvTimestampEchoReply;
    }

    Ptr<TcpSocketBase> Fork() override
    {
        return CopyObject<TcpChallengeAckHarness>(this);
    }
};

NS_OBJECT_ENSURE_REGISTERED(TcpChallengeAckHarness);

TypeId
TcpChallengeAckHarness::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TcpChallengeAckHarness")
                            .SetParent<TcpSocketMsgBase>()
                            .SetGroupName("Internet")
                            .AddConstructor<TcpChallengeAckHarness>();
    return tid;
}

/**
 * @ingroup internet-test
 *
 * @brief RFC 5961 invalid future ACK test.
 */
class TcpChallengeAckInvalidAckTest : public TcpGeneralTest
{
  public:
    TcpChallengeAckInvalidAckTest()
        : TcpGeneralTest("RFC 5961 invalid pure ACKs trigger challenge ACKs without state change")
    {
    }

  protected:
    Ptr<TcpSocketMsgBase> CreateSenderSocket(Ptr<Node> node) override
    {
        return CreateSocket(node,
                            TcpChallengeAckHarness::GetTypeId(),
                            m_congControlTypeId,
                            m_recoveryTypeId);
    }

    void ConfigureEnvironment() override
    {
        TcpGeneralTest::ConfigureEnvironment();
        SetTransmitStart(Seconds(100));
        SetAppPktCount(1);
        SetAppPktSize(500);
    }

    void ConfigureProperties() override
    {
        TcpGeneralTest::ConfigureProperties();
        SetUseEcn(SENDER, TcpSocketState::On);
        SetUseEcn(RECEIVER, TcpSocketState::On);
        // Keep this test focused on pure ACK validation without timestamp handling.
        GetSenderSocket()->SetAttribute("Timestamp", BooleanValue(false));
        GetReceiverSocket()->SetAttribute("Timestamp", BooleanValue(false));
        Simulator::Schedule(Seconds(10.1),
                            &TcpChallengeAckInvalidAckTest::InjectInvalidFutureAck,
                            this);
        Simulator::Schedule(Seconds(10.2),
                            &TcpChallengeAckInvalidAckTest::VerifyAfterInjection,
                            this);
        Simulator::Stop(Seconds(10.3));
    }

    void Tx(const Ptr<const Packet> p, const TcpHeader& h, SocketWho who) override
    {
        if (who == SENDER && m_injectionDone && p->GetSize() == 0 &&
            (h.GetFlags() & TcpHeader::ACK) && !(h.GetFlags() & TcpHeader::RST))
        {
            ++m_challengeAckCount;
            m_lastChallengeAck = h;
        }
    }

  private:
    /**
     * @brief Inject an ACK larger than SND.NXT.
     */
    void InjectInvalidFutureAck()
    {
        auto sender = DynamicCast<TcpChallengeAckHarness>(GetSenderSocket());
        NS_TEST_ASSERT_MSG_NE(sender, nullptr, "Sender harness was not installed");
        NS_TEST_ASSERT_MSG_EQ(GetTcpState(SENDER),
                              TcpSocket::ESTABLISHED,
                              "Sender must be established before injecting ACK");

        uint32_t maxSndWnd = sender->GetMaxSndWnd();
        NS_TEST_ASSERT_MSG_GT(maxSndWnd, 0u, "MAX.SND.WND should be initialized before test");

        Ptr<TcpTxBuffer> txBuffer = GetTxBuffer(SENDER);
        NS_TEST_ASSERT_MSG_NE(txBuffer, nullptr, "Sender Tx buffer should be available");
        m_sndUnaBefore = txBuffer->HeadSequence();
        m_rWndBefore = GetRWnd(SENDER);
        m_highRxAckBefore = sender->GetTrackedHighRxAck();
        m_socketChallengeAckCountBefore = sender->GetChallengeAckCount();
        m_dupAckBefore = GetDupAckCount(SENDER);
        m_cWndBefore = GetTcb(SENDER)->m_cWnd;
        m_ssThreshBefore = GetTcb(SENDER)->m_ssThresh;
        m_congStateBefore = GetTcb(SENDER)->m_congState;
        sender->ForceEcnState(TcpSocketState::ECN_CE_RCVD);
        m_ecnStateBefore = sender->GetEcnState();
        m_rtoBefore = sender->GetRto();

        // ACK beyond SND.NXT should trigger a challenge ACK.
        m_injectionDone = true;
        SequenceNumber32 invalidAck = GetHighestTxMark(SENDER) + 1;
        sender->InjectPureAck(invalidAck, static_cast<uint16_t>(GetRWnd(SENDER)));
    }

    /**
     * @brief Verify sender state after invalid ACK injection.
     */
    void VerifyAfterInjection()
    {
        auto sender = DynamicCast<TcpChallengeAckHarness>(GetSenderSocket());
        NS_TEST_ASSERT_MSG_NE(sender, nullptr, "Sender harness was not installed");
        bool expectEce = (m_ecnStateBefore == TcpSocketState::ECN_CE_RCVD ||
                          m_ecnStateBefore == TcpSocketState::ECN_SENDING_ECE);
        uint8_t expectedFlags = TcpHeader::ACK | (expectEce ? TcpHeader::ECE : 0);
        NS_TEST_ASSERT_MSG_EQ(sender->GetChallengeAckCount(),
                              m_socketChallengeAckCountBefore + 1,
                              "Exactly one challenge ACK is expected");
        NS_TEST_ASSERT_MSG_EQ(m_challengeAckCount, 1u, "Exactly one challenge ACK is expected");
        NS_TEST_ASSERT_MSG_EQ(m_lastChallengeAck.GetFlags(),
                              expectedFlags,
                              "Challenge ACK must reflect ECN state without mutating it");
        NS_TEST_ASSERT_MSG_EQ(m_lastChallengeAck.GetAckNumber(),
                              GetRxBuffer(SENDER)->NextRxSequence(),
                              "Challenge ACK must acknowledge RCV.NXT");
        NS_TEST_ASSERT_MSG_EQ(m_lastChallengeAck.GetSequenceNumber(),
                              GetHighestTxMark(SENDER),
                              "Challenge ACK must use logical SND.NXT as sequence number");
        NS_TEST_ASSERT_MSG_EQ(GetTxBuffer(SENDER)->HeadSequence(),
                              m_sndUnaBefore,
                              "Invalid ACK must not advance SND.UNA");
        NS_TEST_ASSERT_MSG_EQ(GetRWnd(SENDER),
                              m_rWndBefore,
                              "Invalid ACK must not update the remembered remote window");
        NS_TEST_ASSERT_MSG_EQ(sender->GetTrackedHighRxAck(),
                              m_highRxAckBefore,
                              "Invalid ACK must not update the highest ACK received");
        NS_TEST_ASSERT_MSG_EQ(GetDupAckCount(SENDER),
                              m_dupAckBefore,
                              "Invalid ACK must not change dupack accounting");
        NS_TEST_ASSERT_MSG_EQ(GetTcb(SENDER)->m_cWnd,
                              m_cWndBefore,
                              "Invalid ACK must not change the congestion window");
        NS_TEST_ASSERT_MSG_EQ(GetTcb(SENDER)->m_ssThresh,
                              m_ssThreshBefore,
                              "Invalid ACK must not change the slow start threshold");
        NS_TEST_ASSERT_MSG_EQ(GetTcb(SENDER)->m_congState,
                              m_congStateBefore,
                              "Invalid ACK must not change congestion recovery state");
        NS_TEST_ASSERT_MSG_EQ(sender->GetEcnState(),
                              m_ecnStateBefore,
                              "Invalid ACK must not change ECN state");
        NS_TEST_ASSERT_MSG_EQ(sender->GetRto(), m_rtoBefore, "RTO must not change for invalid ACK");
    }

    bool m_injectionDone{false};           ///< Whether the invalid ACK has been injected.
    uint32_t m_challengeAckCount{0};       ///< Challenge ACKs observed on the sender trace.
    TcpHeader m_lastChallengeAck;          ///< Most recent challenge ACK header seen on trace.
    SequenceNumber32 m_sndUnaBefore{0};    ///< Sender SND.UNA before injection.
    SequenceNumber32 m_highRxAckBefore{0}; ///< Highest tracked ACK before injection.
    uint32_t m_socketChallengeAckCountBefore{0}; ///< Socket challenge ACK count before injection.
    uint32_t m_rWndBefore{0};                    ///< Remembered remote window before injection.
    uint32_t m_dupAckBefore{0};                  ///< DupAck counter before injection.
    uint32_t m_cWndBefore{0};                    ///< Congestion window before injection.
    uint32_t m_ssThreshBefore{0};                ///< Slow start threshold before injection.
    TcpSocketState::TcpCongState_t m_congStateBefore{
        TcpSocketState::CA_OPEN}; ///< Congestion state before injection.
    TcpSocketState::EcnState_t m_ecnStateBefore{
        TcpSocketState::ECN_DISABLED}; ///< ECN state before injection.
    Time m_rtoBefore{Time::Min()};     ///< RTO before injection.
};

/**
 * @ingroup internet-test
 *
 * @brief RFC 5961 stale ACK test.
 */
class TcpChallengeAckStaleAckTest : public TcpGeneralTest
{
  public:
    TcpChallengeAckStaleAckTest()
        : TcpGeneralTest("RFC 5961 stale pure ACKs are ignored without challenge ACK")
    {
    }

  protected:
    Ptr<TcpSocketMsgBase> CreateSenderSocket(Ptr<Node> node) override
    {
        return CreateSocket(node,
                            TcpChallengeAckHarness::GetTypeId(),
                            m_congControlTypeId,
                            m_recoveryTypeId);
    }

    void ConfigureEnvironment() override
    {
        TcpGeneralTest::ConfigureEnvironment();
        SetTransmitStart(Seconds(100));
        SetAppPktCount(1);
        SetAppPktSize(500);
    }

    void ConfigureProperties() override
    {
        TcpGeneralTest::ConfigureProperties();
        SetUseEcn(SENDER, TcpSocketState::On);
        SetUseEcn(RECEIVER, TcpSocketState::On);
        // Keep this test focused on pure ACK validation without timestamp handling.
        GetSenderSocket()->SetAttribute("Timestamp", BooleanValue(false));
        GetReceiverSocket()->SetAttribute("Timestamp", BooleanValue(false));
        Simulator::Schedule(Seconds(10.1), &TcpChallengeAckStaleAckTest::InjectStaleAck, this);
        Simulator::Schedule(Seconds(10.2),
                            &TcpChallengeAckStaleAckTest::VerifyAfterInjection,
                            this);
        Simulator::Stop(Seconds(10.3));
    }

    void Tx(const Ptr<const Packet> p, const TcpHeader& h, SocketWho who) override
    {
        if (who == SENDER && m_injectionDone && p->GetSize() == 0 &&
            (h.GetFlags() & TcpHeader::ACK) && !(h.GetFlags() & TcpHeader::RST))
        {
            ++m_challengeAckCount;
            m_lastChallengeAck = h;
        }
    }

  private:
    /**
     * @brief Inject an ACK smaller than SND.UNA.
     */
    void InjectStaleAck()
    {
        auto sender = DynamicCast<TcpChallengeAckHarness>(GetSenderSocket());
        NS_TEST_ASSERT_MSG_NE(sender, nullptr, "Sender harness was not installed");
        NS_TEST_ASSERT_MSG_EQ(GetTcpState(SENDER),
                              TcpSocket::ESTABLISHED,
                              "Sender must be established before injecting ACK");

        uint32_t maxSndWnd = sender->GetMaxSndWnd();
        NS_TEST_ASSERT_MSG_GT(maxSndWnd, 0u, "MAX.SND.WND should be initialized before test");

        Ptr<TcpTxBuffer> txBuffer = GetTxBuffer(SENDER);
        NS_TEST_ASSERT_MSG_NE(txBuffer, nullptr, "Sender Tx buffer should be available");
        m_sndUnaBefore = txBuffer->HeadSequence();
        NS_TEST_ASSERT_MSG_GT(m_sndUnaBefore.GetValue(), 0u, "SND.UNA should be initialized");

        m_rWndBefore = GetRWnd(SENDER);
        m_highRxAckBefore = sender->GetTrackedHighRxAck();
        m_socketChallengeAckCountBefore = sender->GetChallengeAckCount();
        m_dupAckBefore = GetDupAckCount(SENDER);
        m_cWndBefore = GetTcb(SENDER)->m_cWnd;
        m_ssThreshBefore = GetTcb(SENDER)->m_ssThresh;
        m_congStateBefore = GetTcb(SENDER)->m_congState;
        sender->ForceEcnState(TcpSocketState::ECN_CE_RCVD);
        m_ecnStateBefore = sender->GetEcnState();

        // ACK below SND.UNA should be ignored without a challenge ACK.
        m_injectionDone = true;
        SequenceNumber32 staleAck = m_sndUnaBefore - 1;
        sender->InjectPureAck(staleAck, static_cast<uint16_t>(GetRWnd(SENDER)));
    }

    /**
     * @brief Verify sender state after stale ACK injection.
     */
    void VerifyAfterInjection()
    {
        auto sender = DynamicCast<TcpChallengeAckHarness>(GetSenderSocket());
        NS_TEST_ASSERT_MSG_NE(sender, nullptr, "Sender harness was not installed");
        NS_TEST_ASSERT_MSG_EQ(sender->GetChallengeAckCount(),
                              m_socketChallengeAckCountBefore,
                              "Stale ACK must not trigger a challenge ACK");
        NS_TEST_ASSERT_MSG_EQ(m_challengeAckCount,
                              0u,
                              "Stale ACK must not trigger a challenge ACK");
        NS_TEST_ASSERT_MSG_EQ(GetTxBuffer(SENDER)->HeadSequence(),
                              m_sndUnaBefore,
                              "Stale ACK must not advance SND.UNA");
        NS_TEST_ASSERT_MSG_EQ(GetRWnd(SENDER),
                              m_rWndBefore,
                              "Stale ACK must not update the remembered remote window");
        NS_TEST_ASSERT_MSG_EQ(sender->GetTrackedHighRxAck(),
                              m_highRxAckBefore,
                              "Stale ACK must not update the highest ACK received");
        NS_TEST_ASSERT_MSG_EQ(GetDupAckCount(SENDER),
                              m_dupAckBefore,
                              "Stale ACK must not change dupack accounting");
        NS_TEST_ASSERT_MSG_EQ(GetTcb(SENDER)->m_cWnd,
                              m_cWndBefore,
                              "Stale ACK must not change the congestion window");
        NS_TEST_ASSERT_MSG_EQ(GetTcb(SENDER)->m_ssThresh,
                              m_ssThreshBefore,
                              "Stale ACK must not change the slow start threshold");
        NS_TEST_ASSERT_MSG_EQ(GetTcb(SENDER)->m_congState,
                              m_congStateBefore,
                              "Stale ACK must not change congestion recovery state");
        NS_TEST_ASSERT_MSG_EQ(sender->GetEcnState(),
                              m_ecnStateBefore,
                              "Stale ACK must not change ECN state");
    }

    bool m_injectionDone{false};                 ///< Whether the stale ACK has been injected.
    uint32_t m_challengeAckCount{0};             ///< Challenge ACKs observed on the sender trace.
    TcpHeader m_lastChallengeAck;                ///< Most recent ACK header seen after injection.
    SequenceNumber32 m_sndUnaBefore{0};          ///< Sender SND.UNA before injection.
    SequenceNumber32 m_highRxAckBefore{0};       ///< Highest tracked ACK before injection.
    uint32_t m_socketChallengeAckCountBefore{0}; ///< Socket challenge ACK count before injection.
    uint32_t m_rWndBefore{0};                    ///< Remembered remote window before injection.
    uint32_t m_dupAckBefore{0};                  ///< DupAck counter before injection.
    uint32_t m_cWndBefore{0};                    ///< Congestion window before injection.
    uint32_t m_ssThreshBefore{0};                ///< Slow start threshold before injection.
    TcpSocketState::TcpCongState_t m_congStateBefore{
        TcpSocketState::CA_OPEN}; ///< Congestion state before injection.
    TcpSocketState::EcnState_t m_ecnStateBefore{
        TcpSocketState::ECN_DISABLED}; ///< ECN state before injection.
};

/**
 * @ingroup internet-test
 *
 * @brief RFC 5961 invalid future ACK test with timestamps enabled.
 */
class TcpChallengeAckInvalidAckTimestampTest : public TcpGeneralTest
{
  public:
    TcpChallengeAckInvalidAckTimestampTest()
        : TcpGeneralTest("RFC 5961 invalid pure ACKs still trigger challenge ACKs with timestamps")
    {
    }

  protected:
    Ptr<TcpSocketMsgBase> CreateSenderSocket(Ptr<Node> node) override
    {
        return CreateSocket(node,
                            TcpChallengeAckHarness::GetTypeId(),
                            m_congControlTypeId,
                            m_recoveryTypeId);
    }

    void ConfigureEnvironment() override
    {
        TcpGeneralTest::ConfigureEnvironment();
        SetTransmitStart(Seconds(100));
        SetAppPktCount(1);
        SetAppPktSize(500);
    }

    void ConfigureProperties() override
    {
        TcpGeneralTest::ConfigureProperties();
        SetUseEcn(SENDER, TcpSocketState::On);
        SetUseEcn(RECEIVER, TcpSocketState::On);
        Simulator::Schedule(Seconds(10.1),
                            &TcpChallengeAckInvalidAckTimestampTest::InjectInvalidFutureAck,
                            this);
        Simulator::Schedule(Seconds(10.2),
                            &TcpChallengeAckInvalidAckTimestampTest::VerifyAfterInjection,
                            this);
        Simulator::Stop(Seconds(10.3));
    }

    void Tx(const Ptr<const Packet> p, const TcpHeader& h, SocketWho who) override
    {
        if (who == SENDER && m_injectionDone && p->GetSize() == 0 &&
            (h.GetFlags() & TcpHeader::ACK) && !(h.GetFlags() & TcpHeader::RST))
        {
            ++m_challengeAckCount;
            m_lastChallengeAck = h;
        }
    }

  private:
    /**
     * @brief Inject an invalid future ACK carrying timestamps.
     */
    void InjectInvalidFutureAck()
    {
        auto sender = DynamicCast<TcpChallengeAckHarness>(GetSenderSocket());
        NS_TEST_ASSERT_MSG_NE(sender, nullptr, "Sender harness was not installed");
        NS_TEST_ASSERT_MSG_EQ(GetTcpState(SENDER),
                              TcpSocket::ESTABLISHED,
                              "Sender must be established before injecting ACK");
        NS_TEST_ASSERT_MSG_GT(sender->GetMaxSndWnd(),
                              0u,
                              "MAX.SND.WND should be initialized before test");

        m_tsValBefore = sender->GetLastTimestampValue();
        m_tsEchoBefore = sender->GetLastTimestampEcho();
        sender->ForceEcnState(TcpSocketState::ECN_CE_RCVD);
        m_ecnStateBefore = sender->GetEcnState();
        m_injectionDone = true;
        SequenceNumber32 invalidAck = GetHighestTxMark(SENDER) + 1;
        // Use a valid timestamp so the packet reaches RFC 5961 ACK handling.
        sender->InjectPureAckWithTimestamp(invalidAck,
                                           static_cast<uint16_t>(GetRWnd(SENDER)),
                                           m_tsValBefore,
                                           m_tsEchoBefore);
    }

    /**
     * @brief Verify sender state after timestamped invalid ACK injection.
     */
    void VerifyAfterInjection()
    {
        auto sender = DynamicCast<TcpChallengeAckHarness>(GetSenderSocket());
        NS_TEST_ASSERT_MSG_NE(sender, nullptr, "Sender harness was not installed");
        bool expectEce = (m_ecnStateBefore == TcpSocketState::ECN_CE_RCVD ||
                          m_ecnStateBefore == TcpSocketState::ECN_SENDING_ECE);
        uint8_t expectedFlags = TcpHeader::ACK | (expectEce ? TcpHeader::ECE : 0);
        NS_TEST_ASSERT_MSG_EQ(sender->GetChallengeAckCount(),
                              1u,
                              "Challenge ACK must still be sent with timestamps enabled");
        NS_TEST_ASSERT_MSG_EQ(m_challengeAckCount,
                              1u,
                              "Challenge ACK must still be sent with timestamps enabled");
        NS_TEST_ASSERT_MSG_EQ(m_lastChallengeAck.GetFlags(),
                              expectedFlags,
                              "Challenge ACK must reflect ECN state without mutating it");
        NS_TEST_ASSERT_MSG_EQ(sender->GetLastTimestampValue(),
                              m_tsValBefore,
                              "Timestamp value must not change for invalid ACK");
        NS_TEST_ASSERT_MSG_EQ(sender->GetLastTimestampEcho(),
                              m_tsEchoBefore,
                              "Timestamp echo must not change for invalid ACK");
        NS_TEST_ASSERT_MSG_EQ(sender->GetEcnState(),
                              m_ecnStateBefore,
                              "Invalid ACK must not change ECN state");
    }

    bool m_injectionDone{false};     ///< Whether the invalid ACK has been injected.
    uint32_t m_challengeAckCount{0}; ///< Challenge ACKs observed on the sender trace.
    TcpHeader m_lastChallengeAck;    ///< Most recent challenge ACK header seen on trace.
    TcpSocketState::EcnState_t m_ecnStateBefore{
        TcpSocketState::ECN_DISABLED}; ///< ECN state before injection.
    uint32_t m_tsValBefore{0};         ///< Stored TSval before injection.
    uint32_t m_tsEchoBefore{0};        ///< Stored TSecr before injection.
};

/**
 * @ingroup internet-test
 *
 * @brief RFC 5961 TCP test suite.
 */
class TcpChallengeAckTestSuite : public TestSuite
{
  public:
    TcpChallengeAckTestSuite()
        : TestSuite("tcp-challenge-ack-test", Type::UNIT)
    {
        AddTestCase(new TcpChallengeAckInvalidAckTest(), TestCase::Duration::QUICK);
        AddTestCase(new TcpChallengeAckStaleAckTest(), TestCase::Duration::QUICK);
        AddTestCase(new TcpChallengeAckInvalidAckTimestampTest(), TestCase::Duration::QUICK);
    }
};

static TcpChallengeAckTestSuite
    g_tcpChallengeAckTestSuite; //!< Static variable for test initialization
