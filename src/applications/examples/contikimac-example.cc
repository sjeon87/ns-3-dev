/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ns3/bounded-skew-scheduler.h"
#include "ns3/core-module.h"
#include "ns3/dynamic-skew-scheduler.h"
#include "ns3/epoch-table.h"
#include "ns3/internet-module.h"
#include "ns3/local-clock-helper.h"
#include "ns3/mac48-address.h"
#include "ns3/node-container.h"
#include "ns3/ntp-client.h"
#include "ns3/ntp-server.h"
#include "ns3/point-to-point-module.h"
#include "ns3/scheduler-clock.h"
#include "ns3/simple-channel.h"
#include "ns3/simple-net-device.h"
#include "ns3/static-skew-scheduler.h"

#include <iostream>
#include <vector>

/**
 * @file
 * @ingroup applications
 *
 * ContikiMAC duty cycling between a sender and a receiver with a skewed clock.
 */

using namespace ns3;

Time g_ts;                       //!< Transmission duration
Time g_ti;                       //!< Gap between transmissions
Time g_tc;                       //!< Gap between the two CCAs
Time g_tr;                       //!< CCA duration
Time g_wakeInterval;             //!< Receiver wake interval
bool g_channelActive = false;    //!< Whether the sender is transmitting
Time g_nextTxStart = Seconds(0); //!< Start of the next transmission

/**
 * @brief One receiver wake cycle.
 */
struct Message
{
    double cycleStart; //!< Time CCA1 fired, in seconds
    bool hit;          //!< Whether either CCA heard the sender
};

std::vector<Message> g_messages; //!< Every wake cycle

Ptr<SimpleNetDevice> g_senderDevice;   //!< Sender device
Ptr<SimpleNetDevice> g_receiverDevice; //!< Receiver device

void SenderTxEnd();
void ReceiverCCA1(uint32_t context);

/**
 * @brief Accept a received packet.
 * @return true
 */
bool
ReceiverReceive(Ptr<NetDevice>, Ptr<const Packet>, uint16_t, const Address&)
{
    return true;
}

/**
 * @brief Check whether the channel is busy during a CCA.
 * @param start CCA start.
 * @param end CCA end.
 * @return true if busy.
 */
bool
IsChannelBusyDuring(Time start, Time end)
{
    return g_channelActive || (g_nextTxStart >= start && g_nextTxStart < end);
}

/**
 * @brief Start a transmission.
 */
void
SenderTxStart()
{
    g_channelActive = true;
    g_senderDevice->Send(Create<Packet>(64), g_receiverDevice->GetAddress(), 0x0001);
    Simulator::Schedule(g_ts, &SenderTxEnd);
}

/**
 * @brief End a transmission.
 */
void
SenderTxEnd()
{
    g_channelActive = false;
    g_nextTxStart = Simulator::Now() + g_ti;
    Simulator::Schedule(g_ti, &SenderTxStart);
}

/**
 * @brief Second CCA of a wake cycle.
 * @param context receiver node ID.
 */
void
ReceiverCCA2(uint32_t context)
{
    Time now = Simulator::Now();
    if (IsChannelBusyDuring(now, now + g_tr))
    {
        g_messages.back().hit = true;
    }
    Simulator::ScheduleWithContext(context, g_wakeInterval - g_tc, &ReceiverCCA1, context);
}

/**
 * @brief First CCA of a wake cycle.
 * @param context receiver node ID.
 */
void
ReceiverCCA1(uint32_t context)
{
    Time now = Simulator::Now();
    g_messages.push_back({now.GetSeconds(), false});

    if (IsChannelBusyDuring(now, now + g_tr))
    {
        g_messages.back().hit = true;
        Simulator::ScheduleWithContext(context, g_wakeInterval, &ReceiverCCA1, context);
    }
    else
    {
        Simulator::ScheduleWithContext(context, g_tc, &ReceiverCCA2, context);
    }
}

