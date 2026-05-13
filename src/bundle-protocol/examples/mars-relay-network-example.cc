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

uint32_t g_bundlesSent = 500;
uint32_t g_bundlesReceived = 0;
double g_totalDelaySeconds = 0.0;

struct StorageStats
{
    uint64_t cumulativeBundles = 0;
    uint32_t sampleCount = 0;
};

std::map<std::string, StorageStats> g_storageMetrics;

struct LinkElements
{
    Ptr<LtpBundleCla> cla;
    Ptr<PointToPointNetDevice> device;
};

void
SendDeepSpaceBundle(Ptr<BundleAgent> agent, std::string destEid)
{
    NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s: Transmitting burst of "
                           << g_bundlesSent << " bundles...");

    uint32_t payloadSize = 100;
    std::vector<uint8_t> payloadData(payloadSize, 0xAA);
    Time ttl = Hours(24);

    for (uint32_t i = 0; i < g_bundlesSent; i++)
    {
        agent->TransmitBundle(destEid, "dtn:none", payloadData.data(), payloadSize, ttl, 0);
    }
}

void
OnBundleReceived(Ptr<Bundle> bundle)
{
    Time creationTime = bundle->GetPrimaryBlock()->GetHeader().GetCreationTime();
    Time rxTime = Simulator::Now();
    Time delay = rxTime - creationTime;

    g_bundlesReceived++;
    g_totalDelaySeconds += delay.GetSeconds();

    NS_LOG_INFO("Received Bundle " << g_bundlesReceived << "/" << g_bundlesSent
                                   << " | OWT: " << delay.GetSeconds() << "s");

    if (g_bundlesReceived == g_bundlesSent)
    {
        NS_LOG_INFO("==================================================");
        NS_LOG_INFO("SUCCESS! All " << g_bundlesSent << " bundles arrived.");
        NS_LOG_INFO("Final Average OWT: " << (g_totalDelaySeconds / g_bundlesReceived) << " s");
        NS_LOG_INFO("==================================================");
    }
}

void
MonitorStorage(Ptr<BundleAgent> agent, std::string nodeName, Time interval)
{
    uint32_t bytesInStorage = agent->GetStorageEngineSize();
    uint32_t estimatedBundles = bytesInStorage / 1061;

    if (bytesInStorage > 0)
    {
        NS_LOG_INFO(">>> [STORAGE MONITOR] At " << Simulator::Now().GetSeconds() << "s | "
                                                << nodeName << " has " << estimatedBundles
                                                << " bundles in custody.");
    }

    g_storageMetrics[nodeName].cumulativeBundles += estimatedBundles;
    g_storageMetrics[nodeName].sampleCount++;

    Simulator::Schedule(interval, &MonitorStorage, agent, nodeName, interval);
}

void
LinkUp(Ptr<BundleAgent> agent,
       std::string destEid,
       Ptr<LtpBundleCla> cla,
       Ptr<PointToPointNetDevice> device,
       uint32_t dataRate)
{
    NS_LOG_INFO("At time " << Simulator::Now().GetSeconds()
                           << "s: Contact UP - Registering CLA for " << destEid << " at "
                           << dataRate << " bps");

    device->SetAttribute("DataRate", DataRateValue(DataRate(dataRate)));
    agent->RegisterCla(destEid, cla);
}

void
LinkDown(Ptr<BundleAgent> agent, std::string destEid, Ptr<PointToPointNetDevice> device)
{
    NS_LOG_INFO("At time " << Simulator::Now().GetSeconds()
                           << "s: Contact DOWN - Unregistering CLA for " << destEid);

    agent->UnregisterCla(destEid);
    device->SetAttribute("DataRate", DataRateValue(DataRate("1bps")));
}

Time
GetDelayForPair(const std::string& eidA,
                const std::string& eidB,
                const std::vector<ContactWindow>& windows)
{
    for (const auto& w : windows)
    {
        if (w.fromEID == eidA && w.toEID == eidB)
        {
            return w.delay;
        }
    }
    return MilliSeconds(1);
}

