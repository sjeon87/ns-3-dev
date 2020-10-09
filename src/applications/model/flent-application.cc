/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Harsha Sharma (adapted BulkSendApplication
 * and PacketSinkApplication for ns-3 flent)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Harsha Sharma <harshasha256@gmail.com>
 */

#include <chrono>
#include <iostream>
#include <string>
#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/v4ping.h"
#include "ns3/nstime.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/boolean.h"
#include "flent-application.h"
#include "ns3/core-module.h"
#include "ns3/trace-helper.h"
#include "ns3/bulk-send-application.h"
#include "ns3/v4ping.h"
#include "ns3/packet-sink.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-header.h"
#include "ns3/seq-ts-echo-header.h"
#include "ns3/udp-echo-server.h"
#include "ns3/udp-echo-client.h"

namespace ns3 {

class SeqTsEchoHeader;

NS_LOG_COMPONENT_DEFINE ("FlentApplication");

NS_OBJECT_ENSURE_REGISTERED (FlentApplication);

TypeId
FlentApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FlentApplication")
    .SetParent<Application> ()
    .SetGroupName ("Applications") 
    .AddConstructor<FlentApplication> ()
    .AddAttribute ("TestName", "Name of the flent test to be run",
		  StringValue (""),
		  MakeStringAccessor (&FlentApplication::m_testName),
		  MakeStringChecker ())
    .AddAttribute ("ServerAddress", "The address of the destination",
		  AddressValue (),
		  MakeAddressAccessor (&FlentApplication::m_serverAddress),
		  MakeAddressChecker ())
    .AddAttribute ("ClientAddress", "Source address",
		  AddressValue (),
		  MakeAddressAccessor (&FlentApplication::m_clientAddress),
		  MakeAddressChecker ())
    .AddAttribute ("ImageText", "Text to be included in the plot",
		  StringValue (""),
		  MakeStringAccessor (&FlentApplication::m_imageText),
		  MakeStringChecker ())
    .AddAttribute ("ImageName", "Name of the image to save the output plot",
		  StringValue (""),
		  MakeStringAccessor (&FlentApplication::m_imageName),
		  MakeStringChecker ())
    .AddAttribute ("StepSize", "Measurement data point size",
		  TimeValue (Seconds(1)),
		  MakeTimeAccessor (&FlentApplication::m_stepSize),
		  MakeTimeChecker ())
  ;
  return tid;
}


FlentApplication::FlentApplication ()
{
  NS_LOG_FUNCTION (this);
}

FlentApplication::~FlentApplication ()
{
  NS_LOG_FUNCTION (this);
}

void
FlentApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_serverNode = 0;

  // chain up
  Application::DoDispose ();
}

void
FlentApplication::SetTest (std::string testname)
{
  m_testName = testname;
}

void FlentApplication::SetDuration (Time duration)
{
  m_duration = duration;
  m_stopTime = m_startTime + m_duration + Seconds (10);
}

void
FlentApplication::SetServerAddress (Address serverAddress)
{
  m_serverAddress = serverAddress;
}

void
FlentApplication::SetClientAddress (Address clientAddress)
{
  m_clientAddress = clientAddress;
}

void
FlentApplication::SetServerNode (Ptr<Node> serverNode)
{
  m_serverNode = serverNode;
}

void
FlentApplication::SetIncludeText (std::string textInImage)
{
  m_imageText = textInImage;
}

void
FlentApplication::SetOutput (std::string imagename)
{
  m_imageName = imagename;
}

void FlentApplication::SetStepSize (Time stepsize)
{
  m_stepSize = stepsize;
}

std::string FlentApplication::GetUtcFormatTime (int sec) const {
  time_t     now = time (0) + sec;
  struct tm  tstruct;
  char       buf[80];
  tstruct = *gmtime (&now);
  strftime (buf, sizeof (buf), "%Y-%m-%dT%X.000000Z", &tstruct);

  return buf;
}

void FlentApplication::AddMetadata (Json::Value &j)
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
  oss << m_serverAddress;
  std::string hostName = oss.str ();
  j["metadata"]["HOST"] = hostName;
  j["metadata"]["HOSTS"] = Json::Value (Json::arrayValue);
  j["metadata"]["HOSTS"].append (hostName);
  j["metadata"]["HTTP_GETTER_DNS"] = Json::Value::null;
  j["metadata"]["HTTP_GETTER_URLLIST"] = Json::Value::null;
  j["metadata"]["HTTP_GETTER_WORKERS"] = Json::Value::null;
  j["metadata"]["IP_VERSION"] = Json::Value::null;
  j["metadata"]["KERNEL_NAME"] = Json::Value::null;
  j["metadata"]["KERNEL_RELEASE"] = Json::Value::null;
  j["metadata"]["LENGTH"] = m_duration.GetSeconds ();
  j["metadata"]["LOCAL_HOST"] = Json::Value::null;
  j["metadata"]["MODULE_VERSIONS"] = Json::Value::null;
  j["metadata"]["NAME"] = m_testName;
  j["metadata"]["NOTE"] = Json::Value::null;
  j["metadata"]["REMOTE_METADATA"] = Json::Value::null;
  j["metadata"]["STEP_SIZE"] = m_stepSize.GetSeconds ();
  j["metadata"]["TIME"] = GetUtcFormatTime (0);
  j["metadata"]["T0"] = GetUtcFormatTime (0);
  j["metadata"]["TEST_PARAMETERS"] = Json::objectValue;
  j["metadata"]["TITLE"] = Json::Value::null;
  j["metadata"]["TOTAL_LENGTH"] = m_stopTime.GetSeconds ();
  j["version"] = 4;
}

