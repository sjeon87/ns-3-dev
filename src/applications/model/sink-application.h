/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef SINK_APPLICATION_H
#define SINK_APPLICATION_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/traced-callback.h"

#include <limits>

namespace ns3
{

class Packet;
class Address;
class Socket;

/**
 * @ingroup applications
 * @brief Base class for sink applications.
 *
 * This class can be used as a base class for sink applications.
 * A sink application is an application that is primarily used to only receive or echo packets.
 *
 * The main purpose of this base class application public API is to hold attributes for the local
 * (IPv4 or IPv6) address and port to bind to.
 *
 * There are three ways that the port value can be configured. First, and most typically, through
 * the use of a socket address (InetSocketAddress or Inet6SocketAddress) that is configured as the
 * Local address to bind to. Second, through direct configuration of the Port attribute. Third,
 * through the use of an optional constructor argument. If multiple of these port configuration
 * methods are used, it is up to subclass definition which one takes precedence; in the existing
 * subclasses in this directory, the port value configured in the Local socket address (if a socket
 * address is configured there) will take precedence.
 *
 * It is also possible to leave the local address unspecified, in which case the application
 * will open two sockets (one for each of IPv4 and IPv6) and listen for connections on both
 * from any connection request that matches the configured port number.
 *
 * The type of socket(s) created will be based on the protocol TypeId that is specified by the
 * subclass, and the corresponding IpL4Protocol (e.g., TcpL4Protocol or UdpL4Protocol)
 * configuration set on the node.  However, the SetPrimarySocket() and SetDualStackSocket()
 * methods can be used to instead specialize the socket type used by the application and
 * bypass the default socket creation by the internet stack.
 */
class SinkApplication : public Application
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Constructor
     *
     * @param defaultPort the default port number
     */
    SinkApplication(uint16_t defaultPort = 0);
    ~SinkApplication() override;

    static constexpr uint32_t INVALID_PORT{std::numeric_limits<uint32_t>::max()}; //!< invalid port

    /**
     * @brief Set the primary socket for this application.
     *
     * This replaces the primary socket that would be created at StartApplication time.
     * This socket can either be an IPv4- or IPv6-based socket depending on how the Local
     * address attribute is configured.  The socket passed to this method should not yet
     * be used (connected or bound). The socket passed as an argument to this method allows
     * the user to specialize the socket type that will be used.  One use case for this
     * method is to allow different TCP variants to be configured on the same node.
     *
     * @note This method will trigger a simulation abort if called after StartApplication has
     * executed.
     *
     * @param socket The socket to associate to the application
     */
    void SetPrimarySocket(Ptr<Socket> socket);

    /**
     * @brief Get the primary socket that this application is attached to, if any.
     *
     * @return pointer to associated socket
     */
    Ptr<Socket> GetPrimarySocket() const;

    /**
     * @brief Set the secondary socket for this application, for use as the IPv6 socket type
     * in a dual stack configuration.
     *
     * If the Local address attribute is unspecified, and only in this case, this application
     * will open both an IPv4 and an IPv6 socket, and in that case, the IPv4 socket will
     * be the "primary" socket and the IPv6 socket will be the "dual stack" socket.
     * The socket passed to this method should not yet
     * be used (connected or bound). The socket passed as an argument to this method allows
     * the user to specialize the socket type that will be used.  One use case for this
     * method is to allow different TCP variants to be configured on the same node.
     *
     * If the Local address attribute is set before StartApplication is called, only one
     * socket will be used (the primary socket) and this application will not use the
     * socket passed to this method and will release the reference to the smart pointer.
     *
     * @note This method will trigger a simulation abort if called after StartApplication has
     * executed.
     *
     * @note This intermediate base class does not enforce that the address family for this
     * socket is IPv6, but the existing subclasses use it in this way and the underlying
     * protected member variable is named m_socket6 to convey that it is used for IPv6.
     *
     * @param socket The socket to associate to the application
     */
    void SetDualStackSocket(Ptr<Socket> socket);

    /**
     * @brief Get the secondary (dual stack) socket that this application is attached to,
     * if any.
     *
     * @note This socket pointer will only be populated after the application has started
     * if the Local address attribute was unspecified at the starting time, and in that
     * case, this socket will be bound to an IPv6 address.
     *
     * @return pointer to associated socket
     */
    Ptr<Socket> GetDualStackSocket() const;

  protected:
    void DoDispose() override;

    /**
     * @brief Close all the sockets
     * @return true if all sockets closed successfully, false otherwise
     */
    bool CloseAllSockets();

    /// Callbacks for tracing the packet Rx events
    ns3::TracedCallback<Ptr<const Packet>> m_rxTraceWithoutAddress;

    /// Traced Callback: received packets, source address.
    TracedCallback<Ptr<const Packet>, const Address&> m_rxTrace;

    Ptr<Socket> m_socket;                 //!< Socket (IPv4 or IPv6, depending on local address)
    Ptr<Socket> m_pendingPrimarySocket;   //!< User-specified socket
    Ptr<Socket> m_socket6;                //!< IPv6 Socket (used if only port is specified)
    Ptr<Socket> m_pendingDualStackSocket; //!< User-specified IPv6-based socket

    TypeId m_protocolTid; //!< Protocol TypeId value

    Address m_local; //!< Local address to bind to (address and port)
    uint32_t m_port; //!< Local port to bind to

  private:
    void StartApplication() override;
    void StopApplication() override;

    /**
     * @brief set the local address
     * @param addr local address
     */
    virtual void SetLocal(const Address& addr);

    /**
     * @brief get the local address
     * @return the local address
     */
    Address GetLocal() const;

    /**
     * @brief set the server port
     * @param port server port
     */
    virtual void SetPort(uint32_t port);

    /**
     * @brief get the server port
     * @return the server port
     */
    uint32_t GetPort() const;

    /**
     * @brief Close the socket
     * @param socket the socket to close
     * @return true if the socket closed successfully, false otherwise
     */
    bool CloseSocket(Ptr<Socket> socket);

    /**
     * @brief Application specific startup code for child subclasses
     */
    virtual void DoStartApplication();

    /**
     * @brief Application specific shutdown code for child subclasses
     */
    virtual void DoStopApplication();

    bool m_hasStarted{false}; //!< Track whether the application has previously started
};

} // namespace ns3

#endif /* SINK_APPLICATION_H */
