/*
 * Copyright (c) 2010 Georgia Institute of Technology
 * Copyright (c) 2020 Harsha Sharma : Flent application
 * Copyright (c) 2021 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Harsha Sharma <harshasha256@gmail.com>
 *         (adapted from bulk-send-application.cc and
 *          packet-sink-application.cc written by George F. Riley)
 *
 * Modified by: Ameya Deshpande <ameyanrd@outlook.com>
 *              Bhaskar Kataria <bhaskar.k7920@gmail.com> (Post processing of raw data)
 */

#include "flent-application.h"

#include "nlohmann/json.hpp"

#include "ns3/address.h"
#include "ns3/application-container.h"
#include "ns3/boolean.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/loopback-net-device.h"
#include "ns3/string.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/trace-helper.h"
#include "ns3/uinteger.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("FlentApplication");

NS_OBJECT_ENSURE_REGISTERED(FlentApplication);

TypeId
FlentApplication::GetTypeId()
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
            .AddAttribute("OutputFilename",
                          "Path of the .flent output file to write, absolute or "
                          "relative to the working directory. If empty (the default), "
                          "the file is written as 'TestName.flent' in the working "
                          "directory.",
                          StringValue(""),
                          MakeStringAccessor(&FlentApplication::m_outputFilename),
                          MakeStringChecker())
            .AddAttribute("T0",
                          "Absolute start time, as an offset from the Unix epoch, used to "
                          "anchor all timestamps in the output file. The default "
                          "corresponds to 2026-01-01T00:00:00Z. Any fixed value keeps the "
                          "output byte-for-byte reproducible across runs. Ignored if "
                          "UseWallClockT0 is true.",
                          TimeValue(Seconds(1767225600)),
                          MakeTimeAccessor(&FlentApplication::m_t0),
                          MakeTimeChecker())
            .AddAttribute("UseWallClockT0",
                          "If true, read the system wall clock once at application start "
                          "and use it in place of the T0 attribute as the timestamp "
                          "anchor. Output files are then not reproducible across runs.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&FlentApplication::m_useWallClockT0),
                          MakeBooleanChecker())
            .AddAttribute("StepSize",
                          "Measurement data point size",
                          TimeValue(MilliSeconds(200)),
                          MakeTimeAccessor(&FlentApplication::m_stepSize),
                          MakeTimeChecker(MilliSeconds(50), Seconds(1)));
    return tid;
}

FlentApplication::FlentApplication()
    : m_output(std::make_unique<nlohmann::json>())
{
}

FlentApplication::~FlentApplication() = default;

void
FlentApplication::DoInitialize()
{
    NS_LOG_FUNCTION(this);

    if (m_localBindAddress.IsInvalid())
    {
        Ptr<Ipv4L3Protocol> ip = m_node->GetObject<Ipv4L3Protocol>();
        if (ip)
        {
            for (uint32_t deviceId = 0; deviceId < m_node->GetNDevices(); deviceId++)
            {
                Ptr<NetDevice> device = m_node->GetDevice(deviceId);

                if (DynamicCast<LoopbackNetDevice>(device))
                {
                    continue;
                }

                // If this is not a loopback device add the IP address to the map
                int32_t interfaceIndex = (ip)->GetInterfaceForDevice(device);

                if (interfaceIndex != -1)
                {
                    m_localBindAddress = ip->GetAddress(interfaceIndex, 0).GetLocal();
                    break;
                }
            }
        }
    }

    // Override the Stop Time set for the Application Container
    m_stopTime = m_startTime + m_length + Seconds(10);

    Application::DoInitialize();
}

void
FlentApplication::DoDispose()
{
    NS_LOG_FUNCTION(this);

    m_ping = nullptr;

    for (uint32_t i = 0; i < 4; ++i)
    {
        m_packetSinkUp[i] = nullptr;
        m_packetSinkDown[i] = nullptr;
        m_bulkSendUp[i] = nullptr;
        m_bulkSendDown[i] = nullptr;
    }

    for (uint32_t i = 0; i < 3; ++i)
    {
        m_udpserver[i] = nullptr;
        m_udpclient[i] = nullptr;
    }

    // chain up
    Application::DoDispose();
}

std::string
FlentApplication::GetUtcFormatTime() const
{
    std::chrono::duration<double> duration(m_currTime);
    auto wholeSeconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    auto microseconds =
        std::chrono::duration_cast<std::chrono::microseconds>(duration - wholeSeconds);

    std::chrono::system_clock::time_point tp(wholeSeconds);
    std::time_t t = std::chrono::system_clock::to_time_t(tp);

    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%S") << "." << std::setfill('0')
        << std::setw(6) << microseconds.count() << "Z";

    return oss.str();
}

void
FlentApplication::AddMetadata(nlohmann::json& j)
{
    std::string outputPath = m_outputFilename.empty() ? m_testName + ".flent" : m_outputFilename;
    std::string dataFilename = std::filesystem::path(outputPath).filename().string();

    std::ostringstream oss;
    oss << Ipv4Address::ConvertFrom(m_hostAddress);
    std::string hostName = oss.str();

    std::ostringstream ossLocal;
    ossLocal << Ipv4Address::ConvertFrom(m_localBindAddress);
    std::string localHost = ossLocal.str();

    std::string timeStr = GetUtcFormatTime();

    nlohmann::json title = nullptr;
    if (!m_imageText.empty())
    {
        title = m_imageText;
    }

    j["metadata"] = {{"BATCH_NAME", nullptr},
                     {"BATCH_TIME", nullptr},
                     {"BATCH_TITLE", nullptr},
                     {"BATCH_UUID", nullptr},
                     {"DATA_FILENAME", dataFilename},
                     {"EGRESS_INFO", {{"bql", {{"tx-0", ""}}}}},
                     {"classes", nullptr},
                     {"driver", nullptr},
                     {"iface", nullptr},
                     {"link_params", {{"qlen", nullptr}}},
                     {"offloads",
                      {{"generic-receive-offload", nullptr},
                       {"generic-segmentation-offload", nullptr},
                       {"large-receive-offload", nullptr},
                       {"tcp-segmentation", nullptr},
                       {"udp-fragmentation", nullptr}}},
                     {"qdiscs",
                      {{"id", nullptr},
                       {"name", nullptr},
                       {"params",
                        {{"ecn", nullptr},
                         {"flows", nullptr},
                         {"interval", nullptr},
                         {"limit", nullptr},
                         {"memory_limit", nullptr},
                         {"quantum", nullptr},
                         {"refcnt", nullptr},
                         {"target", nullptr}}},
                       {"parent", nullptr}}},
                     {"FAILED_RUNNERS", nullptr},
                     {"FLENT_VERSION", nullptr},
                     {"HOST", hostName},
                     {"HOSTS", nlohmann::json::array({hostName})},
                     {"HTTP_GETTER_DNS", nullptr},
                     {"HTTP_GETTER_URLLIST", nullptr},
                     {"HTTP_GETTER_WORKERS", nullptr},
                     {"IP_VERSION", 4},
                     {"KERNEL_NAME", "ns-3"},
                     {"KERNEL_RELEASE", "ns-3"},
                     {"LENGTH", m_length.GetSeconds()},
                     {"LOCAL_HOST", localHost},
                     {"MODULE_VERSIONS", nullptr},
                     {"NAME", m_testName},
                     {"NOTE", nullptr},
                     {"REMOTE_METADATA", nullptr},
                     {"STEP_SIZE", m_stepSize.GetSeconds()},
                     {"TIME", timeStr},
                     {"T0", timeStr},
                     {"TEST_PARAMETERS", nlohmann::json::object()},
                     {"TITLE", title},
                     {"TOTAL_LENGTH", m_stopTime.GetSeconds()}};

    j["version"] = 4;
}

