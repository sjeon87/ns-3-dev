/*
 * Copyright (c) 2009 Drexel University
 *
 * SPDX-License-Identifier: GPL-2.0-only and NIST-Software
 *
 * Author: Tom Wambold <tom5760@gmail.com>
 */

/* These classes implement the NHDP neighbor discovery protocol.  See
 * https://datatracker.ietf.org/doc/html/rfc6130 for more info */

#ifndef NHDP_CLIENT_H
#define NHDP_CLIENT_H

#include "nhdp-info-base.h"

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ipv4-address.h"
#include "ns3/packetbb.h"
#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"
#include "ns3/traced-callback.h"

#include <list>
#include <map>
#include <optional>
#include <queue>
#include <set>
#include <utility>
#include <vector>

namespace ns3
{

namespace manet
{

/* PacketBB Message Types */
const uint8_t MESSAGE_TYPE_HELLO = 0;

/* PacketBB Message TLV Types (RFC 5497) */
const uint8_t MSG_TLV_INTERVAL_TIME = 0;
const uint8_t MSG_TLV_VALIDITY_TIME = 1;

/* PacketBB Address Block Types */
const uint8_t ADDR_TLV_LOCAL_IF = 2;
const uint8_t ADDR_TLV_LINK_STATUS = 3;
const uint8_t ADDR_TLV_OTHER_NEIGHB = 4;

/* PacketBB Address Block Values */
const uint8_t ADDR_TLV_LOCAL_IF_THIS_IF = 0;
const uint8_t ADDR_TLV_LOCAL_IF_OTHER_IF = 1;
const uint8_t ADDR_TLV_LINK_STATUS_LOST = 0;
const uint8_t ADDR_TLV_LINK_STATUS_SYMMETRIC = 1;
const uint8_t ADDR_TLV_LINK_STATUS_HEARD = 2;
const uint8_t ADDR_TLV_OTHER_NEIGHB_LOST = 0;
const uint8_t ADDR_TLV_OTHER_NEIGHB_SYMMETRIC = 1;

/** Used as the comparator when making heaps out of tuples with expiration
 * times. */
template <class T>
class TimeCompare
{
  public:
    /**
     * Compare two tuples by their expiration time (later time = lower priority).
     *
     * @param a First tuple.
     * @param b Second tuple.
     * @return True if a expires after b.
     */
    bool operator()(const Ptr<T>& a, const Ptr<T>& b)
    {
        return b->time < a->time;
    }
};

/**
 * @ingroup applications
 * @defgroup nhdp NHDP
 */

enum class Action
{
    ADDED,
    CHANGED,
    REMOVED
};

enum class AddressTlvLinkStatus
{
    LOST,
    SYMMETRIC,
    HEARD
};

/**
 * @ingroup nhdp
 * @brief Address family that a NHDP client operates over.
 *
 * RFC 6130 is address-family agnostic, but Sec. 12.1 implies that a router
 * uses a single address length.  Each NhdpClient instance therefore operates
 * over exactly one family; run two instances for dual-stack operation.
 */
enum class AddressMode
{
    IPV4,
    IPV6
};

/**
 * @ingroup nhdp
 * @brief A NHDP client
 */
class NhdpClient : public Application
{
  public:
    NhdpClient();

    /**
     * Get the type ID.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    /* Local Interface Base Methods */
    /**
     * Prevents an interface from being used for transmitting HELLO messages.
     * @param ifaddr The interface index to exclude from MANET operation.
     */
    void MarkIfaceNonManet(uint32_t ifaddr);

    /* End Information Bases */

    /*
     * Adds a PacketBB message to be sent out with the next hello message.
     * void QueueMessage (Ptr<PbbMessage> message);
     *
     * Sets a callback for a particular message type.  On reception of a message
     * of that type, the callback is called; multiple callbacks can be registered
     * to one message type.
     * void RegisterMessageCallback (uint8_t messageType, Callback<PbbMessage> cb);
     */

    /**
     * @brief Sets a callback for a received NHDP packet for calculating link quality.
     *
     * @param cb The callback to call when a packet is received.
     *
     * The received packet is passed to the callback as a non-const object so that any
     * PacketTag object can be removed if needed.
     */
    void RegisterLinkQualityCallback(Callback<double, Ptr<Packet>> cb);

    /**
     * A callback to receive indications that the lower layer link has failed.
     *
     * @param neighborAddr The address associated with the failed link.
     */
    void HandleLinkFailure(const Address& neighborAddr);

    /**
     * Receive a HELLO packet delivered directly in bypass mode (no socket).
     * @param packet The received HELLO packet.
     */
    void BypassRecv(Ptr<Packet> packet);

