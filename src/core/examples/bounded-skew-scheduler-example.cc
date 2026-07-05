/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ns3/bounded-skew-scheduler.h"
#include "ns3/core-module.h"
#include "ns3/epoch-table.h"
#include "ns3/network-module.h"

#include <algorithm>

/**
 * @file
 * @ingroup core-examples
 * @ingroup scheduler
 *
 * This example demonstrates the Bounded Skew Scheduler: a node's clock is allowed to drift
 * within [-Epsilon, +Epsilon] of Simulator::Now().
 */

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("BoundedSkewSchedulerExample");

/**
 * @brief No-op handler
 */
void
NoOp(uint32_t)
{
}

/**
 * @brief Periodically samples a node's clock offset from Simulator::Now() and tracks bounds.
 */
class OffsetSampler
{
  public:
    /**
     * @brief Construct the sampler.
     * @param nodeId the node to sample.
     * @param interval how often to sample, in simulator time.
     * @param epsilon the configured drift bound, for reporting purposes only.
     */
    OffsetSampler(uint32_t nodeId, Time interval, Time epsilon)
        : m_nodeId(nodeId),
          m_interval(interval),
          m_epsilon(epsilon)
    {
    }

    /**
     * @brief Sample the current offset and schedule the next sample.
     */
    void Sample()
    {
        Ptr<EpochTable> table = BoundedSkewScheduler::GetCurrentEpochTable();
        Time now = Simulator::Now();
        Time offset = table->GetNodeTimeFromSimulatorTime(m_nodeId, now) - now;

        if (!m_hasSample)
        {
            m_minOffset = offset;
            m_maxOffset = offset;
            m_hasSample = true;
        }
        else
        {
            m_minOffset = std::min(m_minOffset, offset);
            m_maxOffset = std::max(m_maxOffset, offset);
        }

        std::cout << now.As(Time::S) << ": node " << m_nodeId << " offset=" << offset.As(Time::MS)
                  << " (bound=+/-" << m_epsilon.As(Time::MS) << ")" << std::endl;

        Simulator::Schedule(m_interval, &OffsetSampler::Sample, this);
    }

    /**
     * @brief Print the running minimum and maximum observed offsets.
     */
    void ReportSummary() const
    {
        std::cout << "Observed offset range: [" << m_minOffset.As(Time::MS) << ", "
                  << m_maxOffset.As(Time::MS) << "], bound=+/-" << m_epsilon.As(Time::MS)
                  << std::endl;
    }

  private:
    uint32_t m_nodeId;        //!< The node being sampled
    Time m_interval;          //!< The sampling interval
    Time m_epsilon;           //!< The configured drift bound
    bool m_hasSample = false; //!< Whether m_minOffset/m_maxOffset hold a real sample yet
    Time m_minOffset;         //!< Smallest offset observed so far
    Time m_maxOffset;         //!< Largest offset observed so far
};

int
main(int argc, char* argv[])
{
    double simTime = 200.0;
    double updatePeriodS = 5.0;
    double windowSizeS = 50.0;
    double minSkew = 0.5;
    double maxSkew = 2.0;
    double epsilonMs = 200.0;
    double sampleIntervalS = 1.0;

    CommandLine cmd(__FILE__);
    cmd.AddValue("simTime", "Total simulation time, in seconds", simTime);
    cmd.AddValue("updatePeriod", "How often the skew changes (upsilon), in seconds", updatePeriodS);
    cmd.AddValue("windowSize",
                 "Lookahead window for epoch table extension, in seconds",
                 windowSizeS);
    cmd.AddValue("minSkew", "Minimum skew drawable per epoch", minSkew);
    cmd.AddValue("maxSkew", "Maximum skew drawable per epoch", maxSkew);
    cmd.AddValue("epsilon",
                 "Maximum allowed drift from Simulator::Now(), in milliseconds",
                 epsilonMs);
    cmd.AddValue("sampleInterval",
                 "How often to sample the node's offset, in seconds",
                 sampleIntervalS);
    cmd.Parse(argc, argv);

    ObjectFactory schedulerFactory;
    schedulerFactory.SetTypeId("ns3::BoundedSkewScheduler");
    schedulerFactory.Set("UpdatePeriod", TimeValue(Seconds(updatePeriodS)));
    schedulerFactory.Set("WindowSize", TimeValue(Seconds(windowSizeS)));
    schedulerFactory.Set("MinimumSkew", DoubleValue(minSkew));
    schedulerFactory.Set("MaximumSkew", DoubleValue(maxSkew));
    schedulerFactory.Set("Epsilon", TimeValue(MilliSeconds(epsilonMs)));
    Simulator::SetScheduler(schedulerFactory);

    NodeContainer nodes;
    nodes.Create(1);
    uint32_t nodeId = nodes.Get(0)->GetId();

    Simulator::ScheduleWithContext(nodeId, Seconds(simTime), &NoOp, nodeId);

    OffsetSampler sampler(nodeId, Seconds(sampleIntervalS), MilliSeconds(epsilonMs));
    Simulator::Schedule(Seconds(0.0), &OffsetSampler::Sample, &sampler);

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();
    sampler.ReportSummary();
    Simulator::Destroy();

    return 0;
}
