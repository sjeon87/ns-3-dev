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
//   9. Ipv6SymmetricMultiAddress Two IPv6 nodes each advertise a link-local locator and a
//                            routing ULA (RFC 4193); the peer is tracked as a
//                            single Neighbor/Link Tuple carrying both addresses, a lookup by
//                            either address resolves to that one Link Tuple (overlap), and
//                            the link reaches SYMMETRIC over IPv6.
//  10. Ipv6TwoHopFormation   IPv6 three-node chain; node 1 learns node 3 (at both of its
//                            addresses) as a symmetric 2-hop neighbor via node 2.
//  11. NeighborMerge         A neighbor advertised at address A, then B, then A+B causes the
//                            two resulting Neighbor Tuples to be merged into one carrying both
//                            addresses (RFC 6130 Sec. 12.3 step 4).  Synthetic HELLOs are
//                            injected via BypassRecv.
//

#include "ns3/address.h"
#include "ns3/application-container.h"
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/ipv6-address-helper.h"
#include "ns3/ipv6-address.h"
#include "ns3/ipv6-interface-address.h"
#include "ns3/ipv6-interface-container.h"
#include "ns3/ipv6.h"
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
 * Builds a set of single-interface IPv4 nodes (addresses 7.0.0.1, 7.0.0.2, ...), each
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

    /**
     * Create count IPv6 nodes, each with an auto-configured link-local address plus a
     * Unique Local Address (fd00::1, fd00::2, ...) and an NhdpClient in IPv6 BypassMode.  Models
     * the scenario where neighbors are tracked at two addresses (the link-local locator and the
     * routing ULA).  Only the top 8 bits (fd00::/8) follow the RFC 4193 ULA allocation; for
     * trace simplicity a pseudo-randomly derived 40-bit Global ID (RFC 4193 Sec. 3.2.1) is
     * not used.
     *
     * @param count Number of nodes to create.
     * @param helloInterval HelloInterval attribute for each NhdpClient.
     * @param holdTime HHoldTime (advertised validity) attribute for each NhdpClient.
     */
    void CreateNodesIpv6(uint32_t count, Time helloInterval, Time holdTime);

    /**
     * Get the NhdpClient for a logical node id.
     * @param id The logical node id (1-based).
     * @return The node's NhdpClient.
     */
    Ptr<NhdpClient> Client(uint32_t id) const;
    /**
     * Get the routing MANET address of a logical node id.
     * @param id The logical node id (1-based).
     * @return The node's routing MANET address.
     */
    Address Addr(uint32_t id) const;

    /**
     * Get the link-local address of an IPv6 node's MANET interface.
     * @param id The node id.
     * @return The link-local address, or an invalid Address if none.
     */
    Address LinkLocal6(uint32_t id) const;

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
     * Assert the number of addresses in node id's Neighbor Tuple to neighborId.
     * @param id The observing node id.
     * @param neighborId The neighbor node id.
     * @param count The expected size of N_neighbor_addr_list.
     */
    void CheckNeighborAddrCount(uint32_t id, uint32_t neighborId, std::size_t count);
    /**
     * Assert that two addresses resolve to the same (single) Link Tuple at node id, i.e.
     * a neighbor advertising both addresses is tracked as one link (RFC 6130 overlap).
     * @param id The observing node id.
     * @param a An address of the neighbor.
     * @param b Another address of the same neighbor.
     */
    void CheckSameLink(uint32_t id, const Address& a, const Address& b);

    /**
     * Report a link-layer failure on node id for its link to node neighborId.
     * @param id The node id experiencing the failure.
     * @param neighborId The neighbor node id whose link failed.
     */
    void TriggerLinkFailure(uint32_t id, uint32_t neighborId) const;
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

    /**
     * Inject a synthetic HELLO (LOCAL_IF block only) directly into node id via its BypassRecv
     * entry point, advertising the given addresses as THIS_IF.  Lets a test present a neighbor
     * whose advertised address set varies between HELLOs, which the running clients cannot do.
     * @param toId The receiving node id.
     * @param thisIf The addresses to advertise as the neighbor's THIS_IF (Sending Address List).
     * @param validity The advertised validity time.
     * @param ipv6 True to build an IPv6 HELLO, false for IPv4.
     */
    void InjectLocalIfHello(uint32_t toId,
                            std::vector<Address> thisIf,
                            Time validity,
                            bool ipv6) const;
    /**
     * Assert the number of Neighbor Tuples at node id.
     * @param id The observing node id.
     * @param count The expected Neighbor Set size.
     */
    void CheckNeighborSetSize(uint32_t id, std::size_t count);
    /**
     * Assert that two addresses resolve to a single merged Neighbor Tuple carrying both.
     * @param id The observing node id.
     * @param a One advertised neighbor address.
     * @param b Another advertised neighbor address.
     */
    void CheckMergedNeighbor(uint32_t id, Address a, Address b);

  protected:
    void DoSetup() override;
    void DoTeardown() override;

  private:
    /**
     * Map a logical node id to its ns-3 NodeList id.
     * @param id The logical node id (1-based).
     * @return The corresponding ns-3 NodeList id.
     */
    uint32_t NodeId(uint32_t id) const;
    /**
     * Shared link-quality callback returning a fixed quality for received HELLOs.
     * @param packet The received HELLO packet.
     * @return The link quality to report.
     */
    double QualityCallback(Ptr<Packet> packet) const;
    /**
     * LinkFailure trace sink that records the failed peer address.
     * @param addr The address of the peer whose link failed.
     */
    void RecordLinkFailure(const Address& addr);

    NodeContainer m_nodes;                  //!< The created nodes (index id-1)
    std::vector<Ptr<NhdpClient>> m_clients; //!< NhdpClient per node (index id-1)
    std::vector<Address> m_addrs;           //!< Routing MANET address per node (index id-1)
    double m_quality{1.0};                  //!< Value returned by QualityCallback()
    std::set<Address> m_linkFailures;       //!< Addresses reported on the LinkFailure trace
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
        m_addrs.push_back(Address(ifaces.GetAddress(i)));
    }
    apps.Start(Seconds(0));
}

