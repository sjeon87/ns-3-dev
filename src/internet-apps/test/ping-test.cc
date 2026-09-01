/*
 * Copyright (c) 2022 Chandrakant Jena
 * Copyright (c) 2022 Universita' di Firenze, Italy
 * Copyright (c) 2022 Tom Henderson
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Chandrakant Jena <chandrakant.barcelona@gmail.com>
 *          Tommaso Pecorella <tommaso.pecorella@unifi.it>
 *          Tom Henderson <tomh@tomh.org>
 */

// This test suite performs regression tests on the ns-3 Ping application.
//
// A simple topology is used:
//
// Node 0               Node 1
//   +----SimpleChannel---+
//
// - the SimpleChannel implements a one-way delay of 10 ms
// - each node has a SimpleNetDevice with a DataRate of 1 Gb/s
// - a ReceiveErrorModel can be configured to force packet drops
//
// The following are described in more detail below, inline with the test cases.
// Most tests listed below will test both IPv4 and IPv6 operation.
//
// 1. Unlimited pings, no losses, StopApplication () with no packets in flight
// 2. Unlimited pings, no losses, StopApplication () with 1 packet in flight
// 3. Test for operation of count attribute and exit time after all pings were received
// 4. Test the operation of the interval attribute
// 5. Test for behavior of pinging an unreachable host when the
//    network does not send an ICMP unreachable message.
// 6. Test pinging to IPv4 broadcast address and IPv6 all nodes multicast address
// 7. Test behavior of first reply lost in a count-limited configuration
// 8. Test behavior of second reply lost in a count-limited configuration
// 9. Test behavior of last reply lost in a count-limited configuration.

#include "ns3/boolean.h"
#include "ns3/data-rate.h"
#include "ns3/error-model.h"
#include "ns3/icmpv4-l4-protocol.h"
#include "ns3/icmpv4.h"
#include "ns3/inet-socket-address.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/ipv6-address-helper.h"
#include "ns3/ipv6-interface-container.h"
#include "ns3/log.h"
#include "ns3/neighbor-cache-helper.h"
#include "ns3/node-container.h"
#include "ns3/nstime.h"
#include "ns3/ping.h"
#include "ns3/simple-net-device-helper.h"
#include "ns3/simple-net-device.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/uinteger.h"

#include <list>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("PingTestSuite");

constexpr bool USEIPV6_FALSE = false;
constexpr bool USEIPV6_TRUE = true;

/// ICMP echo identifier used by the Ping application in the test (matches Ping::PING_ID).
constexpr uint16_t PING_ID{0xbeef};

/**
 * @ingroup ping
 * @defgroup ping-test ping tests
 */

/**
 * @ingroup ping-test
 * @ingroup tests
 *
 * @brief ping basic tests
 */
class PingTestCase : public TestCase
{
  public:
    /**
     * Constructor.
     * @param name TestCase name.
     * @param useIpv6 Use IPv6.
     */
    PingTestCase(std::string name, bool useIpv6);

    /**
     * Set the Simulation stop time.
     * @param stopTime The simulation stop time.
     */
    void SetSimulatorStopTime(Time stopTime);

    /**
     * Set the destination address (either IPv4 or IPv6).
     * @param address The destination address.
     */
    void SetDestinationAddress(Address address);

    /**
     * Set the PING start time.
     * @param startTime Ping start time.
     */
    void SetStartTime(Time startTime)
    {
        m_startTime = startTime;
    }

    /**
     * Set the PING stop time.
     * @param stopTime Ping stop time.
     */
    void SetStopTime(Time stopTime)
    {
        m_stopTime = stopTime;
    }

    /**
     * Set the number of pings to send.
     * @param count Number of pings to send.
     */
    void SetCount(uint32_t count)
    {
        m_countAttribute = count;
    }

    /**
     * Set the size of pings.
     * @param size Size of pings.
     */
    void SetSize(uint32_t size)
    {
        m_sizeAttribute = size;
    }

    /**
     * Set the interval of pings.
     * @param interval Interval of pings.
     */
    void SetInterval(Time interval)
    {
        m_interpacketInterval = interval;
    }

    /**
     * Set the packet drop list on the Ping node's interface
     * @param dropList packet drop list
     */
    void SetDropList(const std::list<uint32_t>& dropList)
    {
        m_dropList = dropList;
    }

    /**
     * Enable the check on Tx pings counted in Tx trace source.
     * @param expectedTx Expected Tx.
     */
    void CheckTraceTx(uint32_t expectedTx);

    /**
     * Enable the check on Rtt event count in Rtt trace source.
     * @param expectedRtt Expected Rtt.
     */
    void CheckTraceRtt(uint32_t expectedRtt);

    /**
     * Enable the check on Tx pings.
     * @param expectedReportTx Expected Tx.
     */
    void CheckReportTransmitted(uint32_t expectedReportTx);

    /**
     * Enable the check on Rx pings.
     * @param expectedReportRx Expected Rx.
     */
    void CheckReportReceived(uint32_t expectedReportRx);

    /**
     * Enable the check on Lost pings.
     * @param expectedReportLoss Expected Lost.
     */
    void CheckReportLoss(uint16_t expectedReportLoss);

    /**
     * Enable the check on average RTT.
     * @param expectedTime Expected RTT.
     */
    void CheckReportTime(Time expectedTime);

  private:
    void DoSetup() override;
    void DoTeardown() override;
    void DoRun() override;

    /**
     * Trace TX events.
     * @param seq Sequence number.
     * @param p Tx packet.
     */
    void TxTraceSink(uint16_t seq, Ptr<Packet> p);

    /**
     * Trace RTT events.
     * @param seq Sequence number.
     * @param rttSample RTT sample.
     */
    void RttTraceSink(uint16_t seq, Time rttSample);

    /**
     * Trace Drop events.
     * @param seq Sequence number.
     * @param reason Drop reason.
     */
    void DropTraceSink(uint16_t seq, Ping::DropReason reason);

    /**
     * Trace Report generation events.
     * @param report The report sample.
     */
    void ReportTraceSink(const Ping::PingReport& report);

    Address m_destination{Ipv4Address("10.0.0.2")}; //!< Destination address
    uint32_t m_mtu{1500};                           //!< Link MTU
    uint32_t m_countAttribute{0};                   //!< Number of pings to send
    uint32_t m_sizeAttribute{56};                   //!< Size of pings
    // The following are for setting expected counts for traced events
    uint32_t m_expectedTraceTx{0};  //!< Expected Tx trace sink calls
    uint32_t m_expectedTraceRtt{0}; //!< Expected Rtt trace sink calls
    // The following are counters for traced events
    uint32_t m_countTraceTx{0};  //!< Tx trace counter
    uint32_t m_countTraceRtt{0}; //!< Rtt trace counter
    // The following indicate whether trace counters should be checked
    bool m_checkTraceTx{false};  //!< Set to true to check the Tx number
    bool m_checkTraceRtt{false}; //!< Set to true to check the Rtt number
    // The following are for reported values in the final report
    uint32_t m_expectedReportTx{0};   //!< Expected reported Tx
    uint32_t m_expectedReportRx{0};   //!< Expected reported Rx
    uint16_t m_expectedReportLoss{0}; //!< Expected reported Loss
    Time m_expectedReportTime;        //!< Expected reported time
    // The following indicate whether report quantities should be checked
    bool m_checkReportTransmitted{false}; //!< Set to true to check the Tx number
    bool m_checkReportReceived{false};    //!< Set to true to check the Rx number
    bool m_checkReportLoss{false};        //!< Set to true to check the Loss number
    bool m_checkReportTime{false};        //!< Set to true to check the Time

