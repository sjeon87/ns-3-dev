/*
 * Copyright (c) 2026 Somaiya Vidyavihar University, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manmita Das <das.manmita12@gmail.com>
 */

#include "ns3/ethernet-channel.h"
#include "ns3/ethernet-net-device.h"
#include "ns3/ethernet-phy.h"
#include "ns3/ethernet-switch-fcfs-scheduler.h"
#include "ns3/ethernet-switch.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/test.h"

#include <vector>

using namespace ns3;
using namespace ns3::ethernet;

/**
 * @ingroup ethernet-test
 * @ingroup tests
 *
 * @brief End-to-end test of the Ethernet switch FCFS scheduler.
 *
 * The test verifies:
 *
 * FCFS ordering independently for each output port.
 * A busy output port does not block transmission on another output port.
 *
 * Packets enter the switch in this order:
 *
 *     pkt1 -> port 1
 *     pkt2 -> port 1
 *     pkt3 -> port 2
 *     pkt4 -> port 2
 *
 * Expected ordering:
 *
 *     port 1: pkt1 -> pkt2
 *     port 2: pkt3 -> pkt4
 *
 * pkt1 is intentionally larger than pkt3/pkt4 so that port 1 remains
 * busy while port 2 can independently transmit pkt4.
 */
class EthernetSwitchFcfsTestCase : public TestCase
{
  public:
    EthernetSwitchFcfsTestCase();
    ~EthernetSwitchFcfsTestCase() override;

  private:
    void DoRun() override;

    /**
     * @brief PHY TxBegin callback for switch port 1.
     * @param packet The packet being transmitted.
     */
    void Port1TxBegin(Ptr<const Packet> packet);

    /**
     * @brief PHY TxEnd callback for switch port 1.
     * @param packet The packet that finished transmitting.
     */
    void Port1TxEnd(Ptr<const Packet> packet);

    /**
     * @brief PHY TxBegin callback for switch port 2.
     * @param packet The packet being transmitted.
     */
    void Port2TxBegin(Ptr<const Packet> packet);

    /**
     * @brief PHY TxEnd callback for switch port 2.
     * @param packet The packet that finished transmitting.
     */
    void Port2TxEnd(Ptr<const Packet> packet);

    /**
     * @brief Record a packet transmission start for a given port.
     * @param uids The vector of packet UIDs to record into.
     * @param times The vector of packet transmission start times to record into.
     * @param packet The packet being transmitted.
     */
    void RecordTxBegin(std::vector<uint64_t>& uids,
                       std::vector<Time>& times,
                       Ptr<const Packet> packet);

    std::vector<uint64_t> m_port1TxUids;   //!< Vector of packet UIDs transmitted on port 1
    std::vector<uint64_t> m_port2TxUids;   //!< Vector of packet UIDs transmitted on port 2
    std::vector<Time> m_port1TxBeginTimes; //!< Vector of packet transmission start times on port 1
    std::vector<Time> m_port2TxBeginTimes; //!< Vector of packet transmission start times on port 2

    uint64_t m_pkt1Uid = 0; //!< UID of the first packet transmitted on port 1
    Time
        m_pkt1TxEndTime; //!< Time when the first packet transmitted on port 1 finished transmitting
};

EthernetSwitchFcfsTestCase::EthernetSwitchFcfsTestCase()
    : TestCase("Ethernet switch end-to-end FCFS test")
{
}

EthernetSwitchFcfsTestCase::~EthernetSwitchFcfsTestCase()
{
}

void
EthernetSwitchFcfsTestCase::RecordTxBegin(std::vector<uint64_t>& uids,
                                          std::vector<Time>& times,
                                          Ptr<const Packet> packet)
{
    uint64_t uid = packet->GetUid();

    uids.push_back(uid);
    times.push_back(Simulator::Now());
}

void
EthernetSwitchFcfsTestCase::Port1TxBegin(Ptr<const Packet> packet)
{
    uint64_t uid = packet->GetUid();

    if (uid == 0)
    {
        return;
    }

    m_port1TxUids.push_back(uid);
    m_port1TxBeginTimes.push_back(Simulator::Now());

    NS_LOG_UNCOND("Port 1 TxBegin: packet " << uid << " at " << Simulator::Now());
}

void
EthernetSwitchFcfsTestCase::Port1TxEnd(Ptr<const Packet> packet)
{
    if (packet->GetUid() == m_pkt1Uid)
    {
        m_pkt1TxEndTime = Simulator::Now();

        NS_LOG_UNCOND("Port 1 TxEnd: pkt1 " << packet->GetUid() << " at " << m_pkt1TxEndTime);
    }
}

void
EthernetSwitchFcfsTestCase::Port2TxBegin(Ptr<const Packet> packet)
{
    uint64_t uid = packet->GetUid();

    if (uid == 0)
    {
        return;
    }

    m_port2TxUids.push_back(uid);
    m_port2TxBeginTimes.push_back(Simulator::Now());

    NS_LOG_UNCOND("Port 2 TxBegin: packet " << uid << " at " << Simulator::Now());
}

