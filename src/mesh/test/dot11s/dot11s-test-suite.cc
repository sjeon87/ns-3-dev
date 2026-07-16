/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Pavel Boyko <boyko@iitp.ru>
 */
#include "ns3/buffer.h"
#include "ns3/dot11s-mac-header.h"
#include "ns3/header-serialization-test.h"
#include "ns3/hwmp-rtable.h"
#include "ns3/ie-dot11s-configuration.h"
#include "ns3/ie-dot11s-mesh-peering-management.h"
#include "ns3/mesh-peering-close-header.h"
#include "ns3/mesh-peering-confirm-header.h"
#include "ns3/mesh-peering-open-header.h"
#include "ns3/mgt-headers.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/test.h"

#include <optional>

using namespace ns3;
using namespace dot11s;

/**
 * @ingroup mesh-test
 * @defgroup dot11s-test dot11s sub-module tests
 */

/**
 * @ingroup dot11s-test
 *
 * @brief Built-in self test for MeshHeader
 */
struct MeshHeaderTest : public TestCase
{
    MeshHeaderTest()
        : TestCase("Dot11sMeshHeader roundtrip serialization")
    {
    }

    void DoRun() override;
};

void
MeshHeaderTest::DoRun()
{
    {
        MeshHeader a;
        a.SetAddressExt(3);
        a.SetAddr4(Mac48Address("11:22:33:44:55:66"));
        a.SetAddr5(Mac48Address("11:00:33:00:55:00"));
        a.SetAddr6(Mac48Address("00:22:00:44:00:66"));
        a.SetMeshTtl(122);
        a.SetMeshSeqno(321);
        Ptr<Packet> packet = Create<Packet>();
        packet->AddHeader(a);
        MeshHeader b;
        packet->RemoveHeader(b);
        NS_TEST_ASSERT_MSG_EQ(a, b, "Mesh header roundtrip serialization works, 3 addresses");
    }
    {
        MeshHeader a;
        a.SetAddressExt(2);
        a.SetAddr5(Mac48Address("11:00:33:00:55:00"));
        a.SetAddr6(Mac48Address("00:22:00:44:00:66"));
        a.SetMeshTtl(122);
        a.SetMeshSeqno(321);
        Ptr<Packet> packet = Create<Packet>();
        packet->AddHeader(a);
        MeshHeader b;
        packet->RemoveHeader(b);
        NS_TEST_ASSERT_MSG_EQ(a, b, "Mesh header roundtrip serialization works, 2 addresses");
    }
    {
        MeshHeader a;
        a.SetAddressExt(1);
        a.SetAddr4(Mac48Address("11:22:33:44:55:66"));
        a.SetMeshTtl(122);
        a.SetMeshSeqno(321);
        Ptr<Packet> packet = Create<Packet>();
        packet->AddHeader(a);
        MeshHeader b;
        packet->RemoveHeader(b);
        NS_TEST_ASSERT_MSG_EQ(a, b, "Mesh header roundtrip serialization works, 1 address");
    }
}

/**
 * @ingroup mesh-test
 *
 * @brief Unit test for HwmpRtable
 */
class HwmpRtableTest : public TestCase
{
  public:
    HwmpRtableTest();
    void DoRun() override;

  private:
    /// Test Add apth and lookup path;
    void TestLookup();

    /// Test add path and try to lookup after entry has expired
    void TestAddPath();
    /// Test add path and try to lookup after entry has expired
    void TestExpire();

    /// Test add precursors and find precursor list in rtable
    void TestPrecursorAdd();
    /// Test add precursors and find precursor list in rtable
    void TestPrecursorFind();

  private:
    Mac48Address dst;                     ///< destination address
    Mac48Address hop;                     ///< hop address
    uint32_t iface;                       ///< interface
    uint32_t metric;                      ///< metric
    uint32_t seqnum;                      ///< sequence number
    Time expire;                          ///< expiration time
    Ptr<HwmpRtable> table;                ///< tab;e
    std::vector<Mac48Address> precursors; ///< precursors
};