    Time m_startTime{Seconds(1)};           //!< Start time
    Time m_stopTime{Seconds(5)};            //!< Stop time
    Time m_simulatorStopTime{Seconds(6)};   //!< Simulator stop time
    bool m_useIpv6{false};                  //!< Use IPv6 (true) or IPv4 (false)
    Time m_interpacketInterval{Seconds(1)}; //!< Time between pings
    Time m_lastTx;                          //!< Last ping Tx time
    std::list<uint32_t> m_dropList;         //!< Drop first reply (true)

    NodeContainer m_nodes;                   //!< The simulation nodes
    Ipv4InterfaceContainer m_ipv4Interfaces; //!< The IPv4 interfaces
    Ipv6InterfaceContainer m_ipv6Interfaces; //!< The IPv6 interfaces
    NetDeviceContainer m_devices;            //!< The NetDevices
};

PingTestCase::PingTestCase(std::string name, bool useIpv6)
    : TestCase(name),
      m_useIpv6(useIpv6)
{
}

void
PingTestCase::DoSetup()
{
    NS_LOG_FUNCTION(this);
    m_nodes.Create(2);
    SimpleNetDeviceHelper deviceHelper;
    deviceHelper.SetChannel("ns3::SimpleChannel", "Delay", TimeValue(MilliSeconds(10)));
    deviceHelper.SetDeviceAttribute("DataRate", DataRateValue(DataRate("1Gbps")));
    deviceHelper.SetNetDevicePointToPointMode(true);
    m_devices = deviceHelper.Install(m_nodes);
    // MTU is not an attribute, so set it directly
    Ptr<SimpleNetDevice> device0 = DynamicCast<SimpleNetDevice>(m_devices.Get(0));
    device0->SetMtu(m_mtu);
    Ptr<SimpleNetDevice> device1 = DynamicCast<SimpleNetDevice>(m_devices.Get(1));
    device1->SetMtu(m_mtu);

    InternetStackHelper internetHelper;
    if (!m_useIpv6)
    {
        internetHelper.SetIpv6StackInstall(false);
    }
    else
    {
        internetHelper.SetIpv4StackInstall(false);
    }
    internetHelper.Install(m_nodes);

    if (!m_useIpv6)
    {
        Ipv4AddressHelper ipv4AddrHelper;
        ipv4AddrHelper.SetBase("10.0.0.0", "255.255.255.0");
        m_ipv4Interfaces = ipv4AddrHelper.Assign(m_devices);
    }
    else
    {
        // This is to disable DAD, to simplify the drop testcase.
        m_nodes.Get(0)->GetObject<Icmpv6L4Protocol>()->SetAttribute("DAD", BooleanValue(false));
        m_nodes.Get(1)->GetObject<Icmpv6L4Protocol>()->SetAttribute("DAD", BooleanValue(false));

        Ipv6AddressHelper ipv6AddrHelper;
        ipv6AddrHelper.SetBase(Ipv6Address("2001:1::"), Ipv6Prefix(64));
        m_ipv6Interfaces = ipv6AddrHelper.Assign(m_devices);

        // This is a dirty trick to disable RS/RA, to simplify the drop testcase.
        // Forwarding interfaces do not send RS.
        m_nodes.Get(0)->GetObject<Ipv6L3Protocol>()->SetForwarding(1, true);
        m_nodes.Get(1)->GetObject<Ipv6L3Protocol>()->SetForwarding(1, true);
    }
}

void
PingTestCase::DoTeardown()
{
    Simulator::Destroy();
}

void
PingTestCase::CheckReportTransmitted(uint32_t expectedReportTx)
{
    m_checkReportTransmitted = true;
    m_expectedReportTx = expectedReportTx;
}

void
PingTestCase::CheckReportReceived(uint32_t expectedReportRx)
{
    m_checkReportReceived = true;
    m_expectedReportRx = expectedReportRx;
}

void
PingTestCase::CheckReportLoss(uint16_t expectedReportLoss)
{
    m_checkReportLoss = true;
    m_expectedReportLoss = expectedReportLoss;
}

void
PingTestCase::CheckReportTime(Time expectedTime)
{
    m_checkReportTime = true;
    m_expectedReportTime = expectedTime;
}

void
PingTestCase::RttTraceSink(uint16_t seq, Time rttSample)
{
    NS_LOG_FUNCTION(this << seq << rttSample.As(Time::MS));
    m_countTraceRtt++;
    if (m_sizeAttribute == 56)
    {
        if (!m_useIpv6)
        {
            // Each ping should be received with RTT of 20.0013 ms in this configuration
            // Check that each ping is within 20.0012 ms <= rttSample <= 20.0014 ms
            NS_TEST_ASSERT_MSG_EQ_TOL(
                rttSample.GetSeconds() * 1000,
                20.0013,
                0.0001,
                "Rtt sample not within 0.0001 ms of expected value of 20.0013 ms");
        }
        else
        {
            // Each ping should be received with RTT of 20.0017 ms in this configuration
            // Check that each ping is within 20.0016 ms <= rttSample <= 20.0018 ms
            NS_TEST_ASSERT_MSG_EQ_TOL(
                rttSample.GetSeconds() * 1000,
                20.0017,
                0.0001,
                "Rtt sample not within 0.0001 ms of expected value of 20.0017 ms");
        }
    }
}

void
PingTestCase::TxTraceSink(uint16_t seq, Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this << seq << p);
    m_countTraceTx++;
    if (m_lastTx.IsZero())
    {
        m_lastTx = Simulator::Now();
    }
    else
    {
        NS_TEST_ASSERT_MSG_EQ(Simulator::Now() - m_lastTx,
                              m_interpacketInterval,
                              "configured interval didn't match the observed interval");
        m_lastTx = Simulator::Now();
    }
}

void
PingTestCase::DropTraceSink(uint16_t seq, Ping::DropReason reason)
{
    NS_LOG_FUNCTION(this << static_cast<uint16_t>(reason));
    if (reason == Ping::DropReason::DROP_TIMEOUT)
    {
        NS_LOG_DEBUG("Drop due to timeout " << seq);
    }
    else if (reason == Ping::DROP_HOST_UNREACHABLE)
    {
        NS_LOG_DEBUG("Destination host is unreachable " << seq);
    }
    else if (reason == Ping::DROP_NET_UNREACHABLE)
    {
        NS_LOG_DEBUG("Destination network not reachable " << seq);
    }
}

