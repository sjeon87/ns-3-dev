/*
 * Copyright (c) 2026 Somaiya Vidyavihar University, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manmita Das <das.manmita12@gmail.com>
 */

#ifndef ETHERNET_MAC_H
#define ETHERNET_MAC_H

#include "ns3/ethernet-header.h"
#include "ns3/mac48-address.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/queue.h"
#include "ns3/traced-callback.h"

namespace ns3
{

class NetDevice;

namespace ethernet
{

class EthernetNetDevice;
class EthernetPhy;

/**
 * @brief EthernetMAC states.
 */
enum class EthernetMacState : uint8_t
{
    MAC_IDLE = 0,
    MAC_TRANSMITTING,
    MAC_GAP
};

std::ostream& operator<<(std::ostream& os, EthernetMacState state);

/**
 * @ingroup ethernet
 * @class EthernetMac
 * @brief Represents the mac layer of an Ethernet network interface.
 */
class EthernetMac : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    EthernetMac();

    ~EthernetMac() override;

    // Delete copy constructor and assignment operator to avoid misuse
    EthernetMac(const EthernetMac&) = delete;
    EthernetMac& operator=(const EthernetMac&) = delete;

    /**
     * @brief Set the PHY layer for this MAC.
     * @param phy The PHY layer to set.
     */
    void SetPhy(Ptr<EthernetPhy> phy);

    /**
     * @brief Get the PHY layer for this MAC.
     * @return The PHY layer.
     */
    Ptr<EthernetPhy> GetPhy();

    /**
     * @brief Set the network device for this MAC.
     * @param d The device to set.
     */
    void SetDevice(Ptr<NetDevice> d);

    /**
     * @brief Get the network device for this MAC.
     * @return The network device.
     */
    Ptr<NetDevice> GetDevice() const;

    /**
     * @brief Accept a packet from the device for transmission.
     *
     * The packet is encapsulated in an Ethernet frame. If the transmitter is
     * idle, the frame is handed to the PHY straight away, otherwise it waits in
     * the transmit queue.
     *
     * @param packet The packet to send.
     * @param source The source MAC address.
     * @param destination The destination MAC address.
     * @param protocolNumber The protocol number.
     * @return True if the frame was accepted, false if the transmit queue is full.
     */
    bool Send(Ptr<Packet> packet,
              const Mac48Address& source,
              const Mac48Address& destination,
              uint16_t protocolNumber);

    /**
     * @brief Send a pause frame.
     * @param pauseTime The time for which to pause transmission.
     */
    void SendPauseFrame(uint16_t pauseTime);

    /**
     * @brief Accept a frame from the PHY.
     *
     * The frame is validated before it is enqueued in the receive queue. If the
     * receiver is idle, the frame is processed straight away, otherwise it
     * waits in the receive queue. A frame arriving while the receive queue is
     * full is dropped, and makes the MAC ask the peer to pause.
     *
     * @param frame The received frame.
     */
    void Receive(Ptr<Packet> frame);

    /**
     * @brief Set the interframe gap for the MAC.
     * @param gap The interframe gap to set.
     */
    void SetInterframeGap(Time gap);

    /**
     * @brief Suspend transmission for the given duration.
     *
     * Frames offered while transmission is suspended are queued rather than
     * dropped. Called upon reception of an IEEE 802.3x PAUSE frame.
     *
     * @param pauseDuration The time for which to suspend transmission.
     */
    void PauseTransmission(Time pauseDuration);

    /**
     * @brief Resume transmission, and send whatever is waiting in the queue.
     */
    void ResumeTransmission();

    /**
     * @brief Check whether transmission is currently suspended.
     * @return True if transmission is paused, false otherwise.
     */
    bool IsTxPaused() const;

    /**
     * @brief Get the transmission queue.
     * @return Pointer to the transmission queue.
     */
    Ptr<Queue<Packet>> GetTxQueue() const;

    /**
     * @brief Set the transmission queue.
     * @param queue Pointer to the transmission queue.
     */
    void SetTxQueue(Ptr<Queue<Packet>> queue);

    /**
     * @brief Get the reception queue.
     * @return Pointer to the reception queue.
     */
    Ptr<Queue<Packet>> GetRxQueue() const;

    /**
     * @brief Set the reception queue.
     * @param queue Pointer to the reception queue.
     */
    void SetRxQueue(Ptr<Queue<Packet>> queue);

