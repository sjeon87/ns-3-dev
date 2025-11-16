/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "sink-application.h"

#include "ns3/boolean.h"
#include "ns3/log.h"
#include "ns3/socket.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SinkApplication");

NS_OBJECT_ENSURE_REGISTERED(SinkApplication);

TypeId
SinkApplication::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SinkApplication")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddAttribute(
                "Local",
                "The Address on which to Bind the rx socket. "
                "If it is not specified, it will listen to any address.",
                AddressValue(),
                MakeAddressAccessor(&SinkApplication::SetLocal, &SinkApplication::GetLocal),
                MakeAddressChecker())
            .AddAttribute(
                "Port",
                "Port on which the application listens for incoming packets.",
                UintegerValue(INVALID_PORT),
                MakeUintegerAccessor(&SinkApplication::SetPort, &SinkApplication::GetPort),
                MakeUintegerChecker<uint32_t>())
            .AddAttribute("EnableSeqTsSizeHeader",
                          "Enable optional header tracing of SeqTsSizeHeader (for NS3_SOCK_STREAM "
                          "socket) or SeqTsHeader (for NS3_SOCK_DGRAM socket)",
                          BooleanValue(false),
                          MakeBooleanAccessor(&SinkApplication::m_enableSeqTsSizeHeader),
                          MakeBooleanChecker())
            .AddTraceSource("Rx",
                            "A packet has been received",
                            MakeTraceSourceAccessor(&SinkApplication::m_rxTrace),
                            "ns3::Packet::AddressTracedCallback")
            .AddTraceSource("RxWithoutAddress",
                            "A packet has been received from a given address",
                            MakeTraceSourceAccessor(&SinkApplication::m_rxTraceWithoutAddress),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("RxWithSeqTsSize",
                            "A packet with SeqTsSize header has been received",
                            MakeTraceSourceAccessor(&SinkApplication::m_rxTraceWithSeqTsSize),
                            "ns3::SinkApplication::SeqTsSizeCallback")
            .AddTraceSource("RxWithSeqTs",
                            "A packet with SeqTs header has been received",
                            MakeTraceSourceAccessor(&SinkApplication::m_rxTraceWithSeqTs),
                            "ns3::SinkApplication::SeqTsCallback");
    return tid;
}

SinkApplication::SinkApplication(uint16_t defaultPort)
    : m_port{defaultPort}
{
    NS_LOG_FUNCTION(this << defaultPort);
}

SinkApplication::~SinkApplication()
{
    NS_LOG_FUNCTION(this);
}

void
SinkApplication::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_socket = nullptr;
    m_socket6 = nullptr;
    Application::DoDispose();
}

void
SinkApplication::SetLocal(const Address& addr)
{
    NS_LOG_FUNCTION(this << addr);
    m_local = addr;
}

Address
SinkApplication::GetLocal() const
{
    return m_local;
}

void
SinkApplication::SetPort(uint32_t port)
{
    NS_LOG_FUNCTION(this << port);
    if (port == INVALID_PORT)
    {
        return;
    }
    m_port = port;
}

uint32_t
SinkApplication::GetPort() const
{
    return m_port;
}

void
SinkApplication::StartApplication()
{
    NS_LOG_FUNCTION(this);

    // note: it is currently not possible to restart an application

    m_socket = Socket::CreateSocket(GetNode(), m_protocolTid);
    m_socket->SetRecvCallback(MakeCallback(&SinkApplication::HandleRead, this));
    if (m_local.IsInvalid() && !m_socket6)
    {
        // local address is not specified, so create another socket to also listen to all IPv6
        // addresses
        m_socket6 = Socket::CreateSocket(GetNode(), m_protocolTid);
        m_socket6->SetRecvCallback(MakeCallback(&SinkApplication::HandleRead, this));
    }

    DoStartApplication();
}

void
SinkApplication::StopApplication()
{
    NS_LOG_FUNCTION(this);
    DoStopApplication();
    CloseAllSockets();
}

bool
SinkApplication::CloseAllSockets()
{
    NS_LOG_FUNCTION(this);
    auto ret = CloseSocket(m_socket);
    ret = CloseSocket(m_socket6) && ret;
    return ret;
}

bool
SinkApplication::CloseSocket(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    if (socket)
    {
        const auto ret = socket->Close();
        socket->SetAcceptCallback(MakeNullCallback<bool, Ptr<Socket>, const Address&>(),
                                  MakeNullCallback<void, Ptr<Socket>, const Address&>());
        socket->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket>>(),
                                  MakeNullCallback<void, Ptr<Socket>>());
        socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
        socket->SetSendCallback(MakeNullCallback<void, Ptr<Socket>, uint32_t>());
        return (ret == 0);
    }
    return true;
}

void
SinkApplication::DoStartApplication()
{
    NS_LOG_FUNCTION(this);
}

void
SinkApplication::DoStopApplication()
{
    NS_LOG_FUNCTION(this);
}

void
SinkApplication::ProcessSeqTsSizeHeader(const Ptr<Packet>& p,
                                        const Address& from,
                                        const Address& localAddress)
{
    NS_LOG_FUNCTION(this << p << from << localAddress);

    auto itBuffer = m_buffer.find(from);
    if (itBuffer == m_buffer.end())
    {
        itBuffer = m_buffer.emplace(from, Create<Packet>(0)).first;
    }

    auto buffer = itBuffer->second;
    buffer->AddAtEnd(p);

    SeqTsSizeHeader header;
    while ((buffer->GetSize() >= header.GetSerializedSize()))
    {
        buffer->PeekHeader(header);
        NS_ABORT_MSG_IF((header.GetSize() == 0),
                        "A SeqTsSizeHeader could not be found in the packet");

        if (buffer->GetSize() < header.GetSize())
        {
            break;
        }

        NS_LOG_DEBUG("Removing packet of size " << header.GetSize() << " from buffer of size "
                                                << buffer->GetSize());
        auto complete = buffer->CreateFragment(0, static_cast<uint32_t>(header.GetSize()));
        buffer->RemoveAtStart(static_cast<uint32_t>(header.GetSize()));

        complete->RemoveHeader(header);

        m_rxTraceWithSeqTsSize(complete, from, localAddress, header);
    }
}

void
SinkApplication::ProcessSeqTsHeader(const Ptr<Packet>& p,
                                    const Address& from,
                                    const Address& localAddress)
{
    NS_LOG_FUNCTION(this << p << from << localAddress);

    SeqTsHeader header;
    p->PeekHeader(header);

    if ((header.GetSeq() == 0) && (header.GetTs().IsZero()))
    {
        NS_LOG_WARN("SeqTsHeader not transmitted");
        return;
    }

    p->RemoveHeader(header);
    m_rxTraceWithSeqTs(p, from, localAddress, header);
}

void
SinkApplication::HandleRead(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    Address from;
    while (auto packet = socket->RecvFrom(from))
    {
        if (packet->GetSize() == 0)
        {
            continue;
        }
        ReceivePacket(socket, packet, from);
        if (!m_enableSeqTsSizeHeader || m_rxTraceWithSeqTsSize.IsEmpty())
        {
            continue;
        }
        Address localAddress;
        socket->GetSockName(localAddress);
        if (socket->GetSocketType() == Socket::NS3_SOCK_DGRAM)
        {
            ProcessSeqTsHeader(packet, from, localAddress);
        }
        else if (socket->GetSocketType() == Socket::NS3_SOCK_STREAM)
        {
            ProcessSeqTsSizeHeader(packet, from, localAddress);
        }
    }
}

} // Namespace ns3
