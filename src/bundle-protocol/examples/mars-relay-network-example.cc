#include "ns3/applications-module.h"
#include "ns3/base-routing-engine.h"
#include "ns3/bundle-agent.h"
#include "ns3/bundle-block.h"
#include "ns3/bundle-protocol-helper.h"
#include "ns3/bundle.h"
#include "ns3/contact-graph-helper.h"
#include "ns3/contact-parser.h"
#include "ns3/core-module.h"
#include "ns3/inet-socket-address.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ltp-convergence-layer-adapter.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

#include <map>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MarsRelayNetworkExample");

struct LinkElements
{
    Ptr<LtpBundleCla> cla;
    Ptr<PointToPointNetDevice> device;
};

void
SendDeepSpaceBundle(Ptr<BundleAgent> agent, std::string destEid)
{
    NS_LOG_INFO("At time " << Simulator::Now().GetSeconds()
                           << "s: Initiating bundle transmission to " << destEid);

    uint32_t payloadSize = 5000;
    std::vector<uint8_t> payloadData(payloadSize, 0xAA);

    Time ttl = Hours(24);

    agent->TransmitBundle(destEid, "dtn:none", payloadData.data(), payloadSize, ttl, 0);
}

void
OnBundleReceived(Ptr<Bundle> bundle)
{
    NS_LOG_INFO("==================================================");
    NS_LOG_INFO("SUCCESS! Destination received the Bundle at " << Simulator::Now().GetSeconds()
                                                               << "s");
    NS_LOG_INFO("Source EID: " << bundle->GetSourceEID());
    NS_LOG_INFO("Destination EID: " << bundle->GetDestinationEID());

    Ptr<PayloadBlock> payload = bundle->GetPayloadBlock();
    if (payload)
    {
        NS_LOG_INFO("Payload Size: " << payload->GetPayload()->GetSize() << " bytes");
    }
    NS_LOG_INFO("==================================================");
}

void
LinkUp(Ptr<BundleAgent> agent,
       std::string destEid,
       Ptr<LtpBundleCla> cla,
       Ptr<PointToPointNetDevice> device,
       uint32_t dataRate,
       Time delay)
{
    NS_LOG_INFO("At time " << Simulator::Now().GetSeconds()
                           << "s: Contact UP - Registering CLA for " << destEid << " at "
                           << dataRate << " bps with delay " << delay.GetSeconds() << "s");

    device->SetAttribute("DataRate", DataRateValue(DataRate(dataRate)));
    device->GetChannel()->SetAttribute("Delay", TimeValue(delay));

    agent->RegisterCla(destEid, cla);
}

void
LinkDown(Ptr<BundleAgent> agent, std::string destEid, Ptr<PointToPointNetDevice> device)
{
    NS_LOG_INFO("At time " << Simulator::Now().GetSeconds()
                           << "s: Contact DOWN - Unregistering CLA for " << destEid);

    agent->UnregisterCla(destEid);
    device->SetAttribute("DataRate", DataRateValue(DataRate(0)));
}

