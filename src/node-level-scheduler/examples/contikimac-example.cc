/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ns3/core-module.h"
#include "ns3/mac48-address.h"
#include "ns3/node-container.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/simple-channel.h"
#include "ns3/simple-net-device.h"
#include "ns3/static-skew-scheduler.h"

#include <cmath>
#include <numeric>
#include <utility>
#include <vector>

/**
 * @file
 * @ingroup node-level-scheduler
 *
 * This example demonstrates ContikiMAC-style radio duty cycling over a real sender/receiver
 * node pair connected by a SimpleNetDevice/SimpleChannel: the sender periodically transmits a
 * real packet for a fixed burst duration, and the receiver performs periodic clear-channel
 * assessments (CCAs) using its own (possibly skewed) local clock.
 */

using namespace ns3;

Time g_ts, g_ti, g_tc, g_ccaWakeupInterval;
bool g_isChannelActive = false;

bool g_inBlackout = false;
Time g_blackoutStart = Seconds(0);
Time g_prevBlackoutStart = Seconds(0);
std::vector<double> g_observedDurations;
std::vector<double> g_observedIntervals;

std::vector<std::pair<double, int>> g_timeline;

/**
 * @brief One receiver wake cycle and whether either of its CCAs sensed the channel as busy.
 */
struct Message
{
    double cycleStart; //!< Simulation time the wake cycle's CCA1 fired, in seconds
    bool hit;           //!< Whether CCA1 or its CCA2 followup sensed the channel as busy
};

std::vector<Message> g_messages;

Ptr<SimpleNetDevice> g_senderDevice;
Ptr<SimpleNetDevice> g_receiverDevice;

void SenderTxStart();
void SenderTxEnd();
void ReceiverCCA1(uint32_t context);
void ReceiverCCA2(uint32_t context);

/**
 * @brief No-op receive handler
 * @return true (packet accepted)
 */
bool
ReceiverReceive(Ptr<NetDevice>, Ptr<const Packet>, uint16_t, const Address&)
{
    return true;
}

void
SenderTxStart()
{
    g_isChannelActive = true;

    Ptr<Packet> packet = Create<Packet>(64);
    g_senderDevice->Send(packet, g_receiverDevice->GetAddress(), 0x0001);

    Simulator::Schedule(g_ts, &SenderTxEnd);
}

void
SenderTxEnd()
{
    g_isChannelActive = false;
    Simulator::Schedule(g_ti, &SenderTxStart);
}

void
ReceiverCCA2(uint32_t context)
{
    Time now = Simulator::Now();
    if (g_isChannelActive)
    {
        if (g_inBlackout)
        {
            g_inBlackout = false;
            g_observedDurations.push_back((now - g_blackoutStart).GetSeconds());
        }
        g_timeline.emplace_back(now.GetSeconds(), 1);
        g_messages.back().hit = true;
    }
    else
    {
        if (!g_inBlackout)
        {
            g_inBlackout = true;
            g_blackoutStart = now;
            if (g_prevBlackoutStart.GetSeconds() > 0)
            {
                g_observedIntervals.push_back((g_blackoutStart - g_prevBlackoutStart).GetSeconds());
            }
            g_prevBlackoutStart = g_blackoutStart;
        }
        g_timeline.emplace_back(now.GetSeconds(), 0);
    }

    Simulator::ScheduleWithContext(context, g_ccaWakeupInterval - g_tc, &ReceiverCCA1, context);
}

void
ReceiverCCA1(uint32_t context)
{
    Time now = Simulator::Now();
    g_messages.push_back({now.GetSeconds(), false});

    if (g_isChannelActive)
    {
        if (g_inBlackout)
        {
            g_inBlackout = false;
            g_observedDurations.push_back((now - g_blackoutStart).GetSeconds());
        }
        g_timeline.emplace_back(now.GetSeconds(), 1);
        g_messages.back().hit = true;
        Simulator::ScheduleWithContext(context, g_ccaWakeupInterval, &ReceiverCCA1, context);
    }
    else
    {
        Simulator::ScheduleWithContext(context, g_tc, &ReceiverCCA2, context);
    }
}

int
main(int argc, char* argv[])
{
    uint32_t ts_us = 2082;
    uint32_t ti_us = 1367;
    uint32_t tc_us = 612;
    double skew = 1.0005;
    double simTime = 50.0;

    CommandLine cmd(__FILE__);
    cmd.AddValue("ts", "Transmission duration in microseconds", ts_us);
    cmd.AddValue("ti", "Inter-packet gap in microseconds", ti_us);
    cmd.AddValue("tc", "Time between CCAs in microseconds", tc_us);
    cmd.AddValue("skew", "Clock skew multiplier", skew);
    cmd.AddValue("simTime", "Simulation time in seconds", simTime);
    cmd.Parse(argc, argv);

    LogComponentDisableAll(LOG_LEVEL_ALL);

    g_ts = MicroSeconds(ts_us);
    g_ti = MicroSeconds(ti_us);
    g_tc = MicroSeconds(tc_us);

    Time totalCycleTime = g_ts + g_ti;
    g_ccaWakeupInterval = totalCycleTime * 50;

    ObjectFactory factory;
    factory.SetTypeId("ns3::StaticSkewScheduler");
    factory.Set("MinimumSkew", DoubleValue(skew));
    factory.Set("MaximumSkew", DoubleValue(skew));
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

    Simulator::Schedule(Seconds(0.0), &SenderTxStart);
    Simulator::ScheduleWithContext(receiverContext,
                                   MicroSeconds(1000),
                                   &ReceiverCCA1,
                                   receiverContext);

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();
    Simulator::Destroy();

    double delta_f = std::abs(skew - 1.0);
    double theo_dur = 0.0;
    double theo_rep = 0.0;

    if (delta_f > 0.0 && g_ti > g_tc)
    {
        theo_dur = (g_ti.GetSeconds() - g_tc.GetSeconds()) / delta_f;
        theo_rep = (g_ts.GetSeconds() + g_tc.GetSeconds()) / delta_f;
    }

    double sim_dur = 0.0;
    double sim_rep = 0.0;
    if (!g_observedDurations.empty())
    {
        sim_dur = std::accumulate(g_observedDurations.begin(), g_observedDurations.end(), 0.0) /
                  g_observedDurations.size();
    }
    if (!g_observedIntervals.empty())
    {
        sim_rep = std::accumulate(g_observedIntervals.begin(), g_observedIntervals.end(), 0.0) /
                  g_observedIntervals.size();
    }

    std::cout << "Params: ts=" << ts_us << "us, ti=" << ti_us << "us, tc=" << tc_us
              << "us, skew=" << skew << " | Theo: Dur=" << theo_dur << "s, Rep=" << theo_rep << "s"
              << " | Sim: Dur=" << sim_dur << "s, Rep=" << sim_rep << "s" << std::endl;

    std::cout << "TIMELINE:" << std::endl;
    for (const auto& event : g_timeline)
    {
        std::cout << event.first << "," << event.second << std::endl;
    }

    std::cout << "MESSAGES:" << std::endl;
    for (std::size_t i = 0; i < g_messages.size(); ++i)
    {
        std::cout << i << "," << g_messages[i].cycleStart << "," << (g_messages[i].hit ? 1 : 0)
                  << std::endl;
    }

    return 0;
}
