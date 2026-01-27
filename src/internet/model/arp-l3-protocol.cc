/*
 * Copyright (c) 2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "arp-l3-protocol.h"

#include "arp-cache.h"
#include "arp-header.h"
#include "arp-queue-disc-item.h"
#include "ipv4-interface.h"
#include "ipv4-l3-protocol.h"

#include "ns3/log.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/object-vector.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/string.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/traffic-control-layer.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ArpL3Protocol");

const uint16_t ArpL3Protocol::PROT_NUMBER = 0x0806;

NS_OBJECT_ENSURE_REGISTERED(ArpL3Protocol);

TypeId
ArpL3Protocol::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ArpL3Protocol")
            .SetParent<Object>()
            .AddConstructor<ArpL3Protocol>()
            .SetGroupName("Internet")
            .AddAttribute("CacheList",
                          "The list of ARP caches",
                          ObjectVectorValue(),
                          MakeObjectVectorAccessor(&ArpL3Protocol::m_cacheList),
                          MakeObjectVectorChecker<ArpCache>())
            .AddAttribute("RequestJitter",
                          "The jitter in ms a node is allowed to wait "
                          "before sending an ARP request.  Some jitter aims "
                          "to prevent collisions. By default, the model "
                          "will wait for a duration in ms defined by "
                          "a uniform random-variable between 0 and RequestJitter",
                          StringValue("ns3::UniformRandomVariable[Min=0.0|Max=10.0]"),
                          MakePointerAccessor(&ArpL3Protocol::m_requestJitter),
                          MakePointerChecker<RandomVariableStream>())
            .AddTraceSource("Drop",
                            "Packet dropped because not enough room "
                            "in pending queue for a specific cache entry.",
                            MakeTraceSourceAccessor(&ArpL3Protocol::m_dropTrace),
                            "ns3::Packet::TracedCallback");
    return tid;
}

ArpL3Protocol::ArpL3Protocol()
    : m_tc(nullptr)
{
    NS_LOG_FUNCTION(this);
}

ArpL3Protocol::~ArpL3Protocol()
{
    NS_LOG_FUNCTION(this);
}

int64_t
ArpL3Protocol::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_requestJitter->SetStream(stream);
    return 1;
}

void
ArpL3Protocol::SetNode(Ptr<Node> node)
{
    NS_LOG_FUNCTION(this << node);
    m_node = node;
}

void
ArpL3Protocol::SetTrafficControl(Ptr<TrafficControlLayer> tc)
{
    NS_LOG_FUNCTION(this << tc);
    m_tc = tc;
}

/*
 * This method is called by AggregateObject and completes the aggregation
 * by setting the node in the ipv4 stack
 */
void
ArpL3Protocol::NotifyNewAggregate()
{
    NS_LOG_FUNCTION(this);
    if (!m_node)
    {
        Ptr<Node> node = this->GetObject<Node>();
        // verify that it's a valid node and that
        // the node was not set before
        if (node)
        {
            this->SetNode(node);
        }
    }
    Object::NotifyNewAggregate();
}

void
ArpL3Protocol::DoDispose()
{
    NS_LOG_FUNCTION(this);
    for (auto i = m_cacheList.begin(); i != m_cacheList.end(); ++i)
    {
        Ptr<ArpCache> cache = *i;
        cache->Dispose();
    }
    m_cacheList.clear();
    m_node = nullptr;
    m_tc = nullptr;
    Object::DoDispose();
}

Ptr<ArpCache>
ArpL3Protocol::CreateCache(Ptr<NetDevice> device, Ptr<Ipv4Interface> interface)
{
    NS_LOG_FUNCTION(this << device << interface);
    Ptr<Ipv4L3Protocol> ipv4 = m_node->GetObject<Ipv4L3Protocol>();
    Ptr<ArpCache> cache = CreateObject<ArpCache>();
    cache->SetDevice(device, interface);
    NS_ASSERT(device->IsBroadcast());
    device->AddLinkChangeCallback(MakeCallback(&ArpCache::Flush, cache));
    cache->SetArpRequestCallback(MakeCallback(&ArpL3Protocol::SendArpRequest, this));
    m_cacheList.push_back(cache);
    return cache;
}

Ptr<ArpCache>
ArpL3Protocol::FindCache(Ptr<NetDevice> device)
{
    NS_LOG_FUNCTION(this << device);
    for (auto i = m_cacheList.begin(); i != m_cacheList.end(); i++)
    {
        if ((*i)->GetDevice() == device)
        {
            return *i;
        }
    }
    NS_ASSERT(false);
    // quiet compiler
    return nullptr;
}

