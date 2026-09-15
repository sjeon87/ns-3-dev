/*
 * Copyright (c) 2008 INRIA
 *                  2013 University of New Brunswick
 *                  2014 Universitat Autònoma de Barcelona
 *                  2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *           Dizhi Zhou <dizhi.zhou@gmail.com>
 *           Gerard Garcia <ggarcia@deic.uab.cat>
 *           Rubén Martínez <rmartinez@deic.uab.cat>
 *           Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ns3/abort.h"
#include "ns3/buffer.h"
#include "ns3/bundle-agent.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-address.h"
#include "ns3/log.h"
#include "ns3/ltp-convergence-layer-adapter.h"
#include "ns3/ltp-header.h"
#include "ns3/random-variable-stream.h"
#include "ns3/sdnv.h"
#include "ns3/string.h"
#include "ns3/test.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LtpProtocolTests");

/**
 * @ingroup dtn-test
 * @ingroup tests
 *
 * @brief Test case for verifying LTP header serialization and deserialization methods.
 */
class LtpHeaderTestCase : public TestCase
{
  public:
    LtpHeaderTestCase();
    ~LtpHeaderTestCase() override;

  private:
    void DoRun() override;

    /**
     * @brief Set up SessionId test cases.
     */
    void SetSessionIds();

    /**
     * @brief Set up LtpExtension test cases.
     */
    void SetExtensions();

    /**
     * @brief Set up LtpHeader test cases.
     */
    void SetHeaderTests();

    /**
     * @brief Set up LtpTrailer test cases.
     */
    void SetTrailerTests();

    /**
     * @brief Set up LtpContentHeader test cases.
     */
    void SetContentHeaderTests();

    /**
     * @brief Template class representing a generic test vector.
     * @tparam T The type of data to test.
     */
    template <class T>
    class TestVector
    {
      public:
        uint8_t m_expectedEncodedSz; ///< The expected size in bytes after encoding.
        T m_data;                    ///< The data object to be tested.
    };

    TestVectors<TestVector<SessionId>> m_sessionIds;        ///< Test vectors for SessionId
    TestVectors<TestVector<LtpExtension>> m_extensions;     ///< Test vectors for LtpExtension
    TestVectors<TestVector<LtpHeader>> m_mainHeaderTests;   ///< Test vectors for LtpHeader
    TestVectors<TestVector<LtpTrailer>> m_mainTrailerTests; ///< Test vectors for LtpTrailer
    TestVectors<TestVector<LtpContentHeader>>
        m_mainContentHeaderTests; ///< Test vectors for LtpContentHeader
};

LtpHeaderTestCase::LtpHeaderTestCase()
    : TestCase("LtpHeaderTestCase test case (checks serialization and deserialization methods)")
{
}

LtpHeaderTestCase::~LtpHeaderTestCase()
{
}

void
LtpHeaderTestCase::SetSessionIds()
{
    TestVector<SessionId> test;

    // SessionId constructor explicitly requires uint64_t for its originator
    SessionId session(0, 0);
    test.m_expectedEncodedSz = 2;
    test.m_data = session;
    m_sessionIds.Add(test);

    session.SetSessionOriginator(0x4234);
    session.SetSessionNumber(0x1234);
    test.m_expectedEncodedSz = 5;
    test.m_data = session;
    m_sessionIds.Add(test);

    session.SetSessionOriginator(0xFFFF);
    session.SetSessionNumber(0x7F);
    test.m_expectedEncodedSz = 4;
    test.m_data = session;
    m_sessionIds.Add(test);
}

void
LtpHeaderTestCase::SetExtensions()
{
    TestVector<LtpExtension> test;
    uint8_t extensionSize = 10;

    LtpExtension extension;
    extension.SetExtensionType(LtpExtension::LTPEXT_AUTH);
    for (int i = 0; i < extensionSize; i++)
    {
        extension.AddExtensionData(0);
    }
    test.m_expectedEncodedSz = 1 + 1 + extensionSize;
    test.m_data = extension;
    m_extensions.Add(test);

    extensionSize = 0x80;
    extension.SetExtensionType(LtpExtension::LTPEXT_COOKIE);
    extension.ClearExtensionData();
    for (int i = 0; i < extensionSize; i++)
    {
        extension.AddExtensionData(0);
    }
    test.m_expectedEncodedSz = 1 + 2 + extensionSize;
    test.m_data = extension;
    m_extensions.Add(test);
}

