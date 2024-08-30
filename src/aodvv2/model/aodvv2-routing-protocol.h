/*
 * Copyright (c) 2009 IITP RAS
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Based on
 *      NS-2 AODV model developed by the CMU/MONARCH group and optimized and
 *      tuned by Samir Das and Mahesh Marina, University of Cincinnati;
 *
 *      AODV-UU implementation by Erik Nordström of Uppsala University
 *      https://web.archive.org/web/20100527072022/http://core.it.uu.se/core/index.php/AODV-UU
 *
 * Authors: Elena Buchatskaia <borovkovaes@iitp.ru>
 *          Pavel Boyko <boyko@iitp.ru>
 */
#ifndef AODVV2_ROUTING_PROTOCOL_H
#define AODVV2_ROUTING_PROTOCOL_H

#include "aodvv2-dpd.h"
#include "aodvv2-neighbor.h"
#include "aodvv2-packet.h"
#include "aodvv2-rqueue.h"
#include "aodvv2-rtable.h"

#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv4-routing-helper.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv6-interface.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/ipv6-routing-helper.h"
#include "ns3/ipv6-routing-protocol.h"
#include "ns3/node.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/random-variable-stream.h"

#include <map>

namespace ns3
{

class WifiMpdu;
enum WifiMacDropReason : uint8_t; // opaque enum declaration

namespace aodvv2
{
/**
 * \ingroup aodvv2
 *
 * \brief AODV routing protocol
 */
template <typename T>
class Aodvv2RoutingProtocol : public std::enable_if_t<std::is_same_v<Ipv4RoutingProtocol, T> ||
                                                          std::is_same_v<Ipv6RoutingProtocol, T>,
                                                      T>
{
    /// Alias for determining whether the parent is Ipv4RoutingProtocol or Ipv6RoutingProtocol
    static constexpr bool IsIpv4 = std::is_same_v<Ipv4RoutingProtocol, T>;
    /// Alias for Ipv4 and Ipv6 classes
    using Ip = typename std::conditional_t<IsIpv4, Ipv4, Ipv6>;
    /// Alias for Ipv4Address and Ipv6Address classes
    using IpAddress = typename std::conditional_t<IsIpv4, Ipv4Address, Ipv6Address>;
    /// Alias for Ipv4Route and Ipv6Route classes
    using IpRoute = typename std::conditional_t<IsIpv4, Ipv4Route, Ipv6Route>;
    /// Alias for Ipv4AddressHash and Ipv6AddressHash classes
    using IpAddressHash = typename std::conditional_t<IsIpv4, Ipv4AddressHash, Ipv6AddressHash>;
    /// Alias for Ipv4Header and Ipv6Header classes
    using IpHeader = typename std::conditional_t<IsIpv4, Ipv4Header, Ipv6Header>;
    /// Alias for Ipv4InterfaceAddress and Ipv6InterfaceAddress classes
    using IpInterfaceAddress =
        typename std::conditional_t<IsIpv4, Ipv4InterfaceAddress, Ipv6InterfaceAddress>;
    /// Alias for Ipv4Interface and Ipv6Interface classes
    using IpInterface = typename std::conditional_t<IsIpv4, Ipv4Interface, Ipv6Interface>;
    /// Alias for Ipv4L3Protocol and Ipv6L3Protocol classes
    using IpL3Protocol = typename std::conditional_t<IsIpv4, Ipv4L3Protocol, Ipv6L3Protocol>;
    /// Alias for Ipv4RoutingProtocol and Ipv6RoutingProtocol classes
    using IpRoutingProtocol =
        typename std::conditional_t<IsIpv4, Ipv4RoutingProtocol, Ipv6RoutingProtocol>;
    using InetTSocketAddress =
        typename std::conditional_t<IsIpv4, InetSocketAddress, Inet6SocketAddress>;

    /// Callback for IPv4 unicast packets to be forwarded
    typedef Callback<void, Ptr<IpRoute>, Ptr<const Packet>, const IpHeader&>
        UnicastForwardCallbackv4;

    /// Callback for IPv6 unicast packets to be forwarded
    typedef Callback<void, Ptr<const NetDevice>, Ptr<IpRoute>, Ptr<const Packet>, const IpHeader&>
        UnicastForwardCallbackv6;

    /// Callback for unicast packets to be forwarded
    typedef typename std::conditional_t<IsIpv4, UnicastForwardCallbackv4, UnicastForwardCallbackv6>
        UnicastForwardCallback;

    /// Callback for IPv4 multicast packets to be forwarded
    typedef Callback<void, Ptr<Ipv4MulticastRoute>, Ptr<const Packet>, const IpHeader&>
        MulticastForwardCallbackv4;

    /// Callback for IPv6 multicast packets to be forwarded
    typedef Callback<void,
                     Ptr<const NetDevice>,
                     Ptr<Ipv6MulticastRoute>,
                     Ptr<const Packet>,
                     const IpHeader&>
        MulticastForwardCallbackv6;

    /// Callback for multicast packets to be forwarded
    typedef
        typename std::conditional_t<IsIpv4, MulticastForwardCallbackv4, MulticastForwardCallbackv6>
            MulticastForwardCallback;

    /// Callback for packets to be locally delivered
    typedef Callback<void, Ptr<const Packet>, const IpHeader&, uint32_t> LocalDeliverCallback;

    /// Callback for routing errors (e.g., no route found)
    typedef Callback<void, Ptr<const Packet>, const IpHeader&, Socket::SocketErrno> ErrorCallback;

  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    static const uint32_t AODV_PORT;

    /// constructor
    Aodvv2RoutingProtocol();
    ~Aodvv2RoutingProtocol();
    void DoDispose();

    // Inherited from IpRoutingProtocol
    virtual Ptr<IpRoute> RouteOutput(Ptr<Packet> p,
                                     const IpHeader& header,
                                     Ptr<NetDevice> oif,
                                     Socket::SocketErrno& sockerr);
    virtual bool RouteInput(Ptr<const Packet> p,
                            const IpHeader& header,
                            Ptr<const NetDevice> idev,
                            const UnicastForwardCallback& ucb,
                            const MulticastForwardCallback& mcb,
                            const LocalDeliverCallback& lcb,
                            const ErrorCallback& ecb);
    virtual void NotifyInterfaceUp(uint32_t interface);
    virtual void NotifyInterfaceDown(uint32_t interface);
    virtual void NotifyAddAddress(uint32_t interface, IpInterfaceAddress address);
    virtual void NotifyRemoveAddress(uint32_t interface, IpInterfaceAddress address);
    virtual void SetIpv4(Ptr<Ipv4> ipv4);
    virtual void SetIpv6(Ptr<Ipv6> ipv6);

    /**
     * \brief Notify a new route.
     *
     * \param dst destination address
     * \param mask destination mask
     * \param nextHop nextHop for this destination
     * \param interface output interface
     * \param prefixToUse prefix to use as source with this route
     *
     * \sa Ipv6RoutingProtocol::NotifyAddRoute
     */
    virtual void NotifyAddRoute(IpAddress dst,
                                Ipv6Prefix mask,
                                IpAddress nextHop,
                                uint32_t interface,
                                IpAddress prefixToUse = IpAddress::GetZero());

    /**
     * \brief Notify route removing.
     *
     * \param dst destination address
     * \param mask destination mask
     * \param nextHop nextHop for this destination
     * \param interface output interface
     * \param prefixToUse prefix to use as source with this route
     *
     * \sa Ipv6RoutingProtocol::NotifyRemoveRoute
     */
    virtual void NotifyRemoveRoute(IpAddress dst,
                                   Ipv6Prefix mask,
                                   IpAddress nextHop,
                                   uint32_t interface,
                                   IpAddress prefixToUse = IpAddress::GetZero());

    virtual void PrintRoutingTable(Ptr<OutputStreamWrapper> stream,
                                   Time::Unit unit = Time::S) const;

    // Handle protocol parameters
    /**
     * Get maximum queue time
     * \returns the maximum queue time
     */
    Time GetMaxQueueTime() const
    {
        return m_maxQueueTime;
    }

    /**
     * Set the maximum queue time
     * \param t the maximum queue time
     */
    void SetMaxQueueTime(Time t);

    /**
     * Get the maximum queue length
     * \returns the maximum queue length
     */
    uint32_t GetMaxQueueLen() const
    {
        return m_maxQueueLen;
    }

    /**
     * Set the maximum queue length
     * \param len the maximum queue length
     */
    void SetMaxQueueLen(uint32_t len);

    /**
     * Get destination only flag
     * \returns the destination only flag
     */
    bool GetDestinationOnlyFlag() const
    {
        return m_destinationOnly;
    }

    /**
     * Set destination only flag
     * \param f the destination only flag
     */
    void SetDestinationOnlyFlag(bool f)
    {
        m_destinationOnly = f;
    }

    /**
     * Set broadcast enable flag
     * \param f enable broadcast flag
     */
    void SetBroadcastEnable(bool f)
    {
        m_enableBroadcast = f;
    }

    /**
     * Get broadcast enable flag
     * \returns the broadcast enable flag
     */
    bool GetBroadcastEnable() const
    {
        return m_enableBroadcast;
    }

    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model.  Return the number of streams (possibly zero) that
     * have been assigned.
     *
     * \param stream first stream index to use
     * \return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream);