    /**
     * Socket receive callback for incoming HELLO packets.
     * @param socket The socket with data available to read.
     */
    void HandleRecv(Ptr<Socket> socket);

    /**
     * Trace sink invoked when a lower-layer direct link to a peer is established.
     *
     * @param srcL2Id The local layer-2 identifier.
     * @param selfAddr The local address.
     * @param peerL2Id The peer's layer-2 identifier.
     * @param peerAddr The peer's address.
     */
    void HandleDirectLinkEstablishedTrace(uint32_t srcL2Id,
                                          Address selfAddr,
                                          uint32_t peerL2Id,
                                          Address peerAddr);

    /**
     * Trace sink invoked when a lower-layer direct link to a peer is releasing.
     *
     * @param srcL2Id The local layer-2 identifier.
     * @param selfAddr The local address.
     * @param peerL2Id The peer's layer-2 identifier.
     * @param peerAddr The peer's address.
     */
    void HandleDirectLinkReleasingTrace(uint32_t srcL2Id,
                                        Address selfAddr,
                                        uint32_t peerL2Id,
                                        Address peerAddr);

    /**
     * Get the Neighbor Set (RFC 6130 Sec. 8.1).
     * @return The list of Neighbor Tuples.
     */
    const std::list<NeighborTuple>& GetNeighborInfoBase() const;
    /**
     * Get the Link Set (RFC 6130 Sec. 7.1).
     * @return The list of Link Tuples.
     */
    const std::list<LinkTuple>& GetLinkInfoBase() const;
    /**
     * Get the 2-Hop Set (RFC 6130 Sec. 7.2).
     * @return The list of 2-Hop Tuples.
     */
    const std::list<TwoHopTuple>& GetTwoHopInfoBase() const;
    /**
     * Get the Lost Neighbor Set (RFC 6130 Sec. 8.2).
     * @return The list of Lost Neighbor Tuples.
     */
    const std::list<LostNeighborTuple>& GetLostNeighborSet() const;

    /**
     * Find the Neighbor Tuple whose N_neighbor_addr_list contains an address.
     *
     * @param addr The address to look up.
     * @return Pointer to the matching Neighbor Tuple, or nullptr if none.
     */
    const NeighborTuple* FindNeighborTuple(const Address& addr) const;

    /**
     * Find the Link Tuple whose L_neighbor_iface_addr_list contains an address.
     *
     * @param addr The address to look up.
     * @return Pointer to the matching Link Tuple, or nullptr if none.
     */
    const LinkTuple* FindLinkTuple(const Address& addr) const;

    /**
     * Find the Lost Neighbor Tuple for an address.
     *
     * @param addr The address to look up.
     * @return Pointer to the matching Lost Neighbor Tuple, or nullptr if none.
     */
    const LostNeighborTuple* FindLostNeighbor(const Address& addr) const;

    /**
     * Find the 2-Hop Tuple reached via a 1-hop neighbor address.
     *
     * @param via A 1-hop neighbor interface address (matched by overlap).
     * @param twoHopAddr The 2-hop neighbor address.
     * @return Pointer to the matching 2-Hop Tuple, or nullptr if none.
     */
    const TwoHopTuple* FindTwoHopTuple(const Address& via, const Address& twoHopAddr) const;

    /**
     * TracedCallback signature for neighbor information base change event.
     *
     * @param [in] action The type of change (added, changed, removed)
     * @param [in] oldTuple The old NeighborTuple
     * @param [in] newTuple The new NeighborTuple
     */
    typedef void (*NeighborChangeTracedCallback)(Action action,
                                                 const NeighborTuple& oldTuple,
                                                 const NeighborTuple& newTuple);

    /**
     * TracedCallback signature for Link Set change event.
     *
     * If the type of change is added or removed, both oldTuple and newTuple will
     * be set to the same value.
     *
     * @param [in] action The type of change (added, changed, removed)
     * @param [in] oldTuple The old LinkTuple
     * @param [in] newTuple The new LinkTuple
     */
    typedef void (*LinkChangeTracedCallback)(Action action,
                                             const LinkTuple& oldTuple,
                                             const LinkTuple& newTuple);

    /**
     * TracedCallback signature for two-hop information base change event.
     *
     * @param [in] action The type of change (added, changed, removed)
     * @param [in] oldTuple The old TwoHopTuple
     * @param [in] newTuple The new TwoHopTuple
     */
    typedef void (*TwoHopChangeTracedCallback)(Action action,
                                               const TwoHopTuple& oldTuple,
                                               const TwoHopTuple& newTuple);

