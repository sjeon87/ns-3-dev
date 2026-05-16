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
#include "udp-convergence-layer-adapter.h"

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
UdpBundleCla::Send(Ptr<Packet> packet, uint32_t bundleHandle)
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

    if (!m_txResultCb.IsNull() && bundleHandle != 0)
    {
        m_txResultCb(bundleHandle, true);
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
        uint32_t size = packet->GetSize();
        auto* buf = new uint8_t[size];
        packet->CopyData(buf, size);
        Ptr<Packet> fresh = Create<Packet>(buf, size);
        delete[] buf;

        Ptr<Bundle> bundle = CreateObject<Bundle>();
        bundle->Deserialize(fresh);
        ForwardUp(bundle);
    }
}

} // namespace ns3
