/*
 * Copyright (c) 2010 Hemanth Narra
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Hemanth Narra <hemanth@ittc.ku.com>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  https://resilinets.org/
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture),
 * NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI),
 * US Department of Defense (DoD), and ITTC at The University of Kansas.
 */
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/dsdv-helper.h"
#include "ns3/dsdv-packet.h"
#include "ns3/dsdv-rtable.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/mesh-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/pcap-file.h"
#include "ns3/simple-net-device-helper.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/uinteger.h"

using namespace ns3;

/**
 * @ingroup dsdv
 * @ingroup tests
 * @defgroup dsdv-test DSDV module tests
 */

/**
 * @ingroup dsdv-test
 *
 * @brief DSDV test case to verify the DSDV header
 *
 */
class DsdvHeaderTestCase : public TestCase
{
  public:
    DsdvHeaderTestCase();
    ~DsdvHeaderTestCase() override;
    void DoRun() override;
};

DsdvHeaderTestCase::DsdvHeaderTestCase()
    : TestCase("Verifying the DSDV header")
{
}

DsdvHeaderTestCase::~DsdvHeaderTestCase()
{
}

void
DsdvHeaderTestCase::DoRun()
{
    Ptr<Packet> packet = Create<Packet>();

    {
        dsdv::DsdvHeader hdr1;
        hdr1.SetDst(Ipv4Address("10.1.1.2"));
        hdr1.SetDstSeqno(2);
        hdr1.SetHopCount(2);
        packet->AddHeader(hdr1);
        dsdv::DsdvHeader hdr2;
        hdr2.SetDst(Ipv4Address("10.1.1.3"));
        hdr2.SetDstSeqno(4);
        hdr2.SetHopCount(1);
        packet->AddHeader(hdr2);
        NS_TEST_ASSERT_MSG_EQ(packet->GetSize(), 24, "001");
    }

    {
        dsdv::DsdvHeader hdr2;
        packet->RemoveHeader(hdr2);
        NS_TEST_ASSERT_MSG_EQ(hdr2.GetSerializedSize(), 12, "002");
        NS_TEST_ASSERT_MSG_EQ(hdr2.GetDst(), Ipv4Address("10.1.1.3"), "003");
        NS_TEST_ASSERT_MSG_EQ(hdr2.GetDstSeqno(), 4, "004");
        NS_TEST_ASSERT_MSG_EQ(hdr2.GetHopCount(), 1, "005");
        dsdv::DsdvHeader hdr1;
        packet->RemoveHeader(hdr1);
        NS_TEST_ASSERT_MSG_EQ(hdr1.GetSerializedSize(), 12, "006");
        NS_TEST_ASSERT_MSG_EQ(hdr1.GetDst(), Ipv4Address("10.1.1.2"), "008");
        NS_TEST_ASSERT_MSG_EQ(hdr1.GetDstSeqno(), 2, "009");
        NS_TEST_ASSERT_MSG_EQ(hdr1.GetHopCount(), 2, "010");
    }
}

/**
 * @ingroup dsdv-test
 *
 * @brief DSDV routing table tests (adding and looking up routes)
 */
class DsdvTableTestCase : public TestCase
{
  public:
    DsdvTableTestCase();
    ~DsdvTableTestCase() override;
    void DoRun() override;
};

DsdvTableTestCase::DsdvTableTestCase()
    : TestCase("Dsdv Routing Table test case")
{
}

DsdvTableTestCase::~DsdvTableTestCase()
{
}