  protected:
    void DoInitialize();

  private:
    /**
     * Notify that an MPDU was dropped.
     *
     * \param reason the reason why the MPDU was dropped
     * \param mpdu the dropped MPDU
     */
    void NotifyTxError(WifiMacDropReason reason, Ptr<const WifiMpdu> mpdu);

    // Protocol parameters.
    uint32_t m_rreqRetries; ///< Maximum number of retransmissions of RREQ with TTL = NetDiameter to
                            ///< discover a route
    uint16_t m_ttlStart;    ///< Initial TTL value for RREQ.
    uint16_t m_ttlIncrement; ///< TTL increment for each attempt using the expanding ring search for
                             ///< RREQ dissemination.
    uint16_t m_ttlThreshold; ///< Maximum TTL value for expanding ring search, TTL = NetDiameter is
                             ///< used beyond this value.
    uint16_t m_timeoutBuffer;  ///< Provide a buffer for the timeout.
    uint16_t m_rreqRateLimit;  ///< Maximum number of RREQ per second.
    uint16_t m_rerrRateLimit;  ///< Maximum number of REER per second.
    Time m_activeRouteTimeout; ///< Period of time during which the route is considered to be valid.
    uint32_t m_netDiameter; ///< Net diameter measures the maximum possible number of hops between
                            ///< two nodes in the network
    /**
     * NodeTraversalTime is a conservative estimate of the average one hop traversal time for
     * packets and should include queuing delays, interrupt processing times and transfer times.
     */
    Time m_nodeTraversalTime;
    Time m_netTraversalTime;  ///< Estimate of the average net traversal time.
    Time m_pathDiscoveryTime; ///< Estimate of maximum time needed to find route in network.
    Time m_myRouteTimeout;    ///< Value of lifetime field in RREP generating by this node.
    /**
     * DeletePeriod is intended to provide an upper bound on the time for which an upstream node A
     * can have a neighbor B as an active next hop for destination D, while B has invalidated the
     * route to D.
     */
    Time m_deletePeriod;
    Time m_nextHopWait;      ///< Period of our waiting for the neighbour's RREP_ACK
    Time m_blackListTimeout; ///< Time for which the node is put into the blacklist
    uint32_t m_maxQueueLen;  ///< The maximum number of packets that we allow a routing protocol to
                             ///< buffer.
    Time m_maxQueueTime;     ///< The maximum period of time that a routing protocol is allowed to
                             ///< buffer a packet for.
    bool m_destinationOnly;  ///< Indicates only the destination may respond to this RREQ.
    bool m_gratuitousReply;  ///< Indicates whether a gratuitous RREP should be unicast to the node
                             ///< originated route discovery.
    bool m_enableBroadcast;  ///< Indicates whether a a broadcast data packets forwarding enable