void
FlentApplication::ReceivePing (const Address &address, uint16_t seq, uint8_t ttl, Time t)
{
  Json::Value data;
  double rtt = t.GetSeconds () * 1000;
  data["seq"] = seq;
  data["t"] = (Simulator::Now ().GetSeconds ()+ m_currTime);
  data["val"] = rtt; 
  m_output["raw_values"]["Ping (ms) ICMP"].append(data);
  m_output["results"]["Ping (ms) ICMP"].append(rtt);
  m_output["x_values"].append (Simulator::Now ().GetSeconds ());
}

void
FlentApplication::ReceiveUdpPing (Ptr<const Packet> packet,  const Address &address, const Address &localAddress, const SeqTsEchoHeader &header)
{
  Json::Value data;
  Time t = header.GetTsValue ();
  double rtt = t.GetSeconds () * 1000;
  data["dur"] = m_stepSize.GetSeconds ();
  data["t"] = (Simulator::Now ().GetSeconds ()+ m_currTime);
  data["val"] = rtt;
  m_output["raw_values"]["Ping (ms) UDP BE"].append(data);
  m_output["results"]["Ping (ms) UDP BE"].append(rtt);
}

void
FlentApplication::ReceiveUdpPing2 (Ptr<const Packet> packet,  const Address &address, const Address &localAddress, const SeqTsEchoHeader &header)
{
  Json::Value data;
  Time t = header.GetTsValue ();
  double rtt = t.GetSeconds () * 1000;
  data["dur"] = m_stepSize.GetSeconds ();
  data["t"] = (Simulator::Now ().GetSeconds ()+ m_currTime);
  data["val"] = rtt;
  m_output["raw_values"]["Ping (ms) UDP BK"].append(data);
  m_output["results"]["Ping (ms) UDP BK"].append(rtt);
}

void
FlentApplication::ReceiveUdpPing3 (Ptr<const Packet> packet,  const Address &address, const Address &localAddress, const SeqTsEchoHeader &header)
{
  Json::Value data;
  Time t = header.GetTsValue ();
  double rtt = t.GetSeconds () * 1000;
  data["dur"] = m_stepSize.GetSeconds ();
  data["t"] = (Simulator::Now ().GetSeconds ()+ m_currTime);
  data["val"] = rtt;
  m_output["raw_values"]["Ping (ms) UDP EF"].append(data);
  m_output["results"]["Ping (ms) UDP EF"].append(rtt);
}

void
FlentApplication::SendData1 (Ptr<const Packet> packet)
{
  g_bytesSent1 += packet->GetSize ();
}

void
FlentApplication::SendData2 (Ptr<const Packet> packet)
{
  g_bytesSent2 += packet->GetSize ();
}

void
FlentApplication::SendData3 (Ptr<const Packet> packet)
{
  g_bytesSent3 += packet->GetSize ();
}

void
FlentApplication::SendData4 (Ptr<const Packet> packet)
{
  g_bytesSent4 += packet->GetSize ();
}

void
FlentApplication::ReceiveData1 (Ptr<const Packet> packet, const Address &address)
{
    g_bytesReceived1 += packet->GetSize ();
}

void
FlentApplication::ReceiveData2 (Ptr<const Packet> packet, const Address &address)
{
    g_bytesReceived2 += packet->GetSize ();
}

void
FlentApplication::ReceiveData3 (Ptr<const Packet> packet, const Address &address)
{
    g_bytesReceived3 += packet->GetSize ();
}

void
FlentApplication::ReceiveData4 (Ptr<const Packet> packet, const Address &address)
{
    g_bytesReceived4 += packet->GetSize ();
}

void
FlentApplication::GoodputSampling1 (std::string name) {
  Json::Value data;
  Json::Int64 goodput = (g_bytesSent1 * 8 / m_stepSize.GetSeconds () / 1e6);
  data["dur"] = m_stepSize.GetSeconds ();
  data["t"] = (Simulator::Now ().GetSeconds ()+m_currTime);
  data["val"] = goodput;
  m_output["raw_values"][name].append (data);
  m_output["results"][name].append (goodput);
  g_bytesSent1 = 0;
  Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSampling1, this, name);
}

void
FlentApplication::GoodputSampling2 (std::string name) {
  Json::Value data;
  Json::Int64 goodput = (g_bytesSent2 * 8 / m_stepSize.GetSeconds () / 1e6);
  data["dur"] = m_stepSize.GetSeconds ();
  data["t"] = (Simulator::Now ().GetSeconds ()+m_currTime);
  data["val"] = goodput;
  m_output["raw_values"][name].append (data);
  m_output["results"][name].append (goodput);
  g_bytesSent2 = 0;
  Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSampling2, this, name);
}

void
FlentApplication::GoodputSampling3 (std::string name) {
  Json::Value data;
  Json::Int64 goodput = (g_bytesSent3 * 8 / m_stepSize.GetSeconds () / 1e6);
  data["dur"] = m_stepSize.GetSeconds ();
  data["t"] = (Simulator::Now ().GetSeconds ()+m_currTime);
  data["val"] = goodput;
  m_output["raw_values"][name].append (data);
  m_output["results"][name].append (goodput);
  g_bytesSent3 = 0;
  Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSampling3, this, name);
}

void
FlentApplication::GoodputSampling4 (std::string name) {
  Json::Value data;
  Json::Int64 goodput = (g_bytesSent4 * 8 / m_stepSize.GetSeconds () / 1e6);
  data["dur"] = m_stepSize.GetSeconds ();
  data["t"] = (Simulator::Now ().GetSeconds ()+m_currTime);
  data["val"] = goodput;
  m_output["raw_values"][name].append (data);
  m_output["results"][name].append (goodput);
  g_bytesSent4 = 0;
  Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSampling4, this, name);
}