    /**
     * TracedCallback signature for lost neighbor information base change event.
     *
     * @param [in] action The type of change (added, changed, removed)
     * @param [in] oldTuple The old LostNeighborTuple
     * @param [in] newTuple The new LostNeighborTuple
     */
    typedef void (*LostNeighborChangeTracedCallback)(Action action,
                                                     const LostNeighborTuple& oldTuple,
                                                     const LostNeighborTuple& newTuple);

    /**
     * TracedCallback signature for reported L2 link failure
     *
     * @param [in] address The peer address
     */
    typedef void (*LinkFailureTracedCallback)(const Address& address);

    /**
     * TracedCallback signature for HELLO send trace
     *
     * @param [in] addr The sending address
     * @param [in] links List of links advertised
     */
    typedef void (*HelloSendTracedCallback)(
        Address addr,
        const std::vector<std::pair<Address, AddressTlvLinkStatus>>& links);

    /**
     * TracedCallback signature for HELLO message send trace
     *
     * @param [in] addr The sending address
     * @param [in] helloMsg The (modifiable) HELLO message
     */
    typedef void (*HelloMessageSendTracedCallback)(Address addr, Ptr<PbbMessage> helloMsg);

    /**
     * TracedCallback signature for HELLO recv trace
     *
     * @param [in] addr The address of the originator
     * @param [in] links List of links advertised
     * @param [in] quality Link quality
     */
    typedef void (*HelloRecvTracedCallback)(
        Address addr,
        const std::vector<std::pair<Address, AddressTlvLinkStatus>>& links,
        std::optional<double> quality);

    /**
     * TracedCallback signature for HELLO message receive trace
     *
     * @param [in] helloMsg The HELLO message
     * @param [in] neighborAddr The neighbor address
     * @param [in] quality Link quality if available
     */
    typedef void (*HelloMessageRecvTracedCallback)(Ptr<PbbMessage> helloMsg,
                                                   Address neighborAddr,
                                                   std::optional<double> quality);

    int64_t AssignStreams(int64_t stream) override;

  protected:
    void DoDispose() override;
    void DoInitialize() override;

  private:
    void StartApplication() override;
    void StopApplication() override;

    /**
     * Process a received HELLO message.
     *
     * @param msg The HELLO PbbMessage.
     * @param quality Link quality if available.
     * @param datagramSrc The source address of the IP datagram carrying the HELLO.
     * @return A representative address of the originating neighbor.
     */
    Address HandlePbbMessage(Ptr<PbbMessage> msg,
                             std::optional<double> quality,
                             const Address& datagramSrc);

    /**
     * Collect the address lists from a HELLO message (RFC 6130, Sec. 12.2).
     *
     * @param msg The HELLO PbbMessage.
     * @param datagramSrc The IP datagram source (default Sending Address List if empty).
     * @param [out] sendingList The Sending Address List (LOCAL_IF THIS_IF addresses).
     * @param [out] neighborList The Neighbor Address List (THIS_IF + OTHER_IF addresses).
     * @param [out] linkStatus The (address, status) pairs from LINK_STATUS blocks.
     */
    void CollectAddressLists(Ptr<PbbMessage> msg,
                             const Address& datagramSrc,
                             std::vector<Address>& sendingList,
                             std::vector<Address>& neighborList,
                             std::vector<std::pair<Address, AddressTlvLinkStatus>>& linkStatus);

    /**
     * Update the Neighbor Set (RFC 6130, Sec. 12.3).
     *
     * @param neighborList The Neighbor Address List.
     * @param [out] removedList The Removed Address List (appended to).
     * @param [out] lostList The Lost Address List (appended to).
     */
    void UpdateNeighborSet(const std::vector<Address>& neighborList,
                           std::vector<Address>& removedList,
                           std::vector<Address>& lostList);

    /**
     * Update the Lost Neighbor Set from the Lost Address List (RFC 6130, Sec. 12.4).
     *
     * @param lostList The Lost Address List.
     */
    void UpdateLostNeighborSet(const std::vector<Address>& lostList);

    /**
     * Update the Link Set (RFC 6130, Sec. 12.5).
     *
     * @param sendingList The Sending Address List.
     * @param linkStatus The (address, status) pairs from LINK_STATUS blocks.
     * @param validityTime The HELLO validity time.
     * @param quality Link quality if available.
     * @param removedList The Removed Address List (consumed by Sec. 12.5 steps 1-2).
     */
    void UpdateLinkSet(const std::vector<Address>& sendingList,
                       const std::vector<std::pair<Address, AddressTlvLinkStatus>>& linkStatus,
                       Time validityTime,
                       std::optional<double> quality,
                       const std::vector<Address>& removedList);

