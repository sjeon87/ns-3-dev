/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "ns3/epoch-table.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/test.h"

namespace ns3
{

/**
 * @brief Verifies AddEpoch updates the node's max simulator/node times.
 */
class EpochTableAddEpochTestCase : public TestCase
{
  public:
    EpochTableAddEpochTestCase();
    void DoRun() override;
};

EpochTableAddEpochTestCase::EpochTableAddEpochTestCase()
    : TestCase("EpochTableAddEpochTest")
{
}

void
EpochTableAddEpochTestCase::DoRun()
{
    Ptr<EpochTable> et = CreateObject<EpochTable>();
    uint32_t nodeId = 1;

    EpochTable::Epoch e1;
    e1.simulatorStartTime = Seconds(0.0);
    e1.simulatorEndTime = Seconds(10.0);
    e1.nodeStartTime = Seconds(0.0);
    e1.nodeEndTime = Seconds(20.0);
    e1.skew = 2.0;
    et->AddEpoch(nodeId, e1);

    NS_TEST_ASSERT_MSG_EQ(et->HasNode(nodeId), true, "Node should exist after AddEpoch");
    NS_TEST_ASSERT_MSG_EQ(et->GetMaxSimulatorTime(nodeId),
                          Seconds(10.0),
                          "MaxSimulatorTime should be 10.0s after first epoch");
    NS_TEST_ASSERT_MSG_EQ(et->GetMaxNodeTime(nodeId),
                          Seconds(20.0),
                          "MaxNodeTime should be 20.0s after first epoch");

    EpochTable::Epoch e2;
    e2.simulatorStartTime = Seconds(10.0);
    e2.simulatorEndTime = Seconds(20.0);
    e2.nodeStartTime = Seconds(20.0);
    e2.nodeEndTime = Seconds(25.0);
    e2.skew = 0.5;
    et->AddEpoch(nodeId, e2);

    NS_TEST_ASSERT_MSG_EQ(et->GetMaxSimulatorTime(nodeId),
                          Seconds(20.0),
                          "MaxSimulatorTime should be 20.0s after second epoch");
    NS_TEST_ASSERT_MSG_EQ(et->GetMaxNodeTime(nodeId),
                          Seconds(25.0),
                          "MaxNodeTime should be 25.0s after second epoch");
}

/**
 * @brief Verifies InsertEpoch truncates the active epoch and inserts a new one.
 */
class EpochTableInsertEpochTestCase : public TestCase
{
  public:
    EpochTableInsertEpochTestCase();
    void DoRun() override;
};

EpochTableInsertEpochTestCase::EpochTableInsertEpochTestCase()
    : TestCase("EpochTableInsertEpochTest")
{
}

void
EpochTableInsertEpochTestCase::DoRun()
{
    Ptr<EpochTable> et = CreateObject<EpochTable>();
    uint32_t nodeId = 1;

    EpochTable::Epoch e1;
    e1.simulatorStartTime = Seconds(0.0);
    e1.simulatorEndTime = Seconds(10.0);
    e1.nodeStartTime = Seconds(0.0);
    e1.nodeEndTime = Seconds(20.0);
    e1.skew = 2.0;
    et->AddEpoch(nodeId, e1);

    EpochTable::Epoch e2;
    e2.simulatorStartTime = Seconds(10.0);
    e2.simulatorEndTime = Seconds(20.0);
    e2.nodeStartTime = Seconds(20.0);
    e2.nodeEndTime = Seconds(30.0);
    e2.skew = 1.0;
    et->AddEpoch(nodeId, e2);

    EpochTable::Epoch e3;
    e3.simulatorStartTime = Seconds(20.0);
    e3.simulatorEndTime = Seconds(30.0);
    e3.nodeStartTime = Seconds(30.0);
    e3.nodeEndTime = Seconds(40.0);
    e3.skew = 1.0;
    et->AddEpoch(nodeId, e3);

    NS_TEST_ASSERT_MSG_EQ(et->GetMaxSimulatorTime(nodeId),
                          Seconds(30.0),
                          "Pre-insert max sim time should be 30.0s");

    et->InsertEpoch(nodeId, Seconds(15.0), Seconds(25.0), Seconds(25.0), Seconds(30.0), 0.5);

    NS_TEST_ASSERT_MSG_EQ(et->GetNodeTimeFromSimulatorTime(nodeId, Seconds(12.0)),
                          Seconds(22.0),
                          "Time inside the truncated epoch should evaluate correctly");

    NS_TEST_ASSERT_MSG_EQ(et->GetNodeTimeFromSimulatorTime(nodeId, Seconds(19.0)),
                          Seconds(27.0),
                          "Sim 19.0s with 0.5x skew in new epoch should be Node 27.0s");

    NS_TEST_ASSERT_MSG_EQ(et->GetSimulatorTimeFromNodeTime(nodeId, Seconds(27.0)),
                          Seconds(19.0),
                          "Node 27.0s in new epoch should map back to Sim 19.0s");

    NS_TEST_ASSERT_MSG_EQ(et->GetMaxSimulatorTime(nodeId),
                          Seconds(25.0),
                          "Max sim time should reflect the end of the newly inserted epoch");

    NS_TEST_ASSERT_MSG_EQ(et->GetMaxNodeTime(nodeId),
                          Seconds(30.0),
                          "Max node time should reflect the end of the newly inserted epoch");
}

/**
 * @brief Verifies sim/node time conversion across epochs, including unknown nodes.
 */
class EpochTableTimeConversionTestCase : public TestCase
{
  public:
    EpochTableTimeConversionTestCase();
    void DoRun() override;
};

EpochTableTimeConversionTestCase::EpochTableTimeConversionTestCase()
    : TestCase("EpochTableTimeConversionTest")
{
}

void
EpochTableTimeConversionTestCase::DoRun()
{
    Ptr<EpochTable> et = CreateObject<EpochTable>();
    uint32_t nodeId = 1;

    EpochTable::Epoch e1;
    e1.simulatorStartTime = Seconds(0.0);
    e1.simulatorEndTime = Seconds(10.0);
    e1.nodeStartTime = Seconds(0.0);
    e1.nodeEndTime = Seconds(20.0);
    e1.skew = 2.0;
    et->AddEpoch(nodeId, e1);

    NS_TEST_ASSERT_MSG_EQ(et->GetNodeTimeFromSimulatorTime(nodeId, Seconds(5.0)),
                          Seconds(10.0),
                          "Sim 5.0s with 2.0x skew should give Node 10.0s");
    NS_TEST_ASSERT_MSG_EQ(et->GetSimulatorTimeFromNodeTime(nodeId, Seconds(10.0)),
                          Seconds(5.0),
                          "Node 10.0s with 2.0x skew should give Sim 5.0s");

    EpochTable::Epoch e2;
    e2.simulatorStartTime = Seconds(10.0);
    e2.simulatorEndTime = Seconds(20.0);
    e2.nodeStartTime = Seconds(20.0);
    e2.nodeEndTime = Seconds(25.0);
    e2.skew = 0.5;
    et->AddEpoch(nodeId, e2);

    NS_TEST_ASSERT_MSG_EQ(et->GetNodeTimeFromSimulatorTime(nodeId, Seconds(14.0)),
                          Seconds(22.0),
                          "Sim 14.0s with 0.5x skew should give Node 22.0s");
    NS_TEST_ASSERT_MSG_EQ(et->GetSimulatorTimeFromNodeTime(nodeId, Seconds(22.0)),
                          Seconds(14.0),
                          "Node 22.0s with 0.5x skew should give Sim 14.0s");

    NS_TEST_ASSERT_MSG_EQ(et->GetNodeTimeFromSimulatorTime(99, Seconds(7.0)),
                          Seconds(7.0),
                          "Unknown node should return sim time unchanged");
    NS_TEST_ASSERT_MSG_EQ(et->GetSimulatorTimeFromNodeTime(99, Seconds(7.0)),
                          Seconds(7.0),
                          "Unknown node should return node time unchanged");
}

/**
 * @brief Verifies LocalTimeBinarySearch and GlobalTimeBinarySearch locate the right epoch.
 */
class EpochTableBinarySearchTestCase : public TestCase
{
  public:
    EpochTableBinarySearchTestCase();
    void DoRun() override;
};

EpochTableBinarySearchTestCase::EpochTableBinarySearchTestCase()
    : TestCase("EpochTableBinarySearchTest")
{
}

void
EpochTableBinarySearchTestCase::DoRun()
{
    Ptr<EpochTable> et = CreateObject<EpochTable>();
    uint32_t nodeId = 1;

    EpochTable::Epoch e1;
    e1.simulatorStartTime = Seconds(0.0);
    e1.simulatorEndTime = Seconds(10.0);
    e1.nodeStartTime = Seconds(0.0);
    e1.nodeEndTime = Seconds(10.0);
    e1.skew = 1.0;
    et->AddEpoch(nodeId, e1);

    EpochTable::Epoch e2;
    e2.simulatorStartTime = Seconds(10.0);
    e2.simulatorEndTime = Seconds(20.0);
    e2.nodeStartTime = Seconds(10.0);
    e2.nodeEndTime = Seconds(20.0);
    e2.skew = 1.0;
    et->AddEpoch(nodeId, e2);

    EpochTable::Epoch e3;
    e3.simulatorStartTime = Seconds(20.0);
    e3.simulatorEndTime = Seconds(30.0);
    e3.nodeStartTime = Seconds(20.0);
    e3.nodeEndTime = Seconds(30.0);
    e3.skew = 1.0;
    et->AddEpoch(nodeId, e3);

    const EpochTable::Epoch& foundLocal1 = et->LocalTimeBinarySearch(nodeId, Seconds(5.0));
    NS_TEST_ASSERT_MSG_EQ(foundLocal1.nodeStartTime,
                          Seconds(0.0),
                          "Local search: t=5.0s should land in first epoch");

    const EpochTable::Epoch& foundLocal3 = et->LocalTimeBinarySearch(nodeId, Seconds(25.0));
    NS_TEST_ASSERT_MSG_EQ(foundLocal3.nodeStartTime,
                          Seconds(20.0),
                          "Local search: t=25.0s should land in third epoch");

    const EpochTable::Epoch& foundGlobal2 = et->GlobalTimeBinarySearch(nodeId, Seconds(15.0));
    NS_TEST_ASSERT_MSG_EQ(foundGlobal2.simulatorStartTime,
                          Seconds(10.0),
                          "Global search: t=15.0s should land in second epoch");

    const EpochTable::Epoch& foundGlobalBoundary =
        et->GlobalTimeBinarySearch(nodeId, Seconds(20.0));
    NS_TEST_ASSERT_MSG_EQ(foundGlobalBoundary.simulatorStartTime,
                          Seconds(20.0),
                          "Global search: t=20.0s should land in third epoch (inclusive start)");
}

/**
 * @brief Verifies PruneEpochTable removes old epochs while keeping later ones resolvable.
 */
class EpochTablePruneTestCase : public TestCase
{
  public:
    EpochTablePruneTestCase();
    void DoRun() override;
};

EpochTablePruneTestCase::EpochTablePruneTestCase()
    : TestCase("EpochTablePruneTest")
{
}

void
EpochTablePruneTestCase::DoRun()
{
    Ptr<EpochTable> et = CreateObject<EpochTable>();
    uint32_t nodeId = 1;

    EpochTable::Epoch e1;
    e1.simulatorStartTime = Seconds(0.0);
    e1.simulatorEndTime = Seconds(10.0);
    e1.nodeStartTime = Seconds(0.0);
    e1.nodeEndTime = Seconds(10.0);
    e1.skew = 1.0;
    et->AddEpoch(nodeId, e1);

    EpochTable::Epoch e2;
    e2.simulatorStartTime = Seconds(10.0);
    e2.simulatorEndTime = Seconds(20.0);
    e2.nodeStartTime = Seconds(10.0);
    e2.nodeEndTime = Seconds(20.0);
    e2.skew = 1.0;
    et->AddEpoch(nodeId, e2);

    EpochTable::Epoch e3;
    e3.simulatorStartTime = Seconds(20.0);
    e3.simulatorEndTime = Seconds(30.0);
    e3.nodeStartTime = Seconds(20.0);
    e3.nodeEndTime = Seconds(30.0);
    e3.skew = 1.0;
    et->AddEpoch(nodeId, e3);

    et->PruneEpochTable(Seconds(15.0));

    NS_TEST_ASSERT_MSG_EQ(et->GetNodeTimeFromSimulatorTime(nodeId, Seconds(25.0)),
                          Seconds(25.0),
                          "Post-prune: sim 25.0s should still resolve correctly");

    NS_TEST_ASSERT_MSG_EQ(et->GetMaxSimulatorTime(nodeId),
                          Seconds(30.0),
                          "Post-prune: max simulator time should still be 30.0s");
}

/**
 * @brief Verifies HasNode reflects whether a node has any epochs.
 */
class EpochTableHasNodeTestCase : public TestCase
{
  public:
    EpochTableHasNodeTestCase();
    void DoRun() override;
};

EpochTableHasNodeTestCase::EpochTableHasNodeTestCase()
    : TestCase("EpochTableHasNodeTest")
{
}

void
EpochTableHasNodeTestCase::DoRun()
{
    Ptr<EpochTable> et = CreateObject<EpochTable>();

    NS_TEST_ASSERT_MSG_EQ(et->HasNode(1), false, "Node 1 should not exist before any AddEpoch");

    EpochTable::Epoch e1;
    e1.simulatorStartTime = Seconds(0.0);
    e1.simulatorEndTime = Seconds(10.0);
    e1.nodeStartTime = Seconds(0.0);
    e1.nodeEndTime = Seconds(10.0);
    e1.skew = 1.0;
    et->AddEpoch(1, e1);

    NS_TEST_ASSERT_MSG_EQ(et->HasNode(1), true, "Node 1 should exist after AddEpoch");
    NS_TEST_ASSERT_MSG_EQ(et->HasNode(2), false, "Node 2 should not exist");
}

/**
 * @brief Test suite for the EpochTable class.
 */
class EpochTableTestSuite : public TestSuite
{
  public:
    EpochTableTestSuite();
};

EpochTableTestSuite::EpochTableTestSuite()
    : TestSuite("epoch-table", Type::UNIT)
{
    AddTestCase(new EpochTableHasNodeTestCase, TestCase::Duration::QUICK);
    AddTestCase(new EpochTableAddEpochTestCase, TestCase::Duration::QUICK);
    AddTestCase(new EpochTableInsertEpochTestCase, TestCase::Duration::QUICK);
    AddTestCase(new EpochTableTimeConversionTestCase, TestCase::Duration::QUICK);
    AddTestCase(new EpochTableBinarySearchTestCase, TestCase::Duration::QUICK);
    AddTestCase(new EpochTablePruneTestCase, TestCase::Duration::QUICK);
}

static EpochTableTestSuite g_epochTableTestSuite; //!< Static variable for test initialization

} // namespace ns3
