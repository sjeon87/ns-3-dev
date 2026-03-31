/*
 * Copyright (c) 2010 Georgia Institute of Technology
 * Copyright (c) 2020 Harsha Sharma : Flent application
 * Copyright (c) 2021 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 *
 * Author: Harsha Sharma <harshasha256@gmail.com>
 *         (adapted from bulk-send-application.cc and
 *          packet-sink-application.cc written by George F. Riley)
 *
 * Modified by: Ameya Deshpande <ameyanrd@outlook.com>
 *              Bhaskar Kataria <bhaskar.k7920@gmail.com> (Post processing of raw data)
 */

#include "flent-application.h"

#include "ns3/address.h"
#include "ns3/application-container.h"
#include "ns3/core-module.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/loopback-net-device.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/trace-helper.h"

#include <chrono>
#include <iostream>

namespace ns3
{

class SeqTsEchoHeader;

NS_LOG_COMPONENT_DEFINE("FlentApplication");

NS_OBJECT_ENSURE_REGISTERED(FlentApplication);

namespace
{

std::vector<uint32_t> m_bytesSent{std::vector<uint32_t>(4, 0)};     //!< sent data counters
std::vector<uint32_t> m_bytesReceived{std::vector<uint32_t>(4, 0)}; //!< receive data counters

/*
 * @brief sink for packet transmissions.
 * @param counter counter of bytes sent
 * @param packet Pointer to packet sent
 */
void
TraceSentPacket(uint32_t* counter, Ptr<const Packet> packet)
{
    *counter += packet->GetSize();
}

/*
 * @brief sink for packet received.
 * @param counter counter of bytes received
 * @param packet Pointer to packet received
 * @param address Address of the sender
 */
void
TraceReceivedPacket(uint32_t* counter, Ptr<const Packet> packet, const Address& address)
{
    *counter += packet->GetSize();
}

} // anonymous namespace

TypeId
FlentApplication::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::FlentApplication")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddConstructor<FlentApplication>()
            .AddAttribute("TestName",
                          "Name of the flent test to be run",
                          StringValue(""),
                          MakeStringAccessor(&FlentApplication::m_testName),
                          MakeStringChecker())
            .AddAttribute("Length",
                          "Test length",
                          TimeValue(Seconds(60)),
                          MakeTimeAccessor(&FlentApplication::m_length),
                          MakeTimeChecker())
            .AddAttribute("HostAddress",
                          "The address of the remote host",
                          AddressValue(),
                          MakeAddressAccessor(&FlentApplication::m_hostAddress),
                          MakeAddressChecker())
            .AddAttribute("LocalBindAddress",
                          "The address of the local host",
                          AddressValue(),
                          MakeAddressAccessor(&FlentApplication::m_localBindAddress),
                          MakeAddressChecker())
            .AddAttribute("ImageText",
                          "Text to be included in the plot",
                          StringValue(""),
                          MakeStringAccessor(&FlentApplication::m_imageText),
                          MakeStringChecker())
            .AddAttribute("ImageName",
                          "Name of the image to save the output plot",
                          StringValue(""),
                          MakeStringAccessor(&FlentApplication::m_imageName),
                          MakeStringChecker())
            .AddAttribute("StepSize",
                          "Measurement data point size",
                          TimeValue(MilliSeconds(200)),
                          MakeTimeAccessor(&FlentApplication::m_stepSize),
                          MakeTimeChecker(MilliSeconds(50), Seconds(1)));
    return tid;
}

FlentApplication::FlentApplication()
{
    NS_LOG_FUNCTION(this);
}

FlentApplication::~FlentApplication()
{
    NS_LOG_FUNCTION(this);
}

void
FlentApplication::DoInitialize(void)
{
    NS_LOG_FUNCTION(this);

    m_hostNode = GetHostNode(Ipv4Address::ConvertFrom(m_hostAddress));

    if (m_localBindAddress.IsInvalid())
    {
        Ptr<Ipv4L3Protocol> ip = m_node->GetObject<Ipv4L3Protocol>();
        if (ip)
        {
            for (uint32_t deviceId = 0; deviceId < m_node->GetNDevices(); deviceId++)
            {
                Ptr<NetDevice> device = m_node->GetDevice(deviceId);
                // If this is not a loopback device add the IP address to the map
                if (!DynamicCast<LoopbackNetDevice>(device))
                {
                    int32_t interfaceIndex = (ip)->GetInterfaceForDevice(device);
                    if (interfaceIndex != -1)
                    {
                        m_localBindAddress = ip->GetAddress(interfaceIndex, 0).GetLocal();
                        break;
                    }
                }
            }
        }
    }

    // Override the Stop Time set for the Application Container
    m_stopTime = m_startTime + m_length + Seconds(10);

    Application::DoInitialize();
}

void
FlentApplication::DoDispose(void)
{
    NS_LOG_FUNCTION(this);

    // chain up
    Application::DoDispose();
}

std::string
FlentApplication::GetUtcFormatTime(int sec) const
{
    time_t now = time(0) + sec;
    struct tm tstruct;
    char buf[80];
    tstruct = *gmtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%dT%X.000000Z", &tstruct);

    return buf;
}

