/*
 * Copyright (c) 2025 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"

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
    
    std::string intervals = 
        "0,0,100,0,100,1.0;"
        "1,0,50,0,100,2.0;"
        "2,0,200,0,100,0.5;";

    ObjectFactory schedulerFactory;
    schedulerFactory.SetTypeId("ns3::NodeLevelScheduler");
    schedulerFactory.Set("Intervals", StringValue(intervals));
    
    schedulerFactory.Set("WindowSize", TimeValue(Seconds(500.0))); 
    schedulerFactory.Set("UpdatePeriod", TimeValue(Seconds(10.0)));

    Simulator::SetScheduler(schedulerFactory);

    NodeContainer nodes;
    nodes.Create(3);

    std::cout << "--- Scheduling Events ---" << std::endl;
    std::cout << "Scheduling Ping for Node 0 at Local Time 10.0s" << std::endl;
    std::cout << "Scheduling Ping for Node 1 at Local Time 10.0s" << std::endl;
    std::cout << "Scheduling Ping for Node 2 at Local Time 10.0s" << std::endl;
    std::cout << "-------------------------" << std::endl;

    Simulator::ScheduleWithContext(0, Seconds(10.0), &Ping, 0);
    Simulator::ScheduleWithContext(1, Seconds(10.0), &Ping, 1);
    Simulator::ScheduleWithContext(2, Seconds(10.0), &Ping, 2);

    // --- 4. Run ---
    Simulator::Stop(Seconds(30.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}