    /**
     * @brief The length/type value identifying an IEEE 802.3x MAC control frame.
     */
    static const uint16_t PAUSE_LENGTH_TYPE = 0x8808;

    /**
     * @brief Ethernet PHY has completed transmission of the current packet, schedules a Interframe
     * Gap.
     */
    void TxEnd();

    /**
     * @brief Ethernet MAC has completed the Interframe Gap, and is ready to transmit the next
     * packet. Gap.
     */
    void InterframeGapEnd();

    /**
     * @brief Get the current transmit state of the MAC layer.
     * @return The current MAC state.
     */
    EthernetMacState GetTxMacState() const;

    /**
     * @brief Set the MAC address of this device.
     * @param address The MAC address to set.
     */
    void SetAddress(Address address);

    /**
     * @brief Get the MAC address of this device.
     * @return The MAC address.
     */
    Address GetAddress() const;

    /**
     * @brief Set the MTU for this device.
     * @param mtu The MTU to set.
     * @return True if the MTU was set successfully.
     */
    bool SetMtu(const uint16_t mtu);

    /**
     * @brief Get the MTU for this device.
     * @return The MTU.
     */
    uint16_t GetMtu() const;

    /**
     * @brief Check if a pause frame has been sent.
     * @return True if a pause frame has been sent.
     */
    bool IsPauseFrameSent() const;

    /**
     * @brief Set the flag indicating whether a pause frame has been sent.
     * @param sent The flag value.
     */
    void SetPauseFrameSent(bool sent);

    /**
     * @brief Convert pause quanta to a time duration.
     *
     * IEEE 802.3 defines the PAUSE time field in units of pause quanta.
     * Each pause quantum corresponds to 512 bit-times on the Ethernet link
     * The returned duration is computed using the configured PHY data rate.
     *
     * @param pauseQuanta The pause quanta to convert.
     * @return The equivalent time.
     */
    Time ConvertPauseQuantaToTime(uint16_t pauseQuanta);

    /**
     * @brief Determine whether a pause frame should be sent.
     * @return True if a pause frame should be sent, false otherwise.
     */
    virtual bool ShouldSendPauseFrame() const;

    /**
     * @brief send an unpause frame when the receive queue occupancy is below a certain threshold.
     * @return True if an unpause frame should be sent, false otherwise.
     */
    virtual bool SendUnpauseFrame() const;

    /**
     * @brief The trace source fired when packets come into the "top" of the device
     * at the L3/L2 transition, before being queued for transmission.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macTxTrace;

    /**
     * The trace source fired when packets where successfully transmitted, that is
     * an acknowledgment was received, if requested, or the packet was
     * successfully sent by L1, if no ACK was requested.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macTxOkTrace;

    /**
     * @brief The trace source fired when packets coming into the "top" of the device
     * at the L3/L2 transition are dropped before being queued for transmission.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macTxDropTrace;

    /**
     * The trace source fired for packets successfully received by the device
     * immediately before being forwarded up to higher layers (at the L2/L3
     * transition).  This is a promiscuous trace.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macPromiscRxTrace;

    /**
     * The trace source fired for packets successfully received by the device
     * immediately before being forwarded up to higher layers (at the L2/L3
     * transition).  This is a non-promiscuous trace.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macRxTrace;

    /**
     * The trace source fired for packets successfully received by the device
     * but dropped before being forwarded up to higher layers (at the L2/L3
     * transition).
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macRxDropTrace;

    /**
     * @brief A trace source that emulates a non-promiscuous protocol sniffer connected
     * to the device.  Unlike your average everyday sniffer, this trace source
     * will not fire on PACKET_OTHERHOST events.
     *
     * On the transmit size, this trace hook will fire after a packet is dequeued
     * from the device queue for transmission.  In Linux, for example, this would
     * correspond to the point just before a device hard_start_xmit where
     * dev_queue_xmit_nit is called to dispatch the packet to the PF_PACKET
     * ETH_P_ALL handlers.
     *
     * On the receive side, this trace hook will fire when a packet is received,
     * just before the receive callback is executed.  In Linux, for example,
     * this would correspond to the point at which the packet is dispatched to
     * packet sniffers in netif_receive_skb.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_snifferTrace;

    /**
     * @brief A trace source that emulates a promiscuous mode protocol sniffer connected
     * to the device.  This trace source fire on packets destined for any host
     * just like your average everyday packet sniffer.
     *
     * On the transmit size, this trace hook will fire after a packet is dequeued
     * from the device queue for transmission.  In Linux, for example, this would
     * correspond to the point just before a device hard_start_xmit where
     * dev_queue_xmit_nit is called to dispatch the packet to the PF_PACKET
     * ETH_P_ALL handlers.
     *
     * On the receive side, this trace hook will fire when a packet is received,
     * just before the receive callback is executed.  In Linux, for example,
     * this would correspond to the point at which the packet is dispatched to
     * packet sniffers in netif_receive_skb.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_promiscSnifferTrace;

    /**
     * @brief Get the size of an Ethernet frame on the wire.
     * @param payloadSize Size of the payload in bytes.
     * @return Frame size including Ethernet header and FCS.
     */
    static uint32_t GetFrameSize(uint32_t payloadSize);

