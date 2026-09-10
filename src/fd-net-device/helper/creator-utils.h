/*
 * Copyright (c) University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef CREATOR_UTILS_H
#define CREATOR_UTILS_H

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#ifndef _WIN32
#include <arpa/inet.h>
#include <fcntl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

namespace ns3
{

/**
 * Magic number used to verify TAP creator IPC messages.
 */
constexpr int TAP_MAGIC = 95549;

/**
 * Magic number used to verify EMU creator IPC messages.
 */
constexpr int EMU_MAGIC = 65867;

extern bool gVerbose;

#define LOG(msg)                                                                                   \
    if (gVerbose)                                                                                  \
    {                                                                                              \
        std::cout << __FUNCTION__ << "(): " << msg << std::endl;                                   \
    }

#define ABORT(msg, printErrno)                                                                     \
    std::cout << __FILE__ << ": fatal error at line " << __LINE__ << ": " << __FUNCTION__          \
              << "(): " << msg << std::endl;                                                       \
    if (printErrno)                                                                                \
    {                                                                                              \
        std::cout << "    errno = " << errno << " (" << strerror(errno) << ")" << std::endl;       \
    }                                                                                              \
    exit(-1);

#define ABORT_IF(cond, msg, printErrno)                                                            \
    if (cond)                                                                                      \
    {                                                                                              \
        ABORT(msg, printErrno);                                                                    \
    }

#ifndef _WIN32
/**
 * @ingroup fd-net-device
 * @brief Send the file descriptor back to the code that invoked the creation.
 *
 * Uses POSIX Unix-domain sockets with SCM_RIGHTS ancillary data. Not available
 * on Windows — on Windows, TAP/TUN file descriptors are opened directly in the
 * helper without a privileged child process.
 *
 * @param path The socket address information from the Unix socket we use
 * to send the created socket back to.
 * @param fd The file descriptor we're going to send.
 * @param magic_number A verification number to verify the caller is talking to the
 * right process.
 */
void SendSocket(const char* path, int fd, const int magic_number);
#endif

} // namespace ns3

#endif /* CREATOR_UTILS_DEVICE_H */
