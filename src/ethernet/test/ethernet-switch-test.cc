/*
 * Copyright (c) 2026 Somaiya Vidyavihar University, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manmita Das <das.manmita12@gmail.com>
 */

#include "ns3/drop-tail-queue.h"
#include "ns3/ethernet-channel.h"
#include "ns3/ethernet-header.h"
#include "ns3/ethernet-mac.h"
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
 * @brief A switch with a given number of hosts attached to it, one per port.
 *
 * Each host is an EthernetNetDevice sharing a channel with the switch port it
 * is attached to. Frames received by a host are counted, so that a test can
 * tell which of the hosts a frame reached.
 */
class EthernetSwitchTestCase : public TestCase
{
  public:
    /**
     * @brief Construct a test case.
     * @param name The name of the test case.
     */
    EthernetSwitchTestCase(std::string name);

  protected:
    /**
     * @brief Build a switch with hosts attached to it.
     * @param nHosts The number of hosts to attach to the switch.
     */
    void BuildTopology(uint32_t nHosts);

    /**
     * @brief Count a frame received by a host.
     *
     * Counted promiscuously, so that the frames a host is reached by but is
     * not addressed to, which are the ones flooding produces, are seen too.
     *
     * @param index The index of the host that received the frame.
     * @return Always true, the frame having been consumed.
     */
    bool CountRx(uint32_t index,
                 Ptr<NetDevice>,
                 Ptr<const Packet>,
                 uint16_t,
                 const Address&,
                 const Address&,
                 NetDevice::PacketType);

    /**
     * @brief Count a frame that the switch gave up on forwarding.
     */
    void CountForwardDrop(Ptr<const Packet>);

    /**
     * @brief Count a frame that a switch port dropped before transmitting it.
     */
    void CountPortTxDrop(Ptr<const Packet>);

    Ptr<EthernetSwitch> m_switch;                //!< The switch under test
    std::vector<Ptr<EthernetNetDevice>> m_hosts; //!< The hosts attached to the switch
    std::vector<Ptr<EthernetNetDevice>> m_ports; //!< The switch ports the hosts attach to
    std::vector<uint32_t> m_rxCount;             //!< Frames received by each host
    uint32_t m_forwardDrops{0};                  //!< Frames the switch gave up on
    uint32_t m_portTxDrops{0};                   //!< Frames a port dropped before transmitting
};

EthernetSwitchTestCase::EthernetSwitchTestCase(std::string name)
    : TestCase(name)
{
}

bool
EthernetSwitchTestCase::CountRx(uint32_t index,
                                Ptr<NetDevice>,
                                Ptr<const Packet>,
                                uint16_t,
                                const Address&,
                                const Address&,
                                NetDevice::PacketType)
{
    ++m_rxCount[index];
    return true;
}

void
EthernetSwitchTestCase::CountForwardDrop(Ptr<const Packet>)
{
    ++m_forwardDrops;
}

void
EthernetSwitchTestCase::CountPortTxDrop(Ptr<const Packet>)
{
    ++m_portTxDrops;
}

void
EthernetSwitchTestCase::BuildTopology(uint32_t nHosts)
{
    Ptr<Node> switchNode = CreateObject<Node>();

    m_switch = CreateObject<EthernetSwitch>();
    m_switch->SetScheduler(CreateObject<EthernetSwitchFcfsScheduler>());
    m_switch->SetNode(switchNode);
    switchNode->AggregateObject(m_switch);

    m_switch->TraceConnectWithoutContext(
        "ForwardDrop",
        MakeCallback(&EthernetSwitchTestCase::CountForwardDrop, this));

    m_rxCount.assign(nHosts, 0);

    for (uint32_t i = 0; i < nHosts; ++i)
    {
        Ptr<Node> hostNode = CreateObject<Node>();
        Ptr<EthernetNetDevice> host = CreateObject<EthernetNetDevice>();
        Ptr<EthernetNetDevice> port = CreateObject<EthernetNetDevice>();
        Ptr<EthernetChannel> channel = CreateObject<EthernetChannel>();

        host->SetAddress(Mac48Address::Allocate());
        port->SetAddress(Mac48Address::Allocate());

        hostNode->AddDevice(host);
        port->SetNode(switchNode);
        port->SetIfIndex(i);
        Simulator::ScheduleWithContext(switchNode->GetId(),
                                       Seconds(0),
                                       &NetDevice::Initialize,
                                       port);

        host->Attach(channel);
        port->Attach(channel);

        // Pin the link type, so that the test does not depend on the default.
        host->SetMaxSupportedLinkType(EthernetLinkType::G10_T);
        port->SetMaxSupportedLinkType(EthernetLinkType::G10_T);

        host->SetPromiscReceiveCallback(
            MakeCallback(&EthernetSwitchTestCase::CountRx, this).Bind(i));

        m_switch->AddPort(port);

        m_hosts.push_back(host);
        m_ports.push_back(port);
    }
}

