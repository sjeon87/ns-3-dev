/*
 * Copyright (c) 2026 Somaiya Vidyavihar University, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manmita Das <das.manmita12@gmail.com>
 */

#ifndef ETHERNET_PHY_H
#define ETHERNET_PHY_H

#include "ns3/data-rate.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"

namespace ns3
{

class ErrorModel;
class NetDevice;

namespace ethernet
{

class EthernetChannel;
class EthernetMac;
class EthernetNetDevice;

/**
 * @brief EthernetPHY states.
 */
enum class EthernetPhyState : uint8_t
{
    PHY_IDLE = 0,
    PHY_TRANSMITTING,
    PHY_RECEIVING
};

std::ostream& operator<<(std::ostream& os, EthernetPhyState state);

/**
 * @ingroup ethernet
 * @class EthernetPhy
 * @brief Represents the physical layer of an Ethernet network interface.
 */
class EthernetPhy : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    EthernetPhy();

    ~EthernetPhy() override;

    // Delete copy constructor and assignment operator to avoid misuse
    EthernetPhy(const EthernetPhy&) = delete;
    EthernetPhy& operator=(const EthernetPhy&) = delete;

    /**
     * @brief Set the channel for the EthernetPhy.
     * @param c Pointer to the EthernetChannel to set.
     */
    void SetChannel(Ptr<EthernetChannel> c);

    /**
     * @brief Get the channel for the EthernetPhy.
     * @return Pointer to the currently connected EthernetChannel.
     */
    Ptr<EthernetChannel> GetChannel();

    /**
     * @brief Get the MAC layer associated with this PHY.
     * @return mac Pointer to the associated EthernetMac.
     */
    Ptr<EthernetMac> GetMac() const;

    /**
     * @brief Set the MAC layer for this PHY.
     * @param mac Pointer to the EthernetMac to set.
     */
    void SetMac(Ptr<EthernetMac> mac);

    /**
     * @brief Set the network device for the EthernetPhy.
     * @param d Pointer to the EthernetNetDevice to set.
     */
    void SetDevice(Ptr<EthernetNetDevice> d);

    /**
     * @brief Get the network device for the EthernetPhy.
     * @return Pointer to the currently connected EthernetNetDevice.
     */
    Ptr<EthernetNetDevice> GetDevice() const;

    /**
     * @brief Get the assigned data rate of the channel
     * @return Returns the DataRate to be used by device transmitters.
     */
    DataRate GetDataRate() const;

    /**
     * @brief start transmitting the packet over the channel.
     * @param packet Pointer to the packet to transmit.
     * @return true if transmission is accepted
     */
    bool TxStart(Ptr<Packet> packet);

    /**
     * @brief Handle the completion of a packet transmission.
     * @param packet Pointer to the transmitted packet.
     * @return true if the transmission completed successfully; false otherwise.
     */
    bool TxEnd(Ptr<Packet> packet);

    /**
     * @brief Receiver has received the first bit of the packet
     * @param packet Pointer to the received packet
     * @param sender the EthernetNetDevice that transmitted the packet in the first place
     */
    void RxStart(Ptr<const Packet> packet, Ptr<EthernetNetDevice> sender);

    /**
     * @brief Receive a packet from a connected EthernetChannel.
     *
     * The EthernetNetDevice receives packets from its connected channel
     * and forwards them up the protocol stack.  This is the public method
     * used by the channel to indicate that the last bit of a packet has
     * arrived at the device.
     *
     * @param packet Pointer to the received packet.
     * @param sender the EthernetNetDevice that transmitted the packet in the first place
     */
    void Receive(Ptr<const Packet> packet, Ptr<EthernetNetDevice> sender);

    /**
     * @brief Set the error model for the EthernetPhy.
     * @param e Pointer to the ErrorModel to set.
     */
    void SetErrorModel(Ptr<ErrorModel> e);

    /**
     * @brief Get the error model for the EthernetPhy.
     * @return Pointer to the currently set ErrorModel.
     */
    Ptr<ErrorModel> GetErrorModel() const;

    /**
     * @brief Calculate the transmission time for a packet.
     * @param packet Pointer to the packet for which to calculate transmission time.
     * @return The transmission time.
     */
    Time CalculateTxTime(Ptr<const Packet> packet) const;

    /**
     * @brief Bring the link up and notify the attached netdevice.
     */
    void LinkUp();

    /**
     * @brief Bring the link down and notify the attached netdevice.
     */
    void LinkDown();

    /**
     * @brief Check whether the link is up.
     * @return true if the link is up, false otherwise.
     */
    bool IsLinkUp() const;

    /**
     * @brief Update the MAC inter-frame gap based on the current channel data rate.
     */
    void UpdateInterframeGap();

    /**
     * @brief The trace source fired when a packet begins the transmission process on
     * the medium.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_phyTxBeginTrace;

    /**
     * @brief The trace source fired when a packet ends the transmission process on
     * the medium.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_phyTxEndTrace;

    /**
     * @brief The trace source fired when the phy layer drops a packet as it tries
     * to transmit it.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_phyTxDropTrace;

    /**
     * @brief The trace source fired when a packet begins the reception process from
     * the medium.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_phyRxBeginTrace;

    /**
     * @brief The trace source fired when a packet ends the reception process from
     * the medium.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_phyRxEndTrace;

    /**
     * @brief The trace source fired when the phy layer drops a packet it has received.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_phyRxDropTrace;

  protected:
    void DoDispose() override;

  private:
    /**
     * @brief Change the receive state of the EthernetPhy.
     * @param newState The new state to set.
     */
    void ChangeRxState(EthernetPhyState newState);

    /**
     * @brief Change the transmit state of the EthernetPhy.
     * @param newState The new state to set.
     */
    void ChangeTxState(EthernetPhyState newState);
    /**
     * @brief The EthernetNetDevice to which this EthernetPhy is attached.
     */
    Ptr<EthernetNetDevice> m_device;

    /**
     * @brief The EthernetChannel to which this EthernetPhy has been
     * attached.
     */
    Ptr<EthernetChannel> m_channel;

    /**
     * @brief The EthernetMac to which this EthernetPhy has been
     * attached.
     */
    Ptr<EthernetMac> m_mac;

    /**
     * @brief The error model for the receive path.
     */
    Ptr<ErrorModel> m_receiveErrorModel;

    /**
     * @brief The current state of the receive path.
     */
    EthernetPhyState m_rxState;

    /**
     * @brief The current state of the transmit path.
     */
    EthernetPhyState m_txState;

    /**
     * @brief Indicates whether the link is up.
     */
    bool m_linkUp;
};

} // namespace ethernet
} // namespace ns3

#endif /* ETHERNET_PHY_H */