    /// IP protocol
    Ptr<Ip> m_ip;
    /// Raw unicast socket per each IP interface, map socket -> iface address (IP + mask)
    std::map<Ptr<Socket>, IpInterfaceAddress> m_socketAddresses;
    /// Raw subnet directed broadcast socket per each IP interface, map socket -> iface address (IP
    /// + mask)
    std::map<Ptr<Socket>, IpInterfaceAddress> m_socketSubnetBroadcastAddresses;
    /// Loopback device used to defer RREQ until packet will be fully formed
    Ptr<NetDevice> m_lo;

    /// Routing table
    RoutingTable<IpAddress> m_routingTable;
    /// A "drop-front" queue used by the routing layer to buffer packets to which it does not have a
    /// route.
    RequestQueue<IpAddress> m_queue;
    /// Broadcast ID
    uint32_t m_requestId;
    /// Request sequence number
    uint32_t m_seqNo;
    /// Handle duplicated RREQ
    IdCache<IpAddress> m_rreqIdCache;
    /// Handle duplicated broadcast/multicast packets
    DuplicatePacketDetection<IpHeader> m_dpd;
    /// Handle neighbors
    Neighbors<IpAddress> m_nb;
    /// Number of RREQs used for RREQ rate control
    uint16_t m_rreqCount;
    /// Number of RERRs used for RERR rate control
    uint16_t m_rerrCount;