void
PingTestCase::ReportTraceSink(const Ping::PingReport& report)
{
    NS_LOG_FUNCTION(this << report.m_transmitted << report.m_received << report.m_loss
                         << report.m_rttMin << report.m_rttMax);
    if (m_checkReportTransmitted)
    {
        NS_TEST_ASSERT_MSG_EQ(report.m_transmitted,
                              m_expectedReportTx,
                              "Report transmit count does not equal expected");
    }
    if (m_checkReportReceived)
    {
        NS_TEST_ASSERT_MSG_EQ(report.m_received,
                              m_expectedReportRx,
                              "Report receive count does not equal expected");
    }
    if (m_checkReportLoss)
    {
        NS_TEST_ASSERT_MSG_EQ(report.m_loss,
                              m_expectedReportLoss,
                              "Report lost count does not equal expected");
    }
    if (m_checkReportTime)
    {
        NS_TEST_ASSERT_MSG_EQ_TOL(Simulator::Now().GetMicroSeconds(),
                                  m_expectedReportTime.GetMicroSeconds(),
                                  1, // tolerance of 1 us
                                  "Report application not stopped at expected time");
    }
    NS_TEST_ASSERT_MSG_GT_OR_EQ(report.m_rttMin, 0, "minimum rtt should be greater than 0");
    NS_TEST_ASSERT_MSG_GT_OR_EQ(report.m_rttMax, 0, "maximum rtt should be greater than 0");
}

void
PingTestCase::CheckTraceTx(uint32_t expectedTx)
{
    m_checkTraceTx = true;
    m_expectedTraceTx = expectedTx;
}

void
PingTestCase::CheckTraceRtt(uint32_t expectedRtt)
{
    m_checkTraceRtt = true;
    m_expectedTraceRtt = expectedRtt;
}

void
PingTestCase::SetSimulatorStopTime(Time stopTime)
{
    m_simulatorStopTime = stopTime;
}

void
PingTestCase::SetDestinationAddress(Address address)
{
    m_destination = address;
}

void
PingTestCase::DoRun()
{
    NS_LOG_FUNCTION(this);
    Ptr<Ping> ping = CreateObject<Ping>();

    if (m_useIpv6)
    {
        Ipv6Address dst = Ipv6Address::ConvertFrom(m_destination);
        if (dst.IsLinkLocal() || dst.IsLinkLocalMulticast())
        {
            NS_LOG_LOGIC("Setting local interface address to "
                         << m_ipv6Interfaces.GetAddress(0, 0));
            ping->SetAttribute("InterfaceAddress", AddressValue(m_ipv6Interfaces.GetAddress(0, 0)));
        }
        else
        {
            NS_LOG_LOGIC("Setting local interface address to "
                         << m_ipv6Interfaces.GetAddress(0, 1));
            ping->SetAttribute("InterfaceAddress", AddressValue(m_ipv6Interfaces.GetAddress(0, 1)));
        }
        NS_LOG_LOGIC("Setting destination address to " << Ipv6Address::ConvertFrom(m_destination));
        ping->SetAttribute("Destination", AddressValue(m_destination));
    }
    else
    {
        NS_LOG_LOGIC("Setting destination address to " << Ipv4Address::ConvertFrom(m_destination));
        ping->SetAttribute("InterfaceAddress", AddressValue(m_ipv4Interfaces.GetAddress(0)));
        ping->SetAttribute("Destination", AddressValue(m_destination));
    }

    if (!m_dropList.empty())
    {
        NS_LOG_LOGIC("Setting drop list to list of size " << m_dropList.size());
        Ptr<ReceiveListErrorModel> errorModel = CreateObject<ReceiveListErrorModel>();
        Ptr<SimpleNetDevice> netDev = DynamicCast<SimpleNetDevice>(m_devices.Get(0));
        netDev->SetReceiveErrorModel(errorModel);
        errorModel->SetList(m_dropList);
    }

    ping->SetAttribute("Count", UintegerValue(m_countAttribute));
    ping->SetAttribute("Size", UintegerValue(m_sizeAttribute));
    ping->SetAttribute("Interval", TimeValue(m_interpacketInterval));
    ping->SetStartTime(m_startTime);
    ping->SetStopTime(m_stopTime);
    m_nodes.Get(0)->AddApplication(ping);
    ping->TraceConnectWithoutContext("Tx", MakeCallback(&PingTestCase::TxTraceSink, this));
    ping->TraceConnectWithoutContext("Rtt", MakeCallback(&PingTestCase::RttTraceSink, this));
    ping->TraceConnectWithoutContext("Drop", MakeCallback(&PingTestCase::DropTraceSink, this));
    ping->TraceConnectWithoutContext("Report", MakeCallback(&PingTestCase::ReportTraceSink, this));

    NeighborCacheHelper neighborCacheHelper;
    neighborCacheHelper.PopulateNeighborCache();

    Simulator::Stop(m_simulatorStopTime);
    Simulator::Run();

    if (m_checkTraceTx)
    {
        NS_TEST_ASSERT_MSG_EQ(m_countTraceTx,
                              m_expectedTraceTx,
                              "Traced Tx events do not equal expected");
    }
    if (m_checkTraceRtt)
    {
        NS_TEST_ASSERT_MSG_EQ(m_countTraceRtt,
                              m_expectedTraceRtt,
                              "Traced Rtt events do not equal expected");
    }
}

/**
 * @ingroup ping-test
 * @ingroup tests
 *
 * @brief ping TestSuite
 */
class PingTestSuite : public TestSuite
{
  public:
    PingTestSuite();
};

/**
 * @ingroup ping-test
 * @ingroup tests
 *
 * @brief Base test case exercising the Ping reaction to an ICMPv4 Destination
 *        Unreachable message.
 *
 * A two-node topology (10.0.0.0/24) is set up and a count-limited Ping (5 echo
 * requests at a 1 s interval starting at 1 s) is installed on node 0, targeting
 * node 1.  A concrete sub-class schedules one or more ICMPv4 Destination
 * Unreachable messages to be sent from node 1 shortly after the first echo
 * request is sent (at 1.01 s, before the first echo reply arrives at about
 * 1.02 s), then checks the resulting drop trace behavior via CheckResults.
 */
class PingIcmpv4UnreachableTestCase : public TestCase
{
  public:
    /**
     * Constructor.
     * @param name TestCase name.
     */
    PingIcmpv4UnreachableTestCase(std::string name);

  protected:
    /**
     * Install the topology, Ping application and traces, then run the
     * simulation and check the results.  Concrete sub-classes schedule their
     * ICMPv4 Destination Unreachable injections (via InjectIcmpv4DestUnreachable)
     * and implement CheckResults.
     */
    void DoRun() override;

    /**
     * Schedule an ICMPv4 Destination Unreachable message to be sent to node 0.
     *
     * @param time Simulation time at which to send it.
     * @param code Destination Unreachable code.
     * @param quotedId Identifier quoted in the original ICMP echo request.
     * @param quotedSeq Sequence number quoted in the original ICMP echo request.
     */
    void InjectIcmpv4DestUnreachable(Time time,
                                     uint8_t code,
                                     uint16_t quotedId,
                                     uint16_t quotedSeq);