void
FlentApplication::AddMetadata(Json::Value& j)
{
    j["metadata"]["BATCH_NAME"] = Json::Value::null;
    j["metadata"]["BATCH_TIME"] = Json::Value::null;
    j["metadata"]["BATCH_TITLE"] = Json::Value::null;
    j["metadata"]["BATCH_UUID"] = Json::Value::null;
    std::string filename = (m_testName + "-" + m_imageText + ".flent");
    j["metadata"]["DATA_FILENAME"] = filename;
    j["metadata"]["EGRESS_INFO"]["bql"]["tx-0"] = "";
    j["metadata"]["classes"] = Json::Value::null;
    j["metadata"]["driver"] = Json::Value::null;
    j["metadata"]["iface"] = Json::Value::null;
    j["metadata"]["link_params"]["qlen"] = Json::Value::null;
    j["metadata"]["offloads"]["generic-receive-offload"] = Json::Value::null;
    j["metadata"]["offloads"]["generic-segmentation-offload"] = Json::Value::null;
    j["metadata"]["offloads"]["large-receive-offload"] = Json::Value::null;
    j["metadata"]["offloads"]["tcp-segmentation"] = Json::Value::null;
    j["metadata"]["offloads"]["udp-fragmentation"] = Json::Value::null;
    j["metadata"]["qdiscs"]["id"] = Json::Value::null;
    j["metadata"]["qdiscs"]["name"] = Json::Value::null;
    j["metadata"]["qdiscs"]["params"]["ecn"] = Json::Value::null;
    j["metadata"]["qdiscs"]["params"]["flows"] = Json::Value::null;
    j["metadata"]["qdiscs"]["params"]["interval"] = Json::Value::null;
    j["metadata"]["qdiscs"]["params"]["limit"] = Json::Value::null;
    j["metadata"]["qdiscs"]["params"]["memory_limit"] = Json::Value::null;
    j["metadata"]["qdiscs"]["params"]["quantum"] = Json::Value::null;
    j["metadata"]["qdiscs"]["params"]["refcnt"] = Json::Value::null;
    j["metadata"]["qdiscs"]["params"]["target"] = Json::Value::null;
    j["metadata"]["qdiscs"]["parent"] = Json::Value::null;
    j["metadata"]["FAILED_RUNNERS"] = Json::Value::null;
    j["metadata"]["FLENT_VERSION"] = Json::Value::null;
    std::ostringstream oss;
    oss << m_hostAddress;
    std::string hostName = oss.str();
    j["metadata"]["HOST"] = hostName;
    j["metadata"]["HOSTS"] = Json::Value(Json::arrayValue);
    j["metadata"]["HOSTS"].append(hostName);
    j["metadata"]["HTTP_GETTER_DNS"] = Json::Value::null;
    j["metadata"]["HTTP_GETTER_URLLIST"] = Json::Value::null;
    j["metadata"]["HTTP_GETTER_WORKERS"] = Json::Value::null;
    j["metadata"]["IP_VERSION"] = Json::Value::null;
    j["metadata"]["KERNEL_NAME"] = Json::Value::null;
    j["metadata"]["KERNEL_RELEASE"] = Json::Value::null;
    j["metadata"]["LENGTH"] = m_length.GetSeconds();
    j["metadata"]["LOCAL_HOST"] = Json::Value::null;
    j["metadata"]["MODULE_VERSIONS"] = Json::Value::null;
    j["metadata"]["NAME"] = m_testName;
    j["metadata"]["NOTE"] = Json::Value::null;
    j["metadata"]["REMOTE_METADATA"] = Json::Value::null;
    j["metadata"]["STEP_SIZE"] = m_stepSize.GetSeconds();
    j["metadata"]["TIME"] = GetUtcFormatTime(0);
    j["metadata"]["T0"] = GetUtcFormatTime(0);
    j["metadata"]["TEST_PARAMETERS"] = Json::objectValue;
    j["metadata"]["TITLE"] = Json::Value::null;
    j["metadata"]["TOTAL_LENGTH"] = m_stopTime.GetSeconds();
    j["version"] = 4;
}

Ptr<Node>
FlentApplication::GetHostNode(Ipv4Address hostAddress) const
{
    NS_LOG_FUNCTION(this << hostAddress);

    for (NodeList::Iterator it = NodeList::Begin(); it != NodeList::End(); ++it)
    {
        Ptr<Node> node = *it;
        Ptr<Ipv4L3Protocol> ip = node->GetObject<Ipv4L3Protocol>();

        if (ip)
        {
            for (uint32_t deviceId = 0; deviceId < node->GetNDevices(); deviceId++)
            {
                int32_t interfaceIndex = (ip)->GetInterfaceForDevice(node->GetDevice(deviceId));
                if (interfaceIndex != -1)
                {
                    uint32_t numberOfAddresses = ip->GetNAddresses(interfaceIndex);
                    for (uint32_t addressIndex = 0; addressIndex < numberOfAddresses;
                         addressIndex++)
                    {
                        Ipv4InterfaceAddress ifAddr = ip->GetAddress(interfaceIndex, addressIndex);
                        Ipv4Address addr = ifAddr.GetAddress();

                        if (addr == hostAddress)
                        {
                            return node;
                        }
                    }
                }
            }
        }
    }

    NS_LOG_ERROR("Couldn't find dest node given the IP" << hostAddress);
    return 0;
}

void
FlentApplication::TraceReceivedPing(const Address& address, uint16_t seq, uint8_t ttl, Time t)
{
    Json::Value data;
    double rtt = t.GetSeconds() * 1000;
    data["seq"] = seq;
    data["t"] = (Simulator::Now().GetSeconds() + m_currTime);
    data["val"] = rtt;
    m_output["raw_values"]["Ping (ms) ICMP"].append(data);
}

void
FlentApplication::TraceReceivedUdpPing1(Ptr<const Packet> packet,
                                        const Address& address,
                                        const Address& localAddress,
                                        const SeqTsEchoHeader& header)
{
    Json::Value data;
    Time t = header.GetTsValue();
    double rtt = t.GetSeconds() * 1000;
    data["dur"] = m_stepSize.GetSeconds();
    data["t"] = (Simulator::Now().GetSeconds() + m_currTime);
    data["val"] = rtt;
    m_output["raw_values"]["Ping (ms) UDP BE"].append(data);
}

void
FlentApplication::TraceReceivedUdpPing2(Ptr<const Packet> packet,
                                        const Address& address,
                                        const Address& localAddress,
                                        const SeqTsEchoHeader& header)
{
    Json::Value data;
    Time t = header.GetTsValue();
    double rtt = t.GetSeconds() * 1000;
    data["dur"] = m_stepSize.GetSeconds();
    data["t"] = (Simulator::Now().GetSeconds() + m_currTime);
    data["val"] = rtt;
    m_output["raw_values"]["Ping (ms) UDP BK"].append(data);
}

void
FlentApplication::TraceReceivedUdpPing3(Ptr<const Packet> packet,
                                        const Address& address,
                                        const Address& localAddress,
                                        const SeqTsEchoHeader& header)
{
    Json::Value data;
    Time t = header.GetTsValue();
    double rtt = t.GetSeconds() * 1000;
    data["dur"] = m_stepSize.GetSeconds();
    data["t"] = (Simulator::Now().GetSeconds() + m_currTime);
    data["val"] = rtt;
    m_output["raw_values"]["Ping (ms) UDP EF"].append(data);
}

