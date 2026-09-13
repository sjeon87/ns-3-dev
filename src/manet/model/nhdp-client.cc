/*
 * Copyright (c) 2009 Drexel University
 *
 * SPDX-License-Identifier: GPL-2.0-only and NIST-Software
 *
 * Author: Tom Wambold <tom5760@gmail.com>
 */

/* These classes implement the NHDP neighbor discovery protocol.  See
 * https://datatracker.ietf.org/doc/html/rfc6130 for more info */

#include "nhdp-client.h"

#include "nhdp-hello-tag.h"
#include "time-tlv.h"

#include "ns3/address-utils.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-packet-info-tag.h"
#include "ns3/ipv4.h"
#include "ns3/ipv6-address.h"
#include "ns3/ipv6-interface-address.h"
#include "ns3/ipv6-packet-info-tag.h"
#include "ns3/ipv6.h"
#include "ns3/log.h"
#include "ns3/node-list.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/string.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/udp-socket.h"
#include "ns3/uinteger.h"

#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NhdpClient");

namespace manet
{

/* From RFC 5498 */
static const Ipv4Address LL_MANET_ROUTERS_IPV4("224.0.0.109");
/* From RFC 5498 */
static const Ipv6Address LL_MANET_ROUTERS_IPV6("FF02:0:0:0:0:0:0:6D");

/* From RFC 5498 */
static const uint32_t UDP_PORT_MANET = 269;

static const Time DEFAULT_HELLO_INTERVAL = Seconds(2);
static const Time DEFAULT_HELLO_MIN_INTERVAL = DEFAULT_HELLO_INTERVAL / 4;
static const Time DEFAULT_REFRESH_INTERVAL = DEFAULT_HELLO_INTERVAL;

static const Time DEFAULT_H_HOLD_TIME = 3 * DEFAULT_REFRESH_INTERVAL;
static const Time DEFAULT_L_HOLD_TIME = DEFAULT_H_HOLD_TIME;

static const double DEFAULT_HYST_ACCEPT = 1.0;
static const double DEFAULT_HYST_REJECT = 0.0;
static const double DEFAULT_INITIAL_QUALITY = 1.0;
static const bool DEFAULT_INITIAL_PENDING = false;

static const Time DEFAULT_HP_MAX_JITTER = DEFAULT_HELLO_INTERVAL / 4;
static const Time DEFAULT_HT_MAX_JITTER = DEFAULT_HP_MAX_JITTER;

static const Time DEFAULT_N_HOLD_TIME = DEFAULT_L_HOLD_TIME;
static const Time DEFAULT_I_HOLD_TIME = DEFAULT_N_HOLD_TIME;

static const Time EXPIRED = Seconds(0);

NS_OBJECT_ENSURE_REGISTERED(NhdpClient);

TypeId
NhdpClient::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::manet::NhdpClient")
            .SetParent<Application>()
            .AddConstructor<NhdpClient>()

            .AddAttribute("Address",
                          "IPv4 multicast address to use (used in IPv4 mode only).",
                          Ipv4AddressValue(LL_MANET_ROUTERS_IPV4),
                          MakeIpv4AddressAccessor(&NhdpClient::m_address),
                          MakeIpv4AddressChecker())
            .AddAttribute("AddressMode",
                          "Address family this NHDP instance operates over.  RFC 6130 uses a "
                          "single address length per router; run two instances for dual-stack.",
                          EnumValue(AddressMode::IPV4),
                          MakeEnumAccessor<AddressMode>(&NhdpClient::m_addressMode),
                          MakeEnumChecker(AddressMode::IPV4, "Ipv4", AddressMode::IPV6, "Ipv6"))
            .AddAttribute("Port",
                          "UDP port to use.",
                          UintegerValue(UDP_PORT_MANET),
                          MakeUintegerAccessor(&NhdpClient::m_port),
                          MakeUintegerChecker<uint32_t>())

            /* Interface Parameters */
            .AddAttribute("HelloInterval",
                          "Default maximum interval between HELLOs on a MANET interface.",
                          TimeValue(DEFAULT_HELLO_INTERVAL),
                          MakeTimeAccessor(&NhdpClient::m_helloInterval),
                          MakeTimeChecker())
            .AddAttribute("HelloMinInterval",
                          "Default minimum interval between HELLOs on a MANET interface.",
                          TimeValue(DEFAULT_HELLO_MIN_INTERVAL),
                          MakeTimeAccessor(&NhdpClient::m_helloMinInterval),
                          MakeTimeChecker())
            .AddAttribute("RefreshInterval",
                          "Default maximum interval between advertisements of each 1-hop neighbor "
                          "in a HELLO.",
                          TimeValue(DEFAULT_REFRESH_INTERVAL),
                          MakeTimeAccessor(&NhdpClient::m_refreshInterval),
                          MakeTimeChecker())

            .AddAttribute("LHoldTime",
                          "Time to advertise former 1-hop neighbor addresses as lost for removal "
                          "from Link Set.",
                          TimeValue(DEFAULT_L_HOLD_TIME),
                          MakeTimeAccessor(&NhdpClient::m_lHoldTime),
                          MakeTimeChecker())
            .AddAttribute(
                "HHoldTime",
                "Time advertised for the validity of messages sent from a MANET interface.",
                TimeValue(DEFAULT_H_HOLD_TIME),
                MakeTimeAccessor(&NhdpClient::m_hHoldTime),
                MakeTimeChecker())

            .AddAttribute("HystAccept",
                          "Link quality threshold at or above which a link becomes usable.",
                          DoubleValue(DEFAULT_HYST_ACCEPT),
                          MakeDoubleAccessor(&NhdpClient::m_hystAccept),
                          MakeDoubleChecker<double>(0.0, 1.0))
            .AddAttribute("HystReject",
                          "Link quality threshold below which a link becomes unusable.",
                          DoubleValue(DEFAULT_HYST_REJECT),
                          MakeDoubleAccessor(&NhdpClient::m_hystReject),
                          MakeDoubleChecker<double>(0.0, 1.0))
            .AddAttribute("InitialQuality",
                          "The initial quality of a newly identified link.",
                          DoubleValue(DEFAULT_INITIAL_QUALITY),
                          MakeDoubleAccessor(&NhdpClient::m_initialQuality),
                          MakeDoubleChecker<double>(0.0, 1.0))
            .AddAttribute("InitialPending",
                          "If true, newly identified links are considered pending, and are not "
                          "usable until quality reaches HystAccept.",
                          BooleanValue(DEFAULT_INITIAL_PENDING),
                          MakeBooleanAccessor(&NhdpClient::m_initialPending),
                          MakeBooleanChecker())

            .AddAttribute("HPMaxJitter",
                          "MAXJITTER used in periodically generated HELLO messages",
                          TimeValue(DEFAULT_HP_MAX_JITTER),
                          MakeTimeAccessor(&NhdpClient::m_hpMaxJitter),
                          MakeTimeChecker())
            .AddAttribute("HTMaxJitter",
                          "MAXJITTER used in externally triggered HELLO messages",
                          TimeValue(DEFAULT_HT_MAX_JITTER),
                          MakeTimeAccessor(&NhdpClient::m_htMaxJitter),
                          MakeTimeChecker())

            /* Router parameters */
            .AddAttribute("NHoldTime",
                          "Time to advertise former 1-hop neighbor addresses as lost for removal "
                          "from TwoHopSets.",
                          TimeValue(DEFAULT_N_HOLD_TIME),
                          MakeTimeAccessor(&NhdpClient::m_nHoldTime),
                          MakeTimeChecker())
            .AddAttribute("IHoldTime",
                          "Time to record recently used local interface addresses.",
                          TimeValue(DEFAULT_I_HOLD_TIME),
                          MakeTimeAccessor(&NhdpClient::m_iHoldTime),
                          MakeTimeChecker())
            .AddAttribute("BypassMode",
                          "Whether to bypass the sending of messages on the channel.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&NhdpClient::m_bypassMode),
                          MakeBooleanChecker())
            .AddTraceSource("NeighborChange",
                            "Notification that neighbor information base changed",
                            MakeTraceSourceAccessor(&NhdpClient::m_neighborChangeTrace),
                            "ns3::manet::NhdpClient::NeighborChangeTracedCallback")
            .AddTraceSource("LinkChange",
                            "Notification that link set information base changed",
                            MakeTraceSourceAccessor(&NhdpClient::m_linkChangeTrace),
                            "ns3::manet::NhdpClient::LinkChangeTracedCallback")
            .AddTraceSource("TwoHopChange",
                            "Notification that two-hop information base changed",
                            MakeTraceSourceAccessor(&NhdpClient::m_twoHopChangeTrace),
                            "ns3::manet::NhdpClient::TwoHopChangeTracedCallback")
            .AddTraceSource("LostNeighborChange",
                            "Notification that lost neighbor information base changed",
                            MakeTraceSourceAccessor(&NhdpClient::m_lostNeighborChangeTrace),
                            "ns3::manet::NhdpClient::LostNeighborChangeTracedCallback")
            .AddTraceSource("LinkFailure",
                            "Trace of reported L2 link failure",
                            MakeTraceSourceAccessor(&NhdpClient::m_linkFailureTrace),
                            "ns3::manet::NhdpClient::LinkFailureTracedCallback")
            .AddTraceSource("HelloSend",
                            "Read-only trace of the contents of a sent HELLO",
                            MakeTraceSourceAccessor(&NhdpClient::m_helloSendTrace),
                            "ns3::manet::NhdpClient::HelloSendTracedCallback")
            .AddTraceSource("HelloRecv",
                            "Read-only trace of the contents of a received HELLO",
                            MakeTraceSourceAccessor(&NhdpClient::m_helloRecvTrace),
                            "ns3::manet::NhdpClient::HelloRecvTracedCallback")
            .AddTraceSource("HelloMessageSend",
                            "Trace of (modifiable) PbbMessage before it is sent out as HELLO",
                            MakeTraceSourceAccessor(&NhdpClient::m_helloMessageSendTrace),
                            "ns3::manet::NhdpClient::HelloMessageSendTracedCallback")
            .AddTraceSource("HelloMessageRecv",
                            "Trace of PbbMessage HELLO after it has been processed by NHDP",
                            MakeTraceSourceAccessor(&NhdpClient::m_helloMessageRecvTrace),
                            "ns3::manet::NhdpClient::HelloMessageRecvTracedCallback")
            .AddTraceSource("Tx",
                            "Trace of Packet just before sending to UDP socket",
                            MakeTraceSourceAccessor(&NhdpClient::m_txTrace),
                            "ns3::Packet::TracedCallback");
    return tid;
}

