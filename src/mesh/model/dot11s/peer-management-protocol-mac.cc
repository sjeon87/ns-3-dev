/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Kirill Andreev <andreev@iitp.ru>
 */

#include "peer-management-protocol-mac.h"

#include "ie-dot11s-configuration.h"
#include "ie-dot11s-mesh-peering-management.h"
#include "mesh-peering-close-header.h"
#include "mesh-peering-confirm-header.h"
#include "mesh-peering-open-header.h"
#include "peer-management-protocol.h"

#include "ns3/log.h"
#include "ns3/mesh-information-element-vector.h"
#include "ns3/mesh-wifi-interface-mac.h"
#include "ns3/mgt-action-headers.h"
#include "ns3/simulator.h"
#include "ns3/wifi-mpdu.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PeerManagementProtocolMac");

namespace dot11s
{
PeerManagementProtocolMac::PeerManagementProtocolMac(uint32_t interface,
                                                     Ptr<PeerManagementProtocol> protocol)
{
    m_ifIndex = interface;
    m_protocol = protocol;
}

PeerManagementProtocolMac::~PeerManagementProtocolMac()
{
}

void
PeerManagementProtocolMac::SetParent(Ptr<MeshWifiInterfaceMac> parent)
{
    m_parent = parent;
    m_parent->TraceConnectWithoutContext("DroppedMpdu",
                                         MakeCallback(&PeerManagementProtocolMac::TxError, this));
    m_parent->TraceConnectWithoutContext("AckedMpdu",
                                         MakeCallback(&PeerManagementProtocolMac::TxOk, this));
}

void
PeerManagementProtocolMac::TxError(WifiMacDropReason reason, Ptr<const WifiMpdu> mpdu)
{
    m_protocol->TransmissionFailure(m_ifIndex, mpdu->GetHeader().GetAddr1());
}

void
PeerManagementProtocolMac::TxOk(Ptr<const WifiMpdu> mpdu)
{
    m_protocol->TransmissionSuccess(m_ifIndex, mpdu->GetHeader().GetAddr1());
}

bool
PeerManagementProtocolMac::Receive(Ptr<Packet> const_packet, const WifiMacHeader& header)
{
    NS_LOG_FUNCTION(this << const_packet << header);
    // First of all we copy a packet, because we need to remove some
    // headers
    Ptr<Packet> packet = const_packet->Copy();
    if (header.IsBeacon())
    {
        NS_LOG_DEBUG("Is Beacon from " << header.GetAddr2());
        MgtBeaconHeader beacon_hdr;
        packet->RemoveHeader(beacon_hdr);
        MeshInformationElementVector elements;
        // To determine header size here, we can rely on the knowledge that
        // this is the last header to remove.
        packet->RemoveHeader(elements, packet->GetSize());
        Ptr<IeBeaconTiming> beaconTiming =
            DynamicCast<IeBeaconTiming>(elements.FindFirst(IE_BEACON_TIMING));
        Ptr<IeMeshId> meshId = DynamicCast<IeMeshId>(elements.FindFirst(IE_MESH_ID));

        if (meshId && (m_protocol->GetMeshId()->IsEqual(*meshId)))
        {
            m_protocol->ReceiveBeacon(m_ifIndex,
                                      header.GetAddr2(),
                                      MicroSeconds(beacon_hdr.m_beaconInterval),
                                      beaconTiming);
        }
        else
        {
            NS_LOG_DEBUG("MeshId mismatch " << m_protocol->GetMeshId()->PeekString() << " "
                                            << (*meshId) << "; ignoring");
        }
        // Beacon shall not be dropped. May be needed to another plugins
        return true;
    }
    uint16_t aid = 0; // applicable only in Confirm message
    IeConfiguration config;
    if (header.IsAction())
    {
        NS_LOG_DEBUG("Is action");
        WifiActionHeader actionHdr;
        packet->RemoveHeader(actionHdr);
        WifiActionHeader::ActionValue actionValue = actionHdr.GetAction();
        // If can not handle - just return;
        if (actionHdr.GetCategory() != WifiActionHeader::SELF_PROTECTED)
        {
            NS_LOG_DEBUG("Cannot handle non SELF PROTECTED");
            return m_protocol->IsActiveLink(m_ifIndex, header.GetAddr2());
        }
        m_stats.rxMgt++;
        m_stats.rxMgtBytes += packet->GetSize();
        Mac48Address peerAddress = header.GetAddr2();
        Mac48Address peerMpAddress = header.GetAddr3();
        std::optional<IeMeshPeeringManagement> peerManagement;
        if (actionValue.selfProtectedAction == WifiActionHeader::PEER_LINK_OPEN)
        {
            NS_LOG_DEBUG("Received PEER_LINK_OPEN");
            MeshPeeringOpenHeader peeringOpenHeader;
            packet->RemoveHeader(peeringOpenHeader);

            // compare mesh id, drop if not equal
            auto meshId = peeringOpenHeader.Get<IeMeshId>();
            NS_ASSERT(meshId);
            if (!meshId.value().IsEqual(*(m_protocol->GetMeshId())))
            {
                NS_LOG_DEBUG("PEER_LINK_OPEN:  MeshId mismatch");
                m_protocol->ConfigurationMismatch(m_ifIndex, peerAddress);
                m_stats.brokenMgt++;
                return false;
            }

            // compare supported rates, drop if not compatible
            auto supportedRates = peeringOpenHeader.Get<SupportedRates>();
            NS_ASSERT(supportedRates);
            auto extendedRates = peeringOpenHeader.Get<ExtendedSupportedRatesIE>();
            if (!(m_parent->CheckSupportedRates(AllSupportedRates{.rates = supportedRates.value(),
                                                                  .extendedRates = extendedRates})))
            {
                NS_LOG_DEBUG("PEER_LINK_OPEN:  configuration mismatch");
                m_protocol->ConfigurationMismatch(m_ifIndex, peerAddress);
                m_stats.brokenMgt++;
                return false;
            }

            auto meshConfig = peeringOpenHeader.Get<IeConfiguration>();
            NS_ASSERT(meshConfig);
            config = meshConfig.value();

            peerManagement = peeringOpenHeader.Get<IeMeshPeeringManagement>();

            m_stats.rxOpen++;
        }
        else if (actionValue.selfProtectedAction == WifiActionHeader::PEER_LINK_CONFIRM)
        {
            NS_LOG_DEBUG("Received PEER_LINK_CONFIRM");
            MeshPeeringConfirmHeader peeringConfirmHeader;
            packet->RemoveHeader(peeringConfirmHeader);

            // compare mesh id, drop if not equal
            auto meshId = peeringConfirmHeader.Get<IeMeshId>();
            NS_ASSERT(meshId);
            if (!meshId.value().IsEqual(*(m_protocol->GetMeshId())))
            {
                NS_LOG_DEBUG("PEER_LINK_CONFIRM:  MeshId mismatch");
                m_protocol->ConfigurationMismatch(m_ifIndex, peerAddress);
                m_stats.brokenMgt++;
                return false;
            }

            // compare supported rates, drop if not compatible
            auto supportedRates = peeringConfirmHeader.Get<SupportedRates>();
            NS_ASSERT(supportedRates);
            auto extendedRates = peeringConfirmHeader.Get<ExtendedSupportedRatesIE>();
            if (!(m_parent->CheckSupportedRates(AllSupportedRates{.rates = supportedRates.value(),
                                                                  .extendedRates = extendedRates})))
            {
                NS_LOG_DEBUG("PEER_LINK_CONFIRM:  configuration mismatch");
                m_protocol->ConfigurationMismatch(m_ifIndex, peerAddress);
                m_stats.brokenMgt++;
                return false;
            }

            aid = peeringConfirmHeader.m_aid;

            auto meshConfig = peeringConfirmHeader.Get<IeConfiguration>();
            NS_ASSERT(meshConfig);
            config = meshConfig.value();

            peerManagement = peeringConfirmHeader.Get<IeMeshPeeringManagement>();

            m_stats.rxConfirm++;
        }
        else if (actionValue.selfProtectedAction == WifiActionHeader::PEER_LINK_CLOSE)
        {
            NS_LOG_DEBUG("Received PEER_LINK_CLOSE");
            MeshPeeringCloseHeader peeringCloseHeader;
            packet->RemoveHeader(peeringCloseHeader);

            // compare mesh id, drop if not equal
            auto meshId = peeringCloseHeader.Get<IeMeshId>();
            NS_ASSERT(meshId);
            if (!meshId.value().IsEqual(*(m_protocol->GetMeshId())))
            {
                NS_LOG_DEBUG("PEER_LINK_CLOSE:  configuration mismatch");
                m_protocol->ConfigurationMismatch(m_ifIndex, peerAddress);
                m_stats.brokenMgt++;
                return false;
            }

            peerManagement = peeringCloseHeader.Get<IeMeshPeeringManagement>();
            m_stats.rxClose++;
        }
        else
        {
            NS_FATAL_ERROR(
                "Unknown Self-protected Action type: " << actionValue.selfProtectedAction);
        }

        NS_ASSERT(peerManagement);
        // Deliver Peer link management frame to protocol:
        m_protocol->ReceivePeerLinkFrame(m_ifIndex,
                                         peerAddress,
                                         peerMpAddress,
                                         aid,
                                         actionValue.selfProtectedAction,
                                         peerManagement.value(),
                                         config);
        // if we can handle a frame - drop it
        return false;
    }
    return m_protocol->IsActiveLink(m_ifIndex, header.GetAddr2());
}

bool
PeerManagementProtocolMac::UpdateOutcomingFrame(Ptr<Packet> packet,
                                                WifiMacHeader& header,
                                                Mac48Address from,
                                                Mac48Address to)
{
    NS_LOG_FUNCTION(this << packet << header << from << to);
    if (header.IsAction())
    {
        WifiActionHeader actionHdr;
        packet->PeekHeader(actionHdr);
        if (actionHdr.GetCategory() == WifiActionHeader::SELF_PROTECTED)
        {
            return true;
        }
    }
    if (header.GetAddr1().IsGroup())
    {
        return true;
    }
    else
    {
        if (m_protocol->IsActiveLink(m_ifIndex, header.GetAddr1()))
        {
            return true;
        }
        else
        {
            m_stats.dropped++;
            return false;
        }
    }
}

void
PeerManagementProtocolMac::UpdateBeacon(MeshWifiBeacon& beacon) const
{
    if (m_protocol->GetBeaconCollisionAvoidance())
    {
        Ptr<IeBeaconTiming> beaconTiming = m_protocol->GetBeaconTimingElement(m_ifIndex);
        beacon.AddInformationElement(beaconTiming);
    }
    beacon.AddInformationElement(m_protocol->GetMeshId());
    m_protocol->NotifyBeaconSent(m_ifIndex, beacon.GetBeaconInterval());
}

void
PeerManagementProtocolMac::SendPeerLinkManagementFrame(
    Mac48Address peerAddress,
    Mac48Address peerMpAddress,
    uint16_t aid,
    WifiActionHeader::SelfProtectedActionValue actionFrameType,
    IeMeshPeeringManagement peerElement,
    IeConfiguration meshConfig)
{
    NS_LOG_FUNCTION(this << peerAddress << peerMpAddress);
    meshConfig.SetNeighborCount(m_protocol->GetNumberOfLinks());
    // Create an 802.11 frame header:
    // Send management frame to MAC:
    Ptr<Packet> packet = Create<Packet>();
    if (actionFrameType == WifiActionHeader::PEER_LINK_OPEN)
    {
        MeshPeeringOpenHeader peeringOpenHeader;
        peeringOpenHeader.m_capability = m_parent->GetCapabilities();
        auto allSupportedRates = m_parent->GetSupportedRates();
        peeringOpenHeader.Get<SupportedRates>() = allSupportedRates.rates;
        peeringOpenHeader.Get<ExtendedSupportedRatesIE>() = allSupportedRates.extendedRates;
        peeringOpenHeader.Get<IeMeshId>() = *(m_protocol->GetMeshId());
        peeringOpenHeader.Get<IeConfiguration>() = meshConfig;
        peeringOpenHeader.Get<IeMeshPeeringManagement>() = peerElement;

        WifiActionHeader actionHdr;
        m_stats.txOpen++;
        WifiActionHeader::ActionValue action;
        action.selfProtectedAction = WifiActionHeader::PEER_LINK_OPEN;
        actionHdr.SetAction(WifiActionHeader::SELF_PROTECTED, action);
        packet->AddHeader(peeringOpenHeader);
        packet->AddHeader(actionHdr);
    }
    if (actionFrameType == WifiActionHeader::PEER_LINK_CONFIRM)
    {
        MeshPeeringConfirmHeader peeringConfirmHeader;
        peeringConfirmHeader.m_capability = m_parent->GetCapabilities();
        peeringConfirmHeader.m_aid = aid;
        auto allSupportedRates = m_parent->GetSupportedRates();
        peeringConfirmHeader.Get<SupportedRates>() = allSupportedRates.rates;
        peeringConfirmHeader.Get<ExtendedSupportedRatesIE>() = allSupportedRates.extendedRates;
        peeringConfirmHeader.Get<IeMeshId>() = *(m_protocol->GetMeshId());
        peeringConfirmHeader.Get<IeConfiguration>() = meshConfig;
        peeringConfirmHeader.Get<IeMeshPeeringManagement>() = peerElement;

        WifiActionHeader actionHdr;
        WifiActionHeader::ActionValue action;
        action.selfProtectedAction = WifiActionHeader::PEER_LINK_CONFIRM;
        actionHdr.SetAction(WifiActionHeader::SELF_PROTECTED, action);
        packet->AddHeader(peeringConfirmHeader);
        packet->AddHeader(actionHdr);
        m_stats.txConfirm++;
    }
    if (actionFrameType == WifiActionHeader::PEER_LINK_CLOSE)
    {
        MeshPeeringCloseHeader peeringCloseHeader;
        peeringCloseHeader.Get<IeMeshId>() = *(m_protocol->GetMeshId());
        peeringCloseHeader.Get<IeMeshPeeringManagement>() = peerElement;
        WifiActionHeader actionHdr;
        WifiActionHeader::ActionValue action;
        action.selfProtectedAction = WifiActionHeader::PEER_LINK_CLOSE;
        actionHdr.SetAction(WifiActionHeader::SELF_PROTECTED, action);
        packet->AddHeader(peeringCloseHeader);
        packet->AddHeader(actionHdr);
        m_stats.txClose++;
    }
    m_stats.txMgt++;
    m_stats.txMgtBytes += packet->GetSize();
    // Wifi Mac header:
    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_MGT_ACTION);
    hdr.SetAddr1(peerAddress);
    hdr.SetAddr2(m_parent->GetAddress());
    // Addr is not used here, we use it as our MP address
    hdr.SetAddr3(m_protocol->GetAddress());
    hdr.SetDsNotFrom();
    hdr.SetDsNotTo();
    m_parent->SendManagementFrame(packet, hdr);
}

