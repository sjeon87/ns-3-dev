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
#ifndef UDP_BUNDLE_CLA_H
#define UDP_BUNDLE_CLA_H

#include "generic-convergence-layer-adapter.h"

#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/socket.h"

namespace ns3
{

/**
 * @ingroup dtn
 *
 * @brief UDP-based Convergence Layer Adapter for the Bundle Protocol.
 *
 * This class implements a datagram-based CLA. It manages an underlying
 * ns-3 UDP socket to send and receive serialized bundles.
 */
class UdpBundleCla : public BundleCla
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    UdpBundleCla();
    ~UdpBundleCla() override;

    /**
     * @brief Configure and start the UDP CLA.
     * * Creates the UDP socket, binds it to the local address to listen
     * for incoming bundles, and sets the remote destination for outbound bundles.
     * * @param node The ns-3 Node this CLA resides on.
     * @param localAddress The local address and port to bind to.
     * @param remoteAddress The destination address and port for sending data.
     */
    void Setup(Ptr<Node> node, Address localAddress, Address remoteAddress);

    /**
     * @brief Send a serialized bundle packet over the UDP socket.
     * @param packet The serialized bundle to send.
     */
    void Send(Ptr<Packet> packet) override;

    /**
     * @brief Check if the CLA is ready to send/receive data.
     * @return true if the socket is successfully created and bound.
     */
    bool IsUp() const override;

  private:
    /**
     * @brief Callback invoked by the ns-3 Socket when UDP data arrives.
     * @param socket The socket receiving the data.
     */
    void HandleRead(Ptr<Socket> socket);

    Ptr<Socket> m_socket;    //!< The underlying ns-3 UDP socket
    Address m_remoteAddress; //!< Where to send outbound packets
};

} // namespace ns3

#endif /* UDP_BUNDLE_CLA_H */
