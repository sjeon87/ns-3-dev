/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#ifndef NTP_SERVER_H
#define NTP_SERVER_H

#include "ns3/application.h"
#include "ns3/local-clock.h"
#include "ns3/nstime.h"
#include "ns3/socket.h"

namespace ns3
{

/**
 * @brief Minimal NTP server application.
 *
 * Timestamps each request with its own local clock and echoes the
 * request's T1 plus its own T2 (receive) and T3 (send) timestamps back to the client, after
 * a processing delay.
 */
class NtpServer : public Application
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    NtpServer();
    ~NtpServer() override;

    /**
     * @brief Configure the server.
     * @param port the UDP port to listen on.
     * @param processingDelay simulated time taken to build and send the reply.
     */
    void Setup(uint16_t port, Time processingDelay);

  private:
    void StartApplication() override;
    void StopApplication() override;

    /**
     * @brief Handle an incoming NTP request.
     * @param socket the receiving socket.
     */
    void HandleRead(Ptr<Socket> socket);

    /**
     * @brief Send the NTP reply after the simulated processing delay has elapsed.
     * @param from the client's address.
     * @param t1 the client's send timestamp, echoed back unchanged.
     * @param t2 this server's receive timestamp.
     */
    void SendReply(Address from, Time t1, Time t2);

    Ptr<Socket> m_socket;    //!< The server's UDP socket
    uint16_t m_port;         //!< The UDP port to listen on
    Time m_processingDelay;  //!< Simulated request processing time
    Ptr<LocalClock> m_clock; //!< This node's local clock
};

} // namespace ns3

#endif /* NTP_SERVER_H */