void
NhdpTestCase::CreateNodesIpv6(uint32_t count, Time helloInterval, Time holdTime)
{
    NS_LOG_DEBUG("Creating " << count << " IPv6 NHDP nodes; helloInterval "
                             << helloInterval.As(Time::S) << ", hHoldTime "
                             << holdTime.As(Time::S));

    // Use a fixed RNG seed and run for reproducibility (DoSetup() saved the previous
    // values and DoTeardown() restores them).
    Config::SetGlobal("RngSeed", UintegerValue(1));
    Config::SetGlobal("RngRun", UintegerValue(1));

    m_nodes.Create(count);

    InternetStackHelper internet;
    internet.Install(m_nodes);

    SimpleNetDeviceHelper simpleNet;
    NetDeviceContainer devices = simpleNet.Install(m_nodes);

    // Assign a routing Unique Local Address (fd00::1, fd00::2, ...) to each node; the IPv6
    // stack auto-configures a link-local address as well.  Only the top 8 bits (fd00::/8)
    // follow the RFC 4193 ULA allocation; for trace simplicity these addresses do not use a
    // pseudo-randomly derived 40-bit Global ID as RFC 4193 Sec. 3.2.1 would require.
    Ipv6AddressHelper ipv6;
    ipv6.SetBase(Ipv6Address("fd00::"), Ipv6Prefix(64));
    Ipv6InterfaceContainer ifaces = ipv6.Assign(devices);

    NhdpHelper nhdp;
    nhdp.SetAttribute("BypassMode", BooleanValue(true));
    nhdp.SetAttribute("AddressMode", EnumValue(AddressMode::IPV6));
    nhdp.SetAttribute("HelloInterval", TimeValue(helloInterval));
    nhdp.SetAttribute("RefreshInterval", TimeValue(helloInterval));
    nhdp.SetAttribute("HHoldTime", TimeValue(holdTime));
    nhdp.SetAttribute("HPMaxJitter", TimeValue(Seconds(0)));
    ApplicationContainer apps = nhdp.Install(m_nodes);

    for (uint32_t i = 0; i < count; i++)
    {
        auto client = apps.Get(i)->GetObject<NhdpClient>();
        client->AssignStreams(100 + i);
        m_clients.push_back(client);
        // The routing address is the ULA at interface index 1, address index 1 (index 0 is the
        // link-local).  GetAddress(i, 1) returns that ULA.
        m_addrs.push_back(Address(ifaces.GetAddress(i, 1)));
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

Address
NhdpTestCase::Addr(uint32_t id) const
{
    return m_addrs.at(id - 1); // logical node ids run from 1
}

Address
NhdpTestCase::LinkLocal6(uint32_t id) const
{
    Ptr<Ipv6> ipv6 = Client(id)->GetNode()->GetObject<Ipv6>();
    NS_ASSERT(ipv6);
    for (uint32_t i = 0; i < ipv6->GetNInterfaces(); i++)
    {
        for (uint32_t j = 0; j < ipv6->GetNAddresses(i); j++)
        {
            Ipv6InterfaceAddress ia = ipv6->GetAddress(i, j);
            if (ia.GetScope() == Ipv6InterfaceAddress::LINKLOCAL)
            {
                return Address(ia.GetAddress());
            }
        }
    }
    return Address();
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
    const LinkTuple* link = Client(id)->FindLinkTuple(Addr(neighborId));
    NS_TEST_ASSERT_MSG_NE(link,
                          nullptr,
                          "Node " << id << " missing Link Tuple to " << neighborId << " at "
                                  << Simulator::Now().As(Time::S));
    NS_TEST_ASSERT_MSG_EQ(link->GetLinkStatus(),
                          status,
                          "Node " << id << " Link Tuple to " << neighborId << " wrong status at "
                                  << Simulator::Now().As(Time::S));
}

void
NhdpTestCase::CheckNoLink(uint32_t id, uint32_t neighborId)
{
    NS_LOG_DEBUG("Checking node " << id << " has no link to " << neighborId << " at "
                                  << Simulator::Now().As(Time::S));
    NS_TEST_ASSERT_MSG_EQ((Client(id)->FindLinkTuple(Addr(neighborId)) == nullptr),
                          true,
                          "Node " << id << " unexpectedly has a Link Tuple to " << neighborId
                                  << " at " << Simulator::Now().As(Time::S));
}

void
NhdpTestCase::CheckSymmetric(uint32_t id, uint32_t neighborId, bool symmetric)
{
    NS_LOG_DEBUG("Checking node " << id << " neighbor " << neighborId << " symmetric=" << symmetric
                                  << " at " << Simulator::Now().As(Time::S));
    const NeighborTuple* neighbor = Client(id)->FindNeighborTuple(Addr(neighborId));
    NS_TEST_ASSERT_MSG_NE(neighbor,
                          nullptr,
                          "Node " << id << " missing Neighbor Tuple to " << neighborId << " at "
                                  << Simulator::Now().As(Time::S));
    NS_TEST_ASSERT_MSG_EQ(neighbor->m_symmetric,
                          symmetric,
                          "Node " << id << " Neighbor Tuple to " << neighborId
                                  << " wrong symmetry at " << Simulator::Now().As(Time::S));
}

void
NhdpTestCase::CheckNoNeighbor(uint32_t id, uint32_t neighborId)
{
    NS_LOG_DEBUG("Checking node " << id << " has no neighbor " << neighborId << " at "
                                  << Simulator::Now().As(Time::S));
    NS_TEST_ASSERT_MSG_EQ((Client(id)->FindNeighborTuple(Addr(neighborId)) == nullptr),
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
    bool found = Client(id)->FindTwoHopTuple(Addr(viaId), Addr(twoHopId)) != nullptr;
    NS_TEST_ASSERT_MSG_EQ(found,
                          present,
                          "Node " << id << " 2-Hop Tuple to " << twoHopId << " via " << viaId
                                  << " presence mismatch at " << Simulator::Now().As(Time::S));
}

void
NhdpTestCase::CheckLostNeighbor(uint32_t id, uint32_t neighborId, bool present)
{
    NS_LOG_DEBUG("Checking node " << id << " lost-neighbor " << neighborId << " present=" << present
                                  << " at " << Simulator::Now().As(Time::S));
    bool found = Client(id)->FindLostNeighbor(Addr(neighborId)) != nullptr;
    NS_TEST_ASSERT_MSG_EQ(found,
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
    const LinkTuple* link = Client(id)->FindLinkTuple(Addr(neighborId));
    NS_TEST_ASSERT_MSG_NE(link,
                          nullptr,
                          "Node " << id << " missing Link Tuple to " << neighborId << " at "
                                  << Simulator::Now().As(Time::S));
    NS_TEST_ASSERT_MSG_EQ_TOL(link->m_heardTime,
                              heard,
                              m_timerTolerance,
                              "Node " << id << " L_HEARD_time to " << neighborId << " wrong");
    NS_TEST_ASSERT_MSG_EQ_TOL(link->m_symTime,
                              sym,
                              m_timerTolerance,
                              "Node " << id << " L_SYM_time to " << neighborId << " wrong");
    NS_TEST_ASSERT_MSG_EQ_TOL(link->m_expirationTime,
                              expiration,
                              m_timerTolerance,
                              "Node " << id << " L_time to " << neighborId << " wrong");
}

void
NhdpTestCase::TriggerLinkFailure(uint32_t id, uint32_t neighborId) const
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
NhdpTestCase::RecordLinkFailure(const Address& addr)
{
    NS_LOG_DEBUG("LinkFailure trace fired for " << addr);
    m_linkFailures.insert(addr);
}

void
NhdpTestCase::CheckNeighborAddrCount(uint32_t id, uint32_t neighborId, std::size_t count)
{
    NS_LOG_DEBUG("Checking node " << id << " neighbor " << neighborId << " address count == "
                                  << count << " at " << Simulator::Now().As(Time::S));
    const NeighborTuple* neighbor = Client(id)->FindNeighborTuple(Addr(neighborId));
    NS_TEST_ASSERT_MSG_NE(neighbor,
                          nullptr,
                          "Node " << id << " missing Neighbor Tuple to " << neighborId << " at "
                                  << Simulator::Now().As(Time::S));
    NS_TEST_ASSERT_MSG_EQ(neighbor->m_neighborAddrList.size(),
                          count,
                          "Node " << id << " Neighbor Tuple to " << neighborId
                                  << " wrong address count at " << Simulator::Now().As(Time::S));
}

void
NhdpTestCase::CheckSameLink(uint32_t id, const Address& a, const Address& b)
{
    NS_LOG_DEBUG("Checking node " << id << " resolves " << a << " and " << b
                                  << " to the same Link Tuple at " << Simulator::Now().As(Time::S));
    const LinkTuple* linkA = Client(id)->FindLinkTuple(a);
    const LinkTuple* linkB = Client(id)->FindLinkTuple(b);
    NS_TEST_ASSERT_MSG_NE(linkA, nullptr, "Node " << id << " missing Link Tuple for first address");
    NS_TEST_ASSERT_MSG_EQ(linkA,
                          linkB,
                          "Node " << id << " addresses resolve to different Link Tuples at "
                                  << Simulator::Now().As(Time::S));
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

void
NhdpTestCase::InjectLocalIfHello(uint32_t toId,
                                 std::vector<Address> thisIf,
                                 Time validity,
                                 bool ipv6) const
{
    NS_LOG_DEBUG("Injecting synthetic HELLO into node " << toId << " advertising " << thisIf.size()
                                                        << " THIS_IF address(es) at "
                                                        << Simulator::Now().As(Time::S));
    PbbPacket pbb;
    Ptr<PbbMessage> msg;
    if (ipv6)
    {
        msg = Create<PbbMessageIpv6>();
    }
    else
    {
        msg = Create<PbbMessageIpv4>();
    }
    msg->SetType(MESSAGE_TYPE_HELLO);
    pbb.MessagePushBack(msg);

    // Mandatory VALIDITY_TIME Message TLV (RFC 6130 Sec. 12.1).
    Ptr<PbbTlv> validityTlv = Create<PbbTlv>();
    validityTlv->SetType(MSG_TLV_VALIDITY_TIME);
    uint8_t code = EncodeTimeCode(validity);
    validityTlv->SetValue(&code, 1);
    msg->TlvPushBack(validityTlv);

    // LOCAL_IF address block listing the advertised addresses as THIS_IF.
    Ptr<PbbAddressBlock> block;
    if (ipv6)
    {
        block = Create<PbbAddressBlockIpv6>();
    }
    else
    {
        block = Create<PbbAddressBlockIpv4>();
    }
    for (const auto& addr : thisIf)
    {
        block->AddressPushBack(addr);
        block->PrefixPushBack(ipv6 ? 128 : 32);
    }
    Ptr<PbbAddressTlv> addrTlv = Create<PbbAddressTlv>();
    addrTlv->SetType(ADDR_TLV_LOCAL_IF);
    addrTlv->SetValue(&ADDR_TLV_LOCAL_IF_THIS_IF, sizeof(ADDR_TLV_LOCAL_IF_THIS_IF));
    addrTlv->SetIndexStart(0);
    if (thisIf.size() > 1)
    {
        addrTlv->SetIndexStop(thisIf.size() - 1);
    }
    block->TlvPushBack(addrTlv);
    msg->AddressBlockPushBack(block);

    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(pbb);
    Client(toId)->BypassRecv(packet);
}

void
NhdpTestCase::CheckNeighborSetSize(uint32_t id, std::size_t count)
{
    NS_LOG_DEBUG("Checking node " << id << " Neighbor Set size == " << count << " at "
                                  << Simulator::Now().As(Time::S));
    NS_TEST_ASSERT_MSG_EQ(Client(id)->GetNeighborInfoBase().size(),
                          count,
                          "Node " << id << " wrong Neighbor Set size at "
                                  << Simulator::Now().As(Time::S));
}

void
NhdpTestCase::CheckMergedNeighbor(uint32_t id, Address a, Address b)
{
    NS_LOG_DEBUG("Checking node " << id << " merged neighbor at " << Simulator::Now().As(Time::S));
    const NeighborTuple* ta = Client(id)->FindNeighborTuple(a);
    const NeighborTuple* tb = Client(id)->FindNeighborTuple(b);
    NS_TEST_ASSERT_MSG_NE(ta,
                          nullptr,
                          "Node " << id << " missing Neighbor Tuple for first address");
    NS_TEST_ASSERT_MSG_EQ(ta,
                          tb,
                          "Node " << id << " addresses resolve to different Neighbor Tuples at "
                                  << Simulator::Now().As(Time::S));
    NS_TEST_ASSERT_MSG_EQ(ta->m_neighborAddrList.size(),
                          static_cast<std::size_t>(2),
                          "Node " << id << " merged Neighbor Tuple should carry both addresses");
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
 * @brief Two IPv6 nodes track each other at two addresses (link-local + ULA).
 *
 * Models the scenario where each node advertises both its link-local locator and its
 * routing Unique Local Address (RFC 4193) as THIS_IF.  Verifies that the peer is
 * tracked as a single Neighbor/Link Tuple carrying both addresses (RFC 6130 multi-address
 * handling), that a lookup by either address resolves to the same Link Tuple (overlap), and
 * that the link reaches SYMMETRIC over IPv6.
 */
class Ipv6SymmetricMultiAddressTestCase : public NhdpTestCase
{
  public:
    Ipv6SymmetricMultiAddressTestCase();

  protected:
    void DoRun() override;
};

Ipv6SymmetricMultiAddressTestCase::Ipv6SymmetricMultiAddressTestCase()
    : NhdpTestCase("IPv6 two-node symmetric link with link-local plus ULA")
{
}

void
Ipv6SymmetricMultiAddressTestCase::DoRun()
{
    NS_LOG_DEBUG("Two IPv6 nodes: expect a single SYMMETRIC link carrying both peer addresses");

    const Time helloInterval = Seconds(1);
    const Time validity = Seconds(3); // advertised H_HOLD_TIME
    CreateNodesIpv6(2, helloInterval, validity);
    Link(1, 2);

    // By t = 2 s each node hears itself listed and the link becomes SYMMETRIC.
    Simulator::Schedule(Seconds(2.5),
                        &NhdpTestCase::CheckLinkStatus,
                        this,
                        1,
                        2,
                        LinkStatus::SYMMETRIC);
    Simulator::Schedule(Seconds(2.5), &NhdpTestCase::CheckSymmetric, this, 1, 2, true);
    // The neighbor is tracked at both of its addresses (link-local + ULA).
    Simulator::Schedule(Seconds(2.5), &NhdpTestCase::CheckNeighborAddrCount, this, 1, 2, 2);
    // A lookup by the link-local or the ULA resolves to the same Link Tuple (overlap matching).
    Simulator::Schedule(Seconds(2.6), [this]() { CheckSameLink(1, Addr(2), LinkLocal6(2)); });

    Simulator::Stop(Seconds(3));
    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup nhdp-tests
 *
 * @brief IPv6 three-node chain 2-hop neighbor formation with two addresses per node.
 *
 * As TwoHopFormation but over IPv6: node 2 advertises node 3 (both its link-local and ULA)
 * as a symmetric 1-hop neighbor, so node 1 learns node 3 as a symmetric 2-hop neighbor at
 * both of node 3's addresses via node 2 (RFC 6130 Sec. 12.6 with list-valued addresses).
 */
class Ipv6TwoHopFormationTestCase : public NhdpTestCase
{
  public:
    Ipv6TwoHopFormationTestCase();

  protected:
    void DoRun() override;
};

Ipv6TwoHopFormationTestCase::Ipv6TwoHopFormationTestCase()
    : NhdpTestCase("IPv6 three-node chain 2-hop neighbor formation")
{
}

void
Ipv6TwoHopFormationTestCase::DoRun()
{
    NS_LOG_DEBUG("IPv6 chain 1-2-3: node 1 learns node 3 (two addresses) as a 2-hop via node 2");

    const Time helloInterval = Seconds(1);
    const Time validity = Seconds(3); // advertised H_HOLD_TIME
    CreateNodesIpv6(3, helloInterval, validity);
    Link(1, 2);
    Link(2, 3);

    Simulator::Schedule(Seconds(2.5),
                        &NhdpTestCase::CheckLinkStatus,
                        this,
                        1,
                        2,
                        LinkStatus::SYMMETRIC);
    Simulator::Schedule(Seconds(2.5), &NhdpTestCase::CheckNoLink, this, 1, 3);

    // At t = 3 s node 2 advertises node 3 as SYMMETRIC at both of its addresses; node 1 creates
    // a 2-Hop Tuple to each of node 3's addresses via node 2 (two tuples), and none to itself.
    Simulator::Schedule(Seconds(3.5), &NhdpTestCase::CheckTwoHop, this, 1, 2, 3, true);
    Simulator::Schedule(Seconds(3.5), &NhdpTestCase::CheckTwoHopCount, this, 1, 2);
    Simulator::Schedule(Seconds(3.5), &NhdpTestCase::CheckTwoHop, this, 1, 2, 1, false);

    Simulator::Stop(Seconds(5));
    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup nhdp-tests
 *
 * @brief A neighbor advertising overlapping address sets triggers a Neighbor Set merge.
 *
 * Exercises RFC 6130 Sec. 12.3 step 4: a peer first advertises address A alone, then address
 * B alone (forming two separate Neighbor Tuples at the receiver), then both A and B together.
 * The combined HELLO overlaps both Tuples, which must be merged into a single Neighbor Tuple
 * carrying both addresses.  Synthetic HELLOs are injected via BypassRecv because a running
 * NhdpClient always advertises its full local address set and so cannot itself drive this case.
 */
class NeighborMergeTestCase : public NhdpTestCase
{
  public:
    NeighborMergeTestCase();

  protected:
    void DoRun() override;
};

NeighborMergeTestCase::NeighborMergeTestCase()
    : NhdpTestCase("Neighbor Set merge on overlapping address sets (Sec. 12.3 step 4)")
{
}

void
NeighborMergeTestCase::DoRun()
{
    NS_LOG_DEBUG("Inject HELLOs advertising A, then B, then A+B; expect a single merged neighbor");

    const Time helloInterval = Seconds(1);
    // Long validity so the injected Link/Neighbor Tuples persist for the duration of the test.
    const Time validity = Seconds(10);
    CreateNodes(1, helloInterval, validity);
    const Address a = Address(Ipv4Address("7.0.0.100"));
    const Address b = Address(Ipv4Address("7.0.0.200"));

    // The neighbor advertises only A, then only B: B does not overlap A's Neighbor Tuple, so
    // node 1 ends up with two separate Neighbor Tuples.
    Simulator::Schedule(Seconds(1.5),
                        &NhdpTestCase::InjectLocalIfHello,
                        this,
                        1,
                        std::vector<Address>{a},
                        validity,
                        false);
    Simulator::Schedule(Seconds(2.5),
                        &NhdpTestCase::InjectLocalIfHello,
                        this,
                        1,
                        std::vector<Address>{b},
                        validity,
                        false);
    Simulator::Schedule(Seconds(2.75), &NhdpTestCase::CheckNeighborSetSize, this, 1, 2);

    // The neighbor then advertises both addresses; the HELLO overlaps both Neighbor Tuples,
    // which are merged into one tuple carrying both A and B (RFC 6130 Sec. 12.3 step 4).
    Simulator::Schedule(Seconds(3.5),
                        &NhdpTestCase::InjectLocalIfHello,
                        this,
                        1,
                        std::vector<Address>{a, b},
                        validity,
                        false);
    Simulator::Schedule(Seconds(3.75), &NhdpTestCase::CheckNeighborSetSize, this, 1, 1);
    Simulator::Schedule(Seconds(3.75), &NhdpTestCase::CheckMergedNeighbor, this, 1, a, b);

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
    AddTestCase(new Ipv6SymmetricMultiAddressTestCase(), TestCase::Duration::QUICK);
    AddTestCase(new Ipv6TwoHopFormationTestCase(), TestCase::Duration::QUICK);
    AddTestCase(new NeighborMergeTestCase(), TestCase::Duration::QUICK);
}

/**
 * @ingroup nhdp-tests
 * Static variable for test initialization.
 */
static NhdpTestSuite g_nhdpTestSuite;
