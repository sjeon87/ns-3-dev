/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/test.h"

#include <iomanip>
#include <sstream>
#include <string>

/**
 * @file
 * @ingroup internet-tests
 * Tests for the Ipv6RoutingTableEntry table printout.
 */

namespace ns3
{

namespace tests
{

/**
 * @ingroup internet-tests
 * Verify the fixed-width printout of Ipv6RoutingTableEntry, in "route -n" style.
 */
class Ipv6RoutingTableEntryPrintTestCase : public TestCase
{
  public:
    Ipv6RoutingTableEntryPrintTestCase();
    ~Ipv6RoutingTableEntryPrintTestCase() override = default;

  private:
    void DoRun() override;
};

Ipv6RoutingTableEntryPrintTestCase::Ipv6RoutingTableEntryPrintTestCase()
    : TestCase("Check Ipv6RoutingTableEntry printout")
{
}

void
Ipv6RoutingTableEntryPrintTestCase::DoRun()
{
    const std::string expectedHeader =
        "Destination                    Next Hop                   Flag Met Ref Use If";
    NS_TEST_ASSERT_MSG_EQ(Ipv6RoutingTableEntry::GetPrintColumnHeader(),
                          expectedHeader,
                          "Unexpected column header");

    std::ostringstream os;
    os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);

    Ipv6RoutingTableEntry networkRoute =
        Ipv6RoutingTableEntry::CreateNetworkRouteTo(Ipv6Address("2001:db8:1::"),
                                                    Ipv6Prefix(48),
                                                    Ipv6Address("fe80::1"),
                                                    1);
    networkRoute.PrintRoutingTableEntry(os, 3, "");
    NS_TEST_ASSERT_MSG_EQ(
        os.str(),
        "2001:db8:1::/48                fe80::1                    UG   3   -   -   1\n",
        "Unexpected network route printout");
    os.str("");

    Ipv6RoutingTableEntry defaultRoute =
        Ipv6RoutingTableEntry::CreateDefaultRoute(Ipv6Address("2001:db8::1"), 2);
    defaultRoute.PrintRoutingTableEntry(os, {}, "");
    NS_TEST_ASSERT_MSG_EQ(
        os.str(),
        "::/0                           2001:db8::1                UG   -   -   - "
        "  2\n",
        "Unexpected default route printout");
    os.str("");

    Ipv6RoutingTableEntry hostRoute =
        Ipv6RoutingTableEntry::CreateHostRouteTo(Ipv6Address("2001:db8:1::1"),
                                                 Ipv6Address("fe80::1"),
                                                 1);
    hostRoute.PrintRoutingTableEntry(os, {}, "eth0");
    NS_TEST_ASSERT_MSG_EQ(
        os.str(),
        "2001:db8:1::1/128              fe80::1                    UH   -   -   -   eth0\n",
        "Unexpected host route printout");
}

/**
 * @ingroup internet-tests
 * Ipv6RoutingTableEntry print test suite.
 */
class Ipv6RoutingTableEntryPrintTestSuite : public TestSuite
{
  public:
    Ipv6RoutingTableEntryPrintTestSuite();
};

Ipv6RoutingTableEntryPrintTestSuite::Ipv6RoutingTableEntryPrintTestSuite()
    : TestSuite("ipv6-routing-table-entry-print", Type::UNIT)
{
    AddTestCase(new Ipv6RoutingTableEntryPrintTestCase, TestCase::Duration::QUICK);
}

/**
 * @ingroup internet-tests
 * Ipv6RoutingTableEntryPrintTestSuite instance variable.
 */
static Ipv6RoutingTableEntryPrintTestSuite g_ipv6RoutingTableEntryPrintTestSuite;

} // namespace tests

} // namespace ns3
