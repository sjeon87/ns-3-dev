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
#include "ns3/udp-convergence-layer-adapter.h"

#include <map>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MarsRelayNetworkExample");

static uint32_t g_bundlesSent = 15;
static uint32_t g_bundlesReceived = 0;
static double g_totalDelaySeconds = 0.0;

/**
 * @brief Statistics for bundle storage at a specific node.
 *
 * This structure is used to track the storage utilization of a bundle agent
 * over time, allowing for the calculation of average and peak storage metrics.
 */
struct StorageStats
{
    uint64_t cumulativeBundles = 0; ///< The sum of bundles across all samples.
    uint32_t sampleCount = 0;       ///< The total number of times the storage was sampled.
    uint32_t peakBundles = 0; ///< The maximum number of bundles held in custody at any one time.
    double peakTime = 0.0;    ///< The simulation time when the peak bundle count occurred.
};

static std::map<std::string, StorageStats> g_storageStats;

/**
 * @brief Elements comprising a network link between two DTN nodes.
 *
 * This structure holds the necessary network devices and the Convergence
 * Layer Adapter (CLA) to manage dynamic link availability and data rates.
 */
struct LinkElements
{
    Ptr<LtpBundleCla> cla;                   ///< Pointer to the LTP CLA.
    Ptr<PointToPointNetDevice> localDevice;  ///< Pointer to the local P2P network device.
    Ptr<PointToPointNetDevice> remoteDevice; ///< Pointer to the remote P2P network device.
};

void
CheckBacklog(Ptr<BundleAgent> agent)
{
    agent->ProcessAllBacklog();
    Simulator::Schedule(Seconds(1000.0), &CheckBacklog, agent);
}

void
SendPacedBundle(Ptr<BundleAgent> agent,
                std::string destEid,
                uint32_t bundlesLeft,
                uint32_t totalSent)
{
    if (bundlesLeft == 0)
    {
        NS_LOG_INFO("Finished sending all " << totalSent << " bundles.");
        return;
    }

    const uint32_t payloadSize = 2000000;
    std::vector<uint8_t> payload(payloadSize, 0xAA);
    const Time ttl = Hours(24);

    agent->TransmitBundle(destEid, "dtn:none", payload.data(), payloadSize, ttl, 0);

    NS_LOG_INFO("At " << Simulator::Now().GetSeconds() << "s: Sent bundle "
                      << (totalSent - bundlesLeft + 1) << "/" << totalSent);

    Simulator::Schedule(Seconds(0.1), &SendPacedBundle, agent, destEid, bundlesLeft - 1, totalSent);
}

void
OnBundleReceived(Ptr<Bundle> bundle)
{
    const Time creationTime = bundle->GetPrimaryBlock()->GetHeader().GetCreationTime();
    const Time delay = Simulator::Now() - creationTime;

    ++g_bundlesReceived;
    g_totalDelaySeconds += delay.GetSeconds();

    NS_LOG_INFO("Received bundle " << g_bundlesReceived << "/" << g_bundlesSent
                                   << " | OWT: " << delay.GetSeconds() << " s");

    if (g_bundlesReceived == g_bundlesSent)
    {
        NS_LOG_INFO("SUCCESS: all " << g_bundlesSent << " bundles delivered.");
        NS_LOG_INFO("Average OWT: " << (g_totalDelaySeconds / g_bundlesReceived) << " s");
    }
}

static const uint32_t BUNDLE_SIZE_BYTES = 2000162;

void
MonitorStorage(Ptr<BundleAgent> agent, std::string nodeName, Time interval)
{
    const uint32_t bytes = agent->GetStorageEngineSize();
    const uint32_t estimatedBundles = bytes / BUNDLE_SIZE_BYTES;

    StorageStats& stats = g_storageStats[nodeName];
    stats.cumulativeBundles += estimatedBundles;
    ++stats.sampleCount;

    if (estimatedBundles > stats.peakBundles)
    {
        stats.peakBundles = estimatedBundles;
        stats.peakTime = Simulator::Now().GetSeconds();
    }

    if (bytes > 0)
    {
        NS_LOG_INFO("[STORAGE] " << Simulator::Now().GetSeconds() << "s | " << nodeName << ": "
                                 << estimatedBundles << " bundle(s) in custody.");
    }

    Simulator::Schedule(interval, &MonitorStorage, agent, nodeName, interval);
}

