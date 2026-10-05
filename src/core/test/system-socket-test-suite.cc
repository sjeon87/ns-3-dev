/*
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include "ns3/system-socket.h"
#include "ns3/test.h"

#include <array>

using namespace ns3;

/** @ingroup tests
 * Verify host TCP framing, readiness, and peer closure through a loopback connection.
 */
class SystemSocketTestCase : public TestCase
{
  public:
    SystemSocketTestCase()
        : TestCase("Host socket loopback exchange and closure")
    {
    }

  private:
    void DoRun() override
    {
        /** Socket lifetime guard for assertion failures. */
        struct SocketGuard
        {
            SystemSocket::Handle handle{-1}; ///< Owned socket.

            ~SocketGuard()
            {
                SystemSocket::Close(handle);
            }
        };

        SocketGuard listener{SystemSocket::Listen("127.0.0.1", 0)};
        NS_TEST_ASSERT_MSG_NE(listener.handle, -1, "Listener creation failed");
        const auto port = SystemSocket::GetLocalPort(listener.handle);
        NS_TEST_ASSERT_MSG_NE(port, 0, "Ephemeral port was not assigned");
        SocketGuard client{SystemSocket::Connect("127.0.0.1", port)};
        NS_TEST_ASSERT_MSG_NE(client.handle, -1, "Connection failed");
        SocketGuard server{SystemSocket::Accept(listener.handle, 1000)};
        NS_TEST_ASSERT_MSG_NE(server.handle, -1, "Accept failed");
        NS_TEST_ASSERT_MSG_EQ(SystemSocket::WaitReadable(server.handle, 0),
                              false,
                              "Idle connection reported data");
        const std::array<uint8_t, 5> sent{0, 1, 127, 128, 255};
        NS_TEST_ASSERT_MSG_EQ(SystemSocket::Send(client.handle, sent.data(), sent.size()),
                              static_cast<std::ptrdiff_t>(sent.size()),
                              "Send failed");
        std::array<uint8_t, 5> received{};
        size_t offset = 0;
        while (offset < received.size())
        {
            NS_TEST_ASSERT_MSG_EQ(SystemSocket::WaitReadable(server.handle, 1000),
                                  true,
                                  "Data did not arrive");
            const auto count = SystemSocket::Receive(server.handle,
                                                     received.data() + offset,
                                                     received.size() - offset);
            NS_TEST_ASSERT_MSG_GT(count, 0, "Read failed");
            offset += static_cast<size_t>(count);
        }
        NS_TEST_ASSERT_MSG_EQ(received == sent, true, "Binary payload changed");
        SystemSocket::Close(client.handle);
        client.handle = -1;
        NS_TEST_ASSERT_MSG_EQ(SystemSocket::WaitReadable(server.handle, 1000),
                              true,
                              "Peer closure did not become readable");
        NS_TEST_ASSERT_MSG_EQ(
            SystemSocket::Receive(server.handle, received.data(), received.size()),
            0,
            "Peer closure was not reported");
    }
};

/** @ingroup tests
 * Host networking test suite.
 */
class SystemSocketTestSuite : public TestSuite
{
  public:
    SystemSocketTestSuite()
        : TestSuite("system-socket", Type::UNIT)
    {
        AddTestCase(new SystemSocketTestCase, TestCase::Duration::QUICK);
    }
};

static SystemSocketTestSuite g_systemSocketTestSuite; ///< Register the suite.
