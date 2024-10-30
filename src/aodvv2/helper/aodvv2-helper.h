/*
 * Copyright (c) 2024 University of Florence
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Francesco Todino <todinofrancesco97@gmail.com>
 *          Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#ifndef AODVV2_HELPER_H
#define AODVV2_HELPER_H

#include "ns3/ipv4-routing-helper.h"
#include "ns3/ipv6-routing-helper.h"
#include "ns3/node-container.h"
#include "ns3/node.h"
#include "ns3/object-factory.h"

namespace ns3
{
/**
 * \ingroup aodvv2
 * \brief Helper class that adds AODVv2 routing to nodes.
 */
template <typename T>
class Aodvv2Helper : public std::enable_if_t<std::is_same_v<Ipv4RoutingHelper, T> ||
                                                 std::is_same_v<Ipv6RoutingHelper, T>,
                                             T>
{
    /// Alias for determining whether the parent is Ipv4RoutingHelper or Ipv6RoutingHelper
    static constexpr bool IsIpv4 = std::is_same_v<Ipv4RoutingHelper, T>;
    /// Alias for Ipv4Address and Ipv6Address classes
    using IpAddress = typename std::conditional_t<IsIpv4, Ipv4Address, Ipv6Address>;
    /// Alias for Ipv4RoutingProtocol and Ipv6RoutingProtocol classes
    using IpRoutingProtocol =
        typename std::conditional_t<IsIpv4, Ipv4RoutingProtocol, Ipv6RoutingProtocol>;
    /// Alias for Ipv4ListRouting and Ipv6ListRouting classes
    using IpListRouting = typename std::conditional_t<IsIpv4, Ipv4ListRouting, Ipv6ListRouting>;
    /// Alias for Ipv4 and Ipv6 classes
    using Ip = typename std::conditional_t<IsIpv4, Ipv4, Ipv6>;

  public:
    Aodvv2Helper();

    /**
     * \returns pointer to clone of this Aodvv2Helper
     *
     * \internal
     * This method is mainly for internal use by the other helpers;
     * clients are expected to free the dynamic memory allocated by this method
     */
    Aodvv2Helper* Copy() const override;

    /**
     * \param node the node on which the routing protocol will run
     * \returns a newly-created routing protocol
     *
     * This method will be called by ns3::InternetStackHelper::Install
     */
    Ptr<IpRoutingProtocol> Create(Ptr<Node> node) const override;
    /**
     * \param name the name of the attribute to set
     * \param value the value of the attribute to set.
     *
     * This method controls the attributes of ns3::aodvv2::Aodvv2RoutingProtocol
     */
    void Set(std::string name, const AttributeValue& value);
    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model.  Return the number of streams (possibly zero) that
     * have been assigned.  The Install() method of the InternetStackHelper
     * should have previously been called by the user.
     *
     * \param stream first stream index to use
     * \param c NodeContainer of the set of nodes for which AODVv2
     *          should be modified to use a fixed stream
     * \return the number of stream indices assigned by this helper
     */
    int64_t AssignStreams(NodeContainer c, int64_t stream);

    /**
     * \brief prints the routing path for a source and destination at a particular time.
     * If the routing path does not exist, it prints that the path does not exist between
     * the nodes in the ostream.
     * \param printTime the time at which the routing path is supposed to be printed.
     * \param source the source node pointer to start traversing
     * \param dest the IP destination address
     * \param stream the output stream object to use
     * \param unit the time unit to be used in the report
     *
     * This method calls the PrintRoutingPath() method of the
     * Aodvv2RoutingProtocol for the source and destination to provide
     * the routing path at the specified time.
     */
    void PrintRoutingPathAt(Time printTime,
                            Ptr<Node> source,
                            IpAddress dest,
                            Ptr<OutputStreamWrapper> stream,
                            Time::Unit unit = Time::S);

  private:
    /** the factory to create AODVv2 routing object */
    ObjectFactory m_agentFactory;

    /**
     * \brief prints the routing path for the source and destination. If the routing path
     * does not exist, it prints that the path does not exist between the nodes in the ostream.
     * \param source the source node pointer to start traversing
     * \param dest the IP destination address
     * \param stream the output stream object to use
     * \param unit the time unit to be used in the report
     *
     * This method calls the PrintRoutingPath() method of the
     * Aodvv2RoutingProtocol for the source and destination to provide
     * the routing path.
     */
    static void PrintRoute(Ptr<Node> source,
                           IpAddress dest,
                           Ptr<OutputStreamWrapper> stream,
                           Time::Unit unit = Time::S);
};

} // namespace ns3

#endif /* AODVV2_HELPER_H */