void
LtpHeaderTestCase::SetHeaderTests()
{
    LtpHeader header;
    TestVector<LtpHeader> test;
    uint8_t version = 0;
    SegmentType type = LTPTYPE_RD;
    uint8_t extensionCntHeader = 0b00000000;
    uint8_t extensionCntTrailer = 0b00000000;

    header.SetVersion(version);
    header.SetSegmentType(type);
    header.SetSessionId((SessionId)m_sessionIds.Get(0).m_data);
    header.SetHeaderExtensionCount(extensionCntHeader);
    header.SetTrailerExtensionCount(extensionCntTrailer);

    test.m_data = header;
    test.m_expectedEncodedSz = 2 + m_sessionIds.Get(0).m_expectedEncodedSz;
    m_mainHeaderTests.Add(test);

    header.SetSessionId((SessionId)m_sessionIds.Get(1).m_data);
    header.AddExtension(m_extensions.Get(0).m_data);

    test.m_data = header;
    test.m_expectedEncodedSz =
        2 + m_sessionIds.Get(1).m_expectedEncodedSz + m_extensions.Get(0).m_expectedEncodedSz;

    m_mainHeaderTests.Add(test);

    header.SetSessionId((SessionId)m_sessionIds.Get(2).m_data);
    header.AddExtension(m_extensions.Get(1).m_data);

    test.m_data = header;
    test.m_expectedEncodedSz = 2 + m_sessionIds.Get(2).m_expectedEncodedSz +
                               m_extensions.Get(0).m_expectedEncodedSz +
                               m_extensions.Get(1).m_expectedEncodedSz;

    m_mainHeaderTests.Add(test);
}

void
LtpHeaderTestCase::SetTrailerTests()
{
    LtpTrailer trailer;

    trailer.AddExtension(m_extensions.Get(0).m_data);
    trailer.AddExtension(m_extensions.Get(0).m_data);
    trailer.AddExtension(m_extensions.Get(1).m_data);
    trailer.AddExtension(m_extensions.Get(0).m_data);

    TestVector<LtpTrailer> test;

    test.m_data = trailer;
    test.m_expectedEncodedSz =
        3 * m_extensions.Get(0).m_expectedEncodedSz + m_extensions.Get(1).m_expectedEncodedSz;

    m_mainTrailerTests.Add(test);
}

void
LtpHeaderTestCase::SetContentHeaderTests()
{
    LtpContentHeader header;
    TestVector<LtpContentHeader> test;
    SegmentType type = LTPTYPE_RD;
    uint64_t clientServiceId = 0;
    uint64_t offset = 0;
    uint64_t length = 0x7F;

    header.SetSegmentType(type);
    header.SetClientServiceId(clientServiceId);
    header.SetOffset(offset);
    header.SetLength(length);

    test.m_data = header;
    test.m_expectedEncodedSz = 1 + 1 + 1;
    m_mainContentHeaderTests.Add(test);

    type = LTPTYPE_RS;
    uint64_t cpSerialNumber = 1;
    uint64_t rpSerialNumber = 1;
    uint64_t upperBound = 0x7F;
    uint64_t lowerBound = 0x0;

    header.SetSegmentType(type);
    header.SetCpSerialNumber(cpSerialNumber);
    header.SetRpSerialNumber(rpSerialNumber);
    header.SetUpperBound(upperBound);
    header.SetLowerBound(lowerBound);

    LtpContentHeader::ReceptionClaim claim;
    claim.offset = offset;
    claim.length = length;

    header.AddReceptionClaim(claim);

    test.m_data = header;
    test.m_expectedEncodedSz = 7;

    m_mainContentHeaderTests.Add(test);

    type = LTPTYPE_RS;
    upperBound = 6000;
    lowerBound = 1000;

    header.SetSegmentType(type);
    header.SetCpSerialNumber(cpSerialNumber);
    header.SetRpSerialNumber(rpSerialNumber);
    header.SetUpperBound(upperBound);
    header.SetLowerBound(lowerBound);
    header.ClearReceptionClaims();

    claim.offset = 0;
    claim.length = 2000;
    header.AddReceptionClaim(claim);

    claim.offset = 3000;
    claim.length = 500;
    header.AddReceptionClaim(claim);

    test.m_data = header;
    test.m_expectedEncodedSz = 7 + 7;

    m_mainContentHeaderTests.Add(test);

    type = LTPTYPE_RD_CP_EORP;
    header.SetSegmentType(type);

    test.m_data = header;
    test.m_expectedEncodedSz = 5;

    m_mainContentHeaderTests.Add(test);

    type = LTPTYPE_RAS;
    header.SetSegmentType(type);

    test.m_data = header;
    test.m_expectedEncodedSz = 1;

    m_mainContentHeaderTests.Add(test);

    type = LTPTYPE_CS;
    header.SetSegmentType(type);

    test.m_data = header;
    test.m_expectedEncodedSz = 1;

    m_mainContentHeaderTests.Add(test);

    type = LTPTYPE_CAS;
    header.SetSegmentType(type);

    test.m_data = header;
    test.m_expectedEncodedSz = 0;

    m_mainContentHeaderTests.Add(test);
}

