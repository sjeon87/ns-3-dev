/*
 * Copyright (c) 2025 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/node-level-scheduler.h"

#include <cstdio> 
#include <fstream>
#include <iostream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("NodeLevelSchedulerExample");

/**
 * @brief Simple event handler to print execution time.
 */
void
Ping(uint32_t nodeId)
{
    Time now = Simulator::Now();
    std::cout << "  [SimTime=" << now.GetSeconds() 
              << "s] Executing Ping for Node " << nodeId << std::endl;
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
        outFile << "1,0,50,0,100,2.0\n";
        
        // Node 2: Slow Clock (Skew 0.5)
        // Global: 0-200s -> Local: 0-100s (Time moves 0.5x speed)
        outFile << "2,0,200,0,100,0.5\n";

        outFile.close();
        std::cout << "Created temporary interval file: " << csvFilename << std::endl;
    }

    ObjectFactory schedulerFactory;
    schedulerFactory.SetTypeId("ns3::NodeLevelScheduler");
    
    schedulerFactory.Set("IntervalFile", StringValue(csvFilename));
    
    schedulerFactory.Set("WindowSize", TimeValue(Seconds(60.0))); 
    
    schedulerFactory.Set("UpdatePeriod", TimeValue(Seconds(10.0)));

    Simulator::SetScheduler(schedulerFactory);

    NodeContainer nodes;
    nodes.Create(3);

    std::cout << "--- Scheduling Events ---" << std::endl;
    std::cout << "Target: All nodes schedule 'Ping' at Local Time 10.0s" << std::endl;
    std::cout << "-------------------------" << std::endl;

    Simulator::ScheduleWithContext(0, Seconds(10.0), &Ping, 0);
    Simulator::ScheduleWithContext(1, Seconds(10.0), &Ping, 1);
    Simulator::ScheduleWithContext(2, Seconds(10.0), &Ping, 2);

    Simulator::Stop(Seconds(30.0));
    Simulator::Run();
    Simulator::Destroy();

    std::remove(csvFilename.c_str());
    std::cout << "Deleted temporary interval file." << std::endl;

    return 0;
}