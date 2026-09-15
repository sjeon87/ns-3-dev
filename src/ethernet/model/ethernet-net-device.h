/*
 * Copyright (c) 2026 Somaiya Vidyavihar University, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manmita Das <das.manmita12@gmail.com>
 */

#ifndef ETHERNET_NET_DEVICE_H
#define ETHERNET_NET_DEVICE_H

#include "ns3/address.h"
#include "ns3/callback.h"
#include "ns3/data-rate.h"
#include "ns3/net-device.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/queue-fwd.h"
#include "ns3/queue.h"
#include "ns3/simple-ref-count.h"
#include "ns3/traced-callback.h"

namespace ns3
{

namespace ethernet
{

class EthernetChannel;
class EthernetMac;
class EthernetPhy;

/**
 * Supported Ethernet link types.
 */
enum class EthernetLinkType : uint8_t
{
    BASE10_T = 0,
    BASE100_TX,
    BASE1000_T,
    G10_T
};

/**
 * @brief Get the nominal data rate of an Ethernet link type.
 * @param type The Ethernet link type.
 * @return The data rate defined for that link type by IEEE 802.3.
 */
DataRate EthernetLinkTypeToDataRate(EthernetLinkType type);

/**
 * @brief Stream insertion operator for EthernetLinkType.
 * @param os The output stream.
 * @param type The link type to print.
 * @return The output stream.
 */
std::ostream& operator<<(std::ostream& os, EthernetLinkType type);

/**
 * @ingroup ethernet
 * @class EthernetNetDevice
 * @brief EthernetNetDevice for ns-3 Ethernet NetDevice.
 *
 * The EthernetNetDevice implements the ns-3 NetDevice interface and provides
 * the connection between a node's protocol stack and the underlying Ethernet
 * MAC and PHY layers. It is responsible for transmitting and receiving packets,
 * managing device configuration, and interfacing with upper-layer protocols.
 */
class EthernetNetDevice : public NetDevice
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    EthernetNetDevice();

    ~EthernetNetDevice() override;

    // Delete copy constructor and assignment operator to avoid misuse
    EthernetNetDevice(const EthernetNetDevice&) = delete;
    EthernetNetDevice& operator=(const EthernetNetDevice&) = delete;

    /**
     * @brief attach the mac sublayer object to the Net Device
     * @param mac MAC layer object
     */
    void SetMac(Ptr<EthernetMac> mac);

    /**
     * @brief attach the phy sublayer object to the Net Device
     * @param phy PHY layer object
     */
    void SetPhy(Ptr<EthernetPhy> phy);

    /**
     * @brief Attach the EthernetNetDevice to a EthernetChannel.
     * @param ch a pointer to the channel to which this object is being attached.
     */
    void Attach(Ptr<EthernetChannel> ch);

    /**
     * @brief Set the fastest Ethernet link type this device is able to run at.
     *
     * The link type that is actually used is the slowest of the maximum
     * supported link types of the two devices sharing the channel, and is
     * recomputed whenever this value changes or the set of attached devices
     * changes.
     *
     * @param type The maximum supported Ethernet link type.
     */
    void SetMaxSupportedLinkType(EthernetLinkType type);

    /**
     * @brief Get the fastest Ethernet link type this device is able to run at.
     * @return The maximum supported Ethernet link type.
     */
    EthernetLinkType GetMaxSupportedLinkType() const;

    /**
     * @brief Get the Ethernet link type the device is currently running at.
     *
     * While no peer is attached to the channel, this is the maximum supported
     * link type of this device.
     *
     * @return The Ethernet link type in use.
     */
    EthernetLinkType GetActualLinkType() const;

    /**
     * @brief Set the Ethernet link type the device runs at.
     *
     * This is called by EthernetChannel when the link type is settled between
     * the devices sharing the channel. Models and users should set the maximum
     * supported link type instead.
     *
     * @param type The Ethernet link type to run at.
     */
    void SetActualLinkType(EthernetLinkType type);

    /**
     * @brief Get the data rate corresponding to the link type in use.
     * @return The data rate used by this device.
     */
    DataRate GetDataRate() const;

