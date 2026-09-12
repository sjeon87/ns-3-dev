/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/test.h"

#include <iomanip>
#include <sstream>
#include <string>

/**
 * @file
 * @ingroup internet-tests
 * Tests for the Ipv4RoutingTableEntry table printout.
 */

namespace ns3
{

namespace tests
{

/**
 * @ingroup internet-tests
 * Verify the fixed-width printout of Ipv4RoutingTableEntry, in "route -n" style.
 */
class Ipv4RoutingTableEntryPrintTestCase : public TestCase
{
  public:
    Ipv4RoutingTableEntryPrintTestCase();
    ~Ipv4RoutingTableEntryPrintTestCase() override = default;

  private:
    void DoRun() override;
};

Ipv4RoutingTableEntryPrintTestCase::Ipv4RoutingTableEntryPrintTestCase()
    : TestCase("Check Ipv4RoutingTableEntry printout")
{
}

void
Ipv4RoutingTableEntryPrintTestCase::DoRun()
{
    const std::string expectedHeader =
        "Destination     Gateway         Genmask         Flags Metric Ref    Use Iface";
    NS_TEST_ASSERT_MSG_EQ(Ipv4RoutingTableEntry::GetPrintColumnHeader(),
                          expectedHeader,
                          "Unexpected column header");

    std::ostringstream os;
    os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);

    Ipv4RoutingTableEntry networkRoute =
        Ipv4RoutingTableEntry::CreateNetworkRouteTo(Ipv4Address("10.1.2.0"),
                                                    Ipv4Mask("255.255.255.0"),
                                                    Ipv4Address("10.1.1.1"),
                                                    3);
    networkRoute.PrintRoutingTableEntry(os, 7, "");
    NS_TEST_ASSERT_MSG_EQ(os.str(),
                          "10.1.2.0        10.1.1.1        255.255.255.0   UGS   7      -      "
                          "-   3\n",
                          "Unexpected network route printout");
    os.str("");

    Ipv4RoutingTableEntry defaultRoute =
        Ipv4RoutingTableEntry::CreateDefaultRoute(Ipv4Address("10.1.1.254"), 2);
    defaultRoute.PrintRoutingTableEntry(os, {}, "");
    NS_TEST_ASSERT_MSG_EQ(os.str(),
                          "0.0.0.0         10.1.1.254      0.0.0.0         UGS   -      -      "
                          "-   2\n",
                          "Unexpected default route printout");
    os.str("");

    Ipv4RoutingTableEntry hostRoute =
        Ipv4RoutingTableEntry::CreateHostRouteTo(Ipv4Address("10.1.3.9"),
                                                 Ipv4Address("10.1.3.1"),
                                                 1);
    hostRoute.PrintRoutingTableEntry(os, 3, "eth0");
    NS_TEST_ASSERT_MSG_EQ(os.str(),
                          "10.1.3.9        10.1.3.1        255.255.255.255 UHS   3      -      "
                          "-   eth0\n",
                          "Unexpected host route printout");
}

/**
 * @ingroup internet-tests
 * Ipv4RoutingTableEntry print test suite.
 */
class Ipv4RoutingTableEntryPrintTestSuite : public TestSuite
{
  public:
    Ipv4RoutingTableEntryPrintTestSuite();
};

Ipv4RoutingTableEntryPrintTestSuite::Ipv4RoutingTableEntryPrintTestSuite()
    : TestSuite("ipv4-routing-table-entry-print", Type::UNIT)
{
    AddTestCase(new Ipv4RoutingTableEntryPrintTestCase, TestCase::Duration::QUICK);
}

/**
 * @ingroup internet-tests
 * Ipv4RoutingTableEntryPrintTestSuite instance variable.
 */
static Ipv4RoutingTableEntryPrintTestSuite g_ipv4RoutingTableEntryPrintTestSuite;

} // namespace tests

} // namespace ns3
