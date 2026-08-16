/*
 * Copyright (c) 2010 Georgia Institute of Technology
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: George F. Riley <riley@ece.gatech.edu>
 */

#include "bulk-send-application.h"

#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("BulkSendApplication");

NS_OBJECT_ENSURE_REGISTERED(BulkSendApplication);

TypeId
BulkSendApplication::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::BulkSendApplication")
            .SetParent<SourceApplication>()
            .SetGroupName("Applications")
            .AddConstructor<BulkSendApplication>()
            .AddAttribute("SendSize",
                          "The amount of data to send each time.",
                          UintegerValue(512),
                          MakeUintegerAccessor(&BulkSendApplication::m_sendSize),
                          MakeUintegerChecker<uint32_t>(1))
            .AddAttribute("MaxBytes",
                          "The total number of bytes to send. "
                          "Once these bytes are sent, "
                          "no data  is sent again. The value zero means "
                          "that there is no limit.",
                          UintegerValue(0),
                          MakeUintegerAccessor(&BulkSendApplication::m_maxBytes),
                          MakeUintegerChecker<uint64_t>())
            .AddAttribute("Protocol",
                          "The type of protocol to use.",
                          TypeIdValue(TcpSocketFactory::GetTypeId()),
                          MakeTypeIdAccessor(&BulkSendApplication::m_protocolTid),
                          MakeTypeIdChecker())
            .AddTraceSource("TcpRetransmission",
                            "The TCP socket retransmitted a packet",
                            MakeTraceSourceAccessor(&BulkSendApplication::m_retransmissionTrace),
                            "ns3::TcpSocketBase::RetransmissionCallback");

    return tid;
}

BulkSendApplication::BulkSendApplication()
{
    NS_LOG_FUNCTION(this);
}

BulkSendApplication::~BulkSendApplication()
{
    NS_LOG_FUNCTION(this);
}

void
BulkSendApplication::SetMaxBytes(uint64_t maxBytes)
{
    NS_LOG_FUNCTION(this << maxBytes);
    m_maxBytes = maxBytes;
}

void
BulkSendApplication::CancelEvents()
{
    NS_LOG_FUNCTION(this);
    m_unsentPacket = nullptr;
}

// Application Methods
void
BulkSendApplication::DoStartApplication() // Called at time specified by Start
{
    NS_LOG_FUNCTION(this);

    // Fatal error if socket type is not NS3_SOCK_STREAM
    if (m_socket->GetSocketType() != Socket::NS3_SOCK_STREAM)
    {
        NS_FATAL_ERROR("Using BulkSend with an incompatible socket type. BulkSend requires "
                       "SOCK_STREAM. In other words, use TCP instead of UDP.");
    }

    m_socket->ShutdownRecv();
    m_socket->SetSendCallback(MakeCallback(&BulkSendApplication::DataSend, this));

    if (auto tcpSocket = DynamicCast<TcpSocketBase>(m_socket))
    {
        tcpSocket->TraceConnectWithoutContext(
            "Retransmission",
            MakeCallback(&BulkSendApplication::PacketRetransmitted, this));
    }

    SendData();
}

// Private helpers

void
BulkSendApplication::SendData()
{
    NS_LOG_FUNCTION(this);

    if (!m_connected)
    {
        // We can only send data once the connection has completed
        return;
    }

    while (m_maxBytes == 0 || m_totBytes < m_maxBytes)
    { // Time to send more

        // uint64_t to allow the comparison later.
        // the result is in a uint32_t range anyway, because
        // m_sendSize is uint32_t.
        uint64_t toSend = m_sendSize;
        // Make sure we don't send too many
        if (m_maxBytes > 0)
        {
            toSend = std::min(toSend, m_maxBytes - m_totBytes);
        }

        NS_LOG_LOGIC("sending packet at " << Simulator::Now());

        auto packet = m_unsentPacket ? m_unsentPacket : CreatePacket(toSend);
        toSend = packet->GetSize();

        int actual = SendPacket(packet);
        if ((unsigned)actual == toSend)
        {
            m_totBytes += actual;
            m_txTrace(packet);
            m_unsentPacket = nullptr;
        }
        else if (actual == -1)
        {
            // We exit this loop when actual < toSend as the send side
            // buffer is full. The "DataSent" callback will pop when
            // some buffer space has freed up.
            NS_LOG_DEBUG("Unable to send packet; caching for later attempt");
            m_unsentPacket = packet;
            break;
        }
        else if (actual > 0 && (unsigned)actual < toSend)
        {
            // A Linux socket (non-blocking, such as in DCE) may return
            // a quantity less than the packet size.  Split the packet
            // into two, trace the sent packet, save the unsent packet
            NS_LOG_DEBUG("Packet size: " << packet->GetSize() << "; sent: " << actual
                                         << "; fragment saved: " << toSend - (unsigned)actual);
            Ptr<Packet> sent = packet->CreateFragment(0, actual);
            Ptr<Packet> unsent = packet->CreateFragment(actual, (toSend - (unsigned)actual));
            m_totBytes += actual;
            m_txTrace(sent);
            m_unsentPacket = unsent;
            break;
        }
        else
        {
            NS_FATAL_ERROR("Unexpected return value from m_socket->Send ()");
        }
    }
    // Check if time to close (all sent)
    if (m_totBytes == m_maxBytes && m_connected)
    {
        m_socket->Close();
        m_connected = false;
    }
}

void
BulkSendApplication::DoConnectionSucceeded(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_LOGIC("BulkSendApplication Connection succeeded");
    SendData();
}

void
BulkSendApplication::DataSend(Ptr<Socket> socket, uint32_t)
{
    NS_LOG_FUNCTION(this);
    SendData();
}

void
BulkSendApplication::PacketRetransmitted(Ptr<const Packet> p,
                                         const TcpHeader& header,
                                         const Address& localAddr,
                                         const Address& peerAddr,
                                         Ptr<const TcpSocketBase> socket)
{
    NS_LOG_FUNCTION(this << p << header << localAddr << peerAddr << socket);
    m_retransmissionTrace(p, header, localAddr, peerAddr, socket);
}

} // Namespace ns3
