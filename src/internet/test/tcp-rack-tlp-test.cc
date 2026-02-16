/*
 * Copyright (c) 2018-2026 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Shikha Bakshi <shikhabakshi912@gmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 *          Mohnish Hemanth Kumar <mohnishhemanthkumar@gmail.com>
 *          Nikhil Kottoli <nikhilkottoli2005@gmail.com>
 *          Manish Agarwal <manishagarwal428728@gmail.com>
 *          Patel Pal Bharat <ppal61679@gmail.com>
 */

#include "tcp-error-model.h"
#include "tcp-general-test.h"

#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/simple-channel.h"
#include "ns3/tcp-cubic.h"
#include "ns3/tcp-westwood-plus.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpRackTlpTest");

/**
 * @ingroup internet-test
 * @ingroup tests
 *
 * @brief Testsuite for the RACK with SACK
 */
class TcpRackSackTest : public TcpGeneralTest
{
  public:
    /**
     * @brief Constructor
     * @param sack Boolean value to enable or disable SACK
     * @param rack Boolean variable to enable or disable RACK.
     * @param msg Test message.
     */
    TcpRackSackTest(bool sack, bool rack, const std::string& msg);

    Ptr<TcpSocketMsgBase> CreateSenderSocket(Ptr<Node> node) override;

  protected:
    void Tx(const Ptr<const Packet> p, const TcpHeader& h, SocketWho who) override;

    bool m_sackState; //!< Sack state
    bool m_rackState; //!< RACK state
};

TcpRackSackTest::TcpRackSackTest(bool sack, bool rack, const std::string& msg)
    : TcpGeneralTest(msg),
      m_sackState(sack),
      m_rackState(rack)
{
}

Ptr<TcpSocketMsgBase>
TcpRackSackTest::CreateSenderSocket(Ptr<Node> node)
{
    Ptr<TcpSocketMsgBase> socket = TcpGeneralTest::CreateSenderSocket(node);
    socket->SetAttribute("Rack", BooleanValue(m_rackState));
    socket->SetAttribute("Sack", BooleanValue(m_sackState));
    return socket;
}

void
TcpRackSackTest::Tx(const Ptr<const Packet> p, const TcpHeader& h, SocketWho who)
{
    if ((h.GetFlags() & TcpHeader::SYN) && m_sackState)
    {
        std::cout << h.HasOption(TcpOption::SACKPERMITTED);
        NS_TEST_ASSERT_MSG_EQ(h.HasOption(TcpOption::SACKPERMITTED),
                              true,
                              "SackPermitted disabled but option enabled");
    }
}

/**
 * @ingroup internet-test
 * @ingroup tests
 *
 * @brief Testsuite for the RACK
 */
class TcpRackTest : public TcpGeneralTest
{
  public:
    /**
     * @brief Constructor
     * @param congControl Type of congestion control.
     * @param seqToKill Sequence number of the packet to drop.
     * @param rack Boolean variable to enable or disable RACK.
     * @param msg Test message.
     */
    TcpRackTest(TypeId congControl, uint32_t seqToKill, bool rack, const std::string& msg);

    Ptr<ErrorModel> CreateSenderErrorModel() override;
    Ptr<ErrorModel> CreateReceiverErrorModel() override;

    Ptr<TcpSocketMsgBase> CreateSenderSocket(Ptr<Node> node) override;

  protected:
    void AfterRTOExpired(const Ptr<const TcpSocketState> tcb, SocketWho who) override;
    void CongStateTrace(const TcpSocketState::TcpCongState_t oldValue,
                        const TcpSocketState::TcpCongState_t newValue) override;

    void CWndTrace(uint32_t oldValue, uint32_t newValue) override;

    /**
     * @brief Check if the packet being dropped is the right one.
     * @param ipH IPv4 header.
     * @param tcpH TCP header.
     * @param p The packet.
     */
    void PktDropped(const Ipv4Header& ipH, const TcpHeader& tcpH, Ptr<const Packet> p);

    void ConfigureProperties() override;
    void ConfigureEnvironment() override;

    bool m_pktDropped;    //!< The packet has been dropped.
    bool m_pktWasDropped; //!< The packet was dropped (according to the receiver).
    uint32_t m_seqToKill; //!< Sequence number to drop.
    bool m_rack;          //!< Enable/Disable RACK.

    Ptr<TcpSeqErrorModel> m_errorModel; //!< Error model.
};

TcpRackTest::TcpRackTest(TypeId typeId, uint32_t seqToKill, bool rack, const std::string& msg)
    : TcpGeneralTest(msg),
      m_pktDropped(false),
      m_seqToKill(seqToKill),
      m_rack(rack)
{
    m_congControlTypeId = typeId;
    // creating directories to store the plots
    std::string dir = "rack";
    std::string dirToSave = "mkdir -p " + dir;
    std::string plot_rack_on = dir + "/rack-on.plotme";
    std::string plot_rack_off = dir + "/rack-off.plotme";
    std::string FilesToRemove = "rm -f " + plot_rack_on + " " + plot_rack_off;
    system(dirToSave.c_str());
    system(FilesToRemove.c_str());
}