NhdpClient::NhdpClient()
{
    NS_LOG_FUNCTION(this);
    m_rng = CreateObject<UniformRandomVariable>();
}

void
NhdpClient::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    // In IPv6 mode the IPv4 'Address' attribute is meaningless; the RFC 5498
    // IPv6 LL-MANET-Routers group (FF02::6D) is used.  Reject an inconsistent
    // configuration rather than silently ignoring it.
    if (m_addressMode == AddressMode::IPV6)
    {
        NS_ASSERT_MSG(m_address == LL_MANET_ROUTERS_IPV4,
                      "In IPv6 mode the IPv4 'Address' attribute must be left at its default; "
                      "FF02::6D is used as the multicast destination");
    }
    m_rng->SetAttribute("Max", DoubleValue(m_hpMaxJitter.GetSeconds()));
    if (!m_recvSocket)
    {
        m_recvSocket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
        m_recvSocket->SetAllowBroadcast(true);
        m_recvSocket->SetRecvCallback(MakeCallback(&NhdpClient::HandleRecv, this));
        int bindResult;
        if (m_addressMode == AddressMode::IPV4)
        {
            bindResult = m_recvSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), m_port));
        }
        else
        {
            // RFC 6130 / ns-3 specific: IPv6 multicast is only delivered locally if the group
            // is registered.  Binding the receive socket's local address to the multicast group
            // registers it (UdpSocketImpl::DoBind6 -> Ipv6L3Protocol::AddMulticastAddress).
            // Binding Ipv6Address::GetAny() would silently receive nothing.
            bindResult = m_recvSocket->Bind(Inet6SocketAddress(LL_MANET_ROUTERS_IPV6, m_port));
        }
        if (bindResult)
        {
            NS_FATAL_ERROR("Failed to bind() NHDP receive socket");
        }
        m_recvSocket->SetRecvPktInfo(true);
        m_recvSocket->ShutdownSend();
    }
    Application::DoInitialize();
}

int64_t
NhdpClient::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    auto currentStream = stream;
    m_rng->SetStream(currentStream++);
    currentStream += Application::AssignStreams(currentStream);
    return (currentStream - stream);
}

const std::list<NeighborTuple>&
NhdpClient::GetNeighborInfoBase() const
{
    return m_neighborInfoBase;
}

const std::list<LinkTuple>&
NhdpClient::GetLinkInfoBase() const
{
    return m_linkInfoBase;
}

const std::list<TwoHopTuple>&
NhdpClient::GetTwoHopInfoBase() const
{
    return m_twoHopInfoBase;
}

const std::list<LostNeighborTuple>&
NhdpClient::GetLostNeighborSet() const
{
    return m_lostNeighborSet;
}

const NeighborTuple*
NhdpClient::FindNeighborTuple(const Address& addr) const
{
    for (const auto& tuple : m_neighborInfoBase)
    {
        if (AddressInList(tuple.m_neighborAddrList, addr))
        {
            return &tuple;
        }
    }
    return nullptr;
}

const LinkTuple*
NhdpClient::FindLinkTuple(const Address& addr) const
{
    for (const auto& tuple : m_linkInfoBase)
    {
        if (AddressInList(tuple.m_neighborAddrList, addr))
        {
            return &tuple;
        }
    }
    return nullptr;
}

const LostNeighborTuple*
NhdpClient::FindLostNeighbor(const Address& addr) const
{
    for (const auto& tuple : m_lostNeighborSet)
    {
        if (tuple.m_neighborAddr == addr)
        {
            return &tuple;
        }
    }
    return nullptr;
}

const TwoHopTuple*
NhdpClient::FindTwoHopTuple(const Address& via, const Address& twoHopAddr) const
{
    for (const auto& tuple : m_twoHopInfoBase)
    {
        if (tuple.m_twoHopAddr == twoHopAddr && AddressInList(tuple.m_neighborAddrList, via))
        {
            return &tuple;
        }
    }
    return nullptr;
}

/* Address-list helpers (RFC 6130 overlap matching) */

bool
NhdpClient::AddressInList(const std::vector<Address>& list, const Address& addr)
{
    return std::find(list.begin(), list.end(), addr) != list.end();
}

bool
NhdpClient::AddressListsOverlap(const std::vector<Address>& a, const std::vector<Address>& b)
{
    for (const auto& addr : a)
    {
        if (AddressInList(b, addr))
        {
            return true;
        }
    }
    return false;
}

std::list<NeighborTuple>::iterator
NhdpClient::FindNeighborTupleIt(const Address& addr)
{
    for (auto it = m_neighborInfoBase.begin(); it != m_neighborInfoBase.end(); ++it)
    {
        if (AddressInList(it->m_neighborAddrList, addr))
        {
            return it;
        }
    }
    return m_neighborInfoBase.end();
}

std::list<LinkTuple>::iterator
NhdpClient::FindLinkTupleIt(const Address& addr)
{
    for (auto it = m_linkInfoBase.begin(); it != m_linkInfoBase.end(); ++it)
    {
        if (AddressInList(it->m_neighborAddrList, addr))
        {
            return it;
        }
    }
    return m_linkInfoBase.end();
}

std::list<LostNeighborTuple>::iterator
NhdpClient::FindLostNeighborIt(const Address& addr)
{
    for (auto it = m_lostNeighborSet.begin(); it != m_lostNeighborSet.end(); ++it)
    {
        if (it->m_neighborAddr == addr)
        {
            return it;
        }
    }
    return m_lostNeighborSet.end();
}

std::list<TwoHopTuple>::iterator
NhdpClient::FindTwoHopTupleIt(const std::vector<Address>& via, const Address& twoHopAddr)
{
    for (auto it = m_twoHopInfoBase.begin(); it != m_twoHopInfoBase.end(); ++it)
    {
        if (it->m_twoHopAddr == twoHopAddr && AddressListsOverlap(it->m_neighborAddrList, via))
        {
            return it;
        }
    }
    return m_twoHopInfoBase.end();
}

/* Local Information Base Methods */
void
NhdpClient::MarkIfaceNonManet(uint32_t ifaddr)
{
    m_nonManetSet.insert(ifaddr);
}

/* End information base methods */

void
NhdpClient::RegisterLinkQualityCallback(Callback<double, Ptr<Packet>> cb)
{
    NS_LOG_FUNCTION(this);
    m_linkQualityCallback = cb;
}

void
NhdpClient::HandleLinkFailure(const Address& neighborAddr)
{
    NS_LOG_FUNCTION(this << addressUtils::FormatAddress(neighborAddr));

    // Handle according to RFC 6130 Section 14, like a link failure due to
    // link quality falling below HYST_REJECT.  This includes setting the
    // link to lost and then following Section 13.2 logic.
    auto itLink = FindLinkTupleIt(neighborAddr);
    if (itLink == m_linkInfoBase.end())
    {
        NS_LOG_DEBUG("No link exists to neighbor " << addressUtils::FormatAddress(neighborAddr));
        return;
    }
    NS_LOG_INFO("LinkFailure to existing neighbor " << addressUtils::FormatAddress(neighborAddr)
                                                    << " reported");
    m_linkFailureTrace(neighborAddr);
    auto& linkTuple = *itLink;
    auto oldLinkTuple = linkTuple;
    auto neighborAddrs = linkTuple.m_neighborAddrList;
    // RFC 6130, Sec. 14.3, step 2
    linkTuple.m_lost = true;
    linkTuple.m_pending = true; // Implied by RFC 6130, Sec. 14.2
    linkTuple.m_heardTime = EXPIRED;
    linkTuple.m_symTime = EXPIRED;
    linkTuple.m_expirationTime =
        std::min(linkTuple.m_expirationTime, Simulator::Now() + m_lHoldTime);
    NS_LOG_INFO("LinkTuple to " << addressUtils::FormatAddress(neighborAddr)
                                << " set to LOST by link failure");
    m_linkChangeTrace(Action::CHANGED, oldLinkTuple, linkTuple);
    for (const auto& addr : neighborAddrs)
    {
        if (FindLostNeighborIt(addr) == m_lostNeighborSet.end())
        {
            NS_LOG_INFO("Inserting lost address " << addressUtils::FormatAddress(addr)
                                                  << " into lost neighbor set");
            LostNeighborTuple lostNeighborTuple;
            lostNeighborTuple.m_neighborAddr = addr;
            lostNeighborTuple.m_expirationTime = Simulator::Now() + m_nHoldTime;
            m_lostNeighborSet.push_back(lostNeighborTuple);
            m_lostNeighborChangeTrace(Action::ADDED, lostNeighborTuple, lostNeighborTuple);
        }
    }
    // Section 13.2 processing
    // The link to this neighbor is no longer symmetric, so remove its 2-Hop Tuples
    RemoveTwoHopTuples(neighborAddrs);
    // Modify the neighbor tuple to be not symmetric
    auto itNeigh = FindNeighborTupleIt(neighborAddr);
    if (itNeigh != m_neighborInfoBase.end() && itNeigh->m_symmetric)
    {
        // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
        auto oldNeighborTuple = *itNeigh;
        NS_LOG_INFO("Setting neighbor " << addressUtils::FormatAddress(neighborAddr)
                                        << " to not symmetric");
        itNeigh->m_symmetric = false;
        m_neighborChangeTrace(Action::CHANGED, oldNeighborTuple, *itNeigh);
    }
    NS_LOG_INFO("Tracing link failure to neighbor " << addressUtils::FormatAddress(neighborAddr));
}