/**
 * @ingroup ethernet-test
 * @ingroup tests
 *
 * @brief Test that the switch learns addresses, and floods until it has.
 *
 * A frame sent to an address the switch has not learned yet is expected to
 * reach every host other than the one that sent it. Once the address has been
 * learned, a frame sent to it is expected to reach that host alone.
 */
class EthernetSwitchLearningTestCase : public EthernetSwitchTestCase
{
  public:
    EthernetSwitchLearningTestCase();

  private:
    void DoRun() override;
};

EthernetSwitchLearningTestCase::EthernetSwitchLearningTestCase()
    : EthernetSwitchTestCase("Unknown addresses are flooded, learned addresses are forwarded")
{
}

void
EthernetSwitchLearningTestCase::DoRun()
{
    const uint32_t nHosts = 3;
    const uint32_t payloadSize = 100;

    BuildTopology(nHosts);

    //
    // Nothing has been learned yet, so the frame is flooded to every host
    // other than the one that sent it.
    //
    Simulator::Schedule(MilliSeconds(1),
                        &EthernetNetDevice::Send,
                        m_hosts[0],
                        Create<Packet>(payloadSize),
                        m_hosts[1]->GetAddress(),
                        0x0800);

    Simulator::Stop(MilliSeconds(2));
    Simulator::Run();

    NS_TEST_ASSERT_MSG_EQ(m_rxCount[1], 1u, "The addressed host should receive the flooded frame");
    NS_TEST_ASSERT_MSG_EQ(m_rxCount[2], 1u, "Every other host should receive the flooded frame");
    NS_TEST_ASSERT_MSG_EQ(m_rxCount[0], 0u, "The sending host should not receive its own frame");

    Mac48Address host0 = Mac48Address::ConvertFrom(m_hosts[0]->GetAddress());
    NS_TEST_ASSERT_MSG_EQ(m_switch->LookupMacAddress(host0),
                          m_ports[0],
                          "The address of the sending host should have been learned");

    //
    // The address of host 0 was learned from the frame it sent, so the reply
    // is forwarded to its port alone.
    //
    Simulator::Schedule(MilliSeconds(3),
                        &EthernetNetDevice::Send,
                        m_hosts[1],
                        Create<Packet>(payloadSize),
                        m_hosts[0]->GetAddress(),
                        0x0800);

    Simulator::Stop(MilliSeconds(4));
    Simulator::Run();

    NS_TEST_ASSERT_MSG_EQ(m_rxCount[0], 1u, "The addressed host should receive the reply");
    NS_TEST_ASSERT_MSG_EQ(m_rxCount[2], 1u, "The reply should not be flooded to the other hosts");

    NS_TEST_ASSERT_MSG_EQ(m_forwardDrops, 0u, "No frame should have been dropped by the switch");

    Simulator::Destroy();
}

/**
 * @ingroup ethernet-test
 * @ingroup tests
 *
 * @brief Test that the switch respects the transmit queue limit of its ports.
 *
 * More frames than the transmit queue of the output port can hold are pushed
 * through the switch at once. The switch is expected to hand the port only as
 * many frames as it has room for, and to give up on the rest itself rather
 * than leaving the port to drop them.
 */
class EthernetSwitchQueueLimitTestCase : public EthernetSwitchTestCase
{
  public:
    EthernetSwitchQueueLimitTestCase();

  private:
    void DoRun() override;
};

EthernetSwitchQueueLimitTestCase::EthernetSwitchQueueLimitTestCase()
    : EthernetSwitchTestCase("Frames are not forwarded to a port whose transmit queue is full")
{
}