void
DsdvTableTestCase::DoRun()
{
    dsdv::RoutingTable rtable;
    Ptr<NetDevice> dev;
    {
        dsdv::RoutingTableEntry rEntry1(
            /*dev=*/dev,
            /*dst=*/Ipv4Address("10.1.1.4"),
            /*seqNo=*/2,
            /*iface=*/Ipv4InterfaceAddress(Ipv4Address("10.1.1.1"), Ipv4Mask("255.255.255.0")),
            /*hops=*/2,
            /*nextHop=*/Ipv4Address("10.1.1.2"),
            /*lifetime=*/Seconds(10));
        NS_TEST_EXPECT_MSG_EQ(rtable.AddRoute(rEntry1), true, "add route");

        dsdv::RoutingTableEntry rEntry2(
            /*dev=*/dev,
            /*dst=*/Ipv4Address("10.1.1.2"),
            /*seqNo=*/4,
            /*iface=*/Ipv4InterfaceAddress(Ipv4Address("10.1.1.1"), Ipv4Mask("255.255.255.0")),
            /*hops=*/1,
            /*nextHop=*/Ipv4Address("10.1.1.2"),
            /*lifetime=*/Seconds(10));
        NS_TEST_EXPECT_MSG_EQ(rtable.AddRoute(rEntry2), true, "add route");

        dsdv::RoutingTableEntry rEntry3(
            /*dev=*/dev,
            /*dst=*/Ipv4Address("10.1.1.3"),
            /*seqNo=*/4,
            /*iface=*/Ipv4InterfaceAddress(Ipv4Address("10.1.1.1"), Ipv4Mask("255.255.255.0")),
            /*hops=*/1,
            /*nextHop=*/Ipv4Address("10.1.1.3"),
            /*lifetime=*/Seconds(10));
        NS_TEST_EXPECT_MSG_EQ(rtable.AddRoute(rEntry3), true, "add route");

        dsdv::RoutingTableEntry rEntry4(
            /*dev=*/dev,
            /*dst=*/Ipv4Address("10.1.1.255"),
            /*seqNo=*/0,
            /*iface=*/Ipv4InterfaceAddress(Ipv4Address("10.1.1.1"), Ipv4Mask("255.255.255.0")),
            /*hops=*/0,
            /*nextHop=*/Ipv4Address("10.1.1.255"),
            /*lifetime=*/Seconds(10));
        NS_TEST_EXPECT_MSG_EQ(rtable.AddRoute(rEntry4), true, "add route");
    }
    {
        dsdv::RoutingTableEntry rEntry;
        if (rtable.LookupRoute(Ipv4Address("10.1.1.4"), rEntry))
        {
            NS_TEST_ASSERT_MSG_EQ(rEntry.GetDestination(), Ipv4Address("10.1.1.4"), "100");
            NS_TEST_ASSERT_MSG_EQ(rEntry.GetSeqNo(), 2, "101");
            NS_TEST_ASSERT_MSG_EQ(rEntry.GetHop(), 2, "102");
        }
        if (rtable.LookupRoute(Ipv4Address("10.1.1.2"), rEntry))
        {
            NS_TEST_ASSERT_MSG_EQ(rEntry.GetDestination(), Ipv4Address("10.1.1.2"), "103");
            NS_TEST_ASSERT_MSG_EQ(rEntry.GetSeqNo(), 4, "104");
            NS_TEST_ASSERT_MSG_EQ(rEntry.GetHop(), 1, "105");
        }
        if (rtable.LookupRoute(Ipv4Address("10.1.1.3"), rEntry))
        {
            NS_TEST_ASSERT_MSG_EQ(rEntry.GetDestination(), Ipv4Address("10.1.1.3"), "106");
            NS_TEST_ASSERT_MSG_EQ(rEntry.GetSeqNo(), 4, "107");
            NS_TEST_ASSERT_MSG_EQ(rEntry.GetHop(), 1, "108");
        }
        if (rtable.LookupRoute(Ipv4Address("10.1.1.255"), rEntry))
        {
            NS_TEST_ASSERT_MSG_EQ(rEntry.GetDestination(), Ipv4Address("10.1.1.255"), "109");
        }
        NS_TEST_ASSERT_MSG_EQ(rEntry.GetInterface().GetLocal(), Ipv4Address("10.1.1.1"), "110");
        NS_TEST_ASSERT_MSG_EQ(rEntry.GetInterface().GetBroadcast(),
                              Ipv4Address("10.1.1.255"),
                              "111");
        NS_TEST_ASSERT_MSG_EQ(rtable.RoutingTableSize(), 4, "Rtable size incorrect");
    }
    Simulator::Destroy();
}

