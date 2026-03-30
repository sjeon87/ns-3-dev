/*
 * Copyright (c) 2022 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 *
 * Authors: Bhaskar Kataria <bhaskar.k7920@gmail.com>
 *          Tom Henderson <tomh@tomh.org>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */

/*
 This test suite:
   - Runs the first 4 tests for sanity check and generating the flent files required for the Flent
 file integrity and Flent results test. Flent files generated from the first 4 tests can be reused
 again for the Flent file integrity and Flent results test.
   - Runs the Flent file integrity test which reads the files from first 4 tests and check if
 metadata required for flent-gui is available
   - Runs the Flent result test which reads the files from the 4 tests and calculate average
 throughput and average ICMP latency. The result is checked if its within bounds obtained from
 results at the time of writing the test suite.
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/ns3jsoncpp.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;

/**
 * Flent rrul test, checks if the test is running.
 */
class FlentApplicationRrul : public TestCase
{
  public:
    FlentApplicationRrul();
    virtual ~FlentApplicationRrul();

  private:
    virtual void DoRun(void);
};

FlentApplicationRrul::FlentApplicationRrul()
    : TestCase("Test flent rrul")
{
}

FlentApplicationRrul::~FlentApplicationRrul()
{
}

void
FlentApplicationRrul::DoRun(void)
{
    std::string testName = "rrul";
    Time rtt = MilliSeconds(80);
    DataRate bw("50Mbps");
    Time length = Seconds(5);
    Time delay = Seconds(0);

    // 2 MB of TCP buffer
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1 << 21));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1 << 21));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpCubic"));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));

    NodeContainer n;
    n.Create(4); // client <-> router1 <-> router2 <-> server
    // Create node containers for configuring individual links
    NodeContainer n0; // Group the client and router1 together
    n0.Add(n.Get(0));
    n0.Add(n.Get(1));
    NodeContainer n1; // Group the routers together
    n1.Add(n.Get(1));
    n1.Add(n.Get(2));
    NodeContainer n2; // Group the router2 and server together
    n2.Add(n.Get(2));
    n2.Add(n.Get(3));

    PointToPointHelper deviceHelper;
    DataRate edgeRate(100 * bw.GetBitRate());
    deviceHelper.SetDeviceAttribute("DataRate", DataRateValue(edgeRate));
    deviceHelper.SetChannelAttribute("Delay", TimeValue(MicroSeconds(1)));
    deviceHelper.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p"));
    NetDeviceContainer devices0;
    devices0 = deviceHelper.Install(n0);
    NetDeviceContainer devices2;
    devices2 = deviceHelper.Install(n2);
    // The middle link has the bandwidth and delay constraints
    NetDeviceContainer devices1;
    deviceHelper.SetDeviceAttribute("DataRate", DataRateValue(bw));
    deviceHelper.SetChannelAttribute("Delay", TimeValue(rtt / 2));
    devices1 = deviceHelper.Install(n1);

    // Configure the IP and traffic control layers
    InternetStackHelper stack;
    stack.InstallAll();

    TrafficControlHelper tch;
    tch.SetRootQueueDisc("ns3::FqCoDelQueueDisc");
    Config::SetDefault("ns3::FqCoDelQueueDisc::MaxSize", QueueSizeValue(QueueSize("200p")));
    tch.SetQueueLimits("ns3::DynamicQueueLimits"); // enable BQL
    QueueDiscContainer qdiscs;
    qdiscs = tch.Install(devices0);
    qdiscs = tch.Install(devices1);
    qdiscs = tch.Install(devices2);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces0 = address.Assign(devices0);
    address.NewNetwork();
    Ipv4InterfaceContainer interfaces1 = address.Assign(devices1);
    address.NewNetwork();
    Ipv4InterfaceContainer interfaces2 = address.Assign(devices2);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Configure with the help of FlentHelper
    FlentHelper flentHelper(testName, interfaces2.GetAddress(1));
    flentHelper.SetAttribute("StartTime", TimeValue(delay));
    flentHelper.SetAttribute("StepSize", TimeValue(Seconds(0.2)));
    flentHelper.SetAttribute("Length", TimeValue(length));

    ApplicationContainer flent = flentHelper.Install(n.Get(0));
    flent.Start(delay);
    flent.Stop(delay + length + Seconds(10));

    Simulator::Stop(delay + length + Seconds(10) + Seconds(1));

    Simulator::Run();
    Simulator::Destroy();

    return;
}

