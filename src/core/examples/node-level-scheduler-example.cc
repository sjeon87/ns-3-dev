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

#include <iostream>
#include <iomanip>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("NodeLevelSchedulerExample");

void
Ping(Ptr<Node> node)
{
    Time simTime = Simulator::Now();
    
    Time localTime = node->GetLocalTime(); 

    std::cout << "  [SimTime=" << std::fixed << std::setprecision(4) << simTime.GetSeconds() << "s] "
              << "Node " << node->GetId() << " Ping Executed. "
              << "Node Local Time: " << localTime.GetSeconds() << "s" << std::endl;
}

int
main(int argc, char* argv[])
{

    ObjectFactory schedulerFactory;
    schedulerFactory.SetTypeId("ns3::NodeLevelScheduler");
    
    schedulerFactory.Set("MinimumSkew", DoubleValue(0.5)); 
    schedulerFactory.Set("MaximumSkew", DoubleValue(2.0)); 
    schedulerFactory.Set("UpdatePeriod", TimeValue(Seconds(10.0)));
    schedulerFactory.Set("WindowSize", TimeValue(Seconds(60.0)));
    
    Simulator::SetScheduler(schedulerFactory);

    NodeContainer nodes;
    nodes.Create(3);

    for (uint32_t i = 0; i < nodes.GetN(); ++i)
    {
        Ptr<Node> node = nodes.Get(i);
        Ptr<MultiRateClock> clock = CreateObject<MultiRateClock>();
        clock->SetNodeId(node->GetId());
        node->SetAttribute("LocalClock", PointerValue(clock));
    }
    
    Simulator::ScheduleWithContext(0, Seconds(10.0), &Ping, nodes.Get(0));
    Simulator::ScheduleWithContext(1, Seconds(10.0), &Ping, nodes.Get(1));
    Simulator::ScheduleWithContext(2, Seconds(10.0), &Ping, nodes.Get(2));

    Simulator::Stop(Seconds(60.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}