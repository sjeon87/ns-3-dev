/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */
#include "udp-bundle-cla.h"

#include "ns3/log.h"
#include "ns3/udp-socket-factory.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("UdpBundleCla");
NS_OBJECT_ENSURE_REGISTERED(UdpBundleCla);

TypeId
UdpBundleCla::GetTypeId()
{
    static TypeId tid = TypeId("ns3::UdpBundleCla")
                            .SetParent<BundleCla>()
                            .SetGroupName("BundleProtocol")
                            .AddConstructor<UdpBundleCla>();
    return tid;
}

UdpBundleCla::UdpBundleCla()
    : m_socket(nullptr)
{
    NS_LOG_FUNCTION(this);
}

UdpBundleCla::~UdpBundleCla()
{
    NS_LOG_FUNCTION(this);
    if (m_socket)
    {
        m_socket->Close();
        m_socket = nullptr;
    }
}

void
UdpBundleCla::Setup(Ptr<Node> node, Address localAddress, Address remoteAddress)
{
    NS_LOG_FUNCTION(this << node << localAddress << remoteAddress);
    m_remoteAddress = remoteAddress;

    m_socket = Socket::CreateSocket(node, UdpSocketFactory::GetTypeId());

    if (m_socket->Bind(localAddress) == -1)
    {
        NS_LOG_ERROR("Failed to bind UDP socket to local address");
        return;
    }

    m_socket->SetRecvCallback(MakeCallback(&UdpBundleCla::HandleRead, this));

    NS_LOG_DEBUG("UdpBundleCla configured. Listening on " << localAddress << ", sending to "
                                                          << m_remoteAddress);
}

void
UdpBundleCla::Send(Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this << packet->GetSize());

    if (!IsUp())
    {
        NS_LOG_WARN("Cannot send: Socket is not configured or down.");
        return;
    }

    int bytesSent = m_socket->SendTo(packet, 0, m_remoteAddress);

    if (bytesSent < 0)
    {
        NS_LOG_WARN("Socket SendTo failed.");
    }
    else
    {
        NS_LOG_DEBUG("Sent " << bytesSent << " bytes via UDP to " << m_remoteAddress);
    }
}

bool
UdpBundleCla::IsUp() const
{
    return (m_socket != nullptr);
}

void
UdpBundleCla::HandleRead(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    Ptr<Packet> packet;
    Address from;

    while ((packet = socket->RecvFrom(from)))
    {
        NS_LOG_DEBUG("Received UDP datagram of size " << packet->GetSize() << " from " << from);
        Ptr<Bundle> bundle = CreateObject<Bundle>();
        bundle->Deserialize(packet);
        ForwardUp(bundle);
    }
}

} // namespace ns3