void
FlentApplication::GoodputSamplingDownload1 (std::string name) {
  Json::Value data;
  Json::Int64 goodput = (g_bytesReceived1 * 8 / m_stepSize.GetSeconds () / 1e6);
  data["dur"] = m_stepSize.GetSeconds ();
  data["t"] = (Simulator::Now ().GetSeconds () + m_currTime);
  data["val"] = goodput;
  m_output["raw_values"][name].append (data);
  m_output["results"][name].append (goodput);
  g_bytesReceived1 = 0;
  Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSamplingDownload1, this, name);
}

void
FlentApplication::GoodputSamplingDownload2 (std::string name) {
  Json::Value data;
  Json::Int64 goodput = (g_bytesReceived2 * 8 / m_stepSize.GetSeconds () / 1e6);
  data["dur"] = m_stepSize.GetSeconds ();
  data["t"] = (Simulator::Now ().GetSeconds () + m_currTime);
  data["val"] = goodput;
  m_output["raw_values"][name].append (data);
  m_output["results"][name].append (goodput);
  g_bytesReceived2 = 0;
  Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSamplingDownload2, this, name);
}

void
FlentApplication::GoodputSamplingDownload3 (std::string name) {
  Json::Value data;
  Json::Int64 goodput = (g_bytesReceived3 * 8 / m_stepSize.GetSeconds () / 1e6);
  data["dur"] = m_stepSize.GetSeconds ();
  data["t"] = (Simulator::Now ().GetSeconds () + m_currTime);
  data["val"] = goodput;
  m_output["raw_values"][name].append (data);
  m_output["results"][name].append (goodput);
  g_bytesReceived3 = 0;
  Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSamplingDownload3, this, name);
}

void
FlentApplication::GoodputSamplingDownload4 (std::string name) {
  Json::Value data;
  Json::Int64 goodput = (g_bytesReceived4 * 8 / m_stepSize.GetSeconds () / 1e6);
  data["dur"] = m_stepSize.GetSeconds ();
  data["t"] = (Simulator::Now ().GetSeconds () + m_currTime);
  data["val"] = goodput;
  m_output["raw_values"][name].append (data);
  m_output["results"][name].append (goodput);
  g_bytesReceived4 = 0;
  Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSamplingDownload4, this, name);
}