    /**
     * Check the drop trace counters, to be implemented by the sub-class.
     */
    virtual void CheckResults() = 0;

    /**
     * Trace Drop events.
     * @param seq Sequence number.
     * @param reason Drop reason.
     */
    void DropTraceSink(uint16_t seq, Ping::DropReason reason);

    /**
     * Trace report generation.
     * @param report Ping report.
     */
    void ReportTraceSink(const Ping::PingReport& report);

  private:
    /**
     * Send an ICMPv4 Destination Unreachable message to node 0.
     *
     * @param code Destination Unreachable code.
     * @param quotedId Identifier quoted in the original ICMP echo request.
     * @param quotedSeq Sequence number quoted in the original ICMP echo request.
     */
    void SendIcmpv4DestUnreachable(uint8_t code, uint16_t quotedId, uint16_t quotedSeq);

  protected:
    NodeContainer m_nodes;                   //!< The simulation nodes
    Ipv4InterfaceContainer m_ipv4Interfaces; //!< The IPv4 interfaces
    uint32_t m_dropHostUnreachable{0};       //!< Count of host-unreachable drops
    uint32_t m_dropNetUnreachable{0};        //!< Count of net-unreachable drops
    uint32_t m_dropOther{0};                 //!< Count of any other drop reasons
    uint32_t m_hostUnreachableSeq{0};        //!< Sequence of the host-unreachable drop
    uint32_t m_netUnreachableSeq{0};         //!< Sequence of the net-unreachable drop
    bool m_reportObserved{false};            //!< True once report callback fires
};

PingIcmpv4UnreachableTestCase::PingIcmpv4UnreachableTestCase(std::string name)
    : TestCase(name)
{
}

void
PingIcmpv4UnreachableTestCase::DoRun()
{
    m_nodes.Create(2);

    SimpleNetDeviceHelper deviceHelper;
    deviceHelper.SetChannel("ns3::SimpleChannel", "Delay", TimeValue(MilliSeconds(10)));
    deviceHelper.SetDeviceAttribute("DataRate", DataRateValue(DataRate("1Gbps")));
    deviceHelper.SetNetDevicePointToPointMode(true);
    NetDeviceContainer devices = deviceHelper.Install(m_nodes);

    InternetStackHelper internetHelper;
    internetHelper.SetIpv6StackInstall(false);
    internetHelper.Install(m_nodes);

    Ipv4AddressHelper ipv4AddrHelper;
    ipv4AddrHelper.SetBase("10.0.0.0", "255.255.255.0");
    m_ipv4Interfaces = ipv4AddrHelper.Assign(devices);

    Ptr<Ping> ping = CreateObject<Ping>();
    ping->SetAttribute("InterfaceAddress", AddressValue(m_ipv4Interfaces.GetAddress(0)));
    ping->SetAttribute("Destination", AddressValue(m_ipv4Interfaces.GetAddress(1)));
    ping->SetAttribute("Count", UintegerValue(5));
    ping->SetStartTime(Seconds(1));
    ping->SetStopTime(Seconds(2.5));
    m_nodes.Get(0)->AddApplication(ping);
    ping->TraceConnectWithoutContext(
        "Drop",
        MakeCallback(&PingIcmpv4UnreachableTestCase::DropTraceSink, this));
    ping->TraceConnectWithoutContext(
        "Report",
        MakeCallback(&PingIcmpv4UnreachableTestCase::ReportTraceSink, this));

    Simulator::Stop(Seconds(3));
    Simulator::Run();

    CheckResults();

    Simulator::Destroy();
}

void
PingIcmpv4UnreachableTestCase::InjectIcmpv4DestUnreachable(Time time,
                                                           uint8_t code,
                                                           uint16_t quotedId,
                                                           uint16_t quotedSeq)
{
    Simulator::Schedule(time,
                        &PingIcmpv4UnreachableTestCase::SendIcmpv4DestUnreachable,
                        this,
                        code,
                        quotedId,
                        quotedSeq);
}

void
PingIcmpv4UnreachableTestCase::SendIcmpv4DestUnreachable(uint8_t code,
                                                         uint16_t quotedId,
                                                         uint16_t quotedSeq)
{
    Ptr<Socket> socket =
        Socket::CreateSocket(m_nodes.Get(1), TypeId::LookupByName("ns3::Ipv4RawSocketFactory"));
    socket->SetAttribute("Protocol", UintegerValue(Icmpv4L4Protocol::PROT_NUMBER));

    // Build the quoted ICMP Echo Request (the first 8 bytes of the original
    // datagram) using the ICMP header classes, so the flag/identifier/sequence
    // are set through the header API rather than by filling raw bytes.
    Icmpv4Echo quotedEcho;
    quotedEcho.SetSequenceNumber(quotedSeq);
    quotedEcho.SetIdentifier(quotedId);

    Ptr<Packet> quotedPayload = Create<Packet>();
    quotedPayload->AddHeader(quotedEcho);

    Icmpv4Header quotedIcmpHeader;
    quotedIcmpHeader.SetType(Icmpv4Header::ICMPV4_ECHO);
    quotedIcmpHeader.SetCode(0);
    quotedPayload->AddHeader(quotedIcmpHeader);

    Icmpv4DestinationUnreachable destUnreach;
    Ipv4Header quotedIpHeader;
    quotedIpHeader.SetSource(m_ipv4Interfaces.GetAddress(0));
    quotedIpHeader.SetDestination(m_ipv4Interfaces.GetAddress(1));
    quotedIpHeader.SetProtocol(Icmpv4L4Protocol::PROT_NUMBER);
    quotedIpHeader.SetPayloadSize(8);
    quotedIpHeader.SetTtl(64);
    destUnreach.SetHeader(quotedIpHeader);
    destUnreach.SetData(quotedPayload);

    Ptr<Packet> p = Create<Packet>();
    p->AddHeader(destUnreach);

    Icmpv4Header icmp;
    icmp.SetType(Icmpv4Header::ICMPV4_DEST_UNREACH);
    icmp.SetCode(code);
    if (Node::ChecksumEnabled())
    {
        icmp.EnableChecksum();
    }
    p->AddHeader(icmp);

    socket->SendTo(p, 0, InetSocketAddress(m_ipv4Interfaces.GetAddress(0), 0));
}

void
PingIcmpv4UnreachableTestCase::DropTraceSink(uint16_t seq, Ping::DropReason reason)
{
    if (reason == Ping::DROP_HOST_UNREACHABLE)
    {
        m_hostUnreachableSeq = seq;
        m_dropHostUnreachable++;
    }
    else if (reason == Ping::DROP_NET_UNREACHABLE)
    {
        m_netUnreachableSeq = seq;
        m_dropNetUnreachable++;
    }
    else
    {
        m_dropOther++;
    }
}

void
PingIcmpv4UnreachableTestCase::ReportTraceSink(const Ping::PingReport&)
{
    m_reportObserved = true;
}

/**
 * @ingroup ping-test
 * @ingroup tests
 *
 * @brief ping ICMPv4 Destination Host Unreachable reaction test
 *
 * A single ICMPv4 Destination Host Unreachable quoting the first echo request
 * (identifier 0xbeef, sequence 0) is injected.  The Ping must report exactly one
 * host-unreachable drop for that sequence and no other drops.
 */
