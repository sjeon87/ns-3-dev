/*
 * SPDX-License-Identifier: NIST-Software
 */

// NHDP unit tests (TestSuite "nhdp", Type::UNIT).
//
// Each behavioral test drives NhdpClient instances in BypassMode and wires
// per-pair, directional connectivity via HandleDirectLink{Established,Releasing}
// Trace, then asserts the internal Link, Neighbor, 2-Hop, and Lost Neighbor Sets
// after scheduled events.  The real UDP multicast transport is out of scope for
// these unit tests (it is left to future system tests).
//
//   1. TimeEncodeDecode RFC 5497 time TLV testing; test encode and decode is lossless,
//                       and check the boundary cases: minimum value, overflow clamp,
//                       nearest-representable rounding, and a non-default granularity
//                       constant.
//   2. SymmetricLink    Two nodes with a bidirectional link advance from HEARD to
//                       SYMMETRIC, with link and neighbor timers set from the
//                       advertised validity time and L_HOLD_TIME (RFC 6130 Sec.
//                       12.5, 13.1).
//   3. LinkTimeout      A symmetric link that stops being heard is advertised LOST,
//                       de-symmetrized, removed on expiry, and recorded in the Lost
//                       Neighbor Set for N_HOLD_TIME (RFC 6130 Sec. 13).
//   4. TwoHopFormation  In a three-node chain, the end node learns the far node as a
//                       symmetric 2-hop neighbor via the middle node, and records no
//                       2-hop entry to itself (RFC 6130 Sec. 12.6).
//
//   5. TwoHopTimeoutCleanup  With a 2-hop neighbor reachable via two relays, losing one
//                            relay's link removes the 2-Hop Tuple via that relay while the
//                            Tuple via the other relay persists (RFC 6130 Sec. 13.2).
//   6. LinkQualityHysteresis With link quality in use, a link becomes usable only
//                            when quality reaches HYST_ACCEPT and is lost when it
//                            falls below HYST_REJECT, with no flapping between the
//                            two thresholds (RFC 6130 Sec. 14).
//   7. LinkFailure           A reported link-layer failure immediately sets the link
//                            LOST, de-symmetrizes the neighbor, and removes that
//                            neighbor's 2-Hop Tuples (RFC 6130 Sec. 13.2, 14.3).
//   8. HeterogeneousValidity When two nodes advertise different validity times, each
//                            node's timers for the other derive from the peer's
//                            advertised validity, while the L_HOLD_TIME term uses the
//                            receiver's own local value (RFC 6130 Sec. 12.5).
//

#include "ns3/application-container.h"
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/net-device-container.h"
#include "ns3/nhdp-client.h"
#include "ns3/nhdp-helper.h"
#include "ns3/nhdp-info-base.h"
#include "ns3/node-container.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simple-net-device-helper.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/time-tlv.h"
#include "ns3/uinteger.h"

#include <map>
#include <set>
#include <vector>

using namespace ns3;
using namespace ns3::manet;

NS_LOG_COMPONENT_DEFINE("NhdpTestSuite");

/**
 * @defgroup nhdp-tests Tests for the nhdp module
 * @ingroup nhdp
 * @ingroup tests
 */

/**
 * @ingroup nhdp-tests
 *
 * @brief Shared harness for NHDP behavioral unit tests.
 *
 * Builds a set of single-interface IPv4 nodes (addresses 7.0.0.<id>), each
 * running an NhdpClient in BypassMode, and provides helpers to wire directional
 * connectivity, drive link quality, and assert the resulting Information Bases.
 * Logical node ids run from 1 to the node count.
 */
class NhdpTestCase : public TestCase
{
  public:
    /**
     * Constructor.
     * @param name The test case name.
     */
    NhdpTestCase(std::string name);

    /**
     * Create count nodes with an Internet stack, a SimpleNetDevice to host the
     * MANET address, and an NhdpClient in BypassMode.  Connectivity starts empty.
     *
     * @param count Number of nodes to create.
     * @param helloInterval HelloInterval attribute for each NhdpClient.
     * @param holdTime HHoldTime (advertised validity) attribute for each NhdpClient.
     */
    void CreateNodes(uint32_t count, Time helloInterval, Time holdTime);

    Ptr<NhdpClient> Client(uint32_t id) const; //!< NhdpClient for logical node id
    Ipv4Address Addr(uint32_t id) const;       //!< MANET address 7.0.0.<id>

    /**
     * Set up one-way connectivity so that the receiving node hears the transmitting node.
     * @param from The transmitting node id.
     * @param to The receiving node id.
     */
    void LinkDirected(uint32_t from, uint32_t to);
    /**
     * Establish a bidirectional link between two nodes.
     * @param a A node id.
     * @param b A node id.
     */
    void Link(uint32_t a, uint32_t b);
    /**
     * Remove one-way connectivity so that the receiving node no longer hears the
     * transmitting node.
     * @param from The transmitting node id.
     * @param to The receiving node id.
     */
    void UnlinkDirected(uint32_t from, uint32_t to);
    /**
     * Remove a bidirectional link between two nodes.
     * @param a A node id.
     * @param b A node id.
     */
    void Unlink(uint32_t a, uint32_t b);

    /**
     * Register the shared link-quality callback on node id, so that received
     * HELLOs at that node are assigned the value last set via SetQuality().
     * @param id The node id.
     */
    void EnableQuality(uint32_t id);
    /**
     * Set the link quality value returned to nodes with quality enabled.
     * @param quality The link quality in [0,1].
     */
    void SetQuality(double quality);