void
FlentApplication::GoodputSamplingUpload(std::string name, int i)
{
    Json::Value data;
    double goodput = (m_bytesSent[i] * 8 / m_stepSize.GetSeconds() / 1e6);
    data["dur"] = m_stepSize.GetSeconds();
    data["t"] = (Simulator::Now().GetSeconds() + m_currTime);
    data["val"] = goodput;
    m_output["raw_values"][name].append(data);
    m_bytesSent[i] = 0;
    Simulator::Schedule(m_stepSize, &FlentApplication::GoodputSamplingUpload, this, name, i);
}

void
FlentApplication::GoodputSamplingDownload(std::string name, int i)
{
    Json::Value data;
    double goodput = (m_bytesReceived[i] * 8 / m_stepSize.GetSeconds() / 1e6);
    data["dur"] = m_stepSize.GetSeconds();
    data["t"] = (Simulator::Now().GetSeconds() + m_currTime);
    data["val"] = goodput;
    m_output["raw_values"][name].append(data);
    m_bytesReceived[i] = 0;
    Simulator::Schedule(m_stepSize, &FlentApplication::GoodputSamplingDownload, this, name, i);
}

void
FlentApplication::FillXValues(void)
{
    NS_LOG_DEBUG("Filling x values");
    double stepSize = m_stepSize.GetSeconds();
    NS_LOG_DEBUG(stepSize);
    for (int step = 0; step < int(std::ceil(m_stopTime.GetSeconds() / m_stepSize.GetSeconds()));
         step += 1)
    {
        NS_LOG_DEBUG(step);
        m_output["x_values"].append(std::to_string(step * stepSize)
                                        .substr(0, std::to_string(step * stepSize).find(".") + 3));
    }
}

void
FlentApplication::ProcessRawValues(void)
{
    NS_LOG_DEBUG("Process Raw values");
    int steps = int(std::ceil(m_stopTime.GetSeconds() / m_stepSize.GetSeconds()));
    for (int s = 0; s < steps; s++)
    {
        double t = m_currTime + (m_stepSize.GetSeconds() * s);
        for (auto itrName = m_output["raw_values"].begin(); itrName != m_output["raw_values"].end();
             itrName++)
        {
            std::string rawValueName = itrName.key().asString();

            // --- THE SAFETY GUARD ---
            // If the array is empty, log null and skip the dangerous iterator math
            if (m_output["raw_values"][rawValueName].size() == 0)
            {
                m_output["results"][rawValueName].append(Json::Value::null);
                continue;
            }
            // ------------------------

            double maxDist = m_stepSize.GetSeconds() * 5.0;
            if (!(*itrName))
            {
                continue;
            }
            double tPrev = 0.0;
            double vPrev = 0.0;
            double tNext = 0.0;
            double vNext = 0.0;
            for (auto itrValues = m_output["raw_values"][rawValueName].begin(),
                      prev = --(m_output["raw_values"][rawValueName].end());
                 itrValues != m_output["raw_values"][rawValueName].end();
                 itrValues++, prev = itrValues)
            {
                if ((*itrValues)["t"].asDouble() > t)
                {
                    if (itrValues == m_output["raw_values"][rawValueName].begin())
                    {
                        tPrev = (*prev)["t"].asDouble();
                        vPrev = (*prev)["val"].asDouble();
                    }
                    else
                    {
                        maxDist = m_stepSize.GetSeconds() * 0.5;
                    }
                    tNext = (*itrValues)["t"].asDouble();
                    vNext = (*itrValues)["val"].asDouble();
                    break;
                }
            }
            bool last = false;
            if (tNext == 0)
            {
                tNext = (*(--(m_output["raw_values"][rawValueName].end())))["t"].asDouble();
                vNext = (*(--(m_output["raw_values"][rawValueName].end())))["val"].asDouble();
                last = true;
            }
            if (std::abs(t - tNext) <= maxDist)
            {
                if (tPrev == 0)
                {
                    auto itrResult = m_output["results"][rawValueName].end();
                    if (last && ((*(itrResult) == vNext) || *(itrResult) == Json::Value::null))
                    {
                        m_output["results"][rawValueName].append(Json::Value::null);
                    }
                    else
                    {
                        m_output["results"][rawValueName].append(vNext);
                    }
                }
                else
                {
                    double dvDt = (vNext - vPrev) / (tNext - tPrev);
                    m_output["results"][rawValueName].append(vPrev + dvDt * (t - tPrev));
                }
            }
            else
            {
                m_output["results"][rawValueName].append(Json::Value::null);
            }
        }
    }
}

