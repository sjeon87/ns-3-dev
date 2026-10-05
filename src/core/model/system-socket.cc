/*
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include "system-socket.h"

#include <algorithm>
#include <cerrno>
#include <climits>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace ns3
{
namespace
{
#ifdef _WIN32
using NativeHandle = SOCKET;
using AddressLength = int;

/** Winsock process lifetime. */
struct WinsockLifetime
{
    bool ready; ///< Whether initialization succeeded.

    WinsockLifetime()
    {
        WSADATA data{};
        ready = WSAStartup(MAKEWORD(2, 2), &data) == 0;
    }

    ~WinsockLifetime()
    {
        if (ready)
        {
            WSACleanup();
        }
    }
};
#else
using NativeHandle = int;
using AddressLength = socklen_t;
#endif

/** @return Whether native networking is initialized. */
bool
Initialize()
{
#ifdef _WIN32
    static WinsockLifetime lifetime;
    return lifetime.ready;
#else
    return true;
#endif
}

/** @return Whether the last native call was interrupted. */
bool
Interrupted()
{
#ifdef _WIN32
    return WSAGetLastError() == WSAEINTR;
#else
    return errno == EINTR;
#endif
}

/**
 * Configure low latency and protection against broken pipe signals.
 * @param socket Native socket.
 */
void
Configure(NativeHandle socket)
{
    int one = 1;
    ::setsockopt(socket,
                 IPPROTO_TCP,
                 TCP_NODELAY,
                 reinterpret_cast<const char*>(&one),
                 sizeof(one));
#ifdef SO_NOSIGPIPE
    ::setsockopt(socket, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
#endif
}

/**
 * Resolve and open an IPv4 socket.
 * @param host Hostname or address.
 * @param port Endpoint port.
 * @param protocol Transport protocol.
 * @param listening Whether to bind and listen.
 * @param backlog Pending connection limit.
 * @return Open handle or -1.
 */
SystemSocket::Handle
Open(const std::string& host,
     uint16_t port,
     SystemSocket::Protocol protocol,
     bool listening,
     int backlog)
{
    if (!Initialize() || (listening && protocol == SystemSocket::Protocol::UDP))
    {
        return -1;
    }
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = protocol == SystemSocket::Protocol::UDP ? SOCK_DGRAM : SOCK_STREAM;
    hints.ai_protocol = protocol == SystemSocket::Protocol::UDP ? IPPROTO_UDP : IPPROTO_TCP;
    if (protocol == SystemSocket::Protocol::SCTP)
    {
#ifdef __linux__
        hints.ai_protocol = IPPROTO_SCTP;
#else
        return -1;
#endif
    }
    hints.ai_flags = listening ? AI_PASSIVE : 0;
    const std::string endpoint = host.empty() ? (listening ? "0.0.0.0" : "127.0.0.1") : host;
    addrinfo* addresses = nullptr;
    if (::getaddrinfo(endpoint.c_str(), std::to_string(port).c_str(), &hints, &addresses) != 0)
    {
        return -1;
    }
    SystemSocket::Handle result = -1;
    for (auto* address = addresses; address; address = address->ai_next)
    {
        auto socket = ::socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        if (static_cast<SystemSocket::Handle>(socket) == -1)
        {
            continue;
        }
        Configure(socket);
        int rc;
        if (listening)
        {
            int one = 1;
            ::setsockopt(socket,
                         SOL_SOCKET,
                         SO_REUSEADDR,
                         reinterpret_cast<const char*>(&one),
                         sizeof(one));
            rc = ::bind(socket, address->ai_addr, static_cast<AddressLength>(address->ai_addrlen));
            if (rc == 0)
            {
                rc = ::listen(socket, backlog);
            }
        }
        else
        {
            rc = ::connect(socket,
                           address->ai_addr,
                           static_cast<AddressLength>(address->ai_addrlen));
        }
        if (rc == 0 && protocol == SystemSocket::Protocol::UDP)
        {
#ifdef _WIN32
            u_long nonblocking = 1;
            rc = ::ioctlsocket(socket, FIONBIO, &nonblocking);
#else
            const int flags = ::fcntl(socket, F_GETFL, 0);
            rc = flags < 0 ? -1 : ::fcntl(socket, F_SETFL, flags | O_NONBLOCK);
#endif
        }
        if (rc == 0)
        {
            result = static_cast<SystemSocket::Handle>(socket);
            break;
        }
        SystemSocket::Close(static_cast<SystemSocket::Handle>(socket));
    }
    ::freeaddrinfo(addresses);
    return result;
}
} // namespace

