#include "ns3/applications-module.h"
#include "ns3/bundle-agent.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/ltp-convergence-layer-adapter.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("DeepSpaceNetworkExample");

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
    NS_LOG_INFO("SUCCESS! Mars Application received the Bundle.");
    NS_LOG_INFO("Source EID: " << bundle->GetSourceEID());
    NS_LOG_INFO("Destination EID: " << bundle->GetDestinationEID());

    Ptr<PayloadBlock> payload = bundle->GetPayloadBlock();
    if (payload)
    {
        NS_LOG_INFO("Payload Size: " << payload->GetPayload()->GetSize() << " bytes");
    }
}

int
main(int argc, char* argv[])
{
    LogComponentEnable("DeepSpaceNetworkExample", LOG_LEVEL_INFO);
    LogComponentEnable("BundleAgent", LOG_LEVEL_DEBUG);
    LogComponentEnable("LtpBundleCla", LOG_LEVEL_LOGIC);

    NodeContainer nodes;
    nodes.Create(2);
    Ptr<Node> earthNode = nodes.Get(0);
    Ptr<Node> marsNode = nodes.Get(1);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("2Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("300s"));

    NetDeviceContainer devices;
    devices = p2p.Install(nodes);

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    Ipv4Address earthIp = interfaces.GetAddress(0);
    Ipv4Address marsIp = interfaces.GetAddress(1);

    Ptr<BundleAgent> earthAgent = CreateObject<BundleAgent>();
    earthAgent->SetLocalEID("dtn:earth");
    earthNode->AggregateObject(earthAgent);

    Ptr<BundleAgent> marsAgent = CreateObject<BundleAgent>();
    marsAgent->SetLocalEID("dtn:mars");
    marsNode->AggregateObject(marsAgent);

    Ptr<LtpBundleCla> earthCla = CreateObject<LtpBundleCla>();
    earthCla->Setup(earthNode, InetSocketAddress(earthIp, 1113), InetSocketAddress(marsIp, 1113));
    earthCla->SetOnewayLightTime(Seconds(300.0));

    Ptr<LtpBundleCla> marsCla = CreateObject<LtpBundleCla>();
    marsCla->Setup(marsNode, InetSocketAddress(marsIp, 1113), InetSocketAddress(earthIp, 1113));
    marsCla->SetOnewayLightTime(Seconds(300.0));
    Ptr<ClientServiceStatus> marsClientService = CreateObject<ClientServiceStatus>();
    marsCla->RegisterClientService(0, marsClientService);

    earthAgent->RegisterCla("dtn:mars", earthCla);
    marsAgent->RegisterCla("dtn:earth", marsCla);

    marsAgent->SetReceiveCallback(MakeCallback(&OnBundleReceived));

    Simulator::Schedule(Seconds(10.0), &SendDeepSpaceBundle, earthAgent, "dtn:mars");

    NS_LOG_INFO("Starting Deep Space Simulation...");
    Simulator::Stop(Seconds(1000.0));
    Simulator::Run();
    Simulator::Destroy();

    NS_LOG_INFO("Simulation Finished.");

    return 0;
}