    /**
     * Assert that node id has a Link Tuple to node neighborId with the expected status.
     * @param id The observing node id.
     * @param neighborId The neighbor node id.
     * @param status The expected LinkStatus.
     */
    void CheckLinkStatus(uint32_t id, uint32_t neighborId, LinkStatus status);
    /**
     * Assert that node id has no Link Tuple to node neighborId.
     * @param id The observing node id.
     * @param neighborId The neighbor node id.
     */
    void CheckNoLink(uint32_t id, uint32_t neighborId);
    /**
     * Assert node id's Neighbor Tuple to neighborId has the given symmetry.
     * @param id The observing node id.
     * @param neighborId The neighbor node id.
     * @param symmetric The expected N_symmetric value.
     */
    void CheckSymmetric(uint32_t id, uint32_t neighborId, bool symmetric);
    /**
     * Assert that node id has no Neighbor Tuple to node neighborId.
     * @param id The observing node id.
     * @param neighborId The neighbor node id.
     */
    void CheckNoNeighbor(uint32_t id, uint32_t neighborId);
    /**
     * Assert the size of node id's 2-Hop Set.
     * @param id The observing node id.
     * @param count The expected number of 2-Hop Tuples.
     */
    void CheckTwoHopCount(uint32_t id, std::size_t count);
    /**
     * Assert presence/absence of a 2-Hop Tuple at node id.
     * @param id The observing node id.
     * @param viaId The 1-hop neighbor (relay) node id.
     * @param twoHopId The 2-hop neighbor node id.
     * @param present Whether the tuple is expected to be present.
     */
    void CheckTwoHop(uint32_t id, uint32_t viaId, uint32_t twoHopId, bool present);
    /**
     * Assert presence/absence of a Lost Neighbor Tuple at node id.
     * @param id The observing node id.
     * @param neighborId The lost neighbor node id.
     * @param present Whether the tuple is expected to be present.
     */
    void CheckLostNeighbor(uint32_t id, uint32_t neighborId, bool present);
    /**
     * Assert node id's Link Tuple timers to neighborId (within tolerance).
     * @param id The observing node id.
     * @param neighborId The neighbor node id.
     * @param heard Expected L_HEARD_time.
     * @param sym Expected L_SYM_time.
     * @param expiration Expected L_time.
     */
    void CheckLinkTimers(uint32_t id, uint32_t neighborId, Time heard, Time sym, Time expiration);

    /**
     * Report a link-layer failure on node id for its link to node neighborId.
     * @param id The node id experiencing the failure.
     * @param neighborId The neighbor node id whose link failed.
     */
    void TriggerLinkFailure(uint32_t id, uint32_t neighborId);
    /**
     * Connect node id's LinkFailure trace so reported failures are recorded.
     * @param id The node id.
     */
    void ConnectLinkFailure(uint32_t id);
    /**
     * Assert whether a link failure to node neighborId has been reported on a connected trace.
     * @param neighborId The neighbor node id.
     * @param reported Whether a failure to that neighbor is expected to have been recorded.
     */
    void CheckLinkFailureReported(uint32_t neighborId, bool reported);

  protected:
    void DoSetup() override;
    void DoTeardown() override;

  private:
    uint32_t NodeId(uint32_t id) const;               //!< ns-3 NodeList id for logical node id
    double QualityCallback(Ptr<Packet> packet) const; //!< Shared quality callback
    void RecordLinkFailure(const Ipv4Address& addr);  //!< LinkFailure trace sink

    NodeContainer m_nodes;                  //!< The created nodes (index id-1)
    std::vector<Ptr<NhdpClient>> m_clients; //!< NhdpClient per node (index id-1)
    std::vector<Ipv4Address> m_addrs;       //!< MANET address per node (index id-1)
    double m_quality{1.0};                  //!< Value returned by QualityCallback()
    std::set<Ipv4Address> m_linkFailures;   //!< Addresses reported on the LinkFailure trace
    Time m_timerTolerance{MilliSeconds(1)}; //!< Tolerance for timer assertions
    uint32_t m_previousSeed{1};             //!< RngSeed before the test, restored on teardown
    uint64_t m_previousRun{1};              //!< RngRun before the test, restored on teardown
};

NhdpTestCase::NhdpTestCase(std::string name)
    : TestCase(name)
{
}

void
NhdpTestCase::DoSetup()
{
    // Save the RNG seed and run in effect so DoTeardown() can restore them; CreateNodes()
    // overrides them with fixed values for reproducibility.
    m_previousSeed = RngSeedManager::GetSeed();
    m_previousRun = RngSeedManager::GetRun();
}

void
NhdpTestCase::CreateNodes(uint32_t count, Time helloInterval, Time holdTime)
{
    NS_LOG_DEBUG("Creating " << count << " NHDP nodes; helloInterval " << helloInterval.As(Time::S)
                             << ", hHoldTime " << holdTime.As(Time::S));

    // Use a fixed RNG seed and run for reproducibility (DoSetup() saved the previous
    // values and DoTeardown() restores them).
    Config::SetGlobal("RngSeed", UintegerValue(1));
    Config::SetGlobal("RngRun", UintegerValue(1));

    m_nodes.Create(count);

    InternetStackHelper internet;
    internet.Install(m_nodes);

    // The SimpleNetDevice only hosts the MANET interface address; in BypassMode no
    // traffic is sent over its channel.
    SimpleNetDeviceHelper simpleNet;
    NetDeviceContainer devices = simpleNet.Install(m_nodes);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("7.0.0.0", "255.255.255.0"); // assigns 7.0.0.1, 7.0.0.2, ...
    Ipv4InterfaceContainer ifaces = ipv4.Assign(devices);

    NhdpHelper nhdp;
    nhdp.SetAttribute("BypassMode", BooleanValue(true));
    nhdp.SetAttribute("HelloInterval", TimeValue(helloInterval));
    nhdp.SetAttribute("RefreshInterval", TimeValue(helloInterval));
    nhdp.SetAttribute("HHoldTime", TimeValue(holdTime));
    // Disable HELLO jitter so each node transmits at multiples of HelloInterval,
    // making the per-node send and reception times deterministic for timer assertions.
    nhdp.SetAttribute("HPMaxJitter", TimeValue(Seconds(0)));
    ApplicationContainer apps = nhdp.Install(m_nodes);

    for (uint32_t i = 0; i < count; i++)
    {
        auto client = apps.Get(i)->GetObject<NhdpClient>();
        client->AssignStreams(100 + i);
        m_clients.push_back(client);
        m_addrs.push_back(ifaces.GetAddress(i));
    }
    apps.Start(Seconds(0));
}

void
NhdpTestCase::DoTeardown()
{
    // Restore the RNG seed and run that were in effect before this test.
    Config::SetGlobal("RngSeed", UintegerValue(m_previousSeed));
    Config::SetGlobal("RngRun", UintegerValue(m_previousRun));
}

