/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ntp-client.h"

#include "ntp-header.h"

#include "ns3/dynamic-skew-scheduler.h"
#include "ns3/epoch-table.h"
#include "ns3/inet-socket-address.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/udp-socket-factory.h"

#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NtpClient");

TypeId
NtpClient::GetTypeId()
{
    static TypeId tid = TypeId("ns3::NtpClient")
                            .SetParent<Application>()
                            .SetGroupName("Applications")
                            .AddConstructor<NtpClient>();
    return tid;
}

static constexpr double MIN_SLEW_RATE = 0.01; //!< Floor on the computed clock slew rate

NtpClient::NtpClient()
{
    NS_LOG_FUNCTION(this);
}

NtpClient::~NtpClient()
{
    NS_LOG_FUNCTION(this);
}

void
NtpClient::Setup(Ipv4Address serverAddress, uint16_t serverPort, Time pollInterval)
{
    NS_LOG_FUNCTION(this << serverAddress << serverPort << pollInterval);
    m_peer = InetSocketAddress(serverAddress, serverPort);
    m_pollInterval = pollInterval;
}

void
NtpClient::StartApplication()
{
    NS_LOG_FUNCTION(this);
    m_clock = GetNode()->GetObject<LocalClock>();

    if (!m_socket)
    {
        m_socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
        m_socket->Bind();
    }
    m_socket->SetRecvCallback(MakeCallback(&NtpClient::HandleRead, this));

    m_pollEvent = Simulator::Schedule(Seconds(0.0), &NtpClient::SendRequest, this);
}

void
NtpClient::StopApplication()
{
    NS_LOG_FUNCTION(this);
    Simulator::Cancel(m_pollEvent);
    if (m_socket)
    {
        m_socket->Close();
    }
}

void
NtpClient::SendRequest()
{
    NS_LOG_FUNCTION(this);
    Time t1 = m_clock ? m_clock->Now() : Simulator::Now();

    NtpHeader header;
    header.SetReply(false);
    header.SetT1(t1);

    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(header);
    m_socket->SendTo(packet, 0, m_peer);

    m_pollEvent = Simulator::Schedule(m_pollInterval, &NtpClient::SendRequest, this);
}

void
NtpClient::HandleRead(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from)))
    {
        NtpHeader header;
        packet->RemoveHeader(header);
        if (!header.IsReply())
        {
            continue;
        }

        Time t1 = header.GetT1();
        Time t2 = header.GetT2();
        Time t3 = header.GetT3();
        Time t4 = m_clock ? m_clock->Now() : Simulator::Now();

        Time offset = ((t2 - t1) + (t3 - t4)) / 2;
        Time delay = (t4 - t1) - (t3 - t2);

        uint32_t nodeId = GetNode()->GetId();
        Ptr<EpochTable> table = DynamicSkewScheduler::GetCurrentEpochTable();
        std::string action = "none";

        if (table && table->HasNode(nodeId) && !offset.IsZero())
        {
            double rate = 1.0 + offset.GetSeconds() / m_pollInterval.GetSeconds();
            rate = std::max(rate, MIN_SLEW_RATE);
            DynamicSkewScheduler::ChangeCurrentSkew(nodeId, rate);
            action = "slew";
        }

        std::cout << Simulator::Now().As(Time::S) << "\tnode" << nodeId
                  << "\tT1=" << t1.As(Time::MS) << "\tT4=" << t4.As(Time::MS)
                  << "\toffset=" << offset.As(Time::MS) << "\tdelay=" << delay.As(Time::MS)
                  << "\taction=" << action << std::endl;
    }
}

} // namespace ns3