void
EthernetSwitchQueueLimitTestCase::DoRun()
{
    const uint32_t nHosts = 2;
    const uint32_t payloadSize = 1000;
    const uint32_t queueLimit = 4;
    const uint32_t nFrames = 20;

    BuildTopology(nHosts);

    //
    // Give the port that the frames leave the switch through a queue that is
    // too small to hold the burst, and hold its transmitter down so that the
    // queue cannot drain while the burst is being switched.
    //
    Ptr<DropTailQueue<Packet>> txQueue = CreateObject<DropTailQueue<Packet>>();
    txQueue->SetMaxSize(QueueSize(QueueSizeUnit::PACKETS, queueLimit));
    m_ports[1]->GetMac()->SetTxQueue(txQueue);

    m_ports[1]->GetMac()->TraceConnectWithoutContext(
        "MacTxDrop",
        MakeCallback(&EthernetSwitchQueueLimitTestCase::CountPortTxDrop, this));

    // The address of host 1 has to be known, so that the burst is forwarded to
    // its port alone rather than flooded.
    m_switch->LearnMacAddress(Mac48Address::ConvertFrom(m_hosts[1]->GetAddress()), m_ports[1]);

    Simulator::Schedule(MilliSeconds(1),
                        &EthernetMac::PauseTransmission,
                        m_ports[1]->GetMac(),
                        MilliSeconds(100));

    for (uint32_t i = 0; i < nFrames; ++i)
    {
        Simulator::Schedule(MilliSeconds(2),
                            &EthernetNetDevice::Send,
                            m_hosts[0],
                            Create<Packet>(payloadSize),
                            m_hosts[1]->GetAddress(),
                            0x0800);
    }

    Simulator::Stop(MilliSeconds(50));
    Simulator::Run();

    NS_TEST_ASSERT_MSG_LT_OR_EQ(txQueue->GetNPackets(),
                                queueLimit,
                                "The switch should never overfill the queue of a port");

    NS_TEST_ASSERT_MSG_EQ(m_portTxDrops,
                          0u,
                          "The port should not have to drop frames the switch handed it");

    NS_TEST_ASSERT_MSG_GT(m_forwardDrops,
                          0u,
                          "The switch should give up on the frames the port has no room for");

    NS_TEST_ASSERT_MSG_EQ(m_forwardDrops + txQueue->GetNPackets(),
                          nFrames,
                          "Every frame should be either queued on the port or dropped");

    Simulator::Destroy();
}

/**
 * @ingroup ethernet-test
 * @ingroup tests
 *
 * @brief Test the additional memory limit that the switch can be given.
 *
 * The limits of the port queues are not the only ones a switch may have to
 * respect. A callback refusing every frame is installed, and the switch is
 * expected to forward nothing even though the port queues are empty.
 */
class EthernetSwitchMemoryCheckTestCase : public EthernetSwitchTestCase
{
  public:
    EthernetSwitchMemoryCheckTestCase();

  private:
    void DoRun() override;

    /**
     * @brief Refuse every frame, and record the size it was offered with.
     * @param size The size, in bytes, of the frame that would be queued.
     * @return Always false.
     */
    bool RefuseAll(Ptr<EthernetNetDevice>, uint32_t size);

    std::vector<uint32_t> m_offeredSizes; //!< Frame sizes the callback was offered
};

EthernetSwitchMemoryCheckTestCase::EthernetSwitchMemoryCheckTestCase()
    : EthernetSwitchTestCase("An additional memory limit keeps frames from being forwarded")
{
}

bool
EthernetSwitchMemoryCheckTestCase::RefuseAll(Ptr<EthernetNetDevice>, uint32_t size)
{
    m_offeredSizes.push_back(size);
    return false;
}

void
EthernetSwitchMemoryCheckTestCase::DoRun()
{
    const uint32_t nHosts = 3;
    const uint32_t payloadSize = 100;

    BuildTopology(nHosts);

    m_switch->SetMemoryCheck(MakeCallback(&EthernetSwitchMemoryCheckTestCase::RefuseAll, this));

    Simulator::Schedule(MilliSeconds(1),
                        &EthernetNetDevice::Send,
                        m_hosts[0],
                        Create<Packet>(payloadSize),
                        m_hosts[1]->GetAddress(),
                        0x0800);

    Simulator::Stop(MilliSeconds(2));
    Simulator::Run();

    NS_TEST_ASSERT_MSG_EQ(m_rxCount[1], 0u, "No frame should be forwarded past the memory limit");
    NS_TEST_ASSERT_MSG_EQ(m_rxCount[2], 0u, "No frame should be forwarded past the memory limit");

    // The frame was flooded, so both of the other ports were offered a copy.
    NS_TEST_ASSERT_MSG_EQ(m_forwardDrops, 2u, "Both copies of the frame should be dropped");

    NS_TEST_ASSERT_MSG_EQ(m_offeredSizes.size(), 2u, "The memory limit should be checked per port");

    // The limit is offered the size of the frame on the wire, not of the payload.
    NS_TEST_ASSERT_MSG_EQ(m_offeredSizes[0],
                          EthernetMac::GetFrameSize(payloadSize),
                          "The memory limit should account for the header and the FCS");

    Simulator::Destroy();
}