Ptr<NhdpClient>
NhdpTestCase::Client(uint32_t id) const
{
    return m_clients.at(id - 1); // logical node ids run from 1
}

Ipv4Address
NhdpTestCase::Addr(uint32_t id) const
{
    return m_addrs.at(id - 1); // logical node ids run from 1
}

uint32_t
NhdpTestCase::NodeId(uint32_t id) const
{
    return Client(id)->GetNode()->GetId();
}

void
NhdpTestCase::LinkDirected(uint32_t from, uint32_t to)
{
    // After this, node `to` receives HELLOs sent by node `from`.
    Client(from)->HandleDirectLinkEstablishedTrace(NodeId(from), Addr(from), NodeId(to), Addr(to));
    NS_LOG_DEBUG("Link up: node " << to << " can now hear node " << from);
}

void
NhdpTestCase::Link(uint32_t a, uint32_t b)
{
    LinkDirected(a, b);
    LinkDirected(b, a);
}

void
NhdpTestCase::UnlinkDirected(uint32_t from, uint32_t to)
{
    Client(from)->HandleDirectLinkReleasingTrace(NodeId(from), Addr(from), NodeId(to), Addr(to));
    NS_LOG_DEBUG("Link down: node " << to << " no longer hears node " << from);
}

void
NhdpTestCase::Unlink(uint32_t a, uint32_t b)
{
    UnlinkDirected(a, b);
    UnlinkDirected(b, a);
}

double
NhdpTestCase::QualityCallback(Ptr<Packet> packet [[maybe_unused]]) const
{
    return m_quality;
}

void
NhdpTestCase::EnableQuality(uint32_t id)
{
    Client(id)->RegisterLinkQualityCallback(MakeCallback(&NhdpTestCase::QualityCallback, this));
    NS_LOG_DEBUG("Enabled link quality on node " << id);
}

void
NhdpTestCase::SetQuality(double quality)
{
    m_quality = quality;
    NS_LOG_DEBUG("Set link quality to " << quality);
}

void
NhdpTestCase::CheckLinkStatus(uint32_t id, uint32_t neighborId, LinkStatus status)
{
    NS_LOG_DEBUG("Checking node " << id << " link to " << neighborId << " has status " << status
                                  << " at " << Simulator::Now().As(Time::S));
    const auto& base = Client(id)->GetLinkInfoBase();
    auto it = base.find(Addr(neighborId));
    NS_TEST_ASSERT_MSG_EQ((it != base.end()),
                          true,
                          "Node " << id << " missing Link Tuple to " << neighborId << " at "
                                  << Simulator::Now().As(Time::S));
    NS_TEST_ASSERT_MSG_EQ(it->second.GetLinkStatus(),
                          status,
                          "Node " << id << " Link Tuple to " << neighborId << " wrong status at "
                                  << Simulator::Now().As(Time::S));
}

void
NhdpTestCase::CheckNoLink(uint32_t id, uint32_t neighborId)
{
    NS_LOG_DEBUG("Checking node " << id << " has no link to " << neighborId << " at "
                                  << Simulator::Now().As(Time::S));
    const auto& base = Client(id)->GetLinkInfoBase();
    NS_TEST_ASSERT_MSG_EQ((base.find(Addr(neighborId)) == base.end()),
                          true,
                          "Node " << id << " unexpectedly has a Link Tuple to " << neighborId
                                  << " at " << Simulator::Now().As(Time::S));
}

void
NhdpTestCase::CheckSymmetric(uint32_t id, uint32_t neighborId, bool symmetric)
{
    NS_LOG_DEBUG("Checking node " << id << " neighbor " << neighborId << " symmetric=" << symmetric
                                  << " at " << Simulator::Now().As(Time::S));
    const auto& base = Client(id)->GetNeighborInfoBase();
    auto it = base.find(Addr(neighborId));
    NS_TEST_ASSERT_MSG_EQ((it != base.end()),
                          true,
                          "Node " << id << " missing Neighbor Tuple to " << neighborId << " at "
                                  << Simulator::Now().As(Time::S));
    NS_TEST_ASSERT_MSG_EQ(it->second.m_symmetric,
                          symmetric,
                          "Node " << id << " Neighbor Tuple to " << neighborId
                                  << " wrong symmetry at " << Simulator::Now().As(Time::S));
}

void
NhdpTestCase::CheckNoNeighbor(uint32_t id, uint32_t neighborId)
{
    NS_LOG_DEBUG("Checking node " << id << " has no neighbor " << neighborId << " at "
                                  << Simulator::Now().As(Time::S));
    const auto& base = Client(id)->GetNeighborInfoBase();
    NS_TEST_ASSERT_MSG_EQ((base.find(Addr(neighborId)) == base.end()),
                          true,
                          "Node " << id << " unexpectedly has a Neighbor Tuple to " << neighborId
                                  << " at " << Simulator::Now().As(Time::S));
}

void
NhdpTestCase::CheckTwoHopCount(uint32_t id, std::size_t count)
{
    NS_LOG_DEBUG("Checking node " << id << " 2-hop set size == " << count << " at "
                                  << Simulator::Now().As(Time::S));
    NS_TEST_ASSERT_MSG_EQ(Client(id)->GetTwoHopInfoBase().size(),
                          count,
                          "Node " << id << " wrong 2-Hop Set size at "
                                  << Simulator::Now().As(Time::S));
}

void
NhdpTestCase::CheckTwoHop(uint32_t id, uint32_t viaId, uint32_t twoHopId, bool present)
{
    NS_LOG_DEBUG("Checking node " << id << " 2-hop to " << twoHopId << " via " << viaId
                                  << " present=" << present << " at "
                                  << Simulator::Now().As(Time::S));
    const auto& base = Client(id)->GetTwoHopInfoBase();
    auto key = std::make_pair(Addr(viaId), Addr(twoHopId));
    NS_TEST_ASSERT_MSG_EQ((base.find(key) != base.end()),
                          present,
                          "Node " << id << " 2-Hop Tuple to " << twoHopId << " via " << viaId
                                  << " presence mismatch at " << Simulator::Now().As(Time::S));
}