  private:
    /// Start protocol operation
    void Start();
    /**
     * Queue packet and send route request
     *
     * \param p the packet to route
     * \param header the IP header
     * \param ucb the UnicastForwardCallback function
     * \param ecb the ErrorCallback function
     */
    void DeferredRouteOutput(Ptr<const Packet> p,
                             const IpHeader& header,
                             UnicastForwardCallback ucb,
                             ErrorCallback ecb);
    /**
     * If route exists and is valid, forward packet.
     *
     * \param p the packet to route
     * \param header the IP header
     * \param ucb the UnicastForwardCallback function
     * \param ecb the ErrorCallback function
     * \returns true if forwarded
     */
    bool Forwarding(Ptr<const Packet> p,
                    const IpHeader& header,
                    UnicastForwardCallback ucb,
                    ErrorCallback ecb);
    /**
     * Repeated attempts by a source node at route discovery for a single destination
     * use the expanding ring search technique.
     * \param dst the destination IP address
     */
    void ScheduleRreqRetry(IpAddress dst);
    /**
     * Set lifetime field in routing table entry to the maximum of existing lifetime and lt, if the
     * entry exists
     * \param addr destination address
     * \param lt proposed time for lifetime field in routing table entry for destination with
     * address addr.
     * \return true if route to destination address addr exist
     */
    bool UpdateRouteLifeTime(IpAddress addr, Time lt);
    /**
     * Update neighbor record.
     * \param receiver is supposed to be my interface
     * \param sender is supposed to be IP address of my neighbor.
     */
    void UpdateRouteToNeighbor(IpAddress sender, IpAddress receiver);
    /**
     * Test whether the provided address is assigned to an interface on this node
     * \param src the source IP address
     * \returns true if the IP address is the node's IP address
     */
    bool IsMyOwnAddress(IpAddress src);
    /**
     * Find unicast socket with local interface address iface
     *
     * \param iface the interface
     * \returns the socket associated with the interface
     */
    Ptr<Socket> FindSocketWithInterfaceAddress(IpInterfaceAddress iface) const;
    /**
     * Find subnet directed broadcast socket with local interface address iface
     *
     * \param iface the interface
     * \returns the socket associated with the interface
     */
    Ptr<Socket> FindSubnetBroadcastSocketWithInterfaceAddress(IpInterfaceAddress iface) const;
    /**
     * Create loopback route for given header
     *
     * \param header the IP header
     * \param oif the output interface net device
     * \returns the route
     */
    Ptr<IpRoute> LoopbackRoute(const IpHeader& header, Ptr<NetDevice> oif) const;

    /**
     * \name Receive control packets
     * @{
     */
    /**
     * Receive and process control packet
     * \param socket input socket
     */
    void RecvAodvv2(Ptr<Socket> socket);
    /**
     * Receive RREQ
     * \param p packet
     * \param receiver receiver address
     * \param src sender address
     * \param tlvHeader TLV header
     */
    void RecvRequest(Ptr<Packet> p, IpAddress receiver, IpAddress src, PbbPacket tlvHeader);
    /**
     * Receive RREP
     * \param p packet
     * \param my destination address
     * \param src sender address
     * \param tlvHeader TLV header
     */
    void RecvReply(Ptr<Packet> p, IpAddress my, IpAddress src, PbbPacket tlvHeader);
    /**
     * Receive RREP_ACK
     * \param neighbor neighbor address
     * \param tlvHeader TLV header
     */
    void RecvReplyAck(IpAddress neighbor, PbbPacket tlvHeader);
    /**
     * Receive RERR
     * \param p packet
     * \param src sender address
     * \param tlvHeader TLV header
     */
    /// Receive  from node with address src
    void RecvError(Ptr<Packet> p, IpAddress src, PbbPacket tlvHeader);
    /** @} */