void
ArpL3Protocol::Receive(Ptr<NetDevice> device,
                       Ptr<const Packet> p,
                       uint16_t protocol,
                       const Address& from,
                       const Address& to,
                       NetDevice::PacketType packetType)
{
    NS_LOG_FUNCTION(this << device << p->GetSize() << protocol << from << to << packetType);

    Ptr<Packet> packet = p->Copy();

    NS_LOG_LOGIC("ARP: received packet of size " << packet->GetSize());

    Ptr<ArpCache> cache = FindCache(device);

    //
    // If we're connected to a real world network, then some of the fields sizes
    // in an ARP packet can vary in ways not seen in simulations.  We need to be
    // able to detect ARP packets with headers we don't recognize and not process
    // them instead of crashing.  The ArpHeader will return 0 if it can't deal
    // with the received header.
    //
    ArpHeader arp;
    uint32_t size = packet->RemoveHeader(arp);
    if (size == 0)
    {
        NS_LOG_LOGIC("ARP: Cannot remove ARP header");
        return;
    }
    NS_LOG_LOGIC("ARP: received " << (arp.IsRequest() ? "request" : "reply")
                                  << " node=" << m_node->GetId() << ", got "
                                  << (arp.IsRequest() ? "request" : "reply") << " from "
                                  << arp.GetSourceIpv4Address() << " for address "
                                  << arp.GetDestinationIpv4Address() << "; we have addresses: ");
    for (uint32_t i = 0; i < cache->GetInterface()->GetNAddresses(); i++)
    {
        NS_LOG_LOGIC(cache->GetInterface()->GetAddress(i).GetLocal() << ", ");
    }

    /**
     * @internal
     * Note: we do not update the ARP cache when we receive an ARP request
     *  from an unknown node. See \bugid{107}
     */
    bool found = false;
    for (uint32_t i = 0; i < cache->GetInterface()->GetNAddresses(); i++)
    {
        if (arp.IsRequest() &&
            arp.GetDestinationIpv4Address() == cache->GetInterface()->GetAddress(i).GetLocal())
        {
            found = true;
            NS_LOG_LOGIC("node=" << m_node->GetId() << ", got request from "
                                 << arp.GetSourceIpv4Address() << " -- send reply");
            SendArpReply(cache,
                         arp.GetDestinationIpv4Address(),
                         arp.GetSourceIpv4Address(),
                         arp.GetSourceHardwareAddress());
            break;
        }
        else if (arp.IsReply() &&
                 arp.GetDestinationIpv4Address() ==
                     cache->GetInterface()->GetAddress(i).GetLocal() &&
                 arp.GetDestinationHardwareAddress() == device->GetAddress())
        {
            found = true;
            Ipv4Address from = arp.GetSourceIpv4Address();
            ArpCache::Entry* entry = cache->Lookup(from);
            if (entry != nullptr)
            {
                if (entry->IsWaitReply())
                {
                    NS_LOG_LOGIC("node=" << m_node->GetId() << ", got reply from "
                                         << arp.GetSourceIpv4Address()
                                         << " for waiting entry -- flush");
                    Address from_mac = arp.GetSourceHardwareAddress();
                    entry->MarkAlive(from_mac);
                    ArpCache::Ipv4PayloadHeaderPair pending = entry->DequeuePending();
                    while (pending.first)
                    {
                        cache->GetInterface()->Send(pending.first,
                                                    pending.second,
                                                    arp.GetSourceIpv4Address());
                        pending = entry->DequeuePending();
                    }
                }
                else
                {
                    // ignore this reply which might well be an attempt
                    // at poisoning my arp cache.
                    NS_LOG_LOGIC("node=" << m_node->GetId() << ", got reply from "
                                         << arp.GetSourceIpv4Address()
                                         << " for non-waiting entry -- drop");
                    m_dropTrace(packet);
                }
            }
            else
            {
                NS_LOG_LOGIC("node=" << m_node->GetId() << ", got reply for unknown entry -- drop");
                m_dropTrace(packet);
            }
            break;
        }
    }
    if (!found)
    {
        NS_LOG_LOGIC("node=" << m_node->GetId() << ", got request from "
                             << arp.GetSourceIpv4Address() << " for unknown address "
                             << arp.GetDestinationIpv4Address() << " -- drop");
    }
}