    /**
     * Update the 2-Hop Set (RFC 6130, Sec. 12.6).
     *
     * @param sendingList The Sending Address List.
     * @param linkStatus The (address, status) pairs from LINK_STATUS blocks.
     * @param validityTime The HELLO validity time.
     */
    void UpdateTwoHopSet(const std::vector<Address>& sendingList,
                         const std::vector<std::pair<Address, AddressTlvLinkStatus>>& linkStatus,
                         Time validityTime);

    /**
     * Schedule the next HELLO transmission on a socket (with jitter).
     * @param socket The socket to send the HELLO on.
     */
    void ScheduleHello(Ptr<Socket> socket);
    /**
     * Build and transmit a HELLO message on a socket.
     * @param socket The socket to send the HELLO on.
     */
    void SendHello(Ptr<Socket> socket);

    /**
     * Refresh the Link Set timers and re-derive dependent neighbor state.
     */
    void UpdateLinkTuples();

    // void CleanRemovedInterfaceAddressSet (void);

    /**
     * Build the LOCAL_IF address block (THIS_IF/OTHER_IF) for an outgoing HELLO.
     * @param socket The socket the HELLO will be sent on.
     * @return The constructed address block.
     */
    Ptr<PbbAddressBlock> BuildLocalAddressBlock(Ptr<Socket> socket);
    /**
     * Build the LINK_STATUS address block for an outgoing HELLO.
     * @param socket The socket the HELLO will be sent on.
     * @return The constructed address block.
     */
    Ptr<PbbAddressBlock> BuildLinkStatusAddressBlock(Ptr<Socket> socket);

    /* Address-list helpers (RFC 6130 overlap matching) */

    /**
     * Test whether an address is contained in a list of addresses.
     * @param list The address list.
     * @param addr The address.
     * @return True if addr is in list.
     */
    static bool AddressInList(const std::vector<Address>& list, const Address& addr);

    /**
     * Test whether two address lists share at least one address.
     * @param a First address list.
     * @param b Second address list.
     * @return True if the lists overlap.
     */
    static bool AddressListsOverlap(const std::vector<Address>& a, const std::vector<Address>& b);

    /** @copydoc FindNeighborTuple */
    std::list<NeighborTuple>::iterator FindNeighborTupleIt(const Address& addr);
    /** @copydoc FindLinkTuple */
    std::list<LinkTuple>::iterator FindLinkTupleIt(const Address& addr);
    /** @copydoc FindLostNeighbor */
    std::list<LostNeighborTuple>::iterator FindLostNeighborIt(const Address& addr);
    /** @copydoc FindTwoHopTuple */
    std::list<TwoHopTuple>::iterator FindTwoHopTupleIt(const std::vector<Address>& via,
                                                       const Address& twoHopAddr);

    /* Configuration parameters */
    Ipv4Address m_address; //!< IPv4 multicast destination (used in IPv4 mode)
    AddressMode m_addressMode{AddressMode::IPV4}; //!< Address family of this instance
    uint32_t m_port{0};                           //!< UDP port used for HELLO exchange

    Time m_helloInterval;    //!< HELLO_INTERVAL: nominal interval between HELLOs
    Time m_helloMinInterval; //!< HELLO_MIN_INTERVAL: minimum interval between HELLOs
    Time m_refreshInterval;  //!< REFRESH_INTERVAL: link refresh interval

    Time m_lHoldTime; //!< L_HOLD_TIME: link tuple hold time
    Time m_hHoldTime; //!< H_HOLD_TIME: advertised HELLO validity time

    double m_hystAccept{0};       //!< HYST_ACCEPT: hysteresis acceptance threshold
    double m_hystReject{0};       //!< HYST_REJECT: hysteresis rejection threshold
    double m_initialQuality{0};   //!< INITIAL_QUALITY: link quality for a new link
    bool m_initialPending{false}; //!< INITIAL_PENDING: whether a new link starts pending

    Time m_hpMaxJitter; //!< HP_MAXJITTER: maximum jitter for periodic HELLOs
    Time m_htMaxJitter; //!< HT_MAXJITTER: maximum jitter for triggered HELLOs

    Time m_nHoldTime; //!< N_HOLD_TIME: lost neighbor hold time
    Time m_iHoldTime; //!< I_HOLD_TIME: removed interface address hold time

