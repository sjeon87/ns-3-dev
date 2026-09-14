/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ntp-server.h"

#include "ntp-header.h"

#include "ns3/inet-socket-address.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/udp-socket-factory.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NtpServer");

TypeId
NtpServer::GetTypeId()
{
    static TypeId tid = TypeId("ns3::NtpServer")
                            .SetParent<Application>()
                            .SetGroupName("Applications")
                            .AddConstructor<NtpServer>();
    return tid;
}

NtpServer::NtpServer()
    : m_port(0)
{
    NS_LOG_FUNCTION(this);
}

NtpServer::~NtpServer()
{
    NS_LOG_FUNCTION(this);
}

void
NtpServer::Setup(uint16_t port, Time processingDelay)
{
    NS_LOG_FUNCTION(this << port << processingDelay);
    m_port = port;
    m_processingDelay = processingDelay;
}

void
NtpServer::StartApplication()
{
    NS_LOG_FUNCTION(this);
    m_clock = GetNode()->GetObject<LocalClock>();

    if (!m_socket)
    {
        m_socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
        m_socket->Bind(InetSocketAddress(Ipv4Address::GetAny(), m_port));
    }
    m_socket->SetRecvCallback(MakeCallback(&NtpServer::HandleRead, this));
}

void
NtpServer::StopApplication()
{
    NS_LOG_FUNCTION(this);
    if (m_socket)
    {
        m_socket->Close();
    }
}

void
NtpServer::HandleRead(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from)))
    {
        NtpHeader header;
        packet->RemoveHeader(header);

        Time t2 = m_clock ? m_clock->Now() : Simulator::Now();
        Simulator::Schedule(m_processingDelay,
                            &NtpServer::SendReply,
                            this,
                            from,
                            header.GetT1(),
                            t2);
    }
}

void
NtpServer::SendReply(Address from, Time t1, Time t2)
{
    NS_LOG_FUNCTION(this << from << t1 << t2);
    Time t3 = m_clock ? m_clock->Now() : Simulator::Now();

    NtpHeader header;
    header.SetReply(true);
    header.SetT1(t1);
    header.SetT2(t2);
    header.SetT3(t3);

    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(header);
    m_socket->SendTo(packet, 0, from);
}

} // namespace ns3