// Application Methods
void
FlentApplication::StartApplication(void) // Called at time specified by Start
{
    NS_LOG_FUNCTION(this);
    m_currTime = (std::chrono::duration_cast<std::chrono::nanoseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count() /
                  1000000000);
    AddMetadata(m_output);

    if (m_testName.compare("ping") == 0)
    {
        Ipv4Address hostAddr = Ipv4Address::ConvertFrom(m_hostAddress);
        m_v4ping = CreateObject<Ping>();
        m_v4ping->SetAttribute("Remote", Ipv4AddressValue(hostAddr));
        m_v4ping->SetAttribute("Interval", TimeValue(m_stepSize));
        m_node->AddApplication(m_v4ping);
        m_output["raw_values"]["Ping (ms) ICMP"] = Json::Value(Json::arrayValue);
        m_output["results"]["Ping (ms) ICMP"] = Json::Value(Json::arrayValue);
        m_output["x_values"] = Json::Value(Json::arrayValue);

        m_v4ping->TraceConnectWithoutContext(
            "Rx",
            MakeCallback(&FlentApplication::TraceReceivedPing, this));
    }
    else if (m_testName.compare("tcp_upload") == 0)
    {
        Ipv4Address hostAddr = Ipv4Address::ConvertFrom(m_hostAddress);
        m_v4ping = CreateObject<Ping>();
        m_v4ping->SetAttribute("Remote", Ipv4AddressValue(hostAddr));
        m_v4ping->SetAttribute("Interval", TimeValue(m_stepSize));
        m_node->AddApplication(m_v4ping);
        ApplicationContainer pingContainer;
        pingContainer.Add(m_v4ping);
        pingContainer.Start(m_startTime);
        pingContainer.Stop(m_stopTime);

        m_output["raw_values"]["Ping (ms) ICMP"] = Json::Value(Json::arrayValue);
        m_output["results"]["Ping (ms) ICMP"] = Json::Value(Json::arrayValue);
        m_output["x_values"] = Json::Value(Json::arrayValue);

        m_v4ping->TraceConnectWithoutContext(
            "Rx",
            MakeCallback(&FlentApplication::TraceReceivedPing, this));

        InetSocketAddress clientAddress = InetSocketAddress(hostAddr, 9);
        m_bulkSendUp[0] = CreateObject<BulkSendApplication>();
        m_bulkSendUp[0]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_bulkSendUp[0]->SetAttribute("Remote", AddressValue(clientAddress));
        m_bulkSendUp[0]->SetAttribute("MaxBytes", UintegerValue(0));
        m_node->AddApplication(m_bulkSendUp[0]);
        ApplicationContainer sourceApp;
        sourceApp.Add(m_bulkSendUp[0]);
        sourceApp.Start(m_startTime + Seconds(5));
        sourceApp.Stop(m_stopTime - Seconds(5));
        m_output["results"]["TCP upload"] = Json::Value(Json::arrayValue);
        m_output["raw_values"]["TCP upload"] = Json::Value(Json::arrayValue);
        Json::Value data;
        data["dur"] = m_stepSize.GetSeconds();
        data["t"] = (Simulator::Now().GetSeconds() + m_currTime);
        data["val"] = 0;
        m_output["raw_values"]["TCP upload"].append(data);
        m_bulkSendUp[0]->TraceConnectWithoutContext(
            "Tx",
            MakeBoundCallback(&TraceSentPacket, &m_bytesSent[0]));
        Simulator::Schedule(m_stepSize,
                            &FlentApplication::GoodputSamplingUpload,
                            this,
                            "TCP upload",
                            0);

        Address sinkAddress(InetSocketAddress(Ipv4Address::GetAny(), 9));
        m_packetSinkUp[0] = CreateObject<PacketSink>();
        m_packetSinkUp[0]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_packetSinkUp[0]->SetAttribute("Local", AddressValue(sinkAddress));
        m_hostNode->AddApplication(m_packetSinkUp[0]);
        ApplicationContainer sinkApp;
        sinkApp.Add(m_packetSinkUp[0]);
        sinkApp.Start(m_startTime + Seconds(5));
        sinkApp.Stop(m_stopTime - Seconds(5));
    }
    else if (m_testName.compare("tcp_download") == 0)
    {
        Ipv4Address localBindAddr = Ipv4Address::ConvertFrom(m_localBindAddress);
        m_v4ping = CreateObject<Ping>();
        m_v4ping->SetAttribute("Remote", Ipv4AddressValue(localBindAddr));
        m_v4ping->SetAttribute("Interval", TimeValue(m_stepSize));
        m_hostNode->AddApplication(m_v4ping);
        ApplicationContainer pingContainer;
        pingContainer.Add(m_v4ping);
        pingContainer.Start(m_startTime);
        pingContainer.Stop(m_stopTime);

        m_output["raw_values"]["Ping (ms) ICMP"] = Json::Value(Json::arrayValue);
        m_output["results"]["Ping (ms) ICMP"] = Json::Value(Json::arrayValue);
        m_output["x_values"] = Json::Value(Json::arrayValue);

        m_v4ping->TraceConnectWithoutContext(
            "Rx",
            MakeCallback(&FlentApplication::TraceReceivedPing, this));
        Address sinkAddress(InetSocketAddress(Ipv4Address::GetAny(), 9));
        m_packetSinkDown[0] = CreateObject<PacketSink>();
        m_packetSinkDown[0]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_packetSinkDown[0]->SetAttribute("Local", AddressValue(sinkAddress));
        m_node->AddApplication(m_packetSinkDown[0]);
        ApplicationContainer sinkApp;
        sinkApp.Add(m_packetSinkDown[0]);
        sinkApp.Start(m_startTime + Seconds(5));
        sinkApp.Stop(m_stopTime - Seconds(5));
        m_packetSinkDown[0]->TraceConnectWithoutContext(
            "Rx",
            MakeBoundCallback(&TraceReceivedPacket, &m_bytesReceived[0]));
        m_output["results"]["TCP download"] = Json::Value(Json::arrayValue);
        m_output["raw_values"]["TCP download"] = Json::Value(Json::arrayValue);
        Json::Value data;
        data["dur"] = m_stepSize.GetSeconds();
        data["t"] = (Simulator::Now().GetSeconds() + m_currTime);
        data["val"] = 0;
        m_output["raw_values"]["TCP download"].append(data);
        Simulator::Schedule(m_stepSize,
                            &FlentApplication::GoodputSamplingDownload,
                            this,
                            "TCP download",
                            0);

        InetSocketAddress localBindAddress = InetSocketAddress(localBindAddr, 9);
        m_bulkSendDown[0] = CreateObject<BulkSendApplication>();
        m_bulkSendDown[0]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_bulkSendDown[0]->SetAttribute("Remote", AddressValue(localBindAddress));
        m_bulkSendDown[0]->SetAttribute("MaxBytes", UintegerValue(0));
        m_hostNode->AddApplication(m_bulkSendDown[0]);
        ApplicationContainer sourceApp;
        sourceApp.Add(m_bulkSendDown[0]);
        sourceApp.Start(m_startTime + Seconds(5));
        sourceApp.Stop(m_stopTime - Seconds(5));
    }
    else if (m_testName.compare("rrul") == 0)
    {
        Ipv4Address hostIpv4Address = Ipv4Address::ConvertFrom(m_hostAddress);
        Ipv4Address localIpv4Address = Ipv4Address::ConvertFrom(m_localBindAddress);

        m_v4ping = CreateObject<Ping>();
        m_v4ping->SetAttribute("Remote", Ipv4AddressValue(hostIpv4Address));
        m_v4ping->SetAttribute("Interval", TimeValue(m_stepSize));
        m_node->AddApplication(m_v4ping);
        ApplicationContainer pingContainer;
        pingContainer.Add(m_v4ping);
        pingContainer.Start(m_startTime);
        pingContainer.Stop(m_stopTime);

        m_output["raw_values"]["Ping (ms) ICMP"] = Json::Value(Json::arrayValue);
        m_output["results"]["Ping (ms) ICMP"] = Json::Value(Json::arrayValue);
        m_output["x_values"] = Json::Value(Json::arrayValue);
        m_v4ping->TraceConnectWithoutContext(
            "Rx",
            MakeCallback(&FlentApplication::TraceReceivedPing, this));

        uint16_t port = 9;
        m_udpserver[0] = CreateObject<UdpEchoServer>();
        m_udpserver[0]->SetAttribute("Port", UintegerValue(port));
        m_udpserver[0]->SetAttribute("EnableSeqTsEchoHeader", BooleanValue(true));
        m_hostNode->AddApplication(m_udpserver[0]);
        ApplicationContainer apps;
        apps.Add(m_udpserver[0]);
        apps.Start(m_startTime);
        apps.Stop(m_stopTime);
        uint32_t packetSize = 1024;
        uint32_t maxPacketCount = 10000;
        m_udpclient[0] = CreateObject<UdpEchoClient>();
        m_udpclient[0]->SetAttribute("Remote", AddressValue(hostIpv4Address));
        m_udpclient[0]->SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
        m_udpclient[0]->SetAttribute("Interval", TimeValue(m_stepSize));
        m_udpclient[0]->SetAttribute("PacketSize", UintegerValue(packetSize));
        m_udpclient[0]->SetAttribute("EnableSeqTsEchoHeader", BooleanValue(true));
        m_node->AddApplication(m_udpclient[0]);
        ApplicationContainer apps2;
        apps2.Add(m_udpclient[0]);
        apps2.Start(m_startTime);
        apps2.Stop(m_stopTime);
        m_output["raw_values"]["Ping (ms) UDP BE"] = Json::Value(Json::arrayValue);
        m_output["results"]["Ping (ms) UDP BE"] = Json::Value(Json::arrayValue);
        m_udpclient[0]->TraceConnectWithoutContext(
            "RxWithSeqTsEchoHeader",
            MakeCallback(&FlentApplication::TraceReceivedUdpPing1, this));

        port = 10;
        m_udpserver[1] = CreateObject<UdpEchoServer>();
        m_udpserver[1]->SetAttribute("Port", UintegerValue(port));
        m_udpserver[1]->SetAttribute("EnableSeqTsEchoHeader", BooleanValue(true));
        m_hostNode->AddApplication(m_udpserver[1]);
        ApplicationContainer apps3;
        apps3.Add(m_udpserver[1]);
        apps3.Start(m_startTime);
        apps3.Stop(m_stopTime);
        m_udpclient[1] = CreateObject<UdpEchoClient>();
        m_udpclient[1]->SetAttribute("Remote", AddressValue(hostIpv4Address));
        m_udpclient[1]->SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
        m_udpclient[1]->SetAttribute("Interval", TimeValue(m_stepSize));
        m_udpclient[1]->SetAttribute("PacketSize", UintegerValue(packetSize));
        m_udpclient[1]->SetAttribute("EnableSeqTsEchoHeader", BooleanValue(true));
        m_node->AddApplication(m_udpclient[1]);
        ApplicationContainer apps4;
        apps4.Add(m_udpclient[1]);
        apps4.Start(m_startTime);
        apps4.Stop(m_stopTime);
        m_output["raw_values"]["Ping (ms) UDP BK"] = Json::Value(Json::arrayValue);
        m_output["results"]["Ping (ms) UDP BK"] = Json::Value(Json::arrayValue);
        m_udpclient[1]->TraceConnectWithoutContext(
            "RxWithSeqTsEchoHeader",
            MakeCallback(&FlentApplication::TraceReceivedUdpPing2, this));

        port = 11;
        m_udpserver[2] = CreateObject<UdpEchoServer>();
        m_udpserver[2]->SetAttribute("Port", UintegerValue(port));
        m_udpserver[2]->SetAttribute("EnableSeqTsEchoHeader", BooleanValue(true));
        m_hostNode->AddApplication(m_udpserver[2]);
        ApplicationContainer apps5;
        apps5.Add(m_udpserver[2]);
        apps5.Start(m_startTime);
        apps5.Stop(m_stopTime);
        m_udpclient[2] = CreateObject<UdpEchoClient>();
        m_udpclient[2]->SetAttribute("Remote", AddressValue(hostIpv4Address));
        m_udpclient[2]->SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
        m_udpclient[2]->SetAttribute("Interval", TimeValue(m_stepSize));
        m_udpclient[2]->SetAttribute("PacketSize", UintegerValue(packetSize));
        m_udpclient[2]->SetAttribute("EnableSeqTsEchoHeader", BooleanValue(true));
        m_node->AddApplication(m_udpclient[2]);
        ApplicationContainer apps6;
        apps6.Add(m_udpclient[2]);
        apps6.Start(m_startTime);
        apps6.Stop(m_stopTime);
        m_output["raw_values"]["Ping (ms) UDP EF"] = Json::Value(Json::arrayValue);
        m_output["results"]["Ping (ms) UDP EF"] = Json::Value(Json::arrayValue);
        m_udpclient[2]->TraceConnectWithoutContext(
            "RxWithSeqTsEchoHeader",
            MakeCallback(&FlentApplication::TraceReceivedUdpPing3, this));

        // Download BE
        Address sinkAddress(InetSocketAddress(Ipv4Address::GetAny(), 10));
        m_packetSinkDown[0] = CreateObject<PacketSink>();
        m_packetSinkDown[0]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_packetSinkDown[0]->SetAttribute("Local", AddressValue(sinkAddress));
        m_node->AddApplication(m_packetSinkDown[0]);
        ApplicationContainer sinkApp;
        sinkApp.Add(m_packetSinkDown[0]);
        sinkApp.Start(m_startTime + Seconds(5));
        sinkApp.Stop(m_stopTime - Seconds(5));
        m_packetSinkDown[0]->TraceConnectWithoutContext(
            "Rx",
            MakeBoundCallback(&TraceReceivedPacket, &m_bytesReceived[0]));
        m_output["results"]["TCP download BE"] = Json::Value(Json::arrayValue);
        m_output["raw_values"]["TCP download BE"] = Json::Value(Json::arrayValue);
        Json::Value data;
        data["dur"] = m_stepSize.GetSeconds();
        data["t"] = (Simulator::Now().GetSeconds() + m_currTime);
        data["val"] = 0;
        m_output["raw_values"]["TCP download BE"].append(data);
        Simulator::Schedule(m_stepSize,
                            &FlentApplication::GoodputSamplingDownload,
                            this,
                            "TCP download BE",
                            0);
        InetSocketAddress localBindAddress = InetSocketAddress(localIpv4Address, 10);
        m_bulkSendDown[0] = CreateObject<BulkSendApplication>();
        m_bulkSendDown[0]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_bulkSendDown[0]->SetAttribute("Remote", AddressValue(localBindAddress));
        m_bulkSendDown[0]->SetAttribute("MaxBytes", UintegerValue(0));
        m_hostNode->AddApplication(m_bulkSendDown[0]);
        ApplicationContainer sourceApp;
        sourceApp.Add(m_bulkSendDown[0]);
        sourceApp.Start(m_startTime + Seconds(5));
        sourceApp.Stop(m_stopTime - Seconds(5));

        // Upload BE
        InetSocketAddress hostAddress = InetSocketAddress(hostIpv4Address, 10);
        m_bulkSendUp[0] = CreateObject<BulkSendApplication>();
        m_bulkSendUp[0]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_bulkSendUp[0]->SetAttribute("Remote", AddressValue(hostAddress));
        m_bulkSendUp[0]->SetAttribute("MaxBytes", UintegerValue(0));
        m_node->AddApplication(m_bulkSendUp[0]);
        ApplicationContainer sourceAppUp;
        sourceAppUp.Add(m_bulkSendUp[0]);
        sourceAppUp.Start(m_startTime + Seconds(5));
        sourceAppUp.Stop(m_stopTime - Seconds(5));
        m_output["results"]["TCP upload BE"] = Json::Value(Json::arrayValue);
        m_output["raw_values"]["TCP upload BE"] = Json::Value(Json::arrayValue);
        Json::Value data_up;
        data_up["dur"] = m_stepSize.GetSeconds();
        data_up["t"] = (Simulator::Now().GetSeconds() + m_currTime);
        data_up["val"] = 0;
        m_output["raw_values"]["TCP upload BE"].append(data_up);
        m_bulkSendUp[0]->TraceConnectWithoutContext(
            "Tx",
            MakeBoundCallback(&TraceSentPacket, &m_bytesSent[0]));
        Simulator::Schedule(m_stepSize,
                            &FlentApplication::GoodputSamplingUpload,
                            this,
                            "TCP upload BE",
                            0);
        Address sinkAddressUp(InetSocketAddress(Ipv4Address::GetAny(), 10));
        m_packetSinkUp[0] = CreateObject<PacketSink>();
        m_packetSinkUp[0]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_packetSinkUp[0]->SetAttribute("Local", AddressValue(sinkAddressUp));
        m_hostNode->AddApplication(m_packetSinkUp[0]);
        ApplicationContainer sinkAppUp;
        sinkAppUp.Add(m_packetSinkUp[0]);
        sinkAppUp.Start(m_startTime + Seconds(5));
        sinkAppUp.Stop(m_stopTime - Seconds(5));

        // Download BK
        Address sinkAddress2(InetSocketAddress(Ipv4Address::GetAny(), 9));
        m_packetSinkDown[1] = CreateObject<PacketSink>();
        m_packetSinkDown[1]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_packetSinkDown[1]->SetAttribute("Local", AddressValue(sinkAddress2));
        m_node->AddApplication(m_packetSinkDown[1]);
        ApplicationContainer sinkApp2;
        sinkApp2.Add(m_packetSinkDown[1]);
        sinkApp2.Start(m_startTime + Seconds(5));
        sinkApp2.Stop(m_stopTime - Seconds(5));
        m_packetSinkDown[1]->TraceConnectWithoutContext(
            "Rx",
            MakeBoundCallback(&TraceReceivedPacket, &m_bytesReceived[1]));
        m_output["results"]["TCP download BK"] = Json::Value(Json::arrayValue);
        m_output["raw_values"]["TCP download BK"] = Json::Value(Json::arrayValue);
        Json::Value data2;
        data2["dur"] = m_stepSize.GetSeconds();
        data2["t"] = (Simulator::Now().GetSeconds() + m_currTime);
        data2["val"] = 0;
        m_output["raw_values"]["TCP download BK"].append(data2);
        Simulator::Schedule(m_stepSize,
                            &FlentApplication::GoodputSamplingDownload,
                            this,
                            "TCP download BK",
                            1);
        InetSocketAddress localBindAddress2 = InetSocketAddress(localIpv4Address, 9);
        m_bulkSendDown[1] = CreateObject<BulkSendApplication>();
        m_bulkSendDown[1]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_bulkSendDown[1]->SetAttribute("Remote", AddressValue(localBindAddress2));
        m_bulkSendDown[1]->SetAttribute("MaxBytes", UintegerValue(0));
        m_hostNode->AddApplication(m_bulkSendDown[1]);
        ApplicationContainer sourceApp2;
        sourceApp2.Add(m_bulkSendDown[1]);
        sourceApp2.Start(m_startTime + Seconds(5));
        sourceApp2.Stop(m_stopTime - Seconds(5));

        // Upload BK
        hostAddress = InetSocketAddress(hostIpv4Address, 11);
        m_bulkSendUp[1] = CreateObject<BulkSendApplication>();
        m_bulkSendUp[1]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_bulkSendUp[1]->SetAttribute("Remote", AddressValue(hostAddress));
        m_bulkSendUp[1]->SetAttribute("MaxBytes", UintegerValue(0));
        m_node->AddApplication(m_bulkSendUp[1]);
        ApplicationContainer sourceAppUp2;
        sourceAppUp2.Add(m_bulkSendUp[1]);
        sourceAppUp2.Start(m_startTime + Seconds(5));
        sourceAppUp2.Stop(m_stopTime - Seconds(5));
        m_output["results"]["TCP upload BK"] = Json::Value(Json::arrayValue);
        m_output["raw_values"]["TCP upload BK"] = Json::Value(Json::arrayValue);
        Json::Value data_up2;
        data_up2["dur"] = m_stepSize.GetSeconds();
        data_up2["t"] = (Simulator::Now().GetSeconds() + m_currTime);
        data_up2["val"] = 0;
        m_output["raw_values"]["TCP upload BK"].append(data_up2);
        m_bulkSendUp[1]->TraceConnectWithoutContext(
            "Tx",
            MakeBoundCallback(&TraceSentPacket, &m_bytesSent[1]));
        Simulator::Schedule(m_stepSize,
                            &FlentApplication::GoodputSamplingUpload,
                            this,
                            "TCP upload BK",
                            1);
        Address sinkAddressUp2(InetSocketAddress(Ipv4Address::GetAny(), 11));
        m_packetSinkUp[1] = CreateObject<PacketSink>();
        m_packetSinkUp[1]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_packetSinkUp[1]->SetAttribute("Local", AddressValue(sinkAddressUp2));
        m_hostNode->AddApplication(m_packetSinkUp[1]);
        ApplicationContainer sinkAppUp2;
        sinkAppUp2.Add(m_packetSinkUp[1]);
        sinkAppUp2.Start(m_startTime + Seconds(5));
        sinkAppUp2.Stop(m_stopTime - Seconds(5));

        // Download CS5
        Address sinkAddress3(InetSocketAddress(Ipv4Address::GetAny(), 11));
        m_packetSinkDown[2] = CreateObject<PacketSink>();
        m_packetSinkDown[2]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_packetSinkDown[2]->SetAttribute("Local", AddressValue(sinkAddress3));
        m_node->AddApplication(m_packetSinkDown[2]);
        ApplicationContainer sinkApp3;
        sinkApp3.Add(m_packetSinkDown[2]);
        sinkApp3.Start(m_startTime + Seconds(5));
        sinkApp3.Stop(m_stopTime - Seconds(5));
        m_packetSinkDown[2]->TraceConnectWithoutContext(
            "Rx",
            MakeBoundCallback(&TraceReceivedPacket, &m_bytesReceived[2]));
        m_output["results"]["TCP download CS5"] = Json::Value(Json::arrayValue);
        m_output["raw_values"]["TCP download CS5"] = Json::Value(Json::arrayValue);
        Json::Value data3;
        data3["dur"] = m_stepSize.GetSeconds();
        data3["t"] = (Simulator::Now().GetSeconds() + m_currTime);
        data3["val"] = 0;
        m_output["raw_values"]["TCP download CS5"].append(data3);
        Simulator::Schedule(m_stepSize,
                            &FlentApplication::GoodputSamplingDownload,
                            this,
                            "TCP download CS5",
                            2);
        InetSocketAddress localBindAddress3 = InetSocketAddress(localIpv4Address, 11);
        m_bulkSendDown[2] = CreateObject<BulkSendApplication>();
        m_bulkSendDown[2]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_bulkSendDown[2]->SetAttribute("Remote", AddressValue(localBindAddress3));
        m_bulkSendDown[2]->SetAttribute("MaxBytes", UintegerValue(0));
        m_hostNode->AddApplication(m_bulkSendDown[2]);
        ApplicationContainer sourceApp3;
        sourceApp3.Add(m_bulkSendDown[2]);
        sourceApp3.Start(m_startTime + Seconds(5));
        sourceApp3.Stop(m_stopTime - Seconds(5));

        // Upload CS5
        hostAddress = InetSocketAddress(hostIpv4Address, 12);
        m_bulkSendUp[2] = CreateObject<BulkSendApplication>();
        m_bulkSendUp[2]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_bulkSendUp[2]->SetAttribute("Remote", AddressValue(hostAddress));
        m_bulkSendUp[2]->SetAttribute("MaxBytes", UintegerValue(0));
        m_node->AddApplication(m_bulkSendUp[2]);
        ApplicationContainer sourceAppUp3;
        sourceAppUp3.Add(m_bulkSendUp[2]);
        sourceAppUp3.Start(m_startTime + Seconds(5));
        sourceAppUp3.Stop(m_stopTime - Seconds(5));
        m_output["results"]["TCP upload CS5"] = Json::Value(Json::arrayValue);
        m_output["raw_values"]["TCP upload CS5"] = Json::Value(Json::arrayValue);
        Json::Value data_up3;
        data_up3["dur"] = m_stepSize.GetSeconds();
        data_up3["t"] = (Simulator::Now().GetSeconds() + m_currTime);
        data_up3["val"] = 0;
        m_output["raw_values"]["TCP upload CS5"].append(data_up3);
        m_bulkSendUp[2]->TraceConnectWithoutContext(
            "Tx",
            MakeBoundCallback(&TraceSentPacket, &m_bytesSent[2]));
        Simulator::Schedule(m_stepSize,
                            &FlentApplication::GoodputSamplingUpload,
                            this,
                            "TCP upload CS5",
                            2);
        Address sinkAddressUp3(InetSocketAddress(Ipv4Address::GetAny(), 12));
        m_packetSinkUp[2] = CreateObject<PacketSink>();
        m_packetSinkUp[2]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_packetSinkUp[2]->SetAttribute("Local", AddressValue(sinkAddressUp3));
        m_hostNode->AddApplication(m_packetSinkUp[2]);
        ApplicationContainer sinkAppUp3;
        sinkAppUp3.Add(m_packetSinkUp[2]);
        sinkAppUp3.Start(m_startTime + Seconds(5));
        sinkAppUp3.Stop(m_stopTime - Seconds(5));

        // Download EF
        Address sinkAddress4(InetSocketAddress(Ipv4Address::GetAny(), 12));
        m_packetSinkDown[3] = CreateObject<PacketSink>();
        m_packetSinkDown[3]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_packetSinkDown[3]->SetAttribute("Local", AddressValue(sinkAddress4));
        m_node->AddApplication(m_packetSinkDown[3]);
        ApplicationContainer sinkApp4;
        sinkApp4.Add(m_packetSinkDown[3]);
        sinkApp4.Start(m_startTime + Seconds(5));
        sinkApp4.Stop(m_stopTime - Seconds(5));
        m_packetSinkDown[3]->TraceConnectWithoutContext(
            "Rx",
            MakeBoundCallback(&TraceReceivedPacket, &m_bytesReceived[3]));
        m_output["results"]["TCP download EF"] = Json::Value(Json::arrayValue);
        m_output["raw_values"]["TCP download EF"] = Json::Value(Json::arrayValue);
        Json::Value data4;
        data4["dur"] = m_stepSize.GetSeconds();
        data4["t"] = (Simulator::Now().GetSeconds() + m_currTime);
        data4["val"] = 0;
        m_output["raw_values"]["TCP download EF"].append(data4);
        Simulator::Schedule(m_stepSize,
                            &FlentApplication::GoodputSamplingDownload,
                            this,
                            "TCP download EF",
                            3);
        InetSocketAddress localBindAddress4 = InetSocketAddress(localIpv4Address, 12);
        m_bulkSendDown[3] = CreateObject<BulkSendApplication>();
        m_bulkSendDown[3]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_bulkSendDown[3]->SetAttribute("Remote", AddressValue(localBindAddress4));
        m_bulkSendDown[3]->SetAttribute("MaxBytes", UintegerValue(0));
        m_hostNode->AddApplication(m_bulkSendDown[3]);
        ApplicationContainer sourceApp4;
        sourceApp4.Add(m_bulkSendDown[3]);
        sourceApp4.Start(m_startTime + Seconds(5));
        sourceApp4.Stop(m_stopTime - Seconds(5));

        // Upload EF
        hostAddress = InetSocketAddress(hostIpv4Address, 13);
        m_bulkSendUp[3] = CreateObject<BulkSendApplication>();
        m_bulkSendUp[3]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_bulkSendUp[3]->SetAttribute("Remote", AddressValue(hostAddress));
        m_bulkSendUp[3]->SetAttribute("MaxBytes", UintegerValue(0));
        m_node->AddApplication(m_bulkSendUp[3]);
        ApplicationContainer sourceAppUp4;
        sourceAppUp4.Add(m_bulkSendUp[3]);
        sourceAppUp4.Start(m_startTime + Seconds(5));
        sourceAppUp4.Stop(m_stopTime - Seconds(5));
        m_output["results"]["TCP upload EF"] = Json::Value(Json::arrayValue);
        m_output["raw_values"]["TCP upload EF"] = Json::Value(Json::arrayValue);
        Json::Value data_up4;
        data_up4["dur"] = m_stepSize.GetSeconds();
        data_up4["t"] = (Simulator::Now().GetSeconds() + m_currTime);
        data_up4["val"] = 0;
        m_output["raw_values"]["TCP upload EF"].append(data_up4);
        m_bulkSendUp[3]->TraceConnectWithoutContext(
            "Tx",
            MakeBoundCallback(&TraceSentPacket, &m_bytesSent[3]));
        Simulator::Schedule(m_stepSize,
                            &FlentApplication::GoodputSamplingUpload,
                            this,
                            "TCP upload EF",
                            3);
        Address sinkAddressUp4(InetSocketAddress(Ipv4Address::GetAny(), 13));
        m_packetSinkUp[3] = CreateObject<PacketSink>();
        m_packetSinkUp[3]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_packetSinkUp[3]->SetAttribute("Local", AddressValue(sinkAddressUp4));
        m_hostNode->AddApplication(m_packetSinkUp[3]);
        ApplicationContainer sinkAppUp4;
        sinkAppUp4.Add(m_packetSinkUp[3]);
        sinkAppUp4.Start(m_startTime + Seconds(5));
        sinkAppUp4.Stop(m_stopTime - Seconds(5));
    }
}

