#include "ns3/applications-module.h"
#include "ns3/bundle-agent.h"
#include "ns3/bundle-block.h"
#include "ns3/bundle-protocol-helper.h"
#include "ns3/bundle.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/ltp-convergence-layer-adapter.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h" 

#include "ns3/contact-graph-helper.h"
#include "ns3/contact-graph-routing.h"

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
    NS_LOG_INFO("SUCCESS! Destination received the Bundle at " << Simulator::Now().GetSeconds() << "s");
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
    LogComponentEnable("MarsRelayNetworkExample", LOG_LEVEL_INFO);
    LogComponentEnable("BundleAgent", LOG_LEVEL_ALL);
    LogComponentEnable("ContactGraph", LOG_LEVEL_ALL);
    LogComponentEnable("BundleStorageEngine", LOG_LEVEL_ALL);
    LogComponentEnable("ContactGraphHelper", LOG_LEVEL_INFO);
    LogComponentEnable("ContactParser", LOG_LEVEL_INFO);

    std::vector<std::string> mrnEids = {
        "dtn://earth/dsn",
        "dtn://mars/mro",
        "dtn://mars/ody",
        "dtn://mars/mvn",
        "dtn://mars/tgo",
        "dtn://mars/msl",
        "dtn://mars/m2020",
        "dtn://mars/insight",
        "dtn://mars/ingenuity"
    };
    uint32_t numNodes = mrnEids.size();

    ContactGraphHelper cgrHelper;
    cgrHelper.SetContactPlan("/mnt/home/lagwanka/ns-3-dev/src/bundle-protocol/examples/contactGraph.csv");
    Ptr<ContactGraph> contactGraph = cgrHelper.Install();

    NodeContainer nodes;
    nodes.Create(numNodes);

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(1)));
    NetDeviceContainer devices = csma.Install(nodes);

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    BundleAgentHelper agentHelper;
    BundleClaHelper ltpHelper("ns3::LtpBundleCla");
    ltpHelper.SetAttribute("OnewayLightTime", TimeValue(Seconds(300.0))); 

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

    for (uint32_t i = 0; i < numNodes; ++i) 
    {
        for (uint32_t j = 0; j < numNodes; ++j) 
        {
            if (i == j) continue; 

            BundleClaContainer clas = ltpHelper.Install(nodes.Get(i));
            Ptr<LtpBundleCla> cla = DynamicCast<LtpBundleCla>(clas.Get(0));
            
            Ipv4Address myIp = interfaces.GetAddress(i);
            Ipv4Address targetIp = interfaces.GetAddress(j);

            uint16_t localPort = 10000 + (i * 100) + j;
            uint16_t remotePort = 10000 + (j * 100) + i;
            
            cla->Setup(nodes.Get(i), InetSocketAddress(myIp, localPort), InetSocketAddress(targetIp, remotePort));

            agents[i]->RegisterCla(mrnEids[j], cla);
        }
    }

    Ptr<BundleAgent> dsnAgent = agents[0];
    std::string destinationEid = "dtn://mars/m2020";

    Simulator::Schedule(Seconds(10.0), &SendDeepSpaceBundle, dsnAgent, destinationEid);

    // Simulator::Schedule(Seconds(5000.0), &SendDeepSpaceBundle, dsnAgent, "dtn://mars/ingenuity");

    NS_LOG_INFO("Starting Deep Space MRN Simulation...");
    
    Simulator::Stop(Seconds(12000.0));
    Simulator::Run();
    Simulator::Destroy();

    NS_LOG_INFO("Simulation Finished.");

    return 0;
}