/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ns3/core-module.h"
#include "ns3/dynamic-skew-scheduler.h"
#include "ns3/network-module.h"

/**
 * @file
 * @ingroup core-examples
 * @ingroup scheduler
 *
 * This example demonstrates the Dynamic Skew Scheduler (DSS) reordering pending events in
 * O(log N) when a node's skew changes mid-simulation, rather than recomputing every pending
 * event.
 */

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("DynamicSkewSchedulerExample");

/**
 * @brief Report a node-context event's firing time.
 * @param nodeId the node the event fired on.
 * @param label a short label identifying which event this is.
 */
void
ReportFiring(uint32_t nodeId, std::string label)
{
    std::cout << Simulator::Now().As(Time::S) << ": " << label << " (node " << nodeId << ") fired"
              << std::endl;
}

int
main(int argc, char* argv[])
{
    double simTime = 20.0;
    double updatePeriodS = 10.0;
    double windowSizeS = 5.0;
    double node1InitialSkew = 1.0;
    double node2Skew = 1.5;
    double node1CorrectedSkew = 2.0;
    double correctionTimeS = 5.0;
    double node1DelayS = 12.0;
    double node2DelayS = 15.0;

    CommandLine cmd(__FILE__);
    cmd.AddValue("simTime", "Total simulation time, in seconds", simTime);
    cmd.AddValue("node1InitialSkew", "Node 1's initial skew", node1InitialSkew);
    cmd.AddValue("node2Skew", "Node 2's fixed skew (never changes)", node2Skew);
    cmd.AddValue("node1CorrectedSkew", "Node 1's skew after the correction", node1CorrectedSkew);
    cmd.AddValue("correctionTime", "When the correction is applied, in seconds", correctionTimeS);
    cmd.AddValue("node1Delay", "Node 1's requested local-time delay, in seconds", node1DelayS);
    cmd.AddValue("node2Delay", "Node 2's requested local-time delay, in seconds", node2DelayS);
    cmd.Parse(argc, argv);

    ObjectFactory schedulerFactory;
    schedulerFactory.SetTypeId("ns3::DynamicSkewScheduler");
    schedulerFactory.Set("UpdatePeriod", TimeValue(Seconds(updatePeriodS)));
    schedulerFactory.Set("WindowSize", TimeValue(Seconds(windowSizeS)));
    schedulerFactory.Set("MinimumSkew", DoubleValue(0.5));
    schedulerFactory.Set("MaximumSkew", DoubleValue(2.0));
    Simulator::SetScheduler(schedulerFactory);

    NodeContainer nodes;
    nodes.Create(2);
    uint32_t node1Id = nodes.Get(0)->GetId();
    uint32_t node2Id = nodes.Get(1)->GetId();

    DynamicSkewScheduler::ChangeCurrentSkew(node1Id, node1InitialSkew);
    DynamicSkewScheduler::ChangeCurrentSkew(node2Id, node2Skew);

    Simulator::ScheduleWithContext(node1Id, Seconds(node1DelayS), &ReportFiring, node1Id, "e1");
    Simulator::ScheduleWithContext(node2Id, Seconds(node2DelayS), &ReportFiring, node2Id, "e2");

    std::cout << "Watching two nodes with independent local clocks; a mid-simulation skew "
                 "correction on node "
              << node1Id << " at t=" << correctionTimeS
              << "s will change the relative order in which their pending events fire."
              << std::endl;

    Simulator::Schedule(Seconds(correctionTimeS),
                        &DynamicSkewScheduler::ChangeCurrentSkew,
                        node1Id,
                        node1CorrectedSkew);

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
