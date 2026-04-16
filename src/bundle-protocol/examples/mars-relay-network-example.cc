#include "ns3/applications-module.h"
#include "ns3/bundle-agent.h"
#include "ns3/bundle-block.h"
#include "ns3/bundle-protocol-helper.h"
#include "ns3/bundle.h"
#include "ns3/contact-graph-helper.h"
#include "ns3/contact-graph-routing.h"
#include "ns3/contact-parser.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/ltp-convergence-layer-adapter.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MarsRelayNetworkExample");

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

int
main(int argc, char* argv[])
{
    LogComponentEnable("MarsRelayNetworkExample", LOG_LEVEL_ALL);
    LogComponentEnable("BundleAgent", LOG_LEVEL_ALL);
    LogComponentEnable("ContactGraph", LOG_LEVEL_ALL);

    std::string contactPlanPath =
        "src/bundle-protocol/examples/contactGraph.csv";

    std::vector<std::string> mrnEids = {"dtn://earth/dsn",
                                        "dtn://mars/mro",
                                        "dtn://mars/ody",
                                        "dtn://mars/mvn",
                                        "dtn://mars/tgo",
                                        "dtn://mars/msl",
                                        "dtn://mars/m2020",
                                        "dtn://mars/insight",
                                        "dtn://mars/ingenuity"};
    uint32_t numNodes = mrnEids.size();

    ContactGraphHelper cgrHelper;
    cgrHelper.SetContactPlan(contactPlanPath);
    Ptr<ContactGraph> contactGraph = cgrHelper.Install();

    NodeContainer nodes;
    nodes.Create(numNodes);

    InternetStackHelper stack;
    stack.Install(nodes);

    std::vector<NetDeviceContainer> p2pLinks =
        ContactParser::CreateP2pLinks(contactPlanPath, nodes, mrnEids);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.252");

    BundleAgentHelper agentHelper;
    BundleClaHelper ltpHelper("ns3::LtpBundleCla");
    ltpHelper.SetAttribute("OnewayLightTime", TimeValue(Seconds(1500.0)));

    std::vector<Ptr<BundleAgent>> agents(numNodes);

    for (uint32_t i = 0; i < numNodes; ++i)
    {
        Ptr<Node> node = nodes.Get(i);
        std::string eid = mrnEids[i];

        agentHelper.SetBpEndpointId(eid);
        BundleAgentContainer agentContainer = agentHelper.Install(node);
        agents[i] = agentContainer.Get(0);
        node->AggregateObject(agents[i]);

        agents[i]->SetContactGraph(contactGraph);
        agents[i]->SetReceiveCallback(MakeCallback(&OnBundleReceived));
    }

    for (uint32_t k = 0; k < p2pLinks.size(); ++k)
    {
        NetDeviceContainer link = p2pLinks[k];

        Ipv4InterfaceContainer interfaces = address.Assign(link);
        address.NewNetwork();

        Ptr<Node> node0 = link.Get(0)->GetNode();
        Ptr<Node> node1 = link.Get(1)->GetNode();

        uint32_t id0 = node0->GetId();
        uint32_t id1 = node1->GetId();

        Ipv4Address ip0 = interfaces.GetAddress(0);
        Ipv4Address ip1 = interfaces.GetAddress(1);

        uint16_t port0 = 10000 + (id0 * 100) + id1;
        uint16_t port1 = 10000 + (id1 * 100) + id0;

        BundleClaContainer clas0 = ltpHelper.Install(node0);
        Ptr<LtpBundleCla> cla0 = DynamicCast<LtpBundleCla>(clas0.Get(0));
        cla0->Setup(node0, InetSocketAddress(ip0, port0), InetSocketAddress(ip1, port1));
        agents[id0]->RegisterCla(mrnEids[id1], cla0);

        BundleClaContainer clas1 = ltpHelper.Install(node1);
        Ptr<LtpBundleCla> cla1 = DynamicCast<LtpBundleCla>(clas1.Get(0));
        cla1->Setup(node1, InetSocketAddress(ip1, port1), InetSocketAddress(ip0, port0));
        agents[id1]->RegisterCla(mrnEids[id0], cla1);
    }

    Ptr<BundleAgent> dsnAgent = agents[0];
    std::string destinationEid = "dtn://mars/ingenuity";

    Simulator::Schedule(Seconds(10.0), &SendDeepSpaceBundle, dsnAgent, destinationEid);

    NS_LOG_INFO("Starting Deep Space MRN Simulation...");

    Simulator::Stop(Seconds(20000.0));
    Simulator::Run();
    Simulator::Destroy();

    NS_LOG_INFO("Simulation Finished.");

    return 0;
}
