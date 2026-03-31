/* Copyright(c) 2014 Universitat Autònoma de Barcelona
** SPDX - License - Identifier : GPL-2.0-only
*
*
*
* Author : Rubén Martínez<rmartinez @deic.uab.cat>
*/

//        Network topology
//
//       n0              n1
//       |               |
//       =================
//          PointToPoint
//
// - Send a block of data from one service instance in node n0 to the other in node n1.
// - Data is sent end-to-end through a LtpProtocol <-> UdpLayerAdapter <-> PointToPointLink <->
// UdpLayerAdapter <-> LtpProtocol.
// - Functions (ClientServiceInstanceNotificationsSnd and ClientServiceInstanceNotificationsRcv) are
// used for tracing
//

#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/ltp-protocol-helper.h"
#include "ns3/ltp-protocol.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

#include <sstream>

      using namespace ns3;
using namespace ltp;

NS_LOG_COMPONENT_DEFINE("LtpProtocolSimpleExample");

void
ClientServiceInstanceNotificationsSnd(SessionId id,
                                      StatusNotificationCode code,
                                      std::vector<uint8_t> data,
                                      uint32_t dataLength,
                                      bool endFlag,
                                      uint64_t srcLtpEngine,
                                      uint32_t offset)
{
    std::cout << "ClientServiceNotification - Session ID: " << id.GetSessionNumber()
              << " Code : " << code << std::endl;
}

void
ClientServiceInstanceNotificationsRcv(SessionId id,
                                      StatusNotificationCode code,
                                      std::vector<uint8_t> data,
                                      uint32_t dataLength,
                                      bool endFlag,
                                      uint64_t srcLtpEngine,
                                      uint32_t offset)
{
    static uint32_t totalData = 0;

    std::cout << "ClientServiceNotification - Session ID: " << id.GetSessionNumber()
              << " Code : " << code << std::endl;

    totalData += dataLength;

    if (code == ns3::ltp::GP_SEGMENT_RCV)
    {
        std::cout << "ClientServiceNotification - Received a Green Data Segment of Size: ( "
                  << dataLength << ")" << std::endl;
    }

    if (code == ns3::ltp::RED_PART_RCV)
    {
        std::stringstream ss;

        for (std::vector<uint8_t>::const_iterator i = data.begin(); i != data.end(); ++i)
        {
            ss << *i;
        }

        NS_ASSERT(ss.str().length() == dataLength);

        std::cout << "ClientServiceNotification - Received Full Red Part of Size: ( " << dataLength
                  << ")" << std::endl;
    }
    if (code == ns3::ltp::SESSION_END)
    {
        std::cout << "ClientServiceNotification - Received a full block of data with size: ( "
                  << totalData << ") from LtpEngine: " << srcLtpEngine << std::endl;
    }
}

int
main(int argc, char* argv[])
{
    bool verbose = true;

    LogComponentEnable("LtpProtocolSimpleExample", LOG_LEVEL_ALL);
    LogComponentEnable("LtpProtocol", LOG_LEVEL_ALL);
    // LogComponentEnable ("LtpUdpConvergenceLayerAdapter", LOG_LEVEL_ALL);

    CommandLine cmd;
    cmd.AddValue("verbose", "Tell application to log if true", verbose);
    cmd.Parse(argc, argv);

    // Create the nodes required by the topology (shown above).
    NodeContainer nodes;
    nodes.Create(2);

    // Create point to point links and install them on the nodes
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("500Kbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("5ms"));

    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);

    // Install the internet stack on the nodes
    InternetStackHelper internet;
    internet.Install(nodes);

    // Assign IPv4 addresses.
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i = ipv4.Assign(devices);

    // Define the ClientService ID Code of the Client Service Instance that will be using the Ltp
    // protocol.
    uint64_t ClientServiceId = 0; // Bundle

    // Create a LtpIpResolution table to perform mappings between Ipv4 addresses and LtpEngineIDs
    Ptr<LtpIpResolutionTable> routing =
        CreateObjectWithAttributes<LtpIpResolutionTable>("Addressing", StringValue("Ipv4"));

    // Use a helper to create and install Ltp Protocol instances in the nodes.
    LtpProtocolHelper ltpHelper;
    ltpHelper.SetAttributes("CheckPointRtxLimit",
                            UintegerValue(20),
                            "ReportSegmentRtxLimit",
                            UintegerValue(20),
                            "RetransCyclelimit",
                            UintegerValue(20));
    ltpHelper.SetLtpIpResolutionTable(routing);
    ltpHelper.SetBaseLtpEngineId(0);
    ltpHelper.SetStartTransmissionTime(Seconds(1));
    ltpHelper.InstallAndLink(nodes);

    // Register ClientServiceInstances with the corresponding Ltp Engine of each node
    CallbackBase cb = MakeCallback(&ClientServiceInstanceNotificationsSnd);
    CallbackBase cb2 = MakeCallback(&ClientServiceInstanceNotificationsRcv);
    nodes.Get(0)->GetObject<LtpProtocol>()->RegisterClientService(ClientServiceId, cb);
    nodes.Get(1)->GetObject<LtpProtocol>()->RegisterClientService(ClientServiceId, cb2);

    // Create a block of data
    std::vector<uint8_t> data3(5000, 65);

    // Transmit if from n0 to n1.
    uint64_t receiverLtpId = nodes.Get(1)->GetObject<LtpProtocol>()->GetLocalEngineId();
    nodes.Get(0)->GetObject<LtpProtocol>()->StartTransmission(ClientServiceId,
                                                              ClientServiceId,
                                                              receiverLtpId,
                                                              data3,
                                                              1500);

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