/**
 * @ingroup dsdv-test
 *
 * @brief DSDV chain regression test: a multi-interface relay must deliver.
 *
 * Three nodes in a chain of two point-to-point links, so the middle node has
 * two non-loopback interfaces. Verifies that the simulation completes (the
 * unchecked LookupRoute () in LookForQueuedPackets () used to send on a route
 * with a null output device and die in Ipv4L3Protocol::SendRealOut) and that
 * data is delivered end to end (the node's own address used to be advertised
 * from a hardcoded interface index, so next hops toward a multi-interface
 * node never resolved). A two-node run guards single-interface behaviour.
 */
class DsdvChainTestCase : public TestCase
{
  public:
    DsdvChainTestCase()
        : TestCase("DSDV multi-interface chain delivers end to end")
    {
    }

    /**
     * Receive a packet on the sink socket
     * @param socket the receiving socket
     */
    void Receive(Ptr<Socket> socket)
    {
        Ptr<Packet> p;
        while ((p = socket->Recv()))
        {
            m_rxBytes += p->GetSize();
        }
    }

    /**
     * Send a packet toward the sink
     * @param socket the sending socket
     */
    void Send(Ptr<Socket> socket)
    {
        socket->Send(Create<Packet>(64));
    }

    /**
     * Run one chain of the given length and return the bytes delivered from
     * the last node to the first
     * @param nNodes chain length
     * @return bytes received by the sink
     */
    uint32_t RunChain(uint32_t nNodes)
    {
        m_rxBytes = 0;
        NodeContainer nodes;
        nodes.Create(nNodes);

        SimpleNetDeviceHelper devHelper;
        devHelper.SetNetDevicePointToPointMode(true);
        std::vector<NetDeviceContainer> links;
        for (uint32_t i = 0; i + 1 < nNodes; ++i)
        {
            links.push_back(devHelper.Install(NodeContainer(nodes.Get(i), nodes.Get(i + 1))));
        }

        DsdvHelper dsdv;
        InternetStackHelper internet;
        internet.SetRoutingHelper(dsdv);
        internet.Install(nodes);

        Ipv4AddressHelper ipv4;
        Ipv4Address dst;
        for (uint32_t i = 0; i + 1 < nNodes; ++i)
        {
            std::ostringstream base;
            base << "10.1." << (i + 1) << ".0";
            ipv4.SetBase(base.str().c_str(), "255.255.255.252");
            Ipv4InterfaceContainer ic = ipv4.Assign(links[i]);
            if (i == 0)
            {
                dst = ic.GetAddress(0); // first node
            }
        }

        Ptr<Socket> sink = Socket::CreateSocket(nodes.Get(0), UdpSocketFactory::GetTypeId());
        sink->Bind(InetSocketAddress(Ipv4Address::GetAny(), 9));
        sink->SetRecvCallback(MakeCallback(&DsdvChainTestCase::Receive, this));

        Ptr<Socket> source =
            Socket::CreateSocket(nodes.Get(nNodes - 1), UdpSocketFactory::GetTypeId());
        source->Connect(InetSocketAddress(dst, 9));
        for (uint32_t s = 0; s < 10; ++s)
        {
            Simulator::Schedule(Seconds(5.0 + s), &DsdvChainTestCase::Send, this, source);
        }

        Simulator::Stop(Seconds(20));
        Simulator::Run();
        Simulator::Destroy();
        return m_rxBytes;
    }

    void DoRun() override
    {
        uint32_t multi = RunChain(3);
        NS_TEST_EXPECT_MSG_GT(multi, 0, "no data delivered across the multi-interface relay");
        uint32_t single = RunChain(2);
        NS_TEST_EXPECT_MSG_GT(single, 0, "no data delivered on the single-interface link");
    }

  private:
    uint32_t m_rxBytes{0}; ///< bytes received by the sink
};

/**
 * @ingroup dsdv-test
 *
 * @brief DSDV test suite
 */
class DsdvTestSuite : public TestSuite
{
  public:
    DsdvTestSuite()
        : TestSuite("routing-dsdv", Type::UNIT)
    {
        AddTestCase(new DsdvHeaderTestCase(), TestCase::Duration::QUICK);
        AddTestCase(new DsdvTableTestCase(), TestCase::Duration::QUICK);
        AddTestCase(new DsdvChainTestCase(), TestCase::Duration::QUICK);
    }
} g_dsdvTestSuite; ///< the test suite
