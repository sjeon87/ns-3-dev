/*
 * Copyright (c) 2026 Somaiya Vidyavihar University, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manmita Das <das.manmita12@gmail.com>
 */

#ifndef ETHERNET_HELPER_H
#define ETHERNET_HELPER_H

#include "ns3/attribute.h"
#include "ns3/ethernet-channel.h"
#include "ns3/ethernet-net-device.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/queue.h"
#include "ns3/trace-helper.h"

#include <string>

namespace ns3
{

class NetDevice;
class Node;

namespace ethernet
{

/**
 * @ingroup ethernet
 * @brief Build a set of EthernetNetDevice objects.
 *
 * The helper creates EthernetNetDevice instances, configures their queue and
 * channel attributes, and attaches them to an EthernetChannel.
 */
class EthernetHelper : public PcapHelperForDevice, public AsciiTraceHelperForDevice
{
  public:
    /**
     * Construct an EthernetHelper.
     */
    EthernetHelper();

    ~EthernetHelper() override
    {
    }

    /**
     * Set the type of queue to create and associate to each
     * EthernetNetDevice created through EthernetHelper::Install.
     *
     * The transmit and the receive queue of every device are both created from
     * this type, and take precedence over the TxQueue and RxQueue attributes of
     * the device.
     *
     * @tparam Ts \deduced Argument types
     * @param type the type of queue
     * @param [in] args Name and AttributeValue pairs to set.
     */
    template <typename... Ts>
    void SetQueue(std::string type, Ts&&... args);

    /**
     * Set an attribute value to be propagated to each EthernetNetDevice
     * created by the helper.
     *
     * @param name the name of the attribute to set
     * @param value the value of the attribute to set
     */
    void SetDeviceAttribute(std::string name, const AttributeValue& value);

    /**
     * Set an attribute value to be propagated to each EthernetChannel created
     * by the helper.
     *
     * @param name the name of the attribute to set
     * @param value the value of the attribute to set
     */
    void SetChannelAttribute(std::string name, const AttributeValue& value);

    /**
     * Create two EthernetNetDevice objects and attach them to the nodes in the
     * container.
     *
     * @param c a set of nodes
     * @return a NetDeviceContainer for nodes
     */
    NetDeviceContainer Install(NodeContainer c) const;

    /**
     * Create two EthernetNetDevice objects and attach them to the provided
     * nodes.
     *
     * @param N1 first node
     * @param N2 second node
     * @return a NetDeviceContainer for nodes
     */
    NetDeviceContainer Install(Ptr<Node> N1, Ptr<Node> N2) const;

  private:
    /**
     * @brief Create an EthernetNetDevice, install it on a node and attach it to a channel.
     *
     * @param node The node to install the device on.
     * @param channel The channel to attach the device to.
     * @return The newly created device.
     */
    Ptr<EthernetNetDevice> InstallPriv(Ptr<Node> node, Ptr<EthernetChannel> channel) const;

    /**
     * @brief Enable pcap output on the indicated net device.
     *
     * NetDevice-specific implementation mechanism for hooking the trace and
     * writing to the trace file.
     *
     * @param prefix Filename prefix to use for pcap files.
     * @param nd Net device for which you want to enable tracing.
     * @param promiscuous If true capture all possible packets available at the device.
     * @param explicitFilename Treat the prefix as an explicit filename if true
     */
    void EnablePcapInternal(std::string prefix,
                            Ptr<NetDevice> nd,
                            bool promiscuous,
                            bool explicitFilename) override;

    /**
     * @brief Enable ascii trace output on the indicated net device.
     *
     * NetDevice-specific implementation mechanism for hooking the trace and
     * writing to the trace file.
     *
     * @param stream The output stream object to use when logging ascii traces.
     * @param prefix Filename prefix to use for ascii trace files.
     * @param nd Net device for which you want to enable tracing.
     * @param explicitFilename Treat the prefix as an explicit filename if true
     */
    void EnableAsciiInternal(Ptr<OutputStreamWrapper> stream,
                             std::string prefix,
                             Ptr<NetDevice> nd,
                             bool explicitFilename) override;

    ObjectFactory m_queueFactory;   //!< Queue Factory
    ObjectFactory m_channelFactory; //!< Channel Factory
    ObjectFactory m_deviceFactory;  //!< Device Factory
};

/***************************************************************
 *  Implementation of the templates declared above.
 ***************************************************************/

template <typename... Ts>
void
EthernetHelper::SetQueue(std::string type, Ts&&... args)
{
    QueueBase::AppendItemTypeIfNotPresent(type, "Packet");

    m_queueFactory.SetTypeId(type);
    m_queueFactory.Set(std::forward<Ts>(args)...);
}

} // namespace ethernet
} // namespace ns3

#endif /* ETHERNET_HELPER_H */