void
NhdpClient::BypassRecv(Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this << packet);

    // Allow link quality callbacks even in bypass mode (useful for tests)
    std::optional<double> quality;
    if (!m_linkQualityCallback.IsNull())
    {
        quality = m_linkQualityCallback(packet);
        NS_ABORT_MSG_IF(quality.value() < 0 || quality.value() > 1,
                        "Link quality not between [0,1]");
    }
    PbbPacket pbb;
    packet->RemoveHeader(pbb);
    int16_t seq(-1);
    if (pbb.HasSequenceNumber())
    {
        seq = pbb.GetSequenceNumber();
    }
    NS_LOG_DEBUG("Pbb final message size " << pbb.MessageSize() << " TLV size " << pbb.TlvSize()
                                           << " seq. no. " << seq);
    // TODO:  Clarify whether a NHDP packet can have more than one message (HELLO message)
    NS_ASSERT_MSG(pbb.MessageSize() == 1, "There should be one message in a HELLO");
    // In bypass mode the IP datagram source is not available; the Sending Address List is
    // always carried explicitly in the LOCAL_IF block, so an invalid Address is sufficient.
    auto neighborAddr = HandlePbbMessage(pbb.MessageFront(), quality, Address());

    // Pass HELLO to other protocols that are clients of NHDP
    m_helloMessageRecvTrace(pbb.MessageFront(), neighborAddr, quality);
}

void
NhdpClient::HandleRecv(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    Address from;
    Ptr<Packet> packet = socket->RecvFrom(from);
    Address datagramSrc;
    if (m_addressMode == AddressMode::IPV4)
    {
        Ipv4PacketInfoTag tag;
        auto found = packet->RemovePacketTag(tag);
        NS_ASSERT_MSG(found, "Did not find Ipv4PacketInfoTag");
        datagramSrc = Address(InetSocketAddress::ConvertFrom(from).GetIpv4());
    }
    else
    {
        Ipv6PacketInfoTag tag;
        auto found = packet->RemovePacketTag(tag);
        NS_ASSERT_MSG(found, "Did not find Ipv6PacketInfoTag");
        datagramSrc = Address(Inet6SocketAddress::ConvertFrom(from).GetIpv6());
    }
    std::optional<double> quality;
    if (!m_linkQualityCallback.IsNull())
    {
        quality = m_linkQualityCallback(packet);
        NS_ABORT_MSG_IF(quality.value() < 0 || quality.value() > 1,
                        "Link quality not between [0,1]");
        NS_LOG_DEBUG("Receive HELLO with quality "
                     << quality.value() << " from: " << addressUtils::FormatAddress(datagramSrc));
    }
    else
    {
        NS_LOG_DEBUG("Receive HELLO (link quality callback disabled) from: "
                     << addressUtils::FormatAddress(datagramSrc));
    }
    PbbPacket pbb;
    packet->RemoveHeader(pbb);
    int16_t seq(-1);
    if (pbb.HasSequenceNumber())
    {
        seq = pbb.GetSequenceNumber();
    }
    NS_LOG_DEBUG("Pbb final message size " << pbb.MessageSize() << " TLV size " << pbb.TlvSize()
                                           << " seq. no. " << seq);
    // TODO:  Clarify whether a NHDP packet can have more than one message (HELLO message)
    NS_ASSERT_MSG(pbb.MessageSize() == 1, "There should be one message in a HELLO");
    auto neighborAddr = HandlePbbMessage(pbb.MessageFront(), quality, datagramSrc);

    // Pass HELLO to other protocols that are clients of NHDP
    m_helloMessageRecvTrace(pbb.MessageFront(), neighborAddr, quality);
}

Address
NhdpClient::HandlePbbMessage(Ptr<PbbMessage> msg,
                             std::optional<double> quality,
                             const Address& datagramSrc)
{
    NS_LOG_FUNCTION(this << msg << quality.has_value());
    NS_ASSERT_MSG(msg->GetType() == 0, "HELLO should be message type 0");
    NS_LOG_INFO("Received HELLO; TLVs: "
                << msg->TlvSize() << " address blocks: " << msg->AddressBlockSize()
                << " hops: " << msg->HasHopLimit() << " seq: " << msg->HasSequenceNumber());

    // Phase 0: process message TLVs (RFC 5497 INTERVAL_TIME and VALIDITY_TIME).  OLSRv2 will add
    // the MPR_WILLING TLV here.
    Time validityTime;
    uint32_t validityCount = 0;
    uint32_t intervalCount = 0;
    for (auto it = msg->TlvBegin(); it != msg->TlvEnd(); ++it)
    {
        const auto& tlv = *it;
        if (tlv->GetType() == MSG_TLV_VALIDITY_TIME)
        {
            NS_ASSERT_MSG(tlv->GetValue().GetSize() == 1,
                          "VALIDITY_TIME TLV must carry a single time-code octet");
            validityTime = DecodeTimeCode(tlv->GetValue().Begin().ReadU8());
            validityCount++;
        }
        else if (tlv->GetType() == MSG_TLV_INTERVAL_TIME)
        {
            intervalCount++;
        }
    }
    // RFC 6130, Sec. 12.1: a HELLO MUST contain exactly one VALIDITY_TIME Message TLV;
    // a HELLO with zero, or with more than one, is invalid.  RFC 5497, Sec. 7.1 likewise
    // forbids more than one INTERVAL_TIME TLV.
    NS_ASSERT_MSG(validityCount != 0,
                  "HELLO is missing the mandatory VALIDITY_TIME Message TLV (RFC 6130 Sec. 12.1)");
    NS_ASSERT_MSG(validityCount == 1,
                  "HELLO has more than one VALIDITY_TIME Message TLV (RFC 6130 Sec. 12.1)");
    NS_ASSERT_MSG(intervalCount <= 1,
                  "HELLO has more than one INTERVAL_TIME Message TLV (RFC 6130 Sec. 12.1)");
    NS_LOG_DEBUG("HELLO validity time " << validityTime.As(Time::S));

    // Phase 1: collect the address lists (RFC 6130, Sec. 12.2)
    std::vector<Address> sendingList;
    std::vector<Address> neighborList;
    std::vector<std::pair<Address, AddressTlvLinkStatus>> linkStatus;
    CollectAddressLists(msg, datagramSrc, sendingList, neighborList, linkStatus);

    // Phase 2: apply Information Base updates in RFC order
    std::vector<Address> removedList;
    std::vector<Address> lostList;
    UpdateNeighborSet(neighborList, removedList, lostList);                     // Sec. 12.3
    UpdateLostNeighborSet(lostList);                                            // Sec. 12.4
    UpdateLinkSet(sendingList, linkStatus, validityTime, quality, removedList); // Sec. 12.5
    UpdateTwoHopSet(sendingList, linkStatus, validityTime);                     // Sec. 12.6

    Address neighborAddr = sendingList.empty() ? datagramSrc : sendingList.front();
    m_helloRecvTrace(neighborAddr, linkStatus, quality);

    // Remove expired two hop entries
    RemoveExpiredTwoHopNeighbors();

    return neighborAddr;
}

namespace
{
/**
 * Expand the per-address TLV values of an address block into a flat vector.
 *
 * Address Block TLVs may carry one value for the whole block or a value per
 * index range; this reconstructs the value associated with each address.
 *
 * @param block The address block.
 * @return A vector with one value per address, in address order.
 */
std::vector<uint8_t>
ExpandTlvValues(Ptr<PbbAddressBlock> block)
{
    std::vector<uint8_t> values;
    auto numAddresses = static_cast<int>(block->AddressSize());
    int remaining = numAddresses;
    for (auto it = block->TlvBegin(); it != block->TlvEnd(); ++it)
    {
        const auto& tlv = *it;
        NS_ASSERT_MSG(tlv->GetValue().GetSize() == 1, "TLV should have one byte of value data");
        auto value = tlv->GetValue().Begin().ReadU8();
        int count =
            tlv->HasIndexStop() ? (tlv->GetIndexStop() - tlv->GetIndexStart() + 1) : remaining;
        for (int i = 0; i < count; i++)
        {
            values.push_back(value);
            remaining--;
        }
    }
    NS_ASSERT_MSG(values.size() == static_cast<std::size_t>(numAddresses),
                  "Logic error expanding address block TLV values");
    return values;
}
} // namespace

