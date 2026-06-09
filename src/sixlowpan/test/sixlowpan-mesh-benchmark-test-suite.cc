/*
 * Copyright (c) 2026 SRM Institute of Science and Technology, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Usham Roy <ushamroy80@gmail.com>
 */

#include "ns3/application-container.h"
#include "ns3/boolean.h"
#include "ns3/callback.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv6-address-helper.h"
#include "ns3/ipv6-address.h"
#include "ns3/ipv6-interface-container.h"
#include "ns3/lr-wpan-helper.h"
#include "ns3/lr-wpan-net-device.h"
#include "ns3/lr-wpan-phy.h"
#include "ns3/mobility-helper.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/ping-helper.h"
#include "ns3/ping.h"
#include "ns3/position-allocator.h"
#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/sixlowpan-helper.h"
#include "ns3/sixlowpan-mesh-under-routing.h"
#include "ns3/spectrum-channel.h"
#include "ns3/test.h"
#include "ns3/uinteger.h"
#include "ns3/vector.h"

using namespace ns3;

/**
 * @ingroup sixlowpan-tests
 *
 * @brief Compare flooding and Trickle suppression on a dense mesh-under network.
 *
 * This is the regular-test-program counterpart of the
 * example-sixlowpan-mesh-benchmark.cc example. Rather than freezing the exact
 * output (which is fragile to maintain), it runs the same dense scenario twice,
 * once with plain flooding and once with Trickle suppression, and checks the
 * properties that matter with a tolerance:
 *   - suppression puts substantially fewer frames on the air than flooding;
 *   - suppression does not deliver fewer packets than flooding.
 * Exact counts are deliberately not asserted, only the relations above.
 */
class MeshBenchmarkComparisonTestCase : public TestCase
{
  public:
    MeshBenchmarkComparisonTestCase()
        : TestCase("Trickle suppression cuts transmissions without losing delivery (dense)")
    {
    }

  private:
    /**
     * @brief Count one physical-layer transmission.
     * @param packet The transmitted packet (unused).
     */
    void CountTx(Ptr<const Packet> packet [[maybe_unused]])
    {
        m_phyTx++;
    }

    /**
     * @brief Capture the ping delivery figures at the end of the run.
     * @param report The ping application's report.
     */
    void CaptureReport(const Ping::PingReport& report)
    {
        m_pingSent = report.m_transmitted;
        m_pingReceived = report.m_received;
    }