SystemSocket::Handle
SystemSocket::Connect(const std::string& host, uint16_t port, Protocol protocol)
{
    return Open(host, port, protocol, false, 0);
}

SystemSocket::Handle
SystemSocket::Listen(const std::string& host, uint16_t port, int backlog, Protocol protocol)
{
    return Open(host, port, protocol, true, backlog);
}

SystemSocket::Handle
SystemSocket::Accept(Handle listener, uint32_t timeoutMs)
{
    if (!WaitReadable(listener, timeoutMs))
    {
        return -1;
    }
    const auto socket = ::accept(static_cast<NativeHandle>(listener), nullptr, nullptr);
    if (static_cast<Handle>(socket) != -1)
    {
        Configure(socket);
    }
    return static_cast<Handle>(socket);
}

std::ptrdiff_t
SystemSocket::Send(Handle socket, const uint8_t* data, std::size_t size)
{
    if (socket < 0)
    {
        return -1;
    }
    int flags = 0;
#ifdef MSG_NOSIGNAL
    flags = MSG_NOSIGNAL;
#endif
    std::ptrdiff_t result;
    do
    {
        result = ::send(static_cast<NativeHandle>(socket),
                        reinterpret_cast<const char*>(data),
                        static_cast<int>(std::min(size, static_cast<std::size_t>(INT_MAX))),
                        flags);
    } while (result < 0 && Interrupted());
    return result;
}

std::ptrdiff_t
SystemSocket::Receive(Handle socket, uint8_t* data, std::size_t size)
{
    if (socket < 0)
    {
        return -1;
    }
    std::ptrdiff_t result;
    do
    {
        result = ::recv(static_cast<NativeHandle>(socket),
                        reinterpret_cast<char*>(data),
                        static_cast<int>(std::min(size, static_cast<std::size_t>(INT_MAX))),
                        0);
    } while (result < 0 && Interrupted());
    return result;
}

bool
SystemSocket::WaitReadable(Handle socket, uint32_t timeoutMs)
{
    if (socket < 0)
    {
        return false;
    }
#ifdef _WIN32
    WSAPOLLFD descriptor{};
#else
    pollfd descriptor{};
#endif
    descriptor.fd = static_cast<NativeHandle>(socket);
    descriptor.events = POLLIN;
    const int timeout = static_cast<int>(std::min(timeoutMs, static_cast<uint32_t>(INT_MAX)));
#ifdef _WIN32
    const int result = ::WSAPoll(&descriptor, 1, timeout);
#else
    const int result = ::poll(&descriptor, 1, timeout);
#endif
    return result > 0 && (descriptor.revents & (POLLIN | POLLHUP)) != 0;
}

void
SystemSocket::Close(Handle socket)
{
    if (socket < 0)
    {
        return;
    }
#ifdef _WIN32
    ::shutdown(static_cast<NativeHandle>(socket), SD_BOTH);
    ::closesocket(static_cast<NativeHandle>(socket));
#else
    ::shutdown(static_cast<NativeHandle>(socket), SHUT_RDWR);
    ::close(static_cast<NativeHandle>(socket));
#endif
}

uint16_t
SystemSocket::GetLocalPort(Handle socket)
{
    sockaddr_in address{};
    AddressLength size = sizeof(address);
    return socket >= 0 && ::getsockname(static_cast<NativeHandle>(socket),
                                        reinterpret_cast<sockaddr*>(&address),
                                        &size) == 0
               ? ntohs(address.sin_port)
               : 0;
}
} // namespace ns3