int
main(int argc, char* argv[])
{
    LogComponentEnable("MarsRelayNetworkExample", LOG_LEVEL_ALL);
    // LogComponentEnable("LtpBundleCla", LOG_LEVEL_ALL);

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

    cgrHelper.SetRoutingEngine("ns3::ContactMultigraphRouting");

    Ptr<BaseRoutingEngine> contactGraph = cgrHelper.Install();

    const auto& contactWindows = contactGraph->GetContactWindows();

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
            std::string eidI = mrnEids[i];
            std::string eidJ = mrnEids[j];

            Time linkDelay = GetDelayForPair(eidI, eidJ, contactWindows);

            PointToPointHelper p2p;
            p2p.SetDeviceAttribute("DataRate", StringValue("1bps"));
            p2p.SetChannelAttribute("Delay", TimeValue(linkDelay));

            NetDeviceContainer devices = p2p.Install(nodes.Get(i), nodes.Get(j));

            Ptr<Channel> channel = devices.Get(0)->GetChannel();
            channel->SetAttribute("Delay", TimeValue(linkDelay));

            Ipv4InterfaceContainer ifaces = address.Assign(devices);
            address.NewNetwork();

            Ptr<PointToPointNetDevice> devI = DynamicCast<PointToPointNetDevice>(devices.Get(0));
            Ptr<PointToPointNetDevice> devJ = DynamicCast<PointToPointNetDevice>(devices.Get(1));

            Ptr<LtpBundleCla> claI = CreateObject<LtpBundleCla>();
            Ptr<LtpBundleCla> claJ = CreateObject<LtpBundleCla>();

            claI->SetAttribute("OnewayLightTime", TimeValue(linkDelay));
            claJ->SetAttribute("OnewayLightTime", TimeValue(linkDelay));

            InetSocketAddress addrI(ifaces.GetAddress(0), ltpPort);
            InetSocketAddress addrJ(ifaces.GetAddress(1), ltpPort);

            claI->Setup(nodes.Get(i), addrI, addrJ);
            claJ->Setup(nodes.Get(j), addrJ, addrI);

            claI->SetRxCallback(MakeCallback(&BundleAgent::RecvBundle, agentMap[eidI]));
            claJ->SetRxCallback(MakeCallback(&BundleAgent::RecvBundle, agentMap[eidJ]));

            linkMap[{eidI, eidJ}] = {claI, devI};
            linkMap[{eidJ, eidI}] = {claJ, devJ};
        }
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

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
                                window.dataRate);
            Simulator::Schedule(window.endTime, &LinkDown, srcAgent, window.toEID, link.device);
        }
    }

    Ptr<BundleAgent> dsnAgent = agentMap["dtn://earth/dsn"];
    std::string destinationEid = "dtn://mars/ingenuity";

    Simulator::Schedule(Seconds(0.0), &SendDeepSpaceBundle, dsnAgent, destinationEid);

    // Track storage for key nodes along the expected paths
    Time checkInterval = Seconds(500.0);
    Simulator::Schedule(Seconds(1.0),
                        &MonitorStorage,
                        agentMap["dtn://earth/dsn"],
                        "Earth DSN",
                        checkInterval);
    Simulator::Schedule(Seconds(1.0),
                        &MonitorStorage,
                        agentMap["dtn://mars/tgo"],
                        "Mars TGO",
                        checkInterval);
    Simulator::Schedule(Seconds(1.0),
                        &MonitorStorage,
                        agentMap["dtn://mars/m2020"],
                        "Perseverance",
                        checkInterval);

    NS_LOG_INFO("Starting Deep Space MRN Simulation...");

    Simulator::Stop(Seconds(86400.0));
    Simulator::Run();

    NS_LOG_INFO("Simulation Finished.");
    NS_LOG_INFO("================= SIMULATION RESULTS =================");
    NS_LOG_INFO("Total Bundles Sent:     " << g_bundlesSent);
    NS_LOG_INFO("Total Bundles Received: " << g_bundlesReceived);

    if (g_bundlesReceived > 0)
    {
        NS_LOG_INFO("Delivery Ratio:         "
                    << ((double)g_bundlesReceived / g_bundlesSent) * 100.0 << "%");
        NS_LOG_INFO("Average One-Way Time:   " << (g_totalDelaySeconds / g_bundlesReceived)
                                               << " seconds");
    }
    else
    {
        NS_LOG_INFO("Average One-Way Time:   N/A (0 bundles received)");
    }

    NS_LOG_INFO("----------------- BUFFER UTILIZATION -----------------");
    for (const auto& pair : g_storageMetrics)
    {
        double averageBundles = 0.0;
        if (pair.second.sampleCount > 0)
        {
            averageBundles = (double)pair.second.cumulativeBundles / pair.second.sampleCount;
        }
        NS_LOG_INFO(pair.first << " Average Backlog: " << averageBundles << " bundles");
    }
    NS_LOG_INFO("======================================================");

    Simulator::Destroy();

    return 0;
}