//Application Methods
void FlentApplication::StartApplication (void) //Called at time specified by Start
{
  NS_LOG_FUNCTION (this);
  m_currTime = (std::chrono::duration_cast<std::chrono::nanoseconds> (std::chrono::system_clock::now ().time_since_epoch ()).count ()/1000000000);
  FlentApplication::AddMetadata (m_output);

  if (m_testName.compare ("ping") == 0)
    {
      Ipv4Address serverAddr = Ipv4Address::ConvertFrom (m_serverAddress);
      Ptr<V4Ping> m_v4ping = CreateObject<V4Ping> ();
      m_v4ping->SetAttribute ("Remote", Ipv4AddressValue (serverAddr));
      m_v4ping->SetAttribute ("Interval", TimeValue (m_stepSize));
      GetNode ()->AddApplication (m_v4ping);
      m_output["raw_values"]["Ping (ms) ICMP"] = Json::Value (Json::arrayValue);
      m_output["results"]["Ping (ms) ICMP"] = Json::Value (Json::arrayValue);
      m_output["x_values"] = Json::Value (Json::arrayValue);
      
      m_v4ping->TraceConnectWithoutContext ("Rx", MakeCallback (&FlentApplication::ReceivePing, this));
    }
  else if (m_testName.compare ("tcp_upload") == 0)
    {
      Ipv4Address serverAddr = Ipv4Address::ConvertFrom (m_serverAddress);
      Ptr<V4Ping> m_v4ping = CreateObject<V4Ping> ();
      m_v4ping->SetAttribute ("Remote", Ipv4AddressValue (serverAddr));
      m_v4ping->SetAttribute ("Interval", TimeValue (m_stepSize));
      GetNode ()->AddApplication (m_v4ping);
      ApplicationContainer pingContainer;
      pingContainer.Add (m_v4ping);
      pingContainer.Start (m_startTime);
      pingContainer.Stop (m_stopTime);

      m_output["raw_values"]["Ping (ms) ICMP"] = Json::Value (Json::arrayValue);
      m_output["results"]["Ping (ms) ICMP"] = Json::Value (Json::arrayValue);
      m_output["x_values"] = Json::Value (Json::arrayValue);
      
      m_v4ping->TraceConnectWithoutContext ("Rx", MakeCallback (&FlentApplication::ReceivePing, this));

      InetSocketAddress clientAddress = InetSocketAddress (serverAddr, 9);
      Ptr<BulkSendApplication> m_bulkSend = CreateObject<BulkSendApplication> ();
      m_bulkSend->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_bulkSend->SetAttribute ("Remote", AddressValue (clientAddress));
      m_bulkSend->SetAttribute ("MaxBytes", UintegerValue (0));
      GetNode ()->AddApplication (m_bulkSend);
      ApplicationContainer sourceApp;
      sourceApp.Add(m_bulkSend);
      sourceApp.Start (m_startTime + Seconds(5));
      sourceApp.Stop (m_stopTime - Seconds(5));
      m_output["results"]["TCP upload"] = Json::Value (Json::arrayValue);
      m_output["raw_values"]["TCP upload"] = Json::Value (Json::arrayValue);
      Json::Value data;
      data["dur"] = m_stepSize.GetSeconds ();
      data["t"] = (Simulator::Now ().GetSeconds ()+m_currTime);
      data["val"] = 0;
      m_output["raw_values"]["TCP upload"].append (data);
      m_output["results"]["TCP upload"].append (data["val"]);
      m_bulkSend->TraceConnectWithoutContext ("Tx", MakeCallback (&FlentApplication::SendData1, this));
      Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSampling1, this, "TCP upload");

      Address sinkAddress (InetSocketAddress (Ipv4Address::GetAny (), 9));
      Ptr<PacketSink> m_packetSink = CreateObject<PacketSink> ();
      m_packetSink->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_packetSink->SetAttribute ("Local", AddressValue (sinkAddress));
      m_serverNode->AddApplication (m_packetSink);
      ApplicationContainer sinkApp;
      sinkApp.Add (m_packetSink);
      sinkApp.Start (m_startTime + Seconds(5));
      sinkApp.Stop (m_stopTime - Seconds(5));

    }
  else if (m_testName.compare ("tcp_download") == 0)
    {
      Ipv4Address serverAddr = Ipv4Address::ConvertFrom (m_serverAddress);
      Ptr<V4Ping> m_v4ping = CreateObject<V4Ping> ();
      m_v4ping->SetAttribute ("Remote", Ipv4AddressValue (serverAddr));
      m_v4ping->SetAttribute ("Interval", TimeValue (m_stepSize));
      m_serverNode->AddApplication (m_v4ping);
      ApplicationContainer pingContainer;
      pingContainer.Add (m_v4ping);
      pingContainer.Start (m_startTime);
      pingContainer.Stop (m_stopTime);

      m_output["raw_values"]["Ping (ms) ICMP"] = Json::Value (Json::arrayValue);
      m_output["results"]["Ping (ms) ICMP"] = Json::Value (Json::arrayValue);
      m_output["x_values"] = Json::Value (Json::arrayValue);

      m_v4ping->TraceConnectWithoutContext ("Rx", MakeCallback (&FlentApplication::ReceivePing, this));
      Address sinkAddress (InetSocketAddress (Ipv4Address::GetAny (), 9));
      Ptr<PacketSink> m_packetSink = CreateObject<PacketSink> ();
      m_packetSink->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_packetSink->SetAttribute ("Local", AddressValue (sinkAddress));
      GetNode ()->AddApplication (m_packetSink);
      ApplicationContainer sinkApp;
      sinkApp.Add (m_packetSink);
      sinkApp.Start (m_startTime + Seconds(5));
      sinkApp.Stop (m_stopTime - Seconds(5));
      m_packetSink->TraceConnectWithoutContext ("Rx", MakeCallback (&FlentApplication::ReceiveData1, this));
      m_output["results"]["TCP download"] = Json::Value (Json::arrayValue);
      m_output["raw_values"]["TCP download"] = Json::Value (Json::arrayValue);
      Json::Value data;
      data["dur"] = m_stepSize.GetSeconds ();
      data["t"] = (Simulator::Now ().GetSeconds ()+m_currTime);
      data["val"] = 0;
      m_output["raw_values"]["TCP download"].append (data);
      m_output["results"]["TCP download"].append (data["val"]);
      Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSamplingDownload1, this, "TCP download");

      InetSocketAddress clientAddress = InetSocketAddress (serverAddr, 9);
      Ptr<BulkSendApplication> m_bulkSend = CreateObject<BulkSendApplication> ();
      m_bulkSend->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_bulkSend->SetAttribute ("Remote", AddressValue (clientAddress));
      m_bulkSend->SetAttribute ("MaxBytes", UintegerValue (0));
      m_serverNode->AddApplication (m_bulkSend);
      ApplicationContainer sourceApp;
      sourceApp.Add(m_bulkSend);
      sourceApp.Start (m_startTime + Seconds(5));
      sourceApp.Stop (m_stopTime - Seconds(5));
    }
  else if (m_testName.compare ("rrul") == 0)
    {
      Ipv4Address serverAddr = Ipv4Address::ConvertFrom (m_serverAddress);
      Ptr<V4Ping>  m_v4ping = CreateObject<V4Ping> ();
      m_v4ping->SetAttribute ("Remote", Ipv4AddressValue (serverAddr));
      m_v4ping->SetAttribute ("Interval", TimeValue (m_stepSize));
      m_serverNode->AddApplication (m_v4ping);
      ApplicationContainer pingContainer;
      pingContainer.Add (m_v4ping);
      pingContainer.Start (m_startTime);
      pingContainer.Stop (m_stopTime);

      m_output["raw_values"]["Ping (ms) ICMP"] = Json::Value (Json::arrayValue);
      m_output["results"]["Ping (ms) ICMP"] = Json::Value (Json::arrayValue);
      m_output["x_values"] = Json::Value (Json::arrayValue);
      m_v4ping->TraceConnectWithoutContext ("Rx", MakeCallback (&FlentApplication::ReceivePing, this));

      uint16_t port = 9;
      bool enableSeqTsEchoHeader = true;
      Ptr<UdpEchoServer> m_udpserver = CreateObject<UdpEchoServer> ();
      m_udpserver->SetAttribute ("Port", UintegerValue (port));
      m_udpserver->SetAttribute ("EnableSeqTsEchoHeader", BooleanValue (enableSeqTsEchoHeader));
      m_serverNode->AddApplication (m_udpserver);
      ApplicationContainer apps;
      apps.Add (m_udpserver);
      apps.Start (m_startTime);
      apps.Stop (m_stopTime);
      uint32_t packetSize = 1024;
      uint32_t maxPacketCount = 200;
      Ipv4Address clientAddr = Ipv4Address::ConvertFrom (m_clientAddress);
      Ptr<UdpEchoClient>  m_udpclient = CreateObject<UdpEchoClient> ();
      m_udpclient->SetAttribute ("RemoteAddress", AddressValue (clientAddr)); 
      m_udpclient->SetAttribute ("RemotePort", UintegerValue (port));
      m_udpclient->SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
      m_udpclient->SetAttribute ("Interval", TimeValue (m_stepSize));
      m_udpclient->SetAttribute ("PacketSize", UintegerValue (packetSize));
      m_udpclient->SetAttribute ("EnableSeqTsEchoHeader", BooleanValue (enableSeqTsEchoHeader));
      GetNode ()->AddApplication (m_udpclient);
      ApplicationContainer apps2;
      apps2.Add(m_udpclient);
      apps2.Start (m_startTime);
      apps2.Stop (m_stopTime);
      m_output["raw_values"]["Ping (ms) UDP BE"] = Json::Value (Json::arrayValue);
      m_output["results"]["Ping (ms) UDP BE"] = Json::Value (Json::arrayValue);
      m_udpclient->TraceConnectWithoutContext ("RxWithSeqTsEchoHeader", MakeCallback (&FlentApplication::ReceiveUdpPing, this));

      port = 10;
      Ptr<UdpEchoServer> m_udpserver2 = CreateObject<UdpEchoServer> ();
      m_udpserver2->SetAttribute ("Port", UintegerValue (port));
      m_udpserver2->SetAttribute ("EnableSeqTsEchoHeader", BooleanValue (enableSeqTsEchoHeader));
      m_serverNode->AddApplication (m_udpserver2);
      ApplicationContainer apps3;
      apps3.Add (m_udpserver2);
      apps3.Start (m_startTime);
      apps3.Stop (m_stopTime);
      Ptr<UdpEchoClient>  m_udpclient2 = CreateObject<UdpEchoClient> ();
      m_udpclient2->SetAttribute ("RemoteAddress", AddressValue (clientAddr)); 
      m_udpclient2->SetAttribute ("RemotePort", UintegerValue (port));
      m_udpclient2->SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
      m_udpclient2->SetAttribute ("Interval", TimeValue (m_stepSize));
      m_udpclient2->SetAttribute ("PacketSize", UintegerValue (packetSize));
      m_udpclient2->SetAttribute ("EnableSeqTsEchoHeader", BooleanValue (enableSeqTsEchoHeader));
      GetNode ()->AddApplication (m_udpclient2);
      ApplicationContainer apps4;
      apps4.Add(m_udpclient2);
      apps4.Start (m_startTime);
      apps4.Stop (m_stopTime);
      m_output["raw_values"]["Ping (ms) UDP BK"] = Json::Value (Json::arrayValue);
      m_output["results"]["Ping (ms) UDP BK"] = Json::Value (Json::arrayValue);
      m_udpclient->TraceConnectWithoutContext ("RxWithSeqTsEchoHeader", MakeCallback (&FlentApplication::ReceiveUdpPing2, this));

      port = 11;
      Ptr<UdpEchoServer> m_udpserver3 = CreateObject<UdpEchoServer> ();
      m_udpserver3->SetAttribute ("Port", UintegerValue (port));
      m_udpserver3->SetAttribute ("EnableSeqTsEchoHeader", BooleanValue (enableSeqTsEchoHeader));
      m_serverNode->AddApplication (m_udpserver3);
      ApplicationContainer apps5;
      apps5.Add (m_udpserver3);
      apps5.Start (m_startTime);
      apps5.Stop (m_stopTime);
      Ptr<UdpEchoClient>  m_udpclient3 = CreateObject<UdpEchoClient> ();
      m_udpclient3->SetAttribute ("RemoteAddress", AddressValue (clientAddr)); 
      m_udpclient3->SetAttribute ("RemotePort", UintegerValue (port));
      m_udpclient3->SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
      m_udpclient3->SetAttribute ("Interval", TimeValue (m_stepSize));
      m_udpclient3->SetAttribute ("PacketSize", UintegerValue (packetSize));
      m_udpclient3->SetAttribute ("EnableSeqTsEchoHeader", BooleanValue (enableSeqTsEchoHeader));
      GetNode ()->AddApplication (m_udpclient3);
      ApplicationContainer apps6;
      apps6.Add(m_udpclient3);
      apps6.Start (m_startTime);
      apps6.Stop (m_stopTime);
      m_output["raw_values"]["Ping (ms) UDP EF"] = Json::Value (Json::arrayValue);
      m_output["results"]["Ping (ms) UDP EF"] = Json::Value (Json::arrayValue);
      m_udpclient->TraceConnectWithoutContext ("RxWithSeqTsEchoHeader", MakeCallback (&FlentApplication::ReceiveUdpPing3, this));

      Address sinkAddress (InetSocketAddress (Ipv4Address::GetAny (), 10));
      Ptr<PacketSink> m_packetSink = CreateObject<PacketSink> ();
      m_packetSink->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_packetSink->SetAttribute ("Local", AddressValue (sinkAddress));
      GetNode ()->AddApplication (m_packetSink);
      ApplicationContainer sinkApp;
      sinkApp.Add (m_packetSink);
      sinkApp.Start (m_startTime + Seconds(5));
      sinkApp.Stop (m_stopTime - Seconds(5));
      m_packetSink->TraceConnectWithoutContext ("Rx", MakeCallback (&FlentApplication::ReceiveData1, this));
      m_output["results"]["TCP download BE"] = Json::Value (Json::arrayValue);
      m_output["raw_values"]["TCP download BE"] = Json::Value (Json::arrayValue);
      Json::Value data;
      data["dur"] = m_stepSize.GetSeconds ();
      data["t"] = (Simulator::Now ().GetSeconds ()+m_currTime);
      data["val"] = 0;
      m_output["raw_values"]["TCP download BE"].append (data);
      m_output["results"]["TCP download BE"].append (data["val"]);
      Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSamplingDownload1, this, "TCP download BE");
      InetSocketAddress clientAddress = InetSocketAddress (serverAddr, 10);
      clientAddress.SetTos (Ipv4Header::DscpType::DscpDefault << 2);
      Ptr<BulkSendApplication> m_bulkSend = CreateObject<BulkSendApplication> ();
      m_bulkSend->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_bulkSend->SetAttribute ("Remote", AddressValue (clientAddress));
      m_bulkSend->SetAttribute ("MaxBytes", UintegerValue (0));
      m_serverNode->AddApplication (m_bulkSend);
      ApplicationContainer sourceApp;
      sourceApp.Add(m_bulkSend);
      sourceApp.Start (m_startTime + Seconds(5));
      sourceApp.Stop (m_stopTime - Seconds(5));
      
      clientAddress = InetSocketAddress (clientAddr, 10);
      clientAddress.SetTos (Ipv4Header::DscpType::DscpDefault << 2);
      Ptr<BulkSendApplication> m_bulkSendUp = CreateObject<BulkSendApplication> ();
      m_bulkSendUp->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_bulkSendUp->SetAttribute ("Remote", AddressValue (clientAddress));
      m_bulkSendUp->SetAttribute ("MaxBytes", UintegerValue (0));
      GetNode ()->AddApplication (m_bulkSendUp);
      ApplicationContainer sourceAppUp;
      sourceAppUp.Add(m_bulkSendUp);
      sourceAppUp.Start (m_startTime + Seconds(5));
      sourceAppUp.Stop (m_stopTime - Seconds(5));
      m_output["results"]["TCP upload BE"] = Json::Value (Json::arrayValue);
      m_output["raw_values"]["TCP upload BE"] = Json::Value (Json::arrayValue);
      Json::Value data_up;
      data_up["dur"] = m_stepSize.GetSeconds ();
      data_up["t"] = (Simulator::Now ().GetSeconds ()+m_currTime);
      data_up["val"] = 0;
      m_output["raw_values"]["TCP upload BE"].append (data_up);
      m_output["results"]["TCP upload BE"].append (data_up["val"]);
      m_bulkSendUp->TraceConnectWithoutContext ("Tx", MakeCallback (&FlentApplication::SendData1, this));
      Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSampling1, this, "TCP upload BE");
      Address sinkAddressUp (InetSocketAddress (Ipv4Address::GetAny (), 10));
      Ptr<PacketSink> m_packetSinkUp = CreateObject<PacketSink> ();
      m_packetSinkUp->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_packetSinkUp->SetAttribute ("Local", AddressValue (sinkAddressUp));
      m_serverNode->AddApplication (m_packetSinkUp);
      ApplicationContainer sinkAppUp;
      sinkAppUp.Add (m_packetSinkUp);
      sinkAppUp.Start (m_startTime + Seconds(5));
      sinkAppUp.Stop (m_stopTime - Seconds(5));

      Address sinkAddress2 (InetSocketAddress (Ipv4Address::GetAny (), 9));
      Ptr<PacketSink> m_packetSink2 = CreateObject<PacketSink> ();
      m_packetSink2->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_packetSink2->SetAttribute ("Local", AddressValue (sinkAddress2));
      GetNode ()->AddApplication (m_packetSink2);
      ApplicationContainer sinkApp2;
      sinkApp2.Add (m_packetSink2);
      sinkApp2.Start (m_startTime + Seconds(5));
      sinkApp2.Stop (m_stopTime - Seconds(5));
      m_packetSink2->TraceConnectWithoutContext ("Rx", MakeCallback (&FlentApplication::ReceiveData2, this));
      m_output["results"]["TCP download BK"] = Json::Value (Json::arrayValue);
      m_output["raw_values"]["TCP download BK"] = Json::Value (Json::arrayValue);
      Json::Value data2;
      data2["dur"] = m_stepSize.GetSeconds ();
      data2["t"] = (Simulator::Now ().GetSeconds ()+m_currTime);
      data2["val"] = 0;
      m_output["raw_values"]["TCP download BK"].append (data2);
      m_output["results"]["TCP download BK"].append (data2["val"]);
      Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSamplingDownload2, this, "TCP download BK");
      InetSocketAddress clientAddress2 = InetSocketAddress (serverAddr, 9);
      clientAddress2.SetTos (Ipv4Header::DscpType::DSCP_CS1 << 2);
      Ptr<BulkSendApplication> m_bulkSend2 = CreateObject<BulkSendApplication> ();
      m_bulkSend2->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_bulkSend2->SetAttribute ("Remote", AddressValue (clientAddress2));
      m_bulkSend2->SetAttribute ("MaxBytes", UintegerValue (0));
      m_serverNode->AddApplication (m_bulkSend2);
      ApplicationContainer sourceApp2;
      sourceApp2.Add(m_bulkSend2);
      sourceApp2.Start (m_startTime + Seconds(5));
      sourceApp2.Stop (m_stopTime - Seconds(5));
      
      clientAddress = InetSocketAddress (clientAddr, 11);
      clientAddress.SetTos (Ipv4Header::DscpType::DSCP_CS1 << 2);
      Ptr<BulkSendApplication> m_bulkSendUp2 = CreateObject<BulkSendApplication> ();
      m_bulkSendUp2->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_bulkSendUp2->SetAttribute ("Remote", AddressValue (clientAddress));
      m_bulkSendUp2->SetAttribute ("MaxBytes", UintegerValue (0));
      GetNode ()->AddApplication (m_bulkSendUp2);
      ApplicationContainer sourceAppUp2;
      sourceAppUp2.Add(m_bulkSendUp2);
      sourceAppUp2.Start (m_startTime + Seconds(5));
      sourceAppUp2.Stop (m_stopTime - Seconds(5));
      m_output["results"]["TCP upload BK"] = Json::Value (Json::arrayValue);
      m_output["raw_values"]["TCP upload BK"] = Json::Value (Json::arrayValue);
      Json::Value data_up2;
      data_up2["dur"] = m_stepSize.GetSeconds ();
      data_up2["t"] = (Simulator::Now ().GetSeconds ()+m_currTime);
      data_up2["val"] = 0;
      m_output["raw_values"]["TCP upload BK"].append (data_up2);
      m_output["results"]["TCP upload BK"].append (data_up2["val"]);
      m_bulkSendUp->TraceConnectWithoutContext ("Tx", MakeCallback (&FlentApplication::SendData2, this));
      Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSampling2, this, "TCP upload BK");
      Address sinkAddressUp2 (InetSocketAddress (Ipv4Address::GetAny (), 11));
      Ptr<PacketSink> m_packetSinkUp2 = CreateObject<PacketSink> ();
      m_packetSinkUp2->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_packetSinkUp2->SetAttribute ("Local", AddressValue (sinkAddressUp2));
      m_serverNode->AddApplication (m_packetSinkUp2);
      ApplicationContainer sinkAppUp2;
      sinkAppUp2.Add (m_packetSinkUp2);
      sinkAppUp2.Start (m_startTime + Seconds(5));
      sinkAppUp2.Stop (m_stopTime - Seconds(5));

      Address sinkAddress3 (InetSocketAddress (Ipv4Address::GetAny (), 11));
      Ptr<PacketSink> m_packetSink3 = CreateObject<PacketSink> ();
      m_packetSink3->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_packetSink3->SetAttribute ("Local", AddressValue (sinkAddress3));
      GetNode ()->AddApplication (m_packetSink3);
      ApplicationContainer sinkApp3;
      sinkApp3.Add (m_packetSink3);
      sinkApp3.Start (m_startTime + Seconds(5));
      sinkApp3.Stop (m_stopTime - Seconds(5));
      m_packetSink3->TraceConnectWithoutContext ("Rx", MakeCallback (&FlentApplication::ReceiveData3, this));
      m_output["results"]["TCP download CS5"] = Json::Value (Json::arrayValue);
      m_output["raw_values"]["TCP download CS5"] = Json::Value (Json::arrayValue);
      Json::Value data3;
      data3["dur"] = m_stepSize.GetSeconds ();
      data3["t"] = (Simulator::Now ().GetSeconds ()+m_currTime);
      data3["val"] = 0;
      m_output["raw_values"]["TCP download CS5"].append (data3);
      m_output["results"]["TCP download CS5"].append (data3["val"]);
      Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSamplingDownload3, this, "TCP download CS5");
      InetSocketAddress clientAddress3 = InetSocketAddress (serverAddr, 11);
      clientAddress3.SetTos (Ipv4Header::DscpType::DSCP_CS5 << 2);
      Ptr<BulkSendApplication> m_bulkSend3 = CreateObject<BulkSendApplication> ();
      m_bulkSend3->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_bulkSend3->SetAttribute ("Remote", AddressValue (clientAddress3));
      m_bulkSend3->SetAttribute ("MaxBytes", UintegerValue (0));
      m_serverNode->AddApplication (m_bulkSend3);
      ApplicationContainer sourceApp3;
      sourceApp3.Add(m_bulkSend3);
      sourceApp3.Start (m_startTime + Seconds(5));
      sourceApp3.Stop (m_stopTime - Seconds(5));

      clientAddress = InetSocketAddress (clientAddr, 12);
      clientAddress.SetTos (Ipv4Header::DscpType::DSCP_CS5 << 2);
      Ptr<BulkSendApplication> m_bulkSendUp3 = CreateObject<BulkSendApplication> ();
      m_bulkSendUp3->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_bulkSendUp3->SetAttribute ("Remote", AddressValue (clientAddress));
      m_bulkSendUp3->SetAttribute ("MaxBytes", UintegerValue (0));
      GetNode ()->AddApplication (m_bulkSendUp3);
      ApplicationContainer sourceAppUp3;
      sourceAppUp3.Add(m_bulkSendUp3);
      sourceAppUp3.Start (m_startTime + Seconds(5));
      sourceAppUp3.Stop (m_stopTime - Seconds(5));
      m_output["results"]["TCP upload CS5"] = Json::Value (Json::arrayValue);
      m_output["raw_values"]["TCP upload CS5"] = Json::Value (Json::arrayValue);
      Json::Value data_up3;
      data_up3["dur"] = m_stepSize.GetSeconds ();
      data_up3["t"] = (Simulator::Now ().GetSeconds ()+m_currTime);
      data_up3["val"] = 0;
      m_output["raw_values"]["TCP upload CS5"].append (data_up3);
      m_output["results"]["TCP upload CS5"].append (data_up3["val"]);
      m_bulkSendUp3->TraceConnectWithoutContext ("Tx", MakeCallback (&FlentApplication::SendData3, this));
      Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSampling3, this, "TCP upload CS5");
      Address sinkAddressUp3 (InetSocketAddress (Ipv4Address::GetAny (), 12));
      Ptr<PacketSink> m_packetSinkUp3 = CreateObject<PacketSink> ();
      m_packetSinkUp3->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_packetSinkUp3->SetAttribute ("Local", AddressValue (sinkAddressUp3));
      m_serverNode->AddApplication (m_packetSinkUp3);
      ApplicationContainer sinkAppUp3;
      sinkAppUp3.Add (m_packetSinkUp3);
      sinkAppUp3.Start (m_startTime + Seconds(5));
      sinkAppUp3.Stop (m_stopTime - Seconds(5));

      Address sinkAddress4 (InetSocketAddress (Ipv4Address::GetAny (), 12));
      Ptr<PacketSink> m_packetSink4 = CreateObject<PacketSink> ();
      m_packetSink4->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_packetSink4->SetAttribute ("Local", AddressValue (sinkAddress4));
      GetNode ()->AddApplication (m_packetSink4);
      ApplicationContainer sinkApp4;
      sinkApp4.Add (m_packetSink4);
      sinkApp4.Start (m_startTime + Seconds(5));
      sinkApp4.Stop (m_stopTime - Seconds(5));
      m_packetSink4->TraceConnectWithoutContext ("Rx", MakeCallback (&FlentApplication::ReceiveData4, this));
      m_output["results"]["TCP download EF"] = Json::Value (Json::arrayValue);
      m_output["raw_values"]["TCP download EF"] = Json::Value (Json::arrayValue);
      Json::Value data4;
      data4["dur"] = m_stepSize.GetSeconds ();
      data4["t"] = (Simulator::Now ().GetSeconds ()+m_currTime);
      data4["val"] = 0;
      m_output["raw_values"]["TCP download EF"].append (data4);
      m_output["results"]["TCP download EF"].append (data4["val"]);
      Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSamplingDownload4, this, "TCP download EF");
      InetSocketAddress clientAddress4 = InetSocketAddress (serverAddr, 12);
      clientAddress4.SetTos (Ipv4Header::DscpType::DSCP_EF << 2);
      Ptr<BulkSendApplication> m_bulkSend4 = CreateObject<BulkSendApplication> ();
      m_bulkSend4->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_bulkSend4->SetAttribute ("Remote", AddressValue (clientAddress4));
      m_bulkSend4->SetAttribute ("MaxBytes", UintegerValue (0));
      m_serverNode->AddApplication (m_bulkSend4);
      ApplicationContainer sourceApp4;
      sourceApp4.Add(m_bulkSend4);
      sourceApp4.Start (m_startTime + Seconds(5));
      sourceApp4.Stop (m_stopTime - Seconds(5));

      clientAddress = InetSocketAddress (clientAddr, 13);
      clientAddress.SetTos (Ipv4Header::DscpType::DSCP_EF << 2);
      Ptr<BulkSendApplication> m_bulkSendUp4 = CreateObject<BulkSendApplication> ();
      m_bulkSendUp4->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_bulkSendUp4->SetAttribute ("Remote", AddressValue (clientAddress));
      m_bulkSendUp4->SetAttribute ("MaxBytes", UintegerValue (0));
      GetNode ()->AddApplication (m_bulkSendUp4);
      ApplicationContainer sourceAppUp4;
      sourceAppUp4.Add(m_bulkSendUp4);
      sourceAppUp4.Start (m_startTime + Seconds(5));
      sourceAppUp4.Stop (m_stopTime - Seconds(5));
      m_output["results"]["TCP upload EF"] = Json::Value (Json::arrayValue);
      m_output["raw_values"]["TCP upload EF"] = Json::Value (Json::arrayValue);
      Json::Value data_up4;
      data_up4["dur"] = m_stepSize.GetSeconds ();
      data_up4["t"] = (Simulator::Now ().GetSeconds ()+m_currTime);
      data_up4["val"] = 0;
      m_output["raw_values"]["TCP upload EF"].append (data_up4);
      m_output["results"]["TCP upload EF"].append (data_up4["val"]);
      m_bulkSendUp4->TraceConnectWithoutContext ("Tx", MakeCallback (&FlentApplication::SendData4, this));
      Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSampling4, this, "TCP upload EF");
      Address sinkAddressUp4 (InetSocketAddress (Ipv4Address::GetAny (), 13));
      Ptr<PacketSink> m_packetSinkUp4 = CreateObject<PacketSink> ();
      m_packetSinkUp4->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_packetSinkUp4->SetAttribute ("Local", AddressValue (sinkAddressUp4));
      m_serverNode->AddApplication (m_packetSinkUp4);
      ApplicationContainer sinkAppUp4;
      sinkAppUp4.Add (m_packetSinkUp4);
      sinkAppUp4.Start (m_startTime + Seconds(5));
      sinkAppUp4.Stop (m_stopTime - Seconds(5));
    }

}