void
FlentApplication::StopApplication(void) // Called at time specified by Stop
{
    NS_LOG_FUNCTION(this);
    FillXValues();
    ProcessRawValues();
    AsciiTraceHelper ascii;
    if (m_testName.compare("ping") == 0)
    {
        m_v4ping->TraceDisconnectWithoutContext(
            "Rx",
            MakeCallback(&FlentApplication::TraceReceivedPing, this));
    }
    else if (m_testName.compare("tcp_upload") == 0)
    {
        m_v4ping->TraceDisconnectWithoutContext(
            "Rx",
            MakeCallback(&FlentApplication::TraceReceivedPing, this));
        m_bulkSendUp[0]->TraceDisconnectWithoutContext(
            "Tx",
            MakeBoundCallback(&TraceSentPacket, &m_bytesSent[0]));
    }
    else if (m_testName.compare("tcp_download") == 0)
    {
        m_v4ping->TraceDisconnectWithoutContext(
            "Rx",
            MakeCallback(&FlentApplication::TraceReceivedPing, this));
        m_packetSinkDown[0]->TraceDisconnectWithoutContext(
            "Rx",
            MakeBoundCallback(&TraceReceivedPacket, &m_bytesReceived[0]));
    }
    else if (m_testName.compare("rrul") == 0)
    {
        m_v4ping->TraceDisconnectWithoutContext(
            "Rx",
            MakeCallback(&FlentApplication::TraceReceivedPing, this));
        m_udpclient[0]->TraceDisconnectWithoutContext(
            "RxWithSeqTsEchoHeader",
            MakeCallback(&FlentApplication::TraceReceivedUdpPing1, this));
        m_udpclient[1]->TraceDisconnectWithoutContext(
            "RxWithSeqTsEchoHeader",
            MakeCallback(&FlentApplication::TraceReceivedUdpPing2, this));
        m_udpclient[2]->TraceDisconnectWithoutContext(
            "RxWithSeqTsEchoHeader",
            MakeCallback(&FlentApplication::TraceReceivedUdpPing3, this));
        for (uint32_t i = 0; i < 4; ++i)
        {
            m_packetSinkDown[i]->TraceDisconnectWithoutContext(
                "Rx",
                MakeBoundCallback(&TraceReceivedPacket, &m_bytesReceived[i]));
            m_bulkSendUp[i]->TraceDisconnectWithoutContext(
                "Tx",
                MakeBoundCallback(&TraceSentPacket, &m_bytesSent[i]));
        }
    }

    Ptr<OutputStreamWrapper> streamOutput = ascii.CreateFileStream(m_testName + ".flent");
    *streamOutput->GetStream() << m_output << std::endl;
}

} // Namespace ns3