void
NhdpClient::CollectAddressLists(Ptr<PbbMessage> msg,
                                const Address& datagramSrc,
                                std::vector<Address>& sendingList,
                                std::vector<Address>& neighborList,
                                std::vector<std::pair<Address, AddressTlvLinkStatus>>& linkStatus)
{
    NS_LOG_FUNCTION(this);
    // OLSRv2 will add the MPR, LINK_METRIC, and NBR_ADDR_TYPE address blocks
    for (auto it = msg->AddressBlockBegin(); it != msg->AddressBlockEnd(); ++it)
    {
        auto addressBlock = *it;
        if (addressBlock->TlvSize() == 0)
        {
            NS_LOG_DEBUG("Skipping address block with no TLVs");
            continue;
        }
        auto type = addressBlock->TlvFront()->GetType();
        NS_LOG_INFO("Address block size " << addressBlock->AddressSize() << " TLV type " << +type);
        if (type == ADDR_TLV_LOCAL_IF)
        {
            auto values = ExpandTlvValues(addressBlock);
            std::size_t k = 0;
            for (auto a = addressBlock->AddressBegin(); a != addressBlock->AddressEnd(); ++a, ++k)
            {
                Address addr = *a;
                // RFC 6130, Sec. 12.2: Neighbor Address List is THIS_IF + OTHER_IF
                neighborList.push_back(addr);
                // Sending Address List is THIS_IF only
                if (values[k] == ADDR_TLV_LOCAL_IF_THIS_IF)
                {
                    sendingList.push_back(addr);
                }
            }
        }
        else if (type == ADDR_TLV_LINK_STATUS)
        {
            auto values = ExpandTlvValues(addressBlock);
            std::size_t k = 0;
            for (auto a = addressBlock->AddressBegin(); a != addressBlock->AddressEnd(); ++a, ++k)
            {
                NS_ASSERT_MSG(values[k] < 3, "LINK_STATUS value " << +values[k] << " unsupported");
                linkStatus.emplace_back(*a, static_cast<AddressTlvLinkStatus>(values[k]));
            }
        }
        else if (type == ADDR_TLV_OTHER_NEIGHB)
        {
            // OTHER_NEIGHB processing is not yet supported (RFC 6130, Sec. 12.6 via OTHER_NEIGHB)
            NS_LOG_WARN("OTHER_NEIGHB address block not yet supported; ignoring");
        }
        else
        {
            NS_LOG_DEBUG("Unknown address block TLV type " << +type);
        }
    }
    // RFC 6130, Sec. 12.2: if the Sending Address List is otherwise empty, it contains the
    // sending address of the IP datagram in which the HELLO was included.
    if (sendingList.empty() && !datagramSrc.IsInvalid())
    {
        NS_LOG_DEBUG("Empty Sending Address List; defaulting to datagram source "
                     << addressUtils::FormatAddress(datagramSrc));
        sendingList.push_back(datagramSrc);
        if (!AddressInList(neighborList, datagramSrc))
        {
            neighborList.push_back(datagramSrc);
        }
    }
}

void
NhdpClient::UpdateNeighborSet(const std::vector<Address>& neighborList,
                              std::vector<Address>& removedList,
                              std::vector<Address>& lostList)
{
    NS_LOG_FUNCTION(this);
    if (neighborList.empty())
    {
        return;
    }
    // RFC 6130, Sec. 12.3, step 1: find all matching Neighbor Tuples (overlap)
    std::vector<std::list<NeighborTuple>::iterator> matching;
    for (auto it = m_neighborInfoBase.begin(); it != m_neighborInfoBase.end(); ++it)
    {
        if (AddressListsOverlap(it->m_neighborAddrList, neighborList))
        {
            matching.push_back(it);
        }
    }

    auto dropLostNeighborEntries = [this](const std::vector<Address>& addrs) {
        for (const auto& addr : addrs)
        {
            auto itLost = FindLostNeighborIt(addr);
            if (itLost != m_lostNeighborSet.end())
            {
                NS_LOG_INFO("Removing " << addressUtils::FormatAddress(addr)
                                        << " from lost neighbor set");
                m_lostNeighborChangeTrace(Action::REMOVED, *itLost, *itLost);
                m_lostNeighborSet.erase(itLost);
            }
        }
    };

    if (matching.empty())
    {
        // RFC 6130, Sec. 12.3, step 2: create a new Neighbor Tuple
        NS_LOG_INFO("Creating new NeighborTuple for "
                    << addressUtils::FormatAddress(neighborList.front()));
        NeighborTuple neighborTuple(neighborList);
        neighborTuple.m_symmetric = false;
        m_neighborInfoBase.push_back(neighborTuple);
        m_neighborChangeTrace(Action::ADDED, neighborTuple, neighborTuple);
        dropLostNeighborEntries(neighborList);
        return;
    }
    if (matching.size() == 1)
    {
        // RFC 6130, Sec. 12.3, step 3
        auto it = matching.front();
        bool changed = false;
        for (const auto& addr : it->m_neighborAddrList)
        {
            if (!AddressInList(neighborList, addr))
            {
                // Sec. 12.3, step 3.1.1: removed address
                removedList.push_back(addr);
                if (it->m_symmetric)
                {
                    lostList.push_back(addr);
                }
                changed = true;
            }
        }
        for (const auto& addr : neighborList)
        {
            if (!AddressInList(it->m_neighborAddrList, addr))
            {
                changed = true;
            }
        }
        if (changed)
        {
            // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
            auto oldNeighborTuple = *it;
            it->m_neighborAddrList = neighborList;
            m_neighborChangeTrace(Action::CHANGED, oldNeighborTuple, *it);
        }
        return;
    }
    // RFC 6130, Sec. 12.3, step 4: two or more matching Neighbor Tuples -> merge
    NS_LOG_INFO("Merging " << matching.size() << " Neighbor Tuples (RFC 6130 Sec. 12.3 step 4)");
    for (auto& it : matching)
    {
        for (const auto& addr : it->m_neighborAddrList)
        {
            if (!AddressInList(neighborList, addr))
            {
                removedList.push_back(addr);
                if (it->m_symmetric)
                {
                    lostList.push_back(addr);
                }
            }
        }
        m_neighborChangeTrace(Action::REMOVED, *it, *it);
        m_neighborInfoBase.erase(it);
    }
    NeighborTuple merged(neighborList);
    merged.m_symmetric = false; // RFC 6130, Sec. 12.3, step 4.2
    m_neighborInfoBase.push_back(merged);
    m_neighborChangeTrace(Action::ADDED, merged, merged);
    dropLostNeighborEntries(neighborList);
}

void
NhdpClient::UpdateLostNeighborSet(const std::vector<Address>& lostList)
{
    NS_LOG_FUNCTION(this);
    // RFC 6130, Sec. 12.4
    for (const auto& addr : lostList)
    {
        if (FindLostNeighborIt(addr) == m_lostNeighborSet.end())
        {
            NS_LOG_INFO("Adding lost address " << addressUtils::FormatAddress(addr)
                                               << " to lost neighbor set");
            LostNeighborTuple lostNeighborTuple;
            lostNeighborTuple.m_neighborAddr = addr;
            lostNeighborTuple.m_expirationTime = Simulator::Now() + m_nHoldTime;
            m_lostNeighborSet.push_back(lostNeighborTuple);
            m_lostNeighborChangeTrace(Action::ADDED, lostNeighborTuple, lostNeighborTuple);
        }
    }
}