HwmpRtableTest::HwmpRtableTest()
    : TestCase("HWMP routing table"),
      dst("01:00:00:01:00:01"),
      hop("01:00:00:01:00:03"),
      iface(8010),
      metric(10),
      seqnum(1),
      expire(Seconds(10))
{
    precursors.emplace_back("00:10:20:30:40:50");
    precursors.emplace_back("00:11:22:33:44:55");
    precursors.emplace_back("00:01:02:03:04:05");
}

void
HwmpRtableTest::TestLookup()
{
    HwmpRtable::LookupResult correct(hop, iface, metric, seqnum);

    // Reactive path
    table->AddReactivePath(dst, hop, iface, metric, expire, seqnum);
    NS_TEST_EXPECT_MSG_EQ((table->LookupReactive(dst) == correct), true, "Reactive lookup works");
    table->DeleteReactivePath(dst);
    NS_TEST_EXPECT_MSG_EQ(table->LookupReactive(dst).IsValid(), false, "Reactive lookup works");

    // Proactive
    table->AddProactivePath(metric, dst, hop, iface, expire, seqnum);
    NS_TEST_EXPECT_MSG_EQ((table->LookupProactive() == correct), true, "Proactive lookup works");
    table->DeleteProactivePath(dst);
    NS_TEST_EXPECT_MSG_EQ(table->LookupProactive().IsValid(), false, "Proactive lookup works");
}

void
HwmpRtableTest::TestAddPath()
{
    table->AddReactivePath(dst, hop, iface, metric, expire, seqnum);
    table->AddProactivePath(metric, dst, hop, iface, expire, seqnum);
}

void
HwmpRtableTest::TestExpire()
{
    // this is assumed to be called when path records are already expired
    HwmpRtable::LookupResult correct(hop, iface, metric, seqnum);
    NS_TEST_EXPECT_MSG_EQ((table->LookupReactiveExpired(dst) == correct),
                          true,
                          "Reactive expiration works");
    NS_TEST_EXPECT_MSG_EQ((table->LookupProactiveExpired() == correct),
                          true,
                          "Proactive expiration works");

    NS_TEST_EXPECT_MSG_EQ(table->LookupReactive(dst).IsValid(), false, "Reactive expiration works");
    NS_TEST_EXPECT_MSG_EQ(table->LookupProactive().IsValid(), false, "Proactive expiration works");
}

void
HwmpRtableTest::TestPrecursorAdd()
{
    for (auto i = precursors.begin(); i != precursors.end(); i++)
    {
        table->AddPrecursor(dst, iface, *i, Seconds(100));
        // Check that duplicates are filtered
        table->AddPrecursor(dst, iface, *i, Seconds(100));
    }
}

void
HwmpRtableTest::TestPrecursorFind()
{
    HwmpRtable::PrecursorList precursorList = table->GetPrecursors(dst);
    NS_TEST_EXPECT_MSG_EQ(precursors.size(), precursorList.size(), "Precursors size works");
    for (unsigned i = 0; i < precursors.size(); i++)
    {
        NS_TEST_EXPECT_MSG_EQ(precursorList[i].first, iface, "Precursors lookup works");
        NS_TEST_EXPECT_MSG_EQ(precursorList[i].second, precursors[i], "Precursors lookup works");
    }
}

void
HwmpRtableTest::DoRun()
{
    table = CreateObject<HwmpRtable>();

    Simulator::Schedule(Seconds(0), &HwmpRtableTest::TestLookup, this);
    Simulator::Schedule(Seconds(1), &HwmpRtableTest::TestAddPath, this);
    Simulator::Schedule(Seconds(2), &HwmpRtableTest::TestPrecursorAdd, this);
    Simulator::Schedule(expire + Seconds(2), &HwmpRtableTest::TestExpire, this);
    Simulator::Schedule(expire + Seconds(3), &HwmpRtableTest::TestPrecursorFind, this);

    Simulator::Run();
    Simulator::Destroy();
}