void
LtpHeaderTestCase::DoRun()
{
    SetSessionIds();
    SetExtensions();
    SetHeaderTests();
    SetTrailerTests();
    SetContentHeaderTests();

    for (uint32_t i = 0; i < m_mainHeaderTests.GetN(); i++)
    {
        TestVector<LtpHeader> test = m_mainHeaderTests.Get(i);

        uint32_t actual = test.m_data.GetSerializedSize();
        uint32_t limit = test.m_expectedEncodedSz;
        NS_TEST_ASSERT_MSG_EQ(actual, limit, "wrong header serialization size");

        Buffer buf;
        buf.AddAtStart(test.m_data.GetSerializedSize());
        test.m_data.Serialize(buf.Begin());

        LtpHeader header;
        header.Deserialize(buf.Begin());
        NS_TEST_ASSERT_MSG_EQ((header == test.m_data), true, "Header serialization methods failed");
    }

    for (uint32_t i = 0; i < m_mainTrailerTests.GetN(); i++)
    {
        TestVector<LtpTrailer> test = m_mainTrailerTests.Get(i);

        uint32_t actual = test.m_data.GetSerializedSize();
        uint32_t limit = test.m_expectedEncodedSz;
        NS_TEST_ASSERT_MSG_EQ(actual, limit, "wrong trailer serialization size");

        Buffer buf;
        buf.AddAtStart(test.m_data.GetSerializedSize());
        test.m_data.Serialize(buf.Begin());

        LtpTrailer trailer;
        trailer.Deserialize(buf.Begin());
        NS_TEST_ASSERT_MSG_EQ((trailer == test.m_data),
                              true,
                              "Trailer serialization methods failed");
    }

    for (uint32_t i = 0; i < m_mainContentHeaderTests.GetN(); i++)
    {
        TestVector<LtpContentHeader> test = m_mainContentHeaderTests.Get(i);

        uint32_t actual = test.m_data.GetSerializedSize();
        uint32_t limit = test.m_expectedEncodedSz;
        NS_TEST_ASSERT_MSG_EQ(actual, limit, "wrong trailer serialization size");

        Buffer buf;
        buf.AddAtStart(test.m_data.GetSerializedSize());
        test.m_data.Serialize(buf.Begin());

        LtpContentHeader content;
        content.SetSegmentType(test.m_data.GetSegmentType());
        content.Deserialize(buf.Begin());
        NS_TEST_ASSERT_MSG_EQ((content == test.m_data),
                              true,
                              "Content Header serialization methods failed");
    }
}

/**
 * @ingroup dtn-test
 * @ingroup tests
 *
 * @brief Test case for checking LtpQueueSet behavior and priorities.
 */
class LtpQueueSetTestCase : public TestCase
{
  public:
    LtpQueueSetTestCase();
    ~LtpQueueSetTestCase() override;

  private:
    void DoRun() override;

    /**
     * @brief Setup the initial test vectors.
     */
    void SetTests();