Mac48Address
PeerManagementProtocolMac::GetAddress() const
{
    if (m_parent)
    {
        return m_parent->GetAddress();
    }
    else
    {
        return Mac48Address();
    }
}

void
PeerManagementProtocolMac::SetBeaconShift(Time shift)
{
    if (!shift.IsZero())
    {
        m_stats.beaconShift++;
    }
    m_parent->ShiftTbtt(shift);
}

PeerManagementProtocolMac::Statistics::Statistics()
    : txOpen(0),
      txConfirm(0),
      txClose(0),
      rxOpen(0),
      rxConfirm(0),
      rxClose(0),
      dropped(0),
      brokenMgt(0),
      txMgt(0),
      txMgtBytes(0),
      rxMgt(0),
      rxMgtBytes(0),
      beaconShift(0)
{
}

void
PeerManagementProtocolMac::Statistics::Print(std::ostream& os) const
{
    os << "<Statistics "
          "txOpen=\""
       << txOpen << "\"" << std::endl
       << "txConfirm=\"" << txConfirm << "\"" << std::endl
       << "txClose=\"" << txClose << "\"" << std::endl
       << "rxOpen=\"" << rxOpen << "\"" << std::endl
       << "rxConfirm=\"" << rxConfirm << "\"" << std::endl
       << "rxClose=\"" << rxClose << "\"" << std::endl
       << "dropped=\"" << dropped << "\"" << std::endl
       << "brokenMgt=\"" << brokenMgt << "\"" << std::endl
       << "txMgt=\"" << txMgt << "\"" << std::endl
       << "txMgtBytes=\"" << txMgtBytes << "\"" << std::endl
       << "rxMgt=\"" << rxMgt << "\"" << std::endl
       << "rxMgtBytes=\"" << rxMgtBytes << "\"" << std::endl
       << "beaconShift=\"" << beaconShift << "\"/>" << std::endl;
}

void
PeerManagementProtocolMac::Report(std::ostream& os) const
{
    os << "<PeerManagementProtocolMac "
          "address=\""
       << m_parent->GetAddress() << "\">" << std::endl;
    m_stats.Print(os);
    os << "</PeerManagementProtocolMac>" << std::endl;
}

void
PeerManagementProtocolMac::ResetStats()
{
    m_stats = Statistics();
}

uint32_t
PeerManagementProtocolMac::GetLinkMetric(Mac48Address peerAddress)
{
    return m_parent->GetLinkMetric(peerAddress);
}

int64_t
PeerManagementProtocolMac::AssignStreams(int64_t stream)
{
    return m_protocol->AssignStreams(stream);
}

} // namespace dot11s
} // namespace ns3