  protected:
    void DoDispose() override;

  private:
    /**
     * @brief Change the transmit state of the MAC layer.
     * @param newState The new state to set.
     */
    void SetTxMacState(EthernetMacState newState);

    /**
     * @brief Hand a complete frame to the PHY for transmission.
     * @param frame The frame to transmit.
     */
    void TxStart(Ptr<Packet> frame);

    /**
     * @brief Transmit the next queued frame, if the transmitter is free to do so.
     */
    void TxNext();

    /**
     * @brief Process a received frame and hand it to the netdevice receive callbacks.
     * @param frame The frame to process.
     */
    void ProcessRxPacket(Ptr<Packet> frame);

    /**
     * @brief Release the receiver and process the next queued frame, if any.
     */
    void RxNext();

    /**
     * @brief Classify a received frame and hand it to the netdevice receive callbacks.
     *
     * The promiscuous callback, when set, is given every frame. The
     * non-promiscuous callback is only given frames addressed to this device,
     * to the broadcast address or to a multicast group.
     *
     * @param frame The complete received frame, including header and FCS.
     * @param payload The decapsulated payload packet.
     * @param header The parsed Ethernet header.
     */
    void ForwardUp(Ptr<const Packet> frame, Ptr<Packet> payload, const EthernetHeader& header);

    /**
     * @brief Apply a received IEEE 802.3x PAUSE frame to the transmit path.
     * @param payload The payload of the received MAC control frame.
     */
    void ProcessPauseFrame(Ptr<Packet> payload);

    /**
     * @brief The device to which this MAC layer is attached.
     */
    Ptr<EthernetNetDevice> m_device;

    /**
     * @brief The PHY to which this MAC is associated with.
     */
    Ptr<EthernetPhy> m_phy;

    /**
     * @brief The MAC address which has been assigned to this device.
     */
    Mac48Address m_address;

    /**
     * @brief The Maximum Transmission Unit. This corresponds to the maximum
     * number of bytes that can be transmitted as seen from higher layers.
     */
    uint32_t m_mtu;

    /**
     * @brief The transmit state of the MAC layer.
     */
    EthernetMacState m_macTxState;

    /**
     * @brief Queue holding the frames waiting for transmission.
     */
    Ptr<Queue<Packet>> m_txQueue;

    /**
     * @brief Queue holding the received frames waiting to be processed.
     */
    Ptr<Queue<Packet>> m_rxQueue;

    /**
     * @brief Flag indicating whether transmission is currently paused due to
     * reception of an Ethernet PAUSE frame.
     */
    bool m_txPaused;

    /**
     * @brief Event used to resume transmission after a PAUSE interval.
     */
    EventId m_resumeEvent;

    /**
     * @brief The interframe gap that the Net Device uses insert time between packet
     * transmission
     */
    Time m_interframeGapTime;

    /**
     * @brief Indicates whether a pause frame has been sent.
     */
    bool m_isPauseFrameSent;

    /**
     * @brief Default Maximum Transmission Unit (MTU) for the EthernetNetDevice
     */
    static const uint16_t DEFAULT_MTU = 1500;

    /**
     * @brief Minimum payload carried by an Ethernet frame.
     *
     * Together with the 14 byte header and the 4 byte FCS this makes up the
     * 64 byte minimum frame size by IEEE 802.3.
     */
    static const uint16_t MIN_PAYLOAD_SIZE = 46;
};

} // namespace ethernet
} // namespace ns3

#endif /* ETHERNET_MAC_H */