void
NhdpClient::UpdateLinkSet(const std::vector<Address>& sendingList,
                          const std::vector<std::pair<Address, AddressTlvLinkStatus>>& linkStatus,
                          Time validityTime,
                          std::optional<double> quality,
                          const std::vector<Address>& removedList)
{
    NS_LOG_FUNCTION(this);
    if (sendingList.empty())
    {
        NS_LOG_DEBUG("No Sending Address List; skipping Link Set update");
        return;
    }

    // RFC 6130, Sec. 12.5, steps 1-2: remove Removed Address List addresses from all Link
    // Tuples and remove any Link Tuple whose address list becomes empty (apply Sec. 13.2,
    // not Sec. 13.3).
    if (!removedList.empty())
    {
        for (auto itLink = m_linkInfoBase.begin(); itLink != m_linkInfoBase.end();)
        {
            auto original = itLink->m_neighborAddrList;
            std::vector<Address> surviving;
            for (const auto& addr : original)
            {
                if (!AddressInList(removedList, addr))
                {
                    surviving.push_back(addr);
                }
            }
            if (surviving.empty() && !original.empty())
            {
                NS_LOG_INFO("Removing Link Tuple emptied by Removed Address List");
                m_linkChangeTrace(Action::REMOVED, *itLink, *itLink);
                itLink = m_linkInfoBase.erase(itLink);
                RemoveTwoHopTuples(original); // Sec. 13.2
                continue;
            }
            itLink->m_neighborAddrList = surviving;
            ++itLink;
        }
    }

    // RFC 6130, Sec. 12.5: find matching Link Tuples (overlap with Sending Address List)
    std::vector<std::list<LinkTuple>::iterator> matching;
    for (auto it = m_linkInfoBase.begin(); it != m_linkInfoBase.end(); ++it)
    {
        if (AddressListsOverlap(it->m_neighborAddrList, sendingList))
        {
            matching.push_back(it);
        }
    }
    // step 2: if more than one matching Link Tuple, remove them all (apply Sec. 13.2 not 13.3)
    if (matching.size() > 1)
    {
        NS_LOG_INFO("Removing " << matching.size() << " ambiguous matching Link Tuples");
        for (auto& it : matching)
        {
            auto addrs = it->m_neighborAddrList;
            m_linkChangeTrace(Action::REMOVED, *it, *it);
            m_linkInfoBase.erase(it);
            RemoveTwoHopTuples(addrs);
        }
        matching.clear();
    }

    std::list<LinkTuple>::iterator linkIt;
    bool created = false;
    if (matching.empty())
    {
        // RFC 6130, Sec. 12.5, step 3: create a new Link Tuple
        LinkTuple linkTuple;
        linkTuple.m_heardTime = EXPIRED;
        linkTuple.m_symTime = EXPIRED;
        linkTuple.m_pending = m_initialPending;
        linkTuple.m_lost = false;
        // See Sec. 14.3, last sentence, on initializing link quality if supported
        linkTuple.m_quality = quality.has_value() ? quality.value() : m_initialQuality;
        if (m_initialPending && linkTuple.m_quality >= m_hystAccept)
        {
            linkTuple.m_pending = false;
        }
        linkTuple.m_expirationTime = Simulator::Now() + validityTime;
        m_linkInfoBase.push_back(linkTuple);
        linkIt = std::prev(m_linkInfoBase.end());
        created = true;
    }
    else
    {
        linkIt = matching.front();
    }

    auto& linkTuple = *linkIt;
    auto oldLinkTuple = linkTuple;

    // Determine whether this HELLO lists any of our local (receiving) addresses, and how.
    // RFC 6130, Sec. 12.5, step 4.1: HEARD/SYMMETRIC takes precedence over LOST.
    bool selfHeardOrSym = false;
    bool selfLost = false;
    for (const auto& [addr, status] : linkStatus)
    {
        if (AddressInList(m_localAddresses, addr))
        {
            if (status == AddressTlvLinkStatus::HEARD || status == AddressTlvLinkStatus::SYMMETRIC)
            {
                selfHeardOrSym = true;
            }
            else if (status == AddressTlvLinkStatus::LOST)
            {
                selfLost = true;
            }
        }
    }

    auto itNeigh = FindNeighborTupleIt(sendingList.front());

    // RFC 6130, Sec. 12.5, step 4.2: L_neighbor_iface_addr_list := Sending Address List
    linkTuple.m_neighborAddrList = sendingList;
    // Receiving a HELLO refreshes L_HEARD_time (the sender is heard).  This is the baseline
    // also set when a HELLO's LOCAL_IF block is processed; the symmetric/lost L_time
    // extensions below only apply when the sender lists one of our addresses.
    linkTuple.m_heardTime = Simulator::Now() + validityTime;
    if (linkTuple.m_pending || created)
    {
        linkTuple.m_expirationTime = linkTuple.m_heardTime;
    }

    auto qualityValue = quality.has_value() ? quality.value() : 1.0;
    if (selfHeardOrSym || selfLost)
    {
        // RFC 6130, Sec. 14.3.  Link-quality accept/reject is evaluated when the sender lists
        // one of our addresses.  Link quality in use corresponds to m_initialPending == true.
        if (m_initialPending && quality.has_value())
        {
            const bool changeToAccept =
                (linkTuple.m_pending || linkTuple.m_lost) && qualityValue >= m_hystAccept;
            const bool changeToReject =
                (!linkTuple.m_pending && !linkTuple.m_lost) && qualityValue < m_hystReject;
            linkTuple.m_quality = qualityValue;
            // RFC 6130, Sec. 14.3, step 1
            if (changeToAccept)
            {
                linkTuple.m_pending = false;
                linkTuple.m_lost = false;
                if (selfHeardOrSym)
                {
                    linkTuple.m_expirationTime =
                        std::max(linkTuple.m_expirationTime, linkTuple.m_heardTime + m_lHoldTime);
                }
                NS_LOG_INFO("LinkTuple to " << addressUtils::FormatAddress(sendingList.front())
                                            << " moved above HYST_ACCEPT");
                for (const auto& addr : sendingList)
                {
                    auto itLost = FindLostNeighborIt(addr);
                    if (itLost != m_lostNeighborSet.end())
                    {
                        m_lostNeighborChangeTrace(Action::REMOVED, *itLost, *itLost);
                        m_lostNeighborSet.erase(itLost);
                    }
                }
            }
            // RFC 6130, Sec. 14.3, step 2
            if (changeToReject)
            {
                linkTuple.m_lost = true;
                linkTuple.m_pending = true; // Implied by RFC 6130, Sec. 14.2
                linkTuple.m_heardTime = EXPIRED;
                linkTuple.m_symTime = EXPIRED;
                linkTuple.m_expirationTime =
                    std::min(linkTuple.m_expirationTime, Simulator::Now() + m_lHoldTime);
                NS_LOG_INFO("LinkTuple to " << addressUtils::FormatAddress(sendingList.front())
                                            << " moved below HYST_REJECT");
                for (const auto& addr : sendingList)
                {
                    if (FindLostNeighborIt(addr) == m_lostNeighborSet.end())
                    {
                        LostNeighborTuple lostNeighborTuple;
                        lostNeighborTuple.m_neighborAddr = addr;
                        lostNeighborTuple.m_expirationTime = Simulator::Now() + m_nHoldTime;
                        m_lostNeighborSet.push_back(lostNeighborTuple);
                        m_lostNeighborChangeTrace(Action::ADDED,
                                                  lostNeighborTuple,
                                                  lostNeighborTuple);
                    }
                }
            }
        }

        if (selfHeardOrSym)
        {
            // RFC 6130, Sec. 12.5, step 4.1.1
            linkTuple.m_symTime = Simulator::Now() + validityTime;
            // step 4.3
            linkTuple.m_heardTime = std::max(Simulator::Now() + validityTime, linkTuple.m_symTime);
            // steps 4.4/4.5
            if (linkTuple.m_pending)
            {
                linkTuple.m_expirationTime =
                    std::max(linkTuple.m_expirationTime, linkTuple.m_heardTime);
            }
            else
            {
                // L_HOLD_TIME is the receiver's local parameter, not the sender's validity time
                linkTuple.m_expirationTime =
                    std::max(linkTuple.m_expirationTime, linkTuple.m_heardTime + m_lHoldTime);
            }
            // RFC 6130, Sec. 13.1, step 1: promote neighbor to symmetric
            if (linkTuple.GetLinkStatus() == LinkStatus::SYMMETRIC &&
                itNeigh != m_neighborInfoBase.end() && !itNeigh->m_symmetric)
            {
                // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
                auto oldNeighborTuple = *itNeigh;
                NS_LOG_DEBUG("Promoting neighbor "
                             << addressUtils::FormatAddress(sendingList.front())
                             << " to SYMMETRIC");
                itNeigh->m_symmetric = true;
                m_neighborChangeTrace(Action::CHANGED, oldNeighborTuple, *itNeigh);
            }
        }
        else // selfLost
        {
            NS_LOG_INFO("HELLO from " << addressUtils::FormatAddress(sendingList.front())
                                      << " considers me as LOST");
            // RFC 6130, Sec. 12.5, step 4.1.2.1: L_SYM_time := EXPIRED
            linkTuple.m_symTime = EXPIRED;
            // step 4.3 (L_SYM_time is EXPIRED, so L_HEARD_time := now + validity)
            linkTuple.m_heardTime = Simulator::Now() + validityTime;
            // steps 4.4/4.5: the link is still HEARD; extend by the receiver's local L_HOLD_TIME
            if (linkTuple.m_pending)
            {
                linkTuple.m_expirationTime =
                    std::max(linkTuple.m_expirationTime, linkTuple.m_heardTime);
            }
            else
            {
                linkTuple.m_expirationTime =
                    std::max(linkTuple.m_expirationTime, linkTuple.m_heardTime + m_lHoldTime);
            }
            // The neighbor no longer considers the link symmetric (RFC 6130, Sec. 13.2 trigger)
            if (itNeigh != m_neighborInfoBase.end() && itNeigh->m_symmetric)
            {
                // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
                auto oldNeighborTuple = *itNeigh;
                itNeigh->m_symmetric = false;
                m_neighborChangeTrace(Action::CHANGED, oldNeighborTuple, *itNeigh);
                RemoveTwoHopTuples(sendingList);
            }
        }
    }

    if (created)
    {
        NS_LOG_INFO("Added new LinkTuple to " << addressUtils::FormatAddress(sendingList.front())
                                              << " with quality " << linkTuple.m_quality);
        m_linkChangeTrace(Action::ADDED, linkTuple, linkTuple);
        for (const auto& addr : sendingList)
        {
            auto itLost = FindLostNeighborIt(addr);
            if (itLost != m_lostNeighborSet.end())
            {
                m_lostNeighborChangeTrace(Action::REMOVED, *itLost, *itLost);
                m_lostNeighborSet.erase(itLost);
            }
        }
    }
    else
    {
        m_linkChangeTrace(Action::CHANGED, oldLinkTuple, linkTuple);
    }
}