    /**
     * @brief Build a dense network and run one source-to-sink ping flow.
     * @param useSuppression Use Trickle suppression instead of plain flooding.
     *
     * Fills m_phyTx, m_pingSent and m_pingReceived for the run.
     */
    void RunScenario(bool useSuppression)
    {
        m_phyTx = 0;
        m_pingSent = 0;
        m_pingReceived = 0;

        const uint32_t nNodes = 10;
        int64_t stream = 100;

        NodeContainer nodes;
        nodes.Create(nNodes);

        // Dense layout: source and sink pinned at opposite corners, the relays
        // scattered randomly but reproducibly (the streams below are fixed).
        Ptr<UniformRandomVariable> jitterX = CreateObject<UniformRandomVariable>();
        Ptr<UniformRandomVariable> jitterY = CreateObject<UniformRandomVariable>();
        jitterX->SetStream(stream++);
        jitterY->SetStream(stream++);
        Ptr<ListPositionAllocator> positions = CreateObject<ListPositionAllocator>();
        for (uint32_t i = 0; i < nNodes; ++i)
        {
            if (i == 0)
            {
                positions->Add(Vector(0.0, 0.0, 0.0));
            }
            else if (i == nNodes - 1)
            {
                positions->Add(Vector(40.0, 40.0, 0.0));
            }
            else
            {
                positions->Add(
                    Vector(jitterX->GetValue(0.0, 40.0), jitterY->GetValue(0.0, 40.0), 0.0));
            }
        }
        MobilityHelper mobility;
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobility.SetPositionAllocator(positions);
        mobility.Install(nodes);

        LrWpanHelper lrWpanHelper;
        lrWpanHelper.SetPropagationDelayModel("ns3::ConstantSpeedPropagationDelayModel");
        lrWpanHelper.AddPropagationLossModel("ns3::LogDistancePropagationLossModel");
        NetDeviceContainer lrwpanDevices = lrWpanHelper.Install(nodes);
        stream += lrWpanHelper.AssignStreams(lrwpanDevices, stream);
        stream += lrWpanHelper.GetChannel()->AssignStreams(stream);
        lrWpanHelper.CreateAssociatedPan(lrwpanDevices, 0);

        InternetStackHelper internetv6;
        internetv6.Install(nodes);
        stream += internetv6.AssignStreams(nodes, stream);

        SixLowPanHelper sixLowPanHelper;
        if (useSuppression)
        {
            sixLowPanHelper.SetMeshUnderRouting("ns3::SixLowPanTrickleSuppression");
        }
        NetDeviceContainer sixLowPanDevices = sixLowPanHelper.Install(lrwpanDevices);
        stream += sixLowPanHelper.AssignStreams(sixLowPanDevices, stream);
        for (uint32_t i = 0; i < sixLowPanDevices.GetN(); ++i)
        {
            sixLowPanDevices.Get(i)->SetAttribute("UseMeshUnder", BooleanValue(true));
            sixLowPanDevices.Get(i)->SetAttribute("MeshUnderRadius", UintegerValue(nNodes));
        }

        Ipv6AddressHelper ipv6;
        ipv6.SetBase(Ipv6Address("2001:f00d::"), Ipv6Prefix(64));
        Ipv6InterfaceContainer interfaces = ipv6.Assign(sixLowPanDevices);

        for (uint32_t i = 0; i < lrwpanDevices.GetN(); ++i)
        {
            Ptr<lrwpan::LrWpanNetDevice> dev =
                DynamicCast<lrwpan::LrWpanNetDevice>(lrwpanDevices.Get(i));
            dev->GetPhy()->TraceConnectWithoutContext(
                "PhyTxBegin",
                MakeCallback(&MeshBenchmarkComparisonTestCase::CountTx, this));
        }

        PingHelper ping(interfaces.GetAddress(nNodes - 1, 1));
        ping.SetAttribute("Count", UintegerValue(5));
        ping.SetAttribute("Interval", TimeValue(Seconds(1.0)));
        ping.SetAttribute("Size", UintegerValue(16));
        ApplicationContainer apps = ping.Install(nodes.Get(0));
        apps.Get(0)->TraceConnectWithoutContext(
            "Report",
            MakeCallback(&MeshBenchmarkComparisonTestCase::CaptureReport, this));
        apps.Start(Seconds(2.0));
        apps.Stop(Seconds(11.0));

        Simulator::Stop(Seconds(12.0));
        Simulator::Run();
        Simulator::Destroy();
    }

    void DoRun() override
    {
        RunScenario(false);
        uint64_t floodTx = m_phyTx;
        uint32_t floodReceived = m_pingReceived;
        uint32_t sent = m_pingSent;

        RunScenario(true);
        uint64_t suppressTx = m_phyTx;
        uint32_t suppressReceived = m_pingReceived;

        NS_TEST_ASSERT_MSG_GT(sent, 0, "the source should have sent pings");
        NS_TEST_ASSERT_MSG_GT(floodTx, 0, "flooding should put frames on the air");

        // Key claim, checked with a margin (not an exact count) so the test is
        // robust to small simulator changes. The observed ratio is around 0.5;
        // we require at least a 20% reduction (suppressTx < 0.8 * floodTx).
        NS_TEST_ASSERT_MSG_LT(
            suppressTx * 5,
            floodTx * 4,
            "suppression should transmit at least 20% fewer frames than flooding");

        // Suppressing redundant rebroadcasts must not cost delivery in a dense
        // network: it should be at least as good as flooding.
        NS_TEST_ASSERT_MSG_GT_OR_EQ(suppressReceived,
                                    floodReceived,
                                    "suppression delivery should be no worse than flooding");
    }

    uint64_t m_phyTx{0};        ///< PHY transmissions counted in the current run.
    uint32_t m_pingSent{0};     ///< Echo requests sent in the current run.
    uint32_t m_pingReceived{0}; ///< Echo replies received in the current run.
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief Mesh-under benchmark comparison test suite.
 */
class SixLowPanMeshBenchmarkTestSuite : public TestSuite
{
  public:
    SixLowPanMeshBenchmarkTestSuite()
        : TestSuite("sixlowpan-mesh-benchmark", Type::UNIT)
    {
        AddTestCase(new MeshBenchmarkComparisonTestCase, Duration::QUICK);
    }
};

/// Static suite registration.
static SixLowPanMeshBenchmarkTestSuite g_sixLowPanMeshBenchmarkTestSuite;
