/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Kirill Andreev  <andreev@iitp.ru>
 */

#include "ns3/ipv4-interface-container.h"
#include "ns3/hwmp-protocol.h"
#include "ns3/node-container.h"
#include "ns3/nstime.h"
#include "ns3/test.h"

#include <string>
#include <vector>

using namespace ns3;

/**
 * @ingroup dot11s-test
 *
 * @brief There are 5 stations set into a row, the center station is root.
 * Regression test indicates that traffic goes from the first to the
 * last stations without reactive route discovery procedure
 * @verbatim
 *          Src                Root                 Dst
 * (node ID) 0         1         2         3         4
 * (MAC)   00:01     00:02     00:03     00:04     00:05
 *           |         |<--------|-------->|         |          Proactive PREQ
 *           |         |-------->|         |         |          PREP
 *           |         |         |<--------|         |          PREP
 *           |<--------|-------->|<--------|-------->|          Proactive PREQ
 *           |-------->|         |         |<--------|          PREP
 *           |         |-------->|         |         |          PREP
 *           |         |         |<--------|         |          PREP
 *  <--------|-------->|         |         |<--------|--------> Proactive PREQ
 *  Note, that at this moment all routes are known, and no reactive
 *  path discovery procedure shall be initiated
 *           |         |         |<--------|-------->|          ARP request
 *           |.........|.........|.........|.........|
 *  <--------|-------->|         |         |         |          ARP request
 *           |-------->|         |         |         |          ARP reply
 *           |.........|.........|.........|.........|
 *           |         |         |         |-------->|          ARP reply
 *           |         |         |         |<--------|          DATA
 *                                             ^
 *               Further data is forwarded by proactive path
 *
 * This regression checks the ordering of the HWMP control messages and
 * successful data delivery; it does not assert exact message times or ARP
 * timing.
 * @endverbatim
 *
 */
class HwmpProactiveRegressionTest : public TestCase
{
  public:
    HwmpProactiveRegressionTest();
    ~HwmpProactiveRegressionTest() override;

    void DoRun() override;
    /// Check results function
    void CheckResults();

  private:
    /// @internal It is important to have pointers here
    NodeContainer* m_nodes;
    /// Simulation time
    Time m_time;
    Ipv4InterfaceContainer m_interfaces; ///< interfaces

    /// Create nodes function
    void CreateNodes();
    /// Create devices function
    void CreateDevices();
    /// Install application function
    void InstallApplications();
    /// Reset position function
    void ResetPosition();

    /// Connect trace sources used to validate the message sequence.
    void ConnectTraces();

    /// Record a HWMP trace event for a given node.
    static void HandleHwmpEvent(HwmpProactiveRegressionTest* testcase,
                  uint32_t nodeId,
                  dot11s::HwmpProtocol::MessageEvent event);

    /// Look up the first recorded event containing a marker.
    size_t FindEvent(std::string const& marker) const;

    /// Server-side socket
    Ptr<Socket> m_serverSocket;
    /// Client-side socket
    Ptr<Socket> m_clientSocket;

    /// sent packets counter
    uint32_t m_sentPktsCounter;
    /// received packets counter on server
    uint32_t m_receivedPktsCounter;

    /// Recorded HWMP control-plane events.
    std::vector<std::string> m_observedEvents;

    /**
     * Send data
     * @param socket the sending socket
     */
    void SendData(Ptr<Socket> socket);

    /**
     * @brief Handle a packet reception.
     *
     * This function is called by lower layers.
     *
     * @param socket the socket the packet was received to.
     */
    void HandleReadServer(Ptr<Socket> socket);

    /**
     * @brief Handle a packet reception.
     *
     * This function is called by lower layers.
     *
     * @param socket the socket the packet was received to.
     */
    void HandleReadClient(Ptr<Socket> socket);
};
