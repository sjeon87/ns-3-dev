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
#ifndef TCP_CONVERGENCE_LAYER_ADAPTER_H
#define TCP_CONVERGENCE_LAYER_ADAPTER_H

#include "generic-convergence-layer-adapter.h"

#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/socket.h"

#include <queue>
#include <utility>
#include <vector>

namespace ns3
{

/**
 * @ingroup dtn
 *
 * @brief A Transmission Control Protocol (TCP) Convergence Layer Adapter (CLA) for the Bundle
 * Protocol.
 *
 * This class implements a TCP-based CLA. It manages both an incoming listening socket (server-side)
 * and an outgoing connection socket (client-side) to facilitate bidirectional bundle transmission
 * over TCP within the ns-3 simulation environment. Bundles sent before the TCP 3-way handshake
 * completes are queued and automatically flushed upon successful connection establishment.
 */
class TcpBundleCla : public BundleCla
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Default constructor. Initializes sockets and connection state.
     */
    TcpBundleCla();

    /**
     * @brief Destructor. Closes all active sockets and cleans up resources.
     */
    ~TcpBundleCla() override;

    /**
     * @brief Configure the TCP CLA to listen on a local port and connect to a remote address.
     *
     * @param node The local node instance where the sockets will be created.
     * @param localAddress The local IP address and port to bind and listen on.
     * @param remoteAddress The destination IP address and port to connect to.
     */
    void Setup(Ptr<Node> node, Address localAddress, Address remoteAddress);

    /**
     * @brief Send a serialized bundle packet via the TCP connection.
     *
     * @param packet The serialized bundle packet to send over the network.
     * @param bundleHandle The tracking handle for the bundle agent's storage.
     */
    void Send(Ptr<Packet> packet, uint32_t bundleHandle) override;

    /**
     * @brief Check if the CLA has been configured and is ready to process data.
     * @return True if Setup() has been called (Note: TCP connection may still be pending).
     */
    bool IsUp() const override;

  private:
    /**
     * @brief Callback invoked when the outgoing TCP connection handshake succeeds.
     * @param socket The socket that successfully connected to the remote peer.
     */
    void ConnectionSucceeded(Ptr<Socket> socket);

    /**
     * @brief Callback invoked when the outgoing TCP connection handshake fails.
     * @param socket The socket that failed to connect.
     */
    void ConnectionFailed(Ptr<Socket> socket);

    /**
     * @brief Callback invoked when a remote node attempts to connect to the local listening socket.
     * @param socket The local listening socket receiving the request.
     * @param from The address of the remote node requesting the connection.
     * @return True to accept the incoming connection, false to reject it.
     */
    bool ConnectionRequest(Ptr<Socket> socket, const Address& from);

    /**
     * @brief Callback invoked when an incoming TCP connection is successfully accepted.
     * @param socket The newly spawned socket dedicated to this specific accepted connection.
     * @param from The address of the connected remote peer.
     */
    void AcceptConnection(Ptr<Socket> socket, const Address& from);

    /**
     * @brief Callback invoked when data is available to be read from any connected socket.
     * @param socket The socket containing the incoming network data.
     */
    void HandleRead(Ptr<Socket> socket);

    Ptr<Socket> m_listenSocket; //!< Socket to listen for incoming connections
    Ptr<Socket> m_sendSocket;   //!< Socket to initiate outgoing connection
    std::vector<Ptr<Socket>>
        m_acceptedSockets; //!< List to track and keep alive incoming connection sockets

    Address m_remoteAddress; //!< The destination address we are attempting to connect to
    bool m_connected; //!< Flag indicating if the outgoing TCP connection is fully established
    bool m_isUp;      //!< Flag indicating if the Setup method has been executed

    std::queue<std::pair<Ptr<Packet>, uint32_t>>
        m_sendQueue; //!< Queue for packets generated before the connection is established
};

} // namespace ns3

#endif /* TCP_CONVERGENCE_LAYER_ADAPTER_H */
