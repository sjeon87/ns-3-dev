/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ntp-client.h"
#include "ntp-server.h"

#include "ns3/core-module.h"
#include "ns3/dynamic-skew-scheduler.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/local-clock-helper.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/scheduler-clock.h"

#include <fstream>
#include <vector>

/**
 * @file
 * @ingroup node-level-scheduler
 *
 * This example demonstrates a simplified NTP (Network Time Protocol) client/server exchange
 * using a star topology: one server node and three client nodes, each connected to the server
 * by its own point-to-point link.
 */

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("NtpExample");

/**
 * @brief Periodically samples each client's clock offset from the reference time
 * and writes it to a CSV trace file.
 */
class OffsetTracer
{
  public:
    /**
     * @brief Construct the tracer and write the CSV header.
     * @param clocks the clients' clocks.
     * @param outputFile path of the CSV file to write.
     * @param interval how often to sample the offsets.
     */
    OffsetTracer(std::vector<Ptr<SchedulerClock>> clocks,
                 const std::string& outputFile,
                 Time interval)
        : m_clocks(std::move(clocks)),
          m_out(outputFile),
          m_interval(interval)
    {
        m_out << "time_s,server_offset_ms";
        for (std::size_t i = 0; i < m_clocks.size(); ++i)
        {
            m_out << ",client" << (i + 1) << "_offset_ms";
        }
        m_out << std::endl;
    }

    /**
     * @brief Sample the current offsets and schedule the next sample.
     */
    void Sample()
    {
        Time now = Simulator::Now();
        m_out << now.GetSeconds() << ",0";
        for (const auto& clock : m_clocks)
        {
            double offsetMs = (clock->Now() - now).GetSeconds() * 1000.0;
            m_out << "," << offsetMs;
        }
        m_out << std::endl;

        m_event = Simulator::Schedule(m_interval, &OffsetTracer::Sample, this);
    }

    /**
     * @brief Cancel the pending sample event, if any.
     */
    void Stop()
    {
        Simulator::Cancel(m_event);
    }

  private:
    std::vector<Ptr<SchedulerClock>> m_clocks; //!< The clients' clocks, in log order
    std::ofstream m_out;                       //!< The CSV trace output stream
    Time m_interval;                           //!< The sampling interval
    EventId m_event;                           //!< The next scheduled sample
};