    /**
     * \name Send
     * @{
     */
    /** Forward packet from route request queue
     * \param dst destination address
     * \param route route to use
     */
    void SendPacketFromQueue(IpAddress dst, Ptr<IpRoute> route);
    /** Send RREQ
     * \param dst destination address
     */
    void SendRequest(IpAddress dst);
    /** Add TLV headers to packet
     * \param packet packet
     * \param socket socket
     * \param dst destination address
     * \param sequenceNumber sequence number
     */
    void AddTlvHeaders(Ptr<Packet> packet,
                       Ptr<Socket> socket,
                       IpAddress dst,
                       uint32_t sequenceNumber);
    /** Send RREP
     * \param rreqHeader route request header
     * \param toOrigin routing table entry to originator
     * \param hopCount hop count
     */
    void SendReply(const RreqHeader<IpAddress>& rreqHeader,
                   const RoutingTableEntry<IpAddress>& toOrigin,
                   uint8_t hopCount);
    /** Send RREP by intermediate node
     * \param toDst routing table entry to destination
     * \param toOrigin routing table entry to originator
     */
    void SendReplyByIntermediateNode(RoutingTableEntry<IpAddress>& toDst,
                                     RoutingTableEntry<IpAddress>& toOrigin);
    /** Send RREP_ACK
     * \param neighbor neighbor address
     */
    void SendReplyAck(IpAddress neighbor);
    /** Initiate RERR
     * \param nextHop next hop address
     */
    void SendRerrWhenBreaksLinkToNextHop(IpAddress nextHop);
    /** Forward RERR
     * \param packet packet
     * \param precursors list of addresses of the visited nodes
     */
    void SendRerrMessage(Ptr<Packet> packet, std::vector<IpAddress> precursors);
    /**
     * Send RERR message when no route to forward input packet. Unicast if there is reverse
     * route to originating node, broadcast otherwise. \param dst destination node IP address
     * \param dstSeqNo destination node sequence number
     * \param origin originating node IP address
     */
    void SendRerrWhenNoRouteToForward(IpAddress dst, uint32_t dstSeqNo, IpAddress origin);
    /** @} */

    /**
     * Send packet to destination socket
     * \param socket destination node socket
     * \param packet packet to send
     * \param destination destination node IP address
     */
    void SendTo(Ptr<Socket> socket, Ptr<Packet> packet, IpAddress destination);

    /// RREQ rate limit timer
    Timer m_rreqRateLimitTimer;
    /// Reset RREQ count and schedule RREQ rate limit timer with delay 1 sec.
    void RreqRateLimitTimerExpire();
    /// RERR rate limit timer
    Timer m_rerrRateLimitTimer;
    /// Reset RERR count and schedule RERR rate limit timer with delay 1 sec.
    void RerrRateLimitTimerExpire();
    /// Map IP address + RREQ timer.
    std::map<IpAddress, Timer> m_addressReqTimer;
    /**
     * Handle route discovery process
     * \param dst the destination IP address
     */
    void RouteRequestTimerExpire(IpAddress dst);
    /**
     * Mark link to neighbor node as unidirectional for blacklistTimeout
     *
     * \param neighbor the IP address of the neighbor node
     * \param blacklistTimeout the black list timeout time
     */
    void AckTimerExpire(IpAddress neighbor, Time blacklistTimeout);

    /// Provides uniform random variables.
    Ptr<UniformRandomVariable> m_uniformRandomVariable;
    /// Keep track of the last bcast time
    Time m_lastBcastTime;
};

typedef Aodvv2RoutingProtocol<Ipv4RoutingProtocol> Ipv4Aodvv2RoutingProtocol;
typedef Aodvv2RoutingProtocol<Ipv6RoutingProtocol> Ipv6Aodvv2RoutingProtocol;

} // namespace aodvv2
} // namespace ns3

#endif /* AODVV2_ROUTING_PROTOCOL_H */