//-----------------------------------------------------------------------------
/// Test mesh peering frame header serialization
class MeshPeeringHeaderSerializationTest : public HeaderSerializationTestCase
{
  public:
    MeshPeeringHeaderSerializationTest()
        : HeaderSerializationTestCase("PeerLinkFrames header serialization tests")
    {
    }

  private:
    void DoRun() override;
};

void
MeshPeeringHeaderSerializationTest::DoRun()
{
    {
        MeshPeeringOpenHeader hdr;

        // set capabilities
        auto capabilities = CapabilityInformation();
        capabilities.SetShortPreamble(true);
        hdr.m_capability = capabilities;

        // set (extended) supported rates
        AllSupportedRates rates;
        rates.AddSupportedRate(6e6);
        rates.AddSupportedRate(12e6);
        rates.AddSupportedRate(24e6);
        rates.AddSupportedRate(48e6);
        rates.AddSupportedRate(1e6);
        rates.AddSupportedRate(2e6);
        hdr.Get<SupportedRates>() = rates.rates;
        hdr.Get<ExtendedSupportedRatesIE>() = rates.extendedRates;

        // set mesh IEs
        hdr.Get<IeMeshId>() = IeMeshId("mesh1");
        auto meshConfig = IeConfiguration();
        meshConfig.SetRouting(PROTOCOL_HWMP);
        hdr.Get<IeConfiguration>() = meshConfig;
        IeMeshPeeringManagement meshPeering;
        meshPeering.SetLocalLinkId(123);
        hdr.Get<IeMeshPeeringManagement>() = meshPeering;

        TestHeaderSerialization(hdr);
    }
    {
        MeshPeeringConfirmHeader hdr;

        // set capabilities
        auto capabilities = CapabilityInformation();
        capabilities.SetShortPreamble(true);
        hdr.m_capability = capabilities;

        // set aid
        hdr.m_aid = 321;

        // set (extended) supported rates
        AllSupportedRates rates;
        rates.AddSupportedRate(6e6);
        rates.AddSupportedRate(12e6);
        rates.AddSupportedRate(24e6);
        rates.AddSupportedRate(48e6);
        rates.AddSupportedRate(1e6);
        rates.AddSupportedRate(2e6);
        hdr.Get<SupportedRates>() = rates.rates;
        hdr.Get<ExtendedSupportedRatesIE>() = rates.extendedRates;

        // set mesh IEs
        hdr.Get<IeMeshId>() = IeMeshId("mesh1");
        auto meshConfig = IeConfiguration();
        meshConfig.SetRouting(PROTOCOL_HWMP);
        hdr.Get<IeConfiguration>() = meshConfig;
        IeMeshPeeringManagement meshPeering;
        meshPeering.SetLocalLinkId(123);
        meshPeering.SetPeerLinkId(321);
        hdr.Get<IeMeshPeeringManagement>() = meshPeering;

        TestHeaderSerialization(hdr);
    }
    {
        MeshPeeringCloseHeader hdr;

        // set mesh IEs
        hdr.Get<IeMeshId>() = IeMeshId("mesh1");
        IeMeshPeeringManagement meshPeering;
        meshPeering.SetLocalLinkId(123);
        meshPeering.SetPeerLinkId(321);
        meshPeering.SetReasonCode(REASON11S_PEERING_CANCELLED);
        hdr.Get<IeMeshPeeringManagement>() = meshPeering;

        TestHeaderSerialization(hdr);
    }
}

/**
 * @ingroup dot11s-test
 *
 * Tests serialization/deserialization of the mesh peering management element
 * for mesh peering open/confirm/close frames.
 */
class IeMeshPeeringManagementSerializationTest : public TestCase
{
  public:
    IeMeshPeeringManagementSerializationTest();