class PingIcmpv4HostUnreachTestCase : public PingIcmpv4UnreachableTestCase
{
  public:
    /**
     * Constructor.
     */
    PingIcmpv4HostUnreachTestCase();

  private:
    void DoRun() override;
    void CheckResults() override;
};

PingIcmpv4HostUnreachTestCase::PingIcmpv4HostUnreachTestCase()
    : PingIcmpv4UnreachableTestCase("11. Test ping reaction to ICMPv4 Destination Host Unreachable")
{
}

void
PingIcmpv4HostUnreachTestCase::DoRun()
{
    InjectIcmpv4DestUnreachable(Seconds(1.01),
                                Icmpv4DestinationUnreachable::ICMPV4_HOST_UNREACHABLE,
                                PING_ID,
                                0);
    PingIcmpv4UnreachableTestCase::DoRun();
}

void
PingIcmpv4HostUnreachTestCase::CheckResults()
{
    NS_TEST_ASSERT_MSG_EQ(m_dropHostUnreachable, 1, "Expected one host unreachable drop");
    NS_TEST_ASSERT_MSG_EQ(m_hostUnreachableSeq,
                          0,
                          "Host-unreachable drop reported for the wrong sequence");
    NS_TEST_ASSERT_MSG_EQ(m_dropNetUnreachable, 0, "Expected no network unreachable drops");
    NS_TEST_ASSERT_MSG_EQ(m_dropOther, 0, "Expected no other drop reasons");
    NS_TEST_ASSERT_MSG_EQ(m_reportObserved, true, "Expected ping report callback to fire");
}

/**
 * @ingroup ping-test
 * @ingroup tests
 *
 * @brief ping ICMPv4 Destination Network Unreachable reaction test
 *
 * A single ICMPv4 Destination Network Unreachable quoting the first echo request
 * (identifier 0xbeef, sequence 0) is injected.  The Ping must report exactly one
 * network-unreachable drop for that sequence and no other drops.
 */
class PingIcmpv4NetUnreachTestCase : public PingIcmpv4UnreachableTestCase
{
  public:
    /**
     * Constructor.
     */
    PingIcmpv4NetUnreachTestCase();

  private:
    void DoRun() override;
    void CheckResults() override;
};

PingIcmpv4NetUnreachTestCase::PingIcmpv4NetUnreachTestCase()
    : PingIcmpv4UnreachableTestCase(
          "12. Test ping reaction to ICMPv4 Destination Network Unreachable")
{
}

void
PingIcmpv4NetUnreachTestCase::DoRun()
{
    InjectIcmpv4DestUnreachable(Seconds(1.01),
                                Icmpv4DestinationUnreachable::ICMPV4_NET_UNREACHABLE,
                                PING_ID,
                                0);
    PingIcmpv4UnreachableTestCase::DoRun();
}

void
PingIcmpv4NetUnreachTestCase::CheckResults()
{
    NS_TEST_ASSERT_MSG_EQ(m_dropNetUnreachable, 1, "Expected one network unreachable drop");
    NS_TEST_ASSERT_MSG_EQ(m_netUnreachableSeq,
                          0,
                          "Network-unreachable drop reported for the wrong sequence");
    NS_TEST_ASSERT_MSG_EQ(m_dropHostUnreachable, 0, "Expected no host unreachable drops");
    NS_TEST_ASSERT_MSG_EQ(m_dropOther, 0, "Expected no other drop reasons");
    NS_TEST_ASSERT_MSG_EQ(m_reportObserved, true, "Expected ping report callback to fire");
}

/**
 * @ingroup ping-test
 * @ingroup tests
 *
 * @brief ping ICMPv4 Destination Unreachable negative matching test
 *
 * Unsupported destination-unreachable codes and messages that do not quote one
 * of the Ping's own echo requests must be ignored without producing a drop.
 * Three injections are made:
 *  - port unreachable quoting echo 0 (valid id/seq, unsupported code),
 *  - host unreachable quoting an unknown identifier (0xbe00),
 *  - network unreachable quoting an out-of-range sequence (99).
 * The Ping must report no host-unreachable or network-unreachable drops.
 */
class PingIcmpv4DestUnreachNegativeTestCase : public PingIcmpv4UnreachableTestCase
{
  public:
    /**
     * Constructor.
     */
    PingIcmpv4DestUnreachNegativeTestCase();

  private:
    void DoRun() override;
    void CheckResults() override;
};

PingIcmpv4DestUnreachNegativeTestCase::PingIcmpv4DestUnreachNegativeTestCase()
    : PingIcmpv4UnreachableTestCase(
          "13. Test ping ignores unsupported and non-matching ICMPv4 destination unreachable")
{
}

void
PingIcmpv4DestUnreachNegativeTestCase::DoRun()
{
    // Unsupported code, but otherwise a valid matching message.
    InjectIcmpv4DestUnreachable(Seconds(1.01),
                                Icmpv4DestinationUnreachable::ICMPV4_PORT_UNREACHABLE,
                                PING_ID,
                                0);
    // Known code but quoting an identifier that is not ours.
    InjectIcmpv4DestUnreachable(Seconds(1.02),
                                Icmpv4DestinationUnreachable::ICMPV4_HOST_UNREACHABLE,
                                0xbe00,
                                0);
    // Known code but quoting an out-of-range (never sent) sequence number.
    InjectIcmpv4DestUnreachable(Seconds(1.03),
                                Icmpv4DestinationUnreachable::ICMPV4_NET_UNREACHABLE,
                                PING_ID,
                                99);

    PingIcmpv4UnreachableTestCase::DoRun();
}

void
PingIcmpv4DestUnreachNegativeTestCase::CheckResults()
{
    NS_TEST_ASSERT_MSG_EQ(m_dropHostUnreachable, 0, "Expected no host unreachable drops");
    NS_TEST_ASSERT_MSG_EQ(m_dropNetUnreachable, 0, "Expected no net unreachable drops");
    NS_TEST_ASSERT_MSG_EQ(m_dropOther, 0, "Expected no other drop reasons");
}