void
NhdpClient::UpdateTwoHopSet(const std::vector<Address>& sendingList,
                            const std::vector<std::pair<Address, AddressTlvLinkStatus>>& linkStatus,
                            Time validityTime)
{
    NS_LOG_FUNCTION(this);
    if (sendingList.empty())
    {
        return;
    }
    // RFC 6130, Sec. 12.6, step 2: gate on the Link Tuple to the sender being SYMMETRIC.
    auto itLink = FindLinkTupleIt(sendingList.front());
    bool linkSymmetric =
        (itLink != m_linkInfoBase.end()) && (itLink->GetLinkStatus() == LinkStatus::SYMMETRIC);

    for (const auto& [addr, status] : linkStatus)
    {
        if (AddressInList(m_localAddresses, addr))
        {
            // This is one of our own addresses; handled by the Link Set update.
            continue;
        }
        if (status == AddressTlvLinkStatus::SYMMETRIC && linkSymmetric)
        {
            auto it = FindTwoHopTupleIt(sendingList, addr);
            if (it == m_twoHopInfoBase.end())
            {
                TwoHopTuple twoHopTuple(sendingList, addr);
                // RFC 6130, Sec. 12.6: N2_time := current time + validity time
                twoHopTuple.m_expirationTime = Simulator::Now() + validityTime;
                m_twoHopInfoBase.push_back(twoHopTuple);
                m_twoHopChangeTrace(Action::ADDED, twoHopTuple, twoHopTuple);
                NS_LOG_INFO("Creating new TwoHopTuple to "
                            << addressUtils::FormatAddress(addr) << " via "
                            << addressUtils::FormatAddress(sendingList.front()));
            }
            else
            {
                NS_LOG_DEBUG("Updating TwoHopTuple to "
                             << addressUtils::FormatAddress(addr) << " via "
                             << addressUtils::FormatAddress(sendingList.front()));
                it->m_neighborAddrList = sendingList;
                it->m_expirationTime = Simulator::Now() + validityTime;
            }
        }
        else
        {
            // HEARD or LOST: remove any 2-Hop Tuple to this address via the sender
            auto it = FindTwoHopTupleIt(sendingList, addr);
            if (it != m_twoHopInfoBase.end())
            {
                NS_LOG_INFO("Removing TwoHopTuple to "
                            << addressUtils::FormatAddress(addr) << " via "
                            << addressUtils::FormatAddress(sendingList.front()));
                m_twoHopChangeTrace(Action::REMOVED, *it, *it);
                m_twoHopInfoBase.erase(it);
            }
        }
    }
}

void
NhdpClient::HandleDirectLinkEstablishedTrace(uint32_t srcL2Id,
                                             Address selfAddr,
                                             uint32_t peerL2Id,
                                             Address peerAddr)
{
    NS_LOG_FUNCTION(this << srcL2Id << addressUtils::FormatAddress(selfAddr) << peerL2Id
                         << addressUtils::FormatAddress(peerAddr));
    auto node = NodeList::GetNode(peerL2Id);
    bool found = false;
    for (uint32_t i = 0; i < node->GetNApplications(); i++)
    {
        auto peerClient = node->GetApplication(i)->GetObject<NhdpClient>();
        if (peerClient)
        {
            auto [it, inserted] = m_establishedPeers.insert_or_assign(peerAddr, peerClient);
            if (inserted)
            {
                NS_LOG_INFO("Inserted NhdpClient pointer for " << peerL2Id << " to map");
            }
            else
            {
                NS_LOG_INFO("Assigned (overwrote) NhdpClient pointer for " << peerL2Id
                                                                           << " to map");
            }
            found = true;
        }
    }
    NS_ASSERT_MSG(found, "NhdpClient pointer not found for node " << peerL2Id);
}

void
NhdpClient::HandleDirectLinkReleasingTrace(uint32_t srcL2Id,
                                           Address selfAddr,
                                           uint32_t peerL2Id,
                                           Address peerAddr)
{
    NS_LOG_FUNCTION(this << srcL2Id << addressUtils::FormatAddress(selfAddr) << peerL2Id
                         << addressUtils::FormatAddress(peerAddr));
    auto it = m_establishedPeers.find(peerAddr);
    if (it != m_establishedPeers.end())
    {
        NS_LOG_INFO("Erasing NhdpClient pointer for " << peerL2Id << " from map");
        m_establishedPeers.erase(it);
    }
    else
    {
        NS_LOG_DEBUG("NhdpClient pointer for " << peerL2Id << " not found in map");
    }
}

void
NhdpClient::DoDispose()
{
    NS_LOG_FUNCTION(this);
    if (m_recvSocket)
    {
        m_recvSocket->Close();
        m_recvSocket = nullptr;
    }
    for (auto& [socket, addr] : m_socketAddresses)
    {
        socket->Close();
    }
    m_socketAddresses.clear();
    Application::DoDispose();
}

void
NhdpClient::StartApplication()
{
    NS_LOG_FUNCTION(this);

    if (m_addressMode == AddressMode::IPV4)
    {
        Ptr<Ipv4> ipv4 = GetNode()->GetObject<Ipv4>();
        NS_ASSERT(ipv4);

        for (uint32_t i = 0; i < ipv4->GetNInterfaces(); i++)
        {
            if (m_nonManetSet.find(i) != m_nonManetSet.end())
            {
                continue;
            }

            Ipv4Address address = ipv4->GetAddress(i, 0).GetLocal();

            if (address == Ipv4Address::GetLoopback())
            {
                continue;
            }

            m_localAddresses.clear();
            m_localAddresses.push_back(Address(address));
            m_datagramSource = Address(address);

            Ptr<Socket> socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
            socket->SetAllowBroadcast(true);
            socket->Bind(InetSocketAddress(address, m_port));
            socket->Connect(InetSocketAddress(m_address, m_port));

            m_socketAddresses[socket] = Address(address);
            ScheduleHello(socket);
        }
    }
    else
    {
        Ptr<Ipv6> ipv6 = GetNode()->GetObject<Ipv6>();
        NS_ASSERT(ipv6);

        for (uint32_t i = 0; i < ipv6->GetNInterfaces(); i++)
        {
            if (m_nonManetSet.find(i) != m_nonManetSet.end())
            {
                continue;
            }

            Address linkLocal;
            bool haveLinkLocal = false;
            std::vector<Address> ifaceAddrs;
            for (uint32_t j = 0; j < ipv6->GetNAddresses(i); j++)
            {
                Ipv6InterfaceAddress ifaceAddr = ipv6->GetAddress(i, j);
                Ipv6Address addr = ifaceAddr.GetAddress();
                if (addr == Ipv6Address::GetLoopback())
                {
                    continue;
                }
                if (ifaceAddr.GetScope() == Ipv6InterfaceAddress::LINKLOCAL)
                {
                    linkLocal = Address(addr);
                    haveLinkLocal = true;
                    // The link-local is the datagram source; list it first.
                    ifaceAddrs.insert(ifaceAddrs.begin(), Address(addr));
                }
                else
                {
                    // Unique Local (RFC 4193) or global address used for routing.
                    ifaceAddrs.push_back(Address(addr));
                }
            }
            if (!haveLinkLocal)
            {
                NS_LOG_DEBUG("Skipping IPv6 interface " << i << " with no link-local address");
                continue;
            }

            m_localAddresses = ifaceAddrs;
            m_datagramSource = linkLocal;

            Ptr<Socket> socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
            socket->SetAllowBroadcast(true);
            socket->Bind(Inet6SocketAddress(Ipv6Address::ConvertFrom(linkLocal), m_port));
            socket->Connect(Inet6SocketAddress(LL_MANET_ROUTERS_IPV6, m_port));

            m_socketAddresses[socket] = linkLocal;
            ScheduleHello(socket);
        }
    }
    m_running = true;
}

void
NhdpClient::StopApplication()
{
    NS_LOG_FUNCTION(this);

    for (auto& [socket, address] : m_socketAddresses)
    {
        socket->Close();
        socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }
    m_socketAddresses.clear();
    m_running = false;
}

void
NhdpClient::ScheduleHello(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    /* TODO: Should be able to store a helloInterval on a per-device basis */
    Time time = m_helloInterval - Seconds(m_rng->GetValue());
    Simulator::Schedule(time, &NhdpClient::SendHello, this, socket);
}

void
NhdpClient::SendHello(Ptr<Socket> socket)
{
    if (!m_running)
    {
        return;
    }

    NS_LOG_FUNCTION(this << addressUtils::FormatAddress(m_socketAddresses[socket]));

    // Perform housekeeping of items that will be used to build the HELLO
    // We do not explicitly run timers for these expirations but instead lazily
    // process them before they are needed
    UpdateLinkTuples();

    // Remove expired Lost Neighbors
    for (auto it = m_lostNeighborSet.begin(); it != m_lostNeighborSet.end();)
    {
        if (it->m_expirationTime <= Simulator::Now())
        {
            NS_LOG_DEBUG("Erasing expired lost neighbor set entry for "
                         << addressUtils::FormatAddress(it->m_neighborAddr));
            m_lostNeighborChangeTrace(Action::REMOVED, *it, *it);
            it = m_lostNeighborSet.erase(it);
        }
        else
        {
            ++it;
        }
    }

    PbbPacket pbb;
    Ptr<PbbMessage> message;
    if (m_addressMode == AddressMode::IPV4)
    {
        message = Create<PbbMessageIpv4>();
    }
    else
    {
        message = Create<PbbMessageIpv6>();
    }
    message->SetType(MESSAGE_TYPE_HELLO);
    pbb.MessagePushBack(message);

    // Add Message TLVs.  Other protocols (e.g., OLSRv2) will add message TLVs (e.g.,
    // MPR_WILLING).
    //
    // RFC 6130 Sec. 11.1: a HELLO MUST include exactly one VALIDITY_TIME Message TLV (the
    // advertised validity, i.e. H_HOLD_TIME), and a periodically generated HELLO SHOULD
    // include exactly one INTERVAL_TIME Message TLV (the current HELLO_INTERVAL).  Both are
    // single-value RFC 5497 Time TLVs carrying one time-code octet.
    Ptr<PbbTlv> validityTlv = Create<PbbTlv>();
    validityTlv->SetType(MSG_TLV_VALIDITY_TIME);
    uint8_t validityCode = EncodeTimeCode(m_hHoldTime);
    validityTlv->SetValue(&validityCode, 1);
    message->TlvPushBack(validityTlv);

    Ptr<PbbTlv> intervalTlv = Create<PbbTlv>();
    intervalTlv->SetType(MSG_TLV_INTERVAL_TIME);
    uint8_t intervalCode = EncodeTimeCode(m_helloInterval);
    intervalTlv->SetValue(&intervalCode, 1);
    message->TlvPushBack(intervalTlv);

    // Next, add Address Blocks.  Other protocols (e.g., OLSRv2) will add address
    // blocks here (e.g., LINK_METRIC, MPR, NBR_ADDR_TYPE).

    if (!m_localAddrBlock)
    {
        m_localAddrBlock = BuildLocalAddressBlock(socket);
    }
    NS_ASSERT_MSG(m_localAddrBlock, "Local address block is required");
    message->AddressBlockPushBack(m_localAddrBlock);

    auto addrBlock = BuildLinkStatusAddressBlock(socket);
    if (addrBlock)
    {
        NS_LOG_DEBUG("Adding LinkStatus Address Block");
        message->AddressBlockPushBack(addrBlock);
    }
    else
    {
        NS_LOG_DEBUG("Not sending an empty LinkStatus address block");
    }

    // Give access to the PbbMessage to other protocols that may wish to extend it

    auto addressBlockSize [[maybe_unused]] = message->AddressBlockSize();
    m_helloMessageSendTrace(m_socketAddresses[socket], message);
    if (message->AddressBlockSize() > addressBlockSize)
    {
        NS_LOG_DEBUG("Other protocols added " << message->AddressBlockSize() - addressBlockSize
                                              << " address blocks to HELLO");
    }

    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(pbb);

    HelloTag tag;
    packet->AddByteTag(tag);

    NS_LOG_INFO("Send HELLO from " << addressUtils::FormatAddress(m_socketAddresses[socket]));
    m_txTrace(packet);
    if (!m_bypassMode)
    {
        socket->Send(packet);
    }
    else
    {
        for (const auto& [peerAddr, peerClient] : m_establishedPeers)
        {
            NS_LOG_INFO("Bypassing channel to send HELLO directly to " << peerAddr);
            Simulator::ScheduleWithContext(peerClient->GetNode()->GetId(),
                                           Seconds(0),
                                           &manet::NhdpClient::BypassRecv,
                                           peerClient,
                                           packet->Copy());
        }
    }
    ScheduleHello(socket);
    // Rather than run a separate timer process to periodically check this, append it here
    RemoveExpiredTwoHopNeighbors();
}

