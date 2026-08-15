/*
 * Copyright (c) 2026 Somaiya Vidyavihar University, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manmita Das <das.manmita12@gmail.com>
 */

#include "ethernet-helper.h"

#include "ns3/abort.h"
#include "ns3/config.h"
#include "ns3/ethernet-channel.h"
#include "ns3/ethernet-mac.h"
#include "ns3/ethernet-net-device.h"
#include "ns3/iana-link-type-numbers.h"
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/trace-helper.h"

namespace ns3
{
namespace ethernet
{

NS_LOG_COMPONENT_DEFINE("EthernetHelper");

EthernetHelper::EthernetHelper()
{
    m_queueFactory.SetTypeId("ns3::DropTailQueue<Packet>");
    m_deviceFactory.SetTypeId("ns3::EthernetNetDevice");
    m_channelFactory.SetTypeId("ns3::EthernetChannel");
}

void
EthernetHelper::SetDeviceAttribute(std::string name, const AttributeValue& value)
{
    m_deviceFactory.Set(name, value);
}

void
EthernetHelper::SetChannelAttribute(std::string name, const AttributeValue& value)
{
    m_channelFactory.Set(name, value);
}

static void
PcapSniffEthernet(Ptr<PcapFileWrapper> file, Ptr<const Packet> packet)
{
    file->Write(Simulator::Now(), packet);
}

void
EthernetHelper::EnablePcapInternal(std::string prefix,
                                   Ptr<NetDevice> nd,
                                   bool promiscuous,
                                   bool explicitFilename)
{
    Ptr<EthernetNetDevice> device = nd->GetObject<EthernetNetDevice>();
    if (!device)
    {
        NS_LOG_INFO("Device " << device << " not of type ns3::EthernetNetDevice");
        return;
    }

    PcapHelper pcapHelper;

    std::string filename;
    if (explicitFilename)
    {
        filename = prefix;
    }
    else
    {
        filename = pcapHelper.GetFilenameFromDevice(prefix, device);
    }

    Ptr<PcapFileWrapper> file =
        pcapHelper.CreateFile(filename, std::ios::out, iana::linktype::ETHERNET);

    if (promiscuous)
    {
        device->GetMac()->TraceConnectWithoutContext("PromiscSniffer",
                                                     MakeBoundCallback(&PcapSniffEthernet, file));
    }
    else
    {
        device->GetMac()->TraceConnectWithoutContext("Sniffer",
                                                     MakeBoundCallback(&PcapSniffEthernet, file));
    }
}

void
EthernetHelper::EnableAsciiInternal(Ptr<OutputStreamWrapper> stream,
                                    std::string prefix,
                                    Ptr<NetDevice> nd,
                                    bool explicitFilename)
{
    Ptr<EthernetNetDevice> device = nd->GetObject<EthernetNetDevice>();
    if (!device)
    {
        NS_LOG_INFO("Device " << device << " not of type ns3::EthernetNetDevice");
        return;
    }

    Packet::EnablePrinting();

    if (!stream)
    {
        AsciiTraceHelper asciiTraceHelper;

        std::string filename;
        if (explicitFilename)
        {
            filename = prefix;
        }
        else
        {
            filename = asciiTraceHelper.GetFilenameFromDevice(prefix, device);
        }

        Ptr<OutputStreamWrapper> theStream = asciiTraceHelper.CreateFileStream(filename);

        asciiTraceHelper.HookDefaultReceiveSinkWithoutContext<EthernetMac>(device->GetMac(),
                                                                           "MacRx",
                                                                           theStream);

        Ptr<Queue<Packet>> rx_queue = device->GetMac()->GetRxQueue();
        asciiTraceHelper.HookDefaultEnqueueSinkWithoutContext<Queue<Packet>>(rx_queue,
                                                                             "Enqueue",
                                                                             theStream);
        asciiTraceHelper.HookDefaultDropSinkWithoutContext<Queue<Packet>>(rx_queue,
                                                                          "Drop",
                                                                          theStream);
        asciiTraceHelper.HookDefaultDequeueSinkWithoutContext<Queue<Packet>>(rx_queue,
                                                                             "Dequeue",
                                                                             theStream);

        Ptr<Queue<Packet>> tx_queue = device->GetMac()->GetTxQueue();
        asciiTraceHelper.HookDefaultEnqueueSinkWithoutContext<Queue<Packet>>(tx_queue,
                                                                             "Enqueue",
                                                                             theStream);
        asciiTraceHelper.HookDefaultDropSinkWithoutContext<Queue<Packet>>(tx_queue,
                                                                          "Drop",
                                                                          theStream);
        asciiTraceHelper.HookDefaultDequeueSinkWithoutContext<Queue<Packet>>(tx_queue,
                                                                             "Dequeue",
                                                                             theStream);
        return;
    }

    uint32_t nodeid = nd->GetNode()->GetId();
    uint32_t deviceid = nd->GetIfIndex();
    std::ostringstream oss;

    oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid
        << "/$ns3::EthernetNetDevice/Mac/$ns3::EthernetMac/MacRx";
    Config::Connect(oss.str(),
                    MakeBoundCallback(&AsciiTraceHelper::DefaultReceiveSinkWithContext, stream));

    oss.str("");
    oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid
        << "/$ns3::EthernetNetDevice/Mac/$ns3::EthernetMac/TxQueue/Enqueue";
    Config::Connect(oss.str(),
                    MakeBoundCallback(&AsciiTraceHelper::DefaultEnqueueSinkWithContext, stream));

    oss.str("");
    oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid
        << "/$ns3::EthernetNetDevice/Mac/$ns3::EthernetMac/TxQueue/Dequeue";
    Config::Connect(oss.str(),
                    MakeBoundCallback(&AsciiTraceHelper::DefaultDequeueSinkWithContext, stream));

    oss.str("");
    oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid
        << "/$ns3::EthernetNetDevice/Mac/$ns3::EthernetMac/TxQueue/Drop";
    Config::Connect(oss.str(),
                    MakeBoundCallback(&AsciiTraceHelper::DefaultDropSinkWithContext, stream));

    oss.str("");
    oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid
        << "/$ns3::EthernetNetDevice/Phy/$ns3::EthernetPhy/PhyRxDrop";
    Config::Connect(oss.str(),
                    MakeBoundCallback(&AsciiTraceHelper::DefaultDropSinkWithContext, stream));
}

NetDeviceContainer
EthernetHelper::Install(NodeContainer c) const
{
    NS_ABORT_MSG_IF(c.GetN() != 2,
                    "EthernetHelper::Install(NodeContainer) requires exactly two nodes");
    return Install(c.Get(0), c.Get(1));
}

NetDeviceContainer
EthernetHelper::Install(Ptr<Node> N1, Ptr<Node> N2) const
{
    NetDeviceContainer container;
    Ptr<EthernetChannel> channel = m_channelFactory.Create<EthernetChannel>();

    container.Add(InstallPriv(N1, channel));
    container.Add(InstallPriv(N2, channel));

    return container;
}

Ptr<EthernetNetDevice>
EthernetHelper::InstallPriv(Ptr<Node> node, Ptr<EthernetChannel> channel) const
{
    Ptr<EthernetNetDevice> device = m_deviceFactory.Create<EthernetNetDevice>();

    Ptr<Queue<Packet>> txQueue = m_queueFactory.Create<Queue<Packet>>();
    device->GetMac()->SetTxQueue(txQueue);
    device->GetMac()->SetRxQueue(m_queueFactory.Create<Queue<Packet>>());

    node->AddDevice(device);
    device->Attach(channel);

    return device;
}

} // namespace ethernet
} // namespace ns3