PingTestSuite::PingTestSuite()
    : TestSuite("ping", Type::UNIT)
{
    // 1. Unlimited pings, no losses, StopApplication () with no packets in flight
    //    Configuration:  Ping::Count = 0,  Ping::Interval = 1s, Ping start
    //                    time = 1s, Ping stop time = 5.5s
    //    Expected behavior:  Pings are sent at times 1, 2, 3, 4, 5 sec.  The
    //                        number sent equals number received, which equals 5.
    //    How validated:  PingReport trace is checked for number of packets
    //                    transmitted and received (5), and number of drops (0).
    auto testcase1v4 = new PingTestCase(
        "1. Unlimited pings, no losses, StopApplication () with no packets in flight IPv4",
        USEIPV6_FALSE);
    testcase1v4->SetStartTime(Seconds(1));
    testcase1v4->SetStopTime(Seconds(5.5));
    testcase1v4->SetCount(0);
    testcase1v4->CheckReportTransmitted(5);
    testcase1v4->CheckReportReceived(5);
    testcase1v4->CheckTraceTx(5);
    testcase1v4->SetDestinationAddress(Ipv4Address("10.0.0.2"));
    AddTestCase(testcase1v4, TestCase::Duration::QUICK);

    auto testcase1v6 = new PingTestCase(
        "1. Unlimited pings, no losses, StopApplication () with no packets in flight IPv6",
        USEIPV6_TRUE);
    testcase1v6->SetStartTime(Seconds(1));
    testcase1v6->SetStopTime(Seconds(5.5));
    testcase1v6->SetCount(0);
    testcase1v6->CheckReportTransmitted(5);
    testcase1v6->CheckReportReceived(5);
    testcase1v6->CheckTraceTx(5);
    testcase1v6->SetDestinationAddress(Ipv6Address("2001:1::200:ff:fe00:2"));
    AddTestCase(testcase1v6, TestCase::Duration::QUICK);

    // 2. Unlimited pings, no losses, StopApplication () with 1 packet in flight
    // Configuration:  Ping::Count = 0,  Ping::Interval = 1s, Ping start time =
    //                 1s, Ping stop time = 1.0001s
    // Expected behavior:  The application is stopped immediately after the
    //                     only ping is sent, and this ping will be reported
    //                     to be lost
    // How validated:  PingReport trace is checked for number of packets
    //                 transmitted (1) and received (0), and number of drops (0)
    auto testcase2v4 = new PingTestCase(
        "2. Unlimited pings, no losses, StopApplication () with 1 packet in flight IPv4",
        USEIPV6_FALSE);
    testcase2v4->SetStartTime(Seconds(1));
    testcase2v4->SetStopTime(Seconds(1.0001));
    testcase2v4->SetSimulatorStopTime(Seconds(5));
    testcase2v4->CheckReportTransmitted(1);
    testcase2v4->CheckReportReceived(0);
    testcase2v4->SetDestinationAddress(Ipv4Address("10.0.0.2"));
    AddTestCase(testcase2v4, TestCase::Duration::QUICK);

    auto testcase2v6 = new PingTestCase(
        "2. Unlimited pings, no losses, StopApplication () with 1 packet in flight IPv6",
        USEIPV6_TRUE);
    testcase2v6->SetStartTime(Seconds(1));
    testcase2v6->SetStopTime(Seconds(1.0001));
    testcase2v6->SetSimulatorStopTime(Seconds(5));
    testcase2v6->CheckReportTransmitted(1);
    testcase2v6->CheckReportReceived(0);
    testcase2v6->SetDestinationAddress(Ipv6Address("2001:1::200:ff:fe00:2"));
    AddTestCase(testcase2v6, TestCase::Duration::QUICK);

    // 3. Test for operation of count attribute and the resulting StopApplication time after all
    // pings were received. Limited pings, no losses, StopApplication () with no packet in flight
    // Configuration:  Ping::Count = 2,  Ping::Interval = 1s, Ping start time =
    //                 1s, Ping stop time = 5s
    // Expected behavior:  Pings are sent at 1 second intervals.  The
    //                     number sent (2) is equal to number received (2).
    //                     After receiving all pings, despite configuring
    //                     the StopApplication time to 5 seconds, the
    //                     application will immediately stop and generate
    //                     the report (at 2020001 us)
    // How validated:  PingReport trace is checked for number of packets
    //                 transmitted (2) and received (2),number of drops (0),
    //                 Report time will be MicroSeconds (2020001).
    uint32_t count = 2;
    uint32_t expectedTx = 2;
    auto testcase3v4 = new PingTestCase("3. Test for operation of count attribute and exit "
                                        "time after all pings were received, IPv4",
                                        USEIPV6_FALSE);
    testcase3v4->SetStartTime(Seconds(1));
    testcase3v4->SetStopTime(Seconds(5));
    testcase3v4->SetCount(count);
    testcase3v4->SetSimulatorStopTime(Seconds(6));
    testcase3v4->CheckReportTransmitted(2);
    testcase3v4->CheckReportReceived(2);
    testcase3v4->CheckReportTime(MicroSeconds(2020001));
    testcase3v4->CheckTraceTx(expectedTx);
    testcase3v4->SetDestinationAddress(Ipv4Address("10.0.0.2"));
    AddTestCase(testcase3v4, TestCase::Duration::QUICK);

    auto testcase3v6 = new PingTestCase("3. Test for operation of count attribute and exit "
                                        "time after all pings were received, IPv6",
                                        USEIPV6_TRUE);
    testcase3v6->SetStartTime(Seconds(1));
    testcase3v6->SetStopTime(Seconds(5));
    testcase3v6->SetCount(count);
    testcase3v6->SetSimulatorStopTime(Seconds(6));
    testcase3v6->CheckReportTransmitted(2);
    testcase3v6->CheckReportReceived(2);
    testcase3v6->CheckReportTime(MicroSeconds(2020001));
    testcase3v6->CheckTraceTx(expectedTx);
    testcase3v6->SetDestinationAddress(Ipv6Address("2001:1::200:ff:fe00:2"));
    AddTestCase(testcase3v6, TestCase::Duration::QUICK);

    // 4. Test for the operation of interval attribute for IPv4
    // Unlimited pings, no losses, StopApplication () with no packet in flight
    // Configuration:  Ping::Count = 0,  Ping::Interval = 3s, Ping start
    //                 time = 1s, Ping stop time = 6s
    // Expected behavior:  Pings are sent at the interval of 3 sec.
    //                     1st packet is sent at 1sec and 2nd packet at 4sec.
    //                     The number sent (2) is equal to number received (2)
    // How validated:  PingReport trace is checked for number of packets
    //                 transmitted (2) and received (2), and number of drops (0)
    Time interval = Seconds(3);
    auto testcase4v4 =
        new PingTestCase("4. Test for the operation of interval attribute for IPv4", USEIPV6_FALSE);
    testcase4v4->SetStartTime(Seconds(1));
    testcase4v4->SetStopTime(Seconds(5));
    testcase4v4->SetInterval(interval);
    testcase4v4->SetSimulatorStopTime(Seconds(6));
    testcase4v4->CheckReportTransmitted(2);
    testcase4v4->CheckReportReceived(2);
    testcase4v4->SetDestinationAddress(Ipv4Address("10.0.0.2"));
    AddTestCase(testcase4v4, TestCase::Duration::QUICK);

    auto testcase4v6 =
        new PingTestCase("4. Test for the operation of interval attribute for IPv6", USEIPV6_TRUE);
    testcase4v6->SetStartTime(Seconds(1));
    testcase4v6->SetStopTime(Seconds(5));
    testcase4v6->SetInterval(interval);
    testcase4v6->SetSimulatorStopTime(Seconds(6));
    testcase4v6->CheckReportTransmitted(2);
    testcase4v6->CheckReportReceived(2);
    testcase4v6->SetDestinationAddress(Ipv6Address("2001:1::200:ff:fe00:2"));
    AddTestCase(testcase4v6, TestCase::Duration::QUICK);

    // 5. Test for behavior of pinging an unreachable host when the
    //    network does not send an ICMP unreachable message.
    // Unlimited pings, StopApplication () with no packet in flight
    // Configuration:  Ping::Count = 5,  Ping start time = 1s
    //                 Ping stop time = 6.5s.  Ping to unknown destination.
    // Expected behavior:  By default, the timeout value is 1 second.  Ping
    //                     sends first packet at time 1 second, and does not
    //                     receive a response.  At the timeout (simulation time
    //                     2 seconds), ping should print out that there was
    //                     a timeout, and the drop trace should be fired.
    //                     At stop time, there should be three drops recorded
    //                     of type DROP_TIMEOUT, and not four,
    //                     because the packet sent at time 5 seconds has not
    //                     had enough time elapsed to timeout.
    // How validated:  PingReport trace is checked for number of packets
    //                 transmitted (5) and  received (0).
    //                 The packet loss rate should be checked to be 100 percent
    auto testcase5v4 =
        new PingTestCase("5. Test for behavior of ping to unreachable IPv4 address", USEIPV6_FALSE);
    testcase5v4->SetStartTime(Seconds(1));
    testcase5v4->SetStopTime(Seconds(5.5));
    testcase5v4->SetDestinationAddress(Ipv4Address("1.2.3.4"));
    testcase5v4->CheckReportTransmitted(5);
    testcase5v4->CheckReportReceived(0);
    testcase5v4->CheckReportLoss(100);
    AddTestCase(testcase5v4, TestCase::Duration::QUICK);

    auto testcase5v6 =
        new PingTestCase("5. Test for behavior of ping to unreachable IPv6 address", USEIPV6_TRUE);
    testcase5v6->SetStartTime(Seconds(1));
    testcase5v6->SetStopTime(Seconds(5.5));
    testcase5v6->SetDestinationAddress(Ipv6Address("2001:1::200:ff:fe00:3"));
    testcase5v6->CheckReportTransmitted(5);
    testcase5v6->CheckReportReceived(0);
    testcase5v6->CheckReportLoss(100);
    AddTestCase(testcase5v6, TestCase::Duration::QUICK);

    // 6. Test for behavior of pinging an broadcast (or multicast) address.
    // Limited pings, no losses, StopApplication () with no packet in flight
    // Configuration:  Ping::Count = 2,  Ping::Interval = 1s, Ping start time =
    //                 1s, Ping stop time = 5s
    // Expected behavior:  Pings are sent at 1 second intervals.  The
    //                     number sent (2) is equal to number received (2).
    //                     After receiving all pings, despite configuring
    //                     the StopApplication time to 5 seconds, the
    //                     application will immediately stop and generate
    //                     the report (at 2020001 us)
    // How validated:  PingReport trace is checked for number of packets
    //                 transmitted (2) and received (2),number of drops (0),
    //                 Report time will be MicroSeconds (2020001).
    auto testcase6v4 =
        new PingTestCase("6. Test for behavior of ping to broadcast IPv4 address", USEIPV6_FALSE);
    testcase6v4->SetStartTime(Seconds(1));
    testcase6v4->SetStopTime(Seconds(5.5));
    testcase6v4->SetDestinationAddress(Ipv4Address::GetBroadcast());
    testcase6v4->CheckReportTransmitted(5);
    testcase6v4->CheckReportReceived(5);
    testcase6v4->CheckReportLoss(0);
    AddTestCase(testcase6v4, TestCase::Duration::QUICK);

    auto testcase6v6 =
        new PingTestCase("6. Test for behavior of ping to all-nodes multicast IPv6 address",
                         USEIPV6_TRUE);
    testcase6v6->SetStartTime(Seconds(1));
    testcase6v6->SetStopTime(Seconds(5.5));
    testcase6v6->SetDestinationAddress(Ipv6Address::GetAllNodesMulticast());
    testcase6v6->CheckReportTransmitted(5);
    testcase6v6->CheckReportReceived(5);
    testcase6v6->CheckReportLoss(0);
    AddTestCase(testcase6v6, TestCase::Duration::QUICK);

    // 7. Test behavior of first reply lost in a count-limited configuration.
    // Limited pings, no losses, StopApplication () with no packet in flight
    // Configuration:  Ping::Count = 3,  Ping::Interval = 1s, Ping start time =
    //                 1s, Ping stop time = 5s.  Loss of first reply is forced.
    // Expected behavior:  Pings are sent at 1 second intervals at times
    //                     1s, 2s, 3s.  The ping Tx trace will record 3 sends,
    //                     but the RTT trace will record 2 receptions.
    //                     The application will stop after the third send
    //                     after waiting for 2*RttMax (40 ms).
    // How validated:  PingReport trace is checked for number of packets
    //                 transmitted (3) and received (2), loss percentage (33).
    //                 Ping Tx trace (3) and Rtt trace (2) are also checked.
    //                 Report time will be MicroSeconds (3040000).
    auto testcase7v4 = new PingTestCase(
        "7. Test behavior of first reply lost in a count-limited configuration, IPv4",
        USEIPV6_FALSE);
    std::list<uint32_t> dropList{0};
    testcase7v4->SetDropList(dropList);
    testcase7v4->SetStartTime(Seconds(1));
    testcase7v4->SetCount(3);
    testcase7v4->SetStopTime(Seconds(5));
    testcase7v4->CheckTraceTx(3);
    testcase7v4->CheckTraceRtt(2);
    testcase7v4->CheckReportTransmitted(3);
    testcase7v4->CheckReportReceived(2);
    testcase7v4->CheckReportLoss(33); // 33%
    testcase7v4->CheckReportTime(MicroSeconds(3040000));
    AddTestCase(testcase7v4, TestCase::Duration::QUICK);

    auto testcase7v6 = new PingTestCase(
        "7. Test behavior of first reply lost in a count-limited configuration, IPv6",
        USEIPV6_TRUE);
    testcase7v6->SetDropList(dropList);
    testcase7v6->SetStartTime(Seconds(1));
    testcase7v6->SetCount(3);
    testcase7v6->SetStopTime(Seconds(5));
    testcase7v6->SetDestinationAddress(Ipv6Address("2001:1::200:ff:fe00:2"));
    testcase7v6->CheckTraceTx(3);
    testcase7v6->CheckTraceRtt(2);
    testcase7v6->CheckReportTransmitted(3);
    testcase7v6->CheckReportReceived(2);
    testcase7v6->CheckReportLoss(33); // 33%
    testcase7v6->CheckReportTime(MicroSeconds(3040000));
    AddTestCase(testcase7v6, TestCase::Duration::QUICK);

    // 8. Test behavior of second reply lost in a count-limited configuration.
    // Limited pings, no losses, StopApplication () with no packet in flight
    // Configuration:  Ping::Count = 3,  Ping::Interval = 1s, Ping start time =
    //                 1s, Ping stop time = 5s.  Loss of second reply is forced.
    // Expected behavior:  Pings are sent at 1 second intervals at times
    //                     1s, 2s, 3s.  The ping Tx trace will record 3 sends,
    //                     but the RTT trace will record 2 receptions.
    //                     The application will stop after the third send
    //                     after waiting for 2*RttMax (40 ms).
    // How validated:  PingReport trace is checked for number of packets
    //                 transmitted (3) and received (2), loss percentage (33).
    //                 Ping Tx trace (3) and Rtt trace (2) are also checked.
    //                 Report time will be MicroSeconds (3040000).
    auto testcase8v4 = new PingTestCase(
        "8. Test behavior of second reply lost in a count-limited configuration, IPv4",
        USEIPV6_FALSE);
    std::list<uint32_t> dropList2{1};
    testcase8v4->SetDropList(dropList2);
    testcase8v4->SetStartTime(Seconds(1));
    testcase8v4->SetCount(3);
    testcase8v4->SetStopTime(Seconds(5));
    testcase8v4->CheckTraceTx(3);
    testcase8v4->CheckTraceRtt(2);
    testcase8v4->CheckReportTransmitted(3);
    testcase8v4->CheckReportReceived(2);
    testcase8v4->CheckReportLoss(33); // 33%
    testcase8v4->CheckReportTime(MicroSeconds(3040000));
    AddTestCase(testcase8v4, TestCase::Duration::QUICK);

    auto testcase8v6 = new PingTestCase(
        "8. Test behavior of second reply lost in a count-limited configuration, IPv6",
        USEIPV6_TRUE);
    testcase8v6->SetDropList(dropList2);
    testcase8v6->SetStartTime(Seconds(1));
    testcase8v6->SetCount(3);
    testcase8v6->SetStopTime(Seconds(5));
    testcase8v6->SetDestinationAddress(Ipv6Address("2001:1::200:ff:fe00:2"));
    testcase8v6->CheckTraceTx(3);
    testcase8v6->CheckTraceRtt(2);
    testcase8v6->CheckReportTransmitted(3);
    testcase8v6->CheckReportReceived(2);
    testcase8v6->CheckReportLoss(33); // 33%
    testcase8v6->CheckReportTime(MicroSeconds(3040000));
    AddTestCase(testcase8v6, TestCase::Duration::QUICK);

    // 9. Test behavior of last reply lost in a count-limited configuration.
    // Limited pings, no losses, StopApplication () with no packet in flight
    // Configuration:  Ping::Count = 3,  Ping::Interval = 1s, Ping start time =
    //                 1s, Ping stop time = 5s.  Loss of last reply is forced.
    // Expected behavior:  Pings are sent at 1 second intervals at times
    //                     1s, 2s, 3s.  The ping Tx trace will record 3 sends,
    //                     but the RTT trace will record 2 receptions.
    //                     The application will stop after the third send
    //                     after waiting for 2*RttMax (40 ms).
    // How validated:  PingReport trace is checked for number of packets
    //                 transmitted (3) and received (2), loss percentage (33).
    //                 Ping Tx trace (3) and Rtt trace (2) are also checked.
    //                 Report time will be MicroSeconds (3040000).
    auto testcase9v4 = new PingTestCase(
        "9. Test behavior of last reply lost in a count-limited configuration, IPv4",
        USEIPV6_FALSE);
    std::list<uint32_t> dropList3{2};
    testcase9v4->SetDropList(dropList3);
    testcase9v4->SetStartTime(Seconds(1));
    testcase9v4->SetCount(3);
    testcase9v4->SetStopTime(Seconds(5));
    testcase9v4->CheckTraceTx(3);
    testcase9v4->CheckTraceRtt(2);
    testcase9v4->CheckReportTransmitted(3);
    testcase9v4->CheckReportReceived(2);
    testcase9v4->CheckReportLoss(33); // 33%
    testcase9v4->CheckReportTime(MicroSeconds(3040000));
    AddTestCase(testcase9v4, TestCase::Duration::QUICK);

    auto testcase9v6 = new PingTestCase(
        "9. Test behavior of last reply lost in a count-limited configuration, IPv6",
        USEIPV6_TRUE);
    testcase9v6->SetDropList(dropList3);
    testcase9v6->SetStartTime(Seconds(1));
    testcase9v6->SetCount(3);
    testcase9v6->SetStopTime(Seconds(5));
    testcase9v6->SetDestinationAddress(Ipv6Address("2001:1::200:ff:fe00:2"));
    testcase9v6->CheckTraceTx(3);
    testcase9v6->CheckTraceRtt(2);
    testcase9v6->CheckReportTransmitted(3);
    testcase9v6->CheckReportReceived(2);
    testcase9v6->CheckReportLoss(33); // 33%
    testcase9v6->CheckReportTime(MicroSeconds(3040000));
    AddTestCase(testcase9v6, TestCase::Duration::QUICK);

    // 10. Test for behavior of pinging with a big size that causes fragmentation
    //    Configuration:  Ping::Count = 5,  Ping::Interval = 1s, Ping start
    //                    time = 1s, Ping stop time = 6.5s,
    //                    Payload size = 60000B
    //    Expected behavior:  Pings are sent at times 1, 2, 3, 4, 5 sec.  The
    //                        number sent equals number received, which equals 5.
    //    How validated:  PingReport trace is checked for number of packets
    //                    transmitted and received (5), and number of drops (0).
    auto testcase10v4 = new PingTestCase("10. 5 pings, heavy payload, no losses, "
                                         "StopApplication () with no packets in flight IPv4",
                                         USEIPV6_FALSE);
    testcase10v4->SetStartTime(Seconds(1));
    testcase10v4->SetStopTime(Seconds(6.5));
    testcase10v4->SetCount(5);
    testcase10v4->SetSize(60000);
    testcase10v4->CheckReportTransmitted(5);
    testcase10v4->CheckReportReceived(5);
    testcase10v4->CheckTraceTx(5);
    testcase10v4->SetDestinationAddress(Ipv4Address("10.0.0.2"));
    AddTestCase(testcase10v4, TestCase::Duration::QUICK);

    auto testcase10v6 = new PingTestCase("10. 5 pings, heavy payload,  no losses, "
                                         "StopApplication () with no packets in flight IPv6",
                                         USEIPV6_TRUE);
    testcase10v6->SetStartTime(Seconds(1));
    testcase10v6->SetStopTime(Seconds(6.5));
    testcase10v6->SetCount(5);
    testcase10v6->SetSize(60000);
    testcase10v6->CheckReportTransmitted(5);
    testcase10v6->CheckReportReceived(5);
    testcase10v6->CheckTraceTx(5);
    testcase10v6->SetDestinationAddress(Ipv6Address("2001:1::200:ff:fe00:2"));
    AddTestCase(testcase10v6, TestCase::Duration::QUICK);

    AddTestCase(new PingIcmpv4HostUnreachTestCase(), TestCase::Duration::QUICK);
    AddTestCase(new PingIcmpv4NetUnreachTestCase(), TestCase::Duration::QUICK);
    AddTestCase(new PingIcmpv4DestUnreachNegativeTestCase(), TestCase::Duration::QUICK);
}

static PingTestSuite pingTestSuite; //!< Static variable for test initialization
