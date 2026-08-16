/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef SOURCE_APPLICATION_H
#define SOURCE_APPLICATION_H

#include "seq-ts-size-header.h"

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/traced-callback.h"

#include <optional>

namespace ns3
{

class Packet;
class Socket;
class UniformRandomVariable;

/**
 * @ingroup applications
 * @brief Base class for source applications.
 *
 * This class can be used as a base class for source applications.
 * A source application is one that primarily sources new data towards a single remote client
 * address and port, and may also receive data (such as an HTTP server).
 *
 * The main purpose of this base class application public API is to provide a uniform way to
 * configure remote and local addresses.
 *
 * Unlike the SinkApplication, the SourceApplication does not expose an individual Port attribute.
 * Instead, the port values are embedded in the Local and Remote address attributes, which should be
 * configured to an InetSocketAddress or Inet6SocketAddress value that contains the desired port
 * number.
 *
 * If the attribute "EnableSeqTsSizeHeader" is enabled, the application will use some bytes of the
 * payload to store an header with a sequence number, a timestamp, and the size of the packet sent
 * (the latter is only present for NS3_SOCK_STREAM socket). Support for extracting statistics from
 * this header have been added to \c ns3::SinkApplication (enable its "EnableSeqTsSizeHeader"
 * attribute), or users may extract the header via trace sources.
 */
class SourceApplication : public Application
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Constructor
     * @param allowPacketSocket flag whether the application should allow the use of packet sockets
     * (enabled if not specified)
     * @param incrementCounterIfTxFailed flag whether to increment the sequence number counter if
     * the transmission of the packet failed (disabled if not specified)
     */
    SourceApplication(bool allowPacketSocket = true, bool incrementCounterIfTxFailed = false);
    ~SourceApplication() override;

    int64_t AssignStreams(int64_t stream) override;

    /**
     * @brief set the remote address
     * @param addr remote address
     */
    virtual void SetRemote(const Address& addr);

    /**
     * @brief get the remote address
     * @return the remote address
     */
    Address GetRemote() const;

    /**
     * @brief Get the socket this application is attached to.
     * @return pointer to associated socket
     */
    Ptr<Socket> GetSocket() const;

    /// Enumeration to specify whether to increment the sequence number counter if the transmission
    /// of the packet failed.
    enum class IncrementCounterIfTxFailed : uint8_t
    {
        UNDEFINED = 0, //!< Undefined: use the default behavior defined by the child class
        ENABLED,       //!< Always increment the counter even if the transmission failed
        DISABLED       //!< Do not increment the counter if the transmission failed
    };

  protected:
    void DoInitialize() override;
    void DoDispose() override;

    /**
     * @brief Close the socket
     * @return true if the socket was closed, false if there was no socket to close
     */
    bool CloseSocket();

    /**
     * @brief This method creates a packet of the given size. If the SeqTsSizeHeader attribute is
     * enabled, it adds the header to the packet, unless the size of the packet is less than the
     * size of the header.
     *
     * @param size the size of the packet to create in bytes
     * @return the created packet
     */
    virtual Ptr<Packet> CreatePacket(uint64_t size);

    /**
     * @brief Send a packet through the socket and increments the sequence number counter if the
     * transmission was successful or if the IncrementCounterIfTxFailed attribute is enabled.
     *
     * @param packet the packet to send
     * @return the number of bytes sent, or -1 if the transmission failed
     */
    int SendPacket(Ptr<Packet> packet);

    /// Traced Callback: transmitted packets.
    TracedCallback<Ptr<const Packet>> m_txTrace;

    /**
     * TracedCallback signature for connection success/failure event.
     *
     * @param [in] socket The socket for which connection succeeded/failed.
     * @param [in] local The local address.
     * @param [in] remote The remote address.
     */
    typedef void (*ConnectionEventCallback)(Ptr<Socket> socket,
                                            const Address& local,
                                            const Address& remote);

