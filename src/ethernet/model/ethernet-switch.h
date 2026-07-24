/*
 * Copyright (c) 2026 Somaiya Vidyavihar University, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manmita Das <das.manmita12@gmail.com>
 */

#ifndef ETHERNET_SWITCH_H
#define ETHERNET_SWITCH_H

#include "ethernet-net-device.h"

#include "ns3/callback.h"
#include "ns3/ethernet-header.h"
#include "ns3/mac48-address.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/traced-callback.h"

#include <list>
#include <map>
#include <vector>

namespace ns3
{
namespace ethernet
{
class EthernetSwitchScheduler;

/**
 * @ingroup ethernet
 * @brief A frame waiting in the switch buffer for its turn on an output port.
 */
struct SwitchBufferEntry
{
    Ptr<const Packet> packet;            //!< The frame payload
    EthernetHeader header;               //!< The header the frame was received with
    Ptr<EthernetNetDevice> outgoingPort; //!< The port the frame is destined to
    Ptr<EthernetNetDevice> incomingPort; //!< The port the frame was received on
};

/**
 * @ingroup ethernet
 * @brief Ethernet Switch model
 *
 * A learning Ethernet switch. Frames received on a port are used to associate
 * their source address with that port, and are forwarded to the port that the
 * destination address was learned on, or flooded to every other port when the
 * destination address is unknown, broadcast or multicast.
 *
 * Frames are not handed to an output port directly. They are placed in a
 * buffer owned by the switch, out of which an EthernetSwitchScheduler picks
 * them and hands them to the output ports. Since the output ports keep their
 * own transmit queues, a frame can only be handed over when there is room in
 * the queue of the port it is destined to. What is done with a frame that an
 * output port cannot take is up to the scheduler.
 */
class EthernetSwitch : public NetDevice
{
  public:
    /**
     * @brief Signature of the callback establishing an additional memory limit.
     *
     * Called with the output port a frame is destined to and the size, in
     * bytes, of the frame that would be queued on it.
     */
    using MemoryCheckCallback = Callback<bool, Ptr<EthernetNetDevice>, uint32_t>;

    /**
     * @brief Signature of the callback establishing a switch-wide memory limit.
     *
     * Called before a frame is removed from an ingress RX queue. The callback
     * receives the size, in bytes, of the frame that would be accepted by the
     * switch.
     */
    using SwitchMemoryCheckCallback = Callback<bool, uint32_t>;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    EthernetSwitch();
    ~EthernetSwitch() override;

    // Delete copy constructor and assignment operator to avoid misuse
    EthernetSwitch(const EthernetSwitch&) = delete;
    EthernetSwitch& operator=(const EthernetSwitch&) = delete;

    /**
     * @brief Add a port to the switch.
     * @param port Pointer to the port to add.
     */
    void AddPort(Ptr<EthernetNetDevice> port);

    /**
     * @brief Get the number of ports in the switch.
     * @return The number of ports.
     */
    uint32_t GetNPorts() const;

    /**
     * @brief Get the n-th port in the switch.
     * @param n The index of the port.
     * @return Pointer to the n-th port.
     */
    Ptr<EthernetNetDevice> GetPort(uint32_t n) const;

    /**
     * @brief Receive queued frames from a port.
     * @param incomingPort Pointer to the port that received the frames.
     */
    void ReceiveFromPort(Ptr<EthernetNetDevice> incomingPort);

    /**
     * @brief Transmit a packet to a port.
     * @param outgoingPort Pointer to the port to transmit the packet to.
     * @param packet Pointer to the packet to transmit.
     * @param protocol The protocol of the packet.
     * @param source The source address of the packet.
     * @param destination The destination address of the packet.
     * @return True if the port accepted the packet.
     */
    bool TransmitToPort(Ptr<EthernetNetDevice> outgoingPort,
                        Ptr<const Packet> packet,
                        uint16_t protocol,
                        const Address& source,
                        const Address& destination);

    /**
     * @brief Check whether a port can take a frame for transmission.
     * @param port The port the frame is destined to.
     * @param packet The frame payload.
     * @return True if the port can take the frame.
     */
    bool CanForwardTo(Ptr<EthernetNetDevice> port, Ptr<const Packet> packet) const;

    /**
     * @brief Check whether switch can accept a frame of a given size.
     * @param bytes The size of the frame.
     * @return True if the switch can accept the frame.
     */
    bool CanAccept(uint32_t bytes) const;

    /**
     * @brief Install an additional limit on top of the port queue limits.
     *
     * Real switches often hold their queues in memory that is shared between
     * the transmit and the receive side of a port, or between all the ports,
     * rather than handing each queue a fixed allocation of its own. The limit
     * that decides whether a frame is accepted is then not the limit of the
     * queue it would be placed in. Installing a callback here lets such a
     * limit be enforced alongside the port queue limits, which are always
     * checked. No additional limit is enforced by default.
     *
     * @param check The callback establishing the additional limit.
     */
    void SetMemoryCheck(MemoryCheckCallback check);

    /**
     * @brief Set the switch-wide memory check callback.
     * @param check Callback determine whether the switch can accept an incoming frame.
     */
    void SetSwitchMemoryCheck(SwitchMemoryCheckCallback check);