    /**
     * @brief Template class for tracking expected queue positions.
     * @tparam T The type of data to be stored.
     */
    template <class T>
    class TestVector
    {
      public:
        uint8_t m_position; ///< The expected extraction order position of this packet.
        T m_data;           ///< The packet to test enqueueing and dequeueing on.
    };

    TestVectors<TestVector<Ptr<ns3::Packet>>> m_tests; ///< The collection of test vectors.
};

LtpQueueSetTestCase::LtpQueueSetTestCase()
    : TestCase("LtpQueueSetTestCase test case (check queues behaviour)")
{
}

LtpQueueSetTestCase::~LtpQueueSetTestCase()
{
}

void
LtpQueueSetTestCase::SetTests()
{
    Ptr<ns3::Packet> packet1 = Create<ns3::Packet>();
    LtpHeader header;
    TestVector<Ptr<ns3::Packet>> test;

    header.SetVersion(0);
    header.SetSegmentType(LTPTYPE_RD);
    header.SetSessionId(SessionId(0, 0));
    header.SetHeaderExtensionCount(0b00000000);
    header.SetTrailerExtensionCount(0b00000000);

    packet1->AddHeader(header);

    test.m_position = 2;
    test.m_data = packet1;
    m_tests.Add(test);

    header.SetSegmentType(LTPTYPE_GD);
    Ptr<ns3::Packet> packet2 = Create<ns3::Packet>();
    packet2->AddHeader(header);

    test.m_position = 3;
    test.m_data = packet2;
    m_tests.Add(test);

    header.SetSegmentType(LTPTYPE_RS);
    Ptr<ns3::Packet> packet3 = Create<ns3::Packet>();
    packet3->AddHeader(header);

    test.m_position = 1;
    test.m_data = packet3;
    m_tests.Add(test);
}

void
LtpQueueSetTestCase::DoRun()
{
    LtpQueueSet queue;
    bool success = true;

    SetTests();

    for (uint32_t i = 0; i < m_tests.GetN(); i++)
    {
        TestVector<Ptr<ns3::Packet>> test = m_tests.Get(i);
        success = success & queue.Enqueue(test.m_data);
    }
    NS_TEST_ASSERT_MSG_EQ(success, true, "Enqueuing failed");
    NS_TEST_ASSERT_MSG_EQ((queue.GetNPackets() == m_tests.GetN()), true, "Wrong queue size");

    Ptr<ns3::Packet> packet = queue.Dequeue();
    NS_TEST_ASSERT_MSG_EQ((m_tests.Get(2).m_data == packet), true, "Wrong queue order");
    packet = queue.Dequeue();
    NS_TEST_ASSERT_MSG_EQ((m_tests.Get(0).m_data == packet), true, "Wrong queue order");
    packet = queue.Dequeue();
    NS_TEST_ASSERT_MSG_EQ((m_tests.Get(1).m_data == packet), true, "Wrong queue order");
}

/**
 * @ingroup dtn-test
 * @ingroup tests
 *
 * @brief Test case to verify internal behavior of LTP session state records, including timers and
 * claims.
 */
class LtpSessionStateRecordTestCase : public TestCase
{
  public:
    LtpSessionStateRecordTestCase();
    ~LtpSessionStateRecordTestCase() override;

  private:
    void DoRun() override;

    /**
     * @brief Setup test vectors for timer expiration logic.
     */
    void SetTimerTests();

    /**
     * @brief Method triggered upon timer execution to validate timing.
     * @param index The index of the TestTimer in m_testTimers.
     */
    void TimerTest(uint32_t index);

    /**
     * @brief Helper to resume a paused timer during testing.
     * @param index The index of the timer to resume.
     */
    void ResumeTimers(uint32_t index);

    /**
     * @brief Configuration for a simulated session timer.
     */
    struct TestTimer
    {
        uint64_t lapse;      ///< The initial duration before expiration.
        uint64_t total;      ///< The total expected simulation time upon expiration.
        uint8_t stops;       ///< How many times the timer will be suspended.
        uint8_t stop_lapses; ///< The duration of each suspension.
        TimerCode timeCode;  ///< The LTP timer code category.
    };