void
TcpRackTest::ConfigureProperties()
{
    TcpGeneralTest::ConfigureProperties();
    SetInitialSsThresh(SENDER, 0);
    SetSegmentSize(SENDER, 500);
}

void
TcpRackTest::ConfigureEnvironment()
{
    TcpGeneralTest::ConfigureEnvironment();
    SetAppPktCount(100);
}

Ptr<ErrorModel>
TcpRackTest::CreateSenderErrorModel()
{
    return nullptr;
}

Ptr<ErrorModel>
TcpRackTest::CreateReceiverErrorModel()
{
    m_errorModel = CreateObject<TcpSeqErrorModel>();
    m_errorModel->AddSeqToKill(SequenceNumber32(m_seqToKill));
    m_errorModel->SetDropCallback(MakeCallback(&TcpRackTest::PktDropped, this));

    return m_errorModel;
}

Ptr<TcpSocketMsgBase>
TcpRackTest::CreateSenderSocket(Ptr<Node> node)
{
    Ptr<TcpSocketMsgBase> socket = TcpGeneralTest::CreateSenderSocket(node);
    socket->SetAttribute("MinRto", TimeValue(Seconds(10.0)));
    socket->SetAttribute("Sack", BooleanValue(true));
    socket->SetAttribute("DSack", BooleanValue(true));
    if (m_rack)
    {
        socket->SetAttribute("Rack", BooleanValue(true));
    }

    return socket;
}

void
TcpRackTest::PktDropped(const Ipv4Header& ipH, const TcpHeader& tcpH, Ptr<const Packet> p)
{
    NS_LOG_FUNCTION(this << ipH << tcpH);

    m_pktDropped = true;

    NS_TEST_ASSERT_MSG_EQ(tcpH.GetSequenceNumber(),
                          SequenceNumber32(m_seqToKill),
                          "Packet dropped but sequence number differs");
}

void
TcpRackTest::AfterRTOExpired(const Ptr<const TcpSocketState> tcb, SocketWho who)
{
    // NS_TEST_ASSERT_MSG_EQ(true, false, "RTO isn't expected here");
}

void
TcpRackTest::CongStateTrace(const TcpSocketState::TcpCongState_t oldValue,
                            const TcpSocketState::TcpCongState_t newValue)
{
    NS_LOG_FUNCTION(this << oldValue << newValue);

    if (oldValue == TcpSocketState::CA_DISORDER && m_rack)
    {
        NS_TEST_ASSERT_MSG_EQ(newValue,
                              TcpSocketState::CA_RECOVERY,
                              "cwnd reduced due to fast retranmit, even though rack is enabled.");
    }
    else if (oldValue == TcpSocketState::CA_DISORDER && !m_rack)
    {
        NS_TEST_ASSERT_MSG_EQ(newValue, TcpSocketState::CA_LOSS, "cwnd reduced due to RTO.");
    }
}

void
TcpRackTest::CWndTrace(uint32_t oldValue, uint32_t newValue)
{
    NS_LOG_FUNCTION(this << oldValue << newValue);
    if (m_rack)
    {
        std::ofstream fPlotQueue("rack/rack-on.plotme", std::ios::out | std::ios::app);
        fPlotQueue << Simulator::Now().GetSeconds() << " " << newValue / 500.0 << std::endl;
        fPlotQueue.close();
    }
    else
    {
        std::ofstream fPlotQueue("rack/rack-off.plotme", std::ios::out | std::ios::app);
        fPlotQueue << Simulator::Now().GetSeconds() << " " << newValue / 500.0 << std::endl;
        fPlotQueue.close();
    }
}

/**
 * @ingroup internet-test
 * @ingroup tests
 *
 * @brief Testsuite for the TLP
 */
class TcpTlpTest : public TcpGeneralTest
{
  public:
    /**
     * @brief Constructor
     * @param congControl Type of congestion control.
     * @param seqToKill Sequence number of the packet to drop.
     * @param tlp Boolean variable to enable or disable TLP.
     * @param msg Test message.
     */
    TcpTlpTest(TypeId congControl, uint32_t seqToKill, bool tlp, const std::string& msg);

    Ptr<ErrorModel> CreateSenderErrorModel() override;
    Ptr<ErrorModel> CreateReceiverErrorModel() override;

    Ptr<TcpSocketMsgBase> CreateSenderSocket(Ptr<Node> node) override;

  protected:
    void CongStateTrace(const TcpSocketState::TcpCongState_t oldValue,
                        const TcpSocketState::TcpCongState_t newValue) override;

    void CWndTrace(uint32_t oldValue, uint32_t newValue) override;

    /**
     * @brief Check if the packet being dropped is the right one.
     * @param ipH IPv4 header.
     * @param tcpH TCP header.
     * @param p The packet.
     */
    void PktDropped(const Ipv4Header& ipH, const TcpHeader& tcpH, Ptr<const Packet> p);

    void ConfigureProperties() override;
    void ConfigureEnvironment() override;