    /**
     * @brief Learn the MAC address and associate it with a port.
     * @param source The MAC address to learn.
     * @param port The port associated with the MAC address.
     */
    void LearnMacAddress(Mac48Address source, Ptr<EthernetNetDevice> port);

    /**
     * @brief Get the learned state of a MAC address.
     * @param source The MAC address to look up.
     * @return Pointer to the port associated with the MAC address, or nullptr if not found.
     */
    Ptr<EthernetNetDevice> LookupMacAddress(Mac48Address source);

    /**
     * @brief Floods frames by transmitting them on every switch port other than
     * the port on which the frame was received.
     *
     * @param incomingPort The ingress switch port.
     * @param packet The Ethernet frame payload.
     * @param header The parsed Ethernet header.
     */
    void FloodFrame(Ptr<EthernetNetDevice> incomingPort,
                    Ptr<const Packet> packet,
                    const EthernetHeader& header);

    /**
     * @brief Install a packet scheduler.
     * @param scheduler Pointer to the scheduler to install.
     */
    void SetScheduler(Ptr<EthernetSwitchScheduler> scheduler);

    /**
     * @brief Add a forwarding request to the switch-owned buffer.
     * @param entry The forwarding request to queue.
     */
    void EnqueueToBuffer(const SwitchBufferEntry& entry);

    /**
     * @brief Check whether the switch-owned buffer contains pending entries.
     * @return True if one or more entries are pending.
     */
    bool HasBufferedFrames() const;

    /**
     * @brief Get reference to switch shared buffer.
     * @return Reference to the list of buffered entries.
     */
    std::list<SwitchBufferEntry>& GetBuffer();

    /**
     * @brief Report a frame that the switch gave up on forwarding.
     *
     * Called by the scheduler when its backpressure policy is to discard a
     * frame that an output port cannot take.
     *
     * @param entry The forwarding request that was given up on.
     */
    void NotifyForwardDrop(const SwitchBufferEntry& entry);

    /**
     * @brief Check whether a port has sending enabled.
     * @param port The port to check.
     * @return True if sending is enabled, false otherwise.
     */
    bool IsPortSendEnabled(Ptr<EthernetNetDevice> port) const;

    // Inherited from NetDevice
    void SetIfIndex(const uint32_t index) override;
    uint32_t GetIfIndex() const override;
    Ptr<Channel> GetChannel() const override;
    void SetAddress(Address address) override;
    Address GetAddress() const override;
    bool SetMtu(const uint16_t mtu) override;
    uint16_t GetMtu() const override;
    bool IsLinkUp() const override;
    void AddLinkChangeCallback(Callback<void> callback) override;
    bool IsBroadcast() const override;
    Address GetBroadcast() const override;
    bool IsMulticast() const override;
    Address GetMulticast(Ipv4Address multicastGroup) const override;
    Address GetMulticast(Ipv6Address addr) const override;
    bool IsBridge() const override;
    bool IsPointToPoint() const override;
    bool Send(Ptr<Packet> packet, const Address& destination, uint16_t protocolNumber) override;
    bool SendFrom(Ptr<Packet> packet,
                  const Address& source,
                  const Address& destination,
                  uint16_t protocolNumber) override;
    Ptr<Node> GetNode() const override;
    void SetNode(Ptr<Node> node) override;
    bool NeedsArp() const override;
    void SetReceiveCallback(NetDevice::ReceiveCallback cb) override;
    void SetPromiscReceiveCallback(NetDevice::PromiscReceiveCallback cb) override;
    bool SupportsSendFrom() const override;

  protected:
    void DoDispose() override;

  private:
    std::vector<Ptr<EthernetNetDevice>> m_ports; //!< Vector of ports in the switch

    /**
     * @brief Structure holding the status of an address
     */
    struct LearnedState
    {
        Ptr<EthernetNetDevice> associatedPort; //!< port associated with the address
        Time expirationTime;                   //!< time it takes for learned MAC state to expire
    };

    std::list<SwitchBufferEntry> m_buffer; //!< Buffer holding pending forwarding requests

    Ptr<EthernetSwitchScheduler> m_scheduler; //!< Packet scheduler manages the shared buffer

    std::map<Mac48Address, LearnedState>
        m_learnState;      //!< Map of learned MAC addresses and their states
    Time m_expirationTime; //!< Time after which learned MAC states expire

    MemoryCheckCallback m_memoryCheck; //!< Additional limit checked on top of the port queue limits
    SwitchMemoryCheckCallback
        m_switchMemoryCheck; //!< Switch-wide limit checked before accepting a frame

    Ptr<Node> m_node;                                      //!< Node owning this netdevice
    uint32_t m_ifIndex;                                    //!< Interface index
    uint16_t m_mtu;                                        //!< MTU of the switch netdevice
    bool m_linkUp;                                         //!< Link state flag
    bool m_sendEnable;                                     //!< Whether sending is enabled
    Address m_address;                                     //!< MAC address of the switch netdevice
    NetDevice::ReceiveCallback m_rxCallback;               //!< Receive callback
    NetDevice::PromiscReceiveCallback m_promiscRxCallback; //!< Promiscuous receive callback
    TracedCallback<> m_linkChangeCallbacks;                //!< Link state callbacks

    /**
     * @brief The trace source fired when the switch gives up on forwarding a frame.
     */
    TracedCallback<Ptr<const Packet>> m_forwardDropTrace;
};
} // namespace ethernet
} // namespace ns3
#endif