    TestVectors<TestTimer> m_testTimers; ///< The list of simulated timer tests.
};

LtpSessionStateRecordTestCase::LtpSessionStateRecordTestCase()
    : TestCase("LtpSessionStateRecordTestCase test case (check queues behaviour)")
{
}

LtpSessionStateRecordTestCase::~LtpSessionStateRecordTestCase()
{
}

void
LtpSessionStateRecordTestCase::TimerTest(uint32_t index)
{
    TestTimer test = m_testTimers.Get(index);

    uint64_t actual = Simulator::Now().GetSeconds();
    uint64_t limit = test.total;

    NS_TEST_ASSERT_MSG_EQ_TOL(actual, limit, 0.005, "Test1 Failed");
}

void
LtpSessionStateRecordTestCase::DoRun()
{
    Ptr<UniformRandomVariable> number = CreateObject<UniformRandomVariable>();

    Address destEngine = InetSocketAddress(Ipv4Address("10.0.0.2"), 1113);
    Address srcEngine = InetSocketAddress(Ipv4Address("10.0.0.1"), 1113);
    uint64_t destClient = 5000;
    uint64_t srcClient = 5000;

    Ptr<SenderSessionStateRecord> ssend = CreateObject<SenderSessionStateRecord>(srcEngine,
                                                                                 srcClient,
                                                                                 destClient,
                                                                                 destEngine,
                                                                                 number);

    SessionId id = ssend->GetSessionId();

    bool test = ((id.GetSessionNumber() >= SessionStateRecord::MIN_INITIAL_SERIAL_NUMBER) &&
                 (id.GetSessionNumber() <= SessionStateRecord::MAX_INITIAL_SERIAL_NUMBER) &&
                 id.GetSessionOriginator() ==
                     0); // We default initialized SessionId to 0 in SenderSessionStateRecord setup

    NS_TEST_ASSERT_MSG_EQ(test, true, "Wrong result for sessionId generation");

    Ptr<ReceiverSessionStateRecord> srecv =
        CreateObject<ReceiverSessionStateRecord>(srcEngine, srcClient, id, number);

    SetTimerTests();

    for (uint32_t i = 0; i < m_testTimers.GetN(); i++)
    {
        TestTimer testTimer = m_testTimers.Get(i);

        srecv->SetTimerFunction(&LtpSessionStateRecordTestCase::TimerTest,
                                this,
                                i,
                                Seconds(testTimer.lapse),
                                testTimer.timeCode);
        srecv->StartTimer(testTimer.timeCode);

        for (uint32_t j = 0; j < testTimer.stops; j++)
        {
            Simulator::Schedule(Seconds(testTimer.stop_lapses * j),
                                &ReceiverSessionStateRecord::SuspendTimer,
                                srecv,
                                testTimer.timeCode);
            Simulator::Schedule(Seconds(testTimer.stop_lapses * (j + 1)),
                                &ReceiverSessionStateRecord::ResumeTimer,
                                srecv,
                                testTimer.timeCode);
        }
    }

    LtpContentHeader::ReceptionClaim claim;

    claim.offset = 0;
    claim.length = 100;

    uint32_t lowerBound = 0;
    uint32_t upperBound = 1000;

    test = srecv->InsertClaim(srecv->GetRpCurrentSerialNumber(), lowerBound, upperBound, claim);
    NS_TEST_ASSERT_MSG_EQ(test, true, "First Claim not inserted");
    test = srecv->InsertClaim(srecv->GetRpCurrentSerialNumber(), lowerBound, upperBound, claim);
    NS_TEST_ASSERT_MSG_EQ(test, false, "Claim with repeated offset inserted");

    claim.offset = 100;
    claim.length = 100;

    test = srecv->InsertClaim(srecv->GetRpCurrentSerialNumber(), lowerBound, upperBound, claim);
    NS_TEST_ASSERT_MSG_EQ(test, true, "New Claim not inserted");
    test = srecv->InsertClaim(srecv->GetRpCurrentSerialNumber(), lowerBound, upperBound, claim);
    NS_TEST_ASSERT_MSG_EQ(test, false, "Claim with repeated offset inserted");

    srecv->IncrementRpCurrentSerialNumber();
    test = srecv->InsertClaim(srecv->GetRpCurrentSerialNumber(), lowerBound, upperBound, claim);
    NS_TEST_ASSERT_MSG_EQ(test, true, "Claim with new Report serial number not Inserted");

    test = ((srecv->GetRpCurrentSerialNumber() < SessionStateRecord::MAX_SERIAL_NUMBER + 1) &&
            (srecv->GetRpCurrentSerialNumber() >= 1));

    NS_TEST_ASSERT_MSG_EQ(test, true, "Wrong RP Serial Number");

    test = ((ssend->GetCpCurrentSerialNumber() < SessionStateRecord::MAX_SERIAL_NUMBER) &&
            (ssend->GetCpCurrentSerialNumber() >= 1));

    NS_TEST_ASSERT_MSG_EQ(test, true, "Wrong CP Serial Number");

    Simulator::Run();
    Simulator::Destroy();
}