/**
 * Flent tcp_upload test, checks if the test is running.
 */
class FlentApplicationTcpUpload : public TestCase
{
  public:
    FlentApplicationTcpUpload();
    virtual ~FlentApplicationTcpUpload();

  private:
    virtual void DoRun(void);
};

FlentApplicationTcpUpload::FlentApplicationTcpUpload()
    : TestCase("Test flent TCP Upload")
{
}

FlentApplicationTcpUpload::~FlentApplicationTcpUpload()
{
}

void
FlentApplicationTcpUpload::DoRun(void)
{
    std::string testName = "tcp_upload";
    Time rtt = MilliSeconds(80);
    DataRate bw("50Mbps");
    Time length = Seconds(5);
    Time delay = Seconds(0);

    // 2 MB of TCP buffer
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1 << 21));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1 << 21));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpCubic"));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));

    NodeContainer n;
    n.Create(4); // client <-> router1 <-> router2 <-> server
    // Create node containers for configuring individual links
    NodeContainer n0; // Group the client and router1 together
    n0.Add(n.Get(0));
    n0.Add(n.Get(1));
    NodeContainer n1; // Group the routers together
    n1.Add(n.Get(1));
    n1.Add(n.Get(2));
    NodeContainer n2; // Group the router2 and server together
    n2.Add(n.Get(2));
    n2.Add(n.Get(3));

    PointToPointHelper deviceHelper;
    DataRate edgeRate(100 * bw.GetBitRate());
    deviceHelper.SetDeviceAttribute("DataRate", DataRateValue(edgeRate));
    deviceHelper.SetChannelAttribute("Delay", TimeValue(MicroSeconds(1)));
    deviceHelper.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p"));
    NetDeviceContainer devices0;
    devices0 = deviceHelper.Install(n0);
    NetDeviceContainer devices2;
    devices2 = deviceHelper.Install(n2);
    // The middle link has the bandwidth and delay constraints
    NetDeviceContainer devices1;
    deviceHelper.SetDeviceAttribute("DataRate", DataRateValue(bw));
    deviceHelper.SetChannelAttribute("Delay", TimeValue(rtt / 2));
    devices1 = deviceHelper.Install(n1);

    // Configure the IP and traffic control layers
    InternetStackHelper stack;
    stack.InstallAll();

    TrafficControlHelper tch;
    tch.SetRootQueueDisc("ns3::FqCoDelQueueDisc");
    Config::SetDefault("ns3::FqCoDelQueueDisc::MaxSize", QueueSizeValue(QueueSize("200p")));
    tch.SetQueueLimits("ns3::DynamicQueueLimits"); // enable BQL
    QueueDiscContainer qdiscs;
    qdiscs = tch.Install(devices0);
    qdiscs = tch.Install(devices1);
    qdiscs = tch.Install(devices2);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces0 = address.Assign(devices0);
    address.NewNetwork();
    Ipv4InterfaceContainer interfaces1 = address.Assign(devices1);
    address.NewNetwork();
    Ipv4InterfaceContainer interfaces2 = address.Assign(devices2);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Configure with the help of FlentHelper
    FlentHelper flentHelper(testName, interfaces2.GetAddress(1));
    flentHelper.SetAttribute("StartTime", TimeValue(delay));
    flentHelper.SetAttribute("StepSize", TimeValue(Seconds(0.2)));
    flentHelper.SetAttribute("Length", TimeValue(length));

    ApplicationContainer flent = flentHelper.Install(n.Get(0));
    flent.Start(delay);
    flent.Stop(delay + length + Seconds(10));

    Simulator::Stop(delay + length + Seconds(10) + Seconds(1));

    Simulator::Run();
    Simulator::Destroy();

    return;
}

/**
 * Flent tcp_download test, checks if the test is running.
 */
class FlentApplicationTcpDownload : public TestCase
{
  public:
    FlentApplicationTcpDownload();
    virtual ~FlentApplicationTcpDownload();

  private:
    virtual void DoRun(void);
};

FlentApplicationTcpDownload::FlentApplicationTcpDownload()
    : TestCase("Test flent TCP Download")
{
}

FlentApplicationTcpDownload::~FlentApplicationTcpDownload()
{
}

