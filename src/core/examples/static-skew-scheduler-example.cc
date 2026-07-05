/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ns3/core-module.h"
#include "ns3/epoch-table.h"
#include "ns3/network-module.h"
#include "ns3/static-skew-scheduler.h"

/**
 * @file
 * @ingroup core-examples
 * @ingroup scheduler
 *
 * This example demonstrates the Static Skew Scheduler (SSS): two nodes are each given the
 * same "N seconds from now" local-time delay, but because each node draws its own
 * random clock skew, they fire at different simulator times.
 */

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("StaticSkewSchedulerExample");

/**
 * @brief Report a node-context event's firing time and cross-check it against the Epoch Table.
 * @param nodeId the node the event fired on.
 * @param requestedLocalDelay the local-time delay originally requested for this event.
 */
void
ReportFiring(uint32_t nodeId, Time requestedLocalDelay)
{
    Ptr<EpochTable> table = StaticSkewScheduler::GetCurrentEpochTable();
    Time now = Simulator::Now();

    std::cout << "Node " << nodeId << ": requested " << requestedLocalDelay.As(Time::S)
              << " of local-time delay, fired at simulator time " << now.As(Time::S) << std::endl;

    if (table)
    {
        std::cout << "  Epoch table for node " << nodeId << " now covers local time up to "
                  << table->GetMaxNodeTime(nodeId).As(Time::S) << ", simulator time up to "
                  << table->GetMaxSimulatorTime(nodeId).As(Time::S) << std::endl;
    }
}

int
main(int argc, char* argv[])
{
    double simTime = 30.0;
    double updatePeriodS = 10.0;
    double windowSizeS = 100.0;
    double minSkew = 0.5;
    double maxSkew = 2.5;
    double localDelayS = 5.0;
    uint32_t seed = 1;

    CommandLine cmd(__FILE__);
    cmd.AddValue("simTime", "Total simulation time, in seconds", simTime);
    cmd.AddValue("updatePeriod", "How often the skew changes (upsilon), in seconds", updatePeriodS);
    cmd.AddValue("windowSize",
                 "Lookahead window for epoch table extension, in seconds",
                 windowSizeS);
    cmd.AddValue("minSkew", "Minimum skew drawable per epoch", minSkew);
    cmd.AddValue("maxSkew", "Maximum skew drawable per epoch", maxSkew);
    cmd.AddValue("localDelay",
                 "Local-time delay requested for each node's event, in seconds",
                 localDelayS);
    cmd.AddValue("seed", "RNG seed, for reproducible skew draws", seed);
    cmd.Parse(argc, argv);

    RngSeedManager::SetSeed(seed);
    RngSeedManager::SetRun(1);

    ObjectFactory schedulerFactory;
    schedulerFactory.SetTypeId("ns3::StaticSkewScheduler");
    schedulerFactory.Set("UpdatePeriod", TimeValue(Seconds(updatePeriodS)));
    schedulerFactory.Set("WindowSize", TimeValue(Seconds(windowSizeS)));
    schedulerFactory.Set("MinimumSkew", DoubleValue(minSkew));
    schedulerFactory.Set("MaximumSkew", DoubleValue(maxSkew));
    Simulator::SetScheduler(schedulerFactory);

    NodeContainer nodes;
    nodes.Create(2);

    Time localDelay = Seconds(localDelayS);
    for (auto it = nodes.Begin(); it != nodes.End(); ++it)
    {
        uint32_t nodeId = (*it)->GetId();
        Simulator::ScheduleWithContext(nodeId, localDelay, &ReportFiring, nodeId, localDelay);
    }

    std::cout << "Both nodes requested the same " << localDelay.As(Time::S)
              << " of local-time delay; watch how their independently-drawn clock skews change "
                 "how long that delay actually takes in simulator time."
              << std::endl;

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