int
main(int argc, char* argv[])
{
    double simTime = 60.0;
    double pollIntervalS = 2.0;
    double driftPpmClient1 = 500.0;
    double driftPpmClient2 = -300.0;
    double driftPpmClient3 = 200.0;
    double skewJitterPpm = 15.0;
    double skewUpdatePeriodS = pollIntervalS * 0.5;
    std::string p2pDelay = "2ms";
    std::string p2pDataRate = "10Mbps";
    double serverProcessingUs = 200.0;
    std::string offsetTraceFile = "ntp-offset-trace.csv";
    bool verbose = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("simTime", "Total simulation time, in seconds", simTime);
    cmd.AddValue("pollInterval", "NTP poll interval, in seconds", pollIntervalS);
    cmd.AddValue("driftPpmClient1",
                 "Client 1's initial oscillator skew, in ppm",
                 driftPpmClient1);
    cmd.AddValue("driftPpmClient2",
                 "Client 2's initial oscillator skew, in ppm",
                 driftPpmClient2);
    cmd.AddValue("driftPpmClient3",
                 "Client 3's initial oscillator skew, in ppm",
                 driftPpmClient3);
    cmd.AddValue("skewJitterPpm",
                 "Half-width of the random oscillator skew jitter reasserted between NTP "
                 "corrections, in ppm (0 to disable)",
                 skewJitterPpm);
    cmd.AddValue("skewUpdatePeriod",
                 "How often the reasserted skew jitter is redrawn, in seconds.",
                 skewUpdatePeriodS);
    cmd.AddValue("p2pDelay", "Point-to-point channel propagation delay", p2pDelay);
    cmd.AddValue("p2pDataRate", "Point-to-point channel data rate", p2pDataRate);
    cmd.AddValue("serverProcessingDelay",
                 "Server-side NTP request processing delay, in microseconds",
                 serverProcessingUs);
    cmd.AddValue("offsetTraceFile",
                 "Path of the CSV file logging per-millisecond clock offsets",
                 offsetTraceFile);
    cmd.AddValue("verbose", "Enable NS_LOG output for NtpExample", verbose);
    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnable("NtpExample", LOG_LEVEL_INFO);
    }

    // Topology: server -- link1 -- client1
    //           server -- link2 -- client2
    //           server -- link3 -- client3
    NodeContainer all;
    all.Create(4);
    Ptr<Node> server = all.Get(0);
    Ptr<Node> client1 = all.Get(1);
    Ptr<Node> client2 = all.Get(2);
    Ptr<Node> client3 = all.Get(3);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue(p2pDataRate));
    p2p.SetChannelAttribute("Delay", StringValue(p2pDelay));

    NodeContainer link1Nodes(server, client1);
    NodeContainer link2Nodes(server, client2);
    NodeContainer link3Nodes(server, client3);

    NetDeviceContainer link1Devices = p2p.Install(link1Nodes);
    NetDeviceContainer link2Devices = p2p.Install(link2Nodes);
    NetDeviceContainer link3Devices = p2p.Install(link3Nodes);

    InternetStackHelper internet;
    internet.Install(all);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer link1Ifaces = address.Assign(link1Devices);
    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer link2Ifaces = address.Assign(link2Devices);
    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer link3Ifaces = address.Assign(link3Devices);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    ObjectFactory schedulerFactory;
    schedulerFactory.SetTypeId("ns3::DynamicSkewScheduler");
    schedulerFactory.Set("MinimumSkew", DoubleValue(1.0 - skewJitterPpm * 1.0e-6));
    schedulerFactory.Set("MaximumSkew", DoubleValue(1.0 + skewJitterPpm * 1.0e-6));
    schedulerFactory.Set("UpdatePeriod", TimeValue(Seconds(skewUpdatePeriodS)));
    Simulator::SetScheduler(schedulerFactory);

    NodeContainer clients(client1, client2, client3);
    LocalClockHelper clockHelper;
    clockHelper.SetClockType("ns3::SchedulerClock");
    clockHelper.Install(clients);

    Ptr<EpochTable> table = DynamicSkewScheduler::GetCurrentEpochTable();
    std::vector<Ptr<SchedulerClock>> clientClocks;
    for (auto it = clients.Begin(); it != clients.End(); ++it)
    {
        Ptr<Node> node = *it;
        Ptr<SchedulerClock> clock = node->GetObject<SchedulerClock>();
        clock->SetNodeId(node->GetId());
        clock->SetEpochTable(table);
        clientClocks.push_back(clock);
    }

    uint16_t ntpPort = 123;

    Ptr<NtpServer> serverApp = CreateObject<NtpServer>();
    serverApp->Setup(ntpPort, MicroSeconds(serverProcessingUs));
    server->AddApplication(serverApp);
    serverApp->SetStartTime(Seconds(0.0));
    serverApp->SetStopTime(Seconds(simTime));

    Ptr<NtpClient> clientApp1 = CreateObject<NtpClient>();
    clientApp1->Setup(link1Ifaces.GetAddress(0), ntpPort, Seconds(pollIntervalS));
    client1->AddApplication(clientApp1);
    clientApp1->SetStartTime(Seconds(0.5));
    clientApp1->SetStopTime(Seconds(simTime));

    Ptr<NtpClient> clientApp2 = CreateObject<NtpClient>();
    clientApp2->Setup(link2Ifaces.GetAddress(0), ntpPort, Seconds(pollIntervalS));
    client2->AddApplication(clientApp2);
    clientApp2->SetStartTime(Seconds(0.5));
    clientApp2->SetStopTime(Seconds(simTime));

    Ptr<NtpClient> clientApp3 = CreateObject<NtpClient>();
    clientApp3->Setup(link3Ifaces.GetAddress(0), ntpPort, Seconds(pollIntervalS));
    client3->AddApplication(clientApp3);
    clientApp3->SetStartTime(Seconds(0.5));
    clientApp3->SetStopTime(Seconds(simTime));

    DynamicSkewScheduler::ChangeCurrentSkew(client1->GetId(), 1.0 + driftPpmClient1 * 1.0e-6);
    DynamicSkewScheduler::ChangeCurrentSkew(client2->GetId(), 1.0 + driftPpmClient2 * 1.0e-6);
    DynamicSkewScheduler::ChangeCurrentSkew(client3->GetId(), 1.0 + driftPpmClient3 * 1.0e-6);

    std::cout << "sim-time\tnode\tT1\t\tT4\t\toffset\t\tdelay\t\taction" << std::endl;

    OffsetTracer offsetTracer(clientClocks, offsetTraceFile, MilliSeconds(1));
    Simulator::Schedule(Seconds(0.0), &OffsetTracer::Sample, &offsetTracer);

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();
    offsetTracer.Stop();
    Simulator::Destroy();

    return 0;
}