void
FlentApplicationTcpDownload::DoRun(void)
{
    std::string testName = "tcp_download";
    Time rtt = MilliSeconds(80);
    DataRate bw("50Mbps");
    Time length = Seconds(5);
    Time delay = Seconds(0);

    // 2 MB of TCP buffer
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1 << 21));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1 << 21));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpCubic"));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));

    NodeContainer n;
    n.Create(4); // client <-> router1 <-> router2 <-> server
    // Create node containers for configuring individual links
    NodeContainer n0; // Group the client and router1 together
    n0.Add(n.Get(0));
    n0.Add(n.Get(1));
    NodeContainer n1; // Group the routers together
    n1.Add(n.Get(1));
    n1.Add(n.Get(2));
    NodeContainer n2; // Group the router2 and server together
    n2.Add(n.Get(2));
    n2.Add(n.Get(3));

    PointToPointHelper deviceHelper;
    DataRate edgeRate(100 * bw.GetBitRate());
    deviceHelper.SetDeviceAttribute("DataRate", DataRateValue(edgeRate));
    deviceHelper.SetChannelAttribute("Delay", TimeValue(MicroSeconds(1)));
    deviceHelper.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p"));
    NetDeviceContainer devices0;
    devices0 = deviceHelper.Install(n0);
    NetDeviceContainer devices2;
    devices2 = deviceHelper.Install(n2);
    // The middle link has the bandwidth and delay constraints
    NetDeviceContainer devices1;
    deviceHelper.SetDeviceAttribute("DataRate", DataRateValue(bw));
    deviceHelper.SetChannelAttribute("Delay", TimeValue(rtt / 2));
    devices1 = deviceHelper.Install(n1);

    // Configure the IP and traffic control layers
    InternetStackHelper stack;
    stack.InstallAll();

    TrafficControlHelper tch;
    tch.SetRootQueueDisc("ns3::FqCoDelQueueDisc");
    Config::SetDefault("ns3::FqCoDelQueueDisc::MaxSize", QueueSizeValue(QueueSize("200p")));
    tch.SetQueueLimits("ns3::DynamicQueueLimits"); // enable BQL
    QueueDiscContainer qdiscs;
    qdiscs = tch.Install(devices0);
    qdiscs = tch.Install(devices1);
    qdiscs = tch.Install(devices2);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces0 = address.Assign(devices0);
    address.NewNetwork();
    Ipv4InterfaceContainer interfaces1 = address.Assign(devices1);
    address.NewNetwork();
    Ipv4InterfaceContainer interfaces2 = address.Assign(devices2);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Configure with the help of FlentHelper
    FlentHelper flentHelper(testName, interfaces2.GetAddress(1));
    flentHelper.SetAttribute("StartTime", TimeValue(delay));
    flentHelper.SetAttribute("StepSize", TimeValue(Seconds(0.2)));
    flentHelper.SetAttribute("Length", TimeValue(length));

    ApplicationContainer flent = flentHelper.Install(n.Get(0));
    flent.Start(delay);
    flent.Stop(delay + length + Seconds(10));

    Simulator::Stop(delay + length + Seconds(10) + Seconds(1));

    Simulator::Run();
    Simulator::Destroy();

    return;
}

/**
 * Flent ping test, checks if the test is running.
 */
class FlentApplicationPing : public TestCase
{
  public:
    FlentApplicationPing();
    virtual ~FlentApplicationPing();

  private:
    virtual void DoRun(void);
};

FlentApplicationPing::FlentApplicationPing()
    : TestCase("Test flent Ping")
{
}

FlentApplicationPing::~FlentApplicationPing()
{
}