  private:
    /**
     * Compares serialized bytes against expected values.
     *
     * @param ie the mesh peering management element to serialize
     * @param expectedLength expected length
     * @param expectedLocalLinkId expected local link id
     * @param expectedPeerLinkId expected peer link id (optional)
     * @param expectedReasonCode expected reason code (optional)
     */
    void CheckSerializedBytes(const IeMeshPeeringManagement& ie,
                              uint8_t expectedLength,
                              uint16_t expectedLocalLinkId,
                              std::optional<uint16_t> expectedPeerLinkId,
                              std::optional<uint16_t> expectedReasonCode);

    /**
     * Builds the ie using a buffer and deserialize them
     * to check that the fields are interpreted correctly.
     *
     * @param length length of the element
     * @param localLinkId local link id field
     * @param peerLinkId peer link id field (optional)
     * @param reasonCode reason code field (optional)
     */
    void CheckDeserializedFields(uint8_t length,
                                 uint16_t localLinkId,
                                 std::optional<uint16_t> peerLinkId,
                                 std::optional<uint16_t> reasonCode);

    void DoRun() override;
};

IeMeshPeeringManagementSerializationTest::IeMeshPeeringManagementSerializationTest()
    : TestCase("IeMeshPeeringManagement serialization tests")
{
}

void
IeMeshPeeringManagementSerializationTest::CheckSerializedBytes(
    const IeMeshPeeringManagement& ie,
    uint8_t expectedLength,
    uint16_t expectedLocalLinkId,
    std::optional<uint16_t> expectedPeerLinkId,
    std::optional<uint16_t> expectedReasonCode)
{
    // serialize element into buffer
    Buffer buffer;
    buffer.AddAtStart(ie.GetSerializedSize());
    ie.Serialize(buffer.Begin());

    // read element id and length from buffer
    Buffer::Iterator i = buffer.Begin();
    const uint16_t elementId = i.ReadU8();
    const uint8_t length = i.ReadU8();

    // compare id and length against expected values
    NS_TEST_EXPECT_MSG_EQ(elementId, IE_MESH_PEERING_MANAGEMENT, "unexpected element id");
    NS_TEST_ASSERT_MSG_EQ(length,
                          expectedLength,
                          "element length must match the peering frame type");

    // read mandatory fields from buffer & compare against expected values
    const uint16_t protocolId = i.ReadU16();
    const uint16_t localLinkId = i.ReadU16();
    NS_TEST_EXPECT_MSG_EQ(protocolId, MESH_PEERING_MANAGEMENT_PROTOCOL, "unexpected protocol id");
    NS_TEST_EXPECT_MSG_EQ(localLinkId, expectedLocalLinkId, "unexpected local link id");

    // read optional fields and compare against expected values
    if (expectedPeerLinkId)
    {
        const uint16_t peerLinkId = i.ReadU16();
        NS_TEST_EXPECT_MSG_EQ(peerLinkId, expectedPeerLinkId.value(), "unexpected peer link id");
    }
    if (expectedReasonCode)
    {
        const uint16_t reasonCode = i.ReadU16();
        NS_TEST_EXPECT_MSG_EQ(reasonCode, expectedReasonCode.value(), "unexpected reason code");
    }

    // afterwards there should be no remaining fields
    NS_TEST_EXPECT_MSG_EQ(i.GetDistanceFrom(buffer.Begin()),
                          buffer.GetSize(),
                          "serialized element contains unexpected bytes");
}

