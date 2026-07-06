/*
 * Copyright (c) 2008 INRIA
 *                  2013 University of New Brunswick
 *                  2014 Universitat Autònoma de Barcelona
 *                  2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *           Dizhi Zhou <dizhi.zhou@gmail.com>
 *           Gerard Garcia <ggarcia@deic.uab.cat>
 *           Rubén Martínez <rmartinez@deic.uab.cat>
 *           Ishaan Lagwankar <lagwanka@msu.edu>
 */
#include "tcp-convergence-layer-adapter.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/tcp-socket-factory.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TcpBundleCla");
NS_OBJECT_ENSURE_REGISTERED(TcpBundleCla);

TypeId
TcpBundleCla::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TcpBundleCla")
                            .SetParent<BundleCla>()
                            .SetGroupName("BundleProtocol")
                            .AddConstructor<TcpBundleCla>();
    return tid;
}

TcpBundleCla::TcpBundleCla()
    : m_listenSocket(nullptr),
      m_sendSocket(nullptr),
      m_connected(false),
      m_isUp(false)
{
    NS_LOG_FUNCTION(this);
}

TcpBundleCla::~TcpBundleCla()
{
    NS_LOG_FUNCTION(this);
    if (m_listenSocket)
    {
        m_listenSocket->Close();
        m_listenSocket = nullptr;
    }
    if (m_sendSocket)
    {
        m_sendSocket->Close();
        m_sendSocket = nullptr;
    }
    for (auto& socket : m_acceptedSockets)
    {
        socket->Close();
    }
    m_acceptedSockets.clear();
}

void
TcpBundleCla::Setup(Ptr<Node> node, Address localAddress, Address remoteAddress)
{
    NS_LOG_FUNCTION(this << node << localAddress << remoteAddress);
    m_remoteAddress = remoteAddress;
    m_isUp = true;

    m_listenSocket = Socket::CreateSocket(node, TcpSocketFactory::GetTypeId());

    if (m_listenSocket->Bind(localAddress) == -1)
    {
        NS_LOG_ERROR("Failed to bind TCP listen socket to " << localAddress);
        return;
    }

    m_listenSocket->Listen();
    m_listenSocket->SetAcceptCallback(MakeCallback(&TcpBundleCla::ConnectionRequest, this),
                                      MakeCallback(&TcpBundleCla::AcceptConnection, this));

    m_sendSocket = Socket::CreateSocket(node, TcpSocketFactory::GetTypeId());

    m_sendSocket->SetConnectCallback(MakeCallback(&TcpBundleCla::ConnectionSucceeded, this),
                                     MakeCallback(&TcpBundleCla::ConnectionFailed, this));

    Simulator::ScheduleWithContext(node->GetId(),
                                   Seconds(0.001),
                                   &Socket::Connect,
                                   m_sendSocket,
                                   m_remoteAddress);

    NS_LOG_DEBUG("TcpBundleCla configured. Listening on " << localAddress << ", connecting to "
                                                          << m_remoteAddress);
}

void
TcpBundleCla::ConnectionSucceeded(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_DEBUG("TCP Connection established to " << m_remoteAddress);
    m_connected = true;

    while (!m_sendQueue.empty())
    {
        auto pair = m_sendQueue.front();
        m_sendQueue.pop();

        Ptr<Packet> framed = FrameBundle(pair.first);
        int bytes = m_sendSocket->Send(framed);
        NS_LOG_DEBUG("Flushed queued packet of size " << bytes << " bytes");

        if (!m_txResultCb.IsNull() && pair.second != 0)
        {
            m_txResultCb(pair.second, bytes == static_cast<int>(framed->GetSize()));
        }
    }
}

void
TcpBundleCla::ConnectionFailed(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_WARN("TCP Connection failed to " << m_remoteAddress);
    m_connected = false;

    while (!m_sendQueue.empty())
    {
        auto pair = m_sendQueue.front();
        m_sendQueue.pop();

        if (!m_txResultCb.IsNull() && pair.second != 0)
        {
            m_txResultCb(pair.second, false);
        }
    }
}

void
TcpBundleCla::Send(Ptr<Packet> packet, uint32_t bundleHandle)
{
    if (m_connected)
    {
        Ptr<Packet> framed = FrameBundle(packet);
        int bytesSent = m_sendSocket->Send(framed);
        if (!m_txResultCb.IsNull() && bundleHandle != 0)
        {
            m_txResultCb(bundleHandle, bytesSent == static_cast<int>(framed->GetSize()));
        }
    }
    else
    {
        m_sendQueue.emplace(packet, bundleHandle);
    }
}

Ptr<Packet>
TcpBundleCla::FrameBundle(Ptr<Packet> packet)
{
    uint32_t length = packet->GetSize();
    uint8_t lengthPrefix[4] = {
        static_cast<uint8_t>((length >> 24) & 0xFF),
        static_cast<uint8_t>((length >> 16) & 0xFF),
        static_cast<uint8_t>((length >> 8) & 0xFF),
        static_cast<uint8_t>(length & 0xFF),
    };

    Ptr<Packet> framed = Create<Packet>(lengthPrefix, 4);
    framed->AddAtEnd(packet);
    return framed;
}

std::vector<Ptr<Packet>>
TcpBundleCla::ExtractFramedBundles(Ptr<Packet> buffer)
{
    std::vector<Ptr<Packet>> bundles;

    while (buffer->GetSize() >= 4)
    {
        uint8_t lengthPrefix[4];
        buffer->CopyData(lengthPrefix, 4);
        uint32_t length = (static_cast<uint32_t>(lengthPrefix[0]) << 24) |
                          (static_cast<uint32_t>(lengthPrefix[1]) << 16) |
                          (static_cast<uint32_t>(lengthPrefix[2]) << 8) |
                          static_cast<uint32_t>(lengthPrefix[3]);

        if (buffer->GetSize() < 4 + length)
        {
            break; // Rest of this message hasn't arrived yet
        }

        bundles.push_back(buffer->CreateFragment(4, length));
        buffer->RemoveAtStart(4 + length);
    }

    return bundles;
}

bool
TcpBundleCla::ConnectionRequest(Ptr<Socket> socket, const Address& from)
{
    NS_LOG_FUNCTION(this << socket << from);
    NS_LOG_DEBUG("Accepting connection request from " << from);
    return true;
}

void
TcpBundleCla::AcceptConnection(Ptr<Socket> socket, const Address& from)
{
    NS_LOG_FUNCTION(this << socket << from);
    NS_LOG_DEBUG("Connection accepted from " << from);

    m_acceptedSockets.push_back(socket);

    socket->SetRecvCallback(MakeCallback(&TcpBundleCla::HandleRead, this));
}

void
TcpBundleCla::HandleRead(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    Ptr<Packet>& buffer = m_rxBuffers[socket];
    if (!buffer)
    {
        buffer = Create<Packet>();
    }

    Ptr<Packet> packet;
    Address from;

    while ((packet = socket->RecvFrom(from)))
    {
        if (packet->GetSize() == 0)
        {
            break;
        }

        NS_LOG_DEBUG("Received TCP packet of size " << packet->GetSize() << " from " << from);
        buffer->AddAtEnd(packet);
    }

    for (const auto& bundlePacket : ExtractFramedBundles(buffer))
    {
        Ptr<Bundle> bundle = CreateObject<Bundle>();
        bundle->Deserialize(bundlePacket);
        ForwardUp(bundle);
    }
}

bool
TcpBundleCla::IsUp() const
{
    return m_isUp;
}

} // namespace ns3
