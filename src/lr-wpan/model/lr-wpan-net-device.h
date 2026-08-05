/*
 * Copyright (c) 2011 The Boeing Company
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:
 *  Tom Henderson <thomas.r.henderson@boeing.com>
 *  Tommaso Pecorella <tommaso.pecorella@unifi.it>
 *  Margherita Filippetti <morag87@gmail.com>
 */
#ifndef LR_WPAN_NET_DEVICE_H
#define LR_WPAN_NET_DEVICE_H

#include "lr-wpan-mac.h"

#include "ns3/net-device.h"
#include "ns3/traced-callback.h"

#include <vector>

namespace ns3
{

class SpectrumChannel;
class Node;

namespace lrwpan
{

class LrWpanPhy;
class LrWpanCsmaCa;

/**
 * @ingroup lr-wpan
 *
 * @brief Network layer to device interface.
 *
 * The ns3::NetDevice includes IP-specific API such as GetMulticast(), Send()
 * and SendTo() methods, which do not map well the 802.15.4 MAC MCPS
 * DataRequest primitive.  So, the basic design is to provide, as
 * much as makes sense, the class ns3::NetDevice API, but rely on the user
 * accessing the LrWpanMac pointer to make 802.15.4-specific API calls.
 * As such, this is really just an encapsulating class.
 */
class LrWpanNetDevice : public NetDevice
{
  public:
    /**
     * Get the type ID.
     *
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    LrWpanNetDevice();
    ~LrWpanNetDevice() override;

    /**
     * Set the MAC to be used by this NetDevice.
     *
     * @param mac the MAC to be used
     */
    void SetMac(Ptr<LrWpanMac> mac);

    /**
     * Set the PHY to be used by the MAC and this NetDevice.
     *
     * @param phy the PHY to be used
     */
    void SetPhy(Ptr<LrWpanPhy> phy);

    /**
     * Set the CSMA/CA implementation to be used by the MAC and this NetDevice.
     *
     * @param csmaca the CSMA/CA implementation to be used
     */
    void SetCsmaCa(Ptr<LrWpanCsmaCa> csmaca);

    /**
     * Set the channel to which the NetDevice, and therefore the PHY, should be
     * attached to.
     *
     * @param channel the channel to be used
     */
    void SetChannel(Ptr<SpectrumChannel> channel);

    /**
     * Get the MAC used by this NetDevice.
     *
     * @return the MAC object
     */
    Ptr<LrWpanMac> GetMac() const;

    /**
     * Get the PHY used by this NetDevice.
     *
     * @return the PHY object
     */
    Ptr<LrWpanPhy> GetPhy() const;

    /**
     * Get the CSMA/CA implementation used by this NetDevice.
     *
     * @return the CSMA/CA implementation object
     */
    Ptr<LrWpanCsmaCa> GetCsmaCa() const;

    // From class NetDevice
    void SetIfIndex(const uint32_t index) override;
    uint32_t GetIfIndex() const override;
    Ptr<Channel> GetChannel() const override;
    /**
     * This method indirects to LrWpanMac::SetShortAddress ()
     * @param address The short address.
     */
    void SetAddress(Address address) override;
    /**
     * This method indirects to LrWpanMac::SetShortAddress ()
     * @returns The short address.
     */
    Address GetAddress() const override;

    /**
     * This method is use to manually configure the coordinator through
     * which the device or coordinator is associated. When assigning a short address
     * the extended address must also be present.
     *
     * @param panId The id of the PAN used by the coordinator device.
     *
     * @param coordExtAddr The coordinator extended address (EUI-64) through which this
     *                     device or coordinator is associated.
     *
     * @param coordShortAddr The coordinator assigned short address through which this
     *                       device or coordinator is associated.
     *                       [FF:FF] address indicates that the value is unknown.
     *                       [FF:FE] indicates that the associated coordinator is using only
     *                       its extended address.
     *
     *
     * @param assignedShortAddr The assigned short address for this device.
     *                          [FF:FF] address indicates that the device have no short address
     *                                  and is not associated.
     *                          [FF:FE] address indicates that the devices has associated but
     *                          has not been allocated a short address.
     *
     */
    void SetPanAssociation(uint16_t panId,
                           Mac64Address coordExtAddr,
                           Mac16Address coordShortAddr,
                           Mac16Address assignedShortAddr);

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
    bool Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber) override;
    bool SendFrom(Ptr<Packet> packet,
                  const Address& source,
                  const Address& dest,
                  uint16_t protocolNumber) override;
    Ptr<Node> GetNode() const override;
    void SetNode(Ptr<Node> node) override;
    bool NeedsArp() const override;

    void SetReceiveCallback(NetDevice::ReceiveCallback cb) override;
    void SetPromiscReceiveCallback(PromiscReceiveCallback cb) override;
    bool SupportsSendFrom() const override;

    /**
     * The callback used by the MAC to hand over incoming packets to the
     * NetDevice. This callback will in turn use the ReceiveCallback set by
     * SetReceiveCallback() to notify upper layers.
     *
     * @param params 802.15.4 specific parameters, including source and destination addresses
     * @param pkt the packet do be delivered
     */
    void McpsDataIndication(McpsDataIndicationParams params, Ptr<Packet> pkt);

    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model.  Return the number of streams that have been assigned.
     *
     * @param stream first stream index to use
     * @return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream);

  private:
    // Inherited from NetDevice/Object
    void DoDispose() override;
    void DoInitialize() override;

    /**
     * Mark NetDevice link as up.
     */
    void LinkUp();

    /**
     * Mark NetDevice link as down.
     */
    void LinkDown();

    /**
     * Attribute accessor method for the "Channel" attribute.
     *
     * @return the channel to which this NetDevice is attached
     */
    Ptr<SpectrumChannel> DoGetChannel() const;

    /**
     * Configure PHY, MAC and CSMA/CA.
     */
    void CompleteConfig();

    /**
     * Get this device's link-layer addresses, most preferred first for IPv6 IID
     * formation: the 16-bit short address (when assigned) then the 64-bit extended
     * address. Used as the LinkLayerAddressProvider callback.
     *
     * @return the link-layer addresses
     */
    std::vector<Address> GetLinkLayerAddresses() const;

    /**
     * The MAC for this NetDevice.
     */
    Ptr<LrWpanMac> m_mac;

    /**
     * The PHY for this NetDevice.
     */
    Ptr<LrWpanPhy> m_phy;

    /**
     * The CSMA/CA implementation for this NetDevice.
     */
    Ptr<LrWpanCsmaCa> m_csmaca;

    /**
     * The node associated with this NetDevice.
     */
    Ptr<Node> m_node;

    /**
     * True if MAC, PHY and CSMA/CA where successfully configured and the
     * NetDevice is ready for being used.
     */
    bool m_configComplete;

    /**
     * Configure the NetDevice to request MAC layer acknowledgments when sending
     * packets using the Send() API.
     */
    bool m_useAcks;

    /**
     * Is the link/device currently up and running?
     */
    bool m_linkUp;

    /**
     * The interface index of this NetDevice.
     */
    uint32_t m_ifIndex;

    /**
     * Trace source for link up/down changes.
     */
    TracedCallback<> m_linkChanges;

    /**
     * Upper layer callback used for notification of new data packet arrivals.
     */
    ReceiveCallback m_receiveCallback;
};

} // namespace lrwpan
} // namespace ns3

#endif /* LR_WPAN_NET_DEVICE_H */