    /* Information bases */
    std::list<NeighborTuple> m_neighborInfoBase;    //!< Neighbor Set (RFC 6130 Sec. 8.1)
    std::list<LinkTuple> m_linkInfoBase;            //!< Link Set (RFC 6130 Sec. 7.1)
    std::list<TwoHopTuple> m_twoHopInfoBase;        //!< 2-Hop Set (RFC 6130 Sec. 7.2)
    std::list<LostNeighborTuple> m_lostNeighborSet; //!< Lost Neighbor Set (RFC 6130 Sec. 8.2)

    /// Map of peer address to peer client used to deliver HELLOs in bypass mode
    std::map<Address, Ptr<NhdpClient>> m_establishedPeers;

    /* Other attributes */
    bool m_running{false};                            //!< Whether the application is running
    std::set<uint32_t> m_nonManetSet;                 //!< Non-MANET interface indices
    Ptr<UniformRandomVariable> m_rng;                 //!< Random variable used for HELLO jitter
    std::map<Ptr<Socket>, Address> m_socketAddresses; //!< Send socket to local address map
    Ptr<Socket> m_recvSocket;                         //!< Receiving socket
    /// Local interface addresses (Receiving / THIS_IF Address List, RFC 6130 Sec. 12.2).
    /// In IPv4 mode this is a single address; in IPv6 mode it is the link-local plus the
    /// routing Unique Local Address (RFC 4193).
    std::vector<Address> m_localAddresses;
    /// Source address of emitted IP datagrams (the link-local address in IPv6 mode).
    Address m_datagramSource;
    Ptr<PbbAddressBlock> m_localAddrBlock; //!< Cached LOCAL_IF address block for HELLOs
    bool m_bypassMode{false};              //!< Whether HELLOs bypass sockets

    /**
     * Remove 2-Hop Tuples whose validity time has expired.
     */
    void RemoveExpiredTwoHopNeighbors();

    /**
     * Remove all 2-Hop Tuples reached via a given 1-hop neighbor.
     *
     * Implements the RFC 6130, Sec. 13.2 cleanup performed when a link to the
     * neighbor is removed or is no longer symmetric.
     *
     * @param neighborAddrs The 1-hop neighbor interface addresses whose 2-Hop
     *                       Tuples to remove (matched by overlap).
     */
    void RemoveTwoHopTuples(const std::vector<Address>& neighborAddrs);

    Callback<double, Ptr<Packet>> m_linkQualityCallback; //!< Link-quality callback for HELLOs

    /// Neighbor Set change trace
    TracedCallback<Action, const NeighborTuple&, const NeighborTuple&> m_neighborChangeTrace;
    /// Link Set change trace
    TracedCallback<Action, const LinkTuple&, const LinkTuple&> m_linkChangeTrace;
    /// 2-Hop Set change trace
    TracedCallback<Action, const TwoHopTuple&, const TwoHopTuple&> m_twoHopChangeTrace;
    /// Lost Neighbor Set change trace
    TracedCallback<Action, const LostNeighborTuple&, const LostNeighborTuple&>
        m_lostNeighborChangeTrace;
    /// HELLO send trace (sending address and advertised links)
    TracedCallback<Address, const std::vector<std::pair<Address, AddressTlvLinkStatus>>&>
        m_helloSendTrace;
    /// HELLO receive trace (originator address, advertised links, link quality)
    TracedCallback<Address,
                   const std::vector<std::pair<Address, AddressTlvLinkStatus>>&,
                   std::optional<double>>
        m_helloRecvTrace;
    /// HELLO message send trace (sending address and HELLO message)
    TracedCallback<Address, Ptr<PbbMessage>> m_helloMessageSendTrace;
    /// HELLO message receive trace (HELLO message, neighbor address, link quality)
    TracedCallback<Ptr<PbbMessage>, Address, std::optional<double>> m_helloMessageRecvTrace;
    /// Link-failure trace (peer address)
    TracedCallback<const Address&> m_linkFailureTrace;
    /// Packet transmit trace
    TracedCallback<Ptr<const Packet>> m_txTrace;

    /*
    std::map< uint32_t, Ptr<Socket> > m_indexSockets;
    std::map< Ptr<Socket>, uint32_t > m_socketsIndex;

    std::queue< Ptr<PbbMessage> > m_messages;
    std::multimap< uint8_t, Callback<PbbMessage> > m_callbacks;
    */
};

std::ostream& operator<<(std::ostream& os, const Action& action);
std::ostream& operator<<(std::ostream& os, const AddressTlvLinkStatus& status);

} // namespace manet

} // namespace ns3

#endif /* NHDP_CLIENT_H */