Ptr<Node>
FlentApplication::GetHostNode(Ipv4Address hostAddress) const
{
    NS_LOG_FUNCTION(this << hostAddress);

    for (auto it = NodeList::Begin(); it != NodeList::End(); ++it)
    {
        Ptr<Node> node = *it;
        Ptr<Ipv4L3Protocol> ip = node->GetObject<Ipv4L3Protocol>();

        if (!ip)
        {
            continue;
        }

        for (uint32_t deviceId = 0; deviceId < node->GetNDevices(); deviceId++)
        {
            int32_t interfaceIndex = (ip)->GetInterfaceForDevice(node->GetDevice(deviceId));
            if (interfaceIndex != -1)
            {
                uint32_t numberOfAddresses = ip->GetNAddresses(interfaceIndex);
                for (uint32_t addressIndex = 0; addressIndex < numberOfAddresses; addressIndex++)
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

    return nullptr;
}

void
FlentApplication::TraceSentPacket(uint32_t* counter, Ptr<const Packet> packet)
{
    *counter += packet->GetSize();
}

void
FlentApplication::TraceReceivedPacket(uint32_t* counter,
                                      Ptr<const Packet> packet,
                                      const Address& address)
{
    *counter += packet->GetSize();
}

void
FlentApplication::TraceReceivedPing(uint16_t seq, Time rtt)
{
    nlohmann::json data;
    data["seq"] = seq;
    data["t"] = (Simulator::Now().GetSeconds() + m_currTime);
    data["val"] = rtt.GetSeconds() * 1000;
    (*m_output)["raw_values"]["Ping (ms) ICMP"].push_back(data);
}

void
FlentApplication::TraceReceivedUdpPing1(Ptr<const Packet> packet,
                                        const Address& address,
                                        const Address& localAddress,
                                        const SeqTsEchoHeader& header)
{
    nlohmann::json data;
    Time t = header.GetTsValue();
    double rtt = t.GetSeconds() * 1000;
    data["dur"] = m_stepSize.GetSeconds();
    data["t"] = (Simulator::Now().GetSeconds() + m_currTime);
    data["val"] = rtt;
    (*m_output)["raw_values"]["Ping (ms) UDP BE"].push_back(data);
}

void
FlentApplication::TraceReceivedUdpPing2(Ptr<const Packet> packet,
                                        const Address& address,
                                        const Address& localAddress,
                                        const SeqTsEchoHeader& header)
{
    nlohmann::json data;
    Time t = header.GetTsValue();
    double rtt = t.GetSeconds() * 1000;
    data["dur"] = m_stepSize.GetSeconds();
    data["t"] = (Simulator::Now().GetSeconds() + m_currTime);
    data["val"] = rtt;
    (*m_output)["raw_values"]["Ping (ms) UDP BK"].push_back(data);
}

void
FlentApplication::TraceReceivedUdpPing3(Ptr<const Packet> packet,
                                        const Address& address,
                                        const Address& localAddress,
                                        const SeqTsEchoHeader& header)
{
    nlohmann::json data;
    Time t = header.GetTsValue();
    double rtt = t.GetSeconds() * 1000;
    data["dur"] = m_stepSize.GetSeconds();
    data["t"] = (Simulator::Now().GetSeconds() + m_currTime);
    data["val"] = rtt;
    (*m_output)["raw_values"]["Ping (ms) UDP EF"].push_back(data);
}

void
FlentApplication::GoodputSamplingUpload(std::string name, int i)
{
    nlohmann::json data;
    double goodput = (m_bytesSent[i] * 8 / m_stepSize.GetSeconds() / 1e6);
    data["dur"] = m_stepSize.GetSeconds();
    data["t"] = (Simulator::Now().GetSeconds() + m_currTime);
    data["val"] = goodput;
    (*m_output)["raw_values"][name].push_back(data);
    m_bytesSent[i] = 0;
    if (Simulator::Now() + m_stepSize < m_stopTime)
    {
        Simulator::Schedule(m_stepSize, &FlentApplication::GoodputSamplingUpload, this, name, i);
    }
}

void
FlentApplication::GoodputSamplingDownload(std::string name, int i)
{
    nlohmann::json data;
    double goodput = (m_bytesReceived[i] * 8 / m_stepSize.GetSeconds() / 1e6);
    data["dur"] = m_stepSize.GetSeconds();
    data["t"] = (Simulator::Now().GetSeconds() + m_currTime);
    data["val"] = goodput;
    (*m_output)["raw_values"][name].push_back(data);
    m_bytesReceived[i] = 0;
    if (Simulator::Now() + m_stepSize < m_stopTime)
    {
        Simulator::Schedule(m_stepSize, &FlentApplication::GoodputSamplingDownload, this, name, i);
    }
}

void
FlentApplication::FillXValues()
{
    NS_LOG_DEBUG("Filling x values");
    double stepSize = m_stepSize.GetSeconds();
    NS_LOG_DEBUG(stepSize);

    int totalSteps = int(std::ceil(m_stopTime.GetSeconds() / stepSize));

    for (int step = 0; step < totalSteps; step += 1)
    {
        NS_LOG_DEBUG(step);
        (*m_output)["x_values"].push_back(step * stepSize);
    }
}

void
FlentApplication::ProcessRawValues()
{
    NS_LOG_DEBUG("Process Raw values");
    int steps = int(std::ceil(m_stopTime.GetSeconds() / m_stepSize.GetSeconds()));

    for (int s = 0; s < steps; s++)
    {
        double t = m_currTime + (m_stepSize.GetSeconds() * s);

        for (auto itrName = (*m_output)["raw_values"].begin();
             itrName != (*m_output)["raw_values"].end();
             itrName++)
        {
            const std::string& rawValueName = itrName.key();
            double maxDist = m_stepSize.GetSeconds() * 5.0;

            if (itrName.value().empty() || itrName.value().is_null())
            {
                (*m_output)["results"][rawValueName].push_back(nullptr);
                continue;
            }

            bool hasPrev = false;
            bool hasNext = false;
            double tPrev = 0.0;
            double vPrev = 0.0;
            double tNext = 0.0;
            double vNext = 0.0;

            auto& arr = (*m_output)["raw_values"][rawValueName];
            auto prev = arr.end();

            for (auto itrValues = arr.begin(); itrValues != arr.end();
                 prev = itrValues, itrValues++)
            {
                if ((*itrValues)["t"].get<double>() > t)
                {
                    if (prev != arr.end())
                    {
                        tPrev = (*prev)["t"].get<double>();
                        vPrev = (*prev)["val"].get<double>();
                        hasPrev = true;
                    }
                    else
                    {
                        maxDist = m_stepSize.GetSeconds() * 0.5;
                    }

                    tNext = (*itrValues)["t"].get<double>();
                    vNext = (*itrValues)["val"].get<double>();
                    hasNext = true;
                    break;
                }
            }

            bool last = false;
            if (!hasNext)
            {
                auto lastItr = std::prev(arr.end());
                tNext = (*lastItr)["t"].get<double>();
                vNext = (*lastItr)["val"].get<double>();
                hasNext = true;
                last = true;
            }

            if (std::fabs(t - tNext) <= maxDist)
            {
                if (!hasPrev)
                {
                    // 0.0 means no data yet - push null
                    if (vNext == 0.0)
                    {
                        (*m_output)["results"][rawValueName].push_back(nullptr);
                    }
                    else if (last)
                    {
                        auto& resultArr = (*m_output)["results"][rawValueName];
                        bool alreadyRecorded = !resultArr.empty() && (resultArr.back() == vNext ||
                                                                      resultArr.back().is_null());
                        if (alreadyRecorded)
                        {
                            (*m_output)["results"][rawValueName].push_back(nullptr);
                        }
                        else
                        {
                            (*m_output)["results"][rawValueName].push_back(vNext);
                        }
                    }
                    else
                    {
                        (*m_output)["results"][rawValueName].push_back(vNext);
                    }
                }
                else
                {
                    // Don't interpolate across a zero boundary
                    if (vPrev == 0.0 || vNext == 0.0)
                    {
                        (*m_output)["results"][rawValueName].push_back(nullptr);
                    }
                    else
                    {
                        double dvDt = (vNext - vPrev) / (tNext - tPrev);
                        (*m_output)["results"][rawValueName].push_back(vPrev + dvDt * (t - tPrev));
                    }
                }
            }
            else
            {
                (*m_output)["results"][rawValueName].push_back(nullptr);
            }
        }
    }
}

// Application Methods
void
FlentApplication::StartApplication()
{
    NS_LOG_FUNCTION(this);
    if (m_useWallClockT0)
    {
        auto sinceEpoch = std::chrono::system_clock::now().time_since_epoch();
        m_currTime = std::chrono::duration_cast<std::chrono::nanoseconds>(sinceEpoch).count() / 1e9;
    }
    else
    {
        m_currTime = m_t0.GetSeconds();
    }
    AddMetadata(*m_output);

    Ptr<Node> hostNode = GetHostNode(Ipv4Address::ConvertFrom(m_hostAddress));

    if (!hostNode)
    {
        NS_FATAL_ERROR("Couldn't find dest node given the IP" << m_hostAddress);
    }

    if (m_testName == "ping")
    {
        m_ping = CreateObjectWithAttributes<Ping>("Destination",
                                                  AddressValue(m_hostAddress),
                                                  "Interval",
                                                  TimeValue(m_stepSize));
        m_node->AddApplication(m_ping);
        m_ping->SetStartTime(m_startTime);
        m_ping->SetStopTime(m_stopTime);

        (*m_output)["raw_values"]["Ping (ms) ICMP"] = nlohmann::json::array();
        (*m_output)["results"]["Ping (ms) ICMP"] = nlohmann::json::array();
        (*m_output)["x_values"] = nlohmann::json::array();

        m_ping->TraceConnectWithoutContext(
            "Rtt",
            MakeCallback(&FlentApplication::TraceReceivedPing, this));
    }
    else if (m_testName == "tcp_upload")
    {
        Ipv4Address hostAddr = Ipv4Address::ConvertFrom(m_hostAddress);
        m_ping = CreateObjectWithAttributes<Ping>("Destination",
                                                  AddressValue(m_hostAddress),
                                                  "Interval",
                                                  TimeValue(m_stepSize));
        m_node->AddApplication(m_ping);
        m_ping->SetStartTime(m_startTime);
        m_ping->SetStopTime(m_stopTime);

        (*m_output)["raw_values"]["Ping (ms) ICMP"] = nlohmann::json::array();
        (*m_output)["results"]["Ping (ms) ICMP"] = nlohmann::json::array();
        (*m_output)["x_values"] = nlohmann::json::array();

        m_ping->TraceConnectWithoutContext(
            "Rtt",
            MakeCallback(&FlentApplication::TraceReceivedPing, this));

        InetSocketAddress clientAddress = InetSocketAddress(hostAddr, 9020);
        m_bulkSendUp[0] = CreateObject<BulkSendApplication>();
        m_bulkSendUp[0]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_bulkSendUp[0]->SetAttribute("Remote", AddressValue(clientAddress));
        m_bulkSendUp[0]->SetAttribute("MaxBytes", UintegerValue(0));
        m_node->AddApplication(m_bulkSendUp[0]);
        m_bulkSendUp[0]->SetStartTime(m_startTime + Seconds(5));
        m_bulkSendUp[0]->SetStopTime(m_stopTime - Seconds(5));
        (*m_output)["results"]["TCP upload"] = nlohmann::json::array();
        (*m_output)["raw_values"]["TCP upload"] = nlohmann::json::array();
        nlohmann::json data;
        data["dur"] = m_stepSize.GetSeconds();
        data["t"] = (Simulator::Now().GetSeconds() + m_currTime);
        data["val"] = 0;
        (*m_output)["raw_values"]["TCP upload"].push_back(data);
        m_bulkSendUp[0]->TraceConnectWithoutContext(
            "Tx",
            MakeBoundCallback(&FlentApplication::TraceSentPacket, &m_bytesSent[0]));
        Simulator::Schedule(m_stepSize,
                            &FlentApplication::GoodputSamplingUpload,
                            this,
                            "TCP upload",
                            0);

        Address sinkAddress(InetSocketAddress(Ipv4Address::GetAny(), 9020));
        m_packetSinkUp[0] = CreateObject<PacketSink>();
        m_packetSinkUp[0]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_packetSinkUp[0]->SetAttribute("Local", AddressValue(sinkAddress));
        hostNode->AddApplication(m_packetSinkUp[0]);
        m_packetSinkUp[0]->SetStartTime(m_startTime + Seconds(5));
        m_packetSinkUp[0]->SetStopTime(m_stopTime - Seconds(5));
    }
    else if (m_testName == "tcp_download")
    {
        Ipv4Address localBindAddr = Ipv4Address::ConvertFrom(m_localBindAddress);
        m_ping = CreateObjectWithAttributes<Ping>("Destination",
                                                  AddressValue(m_localBindAddress),
                                                  "Interval",
                                                  TimeValue(m_stepSize));
        hostNode->AddApplication(m_ping);
        m_ping->SetStartTime(m_startTime);
        m_ping->SetStopTime(m_stopTime);

        (*m_output)["raw_values"]["Ping (ms) ICMP"] = nlohmann::json::array();
        (*m_output)["results"]["Ping (ms) ICMP"] = nlohmann::json::array();
        (*m_output)["x_values"] = nlohmann::json::array();

        m_ping->TraceConnectWithoutContext(
            "Rtt",
            MakeCallback(&FlentApplication::TraceReceivedPing, this));
        Address sinkAddress(InetSocketAddress(Ipv4Address::GetAny(), 9010));
        m_packetSinkDown[0] = CreateObject<PacketSink>();
        m_packetSinkDown[0]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_packetSinkDown[0]->SetAttribute("Local", AddressValue(sinkAddress));
        m_node->AddApplication(m_packetSinkDown[0]);
        m_packetSinkDown[0]->SetStartTime(m_startTime + Seconds(5));
        m_packetSinkDown[0]->SetStopTime(m_stopTime - Seconds(5));
        m_packetSinkDown[0]->TraceConnectWithoutContext(
            "Rx",
            MakeBoundCallback(&FlentApplication::TraceReceivedPacket, &m_bytesReceived[0]));
        (*m_output)["results"]["TCP download"] = nlohmann::json::array();
        (*m_output)["raw_values"]["TCP download"] = nlohmann::json::array();
        nlohmann::json data;
        data["dur"] = m_stepSize.GetSeconds();
        data["t"] = (Simulator::Now().GetSeconds() + m_currTime);
        data["val"] = 0;
        (*m_output)["raw_values"]["TCP download"].push_back(data);
        Simulator::Schedule(m_stepSize,
                            &FlentApplication::GoodputSamplingDownload,
                            this,
                            "TCP download",
                            0);

        InetSocketAddress localBindAddress = InetSocketAddress(localBindAddr, 9010);
        m_bulkSendDown[0] = CreateObject<BulkSendApplication>();
        m_bulkSendDown[0]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_bulkSendDown[0]->SetAttribute("Remote", AddressValue(localBindAddress));
        m_bulkSendDown[0]->SetAttribute("MaxBytes", UintegerValue(0));
        hostNode->AddApplication(m_bulkSendDown[0]);
        m_bulkSendDown[0]->SetStartTime(m_startTime + Seconds(5));
        m_bulkSendDown[0]->SetStopTime(m_stopTime - Seconds(5));
    }
    else if (m_testName == "rrul")
    {
        Ipv4Address hostIpv4Address = Ipv4Address::ConvertFrom(m_hostAddress);
        Ipv4Address localIpv4Address = Ipv4Address::ConvertFrom(m_localBindAddress);

        m_ping = CreateObjectWithAttributes<Ping>("Destination",
                                                  AddressValue(m_hostAddress),
                                                  "Interval",
                                                  TimeValue(m_stepSize));
        m_node->AddApplication(m_ping);
        m_ping->SetStartTime(m_startTime);
        m_ping->SetStopTime(m_stopTime);

        (*m_output)["raw_values"]["Ping (ms) ICMP"] = nlohmann::json::array();
        (*m_output)["results"]["Ping (ms) ICMP"] = nlohmann::json::array();
        (*m_output)["x_values"] = nlohmann::json::array();
        m_ping->TraceConnectWithoutContext(
            "Rtt",
            MakeCallback(&FlentApplication::TraceReceivedPing, this));

        uint16_t port = 9000;
        m_udpserver[0] = CreateObject<UdpEchoServer>();
        m_udpserver[0]->SetAttribute("Port", UintegerValue(port));
        m_udpserver[0]->SetAttribute("EnableSeqTsEchoHeader", BooleanValue(true));
        hostNode->AddApplication(m_udpserver[0]);
        m_udpserver[0]->SetStartTime(m_startTime);
        m_udpserver[0]->SetStopTime(m_stopTime);
        uint32_t packetSize = 1024;
        uint32_t maxPacketCount = 10000;
        m_udpclient[0] = CreateObject<UdpEchoClient>();
        m_udpclient[0]->SetAttribute("Remote",
                                     AddressValue(InetSocketAddress(hostIpv4Address, port)));
        m_udpclient[0]->SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
        m_udpclient[0]->SetAttribute("Interval", TimeValue(m_stepSize));
        m_udpclient[0]->SetAttribute("PacketSize", UintegerValue(packetSize));
        m_udpclient[0]->SetAttribute("EnableSeqTsEchoHeader", BooleanValue(true));
        m_udpclient[0]->SetAttribute("Tos", UintegerValue(Ipv4Header::DscpType::DscpDefault << 2));
        m_node->AddApplication(m_udpclient[0]);
        m_udpclient[0]->SetStartTime(m_startTime);
        m_udpclient[0]->SetStopTime(m_stopTime);
        (*m_output)["raw_values"]["Ping (ms) UDP BE"] = nlohmann::json::array();
        (*m_output)["results"]["Ping (ms) UDP BE"] = nlohmann::json::array();
        m_udpclient[0]->TraceConnectWithoutContext(
            "RxWithSeqTsEchoHeader",
            MakeCallback(&FlentApplication::TraceReceivedUdpPing1, this));

        port = 9001;
        m_udpserver[1] = CreateObject<UdpEchoServer>();
        m_udpserver[1]->SetAttribute("Port", UintegerValue(port));
        m_udpserver[1]->SetAttribute("EnableSeqTsEchoHeader", BooleanValue(true));
        hostNode->AddApplication(m_udpserver[1]);
        m_udpserver[1]->SetStartTime(m_startTime);
        m_udpserver[1]->SetStopTime(m_stopTime);
        m_udpclient[1] = CreateObject<UdpEchoClient>();
        m_udpclient[1]->SetAttribute("Remote",
                                     AddressValue(InetSocketAddress(hostIpv4Address, port)));
        m_udpclient[1]->SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
        m_udpclient[1]->SetAttribute("Interval", TimeValue(m_stepSize));
        m_udpclient[1]->SetAttribute("PacketSize", UintegerValue(packetSize));
        m_udpclient[1]->SetAttribute("EnableSeqTsEchoHeader", BooleanValue(true));
        m_udpclient[1]->SetAttribute("Tos", UintegerValue(Ipv4Header::DscpType::DSCP_CS1 << 2));
        m_node->AddApplication(m_udpclient[1]);
        m_udpclient[1]->SetStartTime(m_startTime);
        m_udpclient[1]->SetStopTime(m_stopTime);
        (*m_output)["raw_values"]["Ping (ms) UDP BK"] = nlohmann::json::array();
        (*m_output)["results"]["Ping (ms) UDP BK"] = nlohmann::json::array();
        m_udpclient[1]->TraceConnectWithoutContext(
            "RxWithSeqTsEchoHeader",
            MakeCallback(&FlentApplication::TraceReceivedUdpPing2, this));

        port = 9002;
        m_udpserver[2] = CreateObject<UdpEchoServer>();
        m_udpserver[2]->SetAttribute("Port", UintegerValue(port));
        m_udpserver[2]->SetAttribute("EnableSeqTsEchoHeader", BooleanValue(true));
        hostNode->AddApplication(m_udpserver[2]);
        m_udpserver[2]->SetStartTime(m_startTime);
        m_udpserver[2]->SetStopTime(m_stopTime);
        m_udpclient[2] = CreateObject<UdpEchoClient>();
        m_udpclient[2]->SetAttribute("Remote",
                                     AddressValue(InetSocketAddress(hostIpv4Address, port)));
        m_udpclient[2]->SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
        m_udpclient[2]->SetAttribute("Interval", TimeValue(m_stepSize));
        m_udpclient[2]->SetAttribute("PacketSize", UintegerValue(packetSize));
        m_udpclient[2]->SetAttribute("EnableSeqTsEchoHeader", BooleanValue(true));
        m_udpclient[2]->SetAttribute("Tos", UintegerValue(Ipv4Header::DscpType::DSCP_EF << 2));
        m_node->AddApplication(m_udpclient[2]);
        m_udpclient[2]->SetStartTime(m_startTime);
        m_udpclient[2]->SetStopTime(m_stopTime);
        (*m_output)["raw_values"]["Ping (ms) UDP EF"] = nlohmann::json::array();
        (*m_output)["results"]["Ping (ms) UDP EF"] = nlohmann::json::array();
        m_udpclient[2]->TraceConnectWithoutContext(
            "RxWithSeqTsEchoHeader",
            MakeCallback(&FlentApplication::TraceReceivedUdpPing3, this));

        // Download BE
        Address sinkAddress(InetSocketAddress(Ipv4Address::GetAny(), 9010));
        m_packetSinkDown[0] = CreateObject<PacketSink>();
        m_packetSinkDown[0]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_packetSinkDown[0]->SetAttribute("Local", AddressValue(sinkAddress));
        m_packetSinkDown[0]->SetAttribute("Tos",
                                          UintegerValue(Ipv4Header::DscpType::DscpDefault << 2));
        m_node->AddApplication(m_packetSinkDown[0]);
        m_packetSinkDown[0]->SetStartTime(m_startTime + Seconds(5));
        m_packetSinkDown[0]->SetStopTime(m_stopTime - Seconds(5));
        m_packetSinkDown[0]->TraceConnectWithoutContext(
            "Rx",
            MakeBoundCallback(&FlentApplication::TraceReceivedPacket, &m_bytesReceived[0]));
        (*m_output)["results"]["TCP download BE"] = nlohmann::json::array();
        (*m_output)["raw_values"]["TCP download BE"] = nlohmann::json::array();
        nlohmann::json data;
        data["dur"] = m_stepSize.GetSeconds();
        data["t"] = (Simulator::Now().GetSeconds() + m_currTime);
        data["val"] = 0;
        (*m_output)["raw_values"]["TCP download BE"].push_back(data);
        Simulator::Schedule(m_stepSize,
                            &FlentApplication::GoodputSamplingDownload,
                            this,
                            "TCP download BE",
                            0);
        InetSocketAddress localBindAddress = InetSocketAddress(localIpv4Address, 9010);
        m_bulkSendDown[0] = CreateObject<BulkSendApplication>();
        m_bulkSendDown[0]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_bulkSendDown[0]->SetAttribute("Remote", AddressValue(localBindAddress));
        m_bulkSendDown[0]->SetAttribute("MaxBytes", UintegerValue(0));
        m_bulkSendDown[0]->SetAttribute("Tos",
                                        UintegerValue(Ipv4Header::DscpType::DscpDefault << 2));
        hostNode->AddApplication(m_bulkSendDown[0]);
        m_bulkSendDown[0]->SetStartTime(m_startTime + Seconds(5));
        m_bulkSendDown[0]->SetStopTime(m_stopTime - Seconds(5));

        // Upload BE
        InetSocketAddress hostAddress = InetSocketAddress(hostIpv4Address, 9020);
        m_bulkSendUp[0] = CreateObject<BulkSendApplication>();
        m_bulkSendUp[0]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_bulkSendUp[0]->SetAttribute("Remote", AddressValue(hostAddress));
        m_bulkSendUp[0]->SetAttribute("MaxBytes", UintegerValue(0));
        m_bulkSendUp[0]->SetAttribute("Tos", UintegerValue(Ipv4Header::DscpType::DscpDefault << 2));
        m_node->AddApplication(m_bulkSendUp[0]);
        m_bulkSendUp[0]->SetStartTime(m_startTime + Seconds(5));
        m_bulkSendUp[0]->SetStopTime(m_stopTime - Seconds(5));
        (*m_output)["results"]["TCP upload BE"] = nlohmann::json::array();
        (*m_output)["raw_values"]["TCP upload BE"] = nlohmann::json::array();
        nlohmann::json data_up;
        data_up["dur"] = m_stepSize.GetSeconds();
        data_up["t"] = (Simulator::Now().GetSeconds() + m_currTime);
        data_up["val"] = 0;
        (*m_output)["raw_values"]["TCP upload BE"].push_back(data_up);
        m_bulkSendUp[0]->TraceConnectWithoutContext(
            "Tx",
            MakeBoundCallback(&FlentApplication::TraceSentPacket, &m_bytesSent[0]));
        Simulator::Schedule(m_stepSize,
                            &FlentApplication::GoodputSamplingUpload,
                            this,
                            "TCP upload BE",
                            0);
        Address sinkAddressUp(InetSocketAddress(Ipv4Address::GetAny(), 9020));
        m_packetSinkUp[0] = CreateObject<PacketSink>();
        m_packetSinkUp[0]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_packetSinkUp[0]->SetAttribute("Local", AddressValue(sinkAddressUp));
        m_packetSinkUp[0]->SetAttribute("Tos",
                                        UintegerValue(Ipv4Header::DscpType::DscpDefault << 2));
        hostNode->AddApplication(m_packetSinkUp[0]);
        m_packetSinkUp[0]->SetStartTime(m_startTime + Seconds(5));
        m_packetSinkUp[0]->SetStopTime(m_stopTime - Seconds(5));

        // Download BK
        Address sinkAddress2(InetSocketAddress(Ipv4Address::GetAny(), 9011));
        m_packetSinkDown[1] = CreateObject<PacketSink>();
        m_packetSinkDown[1]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_packetSinkDown[1]->SetAttribute("Local", AddressValue(sinkAddress2));
        m_packetSinkDown[1]->SetAttribute("Tos",
                                          UintegerValue(Ipv4Header::DscpType::DSCP_CS1 << 2));
        m_node->AddApplication(m_packetSinkDown[1]);
        m_packetSinkDown[1]->SetStartTime(m_startTime + Seconds(5));
        m_packetSinkDown[1]->SetStopTime(m_stopTime - Seconds(5));
        m_packetSinkDown[1]->TraceConnectWithoutContext(
            "Rx",
            MakeBoundCallback(&FlentApplication::TraceReceivedPacket, &m_bytesReceived[1]));
        (*m_output)["results"]["TCP download BK"] = nlohmann::json::array();
        (*m_output)["raw_values"]["TCP download BK"] = nlohmann::json::array();
        nlohmann::json data2;
        data2["dur"] = m_stepSize.GetSeconds();
        data2["t"] = (Simulator::Now().GetSeconds() + m_currTime);
        data2["val"] = 0;
        (*m_output)["raw_values"]["TCP download BK"].push_back(data2);
        Simulator::Schedule(m_stepSize,
                            &FlentApplication::GoodputSamplingDownload,
                            this,
                            "TCP download BK",
                            1);
        InetSocketAddress localBindAddress2 = InetSocketAddress(localIpv4Address, 9011);
        m_bulkSendDown[1] = CreateObject<BulkSendApplication>();
        m_bulkSendDown[1]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_bulkSendDown[1]->SetAttribute("Remote", AddressValue(localBindAddress2));
        m_bulkSendDown[1]->SetAttribute("MaxBytes", UintegerValue(0));
        m_bulkSendDown[1]->SetAttribute("Tos", UintegerValue(Ipv4Header::DscpType::DSCP_CS1 << 2));
        hostNode->AddApplication(m_bulkSendDown[1]);
        m_bulkSendDown[1]->SetStartTime(m_startTime + Seconds(5));
        m_bulkSendDown[1]->SetStopTime(m_stopTime - Seconds(5));

        // Upload BK
        hostAddress = InetSocketAddress(hostIpv4Address, 9021);
        m_bulkSendUp[1] = CreateObject<BulkSendApplication>();
        m_bulkSendUp[1]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_bulkSendUp[1]->SetAttribute("Remote", AddressValue(hostAddress));
        m_bulkSendUp[1]->SetAttribute("MaxBytes", UintegerValue(0));
        m_bulkSendUp[1]->SetAttribute("Tos", UintegerValue(Ipv4Header::DscpType::DSCP_CS1 << 2));
        m_node->AddApplication(m_bulkSendUp[1]);
        m_bulkSendUp[1]->SetStartTime(m_startTime + Seconds(5));
        m_bulkSendUp[1]->SetStopTime(m_stopTime - Seconds(5));
        (*m_output)["results"]["TCP upload BK"] = nlohmann::json::array();
        (*m_output)["raw_values"]["TCP upload BK"] = nlohmann::json::array();
        nlohmann::json data_up2;
        data_up2["dur"] = m_stepSize.GetSeconds();
        data_up2["t"] = (Simulator::Now().GetSeconds() + m_currTime);
        data_up2["val"] = 0;
        (*m_output)["raw_values"]["TCP upload BK"].push_back(data_up2);
        m_bulkSendUp[1]->TraceConnectWithoutContext(
            "Tx",
            MakeBoundCallback(&FlentApplication::TraceSentPacket, &m_bytesSent[1]));
        Simulator::Schedule(m_stepSize,
                            &FlentApplication::GoodputSamplingUpload,
                            this,
                            "TCP upload BK",
                            1);
        Address sinkAddressUp2(InetSocketAddress(Ipv4Address::GetAny(), 9021));
        m_packetSinkUp[1] = CreateObject<PacketSink>();
        m_packetSinkUp[1]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_packetSinkUp[1]->SetAttribute("Local", AddressValue(sinkAddressUp2));
        m_packetSinkUp[1]->SetAttribute("Tos", UintegerValue(Ipv4Header::DscpType::DSCP_CS1 << 2));
        hostNode->AddApplication(m_packetSinkUp[1]);
        m_packetSinkUp[1]->SetStartTime(m_startTime + Seconds(5));
        m_packetSinkUp[1]->SetStopTime(m_stopTime - Seconds(5));

        // Download CS5
        Address sinkAddress3(InetSocketAddress(Ipv4Address::GetAny(), 9012));
        m_packetSinkDown[2] = CreateObject<PacketSink>();
        m_packetSinkDown[2]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_packetSinkDown[2]->SetAttribute("Local", AddressValue(sinkAddress3));
        m_packetSinkDown[2]->SetAttribute("Tos",
                                          UintegerValue(Ipv4Header::DscpType::DSCP_CS5 << 2));
        m_node->AddApplication(m_packetSinkDown[2]);
        m_packetSinkDown[2]->SetStartTime(m_startTime + Seconds(5));
        m_packetSinkDown[2]->SetStopTime(m_stopTime - Seconds(5));
        m_packetSinkDown[2]->TraceConnectWithoutContext(
            "Rx",
            MakeBoundCallback(&FlentApplication::TraceReceivedPacket, &m_bytesReceived[2]));
        (*m_output)["results"]["TCP download CS5"] = nlohmann::json::array();
        (*m_output)["raw_values"]["TCP download CS5"] = nlohmann::json::array();
        nlohmann::json data3;
        data3["dur"] = m_stepSize.GetSeconds();
        data3["t"] = (Simulator::Now().GetSeconds() + m_currTime);
        data3["val"] = 0;
        (*m_output)["raw_values"]["TCP download CS5"].push_back(data3);
        Simulator::Schedule(m_stepSize,
                            &FlentApplication::GoodputSamplingDownload,
                            this,
                            "TCP download CS5",
                            2);
        InetSocketAddress localBindAddress3 = InetSocketAddress(localIpv4Address, 9012);
        m_bulkSendDown[2] = CreateObject<BulkSendApplication>();
        m_bulkSendDown[2]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_bulkSendDown[2]->SetAttribute("Remote", AddressValue(localBindAddress3));
        m_bulkSendDown[2]->SetAttribute("MaxBytes", UintegerValue(0));
        m_bulkSendDown[2]->SetAttribute("Tos", UintegerValue(Ipv4Header::DscpType::DSCP_CS5 << 2));
        hostNode->AddApplication(m_bulkSendDown[2]);
        m_bulkSendDown[2]->SetStartTime(m_startTime + Seconds(5));
        m_bulkSendDown[2]->SetStopTime(m_stopTime - Seconds(5));

        // Upload CS5
        hostAddress = InetSocketAddress(hostIpv4Address, 9022);
        m_bulkSendUp[2] = CreateObject<BulkSendApplication>();
        m_bulkSendUp[2]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_bulkSendUp[2]->SetAttribute("Remote", AddressValue(hostAddress));
        m_bulkSendUp[2]->SetAttribute("MaxBytes", UintegerValue(0));
        m_bulkSendUp[2]->SetAttribute("Tos", UintegerValue(Ipv4Header::DscpType::DSCP_CS5 << 2));
        m_node->AddApplication(m_bulkSendUp[2]);
        m_bulkSendUp[2]->SetStartTime(m_startTime + Seconds(5));
        m_bulkSendUp[2]->SetStopTime(m_stopTime - Seconds(5));
        (*m_output)["results"]["TCP upload CS5"] = nlohmann::json::array();
        (*m_output)["raw_values"]["TCP upload CS5"] = nlohmann::json::array();
        nlohmann::json data_up3;
        data_up3["dur"] = m_stepSize.GetSeconds();
        data_up3["t"] = (Simulator::Now().GetSeconds() + m_currTime);
        data_up3["val"] = 0;
        (*m_output)["raw_values"]["TCP upload CS5"].push_back(data_up3);
        m_bulkSendUp[2]->TraceConnectWithoutContext(
            "Tx",
            MakeBoundCallback(&FlentApplication::TraceSentPacket, &m_bytesSent[2]));
        Simulator::Schedule(m_stepSize,
                            &FlentApplication::GoodputSamplingUpload,
                            this,
                            "TCP upload CS5",
                            2);
        Address sinkAddressUp3(InetSocketAddress(Ipv4Address::GetAny(), 9022));
        m_packetSinkUp[2] = CreateObject<PacketSink>();
        m_packetSinkUp[2]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_packetSinkUp[2]->SetAttribute("Local", AddressValue(sinkAddressUp3));
        m_packetSinkUp[2]->SetAttribute("Tos", UintegerValue(Ipv4Header::DscpType::DSCP_CS5 << 2));
        hostNode->AddApplication(m_packetSinkUp[2]);
        m_packetSinkUp[2]->SetStartTime(m_startTime + Seconds(5));
        m_packetSinkUp[2]->SetStopTime(m_stopTime - Seconds(5));

        // Download EF
        Address sinkAddress4(InetSocketAddress(Ipv4Address::GetAny(), 9013));
        m_packetSinkDown[3] = CreateObject<PacketSink>();
        m_packetSinkDown[3]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_packetSinkDown[3]->SetAttribute("Local", AddressValue(sinkAddress4));
        m_packetSinkDown[3]->SetAttribute("Tos", UintegerValue(Ipv4Header::DscpType::DSCP_EF << 2));
        m_node->AddApplication(m_packetSinkDown[3]);
        m_packetSinkDown[3]->SetStartTime(m_startTime + Seconds(5));
        m_packetSinkDown[3]->SetStopTime(m_stopTime - Seconds(5));
        m_packetSinkDown[3]->TraceConnectWithoutContext(
            "Rx",
            MakeBoundCallback(&FlentApplication::TraceReceivedPacket, &m_bytesReceived[3]));
        (*m_output)["results"]["TCP download EF"] = nlohmann::json::array();
        (*m_output)["raw_values"]["TCP download EF"] = nlohmann::json::array();
        nlohmann::json data4;
        data4["dur"] = m_stepSize.GetSeconds();
        data4["t"] = (Simulator::Now().GetSeconds() + m_currTime);
        data4["val"] = 0;
        (*m_output)["raw_values"]["TCP download EF"].push_back(data4);
        Simulator::Schedule(m_stepSize,
                            &FlentApplication::GoodputSamplingDownload,
                            this,
                            "TCP download EF",
                            3);
        InetSocketAddress localBindAddress4 = InetSocketAddress(localIpv4Address, 9013);
        m_bulkSendDown[3] = CreateObject<BulkSendApplication>();
        m_bulkSendDown[3]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_bulkSendDown[3]->SetAttribute("Remote", AddressValue(localBindAddress4));
        m_bulkSendDown[3]->SetAttribute("MaxBytes", UintegerValue(0));
        m_bulkSendDown[3]->SetAttribute("Tos", UintegerValue(Ipv4Header::DscpType::DSCP_EF << 2));
        hostNode->AddApplication(m_bulkSendDown[3]);
        m_bulkSendDown[3]->SetStartTime(m_startTime + Seconds(5));
        m_bulkSendDown[3]->SetStopTime(m_stopTime - Seconds(5));

        // Upload EF
        hostAddress = InetSocketAddress(hostIpv4Address, 9023);
        m_bulkSendUp[3] = CreateObject<BulkSendApplication>();
        m_bulkSendUp[3]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_bulkSendUp[3]->SetAttribute("Remote", AddressValue(hostAddress));
        m_bulkSendUp[3]->SetAttribute("MaxBytes", UintegerValue(0));
        m_bulkSendUp[3]->SetAttribute("Tos", UintegerValue(Ipv4Header::DscpType::DSCP_EF << 2));
        m_node->AddApplication(m_bulkSendUp[3]);
        m_bulkSendUp[3]->SetStartTime(m_startTime + Seconds(5));
        m_bulkSendUp[3]->SetStopTime(m_stopTime - Seconds(5));
        (*m_output)["results"]["TCP upload EF"] = nlohmann::json::array();
        (*m_output)["raw_values"]["TCP upload EF"] = nlohmann::json::array();
        nlohmann::json data_up4;
        data_up4["dur"] = m_stepSize.GetSeconds();
        data_up4["t"] = (Simulator::Now().GetSeconds() + m_currTime);
        data_up4["val"] = 0;
        (*m_output)["raw_values"]["TCP upload EF"].push_back(data_up4);
        m_bulkSendUp[3]->TraceConnectWithoutContext(
            "Tx",
            MakeBoundCallback(&FlentApplication::TraceSentPacket, &m_bytesSent[3]));
        Simulator::Schedule(m_stepSize,
                            &FlentApplication::GoodputSamplingUpload,
                            this,
                            "TCP upload EF",
                            3);
        Address sinkAddressUp4(InetSocketAddress(Ipv4Address::GetAny(), 9023));
        m_packetSinkUp[3] = CreateObject<PacketSink>();
        m_packetSinkUp[3]->SetAttribute("Protocol", StringValue("ns3::TcpSocketFactory"));
        m_packetSinkUp[3]->SetAttribute("Local", AddressValue(sinkAddressUp4));
        m_packetSinkUp[3]->SetAttribute("Tos", UintegerValue(Ipv4Header::DscpType::DSCP_EF << 2));
        hostNode->AddApplication(m_packetSinkUp[3]);
        m_packetSinkUp[3]->SetStartTime(m_startTime + Seconds(5));
        m_packetSinkUp[3]->SetStopTime(m_stopTime - Seconds(5));
    }
}

void
FlentApplication::StopApplication() // Called at time specified by Stop
{
    NS_LOG_FUNCTION(this);
    FillXValues();
    ProcessRawValues();
    if (m_testName == "ping")
    {
        m_ping->TraceDisconnectWithoutContext(
            "Rtt",
            MakeCallback(&FlentApplication::TraceReceivedPing, this));
    }
    else if (m_testName == "tcp_upload")
    {
        m_ping->TraceDisconnectWithoutContext(
            "Rtt",
            MakeCallback(&FlentApplication::TraceReceivedPing, this));
        m_bulkSendUp[0]->TraceDisconnectWithoutContext(
            "Tx",
            MakeBoundCallback(&FlentApplication::TraceSentPacket, &m_bytesSent[0]));
    }
    else if (m_testName == "tcp_download")
    {
        m_ping->TraceDisconnectWithoutContext(
            "Rtt",
            MakeCallback(&FlentApplication::TraceReceivedPing, this));
        m_packetSinkDown[0]->TraceDisconnectWithoutContext(
            "Rx",
            MakeBoundCallback(&FlentApplication::TraceReceivedPacket, &m_bytesReceived[0]));
    }
    else if (m_testName == "rrul")
    {
        m_ping->TraceDisconnectWithoutContext(
            "Rtt",
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
                MakeBoundCallback(&FlentApplication::TraceReceivedPacket, &m_bytesReceived[i]));
            m_bulkSendUp[i]->TraceDisconnectWithoutContext(
                "Tx",
                MakeBoundCallback(&FlentApplication::TraceSentPacket, &m_bytesSent[i]));
        }
    }

    std::string outputPath = m_outputFilename.empty() ? m_testName + ".flent" : m_outputFilename;
    std::ofstream out(outputPath);
    if (out.is_open())
    {
        out << m_output->dump(1) << std::endl;
        out.close();
    }
    else
    {
        NS_LOG_ERROR("Could not open output file: " << outputPath);
    }
}

} // Namespace ns3