    /// Callback for tracing the packet Tx events, includes source, destination, the packet sent,
    /// and header if EnableSeqTsSizeHeader is enabled (for NS3_SOCK_STREAM sockets only)
    TracedCallback<Ptr<const Packet>, const Address&, const Address&, const SeqTsSizeHeader&>
        m_txTraceWithSeqTsSize;

    /// Callback for tracing the packet Tx events, includes source, destination, the packet sent,
    /// and header if EnableSeqTsSizeHeader is enabled (for NS3_SOCK_DGRAM sockets only)
    TracedCallback<Ptr<const Packet>, const Address&, const Address&, const SeqTsHeader&>
        m_txTraceWithSeqTs;

    /// Traced Callback: connection success event.
    TracedCallback<Ptr<Socket>, const Address&, const Address&> m_connectionSuccess;

    /// Traced Callback: connection failure event.
    TracedCallback<Ptr<Socket>, const Address&, const Address&> m_connectionFailure;

    Ptr<Socket> m_socket; //!< Socket

    TypeId m_protocolTid; //!< Protocol TypeId value

    Address m_peer;  //!< Peer address
    Address m_local; //!< Local address to bind to
    uint8_t m_tos;   //!< The packets Type of Service

    bool m_enableSeqTsSizeHeader{false}; //!< Enable or disable the use of SeqTsSizeHeader
    bool m_randomPayload{false};         //!< Fill packet payload with random bytes

    bool m_connected{false}; //!< flag whether socket is connected

  private:
    void StartApplication() override;
    void StopApplication() override;

    /**
     * @brief Set the flag whether to increment the sequence number counter if the transmission of
     * the packet failed. If the option is set to UNDEFINED, it is ignored to ensure default
     * behavior defined by the child class is used.
     * @param option the option value to set
     */
    void SetIncrementCounterIfTxFailed(IncrementCounterIfTxFailed option);

    /**
     * @brief Get the flag whether to increment the sequence number counter if the transmission of
     * the packet failed.
     * @return the option value
     */
    IncrementCounterIfTxFailed GetIncrementCounterIfTxFailed() const;

    /**
     * @brief Handle a Connection Succeed event
     * @param socket the connected socket
     */
    void ConnectionSucceeded(Ptr<Socket> socket);

    /**
     * @brief Handle a Connection Failed event
     * @param socket the not connected socket
     */
    void ConnectionFailed(Ptr<Socket> socket);

    /**
     * @brief Application specific startup code for child subclasses
     */
    virtual void DoStartApplication();

    /**
     * @brief Application specific shutdown code for child subclasses
     */
    virtual void DoStopApplication();

    /**
     * @brief Application specific code for child subclasses upon a Connection Succeed event
     * @param socket the connected socket
     */
    virtual void DoConnectionSucceeded(Ptr<Socket> socket);

    /**
     * @brief Application specific code for child subclasses upon a Connection Failed event
     * @param socket the not connected socket
     */
    virtual void DoConnectionFailed(Ptr<Socket> socket);

    /**
     * @brief Cancel all pending events.
     */
    virtual void CancelEvents() = 0;

    /**
     * @brief Create a packet with a SeqTsSizeHeader
     * @param seq the sequence number to set in the header
     * @param size the size to set in the header
     * @return the created packet
     */
    Ptr<Packet> CreatePacketWithSeqTsSizeHeader(uint32_t seq, uint64_t size);

    /**
     * @brief Create a packet with a payload, either filled in with zeros or random bytes depending
     * on the setting of the RandomPayload attribute
     * @param size the size of the payload
     * @return the created packet
     */
    Ptr<Packet> CreatePacketWithPayload(uint64_t size);

    bool m_allowPacketSocket;          //!< Allow use of packet socket
    bool m_incrementCounterIfTxFailed; //!< Flag whether to increment the sequence number counter if
                                       //!< the transmission of the packet failed
    Ptr<UniformRandomVariable> m_bytesRng; //!< Random variable for random payload generation

    uint32_t m_seq{0}; //!< Sequence number for created packets
};

} // namespace ns3

#endif /* SOURCE_APPLICATION_H */
