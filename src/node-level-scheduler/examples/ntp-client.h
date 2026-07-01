/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#ifndef NTP_CLIENT_H
#define NTP_CLIENT_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ipv4-address.h"
#include "ns3/local-clock.h"
#include "ns3/nstime.h"
#include "ns3/socket.h"

namespace ns3
{

/**
 * @brief Minimal NTP client application.
 *
 * Periodically polls a server with the classic four-timestamp NTP exchange, computes the
 * resulting clock offset and round-trip delay, and disciplines its own local clock by
 * slewing its rate to close the offset over the next poll interval.
 */
class NtpClient : public Application
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    NtpClient();
    ~NtpClient() override;

    /**
     * @brief Configure the client.
     * @param serverAddress the server's IPv4 address.
     * @param serverPort the server's UDP port.
     * @param pollInterval how often to poll the server.
     */
    void Setup(Ipv4Address serverAddress, uint16_t serverPort, Time pollInterval);

  private:
    void StartApplication() override;
    void StopApplication() override;

    /**
     * @brief Send an NTP request to the server, stamped with the local clock's current time.
     */
    void SendRequest();

    /**
     * @brief Handle an NTP reply, compute offset, and slew the local clock.
     * @param socket the receiving socket.
     */
    void HandleRead(Ptr<Socket> socket);

    Ptr<Socket> m_socket;    //!< The client's UDP socket
    Address m_peer;          //!< The server's address
    Time m_pollInterval;     //!< How often to poll the server
    EventId m_pollEvent;     //!< The next scheduled poll
    Ptr<LocalClock> m_clock; //!< This node's local clock
};

} // namespace ns3

#endif /* NTP_CLIENT_H */
