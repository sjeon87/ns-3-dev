/*
 * Copyright (c) 2025 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ns3/core-module.h"
#include "ns3/multi-rate-clock.h"
#include "ns3/network-module.h"
#include "ns3/node-level-scheduler.h"

#include <cstdio>
#include <fstream>
#include <iostream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("NodeLevelSchedulerExample");

/**
 * @brief Event handler to print execution time and verify local clock.
 */
void
Ping(Ptr<Node> node)
{
    Time simTime = Simulator::Now();
    Time localTime = node->GetLocalTime(); // Queries MultiRateClock

    std::cout << "  [SimTime=" << simTime.GetSeconds() << "s] "
              << "Node " << node->GetId() << " Ping Executed. "
              << "Node believes it is: " << localTime.GetSeconds() << "s" << std::endl;
}

int
main(int argc, char* argv[])
{
    LogComponentEnable("NodeLevelScheduler", LOG_LEVEL_LOGIC);

    std::string csvFilename = "node-intervals.csv";
    {
        std::ofstream outFile(csvFilename);

        // File Format: nodeId, simStart, simEnd, nodeStart, nodeEnd, skew

        // Node 0: Normal Clock (Skew 1.0)
        // Global: 0-100s -> Local: 0-100s
        outFile << "0,0,100,0,100,1.0\n";

        // Node 1: Fast Clock (Skew 2.0)
        // Global: 0-50s -> Local: 0-100s (Time moves 2x faster)
        // A scheduled event at NodeTime 10s should happen at SimTime 5s.
        outFile << "1,0,50,0,100,2.0\n";

        // Node 2: Slow Clock (Skew 0.5)
        // Global: 0-200s -> Local: 0-100s (Time moves 0.5x speed)
        // A scheduled event at NodeTime 10s should happen at SimTime 20s.
        outFile << "2,0,200,0,100,0.5\n";

        outFile.close();
        std::cout << "Created temporary interval file: " << csvFilename << std::endl;
    }

    // 1. Configure the Scheduler
    ObjectFactory schedulerFactory;
    schedulerFactory.SetTypeId("ns3::NodeLevelScheduler");
    schedulerFactory.Set("IntervalFile", StringValue(csvFilename));
    schedulerFactory.Set("WindowSize", TimeValue(Seconds(60.0)));
    schedulerFactory.Set("UpdatePeriod", TimeValue(Seconds(10.0)));
    Simulator::SetScheduler(schedulerFactory);

    // 2. Create Nodes
    NodeContainer nodes;
    nodes.Create(3);

    // 3. Install MultiRateClock on ALL nodes
    // This is crucial: The scheduler handles the event dispatch time,
    // but the clock handles what the node replies when asked "GetLocalTime()".
    for (uint32_t i = 0; i < nodes.GetN(); ++i)
    {
        Ptr<Node> node = nodes.Get(i);
        Ptr<MultiRateClock> clock = CreateObject<MultiRateClock>();

        // Link the clock to the specific Node ID so it checks the right intervals
        clock->SetNodeId(node->GetId());

        // Replace the default clock with our MultiRateClock
        node->SetAttribute("LocalClock", PointerValue(clock));
    }

    std::cout << "--- Scheduling Events ---" << std::endl;
    std::cout << "Target: All nodes schedule 'Ping' at their Local Time 10.0s" << std::endl;
    std::cout << "-------------------------" << std::endl;

    // ScheduleWithContext(nodeId, delay, ...)
    // "Delay" is interpreted as time relative to the Node's specific clock.
    Simulator::ScheduleWithContext(0, Seconds(10.0), &Ping, nodes.Get(0));
    Simulator::ScheduleWithContext(1, Seconds(10.0), &Ping, nodes.Get(1));
    Simulator::ScheduleWithContext(2, Seconds(10.0), &Ping, nodes.Get(2));

    Simulator::Stop(Seconds(30.0));
    Simulator::Run();
    Simulator::Destroy();

    std::remove(csvFilename.c_str());
    std::cout << "Deleted temporary interval file." << std::endl;

    return 0;
}
