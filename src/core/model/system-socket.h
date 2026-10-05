/*
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef NS3_SYSTEM_SOCKET_H
#define NS3_SYSTEM_SOCKET_H

#include <cstddef>
#include <cstdint>
#include <string>

namespace ns3
{
/**
 * @ingroup core
 * Host operating system sockets for external services, independent of simulated sockets.
 */
class SystemSocket
{
  public:
    using Handle = std::intptr_t; ///< Native socket handle; -1 indicates failure.
    /** Transport supported by the host operating system. */
    enum class Protocol
    {
        TCP,  ///< TCP stream.
        SCTP, ///< SCTP stream, available on Linux with kernel support.
        UDP   ///< Nonblocking connected UDP socket.
    };

    /**
     * Connect to an IPv4 endpoint.
     * @param host Hostname or address; empty selects loopback.
     * @param port Destination port.
     * @param protocol Transport protocol.
     * @return Connected handle, or -1 on failure or unsupported protocol.
     */
    static Handle Connect(const std::string& host,
                          uint16_t port,
                          Protocol protocol = Protocol::TCP);
    /**
     * Listen on an IPv4 endpoint.
     * @param host Hostname or address; empty selects all interfaces.
     * @param port Listening port; zero requests an ephemeral port.
     * @param backlog Maximum pending connections.
     * @param protocol Stream transport protocol.
     * @return Listening handle, or -1 on failure.
     */
    static Handle Listen(const std::string& host,
                         uint16_t port,
                         int backlog = 8,
                         Protocol protocol = Protocol::TCP);
    /**
     * Accept a pending stream connection.
     * @param listener Listening handle.
     * @param timeoutMs Maximum wait in milliseconds.
     * @return Accepted handle, or -1 on timeout or failure.
     */
    static Handle Accept(Handle listener, uint32_t timeoutMs);
    /**
     * Write bytes, retrying interrupted system calls.
     * @param socket Connected handle.
     * @param data Bytes to send.
     * @param size Number of bytes.
     * @return Bytes sent, or -1 on failure; stream writes may be partial.
     */
    static std::ptrdiff_t Send(Handle socket, const uint8_t* data, std::size_t size);
    /**
     * Read bytes, retrying interrupted system calls.
     * @param socket Connected handle.
     * @param data Destination buffer.
     * @param size Buffer capacity.
     * @return Bytes read, zero on stream closure, or -1 on failure.
     */
    static std::ptrdiff_t Receive(Handle socket, uint8_t* data, std::size_t size);
    /**
     * Wait for input or peer closure.
     * @param socket Socket handle.
     * @param timeoutMs Maximum wait in milliseconds.
     * @return Whether a read can proceed.
     */
    static bool WaitReadable(Handle socket, uint32_t timeoutMs);
    /**
     * Shut down and close a handle; invalid handles are ignored.
     * @param socket Handle to close.
     */
    static void Close(Handle socket);
    /**
     * Get the bound local port.
     * @param socket Socket handle.
     * @return Local port, or zero on failure.
     */
    static uint16_t GetLocalPort(Handle socket);
};
} // namespace ns3
#endif
