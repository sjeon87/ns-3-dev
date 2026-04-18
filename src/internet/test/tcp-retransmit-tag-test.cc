/*
 * Copyright (c) 2026 Sergio Andreozzi
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sergio Andreozzi <digitalities@gmail.com>
 */

#include "tcp-general-test.h"

#include "ns3/config.h"
#include "ns3/error-model.h"
#include "ns3/log.h"
#include "ns3/tcp-retransmit-tag.h"

#include <list>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpRetransmitTagTestSuite");

/**
 * @ingroup internet-test
 *
 * Verify that TcpSocketBase stamps TcpRetransmitTag on retransmitted
 * segments only, and that the tag is removed before delivery to the
 * receiver socket.
 */
class TcpRetransmitTagTest : public TcpGeneralTest
{
  public:
    /**
     * @brief Constructor.
     * @param dropSegment Segment to drop at the receiver (1-indexed).
     * @param desc Test description.
     */
    TcpRetransmitTagTest(uint32_t dropSegment, const std::string& desc);

  protected:
    void ConfigureEnvironment() override;
    void ConfigureProperties() override;
    Ptr<ErrorModel> CreateReceiverErrorModel() override;
    void Tx(const Ptr<const Packet> p, const TcpHeader& h, SocketWho who) override;
    void Rx(const Ptr<const Packet> p, const TcpHeader& h, SocketWho who) override;
    void FinalChecks() override;

  private:
    uint32_t m_dropSegment;         //!< Segment number to drop at the receiver.
    uint32_t m_taggedTx{0};         //!< Sender-side transmissions carrying TcpRetransmitTag.
    uint32_t m_untaggedTx{0};       //!< Sender-side transmissions without the tag.
    uint32_t m_minRetxCount{0};     //!< Minimum retxCount value observed on any tag.
    uint32_t m_taggedReceiverRx{0}; //!< Receiver-side Rx packets still carrying the tag.
};

TcpRetransmitTagTest::TcpRetransmitTagTest(uint32_t dropSegment, const std::string& desc)
    : TcpGeneralTest(desc),
      m_dropSegment(dropSegment)
{
}

void
TcpRetransmitTagTest::ConfigureEnvironment()
{
    TcpGeneralTest::ConfigureEnvironment();
    SetAppPktSize(500);
    SetAppPktCount(50);
    SetAppPktInterval(MilliSeconds(1));
    SetPropagationDelay(MilliSeconds(5));
    SetTransmitStart(Seconds(2));
    Simulator::Stop(Seconds(20));
}

void
TcpRetransmitTagTest::ConfigureProperties()
{
    TcpGeneralTest::ConfigureProperties();
    SetSegmentSize(SENDER, 500);
    SetSegmentSize(RECEIVER, 500);
    SetInitialCwnd(SENDER, 10);
}

Ptr<ErrorModel>
TcpRetransmitTagTest::CreateReceiverErrorModel()
{
    Ptr<ReceiveListErrorModel> rem = CreateObject<ReceiveListErrorModel>();
    std::list<uint32_t> errorList;
    errorList.push_back(m_dropSegment);
    rem->SetList(errorList);
    return rem;
}

void
TcpRetransmitTagTest::Tx(const Ptr<const Packet> p, const TcpHeader& h, SocketWho who)
{
    if (who != SENDER)
    {
        return;
    }
    // Tag only attaches to data segments; empty payloads (ACK/SYN/FIN)
    // are skipped above SendDataPacket.
    TcpRetransmitTag tag;
    if (p->PeekPacketTag(tag))
    {
        ++m_taggedTx;
        uint32_t retx = tag.GetRetxCount();
        if (m_minRetxCount == 0 || retx < m_minRetxCount)
        {
            m_minRetxCount = retx;
        }
    }
    else if (p->GetSize() > 0)
    {
        ++m_untaggedTx;
    }
}

void
TcpRetransmitTagTest::Rx(const Ptr<const Packet> p, const TcpHeader& h, SocketWho who)
{
    if (who != RECEIVER)
    {
        return;
    }
    // Rx fires after DoForwardUp strips the tag; any tagged packet here
    // means the strip is broken.
    TcpRetransmitTag tag;
    if (p->PeekPacketTag(tag))
    {
        ++m_taggedReceiverRx;
    }
}

void
TcpRetransmitTagTest::FinalChecks()
{
    NS_TEST_ASSERT_MSG_GT(m_taggedTx, 0, "Expected at least one tagged retransmission");
    NS_TEST_ASSERT_MSG_GT(m_untaggedTx, 0, "Expected at least one untagged fresh transmission");
    NS_TEST_ASSERT_MSG_GT(m_untaggedTx,
                          m_taggedTx,
                          "Fresh transmissions should outnumber retransmissions");
    NS_TEST_ASSERT_MSG_GT_OR_EQ(m_minRetxCount, 1, "Tag retxCount must be at least 1");
    NS_TEST_ASSERT_MSG_EQ(m_taggedReceiverRx,
                          0,
                          "Strip at DoForwardUp failed: receiver Rx trace saw a "
                          "packet still carrying TcpRetransmitTag");
}

/**
 * @ingroup internet-test
 *
 * Test suite for TcpRetransmitTag stamping behaviour.
 */
class TcpRetransmitTagTestSuite : public TestSuite
{
  public:
    TcpRetransmitTagTestSuite()
        : TestSuite("tcp-retransmit-tag", Type::UNIT)
    {
        // Drop segment 10 of the bulk transfer; subsequent segments deliver
        // duplicate ACKs that trigger fast retransmit on the sender.
        AddTestCase(new TcpRetransmitTagTest(10, "fast retx stamps TcpRetransmitTag"),
                    TestCase::Duration::QUICK);
    }
};

static TcpRetransmitTagTestSuite
    g_tcpRetransmitTagTestSuite; //!< Static variable for test initialization