void
NhdpClient::RemoveExpiredTwoHopNeighbors()
{
    NS_LOG_FUNCTION(this);
    for (auto it = m_twoHopInfoBase.begin(); it != m_twoHopInfoBase.end();)
    {
        if (it->m_expirationTime <= Simulator::Now())
        {
            NS_LOG_INFO("Removing TwoHopTuple to "
                        << addressUtils::FormatAddress(it->m_twoHopAddr) << " via "
                        << addressUtils::FormatAddress(it->m_neighborAddrList.front()));
            m_twoHopChangeTrace(Action::REMOVED, *it, *it);
            it = m_twoHopInfoBase.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void
NhdpClient::RemoveTwoHopTuples(const std::vector<Address>& neighborAddrs)
{
    NS_LOG_FUNCTION(this);
    for (auto it = m_twoHopInfoBase.begin(); it != m_twoHopInfoBase.end();)
    {
        if (AddressListsOverlap(it->m_neighborAddrList, neighborAddrs))
        {
            NS_LOG_INFO("Removing TwoHopTuple to "
                        << addressUtils::FormatAddress(it->m_twoHopAddr) << " via "
                        << addressUtils::FormatAddress(it->m_neighborAddrList.front()));
            m_twoHopChangeTrace(Action::REMOVED, *it, *it);
            it = m_twoHopInfoBase.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

// Check for expirations
void
NhdpClient::UpdateLinkTuples()
{
    NS_LOG_FUNCTION(this);
    for (auto itLink = m_linkInfoBase.begin(); itLink != m_linkInfoBase.end();)
    {
        auto oldLinkTuple = *itLink;
        auto neighborAddrs = itLink->m_neighborAddrList;
        // Check if expiration time (L_time) has been reached
        if (itLink->m_expirationTime <= Simulator::Now())
        {
            NS_LOG_DEBUG("Removing expired LinkTuple to neighbor "
                         << addressUtils::FormatAddress(neighborAddrs.front()));
            m_linkChangeTrace(Action::REMOVED, *itLink, *itLink);
            itLink = m_linkInfoBase.erase(itLink);
            // RFC 6130, Sec. 13.2: remove 2-Hop Tuples via this neighbor
            RemoveTwoHopTuples(neighborAddrs);
            // RFC 6130, Sec. 13.3: remove the corresponding Neighbor Tuple
            auto itNeigh = FindNeighborTupleIt(neighborAddrs.front());
            if (itNeigh != m_neighborInfoBase.end())
            {
                NS_LOG_INFO("Erasing neighbor "
                            << addressUtils::FormatAddress(neighborAddrs.front()));
                m_neighborChangeTrace(Action::REMOVED, *itNeigh, *itNeigh);
                m_neighborInfoBase.erase(itNeigh);
            }
            continue;
        }
        // Check if sym time and heard time both expired
        if (itLink->m_symTime <= Simulator::Now() && itLink->m_heardTime <= Simulator::Now())
        {
            if (!itLink->m_lost)
            {
                // Record the (formerly heard/symmetric) neighbor as lost
                for (const auto& addr : neighborAddrs)
                {
                    if (FindLostNeighborIt(addr) == m_lostNeighborSet.end())
                    {
                        LostNeighborTuple lostNeighborTuple;
                        lostNeighborTuple.m_neighborAddr = addr;
                        lostNeighborTuple.m_expirationTime = Simulator::Now() + m_nHoldTime;
                        NS_LOG_INFO("Adding LostNeighborTuple to "
                                    << addressUtils::FormatAddress(addr) << " with expiration time "
                                    << lostNeighborTuple.m_expirationTime.As(Time::S));
                        m_lostNeighborSet.push_back(lostNeighborTuple);
                        m_lostNeighborChangeTrace(Action::ADDED,
                                                  lostNeighborTuple,
                                                  lostNeighborTuple);
                    }
                }
                itLink->m_heardTime = EXPIRED;
                itLink->m_symTime = EXPIRED;
                NS_LOG_INFO("Setting link to " << addressUtils::FormatAddress(neighborAddrs.front())
                                               << " to " << itLink->GetLinkStatus());
                m_linkChangeTrace(Action::CHANGED, oldLinkTuple, *itLink);
                // RFC 6130, Sec. 13.2 + 13.3: remove the link, its 2-Hop Tuples, and the neighbor
                itLink = m_linkInfoBase.erase(itLink);
                RemoveTwoHopTuples(neighborAddrs);
                auto itNeigh = FindNeighborTupleIt(neighborAddrs.front());
                if (itNeigh != m_neighborInfoBase.end())
                {
                    NS_LOG_INFO("Erasing neighbor "
                                << addressUtils::FormatAddress(neighborAddrs.front()));
                    m_neighborChangeTrace(Action::REMOVED, *itNeigh, *itNeigh);
                    m_neighborInfoBase.erase(itNeigh);
                }
                continue;
            }
            // A lost link lingers until L_time (handled by the expiration branch above).
        }
        else if (itLink->m_symTime <= Simulator::Now() && itLink->m_heardTime > Simulator::Now())
        {
            auto itNeigh = FindNeighborTupleIt(neighborAddrs.front());
            if (itNeigh != m_neighborInfoBase.end() && itNeigh->m_symmetric)
            {
                // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
                auto oldNeighborTuple = *itNeigh;
                NS_LOG_DEBUG("Setting neighbor "
                             << addressUtils::FormatAddress(neighborAddrs.front())
                             << " to not symmetric");
                itNeigh->m_symmetric = false;
                m_neighborChangeTrace(Action::CHANGED, oldNeighborTuple, *itNeigh);
                // RFC 6130, Sec. 13.2: link is no longer symmetric, so remove its 2-Hop Tuples
                RemoveTwoHopTuples(neighborAddrs);
            }
        }
        ++itLink;
    }
}

Ptr<PbbAddressBlock>
NhdpClient::BuildLocalAddressBlock(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << addressUtils::FormatAddress(m_socketAddresses[socket]));

    Ptr<PbbAddressBlock> addrBlock;
    if (m_addressMode == AddressMode::IPV4)
    {
        addrBlock = Create<PbbAddressBlockIpv4>();
        Ptr<Ipv4> ipv4 = GetNode()->GetObject<Ipv4>();
        NS_ASSERT(ipv4);
        int32_t localIface =
            ipv4->GetInterfaceForAddress(Ipv4Address::ConvertFrom(m_socketAddresses[socket]));
        NS_ASSERT(localIface >= 0);
        uint32_t numLocalAddrs = ipv4->GetNAddresses(localIface);
        for (uint32_t i = 0; i < numLocalAddrs; i++)
        {
            Ipv4InterfaceAddress ifaceAddr = ipv4->GetAddress(localIface, i);
            addrBlock->AddressPushBack(ifaceAddr.GetLocal());
            addrBlock->PrefixPushBack(ifaceAddr.GetMask().GetPrefixLength());
            NS_LOG_DEBUG("Adding address " << ifaceAddr.GetLocal() << " to address block");
        }
        /* Put in the addresses belonging to all the other interfaces */
        for (uint32_t i = 0; i < ipv4->GetNInterfaces(); i++)
        {
            if (i == static_cast<uint32_t>(localIface))
            {
                continue;
            }
            for (uint32_t j = 0; j < ipv4->GetNAddresses(i); j++)
            {
                Ipv4InterfaceAddress ifaceAddr = ipv4->GetAddress(i, j);
                if (ifaceAddr.GetLocal() == Ipv4Address::GetLoopback())
                {
                    continue;
                }
                addrBlock->AddressPushBack(ifaceAddr.GetLocal());
                addrBlock->PrefixPushBack(ifaceAddr.GetMask().GetPrefixLength());
            }
        }
        /* Mark the local addresses */
        Ptr<PbbAddressTlv> addrTlv = Create<PbbAddressTlv>();
        addrBlock->TlvPushBack(addrTlv);
        addrTlv->SetType(ADDR_TLV_LOCAL_IF);
        addrTlv->SetValue(&ADDR_TLV_LOCAL_IF_THIS_IF, sizeof(ADDR_TLV_LOCAL_IF_THIS_IF));
        addrTlv->SetIndexStart(0);
        if (numLocalAddrs > 1)
        {
            addrTlv->SetIndexStop(numLocalAddrs - 1);
        }
        if (numLocalAddrs != static_cast<uint32_t>(addrBlock->AddressSize()))
        {
            addrTlv = Create<PbbAddressTlv>();
            addrBlock->TlvPushBack(addrTlv);
            addrTlv->SetType(ADDR_TLV_LOCAL_IF);
            addrTlv->SetValue(&ADDR_TLV_LOCAL_IF_OTHER_IF, sizeof(ADDR_TLV_LOCAL_IF_OTHER_IF));
            addrTlv->SetIndexStart(numLocalAddrs);
            addrTlv->SetIndexStop(addrBlock->AddressSize() - 1);
        }
    }
    else
    {
        addrBlock = Create<PbbAddressBlockIpv6>();
        Ptr<Ipv6> ipv6 = GetNode()->GetObject<Ipv6>();
        NS_ASSERT(ipv6);
        int32_t localIface =
            ipv6->GetInterfaceForAddress(Ipv6Address::ConvertFrom(m_socketAddresses[socket]));
        NS_ASSERT(localIface >= 0);
        // Advertise all of this interface's usable addresses (link-local and the Multilink
        // Local / Unique Local Address) as THIS_IF (single-interface scope).
        for (const auto& addr : m_localAddresses)
        {
            Ipv6Address ipv6Addr = Ipv6Address::ConvertFrom(addr);
            int32_t addrIndex = -1;
            for (uint32_t j = 0; j < ipv6->GetNAddresses(localIface); j++)
            {
                if (ipv6->GetAddress(localIface, j).GetAddress() == ipv6Addr)
                {
                    addrIndex = static_cast<int32_t>(j);
                    break;
                }
            }
            uint8_t prefixLen = 128;
            if (addrIndex >= 0)
            {
                prefixLen = ipv6->GetAddress(localIface, addrIndex).GetPrefix().GetPrefixLength();
            }
            addrBlock->AddressPushBack(addr);
            addrBlock->PrefixPushBack(prefixLen);
            NS_LOG_DEBUG("Adding address " << addressUtils::FormatAddress(addr)
                                           << " to address block");
        }
        Ptr<PbbAddressTlv> addrTlv = Create<PbbAddressTlv>();
        addrBlock->TlvPushBack(addrTlv);
        addrTlv->SetType(ADDR_TLV_LOCAL_IF);
        addrTlv->SetValue(&ADDR_TLV_LOCAL_IF_THIS_IF, sizeof(ADDR_TLV_LOCAL_IF_THIS_IF));
        addrTlv->SetIndexStart(0);
        if (m_localAddresses.size() > 1)
        {
            addrTlv->SetIndexStop(m_localAddresses.size() - 1);
        }
    }

    return addrBlock;
}

Ptr<PbbAddressBlock>
NhdpClient::BuildLinkStatusAddressBlock(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << addressUtils::FormatAddress(m_socketAddresses[socket]));

    std::vector<Address> heard;
    std::vector<Address> symmetric;
    std::vector<std::pair<Address, AddressTlvLinkStatus>> links;

    NS_LOG_DEBUG("LinkTuple info base size " << m_linkInfoBase.size());
    for (auto& linkTuple : m_linkInfoBase)
    {
        // o  Network addresses of MANET interfaces of 1-hop neighbors from the Link Set
        // (i.e., from an L_neighbor_iface_addr_list), other than those from Link Tuples
        // with L_status = PENDING.
        if (linkTuple.GetLinkStatus() == LinkStatus::SYMMETRIC)
        {
            for (const auto& addr : linkTuple.m_neighborAddrList)
            {
                symmetric.push_back(addr);
            }
        }
        else if (linkTuple.GetLinkStatus() == LinkStatus::HEARD)
        {
            for (const auto& addr : linkTuple.m_neighborAddrList)
            {
                heard.push_back(addr);
            }
        }
    }
    if (heard.empty() && symmetric.empty() && m_lostNeighborSet.empty())
    {
        m_helloSendTrace(m_socketAddresses[socket], links);
        return nullptr;
    }
    std::set<Address> heardAdvertised;
    std::set<Address> symAdvertised;
    Ptr<PbbAddressBlock> addrBlock;
    if (m_addressMode == AddressMode::IPV4)
    {
        addrBlock = Create<PbbAddressBlockIpv4>();
    }
    else
    {
        addrBlock = Create<PbbAddressBlockIpv6>();
    }
    for (const auto& addr : heard)
    {
        addrBlock->AddressPushBack(addr);
        links.emplace_back(addr, AddressTlvLinkStatus::HEARD);
        NS_LOG_DEBUG("Adding HEARD address " << addressUtils::FormatAddress(addr));
        heardAdvertised.insert(addr);
    }
    for (const auto& addr : symmetric)
    {
        addrBlock->AddressPushBack(addr);
        links.emplace_back(addr, AddressTlvLinkStatus::SYMMETRIC);
        NS_LOG_DEBUG("Adding SYMMETRIC address " << addressUtils::FormatAddress(addr));
        symAdvertised.insert(addr);
    }
    for (auto& lostNeighborTuple : m_lostNeighborSet)
    {
        const auto& addr = lostNeighborTuple.m_neighborAddr;
        NS_ASSERT_MSG(heardAdvertised.find(addr) == heardAdvertised.end(),
                      "Overlap between HEARD and lost neighbor");
        NS_ASSERT_MSG(symAdvertised.find(addr) == symAdvertised.end(),
                      "Overlap between SYMMETRIC and lost neighbor");
        addrBlock->AddressPushBack(addr);
        links.emplace_back(addr, AddressTlvLinkStatus::LOST);
        NS_LOG_DEBUG("Adding LOST address " << addressUtils::FormatAddress(addr));
    }
    uint32_t index = 0;
    if (!heard.empty())
    {
        Ptr<PbbAddressTlv> addrTlv = Create<PbbAddressTlv>();
        addrTlv->SetType(ADDR_TLV_LINK_STATUS);
        addrTlv->SetValue(&ADDR_TLV_LINK_STATUS_HEARD, sizeof(ADDR_TLV_LINK_STATUS_HEARD));
        addrTlv->SetIndexStart(0);
        addrTlv->SetIndexStop(heard.size() - 1);
        addrBlock->TlvPushBack(addrTlv);
        NS_LOG_DEBUG("Adding PbbAddressTlv for HEARD address from index 0 to " << heard.size() - 1);
    }
    index += heard.size();
    if (!symmetric.empty())
    {
        Ptr<PbbAddressTlv> addrTlv = Create<PbbAddressTlv>();
        addrTlv->SetType(ADDR_TLV_LINK_STATUS);
        addrTlv->SetValue(&ADDR_TLV_LINK_STATUS_SYMMETRIC, sizeof(ADDR_TLV_LINK_STATUS_SYMMETRIC));
        addrTlv->SetIndexStart(index);
        addrTlv->SetIndexStop(index + symmetric.size() - 1);
        addrBlock->TlvPushBack(addrTlv);
        NS_LOG_DEBUG("Adding PbbAddressTlv for SYMMETRIC address from index "
                     << index << " to " << index + symmetric.size() - 1);
    }
    index += symmetric.size();
    if (!m_lostNeighborSet.empty())
    {
        Ptr<PbbAddressTlv> addrTlv = Create<PbbAddressTlv>();
        addrTlv->SetType(ADDR_TLV_LINK_STATUS);
        addrTlv->SetValue(&ADDR_TLV_LINK_STATUS_LOST, sizeof(ADDR_TLV_LINK_STATUS_LOST));
        addrTlv->SetIndexStart(index);
        addrTlv->SetIndexStop(index + m_lostNeighborSet.size() - 1);
        addrBlock->TlvPushBack(addrTlv);
        NS_LOG_DEBUG("Adding PbbAddressTlv for LOST address from index "
                     << index << " to " << index + m_lostNeighborSet.size() - 1);
    }
    m_helloSendTrace(m_socketAddresses[socket], links);
    return addrBlock;
}

std::ostream&
operator<<(std::ostream& os, const Action& action)
{
    if (action == Action::ADDED)
    {
        os << "ADDED";
    }
    else if (action == Action::CHANGED)
    {
        os << "CHANGED";
    }
    else if (action == Action::REMOVED)
    {
        os << "REMOVED";
    }
    return os;
}

std::ostream&
operator<<(std::ostream& os, const AddressTlvLinkStatus& status)
{
    if (status == AddressTlvLinkStatus::LOST)
    {
        os << "LOST";
    }
    else if (status == AddressTlvLinkStatus::SYMMETRIC)
    {
        os << "SYMMETRIC";
    }
    else if (status == AddressTlvLinkStatus::HEARD)
    {
        os << "HEARD";
    }
    return os;
}

} // namespace manet

} // namespace ns3