    /**
     * @brief Get the MAC layer object.
     * @return Pointer to the MAC layer object.
     */
    Ptr<EthernetMac> GetMac() const;

    /**
     * @brief Get the PHY layer object.
     * @return Pointer to the PHY layer object.
     */
    Ptr<EthernetPhy> GetPhy() const;

    //
    // The following methods are inherited from NetDevice base class.
    //
    Ptr<Channel> GetChannel() const override;
    void SetIfIndex(const uint32_t index) override;
    uint32_t GetIfIndex() const override;
    void SetAddress(Address address) override;
    Address GetAddress() const override;
    bool SetMtu(const uint16_t mtu) override;
    uint16_t GetMtu() const override;
    uint16_t GetPaddingThreshold() const override;
    bool IsLinkUp() const override;
    void AddLinkChangeCallback(Callback<void> callback) override;
    bool IsBroadcast() const override;
    Address GetBroadcast() const override;
    bool IsMulticast() const override;
    bool IsBridge() const override;
    bool IsPointToPoint() const override;
    Address GetMulticast(Ipv4Address multicastGroup) const override;
    Address GetMulticast(Ipv6Address addr) const override;
    bool Send(Ptr<Packet> packet, const Address& destination, uint16_t protocolNumber) override;
    bool SendFrom(Ptr<Packet> packet,
                  const Address& source,
                  const Address& destination,
                  uint16_t protocolNumber) override;
    Ptr<Node> GetNode() const override;
    void SetNode(Ptr<Node> node) override;
    bool NeedsArp() const override;
    void SetReceiveCallback(NetDevice::ReceiveCallback cb) override;
    void SetPromiscReceiveCallback(PromiscReceiveCallback cb) override;
    bool SupportsSendFrom() const override;

    /**
     * @brief Check if the receive side of the network device is enabled.
     * @returns True if the receiver side is enabled, otherwise false.
     */
    bool IsReceiveEnabled() const;

    /**
     * @brief Notify the device that the link is up.
     */
    void LinkUp();

    /**
     * @brief Notify the device that the link is down.
     */
    void LinkDown();

    /**
     * @brief Handle a receive indication from the MAC that a packet is available
     * in the receive queue.
     */
    void RxIndication();

  protected:
    void DoDispose() override;
    void DoInitialize() override;

  private:
    /**
     * @brief Pointer to the node to which the device is attached.
     */
    Ptr<Node> m_node;

    /**
     * @brief Pointer to the MAC layer.
     */
    Ptr<EthernetMac> m_mac;

    /**
     * @brief Pointer to the PHY layer.
     */
    Ptr<EthernetPhy> m_phy;

    /**
     * @brief Pointer to the channel.
     */
    Ptr<EthernetChannel> m_channel;

    /**
     * @brief Flag indicating if the send side is enabled.
     */
    bool m_sendEnable;

    /**
     * @brief Flag indicating if the receive side is enabled.
     */
    bool m_receiveEnable;

    /**
     * @brief Flag indicating if the link is up.
     */
    bool m_linkUp;

    /**
     * @brief The interface index that has been assigned to this network device.
     */
    uint32_t m_ifIndex;

    /**
     * @brief The ID assigned to this device by the channel it is attached to.
     */
    uint32_t m_deviceId;

    /**
     * @brief The fastest Ethernet link type this device is able to run at.
     */
    EthernetLinkType m_maxLinkType;

    /**
     * @brief The Ethernet link type the device is currently running at.
     */
    EthernetLinkType m_actualLinkType;

    /**
     * @brief Callback to be used to notify higher layers when a packet has been received.
     */
    NetDevice::ReceiveCallback m_rxCallback;

    /**
     * @brief Callback to be used to notify higher layers when a packet is received in promiscuous
     * mode.
     */
    NetDevice::PromiscReceiveCallback m_promiscRxCallback;

    /**
     * @brief Callbacks to fire when the link state changes.
     */
    TracedCallback<> m_linkChangeCallbacks;
};

} // namespace ethernet
} // namespace ns3

#endif /* ETHERNET_NET_DEVICE_H */