    bool m_pktDropped;    //!< The packet has been dropped.
    bool m_pktWasDropped; //!< The packet was dropped (according to the receiver).
    uint32_t m_seqToKill; //!< Sequence number to drop.
    bool m_tlp;           //!< Enable/Disable TLP.

    Ptr<TcpSeqErrorModel> m_errorModel; //!< Error model.
};

TcpTlpTest::TcpTlpTest(TypeId typeId, uint32_t seqToKill, bool tlp, const std::string& msg)
    : TcpGeneralTest(msg),
      m_pktDropped(false),
      m_seqToKill(seqToKill),
      m_tlp(tlp)
{
    m_congControlTypeId = typeId;
}

void
TcpTlpTest::ConfigureProperties()
{
    TcpGeneralTest::ConfigureProperties();
    SetInitialSsThresh(SENDER, 0);
}

void
TcpTlpTest::ConfigureEnvironment()
{
    TcpGeneralTest::ConfigureEnvironment();
    SetAppPktCount(100);
}

Ptr<ErrorModel>
TcpTlpTest::CreateSenderErrorModel()
{
    return nullptr;
}

Ptr<ErrorModel>
TcpTlpTest::CreateReceiverErrorModel()
{
    m_errorModel = CreateObject<TcpSeqErrorModel>();
    m_errorModel->AddSeqToKill(SequenceNumber32(m_seqToKill));
    m_errorModel->SetDropCallback(MakeCallback(&TcpTlpTest::PktDropped, this));

    return m_errorModel;
}

Ptr<TcpSocketMsgBase>
TcpTlpTest::CreateSenderSocket(Ptr<Node> node)
{
    Ptr<TcpSocketMsgBase> socket = TcpGeneralTest::CreateSenderSocket(node);
    socket->SetAttribute("MinRto", TimeValue(Seconds(10.0)));
    socket->SetAttribute("Sack", BooleanValue(true));
    socket->SetAttribute("DSack", BooleanValue(true));
    if (m_tlp)
    {
        socket->SetAttribute("Tlp", BooleanValue(true));
    }

    return socket;
}

void
TcpTlpTest::PktDropped(const Ipv4Header& ipH, const TcpHeader& tcpH, Ptr<const Packet> p)
{
    NS_LOG_FUNCTION(this << ipH << tcpH);

    m_pktDropped = true;

    NS_TEST_ASSERT_MSG_EQ(tcpH.GetSequenceNumber(),
                          SequenceNumber32(m_seqToKill),
                          "Packet dropped but sequence number differs");
}

void
TcpTlpTest::CongStateTrace(const TcpSocketState::TcpCongState_t oldValue,
                           const TcpSocketState::TcpCongState_t newValue)
{
    NS_LOG_FUNCTION(this << oldValue << newValue);

    if (oldValue == TcpSocketState::CA_OPEN && m_tlp)
    {
        NS_TEST_ASSERT_MSG_EQ(newValue, TcpSocketState::CA_DISORDER, "TLP not working");
    }
    else if (oldValue == TcpSocketState::CA_OPEN && !m_tlp)
    {
        NS_TEST_ASSERT_MSG_EQ(newValue, TcpSocketState::CA_LOSS, "Something working");
    }
}

void
TcpTlpTest::CWndTrace(uint32_t oldValue, uint32_t newValue)
{
    NS_LOG_FUNCTION(this << oldValue << newValue);

    if (m_tlp)
    {
        std::ofstream fPlotQueue("tlp-on.plotme", std::ios::out | std::ios::app);
        fPlotQueue << Simulator::Now().GetSeconds() << " " << newValue / 500.0 << std::endl;
        fPlotQueue.close();
    }
    else
    {
        std::ofstream fPlotQueue("tlp-off.plotme", std::ios::out | std::ios::app);
        fPlotQueue << Simulator::Now().GetSeconds() << " " << newValue / 500.0 << std::endl;
        fPlotQueue.close();
    }
}

/**
 * @ingroup internet-test
 * @ingroup tests
 *
 * @brief Testsuite for RACK and TLP (RFC 8985)
 */
class TcpRackTlpTestSuite : public TestSuite
{
  public:
    TcpRackTlpTestSuite()
        : TestSuite("tcp-rack-tlp-test", Type::UNIT)
    {
        AddTestCase(new TcpRackSackTest(true, true, "Sack enable"), TestCase::Duration::QUICK);
        AddTestCase(new TcpRackSackTest(false, true, "Sack disable"), TestCase::Duration::QUICK);

        AddTestCase(new TcpRackTest(TcpCubic::GetTypeId(), 48501, true, "Rack Enabled testing"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpRackTest(TcpCubic::GetTypeId(), 48501, false, "Rack Disabled testing"),
                    TestCase::Duration::QUICK);

        AddTestCase(new TcpTlpTest(TcpCubic::GetTypeId(), 49501, true, "Tlp testing"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpTlpTest(TcpCubic::GetTypeId(), 49501, false, "Tlp testing"),
                    TestCase::Duration::QUICK);
    }
};

static TcpRackTlpTestSuite g_TcpRackTlpTestSuite; //!< Static variable for test initialization
