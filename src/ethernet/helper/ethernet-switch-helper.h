/*
 * Copyright (c) 2026 Somaiya Vidyavihar University, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manmita Das <das.manmita12@gmail.com>
 */

#ifndef ETHERNET_SWITCH_HELPER_H
#define ETHERNET_SWITCH_HELPER_H

#include "ns3/ethernet-net-device.h"
#include "ns3/ethernet-switch.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"

#include <string>

namespace ns3
{

class Node;

namespace ethernet
{

/**
 * @ingroup ethernet
 *
 * @brief Helper to create and install Ethernet switches.
 *
 * This helper creates EthernetSwitch objects, creates EthernetNetDevice
 * devices as the switch's physical ports, and attaches them to the switch.
 */
class EthernetSwitchHelper
{
  public:
    EthernetSwitchHelper();

    /**
     * Set an attribute value to be propagated to each EthernetSwitch created.
     *
     * @param name attribute name
     * @param value attribute value
     */
    void SetSwitchAttribute(std::string name, const AttributeValue& value);

    /**
     * Set an attribute value to be propagated to each EthernetNetDevice port
     * created by this helper.
     *
     * @param name attribute name
     * @param value attribute value
     */
    void SetPortAttribute(std::string name, const AttributeValue& value);

    /**
     * @brief Install EthernetNetDevice on each host node.
     * @param nodes Container of nodes on which to install switches.
     * @returns Container of NetDevices installed on the nodes.
     */
    NetDeviceContainer Install(NodeContainer nodes);

    /**
     * @brief Create and install an EthernetSwitch on the given node.
     * @param node Switch node.
     * @param devices Container of NetDevices to attach to the switch.
     */
    void Install(Ptr<Node> node, NetDeviceContainer devices);

  private:
    ObjectFactory m_switchFactory;    //!< Switch Factory
    ObjectFactory m_portFactory;      //!< Switch port device factory
    ObjectFactory m_deviceFactory;    //!< EthernetNetDevice Factory
    ObjectFactory m_schedulerFactory; //!< Scheduler Factory
};

} // namespace ethernet
} // namespace ns3

#endif /* ETHERNET_SWITCH_HELPER_H */