int
main(int argc, char* argv[])
{
    LogComponentEnable("MarsRelayNetworkExample", LOG_LEVEL_ALL);
    LogComponentEnable("BundleAgent", LOG_LEVEL_ALL);
    LogComponentEnable("ContactOptimizedDijkstraRouting", LOG_LEVEL_ALL);

    std::string contactPlanPath = "src/bundle-protocol/examples/contactGraph.csv";

    std::vector<std::string> mrnEids = {"dtn://earth/dsn",
                                        "dtn://mars/mro",
                                        "dtn://mars/odyssey",
                                        "dtn://mars/mvn",
                                        "dtn://mars/tgo",
                                        "dtn://mars/msl",
                                        "dtn://mars/m2020",
                                        "dtn://mars/insight",
                                        "dtn://mars/ingenuity"};
    uint32_t numNodes = mrnEids.size();

    ContactGraphHelper cgrHelper;
    cgrHelper.SetContactPlan(contactPlanPath);

    cgrHelper.SetRoutingEngine("ns3::ContactOptimizedDijkstraRouting");

    Ptr<BaseRoutingEngine> contactGraph = cgrHelper.Install();

    NodeContainer nodes;
    nodes.Create(numNodes);

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.252");

    BundleAgentHelper agentHelper;

    std::map<std::string, Ptr<BundleAgent>> agentMap;
    std::map<std::pair<std::string, std::string>, LinkElements> linkMap;

    for (uint32_t i = 0; i < numNodes; ++i)
    {
        Ptr<Node> node = nodes.Get(i);
        const std::string& eid = mrnEids[i];

        agentHelper.SetBpEndpointId(eid);
        BundleAgentContainer agentContainer = agentHelper.Install(node);
        Ptr<BundleAgent> agent = agentContainer.Get(0);
        node->AggregateObject(agent);

        agent->SetContactGraph(contactGraph);
        agent->SetReceiveCallback(MakeCallback(&OnBundleReceived));

        agentMap[eid] = agent;
    }

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("1bps"));
    p2p.SetChannelAttribute("Delay", StringValue("1ms"));

    uint16_t ltpPort = 1113;

    for (uint32_t i = 0; i < numNodes; ++i)
    {
        for (uint32_t j = i + 1; j < numNodes; ++j)
        {
            NetDeviceContainer devices = p2p.Install(nodes.Get(i), nodes.Get(j));

            Ipv4InterfaceContainer ifaces = address.Assign(devices);
            address.NewNetwork();

            Ptr<PointToPointNetDevice> devI = DynamicCast<PointToPointNetDevice>(devices.Get(0));
            Ptr<PointToPointNetDevice> devJ = DynamicCast<PointToPointNetDevice>(devices.Get(1));

            Ptr<LtpBundleCla> claI = CreateObject<LtpBundleCla>();
            Ptr<LtpBundleCla> claJ = CreateObject<LtpBundleCla>();

            InetSocketAddress addrI(ifaces.GetAddress(0), ltpPort);
            InetSocketAddress addrJ(ifaces.GetAddress(1), ltpPort);

            claI->Setup(nodes.Get(i), addrI, addrJ);
            claJ->Setup(nodes.Get(j), addrJ, addrI);

            std::string eidI = mrnEids[i];
            std::string eidJ = mrnEids[j];

            linkMap[{eidI, eidJ}] = {claI, devI};
            linkMap[{eidJ, eidI}] = {claJ, devJ};
        }
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    const auto& contactWindows = contactGraph->GetContactWindows();

    for (const auto& window : contactWindows)
    {
        auto linkIt = linkMap.find({window.fromEID, window.toEID});
        auto agentIt = agentMap.find(window.fromEID);

        if (linkIt != linkMap.end() && agentIt != agentMap.end())
        {
            LinkElements link = linkIt->second;
            Ptr<BundleAgent> srcAgent = agentIt->second;

            Simulator::Schedule(window.startTime,
                                &LinkUp,
                                srcAgent,
                                window.toEID,
                                link.cla,
                                link.device,
                                window.dataRate,
                                window.delay);
            Simulator::Schedule(window.endTime, &LinkDown, srcAgent, window.toEID, link.device);
        }
        else
        {
            NS_LOG_WARN("Skipping schedule: Cannot find physical link or agent for "
                        << window.fromEID << " to " << window.toEID);
        }
    }

    Ptr<BundleAgent> dsnAgent = agentMap["dtn://earth/dsn"];
    std::string destinationEid = "dtn://mars/ingenuity";

    Simulator::Schedule(Seconds(10.0), &SendDeepSpaceBundle, dsnAgent, destinationEid);

    NS_LOG_INFO("Starting Deep Space MRN Simulation...");

    Simulator::Stop(Seconds(20000.0));
    Simulator::Run();
    Simulator::Destroy();

    NS_LOG_INFO("Simulation Finished.");

    return 0;
}