void
EthernetSwitchFcfsTestCase::Port2TxEnd(Ptr<const Packet> packet)
{
    NS_LOG_UNCOND("Port 2 TxEnd: packet " << packet->GetUid() << " at " << Simulator::Now());
}

void
EthernetSwitchFcfsTestCase::DoRun()
{
    //
    // Topology:
    //
    //                         +----------------+
    //                         |    Ethernet    |
    //                         |     Switch     |
    //                         |                |
    // Host A -----------------| port 0         |
    //                         |                |
    // Host C -----------------| port 1         |
    //                         |                |
    // Host D -----------------| port 2         |
    //                         +----------------+
    //
    // Host A is the ingress source.
    //
    // Host C is connected to output port 1.
    // Host D is connected to output port 2.
    //

    Ptr<Node> nodeA = CreateObject<Node>();
    Ptr<Node> nodeC = CreateObject<Node>();
    Ptr<Node> nodeD = CreateObject<Node>();

    Ptr<Node> switchNode = CreateObject<Node>();

    Ptr<EthernetNetDevice> devA = CreateObject<EthernetNetDevice>();

    Ptr<EthernetNetDevice> devC = CreateObject<EthernetNetDevice>();

    Ptr<EthernetNetDevice> devD = CreateObject<EthernetNetDevice>();

    Ptr<EthernetNetDevice> switchPort0 = CreateObject<EthernetNetDevice>();

    Ptr<EthernetNetDevice> switchPort1 = CreateObject<EthernetNetDevice>();

    Ptr<EthernetNetDevice> switchPort2 = CreateObject<EthernetNetDevice>();

    nodeA->AddDevice(devA);
    nodeC->AddDevice(devC);
    nodeD->AddDevice(devD);

    switchNode->AddDevice(switchPort0);
    switchNode->AddDevice(switchPort1);
    switchNode->AddDevice(switchPort2);

    Ptr<EthernetChannel> channel0 = CreateObject<EthernetChannel>();

    Ptr<EthernetChannel> channel1 = CreateObject<EthernetChannel>();

    Ptr<EthernetChannel> channel2 = CreateObject<EthernetChannel>();

    devA->Attach(channel0);
    switchPort0->Attach(channel0);

    devC->Attach(channel1);
    switchPort1->Attach(channel1);

    devD->Attach(channel2);
    switchPort2->Attach(channel2);

    devA->SetAddress(Mac48Address::Allocate());
    devC->SetAddress(Mac48Address::Allocate());
    devD->SetAddress(Mac48Address::Allocate());

    switchPort0->SetAddress(Mac48Address::Allocate());
    switchPort1->SetAddress(Mac48Address::Allocate());
    switchPort2->SetAddress(Mac48Address::Allocate());

    nodeA->AddDevice(devA);
    nodeC->AddDevice(devC);
    nodeD->AddDevice(devD);

    Ptr<EthernetSwitch> switchDevice = CreateObject<EthernetSwitch>();

    Ptr<EthernetSwitchFcfsScheduler> scheduler = CreateObject<EthernetSwitchFcfsScheduler>();

    switchDevice->SetScheduler(scheduler);

    switchDevice->AddPort(switchPort0);
    switchDevice->AddPort(switchPort1);
    switchDevice->AddPort(switchPort2);

    Ptr<Packet> learningPacketC = Create<Packet>(100);

    Simulator::Schedule(Seconds(0.1),
                        &EthernetNetDevice::Send,
                        devC,
                        learningPacketC,
                        devA->GetAddress(),
                        0x0800);

    Ptr<Packet> learningPacketD = Create<Packet>(100);

    Simulator::Schedule(Seconds(0.2),
                        &EthernetNetDevice::Send,
                        devD,
                        learningPacketD,
                        devA->GetAddress(),
                        0x0800);

    //
    // ------------------------------------------------------------
    // Create test packets.
    // ------------------------------------------------------------
    //
    // pkt1 -> port 1
    // pkt2 -> port 1
    // pkt3 -> port 2
    // pkt4 -> port 2
    //
    // pkt1 is intentionally large so that port 1 remains busy while
    // port 2 independently processes pkt3 and pkt4.
    //

    Ptr<Packet> packet1 = Create<Packet>(1500);
    Ptr<Packet> packet2 = Create<Packet>(1500);

    Ptr<Packet> packet3 = Create<Packet>(100);
    Ptr<Packet> packet4 = Create<Packet>(100);

    uint64_t packet1Uid = packet1->GetUid();
    uint64_t packet2Uid = packet2->GetUid();
    uint64_t packet3Uid = packet3->GetUid();
    uint64_t packet4Uid = packet4->GetUid();

    m_pkt1Uid = packet1Uid;

    switchPort1->GetPhy()->TraceConnectWithoutContext(
        "PhyTxBegin",
        MakeCallback(&EthernetSwitchFcfsTestCase::Port1TxBegin, this));

    switchPort1->GetPhy()->TraceConnectWithoutContext(
        "PhyTxEnd",
        MakeCallback(&EthernetSwitchFcfsTestCase::Port1TxEnd, this));

    switchPort2->GetPhy()->TraceConnectWithoutContext(
        "PhyTxBegin",
        MakeCallback(&EthernetSwitchFcfsTestCase::Port2TxBegin, this));

    switchPort2->GetPhy()->TraceConnectWithoutContext(
        "PhyTxEnd",
        MakeCallback(&EthernetSwitchFcfsTestCase::Port2TxEnd, this));

    Simulator::Schedule(Seconds(1.0),
                        &EthernetNetDevice::Send,
                        devA,
                        packet1,
                        devC->GetAddress(),
                        0x0800);

    Simulator::Schedule(Seconds(1.0),
                        &EthernetNetDevice::Send,
                        devA,
                        packet2,
                        devC->GetAddress(),
                        0x0800);

    Simulator::Schedule(Seconds(1.0),
                        &EthernetNetDevice::Send,
                        devA,
                        packet3,
                        devD->GetAddress(),
                        0x0800);

    Simulator::Schedule(Seconds(1.0),
                        &EthernetNetDevice::Send,
                        devA,
                        packet4,
                        devD->GetAddress(),
                        0x0800);

    Simulator::Stop(Seconds(2.0));
    Simulator::Run();

    std::vector<uint64_t> port1Uids;
    std::vector<Time> port1Times;

    for (size_t i = 0; i < m_port1TxUids.size(); ++i)
    {
        uint64_t uid = m_port1TxUids[i];

        if (uid == packet1Uid || uid == packet2Uid)
        {
            port1Uids.push_back(uid);
            port1Times.push_back(m_port1TxBeginTimes[i]);
        }
    }

    std::vector<uint64_t> port2Uids;
    std::vector<Time> port2Times;

    for (size_t i = 0; i < m_port2TxUids.size(); ++i)
    {
        uint64_t uid = m_port2TxUids[i];

        if (uid == packet3Uid || uid == packet4Uid)
        {
            port2Uids.push_back(uid);
            port2Times.push_back(m_port2TxBeginTimes[i]);
        }
    }

    NS_TEST_ASSERT_MSG_EQ(port1Uids.size(), 2, "Port 1 should transmit exactly pkt1 and pkt2");

    NS_TEST_ASSERT_MSG_EQ(port2Uids.size(), 2, "Port 2 should transmit exactly pkt3 and pkt4");

    NS_TEST_ASSERT_MSG_EQ(port1Uids[0], packet1Uid, "Port 1 must transmit pkt1 before pkt2");

    NS_TEST_ASSERT_MSG_EQ(port1Uids[1], packet2Uid, "Port 1 must transmit pkt2 after pkt1");

    NS_TEST_ASSERT_MSG_EQ(port2Uids[0], packet3Uid, "Port 2 must transmit pkt3 before pkt4");

    NS_TEST_ASSERT_MSG_EQ(port2Uids[1], packet4Uid, "Port 2 must transmit pkt4 after pkt3");

    NS_TEST_ASSERT_MSG_LT(port2Times[1],
                          m_pkt1TxEndTime,
                          "Port 2 should transmit pkt4 while port 1 is still "
                          "transmitting pkt1");

    NS_LOG_UNCOND("FCFS and independent output-port scheduling verified");

    NS_LOG_UNCOND("Port 1:");
    NS_LOG_UNCOND("  pkt1 = " << port1Uids[0] << " at " << port1Times[0]);
    NS_LOG_UNCOND("  pkt2 = " << port1Uids[1] << " at " << port1Times[1]);

    NS_LOG_UNCOND("Port 2:");
    NS_LOG_UNCOND("  pkt3 = " << port2Uids[0] << " at " << port2Times[0]);
    NS_LOG_UNCOND("  pkt4 = " << port2Uids[1] << " at " << port2Times[1]);

    NS_LOG_UNCOND("pkt1 TxEnd = " << m_pkt1TxEndTime);
    NS_LOG_UNCOND("pkt4 TxBegin = " << port2Times[1]);

    Simulator::Destroy();
}

/**
 * @ingroup ethernet-test
 *
 * @brief Ethernet switch FCFS test suite.
 */
class EthernetSwitchFcfsTestSuite : public TestSuite
{
  public:
    EthernetSwitchFcfsTestSuite();
};

EthernetSwitchFcfsTestSuite::EthernetSwitchFcfsTestSuite()
    : TestSuite("ethernet-switch-fcfs-test", Type::UNIT)
{
    AddTestCase(new EthernetSwitchFcfsTestCase, TestCase::Duration::QUICK);
}

static EthernetSwitchFcfsTestSuite g_ethernetSwitchFcfsTestSuite;