void
NhdpTestCase::CheckLostNeighbor(uint32_t id, uint32_t neighborId, bool present)
{
    NS_LOG_DEBUG("Checking node " << id << " lost-neighbor " << neighborId << " present=" << present
                                  << " at " << Simulator::Now().As(Time::S));
    const auto& base = Client(id)->GetLostNeighborSet();
    NS_TEST_ASSERT_MSG_EQ((base.find(Addr(neighborId)) != base.end()),
                          present,
                          "Node " << id << " lost-neighbor " << neighborId
                                  << " presence mismatch at " << Simulator::Now().As(Time::S));
}

void
NhdpTestCase::CheckLinkTimers(uint32_t id,
                              uint32_t neighborId,
                              Time heard,
                              Time sym,
                              Time expiration)
{
    NS_LOG_DEBUG("Checking node " << id << " link timers to " << neighborId << " at "
                                  << Simulator::Now().As(Time::S));
    const auto& base = Client(id)->GetLinkInfoBase();
    auto it = base.find(Addr(neighborId));
    NS_TEST_ASSERT_MSG_EQ((it != base.end()),
                          true,
                          "Node " << id << " missing Link Tuple to " << neighborId << " at "
                                  << Simulator::Now().As(Time::S));
    NS_TEST_ASSERT_MSG_EQ_TOL(it->second.m_heardTime,
                              heard,
                              m_timerTolerance,
                              "Node " << id << " L_HEARD_time to " << neighborId << " wrong");
    NS_TEST_ASSERT_MSG_EQ_TOL(it->second.m_symTime,
                              sym,
                              m_timerTolerance,
                              "Node " << id << " L_SYM_time to " << neighborId << " wrong");
    NS_TEST_ASSERT_MSG_EQ_TOL(it->second.m_expirationTime,
                              expiration,
                              m_timerTolerance,
                              "Node " << id << " L_time to " << neighborId << " wrong");
}

void
NhdpTestCase::TriggerLinkFailure(uint32_t id, uint32_t neighborId)
{
    NS_LOG_DEBUG("Reporting link failure on node " << id << " for neighbor " << neighborId);
    Client(id)->HandleLinkFailure(Addr(neighborId));
}

void
NhdpTestCase::ConnectLinkFailure(uint32_t id)
{
    Client(id)->TraceConnectWithoutContext("LinkFailure",
                                           MakeCallback(&NhdpTestCase::RecordLinkFailure, this));
}

void
NhdpTestCase::RecordLinkFailure(const Ipv4Address& addr)
{
    NS_LOG_DEBUG("LinkFailure trace fired for " << addr);
    m_linkFailures.insert(addr);
}

void
NhdpTestCase::CheckLinkFailureReported(uint32_t neighborId, bool reported)
{
    NS_LOG_DEBUG("Checking link failure to " << neighborId << " reported=" << reported << " at "
                                             << Simulator::Now().As(Time::S));
    NS_TEST_ASSERT_MSG_EQ((m_linkFailures.count(Addr(neighborId)) > 0),
                          reported,
                          "Link failure to " << neighborId << " report mismatch at "
                                             << Simulator::Now().As(Time::S));
}

/**
 * @ingroup nhdp-tests
 *
 * @brief Verifies the RFC 5497 time-code codec (EncodeTimeCode/DecodeTimeCode).
 */
class TimeEncodeDecodeTestCase : public TestCase
{
  public:
    TimeEncodeDecodeTestCase();

  protected:
    void DoRun() override;
};

TimeEncodeDecodeTestCase::TimeEncodeDecodeTestCase()
    : TestCase("RFC 5497 time-code encode and decode")
{
}

void
TimeEncodeDecodeTestCase::DoRun()
{
    // Default granularity C = 1/1024 s.  Decode for known time-codes.
    NS_LOG_DEBUG("Checking decode and encode for the default granularity C");
    NS_TEST_ASSERT_MSG_EQ(DecodeTimeCode(100), Seconds(6), "time-code 100 should decode to 6 s");
    NS_TEST_ASSERT_MSG_EQ(DecodeTimeCode(88), Seconds(2), "time-code 88 should decode to 2 s");

    // The default hold/interval times (6 s, 2 s) are representable as doubles, so they
    // encode to their expected time-codes with no floating-point rounding.
    NS_TEST_ASSERT_MSG_EQ(static_cast<uint32_t>(EncodeTimeCode(Seconds(6))),
                          100u,
                          "6 s should encode to time-code 100");
    NS_TEST_ASSERT_MSG_EQ(static_cast<uint32_t>(EncodeTimeCode(Seconds(2))),
                          88u,
                          "2 s should encode to time-code 88");

    // Minimum: values at or below C encode to time-code 0.
    NS_LOG_DEBUG("Checking minimum and overflow boundary cases");
    NS_TEST_ASSERT_MSG_EQ(static_cast<uint32_t>(EncodeTimeCode(Seconds(TIME_TLV_C))),
                          0u,
                          "C should encode to time-code 0");
    NS_TEST_ASSERT_MSG_EQ(static_cast<uint32_t>(EncodeTimeCode(Seconds(0.0001))),
                          0u,
                          "a value below C should encode to time-code 0");

    // Overflow: values beyond the maximum representable clamp to time-code 255.
    NS_TEST_ASSERT_MSG_EQ(static_cast<uint32_t>(EncodeTimeCode(Seconds(1e8))),
                          255u,
                          "a value beyond the maximum should clamp to time-code 255");

    // Nearest-representable rounding for a value not representable as a
    // double (0.1 s).  The smallest representable value not less than 0.1 s is
    // time-code 53 (0.1015625 s); time-code 52 (0.09375 s) is just below.
    NS_LOG_DEBUG("Checking nearest-representable rounding for 0.1 s");
    NS_TEST_ASSERT_MSG_EQ(static_cast<uint32_t>(EncodeTimeCode(Seconds(0.1))),
                          53u,
                          "0.1 s should round up to time-code 53");
    NS_TEST_ASSERT_MSG_EQ(DecodeTimeCode(53),
                          Seconds(0.1015625),
                          "time-code 53 should decode to 0.1015625 s");
    NS_TEST_ASSERT_MSG_EQ(DecodeTimeCode(52),
                          Seconds(0.09375),
                          "time-code 52 should decode to 0.09375 s");

    // A non-default granularity constant (C = 1/512 s) encodes and decodes consistently.
    NS_LOG_DEBUG("Checking encode and decode with a non-default granularity C = 1/512 s");
    NS_TEST_ASSERT_MSG_EQ(static_cast<uint32_t>(EncodeTimeCode(Seconds(6), 1.0 / 512.0)),
                          92u,
                          "6 s with C = 1/512 should encode to time-code 92");
    NS_TEST_ASSERT_MSG_EQ(DecodeTimeCode(92, 1.0 / 512.0),
                          Seconds(6),
                          "time-code 92 with C = 1/512 should decode to 6 s");
}

