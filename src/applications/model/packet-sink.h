/*
 * Copyright 2007 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:  Tom Henderson (tomhend@u.washington.edu)
 */

#ifndef PACKET_SINK_H
#define PACKET_SINK_H

#include "sink-application.h"

#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"

#include <unordered_map>

namespace ns3
{

class Packet;

/**
 * @ingroup applications
 * @defgroup packetsink PacketSink
 *
 * This application was written to complement OnOffApplication, but it
 * is more general so a PacketSink name was selected.  Functionally it is
 * important to use in multicast situations, so that reception of the layer-2
 * multicast frames of interest are enabled, but it is also useful for
 * unicast as an example of how you can write something simple to receive
 * packets at the application layer.  Also, if an IP stack generates
 * ICMP Port Unreachable errors, receiving applications will be needed.
 */

/**
 * @ingroup packetsink
 *
 * @brief Receive and consume traffic generated to an IP address and port
 *
 * This application was written to complement OnOffApplication, but it
 * is more general so a PacketSink name was selected.  Functionally it is
 * important to use in multicast situations, so that reception of the layer-2
 * multicast frames of interest are enabled, but it is also useful for
 * unicast as an example of how you can write something simple to receive
 * packets at the application layer.  Also, if an IP stack generates
 * ICMP Port Unreachable errors, receiving applications will be needed.
 *
 * The constructor specifies the Address (IP address and port) and the
 * transport protocol to use.   A virtual Receive () method is installed
 * as a callback on the receiving socket.  By default, when logging is
 * enabled, it prints out the size of packets and their address.
 * A tracing source to Receive() is also available.
 */
class PacketSink : public SinkApplication
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    PacketSink();
    ~PacketSink() override;

    /**
     * @return the total bytes received in this sink app
     */
    uint64_t GetTotalRx() const;

    /**
     * @return pointer to listening socket
     */
    Ptr<Socket> GetListeningSocket() const;

    /**
     * @return list of pointers to accepted sockets
     */
    std::list<Ptr<Socket>> GetAcceptedSockets() const;

  protected:
    void DoDispose() override;

  private:
    void DoStartApplication() override;
    void DoStopApplication() override;
    void ReceivePacket(Ptr<Socket> socket, Ptr<Packet> packet, const Address& from) override;

    /**
     * @brief Handle an incoming connection
     * @param socket the incoming connection socket
     * @param from the address the connection is from
     */
    void HandleAccept(Ptr<Socket> socket, const Address& from);
    /**
     * @brief Handle an connection close
     * @param socket the connected socket
     */
    void HandlePeerClose(Ptr<Socket> socket);
    /**
     * @brief Handle an connection error
     * @param socket the connected socket
     */
    void HandlePeerError(Ptr<Socket> socket);

    // In the case of TCP, each socket accept returns a new socket, so the
    // listening socket is stored separately from the accepted sockets
    std::list<Ptr<Socket>> m_socketList; //!< the accepted sockets

    uint64_t m_totalRx{0}; //!< Total bytes received

    /// Callback for tracing the packet Rx events, includes source and destination addresses
    TracedCallback<Ptr<const Packet>, const Address&, const Address&> m_rxTraceWithAddresses;

    uint8_t m_tos{0}; //!< Type of Service for outbound packets
};

} // namespace ns3

#endif /* PACKET_SINK_H */