void
FlentApplicationPing::DoRun(void)
{
    std::string testName = "ping";
    Time rtt = MilliSeconds(80);
    DataRate bw("50Mbps");
    Time length = Seconds(5);
    Time delay = Seconds(0);

    // 2 MB of TCP buffer
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1 << 21));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1 << 21));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpCubic"));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));

    NodeContainer n;
    n.Create(4); // client <-> router1 <-> router2 <-> server
    // Create node containers for configuring individual links
    NodeContainer n0; // Group the client and router1 together
    n0.Add(n.Get(0));
    n0.Add(n.Get(1));
    NodeContainer n1; // Group the routers together
    n1.Add(n.Get(1));
    n1.Add(n.Get(2));
    NodeContainer n2; // Group the router2 and server together
    n2.Add(n.Get(2));
    n2.Add(n.Get(3));

    PointToPointHelper deviceHelper;
    DataRate edgeRate(100 * bw.GetBitRate());
    deviceHelper.SetDeviceAttribute("DataRate", DataRateValue(edgeRate));
    deviceHelper.SetChannelAttribute("Delay", TimeValue(MicroSeconds(1)));
    deviceHelper.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p"));
    NetDeviceContainer devices0;
    devices0 = deviceHelper.Install(n0);
    NetDeviceContainer devices2;
    devices2 = deviceHelper.Install(n2);
    // The middle link has the bandwidth and delay constraints
    NetDeviceContainer devices1;
    deviceHelper.SetDeviceAttribute("DataRate", DataRateValue(bw));
    deviceHelper.SetChannelAttribute("Delay", TimeValue(rtt / 2));
    devices1 = deviceHelper.Install(n1);

    // Configure the IP and traffic control layers
    InternetStackHelper stack;
    stack.InstallAll();

    TrafficControlHelper tch;
    tch.SetRootQueueDisc("ns3::FqCoDelQueueDisc");
    Config::SetDefault("ns3::FqCoDelQueueDisc::MaxSize", QueueSizeValue(QueueSize("200p")));
    tch.SetQueueLimits("ns3::DynamicQueueLimits"); // enable BQL
    QueueDiscContainer qdiscs;
    qdiscs = tch.Install(devices0);
    qdiscs = tch.Install(devices1);
    qdiscs = tch.Install(devices2);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces0 = address.Assign(devices0);
    address.NewNetwork();
    Ipv4InterfaceContainer interfaces1 = address.Assign(devices1);
    address.NewNetwork();
    Ipv4InterfaceContainer interfaces2 = address.Assign(devices2);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Configure with the help of FlentHelper
    FlentHelper flentHelper(testName, interfaces2.GetAddress(1));
    flentHelper.SetAttribute("StartTime", TimeValue(delay));
    flentHelper.SetAttribute("StepSize", TimeValue(Seconds(0.2)));
    flentHelper.SetAttribute("Length", TimeValue(length));

    ApplicationContainer flent = flentHelper.Install(n.Get(0));
    flent.Start(delay);
    flent.Stop(delay + length + Seconds(10));

    Simulator::Stop(delay + length + Seconds(10) + Seconds(1));

    Simulator::Run();
    Simulator::Destroy();

    return;
}

/**
 * Flent file integrity test, checks if the created flent files have the required fields.
 */
class FlentApplicationFileIntegrity : public TestCase
{
  public:
    FlentApplicationFileIntegrity();
    virtual ~FlentApplicationFileIntegrity();

  private:
    virtual void DoRun(void);
};

FlentApplicationFileIntegrity::FlentApplicationFileIntegrity()
    : TestCase("Test flent file integrity")
{
}

FlentApplicationFileIntegrity::~FlentApplicationFileIntegrity()
{
}

void
FlentApplicationFileIntegrity::DoRun(void)
{
    std::vector<std::string> flentFiles{"rrul.flent",
                                        "tcp_upload.flent",
                                        "tcp_download.flent",
                                        "ping.flent"};
    for (unsigned int i = 0; i < flentFiles.size(); i++)
    {
        Json::Value root;
        std::ifstream test(flentFiles[i], std::ifstream::binary);
        test >> root;
        Json::Value::const_iterator itr_name = root.begin();
        NS_TEST_ASSERT_MSG_EQ((itr_name++).key().asString(),
                              "metadata",
                              "in file " + flentFiles[i] + " metadata not available");
        NS_TEST_ASSERT_MSG_EQ((itr_name++).key().asString(),
                              "raw_values",
                              "in file " + flentFiles[i] + " raw_values not available");
        NS_TEST_ASSERT_MSG_EQ((itr_name++).key().asString(),
                              "results",
                              "in file " + flentFiles[i] + " results not available");
        NS_TEST_ASSERT_MSG_EQ((itr_name++).key().asString(),
                              "version",
                              "in file " + flentFiles[i] + " version not available");
        NS_TEST_ASSERT_MSG_EQ((itr_name).key().asString(),
                              "x_values",
                              "in file " + flentFiles[i] + " x_values not available");

        for (auto itrRawValues = root["raw_values"].begin(), itrResults = root["results"].begin();
             itrRawValues != root["raw_values"].end();
             itrRawValues++, itrResults++)
        {
            NS_TEST_ASSERT_MSG_EQ(itrRawValues.key().asString(),
                                  itrResults.key().asString(),
                                  flentFiles[i] + " number of results and raw values do not match");
            NS_TEST_ASSERT_MSG_EQ(root["results"][itrResults.key().asString()].size(),
                                  root["x_values"].size(),
                                  flentFiles[i] +
                                      " number of results and raw values does not match");
        }
    }
    return;
}