/**
 * @ingroup ethernet-test
 * @ingroup tests
 *
 * @brief Test that switch memory backpressure keeps a frame in the ingress
 * RX queue when the switch cannot accept it.
 *
 * The switch memory policy refuses the incoming frame. The frame must remain
 * in the RX queue of the ingress port so that it can be processed later when
 * switch memory becomes available.
 */
class EthernetSwitchRxBackpressureTestCase : public EthernetSwitchTestCase
{
  public:
    EthernetSwitchRxBackpressureTestCase();

  private:
    void DoRun() override;

    /**
     * @brief Refuse every frame offered to the switch.
     * @param size The size, in bytes, of the frame.
     * @return Always false.
     */
    bool RefuseAll(uint32_t size);

    uint32_t m_memoryChecks{0}; //!< Number of memory checks performed.
};

EthernetSwitchRxBackpressureTestCase::EthernetSwitchRxBackpressureTestCase()
    : EthernetSwitchTestCase("Ingress RX queue applies switch memory backpressure")
{
}

bool
EthernetSwitchRxBackpressureTestCase::RefuseAll(uint32_t size)
{
    ++m_memoryChecks;
    return false;
}

void
EthernetSwitchRxBackpressureTestCase::DoRun()
{
    const uint32_t nHosts = 2;
    const uint32_t payloadSize = 100;

    BuildTopology(nHosts);

    Ptr<Queue<Packet>> rxQueue = m_ports[0]->GetMac()->GetRxQueue();

    NS_TEST_ASSERT_MSG_EQ(rxQueue->GetNPackets(), 0u, "Ingress RX queue should initially be empty");

    //
    // Refuse the frame before it can be removed from the ingress RX queue.
    //
    m_switch->SetSwitchMemoryCheck(
        MakeCallback(&EthernetSwitchRxBackpressureTestCase::RefuseAll, this));

    Simulator::Schedule(MilliSeconds(1),
                        &EthernetNetDevice::Send,
                        m_hosts[0],
                        Create<Packet>(payloadSize),
                        m_hosts[1]->GetAddress(),
                        0x0800);

    Simulator::Stop(MilliSeconds(2));
    Simulator::Run();

    //
    // The switch memory policy rejected the frame, so ReceiveFromPort()
    // must leave it in the ingress RX queue.
    //
    NS_TEST_ASSERT_MSG_EQ(m_memoryChecks, 1u, "The switch memory policy should be checked");

    NS_TEST_ASSERT_MSG_EQ(rxQueue->GetNPackets(),
                          1u,
                          "Rejected frame should remain in the ingress RX queue");

    NS_TEST_ASSERT_MSG_EQ(m_rxCount[1], 0u, "Frame should not reach the destination host");

    NS_TEST_ASSERT_MSG_EQ(m_forwardDrops, 0u, "Frame should not be dropped by the forwarding path");

    Simulator::Destroy();
}

/**
 * @ingroup ethernet-test
 * @brief Ethernet switch test suite.
 */
class EthernetSwitchTestSuite : public TestSuite
{
  public:
    EthernetSwitchTestSuite();
};

EthernetSwitchTestSuite::EthernetSwitchTestSuite()
    : TestSuite("ethernet-switch-test", Type::UNIT)
{
    AddTestCase(new EthernetSwitchLearningTestCase, TestCase::Duration::QUICK);
    AddTestCase(new EthernetSwitchQueueLimitTestCase, TestCase::Duration::QUICK);
    AddTestCase(new EthernetSwitchMemoryCheckTestCase, TestCase::Duration::QUICK);
    AddTestCase(new EthernetSwitchRxBackpressureTestCase, TestCase::Duration::QUICK);
}

static EthernetSwitchTestSuite g_ethernetSwitchTestSuite; //!< The testsuite