bool
ArpL3Protocol::Lookup(Ptr<Packet> packet,
                      const Ipv4Header& ipHeader,
                      Ipv4Address destination,
                      Ptr<NetDevice> device,
                      Ptr<ArpCache> cache,
                      Address* hardwareDestination)
{
    NS_LOG_FUNCTION(this << packet << destination << device << cache << hardwareDestination);

    ArpCache::Entry* entry = cache->Lookup(destination);

    if (entry != nullptr)
    {
        //
        // 1. On-access Validation (Lazy State Transition)
        // Instead of using active timers for every entry, we validate the
        // entry's freshness whenever it is accessed. If it has exceeded its
        // lifetime, we initiate re-resolution.
        //
        if (entry->IsExpired())
        {
            if (entry->IsAlive() || entry->IsDead())
            {
                NS_LOG_LOGIC("node=" << m_node->GetId() << ", entry for " << destination
                                     << " expired -- initiating re-resolution");

                entry->MarkWaitReply(ArpCache::Ipv4PayloadHeaderPair(packet, ipHeader));
                Simulator::Schedule(MilliSeconds(m_requestJitter->GetValue()),
                                    &ArpL3Protocol::SendArpRequest,
                                    this,
                                    cache,
                                    destination);
                return false;
            }
        }

        //
        // 2. State-based Traffic Handling
        // Determine the fate of the packet based on the current validated state.
        //

        // Case: Entry is confirmed and valid for transmission
        if (entry->IsAlive() || entry->IsPermanent() || entry->IsAutoGenerated())
        {
            if (entry->IsStale())
            {
                entry->MarkDelay();
            }
            *hardwareDestination = entry->GetMacAddress();
            return true;
        }
        // Case: Resolution is currently in progress; queue the packet
        else if (entry->IsWaitReply())
        {
            NS_LOG_LOGIC("node=" << m_node->GetId() << ", resolution pending for " << destination
                                 << " -- buffering packet");
            if (!entry->UpdateWaitReply(ArpCache::Ipv4PayloadHeaderPair(packet, ipHeader)))
            {
                packet->AddHeader(ipHeader);
                m_dropTrace(packet);
            }
            return false;
        }
        //
        // Case: Previous Resolution Failed
        // A lookup on a failed entry now triggers a fresh resolution attempt
        // to allow for network recovery.
        //
        else if (entry->IsDead())
        {
            NS_LOG_LOGIC("node=" << m_node->GetId() << ", entry for " << destination
                                 << " was in FAILED state -- retrying resolution");

            entry->MarkWaitReply(ArpCache::Ipv4PayloadHeaderPair(packet, ipHeader));
            Simulator::Schedule(MilliSeconds(m_requestJitter->GetValue()),
                                &ArpL3Protocol::SendArpRequest,
                                this,
                                cache,
                                destination);
            return false;
        }
    }
    else
    {
        //
        // 3. New Destination Discovery
        // Create a new cache entry and initiate the standard ARP resolution process.
        //
        NS_LOG_LOGIC("node=" << m_node->GetId() << ", cache miss for " << destination
                             << " -- creating entry and sending request");

        entry = cache->Add(destination);
        entry->MarkWaitReply(ArpCache::Ipv4PayloadHeaderPair(packet, ipHeader));
        Simulator::Schedule(MilliSeconds(m_requestJitter->GetValue()),
                            &ArpL3Protocol::SendArpRequest,
                            this,
                            cache,
                            destination);
    }

    return false;
}

void
ArpL3Protocol::SendArpRequest(Ptr<const ArpCache> cache, Ipv4Address to)
{
    NS_LOG_FUNCTION(this << cache << to);
    ArpHeader arp;
    // need to pick a source address; use routing implementation to select
    Ptr<Ipv4L3Protocol> ipv4 = m_node->GetObject<Ipv4L3Protocol>();
    Ptr<NetDevice> device = cache->GetDevice();
    NS_ASSERT(device);
    Ptr<Packet> packet = Create<Packet>();
    Ipv4Address source = ipv4->SelectSourceAddress(device, to, Ipv4InterfaceAddress::GLOBAL);
    NS_LOG_LOGIC("ARP: sending request from node "
                 << m_node->GetId() << " || src: " << device->GetAddress() << " / " << source
                 << " || dst: " << device->GetBroadcast() << " / " << to);
    arp.SetRequest(device->GetAddress(), source, device->GetBroadcast(), to);
    NS_ASSERT(m_tc);
    m_tc->Send(device, Create<ArpQueueDiscItem>(packet, device->GetBroadcast(), PROT_NUMBER, arp));
}

void
ArpL3Protocol::SendArpReply(Ptr<const ArpCache> cache,
                            Ipv4Address myIp,
                            Ipv4Address toIp,
                            Address toMac)
{
    NS_LOG_FUNCTION(this << cache << myIp << toIp << toMac);
    ArpHeader arp;
    NS_LOG_LOGIC("ARP: sending reply from node "
                 << m_node->GetId() << "|| src: " << cache->GetDevice()->GetAddress() << " / "
                 << myIp << " || dst: " << toMac << " / " << toIp);
    arp.SetReply(cache->GetDevice()->GetAddress(), myIp, toMac, toIp);
    Ptr<Packet> packet = Create<Packet>();
    NS_ASSERT(m_tc);
    m_tc->Send(cache->GetDevice(), Create<ArpQueueDiscItem>(packet, toMac, PROT_NUMBER, arp));
}

} // namespace ns3
