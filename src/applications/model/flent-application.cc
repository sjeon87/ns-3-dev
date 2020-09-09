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
#include "ns3/bulk-send-helper.h"
#include "ns3/v4ping.h"
#include "ns3/packet-sink.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FlentApplication");

NS_OBJECT_ENSURE_REGISTERED (FlentApplication);

uint32_t g_bytesSent1 = 0;
uint32_t g_bytesSent2 = 0;
uint32_t g_bytesSent3 = 0;
uint32_t g_bytesSent4 = 0;
uint32_t g_bytesReceived1 = 0;
uint32_t g_bytesReceived2 = 0;
uint32_t g_bytesReceived3 = 0;
uint32_t g_bytesReceived4 = 0;

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
  m_v4ping = 0;
  m_packetSink = 0;
  m_bulkSend = 0;
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
      m_v4ping = CreateObject<V4Ping> ();
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
      m_v4ping = CreateObject<V4Ping> ();
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
      BulkSendHelper tcp ("ns3::TcpSocketFactory", clientAddress);
      tcp.SetAttribute ("MaxBytes", UintegerValue (0));
      ApplicationContainer sourceApp = tcp.Install (GetNode ());
      sourceApp.Start (Seconds (m_startTime.GetSeconds () + 5));
      sourceApp.Stop (Seconds (m_stopTime.GetSeconds () - 5));
      m_bulkSend = DynamicCast<BulkSendApplication> (sourceApp.Get (0));
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
      PacketSinkHelper netserver ("ns3::TcpSocketFactory", sinkAddress);
      netserver.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
      ApplicationContainer sinkApp = netserver.Install (m_serverNode);
      sinkApp.Start (Seconds (m_startTime.GetSeconds () + 5));
      sinkApp.Stop (Seconds (m_stopTime.GetSeconds () - 5));
      m_packetSink = DynamicCast<PacketSink> (sinkApp.Get (0));

    }
  else if (m_testName.compare ("tcp_download") == 0)
    {
      Ipv4Address serverAddr = Ipv4Address::ConvertFrom (m_serverAddress);
      m_v4ping = CreateObject<V4Ping> ();
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
      PacketSinkHelper netserver ("ns3::TcpSocketFactory", sinkAddress);
      netserver.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
      ApplicationContainer sinkApp = netserver.Install (GetNode ());
      sinkApp.Start (Seconds (m_startTime.GetSeconds () + 5));
      sinkApp.Stop (Seconds (m_stopTime.GetSeconds () - 6));
      m_packetSink = DynamicCast<PacketSink> (sinkApp.Get (0));
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
      BulkSendHelper tcp ("ns3::TcpSocketFactory", clientAddress);
      tcp.SetAttribute ("MaxBytes", UintegerValue (0));
      ApplicationContainer sourceApp = tcp.Install (m_serverNode);
      sourceApp.Start (Seconds (m_startTime.GetSeconds () + 5));
      sourceApp.Stop (Seconds (m_stopTime.GetSeconds () - 5));
      m_bulkSend = DynamicCast<BulkSendApplication> (sourceApp.Get (0));
    }
  else if (m_testName.compare ("rrul") == 0)
    {
      Ipv4Address serverAddr = Ipv4Address::ConvertFrom (m_serverAddress);
      m_v4ping = CreateObject<V4Ping> ();
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

      Address sinkAddress (InetSocketAddress (Ipv4Address::GetAny (), 10));
      PacketSinkHelper netserver ("ns3::TcpSocketFactory", sinkAddress);
      netserver.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
      ApplicationContainer sinkApp = netserver.Install (GetNode ());
      sinkApp.Start (Seconds (m_startTime.GetSeconds () + 5));
      sinkApp.Stop (Seconds (m_stopTime.GetSeconds () - 6));
      m_packetSink = DynamicCast<PacketSink> (sinkApp.Get (0));
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
      BulkSendHelper tcp ("ns3::TcpSocketFactory", clientAddress);
      tcp.SetAttribute ("MaxBytes", UintegerValue (0));
      ApplicationContainer sourceApp = tcp.Install (m_serverNode);
      sourceApp.Start (Seconds (m_startTime.GetSeconds () + 5));
      sourceApp.Stop (Seconds (m_stopTime.GetSeconds () - 5));
      m_bulkSend = DynamicCast<BulkSendApplication> (sourceApp.Get (0));
      
      Ipv4Address clientAddr = Ipv4Address::ConvertFrom (m_clientAddress);
      clientAddress = InetSocketAddress (clientAddr, 10);
      clientAddress.SetTos (Ipv4Header::DscpType::DscpDefault << 2);
      BulkSendHelper tcp_upload ("ns3::TcpSocketFactory", clientAddress);
      tcp_upload.SetAttribute ("MaxBytes", UintegerValue (0));
      ApplicationContainer sourceAppUp = tcp_upload.Install (GetNode ());
      sourceAppUp.Start (Seconds (m_startTime.GetSeconds () + 5));
      sourceAppUp.Stop (Seconds (m_stopTime.GetSeconds () - 5));
      Ptr<BulkSendApplication> m_bulkSendUp = DynamicCast<BulkSendApplication> (sourceAppUp.Get (0));
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
      PacketSinkHelper netserverUp ("ns3::TcpSocketFactory", sinkAddressUp);
      netserverUp.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
      ApplicationContainer sinkAppUp = netserverUp.Install (m_serverNode);
      sinkAppUp.Start (Seconds (m_startTime.GetSeconds () + 5));
      sinkAppUp.Stop (Seconds (m_stopTime.GetSeconds () - 5));
      Ptr<PacketSink> m_packetSinkUp = DynamicCast<PacketSink> (sinkAppUp.Get (0));

      Address sinkAddress2 (InetSocketAddress (Ipv4Address::GetAny (), 9));
      PacketSinkHelper netserver2 ("ns3::TcpSocketFactory", sinkAddress2);
      netserver2.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
      ApplicationContainer sinkApp2 = netserver2.Install (GetNode ());
      sinkApp2.Start (Seconds (m_startTime.GetSeconds () + 5));
      sinkApp2.Stop (Seconds (m_stopTime.GetSeconds () - 6));
      Ptr<PacketSink> m_packetSink2 = DynamicCast<PacketSink> (sinkApp2.Get (0));
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
      BulkSendHelper tcp2 ("ns3::TcpSocketFactory", clientAddress2);
      tcp2.SetAttribute ("MaxBytes", UintegerValue (0));
      ApplicationContainer sourceApp2 = tcp2.Install (m_serverNode);
      sourceApp2.Start (Seconds (m_startTime.GetSeconds () + 5));
      sourceApp2.Stop (Seconds (m_stopTime.GetSeconds () - 5));
      Ptr<BulkSendApplication> m_bulkSend2 = DynamicCast<BulkSendApplication> (sourceApp2.Get (0));
      
      clientAddress = InetSocketAddress (clientAddr, 11);
      clientAddress.SetTos (Ipv4Header::DscpType::DSCP_CS1 << 2);
      BulkSendHelper tcp_upload2 ("ns3::TcpSocketFactory", clientAddress);
      tcp_upload2.SetAttribute ("MaxBytes", UintegerValue (0));
      ApplicationContainer sourceAppUp2 = tcp_upload2.Install (GetNode ());
      sourceAppUp2.Start (Seconds (m_startTime.GetSeconds () + 5));
      sourceAppUp2.Stop (Seconds (m_stopTime.GetSeconds () - 5));
      Ptr<BulkSendApplication> m_bulkSendUp2 = DynamicCast<BulkSendApplication> (sourceAppUp2.Get (0));
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
      PacketSinkHelper netserverUp2 ("ns3::TcpSocketFactory", sinkAddressUp2);
      netserverUp2.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
      ApplicationContainer sinkAppUp2 = netserverUp2.Install (m_serverNode);
      sinkAppUp2.Start (Seconds (m_startTime.GetSeconds () + 5));
      sinkAppUp2.Stop (Seconds (m_stopTime.GetSeconds () - 5));
      Ptr<PacketSink> m_packetSinkUp2 = DynamicCast<PacketSink> (sinkAppUp2.Get (0));

      Address sinkAddress3 (InetSocketAddress (Ipv4Address::GetAny (), 11));
      PacketSinkHelper netserver3 ("ns3::TcpSocketFactory", sinkAddress3);
      netserver3.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
      ApplicationContainer sinkApp3 = netserver3.Install (GetNode ());
      sinkApp3.Start (Seconds (m_startTime.GetSeconds () + 5));
      sinkApp3.Stop (Seconds (m_stopTime.GetSeconds () - 6));
      Ptr<PacketSink> m_packetSink3 = DynamicCast<PacketSink> (sinkApp3.Get (0));
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
      BulkSendHelper tcp3 ("ns3::TcpSocketFactory", clientAddress3);
      tcp3.SetAttribute ("MaxBytes", UintegerValue (0));
      ApplicationContainer sourceApp3 = tcp3.Install (m_serverNode);
      sourceApp3.Start (Seconds (m_startTime.GetSeconds () + 5));
      sourceApp3.Stop (Seconds (m_stopTime.GetSeconds () - 5));
      Ptr<BulkSendApplication> m_bulkSend3 = DynamicCast<BulkSendApplication> (sourceApp3.Get (0));

      clientAddress = InetSocketAddress (clientAddr, 12);
      clientAddress.SetTos (Ipv4Header::DscpType::DSCP_CS5 << 2);
      BulkSendHelper tcp_upload3 ("ns3::TcpSocketFactory", clientAddress);
      tcp_upload3.SetAttribute ("MaxBytes", UintegerValue (0));
      ApplicationContainer sourceAppUp3 = tcp_upload3.Install (GetNode ());
      sourceAppUp3.Start (Seconds (m_startTime.GetSeconds () + 5));
      sourceAppUp3.Stop (Seconds (m_stopTime.GetSeconds () - 5));
      Ptr<BulkSendApplication> m_bulkSendUp3 = DynamicCast<BulkSendApplication> (sourceAppUp3.Get (0));
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
      PacketSinkHelper netserverUp3 ("ns3::TcpSocketFactory", sinkAddressUp3);
      netserverUp3.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
      ApplicationContainer sinkAppUp3 = netserverUp3.Install (m_serverNode);
      sinkAppUp3.Start (Seconds (m_startTime.GetSeconds () + 5));
      sinkAppUp3.Stop (Seconds (m_stopTime.GetSeconds () - 5));
      Ptr<PacketSink> m_packetSinkUp3 = DynamicCast<PacketSink> (sinkAppUp3.Get (0));

      Address sinkAddress4 (InetSocketAddress (Ipv4Address::GetAny (), 12));
      PacketSinkHelper netserver4 ("ns3::TcpSocketFactory", sinkAddress4);
      netserver4.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
      ApplicationContainer sinkApp4 = netserver4.Install (GetNode ());
      sinkApp4.Start (Seconds (m_startTime.GetSeconds () + 5));
      sinkApp4.Stop (Seconds (m_stopTime.GetSeconds () - 6));
      Ptr<PacketSink> m_packetSink4 = DynamicCast<PacketSink> (sinkApp4.Get (0));
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
      BulkSendHelper tcp4 ("ns3::TcpSocketFactory", clientAddress4);
      tcp4.SetAttribute ("MaxBytes", UintegerValue (0));
      ApplicationContainer sourceApp4 = tcp4.Install (m_serverNode);
      sourceApp4.Start (Seconds (m_startTime.GetSeconds () + 5));
      sourceApp4.Stop (Seconds (m_stopTime.GetSeconds () - 5));
      Ptr<BulkSendApplication> m_bulkSend4 = DynamicCast<BulkSendApplication> (sourceApp4.Get (0));

      clientAddress = InetSocketAddress (clientAddr, 13);
      clientAddress.SetTos (Ipv4Header::DscpType::DSCP_EF << 2);
      BulkSendHelper tcp_upload4 ("ns3::TcpSocketFactory", clientAddress);
      tcp_upload4.SetAttribute ("MaxBytes", UintegerValue (0));
      ApplicationContainer sourceAppUp4 = tcp_upload4.Install (GetNode ());
      sourceAppUp4.Start (Seconds (m_startTime.GetSeconds () + 5));
      sourceAppUp4.Stop (Seconds (m_stopTime.GetSeconds () - 5));
      Ptr<BulkSendApplication> m_bulkSendUp4 = DynamicCast<BulkSendApplication> (sourceAppUp4.Get (0));
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
      PacketSinkHelper netserverUp4 ("ns3::TcpSocketFactory", sinkAddressUp4);
      netserverUp4.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
      ApplicationContainer sinkAppUp4 = netserverUp4.Install (m_serverNode);
      sinkAppUp4.Start (Seconds (m_startTime.GetSeconds () + 5));
      sinkAppUp4.Stop (Seconds (m_stopTime.GetSeconds () - 5));
      Ptr<PacketSink> m_packetSinkUp4 = DynamicCast<PacketSink> (sinkAppUp4.Get (0));
    }

}

void FlentApplication::StopApplication (void) // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);
  Simulator::Schedule (MilliSeconds (1), &Simulator::Stop);
  AsciiTraceHelper ascii;
  if (m_testName.compare ("tcp_upload") == 0)
    {
      m_v4ping->TraceDisconnectWithoutContext ("Rx", MakeCallback (&FlentApplication::ReceivePing, this));
      m_bulkSend->TraceDisconnectWithoutContext ("Tx", MakeCallback (&FlentApplication::SendData1, this));
      Ptr<OutputStreamWrapper> streamOutput = ascii.CreateFileStream (m_testName + ".flent");
      *streamOutput->GetStream () << m_output << std::endl;
    }
  else if (m_testName.compare ("tcp_download") == 0)
    {
      m_v4ping->TraceDisconnectWithoutContext ("Rx", MakeCallback (&FlentApplication::ReceivePing, this));
      m_packetSink->TraceDisconnectWithoutContext ("Rx", MakeCallback (&FlentApplication::ReceiveData1, this));
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