/**
 * Flent results test, checks if the average throughput and average ICMP latency are as expected.
 * Bounds are kept as per the results obtained from running the test and calculating the average
 * throughput and average ICMP latency.
 */
class FlentApplicationResults : public TestCase
{
  public:
    FlentApplicationResults();
    virtual ~FlentApplicationResults();

  private:
    virtual void DoRun(void);
};

FlentApplicationResults::FlentApplicationResults()
    : TestCase("Test flent average throughput and average ICMP Ping latency")
{
}

FlentApplicationResults::~FlentApplicationResults()
{
}

void
FlentApplicationResults::DoRun(void)
{
    std::vector<std::string> flentFiles{"rrul.flent",
                                        "tcp_upload.flent",
                                        "tcp_download.flent",
                                        "ping.flent"};
    int count = 0;
    double throughput = 0.0;
    double pingLatency = 0.0;
    for (unsigned int i = 0; i < flentFiles.size(); i++)
    {
        Json::Value root;
        std::ifstream test(flentFiles[i], std::ifstream::binary);
        test >> root;
        count = 0;
        throughput = 0.0;
        pingLatency = 0.0;
        if (flentFiles[i] == "tcp_upload.flent")
        {
            for (auto itrTcpUpload = root["results"]["TCP upload"].begin();
                 itrTcpUpload != root["results"]["TCP upload"].end();
                 itrTcpUpload++)
            {
                if ((*itrTcpUpload).asString() == "0.0")
                {
                    continue;
                }
                count += 1;
                throughput += std::atof(((*itrTcpUpload).asString()).c_str());
            }
            NS_TEST_ASSERT_MSG_LT(throughput / count,
                                  50,
                                  flentFiles[i] + " TCP upload throughput should be less than 50");
            NS_TEST_ASSERT_MSG_GT(throughput / count,
                                  43,
                                  flentFiles[i] +
                                      " TCP upload throughput should be greater than 43");

            count = 0;
            for (auto itrPing = root["results"]["Ping (ms) ICMP"].begin();
                 itrPing != root["results"]["Ping (ms) ICMP"].end();
                 itrPing++)
            {
                if ((*itrPing).asString() == "0.0")
                {
                    continue;
                }
                count += 1;
                pingLatency += std::atof(((*itrPing).asString()).c_str());
            }
            NS_TEST_ASSERT_MSG_LT(pingLatency / count,
                                  82,
                                  flentFiles[i] + " Ping latency should be less than 82");
            NS_TEST_ASSERT_MSG_GT(pingLatency / count,
                                  80,
                                  flentFiles[i] +
                                      " Ping latency throughput should be greater than 80");
        }
        else if (flentFiles[i] == "tcp_download.flent")
        {
            for (auto itrTcpDownload = root["results"]["TCP download"].begin();
                 itrTcpDownload != root["results"]["TCP download"].end();
                 itrTcpDownload++)
            {
                if ((*itrTcpDownload).asString() == "0.0")
                {
                    continue;
                }
                count += 1;
                throughput += std::atof(((*itrTcpDownload).asString()).c_str());
            }
            NS_TEST_ASSERT_MSG_LT(throughput / count,
                                  50,
                                  flentFiles[i] +
                                      " TCP Download throughput should be less than 50");
            NS_TEST_ASSERT_MSG_GT(throughput / count,
                                  43,
                                  flentFiles[i] +
                                      " TCP Download throughput should be greater than 43");

            count = 0;
            for (auto itrPing = root["results"]["Ping (ms) ICMP"].begin();
                 itrPing != root["results"]["Ping (ms) ICMP"].end();
                 itrPing++)
            {
                if ((*itrPing).asString() == "0.0")
                {
                    continue;
                }
                count += 1;
                pingLatency += std::atof(((*itrPing).asString()).c_str());
            }
            NS_TEST_ASSERT_MSG_LT(pingLatency / count,
                                  82,
                                  flentFiles[i] + " Ping latency should be less than 82");
            NS_TEST_ASSERT_MSG_GT(pingLatency / count,
                                  80,
                                  flentFiles[i] +
                                      " Ping latency throughput should be greater than 80");
        }
        else if (flentFiles[i] == "ping.flent")
        {
            for (auto itrPing = root["results"]["Ping (ms) ICMP"].begin();
                 itrPing != root["results"]["Ping (ms) ICMP"].end();
                 itrPing++)
            {
                if ((*itrPing).asString() == "0.0")
                {
                    continue;
                }
                count += 1;
                pingLatency += std::atof(((*itrPing).asString()).c_str());
            }
            NS_TEST_ASSERT_MSG_LT(pingLatency / count,
                                  82,
                                  flentFiles[i] + " Ping latency should be less than 82");
            NS_TEST_ASSERT_MSG_GT(pingLatency / count,
                                  80,
                                  flentFiles[i] +
                                      " Ping latency throughput should be greater than 80");
        }
        else if (flentFiles[i] == "rrul.flent")
        {
            for (auto itrTcpDownload = root["results"]["TCP download BE"].begin();
                 itrTcpDownload != root["results"]["TCP download BE"].end();
                 itrTcpDownload++)
            {
                if ((*itrTcpDownload).asString() == "0.0")
                {
                    continue;
                }
                count += 1;
                throughput += std::atof(((*itrTcpDownload).asString()).c_str());
            }
            NS_TEST_ASSERT_MSG_LT(throughput / count,
                                  12,
                                  flentFiles[i] +
                                      " TCP Download throughput should be less than 12");
            NS_TEST_ASSERT_MSG_GT(throughput / count,
                                  11,
                                  flentFiles[i] +
                                      " TCP Download throughput should be greater than 11");

            count = 0;
            throughput = 0.0;
            for (auto itrTcpDownload = root["results"]["TCP download BK"].begin();
                 itrTcpDownload != root["results"]["TCP download BK"].end();
                 itrTcpDownload++)
            {
                if ((*itrTcpDownload).asString() == "0.0")
                {
                    continue;
                }
                count += 1;
                throughput += std::atof(((*itrTcpDownload).asString()).c_str());
            }
            NS_TEST_ASSERT_MSG_LT(throughput / count,
                                  12,
                                  flentFiles[i] +
                                      " TCP Download throughput should be less than 12");
            NS_TEST_ASSERT_MSG_GT(throughput / count,
                                  11,
                                  flentFiles[i] +
                                      " TCP Download throughput should be greater than 11");

            count = 0;
            for (auto itrPing = root["results"]["Ping (ms) ICMP"].begin();
                 itrPing != root["results"]["Ping (ms) ICMP"].end();
                 itrPing++)
            {
                if ((*itrPing).asString() == "0.0")
                {
                    continue;
                }
                count += 1;
                pingLatency += std::atof(((*itrPing).asString()).c_str());
            }
            NS_TEST_ASSERT_MSG_LT(pingLatency / count,
                                  84,
                                  flentFiles[i] + " Ping latency should be less than 84");
            NS_TEST_ASSERT_MSG_GT(pingLatency / count,
                                  80,
                                  flentFiles[i] +
                                      " Ping latency throughput should be greater than 80");
        }
    }
    return;
}

class FlentApplicationTestSuite : public TestSuite
{
  public:
    FlentApplicationTestSuite();
};

FlentApplicationTestSuite::FlentApplicationTestSuite()
    : TestSuite("flent-application", ns3::TestSuite::Type::UNIT)
{
    AddTestCase(new FlentApplicationRrul, ns3::TestCase::Duration::QUICK);
    AddTestCase(new FlentApplicationTcpUpload, ns3::TestCase::Duration::QUICK);
    AddTestCase(new FlentApplicationTcpDownload, ns3::TestCase::Duration::QUICK);
    AddTestCase(new FlentApplicationPing, ns3::TestCase::Duration::QUICK);
    AddTestCase(new FlentApplicationFileIntegrity, ns3::TestCase::Duration::QUICK);
    AddTestCase(new FlentApplicationResults, ns3::TestCase::Duration::QUICK);
}

static FlentApplicationTestSuite
    g_FlentApplicationTestSuite; //!< Static variable for test initialization