int
main(int argc, char* argv[])
{
    uint32_t tsUs = 2082;
    uint32_t tiUs = 1367;
    uint32_t tcUs = 612;
    uint32_t trUs = 333;
    uint32_t phaseUs = 1000;
    double skew = 1.0005;
    double skewMin = 0.0;
    double skewMax = 0.0;
    double simTime = 50.0;
    bool mapScheduler = false;
    bool bounded = false;
    uint32_t epsilonUs = 500;
    bool ntp = false;
    double ntpPollS = 1.0;
    uint32_t runNumber = 1;

    CommandLine cmd(__FILE__);
    cmd.AddValue("ts", "Transmission duration (us)", tsUs);
    cmd.AddValue("ti", "Gap between transmissions (us)", tiUs);
    cmd.AddValue("tc", "Gap between CCAs (us)", tcUs);
    cmd.AddValue("tr", "CCA duration (us)", trUs);
    cmd.AddValue("phase", "Receiver's first CCA offset (us)", phaseUs);
    cmd.AddValue("skew", "Receiver clock skew", skew);
    cmd.AddValue("skewMin", "Lower bound of a skew band (0 uses --skew)", skewMin);
    cmd.AddValue("skewMax", "Upper bound of a skew band (0 uses --skew)", skewMax);
    cmd.AddValue("simTime", "Simulation time (s)", simTime);
    cmd.AddValue("mapScheduler", "Use the MapScheduler", mapScheduler);
    cmd.AddValue("bounded", "Use the BoundedSkewScheduler", bounded);
    cmd.AddValue("epsilon", "BoundedSkewScheduler drift bound (us)", epsilonUs);
    cmd.AddValue("ntp", "Use the DynamicSkewScheduler with NTP", ntp);
    cmd.AddValue("ntpPoll", "NTP poll interval (s)", ntpPollS);
    cmd.AddValue("run", "RNG run number", runNumber);
    cmd.Parse(argc, argv);

    bool banded = skewMin > 0.0 && skewMax > 0.0;
    if (banded)
    {
        NS_ABORT_MSG_IF(skewMin > skewMax, "skewMin must not exceed skewMax");
        skew = 0.5 * (skewMin + skewMax);
    }
    else
    {
        skewMin = skewMax = skew;
    }

    LogComponentDisableAll(LOG_LEVEL_ALL);
    RngSeedManager::SetRun(runNumber);

    g_ts = MicroSeconds(tsUs);
    g_ti = MicroSeconds(tiUs);
    g_tc = MicroSeconds(tcUs);
    g_tr = MicroSeconds(trUs);
    g_wakeInterval = (g_ts + g_ti) * 50;

    ObjectFactory factory;
    if (mapScheduler)
    {
        factory.SetTypeId("ns3::MapScheduler");
    }
    else if (ntp)
    {
        factory.SetTypeId("ns3::DynamicSkewScheduler");
        factory.Set("MinimumSkew", DoubleValue(skewMin));
        factory.Set("MaximumSkew", DoubleValue(skewMax));
        factory.Set("UpdatePeriod", TimeValue(Seconds(ntpPollS) / 2));
    }
    else if (bounded)
    {
        // Symmetric band about 1.0.
        factory.SetTypeId("ns3::BoundedSkewScheduler");
        factory.Set("MinimumSkew", DoubleValue(2.0 - skewMax));
        factory.Set("MaximumSkew", DoubleValue(skewMax));
        factory.Set("Epsilon", TimeValue(MicroSeconds(epsilonUs)));
    }
    else
    {
        factory.SetTypeId("ns3::StaticSkewScheduler");
        factory.Set("MinimumSkew", DoubleValue(skewMin));
        factory.Set("MaximumSkew", DoubleValue(skewMax));
    }
    Simulator::SetScheduler(factory);

    NodeContainer nodes;
    nodes.Create(2);
    Ptr<Node> senderNode = nodes.Get(0);
    Ptr<Node> receiverNode = nodes.Get(1);

    Ptr<SimpleChannel> channel = CreateObject<SimpleChannel>();

    g_senderDevice = CreateObject<SimpleNetDevice>();
    g_senderDevice->SetAddress(Mac48Address::Allocate());
    senderNode->AddDevice(g_senderDevice);
    g_senderDevice->SetNode(senderNode);
    g_senderDevice->SetChannel(channel);

    g_receiverDevice = CreateObject<SimpleNetDevice>();
    g_receiverDevice->SetAddress(Mac48Address::Allocate());
    receiverNode->AddDevice(g_receiverDevice);
    g_receiverDevice->SetNode(receiverNode);
    g_receiverDevice->SetChannel(channel);
    g_receiverDevice->SetReceiveCallback(MakeCallback(&ReceiverReceive));

    uint32_t receiverContext = receiverNode->GetId();

    if (ntp)
    {
        Ptr<EpochTable> table = DynamicSkewScheduler::GetCurrentEpochTable();
        NS_ABORT_MSG_IF(!table, "The dynamic skew scheduler is not active");

        // The sender is the time reference.
        EpochTable::Epoch reference;
        reference.simulatorStartTime = Seconds(0);
        reference.simulatorEndTime = Seconds(simTime) + Seconds(10.0);
        reference.nodeStartTime = Seconds(0);
        reference.nodeEndTime = reference.simulatorEndTime;
        reference.skew = 1.0;
        table->AddEpoch(senderNode->GetId(), reference);

        // NTP runs on a separate link.
        PointToPointHelper p2p;
        p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
        p2p.SetChannelAttribute("Delay", StringValue("2ms"));
        NetDeviceContainer ntpDevices = p2p.Install(nodes);

        InternetStackHelper internet;
        internet.Install(nodes);

        Ipv4AddressHelper address;
        address.SetBase("10.1.1.0", "255.255.255.0");
        Ipv4InterfaceContainer ntpInterfaces = address.Assign(ntpDevices);

        LocalClockHelper clockHelper;
        clockHelper.SetClockType("ns3::SchedulerClock");
        clockHelper.Install(receiverNode);
        Ptr<SchedulerClock> receiverClock = receiverNode->GetObject<SchedulerClock>();
        receiverClock->SetNodeId(receiverContext);
        receiverClock->SetEpochTable(table);

        uint16_t ntpPort = 123;

        Ptr<NtpServer> serverApp = CreateObject<NtpServer>();
        serverApp->Setup(ntpPort, MicroSeconds(200));
        senderNode->AddApplication(serverApp);
        serverApp->SetStartTime(Seconds(0.0));
        serverApp->SetStopTime(Seconds(simTime));

        Ptr<NtpClient> clientApp = CreateObject<NtpClient>();
        clientApp->Setup(ntpInterfaces.GetAddress(0), ntpPort, Seconds(ntpPollS));
        receiverNode->AddApplication(clientApp);
        clientApp->SetStartTime(Seconds(0.5));
        clientApp->SetStopTime(Seconds(simTime));
    }

    Simulator::Schedule(Seconds(0.0), &SenderTxStart);
    Simulator::ScheduleWithContext(receiverContext,
                                   MicroSeconds(phaseUs),
                                   &ReceiverCCA1,
                                   receiverContext);

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();
    Simulator::Destroy();

    // <cycle>,<cycle start s>,<hit>
    std::cout << "MESSAGES:" << std::endl;
    for (std::size_t i = 0; i < g_messages.size(); ++i)
    {
        std::cout << i << "," << g_messages[i].cycleStart << "," << (g_messages[i].hit ? 1 : 0)
                  << std::endl;
    }

    return 0;
}
