/*
 * Copyright (c) 2026 Somaiya Vidyavihar University, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manmita Das <das.manmita12@gmail.com>
 */

#ifndef ETHERNET_CHANNEL_H
#define ETHERNET_CHANNEL_H

#include "ns3/channel.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"

#include <vector>

namespace ns3
{

namespace ethernet
{

class EthernetNetDevice;

/**
 * @ingroup ethernet
 * @brief Ethernet channel model
 *
 * EthernetChannel models a point-to-point full-duplex for
 * EthernetNetDevice instances. It provides a interface to
 * attach devices and propagation of packets is done.
 *
 * Length and medium speed of the channel can be set using attributes.
 * Changing these attributes while packets are being transmitted is not
 * supported.
 *
 * The channel is assumed to be of a quality that supports the link types of
 * the devices attached to it, that is, Cat-6 or better. Cable types are not
 * modelled; the only cable property the channel enforces is the 100 m maximum
 * length.
 */
class EthernetChannel : public Channel
{
  public:
    /**
     * Get the TypeId for this class.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    EthernetChannel();

    ~EthernetChannel() override;

    // Delete copy constructor and assignment operator to avoid misuse
    EthernetChannel(const EthernetChannel&) = delete;
    EthernetChannel& operator=(const EthernetChannel&) = delete;

    /**
     * @brief Attach an EthernetNetDevice to this channel.
     * @param device Pointer to the EthernetNetDevice to attach
     * @return The assigned device ID
     */
    uint32_t Attach(Ptr<EthernetNetDevice> device);

    /**
     * @brief Detach a EthernetNetDevice from the channel.
     * @param device Pointer to the EthernetNetDevice to detach
     * @return true if detached successfully
     */
    bool Detach(Ptr<EthernetNetDevice> device);

    /**
     * @returns the number of NetDevices connected to this Channel.
     */
    std::size_t GetNDevices() const override;

    /**
     * @param i index of NetDevice to retrieve
     * @returns one of the NetDevices connected to this channel.
     */
    Ptr<NetDevice> GetDevice(std::size_t i) const override;

    /**
     * @brief Get the assigned speed-of-light delay of the channel
     * @return Returns the delay used by the channel.
     */
    Time GetDelay() const;

    /**
     * @brief Settle the Ethernet link type used by the attached devices.
     *
     * IEEE 802.3 autonegotiation is not modelled. Instead, the devices sharing
     * the channel simply settle on the slowest of the link types they support,
     * and they do so with no delay. While a single device is attached, it runs
     * at its own maximum supported link type.
     *
     * This is called when a device is attached or detached, and when the maximum
     * supported link type of an attached device changes.
     */
    void NegotiateLinkType();

    /**
     * @brief Starts the propagation of data on the wire
     * @param packet Pointer to the packet being propagated
     * @param srcDevice Device from which propagation is starting
     */
    void PropagationStart(Ptr<const Packet> packet, Ptr<EthernetNetDevice> srcDevice);

    /**
     * @brief PHY layer has finished transmitting the packet on the channel
     * @param packet Pointer to the packet whose transmission has been completed on the channel
     * @param srcDevice Device from which transmission is completing
     */
    void TxEnd(Ptr<Packet> packet, Ptr<EthernetNetDevice> srcDevice);

    /**
     * @brief Set the length of the channel (m)
     * @param length The length of the channel (m)
     */
    void SetLength(double length);

    /**
     * @brief Get the length of the channel (m)
     * @return The length of the channel (m)
     */
    double GetLength() const;

    /**
     * @brief Set the propagation speed (m/s) in the propagation medium being considered
     * @param speed The propagation speed (m/s)
     */
    void SetSpeed(double speed);

    /**
     * @brief Get the propagation speed (m/s) in the propagation medium being considered
     * @return The propagation speed (m/s)
     */
    double GetSpeed() const;

  protected:
    void DoDispose() override;

  private:
    /**
     * @brief NetDevices currently connected to the channel
     */
    std::array<Ptr<EthernetNetDevice>, 2> m_deviceList{};

    /**
     * @brief The number of devices currently attached to the channel
     */
    uint8_t m_deviceCount = 0;

    /**
     * @brief The length of the channel (m).
     */
    double m_length;

    /**
     * @brief The propagation speed (m/s) in the propagation medium being considered.
     */
    double m_speed;
};

} // namespace ethernet
} // namespace ns3

#endif /* ETHERNET_CHANNEL_H */