void FlentApplication::StopApplication (void) // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);
  Simulator::Schedule (MilliSeconds (1), &Simulator::Stop);
  AsciiTraceHelper ascii;
  if (m_testName.compare ("tcp_upload") == 0)
    {
      //m_v4ping->TraceDisconnectWithoutContext ("Rx", MakeCallback (&FlentApplication::ReceivePing, this));
      //m_bulkSend->TraceDisconnectWithoutContext ("Tx", MakeCallback (&FlentApplication::SendData1, this));
      Ptr<OutputStreamWrapper> streamOutput = ascii.CreateFileStream (m_testName + ".flent");
      *streamOutput->GetStream () << m_output << std::endl;
    }
  else if (m_testName.compare ("tcp_download") == 0)
    {
      //m_v4ping->TraceDisconnectWithoutContext ("Rx", MakeCallback (&FlentApplication::ReceivePing, this));
      //m_packetSink->TraceDisconnectWithoutContext ("Rx", MakeCallback (&FlentApplication::ReceiveData1, this));
      Ptr<OutputStreamWrapper> streamOutput = ascii.CreateFileStream (m_testName + ".flent");
      *streamOutput->GetStream () << m_output << std::endl;
    }
  else if (m_testName.compare ("rrul") == 0)
    {
      //m_v4ping->TraceDisconnectWithoutContext ("Rx", MakeCallback (&FlentApplication::ReceivePing, this));
      //m_packetSink->TraceDisconnectWithoutContext ("Rx", MakeCallback (&FlentApplication::ReceiveData, this));
      Ptr<OutputStreamWrapper> streamOutput = ascii.CreateFileStream (m_testName + ".flent");
      *streamOutput->GetStream () << m_output << std::endl;
    }
}

} // Namespace ns3