/**
 * @ingroup nhdp-tests
 *
 * @brief Two nodes with a bidirectional link advance from HEARD to SYMMETRIC.
 *
 * Verifies the core HELLO handshake (RFC 6130 Sec. 12.5, 13.1) and that the Link
 * Tuple timers are derived from the advertised validity time and the local
 * L_HOLD_TIME.
 */
class SymmetricLinkTestCase : public NhdpTestCase
{
  public:
    SymmetricLinkTestCase();

  protected:
    void DoRun() override;
};

SymmetricLinkTestCase::SymmetricLinkTestCase()
    : NhdpTestCase("Two-node symmetric link establishment")
{
}

void
SymmetricLinkTestCase::DoRun()
{
    NS_LOG_DEBUG("Two-node bidirectional link: expect HEARD then SYMMETRIC");

    const Time helloInterval = Seconds(1);
    const Time validity = Seconds(3);  // advertised H_HOLD_TIME
    const Time lHoldTime = Seconds(6); // model default LHoldTime (receiver's L_HOLD_TIME)
    CreateNodes(2, helloInterval, validity);
    Link(1, 2);

    // Jitter is disabled, so each node transmits at t = 1 s, 2 s, ...  At t = 1 s both
    // HELLOs carry only the LOCAL_IF block (neither node has heard the other yet), so each
    // node creates a HEARD link to the other with L_HEARD_time = t + validity and
    // L_SYM_time EXPIRED.
    Simulator::Schedule(Seconds(1.5),
                        &NhdpTestCase::CheckLinkStatus,
                        this,
                        1,
                        2,
                        LinkStatus::HEARD);
    Simulator::Schedule(Seconds(1.5),
                        &NhdpTestCase::CheckLinkStatus,
                        this,
                        2,
                        1,
                        LinkStatus::HEARD);
    Simulator::Schedule(Seconds(1.5), &NhdpTestCase::CheckSymmetric, this, 1, 2, false);
    Simulator::Schedule(Seconds(1.5), &NhdpTestCase::CheckSymmetric, this, 2, 1, false);
    Simulator::Schedule(Seconds(1.5),
                        &NhdpTestCase::CheckLinkTimers,
                        this,
                        1,
                        2,
                        Seconds(1) + validity,
                        Seconds(0),
                        Seconds(1) + validity);

    // At t = 2 s each node hears itself listed as HEARD, so the link becomes SYMMETRIC with
    // L_SYM_time = L_HEARD_time = t + validity and L_time = L_HEARD_time + L_HOLD_TIME.
    Simulator::Schedule(Seconds(2.5),
                        &NhdpTestCase::CheckLinkStatus,
                        this,
                        1,
                        2,
                        LinkStatus::SYMMETRIC);
    Simulator::Schedule(Seconds(2.5),
                        &NhdpTestCase::CheckLinkStatus,
                        this,
                        2,
                        1,
                        LinkStatus::SYMMETRIC);
    Simulator::Schedule(Seconds(2.5), &NhdpTestCase::CheckSymmetric, this, 1, 2, true);
    Simulator::Schedule(Seconds(2.5), &NhdpTestCase::CheckSymmetric, this, 2, 1, true);
    Simulator::Schedule(Seconds(2.5),
                        &NhdpTestCase::CheckLinkTimers,
                        this,
                        1,
                        2,
                        Seconds(2) + validity,
                        Seconds(2) + validity,
                        Seconds(2) + validity + lHoldTime);
    Simulator::Schedule(Seconds(2.5),
                        &NhdpTestCase::CheckLinkTimers,
                        this,
                        2,
                        1,
                        Seconds(2) + validity,
                        Seconds(2) + validity,
                        Seconds(2) + validity + lHoldTime);

    Simulator::Stop(Seconds(3));
    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup nhdp-tests
 *
 * @brief A symmetric link that stops being heard is torn down and recorded as lost.
 *
 * Verifies the RFC 6130 Sec. 13 timeout behavior: once a symmetric link's L_SYM_time and
 * L_HEARD_time expire, the Link and Neighbor Tuples are removed and the neighbor is held in
 * the Lost Neighbor Set for N_HOLD_TIME.
 */
class LinkTimeoutTestCase : public NhdpTestCase
{
  public:
    LinkTimeoutTestCase();

  protected:
    void DoRun() override;
};

LinkTimeoutTestCase::LinkTimeoutTestCase()
    : NhdpTestCase("Symmetric link timeout and lost-neighbor recording")
{
}

void
LinkTimeoutTestCase::DoRun()
{
    NS_LOG_DEBUG("Two-node symmetric link, then loss: expect teardown and lost-neighbor record");

    const Time helloInterval = Seconds(1);
    const Time validity = Seconds(3); // advertised H_HOLD_TIME
    CreateNodes(2, helloInterval, validity);
    Link(1, 2);

    // The link is symmetric by t = 2 s and is refreshed each second.  Stop the two nodes
    // from hearing each other at t = 5.5 s; the last reception was at t = 5 s, so
    // L_SYM_time = L_HEARD_time = 5 s + validity = 8 s.
    Simulator::Schedule(Seconds(5.5), &NhdpTestCase::Unlink, this, 1, 2);

    // Before those timers expire (t < 8 s) the link is still SYMMETRIC.
    Simulator::Schedule(Seconds(7.5),
                        &NhdpTestCase::CheckLinkStatus,
                        this,
                        1,
                        2,
                        LinkStatus::SYMMETRIC);
    Simulator::Schedule(Seconds(7.5), &NhdpTestCase::CheckSymmetric, this, 1, 2, true);

    // At t = 8 s both L_SYM_time and L_HEARD_time expire: each node removes the Link and
    // Neighbor Tuples and records the other in the Lost Neighbor Set.
    Simulator::Schedule(Seconds(8.5), &NhdpTestCase::CheckNoLink, this, 1, 2);
    Simulator::Schedule(Seconds(8.5), &NhdpTestCase::CheckNoNeighbor, this, 1, 2);
    Simulator::Schedule(Seconds(8.5), &NhdpTestCase::CheckLostNeighbor, this, 1, 2, true);
    Simulator::Schedule(Seconds(8.5), &NhdpTestCase::CheckNoLink, this, 2, 1);
    Simulator::Schedule(Seconds(8.5), &NhdpTestCase::CheckNoNeighbor, this, 2, 1);
    Simulator::Schedule(Seconds(8.5), &NhdpTestCase::CheckLostNeighbor, this, 2, 1, true);

    // The Lost Neighbor Tuple expires N_HOLD_TIME (default 6 s) after it was added, at
    // t = 8 s + 6 s = 14 s.
    Simulator::Schedule(Seconds(13.5), &NhdpTestCase::CheckLostNeighbor, this, 1, 2, true);
    Simulator::Schedule(Seconds(14.5), &NhdpTestCase::CheckLostNeighbor, this, 1, 2, false);

    Simulator::Stop(Seconds(15));
    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup nhdp-tests
 *
 * @brief In a three-node chain, the end node learns the far node as a 2-hop neighbor.
 *
 * Verifies RFC 6130 Sec. 12.6 2-Hop Set formation: node 1 (linked only to node 2) learns
 * node 3 as a symmetric 2-hop neighbor via node 2 once node 2 advertises node 3 as a
 * symmetric 1-hop neighbor, and records no 2-Hop Tuple to itself.
 */
class TwoHopFormationTestCase : public NhdpTestCase
{
  public:
    TwoHopFormationTestCase();

  protected:
    void DoRun() override;
};

TwoHopFormationTestCase::TwoHopFormationTestCase()
    : NhdpTestCase("Three-node chain 2-hop neighbor formation")
{
}

void
TwoHopFormationTestCase::DoRun()
{
    NS_LOG_DEBUG("Three-node chain 1-2-3: node 1 should learn node 3 as a 2-hop neighbor via 2");

    const Time helloInterval = Seconds(1);
    const Time validity = Seconds(3); // advertised H_HOLD_TIME
    CreateNodes(3, helloInterval, validity);
    // Chain: node 2 is linked to both 1 and 3; nodes 1 and 3 cannot hear each other.
    Link(1, 2);
    Link(2, 3);

    // By t = 2 s the 1-2 and 2-3 links are symmetric, but node 2's t = 2 s HELLO still lists
    // its neighbors as HEARD (they became symmetric only while processing that round), so
    // node 1 has no 2-Hop Tuple yet.  Node 1 never hears node 3 directly.
    Simulator::Schedule(Seconds(2.5),
                        &NhdpTestCase::CheckLinkStatus,
                        this,
                        1,
                        2,
                        LinkStatus::SYMMETRIC);
    Simulator::Schedule(Seconds(2.5), &NhdpTestCase::CheckSymmetric, this, 1, 2, true);
    Simulator::Schedule(Seconds(2.5), &NhdpTestCase::CheckNoLink, this, 1, 3);
    Simulator::Schedule(Seconds(2.5), &NhdpTestCase::CheckTwoHopCount, this, 1, 0);

    // At t = 3 s node 2 advertises node 3 as SYMMETRIC; node 1 (symmetric with node 2) then
    // creates a 2-Hop Tuple to node 3 via node 2, and none to itself.  Node 3 learns node 1
    // symmetrically in the same way.
    Simulator::Schedule(Seconds(3.5), &NhdpTestCase::CheckTwoHopCount, this, 1, 1);
    Simulator::Schedule(Seconds(3.5), &NhdpTestCase::CheckTwoHop, this, 1, 2, 3, true);
    Simulator::Schedule(Seconds(3.5), &NhdpTestCase::CheckTwoHop, this, 1, 2, 1, false);
    Simulator::Schedule(Seconds(3.5), &NhdpTestCase::CheckTwoHop, this, 3, 2, 1, true);

    Simulator::Stop(Seconds(5));
    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup nhdp-tests
 *
 * @brief A 2-hop neighbor reachable via two relays loses only the Tuple via a failed relay.
 *
 * Node 1 reaches node 3 as a 2-hop neighbor via both node 2 and node 4.  When the link to
 * node 2 is lost, node 1's 2-Hop Tuple via node 2 is removed (RFC 6130 Sec. 13.2) while the
 * Tuple via node 4 persists.  This guards the multi-hop teardown behavior; the removal
 * coincides with the periodic 2-Hop Set expiry sweep, so it does not isolate any single
 * removal path.
 */
class TwoHopTimeoutCleanupTestCase : public NhdpTestCase
{
  public:
    TwoHopTimeoutCleanupTestCase();

  protected:
    void DoRun() override;
};

TwoHopTimeoutCleanupTestCase::TwoHopTimeoutCleanupTestCase()
    : NhdpTestCase("Two-hop tuple cleanup when a relay link is lost")
{
}

void
TwoHopTimeoutCleanupTestCase::DoRun()
{
    NS_LOG_DEBUG("Node 1 reaches node 3 via relays 2 and 4; dropping relay 2 keeps the path via 4");

    const Time helloInterval = Seconds(1);
    const Time validity = Seconds(3); // advertised H_HOLD_TIME
    CreateNodes(4, helloInterval, validity);
    // Two disjoint relays between node 1 and node 3; node 1 cannot hear node 3, and the two
    // relays cannot hear each other.
    Link(1, 2);
    Link(2, 3);
    Link(1, 4);
    Link(4, 3);

    // By t = 3 s node 1 has learned node 3 as a 2-hop neighbor via both relays.
    Simulator::Schedule(Seconds(4.5), &NhdpTestCase::CheckTwoHopCount, this, 1, 2);
    Simulator::Schedule(Seconds(4.5), &NhdpTestCase::CheckTwoHop, this, 1, 2, 3, true);
    Simulator::Schedule(Seconds(4.5), &NhdpTestCase::CheckTwoHop, this, 1, 4, 3, true);

    // Lose the link to relay 2 only (last reception t = 5 s, so L_SYM_time = 8 s).  The
    // 2-Hop Tuple via node 2 is gone once that link times out, while the Tuple via node 4
    // keeps being refreshed and remains.
    Simulator::Schedule(Seconds(5.5), &NhdpTestCase::Unlink, this, 1, 2);
    Simulator::Schedule(Seconds(8.5), &NhdpTestCase::CheckNoLink, this, 1, 2);
    Simulator::Schedule(Seconds(8.5),
                        &NhdpTestCase::CheckLinkStatus,
                        this,
                        1,
                        4,
                        LinkStatus::SYMMETRIC);
    Simulator::Schedule(Seconds(8.5), &NhdpTestCase::CheckTwoHopCount, this, 1, 1);
    Simulator::Schedule(Seconds(8.5), &NhdpTestCase::CheckTwoHop, this, 1, 2, 3, false);
    Simulator::Schedule(Seconds(8.5), &NhdpTestCase::CheckTwoHop, this, 1, 4, 3, true);

    Simulator::Stop(Seconds(10));
    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup nhdp-tests
 *
 * @brief Link quality gates link usability with accept/reject hysteresis (RFC 6130 Sec. 14).
 *
 * Quality is enabled on node 1 only (node 2 is a normal peer that always advertises node 1).
 * With HYST_ACCEPT = 0.8 and HYST_REJECT = 0.3, the link to node 2 stays pending while
 * quality is below HYST_ACCEPT, becomes usable (SYMMETRIC) once quality reaches HYST_ACCEPT,
 * stays usable when quality falls back between the thresholds (hysteresis), and is rejected
 * (returned to a non-usable, pending state) once quality drops below HYST_REJECT.
 */
class LinkQualityHysteresisTestCase : public NhdpTestCase
{
  public:
    LinkQualityHysteresisTestCase();

  protected:
    void DoRun() override;
};

LinkQualityHysteresisTestCase::LinkQualityHysteresisTestCase()
    : NhdpTestCase("Link quality accept and reject hysteresis")
{
}

void
LinkQualityHysteresisTestCase::DoRun()
{
    NS_LOG_DEBUG("Two-node link, quality on node 1: pending below accept, usable above, "
                 "with hysteresis between the thresholds");

    const Time helloInterval = Seconds(1);
    const Time validity = Seconds(3); // advertised H_HOLD_TIME
    CreateNodes(2, helloInterval, validity);
    Link(1, 2);

    // Enable link quality on node 1 only; node 2 is a normal peer (not pending, no quality
    // callback) that always advertises node 1.
    Client(1)->SetAttribute("InitialPending", BooleanValue(true));
    Client(1)->SetAttribute("HystAccept", DoubleValue(0.8));
    Client(1)->SetAttribute("HystReject", DoubleValue(0.3));
    EnableQuality(1);

    // Start between the thresholds: node 1's link to node 2 is created pending and stays
    // pending (not usable, neighbor never symmetric).
    SetQuality(0.5);
    Simulator::Schedule(Seconds(4),
                        &NhdpTestCase::CheckLinkStatus,
                        this,
                        1,
                        2,
                        LinkStatus::PENDING);
    Simulator::Schedule(Seconds(4), &NhdpTestCase::CheckSymmetric, this, 1, 2, false);

    // Raise quality to at least HYST_ACCEPT: the link becomes usable and reaches SYMMETRIC.
    Simulator::Schedule(Seconds(4.5), &NhdpTestCase::SetQuality, this, 0.9);
    Simulator::Schedule(Seconds(7),
                        &NhdpTestCase::CheckLinkStatus,
                        this,
                        1,
                        2,
                        LinkStatus::SYMMETRIC);
    Simulator::Schedule(Seconds(7), &NhdpTestCase::CheckSymmetric, this, 1, 2, true);

    // Lower quality back between the thresholds: hysteresis keeps the link usable.
    Simulator::Schedule(Seconds(7.5), &NhdpTestCase::SetQuality, this, 0.5);
    Simulator::Schedule(Seconds(10),
                        &NhdpTestCase::CheckLinkStatus,
                        this,
                        1,
                        2,
                        LinkStatus::SYMMETRIC);
    Simulator::Schedule(Seconds(10), &NhdpTestCase::CheckSymmetric, this, 1, 2, true);

    // Drop quality below HYST_REJECT: the link is rejected and returns to a non-usable
    // (pending) state.
    Simulator::Schedule(Seconds(10.5), &NhdpTestCase::SetQuality, this, 0.2);
    Simulator::Schedule(Seconds(13),
                        &NhdpTestCase::CheckLinkStatus,
                        this,
                        1,
                        2,
                        LinkStatus::PENDING);

    Simulator::Stop(Seconds(15));
    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup nhdp-tests
 *
 * @brief A reported link-layer failure immediately tears down the link and its 2-hop neighbor.
 *
 * In a three-node chain, node 1 reports a link-layer failure for its link to node 2.  Per
 * RFC 6130 Sec. 13.2 / 14.3 this takes effect immediately (before any timer would expire):
 * the link becomes non-usable, the neighbor is de-symmetrized, node 2's 2-Hop Tuples are
 * removed, node 2 is recorded in the Lost Neighbor Set, and the failure is reported on the
 * LinkFailure trace.
 */
class LinkFailureTestCase : public NhdpTestCase
{
  public:
    LinkFailureTestCase();

  protected:
    void DoRun() override;
};

LinkFailureTestCase::LinkFailureTestCase()
    : NhdpTestCase("Reported link-layer failure teardown")
{
}

void
LinkFailureTestCase::DoRun()
{
    NS_LOG_DEBUG("Three-node chain; report a link failure on node 1 for node 2 and check teardown");

    const Time helloInterval = Seconds(1);
    const Time validity = Seconds(3); // advertised H_HOLD_TIME
    CreateNodes(3, helloInterval, validity);
    Link(1, 2);
    Link(2, 3);
    ConnectLinkFailure(1);

    // By t = 4 s node 1 is symmetric with node 2 and has node 3 as a 2-hop neighbor via node 2.
    Simulator::Schedule(Seconds(4),
                        &NhdpTestCase::CheckLinkStatus,
                        this,
                        1,
                        2,
                        LinkStatus::SYMMETRIC);
    Simulator::Schedule(Seconds(4), &NhdpTestCase::CheckSymmetric, this, 1, 2, true);
    Simulator::Schedule(Seconds(4), &NhdpTestCase::CheckTwoHop, this, 1, 2, 3, true);

    // Report a link-layer failure for the link to node 2 between HELLO rounds; it takes
    // effect immediately.
    Simulator::Schedule(Seconds(4.5), &NhdpTestCase::TriggerLinkFailure, this, 1, 2);

    // Immediately afterward (before the t = 5 s HELLO): the link is non-usable (pending), the
    // neighbor is de-symmetrized, the 2-Hop Tuple via node 2 is gone, node 2 is in the Lost
    // Neighbor Set, and the failure was reported on the trace.
    Simulator::Schedule(Seconds(4.75), &NhdpTestCase::CheckLinkFailureReported, this, 2, true);
    Simulator::Schedule(Seconds(4.75),
                        &NhdpTestCase::CheckLinkStatus,
                        this,
                        1,
                        2,
                        LinkStatus::PENDING);
    Simulator::Schedule(Seconds(4.75), &NhdpTestCase::CheckSymmetric, this, 1, 2, false);
    Simulator::Schedule(Seconds(4.75), &NhdpTestCase::CheckTwoHopCount, this, 1, 0);
    Simulator::Schedule(Seconds(4.75), &NhdpTestCase::CheckTwoHop, this, 1, 2, 3, false);
    Simulator::Schedule(Seconds(4.75), &NhdpTestCase::CheckLostNeighbor, this, 1, 2, true);

    Simulator::Stop(Seconds(6));
    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup nhdp-tests
 *
 * @brief Each node's link timers derive from the peer's advertised validity time.
 *
 * Two nodes advertise different H_HOLD_TIME values.  Per RFC 6130 Sec. 12.2/12.5, a node's
 * L_SYM_time and L_HEARD_time for a neighbor are computed from that neighbor's advertised
 * validity time (carried in its VALIDITY_TIME TLV), while the L_time extension uses the
 * receiver's own local L_HOLD_TIME.
 */
class HeterogeneousValidityTestCase : public NhdpTestCase
{
  public:
    HeterogeneousValidityTestCase();

  protected:
    void DoRun() override;
};

HeterogeneousValidityTestCase::HeterogeneousValidityTestCase()
    : NhdpTestCase("Heterogeneous validity times drive per-peer link timers")
{
}

void
HeterogeneousValidityTestCase::DoRun()
{
    NS_LOG_DEBUG("Two nodes advertising different validity times; each uses the peer's value");

    const Time helloInterval = Seconds(1);
    const Time validityA = Seconds(6); // node 1's advertised H_HOLD_TIME
    const Time validityB = Seconds(3); // node 2's advertised H_HOLD_TIME
    const Time lHoldTime = Seconds(6); // model default LHoldTime (both nodes)
    CreateNodes(2, helloInterval, validityA);
    Client(2)->SetAttribute("HHoldTime", TimeValue(validityB));
    Link(1, 2);

    // Both links are symmetric from t = 2 s.  Node 1's link to node 2 uses node 2's advertised
    // validity (validityB = 3 s) for L_SYM_time/L_HEARD_time; node 2's link to node 1 uses
    // node 1's (validityA = 6 s).  The L_time extension uses each receiver's own L_HOLD_TIME.
    Simulator::Schedule(Seconds(2.5), &NhdpTestCase::CheckSymmetric, this, 1, 2, true);
    Simulator::Schedule(Seconds(2.5), &NhdpTestCase::CheckSymmetric, this, 2, 1, true);
    Simulator::Schedule(Seconds(2.5),
                        &NhdpTestCase::CheckLinkTimers,
                        this,
                        1,
                        2,
                        Seconds(2) + validityB,              // L_HEARD_time = 5 s
                        Seconds(2) + validityB,              // L_SYM_time = 5 s
                        Seconds(2) + validityB + lHoldTime); // L_time = 11 s
    Simulator::Schedule(Seconds(2.5),
                        &NhdpTestCase::CheckLinkTimers,
                        this,
                        2,
                        1,
                        Seconds(2) + validityA,              // L_HEARD_time = 8 s
                        Seconds(2) + validityA,              // L_SYM_time = 8 s
                        Seconds(2) + validityA + lHoldTime); // L_time = 14 s

    Simulator::Stop(Seconds(4));
    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup nhdp-tests
 *
 * @brief Test suite for the nhdp module.
 */
class NhdpTestSuite : public TestSuite
{
  public:
    NhdpTestSuite();
};

NhdpTestSuite::NhdpTestSuite()
    : TestSuite("nhdp", Type::UNIT)
{
    AddTestCase(new TimeEncodeDecodeTestCase(), TestCase::Duration::QUICK);
    AddTestCase(new SymmetricLinkTestCase(), TestCase::Duration::QUICK);
    AddTestCase(new LinkTimeoutTestCase(), TestCase::Duration::QUICK);
    AddTestCase(new TwoHopFormationTestCase(), TestCase::Duration::QUICK);
    AddTestCase(new TwoHopTimeoutCleanupTestCase(), TestCase::Duration::QUICK);
    AddTestCase(new LinkQualityHysteresisTestCase(), TestCase::Duration::QUICK);
    AddTestCase(new LinkFailureTestCase(), TestCase::Duration::QUICK);
    AddTestCase(new HeterogeneousValidityTestCase(), TestCase::Duration::QUICK);
}

/**
 * @ingroup nhdp-tests
 * Static variable for test initialization.
 */
static NhdpTestSuite g_nhdpTestSuite;