void
IeMeshPeeringManagementSerializationTest::CheckDeserializedFields(
    uint8_t length,
    uint16_t localLinkId,
    std::optional<uint16_t> peerLinkId,
    std::optional<uint16_t> reasonCode)
{
    // build the element manually in a buffer
    Buffer buffer;
    buffer.AddAtStart(2 + length);
    Buffer::Iterator i = buffer.Begin();
    i.WriteU8(IE_MESH_PEERING_MANAGEMENT);
    i.WriteU8(length);
    i.WriteU16(MESH_PEERING_MANAGEMENT_PROTOCOL);
    i.WriteU16(localLinkId);
    if (peerLinkId)
    {
        i.WriteU16(peerLinkId.value());
    }
    if (reasonCode)
    {
        i.WriteU16(reasonCode.value());
    }

    // deserialize the element
    IeMeshPeeringManagement ie;
    Buffer::Iterator end = ie.Deserialize(buffer.Begin());

    // afterwards there should be nothing left in the buffer
    NS_TEST_EXPECT_MSG_EQ(end.GetDistanceFrom(buffer.Begin()),
                          buffer.GetSize(),
                          "buffer contains remaining bytes");

    // compare mandatory fields
    NS_TEST_EXPECT_MSG_EQ(ie.GetProtocolId(),
                          MESH_PEERING_MANAGEMENT_PROTOCOL,
                          "unexpected protocol id");
    NS_TEST_EXPECT_MSG_EQ(ie.GetLocalLinkId(), localLinkId, "unexpected local link id");

    // compare optional peer link id
    NS_TEST_EXPECT_MSG_EQ(ie.GetPeerLinkId().has_value(),
                          peerLinkId.has_value(),
                          "unexpected peer link id presence");
    if (peerLinkId && ie.GetPeerLinkId())
    {
        NS_TEST_EXPECT_MSG_EQ(ie.GetPeerLinkId().value(),
                              peerLinkId.value(),
                              "unexpected peer link id");
    }

    // compare optional reason code
    NS_TEST_EXPECT_MSG_EQ(ie.GetReasonCode().has_value(),
                          reasonCode.has_value(),
                          "unexpected reason code presence");
    if (reasonCode && ie.GetReasonCode())
    {
        NS_TEST_EXPECT_MSG_EQ(ie.GetReasonCode().value(),
                              reasonCode.value(),
                              "unexpected reason code");
    }
}

void
IeMeshPeeringManagementSerializationTest::DoRun()
{
    {
        // open: protocol id + local link id
        IeMeshPeeringManagement ie;
        ie.SetLocalLinkId(123);
        CheckSerializedBytes(ie, 4, 123, std::nullopt, std::nullopt);
        CheckDeserializedFields(4, 123, std::nullopt, std::nullopt);
    }
    {
        // confirm: protocol id + local link id + peer link id
        IeMeshPeeringManagement ie;
        ie.SetLocalLinkId(123);
        ie.SetPeerLinkId(321);
        CheckSerializedBytes(ie, 6, 123, 321, std::nullopt);
        CheckDeserializedFields(6, 123, 321, std::nullopt);
    }
    {
        // close: protocol id + local link id + peer link id + reason code
        IeMeshPeeringManagement ie;
        ie.SetLocalLinkId(123);
        ie.SetPeerLinkId(321);
        ie.SetReasonCode(REASON11S_PEERING_CANCELLED);
        CheckSerializedBytes(ie, 8, 123, 321, REASON11S_PEERING_CANCELLED);
        CheckDeserializedFields(8, 123, 321, REASON11S_PEERING_CANCELLED);
    }
}

/**
 * @ingroup mesh-test
 *
 * @brief Dot11s Test Suite
 */
class Dot11sTestSuite : public TestSuite
{
  public:
    Dot11sTestSuite();
};

Dot11sTestSuite::Dot11sTestSuite()
    : TestSuite("devices-mesh-dot11s", Type::UNIT)
{
    AddTestCase(new MeshHeaderTest, TestCase::Duration::QUICK);
    AddTestCase(new HwmpRtableTest, TestCase::Duration::QUICK);
    AddTestCase(new MeshPeeringHeaderSerializationTest, TestCase::Duration::QUICK);
    AddTestCase(new IeMeshPeeringManagementSerializationTest, TestCase::Duration::QUICK);
}

static Dot11sTestSuite g_dot11sTestSuite; ///< the test suite