void
LinkUp(Ptr<BundleAgent> agent,
       std::string destEid,
       Ptr<LtpBundleCla> cla,
       Ptr<PointToPointNetDevice> localDevice,
       Ptr<PointToPointNetDevice> remoteDevice,
       uint32_t dataRateBps)
{
    NS_LOG_INFO("At " << Simulator::Now().GetSeconds() << "s: Contact UP  " << agent->GetLocalEID()
                      << " -> " << destEid << " @ " << dataRateBps << " bps");

    DataRateValue dr{DataRate(dataRateBps)};
    localDevice->SetAttribute("DataRate", dr);
    remoteDevice->SetAttribute("DataRate", dr);
    agent->RegisterCla(destEid, cla);
}

void
LinkDown(Ptr<BundleAgent> agent,
         std::string destEid,
         Ptr<PointToPointNetDevice> localDevice,
         Ptr<PointToPointNetDevice> remoteDevice)
{
    NS_LOG_INFO("At " << Simulator::Now().GetSeconds() << "s: Contact DOWN " << agent->GetLocalEID()
                      << " -> " << destEid);

    agent->UnregisterCla(destEid);
    DataRateValue sentinel{DataRate("1bps")};
    localDevice->SetAttribute("DataRate", sentinel);
    remoteDevice->SetAttribute("DataRate", sentinel);
}