void
LtpSessionStateRecordTestCase::SetTimerTests()
{
    TestTimer test;
    test.lapse = 1;
    test.stops = 0;
    test.stop_lapses = 0;
    test.total = test.lapse + (test.stop_lapses * test.stops);
    test.timeCode = CHECKPOINT;

    m_testTimers.Add(test);

    test.lapse = 2;
    test.stops = 1;
    test.stop_lapses = 1;
    test.total = test.lapse + (test.stop_lapses * test.stops);
    test.timeCode = REPORT;

    m_testTimers.Add(test);

    test.lapse = 4;
    test.stops = 3;
    test.stop_lapses = 2;
    test.total = test.lapse + (test.stop_lapses * test.stops);
    test.timeCode = CANCEL;

    m_testTimers.Add(test);
}

/**
 * @ingroup dtn-test
 * @ingroup tests
 *
 * @brief Test case checking the integration and attachment of the LTP CLA to the BundleAgent.
 */
class BundleAgentLtpClaTestCase : public TestCase
{
  public:
    BundleAgentLtpClaTestCase();
    ~BundleAgentLtpClaTestCase() override;

  private:
    void DoRun() override;
};

BundleAgentLtpClaTestCase::BundleAgentLtpClaTestCase()
    : TestCase("BundleAgentLtpClaTestCase test case (check protocol core)")
{
}

BundleAgentLtpClaTestCase::~BundleAgentLtpClaTestCase()
{
}

void
BundleAgentLtpClaTestCase::DoRun()
{
    Ptr<BundleAgent> agent = CreateObject<BundleAgent>();
    agent->SetLocalEID("dtn:nodeA");

    Ptr<LtpBundleCla> cla = CreateObject<LtpBundleCla>();

    bool test = agent->RegisterCla("dtn:nodeB", cla);
    NS_TEST_ASSERT_MSG_EQ(test, true, "New CLA registration failed");

    test = agent->RegisterCla("dtn:nodeB", cla);
    NS_TEST_ASSERT_MSG_EQ(test, false, "CLA registered twice");

    Ptr<LtpBundleCla> cla2 = CreateObject<LtpBundleCla>();
    test = agent->RegisterCla("dtn:nodeC", cla2);
    NS_TEST_ASSERT_MSG_EQ(test, true, "Second CLA registration failed");

    agent->UnregisterCla("dtn:nodeB");
}

/**
 * @ingroup dtn-test
 * @ingroup tests
 *
 * @brief Main test suite encapsulating all unit tests for the LTP protocol layer.
 */
class LtpProtocolTestSuite : public TestSuite
{
  public:
    LtpProtocolTestSuite();
};

LtpProtocolTestSuite::LtpProtocolTestSuite()
    : TestSuite("ltp-protocol", Type::UNIT)
{
    AddTestCase(new LtpHeaderTestCase, TestCase::Duration::QUICK);
    AddTestCase(new LtpQueueSetTestCase, TestCase::Duration::QUICK);
    AddTestCase(new LtpSessionStateRecordTestCase, TestCase::Duration::QUICK);
    AddTestCase(new BundleAgentLtpClaTestCase, TestCase::Duration::QUICK);
}

static LtpProtocolTestSuite ltpProtocolTestSuite;
