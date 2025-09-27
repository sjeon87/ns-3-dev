/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "source-application.h"

#include "ns3/boolean.h"
#include "ns3/enum.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/log.h"
#include "ns3/packet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SourceApplication");

NS_OBJECT_ENSURE_REGISTERED(SourceApplication);

TypeId
SourceApplication::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SourceApplication")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddAttribute(
                "Remote",
                "The address of the destination, made of the remote IP address and the "
                "destination port",
                AddressValue(),
                MakeAddressAccessor(&SourceApplication::SetRemote, &SourceApplication::GetRemote),
                MakeAddressChecker())
            .AddAttribute("Local",
                          "The Address on which to bind the socket. If not set, it is generated "
                          "automatically when needed by the application.",
                          AddressValue(),
                          MakeAddressAccessor(&SourceApplication::m_local),
                          MakeAddressChecker())
            .AddAttribute("Tos",
                          "The Type of Service used to send IPv4 packets. "
                          "All 8 bits of the TOS byte are set (including ECN bits).",
                          UintegerValue(0),
                          MakeUintegerAccessor(&SourceApplication::m_tos),
                          MakeUintegerChecker<uint8_t>())
            .AddAttribute("EnableSeqTsSizeHeader",
                          "Enable use of SeqTsHeader (for NS3_SOCK_DGRAM socket) or "
                          "SeqTsSizeHeader (for NS3_SOCK_STREAM socket) for sequence number, "
                          "timestamp and size (for NS3_SOCK_STREAM socket only)",
                          BooleanValue(false),
                          MakeBooleanAccessor(&SourceApplication::m_enableSeqTsSizeHeader),
                          MakeBooleanChecker())
            .AddAttribute("IncrementCounterIfTxFailed",
                          "If enabled, the sequence number counter is incremented even if the "
                          "transmission of the packet failed.",
                          EnumValue(SourceApplication::IncrementCounterIfTxFailed::
                                        UNDEFINED), // default value defined by the the child class
                          MakeEnumAccessor<SourceApplication::IncrementCounterIfTxFailed>(
                              &SourceApplication::SetIncrementCounterIfTxFailed,
                              &SourceApplication::GetIncrementCounterIfTxFailed),
                          MakeEnumChecker(SourceApplication::IncrementCounterIfTxFailed::UNDEFINED,
                                          "Undefined",
                                          SourceApplication::IncrementCounterIfTxFailed::ENABLED,
                                          "Enabled",
                                          SourceApplication::IncrementCounterIfTxFailed::DISABLED,
                                          "Disabled"))
            .AddTraceSource("Tx",
                            "A packet is sent",
                            MakeTraceSourceAccessor(&SourceApplication::m_txTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("TxWithSeqTsSize",
                            "A new packet is created with SeqTsSizeHeader",
                            MakeTraceSourceAccessor(&SourceApplication::m_txTraceWithSeqTsSize),
                            "ns3::SinkApplication::SeqTsSizeCallback")
            .AddTraceSource("TxWithSeqTs",
                            "A new packet is created with SeqTsHeader",
                            MakeTraceSourceAccessor(&SourceApplication::m_txTraceWithSeqTs),
                            "ns3::SinkApplication::SeqTsCallback")
            .AddTraceSource("ConnectionSucceeded",
                            "Succeeded to establish connection",
                            MakeTraceSourceAccessor(&SourceApplication::m_connectionSuccess),
                            "ns3::SourceApplication::ConnectionEventCallback")
            .AddTraceSource("ConnectionFailed",
                            "Failed to establish connection",
                            MakeTraceSourceAccessor(&SourceApplication::m_connectionFailure),
                            "ns3::SourceApplication::ConnectionEventCallback");
    return tid;
}

SourceApplication::SourceApplication(bool allowPacketSocket, bool incrementCounterIfTxFailed)
    : m_allowPacketSocket{allowPacketSocket},
      m_incrementCounterIfTxFailed{incrementCounterIfTxFailed}
{
    NS_LOG_FUNCTION(this);
}

SourceApplication::~SourceApplication()
{
    NS_LOG_FUNCTION(this);
}

void
SourceApplication::DoDispose()
{
    NS_LOG_FUNCTION(this);
    CancelEvents();
    m_socket = nullptr;
    Application::DoDispose();
}

void
SourceApplication::SetRemote(const Address& addr)
{
    NS_LOG_FUNCTION(this << addr);
    if (!addr.IsInvalid())
    {
        m_peer = addr;
    }
}

Address
SourceApplication::GetRemote() const
{
    return m_peer;
}

Ptr<Socket>
SourceApplication::GetSocket() const
{
    return m_socket;
}

void
SourceApplication::SetIncrementCounterIfTxFailed(
    SourceApplication::IncrementCounterIfTxFailed option)
{
    NS_LOG_FUNCTION(this << static_cast<uint8_t>(option));
    if (option == IncrementCounterIfTxFailed::UNDEFINED)
    {
        return;
    }
    m_incrementCounterIfTxFailed = (option == IncrementCounterIfTxFailed::ENABLED);
}

SourceApplication::IncrementCounterIfTxFailed
SourceApplication::GetIncrementCounterIfTxFailed() const
{
    NS_LOG_FUNCTION(this);
    return m_incrementCounterIfTxFailed ? IncrementCounterIfTxFailed::ENABLED
                                        : IncrementCounterIfTxFailed::DISABLED;
}

void
SourceApplication::StartApplication()
{
    NS_LOG_FUNCTION(this);

    // note: it is currently not possible to restart an application

    NS_ABORT_MSG_IF(m_peer.IsInvalid(), "Remote address not properly set");
    if (!m_local.IsInvalid())
    {
        NS_ABORT_MSG_IF((Inet6SocketAddress::IsMatchingType(m_peer) &&
                         InetSocketAddress::IsMatchingType(m_local)) ||
                            (InetSocketAddress::IsMatchingType(m_peer) &&
                             Inet6SocketAddress::IsMatchingType(m_local)),
                        "Incompatible peer and local address IP version");
    }

    m_socket = Socket::CreateSocket(GetNode(), m_protocolTid);
    m_socket->SetConnectCallback(MakeCallback(&SourceApplication::ConnectionSucceeded, this),
                                 MakeCallback(&SourceApplication::ConnectionFailed, this));

    int ret{-1};
    if (InetSocketAddress::IsMatchingType(m_peer) ||
        (m_allowPacketSocket && PacketSocketAddress::IsMatchingType(m_peer)))
    {
        ret = m_socket->Bind();
    }
    else if (Inet6SocketAddress::IsMatchingType(m_peer))
    {
        ret = m_socket->Bind6();
    }
    else
    {
        NS_FATAL_ERROR("Incompatible address type: " << m_peer);
    }
    if (ret == -1)
    {
        NS_FATAL_ERROR("Failed to bind socket");
    }

    if (InetSocketAddress::IsMatchingType(m_peer))
    {
        m_socket->SetIpTos(m_tos); // Affects only IPv4 sockets.
    }

    m_socket->Connect(m_peer);

    CancelEvents();

    DoStartApplication();
}

void
SourceApplication::StopApplication()
{
    NS_LOG_FUNCTION(this);
    DoStopApplication();
    CancelEvents();
    CloseSocket();
}

bool
SourceApplication::CloseSocket()
{
    m_connected = false;
    if (m_socket)
    {
        const auto ret = m_socket->Close();
        m_socket->SetConnectCallback(MakeNullCallback<void, Ptr<Socket>>(),
                                     MakeNullCallback<void, Ptr<Socket>>());
        m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
        return (ret == 0);
    }
    return true;
}

void
SourceApplication::ConnectionSucceeded(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    m_connected = true;
    DoConnectionSucceeded(socket);
    m_connectionSuccess(socket, m_local, m_peer);
}

void
SourceApplication::ConnectionFailed(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    m_connected = false;
    DoConnectionFailed(socket);
    m_connectionFailure(socket, m_local, m_peer);
}

void
SourceApplication::DoStartApplication()
{
    NS_LOG_FUNCTION(this);
}

void
SourceApplication::DoStopApplication()
{
    NS_LOG_FUNCTION(this);
}

void
SourceApplication::DoConnectionSucceeded(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
}

void
SourceApplication::DoConnectionFailed(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
}

Ptr<Packet>
SourceApplication::CreatePacketWithSeqTsSizeHeader(uint32_t seq, uint64_t size)
{
    NS_LOG_FUNCTION(this << seq << size);
    NS_ASSERT_MSG(m_enableSeqTsSizeHeader, "Use of SeqTsSizeHeader not enabled");

    uint32_t headerSize{0};
    SeqTsSizeHeader seqTsSizeHdr;
    SeqTsHeader seqTsHdr;
    if (m_socket->GetSocketType() == Socket::NS3_SOCK_STREAM)
    {
        seqTsSizeHdr.SetSeq(seq);
        seqTsSizeHdr.SetSize(size);
        headerSize = seqTsSizeHdr.GetSerializedSize();
    }
    else if (m_socket->GetSocketType() == Socket::NS3_SOCK_DGRAM)
    {
        seqTsHdr.SetSeq(seq);
        headerSize = seqTsHdr.GetSerializedSize();
    }

    NS_ABORT_IF(size < headerSize);
    auto packet = Create<Packet>(size - headerSize);

    // Trace before adding header, for consistency with sink applications
    NS_ASSERT(m_socket);
    Address from;
    Address to;
    m_socket->GetSockName(from);
    m_socket->GetPeerName(to);
    if (m_socket->GetSocketType() == Socket::NS3_SOCK_STREAM)
    {
        m_txTraceWithSeqTsSize(packet, from, to, seqTsSizeHdr);
        packet->AddHeader(seqTsSizeHdr);
    }
    else if (m_socket->GetSocketType() == Socket::NS3_SOCK_DGRAM)
    {
        m_txTraceWithSeqTs(packet, from, to, seqTsHdr);
        packet->AddHeader(seqTsHdr);
    }

    return packet;
}

Ptr<Packet>
SourceApplication::CreatePacket(uint64_t size)
{
    NS_LOG_FUNCTION(this << size);
    return m_enableSeqTsSizeHeader ? CreatePacketWithSeqTsSizeHeader(m_seq, size)
                                   : Create<Packet>(size);
}

int
SourceApplication::SendPacket(Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this << packet);
    auto ret = m_socket->Send(packet);
    if (ret >= 0 || m_incrementCounterIfTxFailed)
    {
        ++m_seq;
    }
    return ret;
}

} // Namespace ns3
