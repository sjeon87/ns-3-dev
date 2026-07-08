/*
 * Copyright (c) 2024 University of Florence
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Francesco Todino <todinofrancesco97@gmail.com>
 *          Tommaso Pecorella <tommaso.pecorella@unifi.it>
 *          Sweekar Nepal (Sweekar-cmd) <sweekar728@gmail.com>
 */

#include "ns3/aodvv2-packet.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/simulator.h"
#include "ns3/test.h"

#include <limits>
#include <string>

using namespace ns3;
using namespace ns3::aodvv2;

/**
 * @ingroup aodvv2-test
 * @brief Test AODVv2 packet serialization and deserialization (Test 0)
 *
 * Verifies that each message type (RREQ, RREP, RERR, RREP_ACK)
 * can be serialized to a buffer and deserialized back to the same values.
 */
class Aodvv2PacketSerializationTest : public TestCase
{
  public:
    Aodvv2PacketSerializationTest()
        : TestCase("AODVv2 packet serialization/deserialization (Test 0)")
    {
    }

    void DoRun() override
    {
        // --- Test RREQ IPv4 ---
        {
            Ipv4Address origIp("10.0.0.1");
            Ipv4Address targIp("10.0.0.2");
            uint16_t seqNo = 42;
            uint8_t hopLimit = 10;

            RreqHeader<Ipv4Address> rreqOut(origIp, 32, targIp, 32, seqNo, hopLimit);
            rreqOut.CreateTlvHeader();

            Ptr<Packet> p = Create<Packet>();
            p->AddHeader(rreqOut);

            RreqHeader<Ipv4Address> rreqIn;
            p->RemoveHeader(rreqIn);

            NS_TEST_ASSERT_MSG_EQ(rreqIn.GetOrigIp(),
                                  origIp,
                                  "RREQ IPv4: OrigIp mismatch after deserialization");
            NS_TEST_ASSERT_MSG_EQ(rreqIn.GetTargIp(),
                                  targIp,
                                  "RREQ IPv4: TargIp mismatch after deserialization");
            NS_TEST_ASSERT_MSG_EQ(rreqIn.GetSeqNo(),
                                  seqNo,
                                  "RREQ IPv4: SeqNo mismatch after deserialization");
            NS_TEST_ASSERT_MSG_EQ(rreqIn.GetHopLimit(),
                                  hopLimit,
                                  "RREQ IPv4: HopLimit mismatch after deserialization");
        }

        // --- Test RREP IPv4 ---
        {
            Ipv4Address origIp("10.0.0.1");
            Ipv4Address targIp("10.0.0.2");
            uint16_t seqNo = 7;
            uint8_t hopLimit = 5;

            RrepHeader<Ipv4Address> rrepOut(origIp, 32, targIp, 32, seqNo, hopLimit);
            rrepOut.CreateTlvHeader();

            Ptr<Packet> p = Create<Packet>();
            p->AddHeader(rrepOut);

            RrepHeader<Ipv4Address> rrepIn;
            p->RemoveHeader(rrepIn);

            NS_TEST_ASSERT_MSG_EQ(rrepIn.GetOrigIp(),
                                  origIp,
                                  "RREP IPv4: OrigIp mismatch after deserialization");
            NS_TEST_ASSERT_MSG_EQ(rrepIn.GetTargIp(),
                                  targIp,
                                  "RREP IPv4: TargIp mismatch after deserialization");
            NS_TEST_ASSERT_MSG_EQ(rrepIn.GetSeqNo(),
                                  seqNo,
                                  "RREP IPv4: SeqNo mismatch after deserialization");
        }

        // --- Test RREP_ACK IPv4 ---
        {
            RrepAckHeader<Ipv4Address> ackOut;
            ackOut.SetSeqNo(99);
            ackOut.CreateTlvHeader();

            Ptr<Packet> p = Create<Packet>();
            p->AddHeader(ackOut);

            RrepAckHeader<Ipv4Address> ackIn;
            p->RemoveHeader(ackIn);

            NS_TEST_ASSERT_MSG_EQ(ackIn.GetSeqNo(),
                                  99,
                                  "RREP_ACK IPv4: SeqNo mismatch after deserialization");
        }

        // --- Test RERR IPv4 ---
        {
            RerrHeader<Ipv4Address> rerrOut;
            rerrOut.SetOrigIp(Ipv4Address("10.0.0.1"));
            rerrOut.AddUnDestination(Ipv4Address("10.0.0.3"), 5);
            rerrOut.CreateTlvHeader();

            Ptr<Packet> p = Create<Packet>();
            p->AddHeader(rerrOut);

            RerrHeader<Ipv4Address> rerrIn;
            p->RemoveHeader(rerrIn);

            NS_TEST_ASSERT_MSG_EQ(rerrIn.GetOrigIp(),
                                  Ipv4Address("10.0.0.1"),
                                  "RERR IPv4: OrigIp mismatch after deserialization");
            NS_TEST_ASSERT_MSG_EQ(rerrIn.GetDestCount(),
                                  1,
                                  "RERR IPv4: DestCount mismatch after deserialization");
        }

        Simulator::Destroy();
    }
};

/**
 * @ingroup aodvv2-test
 * @brief AODVv2 TestSuite
 */
class Aodvv2TestSuite : public TestSuite
{
  public:
    Aodvv2TestSuite()
        : TestSuite("aodvv2", Type::UNIT)
    {
        AddTestCase(new Aodvv2PacketSerializationTest, TestCase::Duration::QUICK);
    }
};

static Aodvv2TestSuite g_aodvv2TestSuite;
