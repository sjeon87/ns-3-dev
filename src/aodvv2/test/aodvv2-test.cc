/*
 * Copyright (c) 2016 Universita' di Firenze
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Francesco Todino <francesco.todino@edu.unifi.it>
 *          Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "ns3/internet-stack-helper.h"
#include "ns3/simulator.h"
#include "ns3/test.h"

#include <limits>
#include <string>

using namespace ns3;

/**
 * \ingroup internet-test
 *
 * \brief AODVv2 Test
 */
class Aodvv2Test : public TestCase
{
    Ptr<Packet> m_receivedPacket; //!< Received packet

    /**
     * \brief Send data.
     * \param socket The sending socket.
     * \param to Destination address.
     */
    void DoSendData(Ptr<Socket> socket, std::string to);
    /**
     * \brief Send data.
     * \param socket The sending socket.
     * \param to Destination address.
     */
    void SendData(Ptr<Socket> socket, std::string to);

  public:
    void DoRun() override;
    Aodvv2Test();

    /**
     * \brief Receive data.
     * \param socket The receiving socket.
     */
    void ReceivePkt(Ptr<Socket> socket);
};

Aodvv2Test::Aodvv2Test()
    : TestCase("AODVv2")
{
}

void
Aodvv2Test::ReceivePkt(Ptr<Socket> socket)
{
    uint32_t availableData [[maybe_unused]] = socket->GetRxAvailable();
    m_receivedPacket = socket->Recv(std::numeric_limits<uint32_t>::max(), 0);
    NS_TEST_ASSERT_MSG_EQ(availableData,
                          m_receivedPacket->GetSize(),
                          "Received Packet size is not equal to the Rx buffer size");
}

void
Aodvv2Test::DoSendData(Ptr<Socket> socket, std::string to)
{
    Address realTo = InetSocketAddress(Ipv4Address(to.c_str()), 1234);
    NS_TEST_EXPECT_MSG_EQ(socket->SendTo(Create<Packet>(123), 0, realTo), 123, "100");
}

void
Aodvv2Test::SendData(Ptr<Socket> socket, std::string to)
{
    m_receivedPacket = Create<Packet>();
    Simulator::ScheduleWithContext(socket->GetNode()->GetId(),
                                   Seconds(60),
                                   &Aodvv2Test::DoSendData,
                                   this,
                                   socket,
                                   to);
    Simulator::Stop(Seconds(66));
    Simulator::Run();
}

void
Aodvv2Test::DoRun()
{
    Simulator::Destroy();
}

/**
 * \ingroup internet-test
 *
 * \brief AODVv2 TestSuite
 */
class Aodvv2TestSuite : public TestSuite
{
  public:
    Aodvv2TestSuite()
        : TestSuite("aodvv2", Type::UNIT)
    {
        AddTestCase(new Aodvv2Test, TestCase::Duration::QUICK);
    }
};

static Aodvv2TestSuite g_aodvv2TestSuite; //!< Static variable for test initialization