Time
GetPropagationDelay(const std::string& fromEid,
                    const std::string& toEid,
                    const std::vector<ContactWindow>& windows)
{
    for (const auto& w : windows)
    {
        if (w.fromEID == fromEid && w.toEID == toEid)
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

    const std::string contactPlanPath = "src/bundle-protocol/examples/contactGraph.csv";

    ContactGraphHelper cgrHelper;
    cgrHelper.SetContactPlan(contactPlanPath);
    cgrHelper.SetRoutingEngine("ns3::ContactMultigraphRouting");

    Ptr<BaseRoutingEngine> contactGraph = cgrHelper.Install();
    const std::vector<ContactWindow>& windows = contactGraph->GetContactWindows();

    const std::vector<std::string> eids = {
        "dtn://earth/dsn",
        "dtn://mars/tgo",
        "dtn://mars/mro",
        "dtn://mars/ingenuity",
    };
    const uint32_t numNodes = eids.size();

    NodeContainer nodes;
    nodes.Create(numNodes);

    InternetStackHelper stack;
    stack.Install(nodes);

    std::map<std::string, Ptr<BundleAgent>> agentMap;
    BundleAgentHelper agentHelper;

    for (uint32_t i = 0; i < numNodes; ++i)
    {
        agentHelper.SetBpEndpointId(eids[i]);
        BundleAgentContainer container = agentHelper.Install(nodes.Get(i));
        Ptr<BundleAgent> agent = container.Get(0);

        nodes.Get(i)->AggregateObject(agent);
        agent->SetContactGraph(contactGraph);
        agent->SetReceiveCallback(MakeCallback(&OnBundleReceived));

        agentMap[eids[i]] = agent;
    }

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.252");

    const uint16_t ltpPort = 1113;
    std::map<std::pair<std::string, std::string>, LinkElements> linkMap;

    for (uint32_t i = 0; i < numNodes; ++i)
    {
        for (uint32_t j = i + 1; j < numNodes; ++j)
        {
            const std::string& eidI = eids[i];
            const std::string& eidJ = eids[j];
            const Time delay = GetPropagationDelay(eidI, eidJ, windows);

            PointToPointHelper p2p;
            p2p.SetDeviceAttribute("DataRate", StringValue("1bps"));
            p2p.SetChannelAttribute("Delay", TimeValue(delay));
            p2p.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", StringValue("100000p"));

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

            claI->SetAttribute("OnewayLightTime", TimeValue(delay));
            claI->SetAttribute("RedPartRatio", DoubleValue(0.2));
            claI->SetAttribute("CheckPointRetransLimit", UintegerValue(0));

            claJ->SetAttribute("OnewayLightTime", TimeValue(delay));
            claJ->SetAttribute("RedPartRatio", DoubleValue(0.2));
            claJ->SetAttribute("CheckPointRetransLimit", UintegerValue(0));

            claI->SetRxCallback(MakeCallback(&BundleAgent::RecvBundle, agentMap[eidI]));
            claJ->SetRxCallback(MakeCallback(&BundleAgent::RecvBundle, agentMap[eidJ]));

            linkMap[{eidI, eidJ}] = {claI, devI, devJ};
            linkMap[{eidJ, eidI}] = {claJ, devJ, devI};
        }
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    for (const auto& w : windows)
    {
        auto linkIt = linkMap.find({w.fromEID, w.toEID});
        auto agentIt = agentMap.find(w.fromEID);

        if (linkIt == linkMap.end() || agentIt == agentMap.end())
        {
            continue;
        }

        Simulator::Schedule(w.startTime,
                            &LinkUp,
                            agentIt->second,
                            w.toEID,
                            linkIt->second.cla,
                            linkIt->second.localDevice,
                            linkIt->second.remoteDevice,
                            w.dataRate);
        Simulator::Schedule(w.endTime,
                            &LinkDown,
                            agentIt->second,
                            w.toEID,
                            linkIt->second.localDevice,
                            linkIt->second.remoteDevice);
    }

    Ptr<BundleAgent> dsnAgent = agentMap["dtn://earth/dsn"];

    Simulator::Schedule(Seconds(2000.0), &CheckBacklog, dsnAgent);

    Simulator::Schedule(Seconds(1000.0),
                        &SendPacedBundle,
                        dsnAgent,
                        "dtn://mars/ingenuity",
                        g_bundlesSent,
                        g_bundlesSent);

    const Time monitorInterval = Seconds(1.0);

    // Simulator::Schedule(Seconds(1.0),
    //                     &MonitorStorage,
    //                     agentMap["dtn://mars/tgo"],
    //                     "Mars TGO",
    //                     monitorInterval);
    // Simulator::Schedule(Seconds(1.0),
    //                     &MonitorStorage,
    //                     agentMap["dtn://mars/mro"],
    //                     "Mars MRO",
    //                     monitorInterval);
    // Simulator::Schedule(Seconds(1.0),
    //                     &MonitorStorage,
    //                     agentMap["dtn://earth/dsn"],
    //                     "Earth DSN",
    //                     monitorInterval);

    NS_LOG_INFO("Starting Mars Relay Network simulation...");

    Simulator::Stop(Seconds(86400.0));
    Simulator::Run();

    NS_LOG_INFO("Simulation finished.");
    NS_LOG_INFO("Bundles sent:     " << g_bundlesSent);
    NS_LOG_INFO("Bundles received: " << g_bundlesReceived);

    if (g_bundlesReceived > 0)
    {
        NS_LOG_INFO("Delivery ratio:   " << (100.0 * g_bundlesReceived / g_bundlesSent) << " %");
        NS_LOG_INFO("Average OWT:      " << (g_totalDelaySeconds / g_bundlesReceived) << " s");
    }

    for (const auto& [name, stats] : g_storageStats)
    {
        const double avg = stats.sampleCount > 0
                               ? static_cast<double>(stats.cumulativeBundles) / stats.sampleCount
                               : 0.0;
        NS_LOG_INFO(name << " | avg backlog: " << avg << " bundles | peak: " << stats.peakBundles
                         << " bundles at t=" << stats.peakTime << " s");
    }

    Simulator::Destroy();
    return 0;
}
