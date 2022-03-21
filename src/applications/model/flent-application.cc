/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Georgia Institute of Technology
 * Copyright (c) 2020 Harsha Sharma : Flent application
 * Copyright (c) 2021 NITK Surathkal
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
 *         (adapted from bulk-send-application.cc and
 *          packet-sink-application.cc written by George F. Riley)
 *
 * Modified by: Ameya Deshpande <ameyanrd@outlook.com>
 */

#include <chrono>
#include <iostream>
#include "ns3/address.h"
#include "ns3/v4ping.h"
#include "ns3/tcp-socket-factory.h"
#include "flent-application.h"
#include "ns3/core-module.h"
#include "ns3/trace-helper.h"
#include "ns3/loopback-net-device.h"
#include "ns3/bulk-send-application.h"
#include "ns3/packet-sink.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/udp-echo-server.h"
#include "ns3/udp-echo-client.h"
#include "ns3/application-container.h"

namespace ns3 {

class SeqTsEchoHeader;

NS_LOG_COMPONENT_DEFINE ("FlentApplication");

NS_OBJECT_ENSURE_REGISTERED (FlentApplication);

namespace {

std::vector<uint32_t> m_bytesSent {std::vector<uint32_t> (4, 0)}; //!< sent data counters
std::vector<uint32_t> m_bytesReceived {std::vector<uint32_t> (4, 0)}; //!< receive data counters

/*
 * \brief sink for packet transmissions.
 * \param counter counter of bytes sent
 * \param packet Pointer to packet sent
 */
void TraceSentPacket (uint32_t *counter, Ptr<const Packet> packet)
{
  *counter += packet->GetSize ();
}

/*
 * \brief sink for packet received.
 * \param counter counter of bytes received
 * \param packet Pointer to packet received
 * \param address Address of the sender
 */
void TraceReceivedPacket (uint32_t *counter, Ptr<const Packet> packet, const Address &address)
{
  *counter += packet->GetSize ();
}

} // anonymous namespace

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
    .AddAttribute ("Length", "Test length",
      TimeValue (Seconds(60)),
      MakeTimeAccessor (&FlentApplication::m_length),
		  MakeTimeChecker ())
    .AddAttribute ("HostAddress", "The address of the remote host",
		  AddressValue (),
		  MakeAddressAccessor (&FlentApplication::m_hostAddress),
		  MakeAddressChecker ())
    .AddAttribute ("LocalBindAddress", "The address of the local host",
		  AddressValue (),
		  MakeAddressAccessor (&FlentApplication::m_localBindAddress),
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
		  TimeValue (MilliSeconds(200)),
		  MakeTimeAccessor (&FlentApplication::m_stepSize),
		  MakeTimeChecker (MilliSeconds (50), Seconds (1)))
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
FlentApplication::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);

  m_hostNode = GetHostNode (Ipv4Address::ConvertFrom (m_hostAddress));

  if (m_localBindAddress.IsInvalid ())
    {
      Ptr<Ipv4L3Protocol> ip = m_node->GetObject<Ipv4L3Protocol> ();
      if(ip)
        {
          for (uint32_t deviceId = 0; deviceId < m_node->GetNDevices (); deviceId++)
            {
              Ptr<NetDevice> device = m_node->GetDevice (deviceId);
              // If this is not a loopback device add the IP address to the map
              if ( !DynamicCast<LoopbackNetDevice>(device) )
                {
                  int32_t interfaceIndex = (ip)->GetInterfaceForDevice (device);
                  if (interfaceIndex != -1)
                    {
                      m_localBindAddress = ip->GetAddress (interfaceIndex, 0).GetLocal ();
                      break;
                    }
                }
            }
        }
    }

  // Override the Stop Time set for the Application Container
  m_stopTime = m_startTime + m_length + Seconds (10);

  Application::DoInitialize ();
}

void
FlentApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  // chain up
  Application::DoDispose ();
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
  oss << m_hostAddress;
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
  j["metadata"]["LENGTH"] = m_length.GetSeconds ();
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

Ptr<Node>
FlentApplication::GetHostNode (Ipv4Address hostAddress) const
{
  NS_LOG_FUNCTION (this << hostAddress);

  for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
    {
      Ptr<Node> node = *it;
      Ptr<Ipv4L3Protocol> ip = node->GetObject<Ipv4L3Protocol> ();

      if(ip)
        {
          for (uint32_t deviceId = 0; deviceId < node->GetNDevices (); deviceId++)
            {
              int32_t interfaceIndex = (ip)->GetInterfaceForDevice (node->GetDevice (deviceId));
              if (interfaceIndex != -1)
                {
                  uint32_t numberOfAddresses = ip->GetNAddresses (interfaceIndex);
                  for (uint32_t addressIndex = 0; addressIndex < numberOfAddresses; addressIndex++)
                    {
                      Ipv4InterfaceAddress ifAddr = ip->GetAddress (interfaceIndex, addressIndex);
                      Ipv4Address addr = ifAddr.GetAddress ();

                      if (addr == hostAddress)
                        {
                          return node;
                        }
                    }
                }
            }
        }
    }

  NS_LOG_ERROR ("Couldn't find dest node given the IP" << hostAddress);
  return 0;
}

void
FlentApplication::TraceReceivedPing (const Address &address, uint16_t seq, uint8_t ttl, Time t)
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
FlentApplication::TraceReceivedUdpPing1 (Ptr<const Packet> packet, const Address &address, const Address &localAddress, const SeqTsEchoHeader &header)
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
FlentApplication::TraceReceivedUdpPing2 (Ptr<const Packet> packet, const Address &address, const Address &localAddress, const SeqTsEchoHeader &header)
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
FlentApplication::TraceReceivedUdpPing3 (Ptr<const Packet> packet, const Address &address, const Address &localAddress, const SeqTsEchoHeader &header)
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
FlentApplication::GoodputSamplingUpload (std::string name, int i) {
  Json::Value data;
  double goodput = (m_bytesSent[i] * 8 / m_stepSize.GetSeconds () / 1e6);
  data["dur"] = m_stepSize.GetSeconds ();
  data["t"] = (Simulator::Now ().GetSeconds ()+m_currTime);
  data["val"] = goodput;
  m_output["raw_values"][name].append (data);
  m_output["results"][name].append (goodput);
  m_bytesSent[i] = 0;
  Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSamplingUpload, this, name, i);
}

void
FlentApplication::GoodputSamplingDownload (std::string name, int i) {
  Json::Value data;
  double goodput = (m_bytesReceived[i] * 8 / m_stepSize.GetSeconds () / 1e6);
  data["dur"] = m_stepSize.GetSeconds ();
  data["t"] = (Simulator::Now ().GetSeconds () + m_currTime);
  data["val"] = goodput;
  m_output["raw_values"][name].append (data);
  m_output["results"][name].append (goodput);
  m_bytesReceived[i] = 0;
  Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSamplingDownload, this, name, i);
}

//Application Methods
void FlentApplication::StartApplication (void) //Called at time specified by Start
{
  NS_LOG_FUNCTION (this);
  m_currTime = (std::chrono::duration_cast<std::chrono::nanoseconds> (std::chrono::system_clock::now ().time_since_epoch ()).count ()/1000000000);
  AddMetadata (m_output);

  if (m_testName.compare ("ping") == 0)
    {
      Ipv4Address hostAddr = Ipv4Address::ConvertFrom (m_hostAddress);
      Ptr<V4Ping> m_v4ping = CreateObject<V4Ping> ();
      m_v4ping->SetAttribute ("Remote", Ipv4AddressValue (hostAddr));
      m_v4ping->SetAttribute ("Interval", TimeValue (m_stepSize));
      m_node->AddApplication (m_v4ping);
      m_output["raw_values"]["Ping (ms) ICMP"] = Json::Value (Json::arrayValue);
      m_output["results"]["Ping (ms) ICMP"] = Json::Value (Json::arrayValue);
      m_output["x_values"] = Json::Value (Json::arrayValue);
      
      m_v4ping->TraceConnectWithoutContext ("Rx", MakeCallback (&FlentApplication::TraceReceivedPing, this));
    }
  else if (m_testName.compare ("tcp_upload") == 0)
    {
      Ipv4Address hostAddr = Ipv4Address::ConvertFrom (m_hostAddress);
      Ptr<V4Ping> m_v4ping = CreateObject<V4Ping> ();
      m_v4ping->SetAttribute ("Remote", Ipv4AddressValue (hostAddr));
      m_v4ping->SetAttribute ("Interval", TimeValue (m_stepSize));
      m_node->AddApplication (m_v4ping);
      ApplicationContainer pingContainer;
      pingContainer.Add (m_v4ping);
      pingContainer.Start (m_startTime);
      pingContainer.Stop (m_stopTime);

      m_output["raw_values"]["Ping (ms) ICMP"] = Json::Value (Json::arrayValue);
      m_output["results"]["Ping (ms) ICMP"] = Json::Value (Json::arrayValue);
      m_output["x_values"] = Json::Value (Json::arrayValue);
      
      m_v4ping->TraceConnectWithoutContext ("Rx", MakeCallback (&FlentApplication::TraceReceivedPing, this));

      InetSocketAddress clientAddress = InetSocketAddress (hostAddr, 9);
      Ptr<BulkSendApplication> m_bulkSend = CreateObject<BulkSendApplication> ();
      m_bulkSend->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_bulkSend->SetAttribute ("Remote", AddressValue (clientAddress));
      m_bulkSend->SetAttribute ("MaxBytes", UintegerValue (0));
      m_node->AddApplication (m_bulkSend);
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
      m_bulkSend->TraceConnectWithoutContext ("Tx", MakeBoundCallback (&TraceSentPacket, &m_bytesSent[0]));
      Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSamplingUpload, this, "TCP upload", 0);

      Address sinkAddress (InetSocketAddress (Ipv4Address::GetAny (), 9));
      Ptr<PacketSink> m_packetSink = CreateObject<PacketSink> ();
      m_packetSink->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_packetSink->SetAttribute ("Local", AddressValue (sinkAddress));
      m_hostNode->AddApplication (m_packetSink);
      ApplicationContainer sinkApp;
      sinkApp.Add (m_packetSink);
      sinkApp.Start (m_startTime + Seconds(5));
      sinkApp.Stop (m_stopTime - Seconds(5));

    }
  else if (m_testName.compare ("tcp_download") == 0)
    {
      Ipv4Address localBindAddr = Ipv4Address::ConvertFrom (m_localBindAddress);
      Ptr<V4Ping> m_v4ping = CreateObject<V4Ping> ();
      m_v4ping->SetAttribute ("Remote", Ipv4AddressValue (localBindAddr));
      m_v4ping->SetAttribute ("Interval", TimeValue (m_stepSize));
      m_hostNode->AddApplication (m_v4ping);
      ApplicationContainer pingContainer;
      pingContainer.Add (m_v4ping);
      pingContainer.Start (m_startTime);
      pingContainer.Stop (m_stopTime);

      m_output["raw_values"]["Ping (ms) ICMP"] = Json::Value (Json::arrayValue);
      m_output["results"]["Ping (ms) ICMP"] = Json::Value (Json::arrayValue);
      m_output["x_values"] = Json::Value (Json::arrayValue);

      m_v4ping->TraceConnectWithoutContext ("Rx", MakeCallback (&FlentApplication::TraceReceivedPing, this));
      Address sinkAddress (InetSocketAddress (Ipv4Address::GetAny (), 9));
      Ptr<PacketSink> m_packetSink = CreateObject<PacketSink> ();
      m_packetSink->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_packetSink->SetAttribute ("Local", AddressValue (sinkAddress));
      m_node->AddApplication (m_packetSink);
      ApplicationContainer sinkApp;
      sinkApp.Add (m_packetSink);
      sinkApp.Start (m_startTime + Seconds(5));
      sinkApp.Stop (m_stopTime - Seconds(5));
      m_packetSink->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&TraceReceivedPacket, &m_bytesReceived[0]));
      m_output["results"]["TCP download"] = Json::Value (Json::arrayValue);
      m_output["raw_values"]["TCP download"] = Json::Value (Json::arrayValue);
      Json::Value data;
      data["dur"] = m_stepSize.GetSeconds ();
      data["t"] = (Simulator::Now ().GetSeconds ()+m_currTime);
      data["val"] = 0;
      m_output["raw_values"]["TCP download"].append (data);
      m_output["results"]["TCP download"].append (data["val"]);
      Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSamplingDownload, this, "TCP download", 0);

      InetSocketAddress localBindAddress = InetSocketAddress (localBindAddr, 9);
      Ptr<BulkSendApplication> m_bulkSend = CreateObject<BulkSendApplication> ();
      m_bulkSend->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_bulkSend->SetAttribute ("Remote", AddressValue (localBindAddress));
      m_bulkSend->SetAttribute ("MaxBytes", UintegerValue (0));
      m_hostNode->AddApplication (m_bulkSend);
      ApplicationContainer sourceApp;
      sourceApp.Add(m_bulkSend);
      sourceApp.Start (m_startTime + Seconds(5));
      sourceApp.Stop (m_stopTime - Seconds(5));
    }
  else if (m_testName.compare ("rrul") == 0)
    {
      Ipv4Address hostIpv4Address = Ipv4Address::ConvertFrom (m_hostAddress);
      Ipv4Address localIpv4Address = Ipv4Address::ConvertFrom (m_localBindAddress);

      Ptr<V4Ping>  m_v4ping = CreateObject<V4Ping> ();
      m_v4ping->SetAttribute ("Remote", Ipv4AddressValue (hostIpv4Address));
      m_v4ping->SetAttribute ("Interval", TimeValue (m_stepSize));
      m_node->AddApplication (m_v4ping);
      ApplicationContainer pingContainer;
      pingContainer.Add (m_v4ping);
      pingContainer.Start (m_startTime);
      pingContainer.Stop (m_stopTime);

      m_output["raw_values"]["Ping (ms) ICMP"] = Json::Value (Json::arrayValue);
      m_output["results"]["Ping (ms) ICMP"] = Json::Value (Json::arrayValue);
      m_output["x_values"] = Json::Value (Json::arrayValue);
      m_v4ping->TraceConnectWithoutContext ("Rx", MakeCallback (&FlentApplication::TraceReceivedPing, this));

      uint16_t port = 9;
      Ptr<UdpEchoServer> m_udpserver = CreateObject<UdpEchoServer> ();
      m_udpserver->SetAttribute ("Port", UintegerValue (port));
      m_udpserver->SetAttribute ("EnableSeqTsEchoHeader", BooleanValue (true));
      m_hostNode->AddApplication (m_udpserver);
      ApplicationContainer apps;
      apps.Add (m_udpserver);
      apps.Start (m_startTime);
      apps.Stop (m_stopTime);
      uint32_t packetSize = 1024;
      uint32_t maxPacketCount = 10000;
      Ptr<UdpEchoClient>  m_udpclient = CreateObject<UdpEchoClient> ();
      m_udpclient->SetAttribute ("RemoteAddress", AddressValue (hostIpv4Address)); 
      m_udpclient->SetAttribute ("RemotePort", UintegerValue (port));
      m_udpclient->SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
      m_udpclient->SetAttribute ("Interval", TimeValue (m_stepSize));
      m_udpclient->SetAttribute ("PacketSize", UintegerValue (packetSize));
      m_udpclient->SetAttribute ("EnableSeqTsEchoHeader", BooleanValue (true));
      m_node->AddApplication (m_udpclient);
      ApplicationContainer apps2;
      apps2.Add(m_udpclient);
      apps2.Start (m_startTime);
      apps2.Stop (m_stopTime);
      m_output["raw_values"]["Ping (ms) UDP BE"] = Json::Value (Json::arrayValue);
      m_output["results"]["Ping (ms) UDP BE"] = Json::Value (Json::arrayValue);
      m_udpclient->TraceConnectWithoutContext ("RxWithSeqTsEchoHeader", MakeCallback (&FlentApplication::TraceReceivedUdpPing1, this));

      port = 10;
      Ptr<UdpEchoServer> m_udpserver2 = CreateObject<UdpEchoServer> ();
      m_udpserver2->SetAttribute ("Port", UintegerValue (port));
      m_udpserver2->SetAttribute ("EnableSeqTsEchoHeader", BooleanValue (true));
      m_hostNode->AddApplication (m_udpserver2);
      ApplicationContainer apps3;
      apps3.Add (m_udpserver2);
      apps3.Start (m_startTime);
      apps3.Stop (m_stopTime);
      Ptr<UdpEchoClient>  m_udpclient2 = CreateObject<UdpEchoClient> ();
      m_udpclient2->SetAttribute ("RemoteAddress", AddressValue (hostIpv4Address)); 
      m_udpclient2->SetAttribute ("RemotePort", UintegerValue (port));
      m_udpclient2->SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
      m_udpclient2->SetAttribute ("Interval", TimeValue (m_stepSize));
      m_udpclient2->SetAttribute ("PacketSize", UintegerValue (packetSize));
      m_udpclient2->SetAttribute ("EnableSeqTsEchoHeader", BooleanValue (true));
      m_node->AddApplication (m_udpclient2);
      ApplicationContainer apps4;
      apps4.Add(m_udpclient2);
      apps4.Start (m_startTime);
      apps4.Stop (m_stopTime);
      m_output["raw_values"]["Ping (ms) UDP BK"] = Json::Value (Json::arrayValue);
      m_output["results"]["Ping (ms) UDP BK"] = Json::Value (Json::arrayValue);
      m_udpclient->TraceConnectWithoutContext ("RxWithSeqTsEchoHeader", MakeCallback (&FlentApplication::TraceReceivedUdpPing2, this));

      port = 11;
      Ptr<UdpEchoServer> m_udpserver3 = CreateObject<UdpEchoServer> ();
      m_udpserver3->SetAttribute ("Port", UintegerValue (port));
      m_udpserver3->SetAttribute ("EnableSeqTsEchoHeader", BooleanValue (true));
      m_hostNode->AddApplication (m_udpserver3);
      ApplicationContainer apps5;
      apps5.Add (m_udpserver3);
      apps5.Start (m_startTime);
      apps5.Stop (m_stopTime);
      Ptr<UdpEchoClient>  m_udpclient3 = CreateObject<UdpEchoClient> ();
      m_udpclient3->SetAttribute ("RemoteAddress", AddressValue (hostIpv4Address)); 
      m_udpclient3->SetAttribute ("RemotePort", UintegerValue (port));
      m_udpclient3->SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
      m_udpclient3->SetAttribute ("Interval", TimeValue (m_stepSize));
      m_udpclient3->SetAttribute ("PacketSize", UintegerValue (packetSize));
      m_udpclient3->SetAttribute ("EnableSeqTsEchoHeader", BooleanValue (true));
      m_node->AddApplication (m_udpclient3);
      ApplicationContainer apps6;
      apps6.Add(m_udpclient3);
      apps6.Start (m_startTime);
      apps6.Stop (m_stopTime);
      m_output["raw_values"]["Ping (ms) UDP EF"] = Json::Value (Json::arrayValue);
      m_output["results"]["Ping (ms) UDP EF"] = Json::Value (Json::arrayValue);
      m_udpclient->TraceConnectWithoutContext ("RxWithSeqTsEchoHeader", MakeCallback (&FlentApplication::TraceReceivedUdpPing3, this));

      //Download BE
      Address sinkAddress (InetSocketAddress (Ipv4Address::GetAny (), 10));
      Ptr<PacketSink> m_packetSink = CreateObject<PacketSink> ();
      m_packetSink->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_packetSink->SetAttribute ("Local", AddressValue (sinkAddress));
      m_node->AddApplication (m_packetSink);
      ApplicationContainer sinkApp;
      sinkApp.Add (m_packetSink);
      sinkApp.Start (m_startTime + Seconds(5));
      sinkApp.Stop (m_stopTime - Seconds(5));
      m_packetSink->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&TraceReceivedPacket, &m_bytesReceived[0]));
      m_output["results"]["TCP download BE"] = Json::Value (Json::arrayValue);
      m_output["raw_values"]["TCP download BE"] = Json::Value (Json::arrayValue);
      Json::Value data;
      data["dur"] = m_stepSize.GetSeconds ();
      data["t"] = (Simulator::Now ().GetSeconds ()+m_currTime);
      data["val"] = 0;
      m_output["raw_values"]["TCP download BE"].append (data);
      m_output["results"]["TCP download BE"].append (data["val"]);
      Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSamplingDownload, this, "TCP download BE", 0);
      InetSocketAddress localBindAddress = InetSocketAddress (localIpv4Address, 10);
      localBindAddress.SetTos (Ipv4Header::DscpType::DscpDefault << 2);
      Ptr<BulkSendApplication> m_bulkSend = CreateObject<BulkSendApplication> ();
      m_bulkSend->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_bulkSend->SetAttribute ("Remote", AddressValue (localBindAddress));
      m_bulkSend->SetAttribute ("MaxBytes", UintegerValue (0));
      m_hostNode->AddApplication (m_bulkSend);
      ApplicationContainer sourceApp;
      sourceApp.Add(m_bulkSend);
      sourceApp.Start (m_startTime + Seconds(5));
      sourceApp.Stop (m_stopTime - Seconds(5));
      
      // Upload BE
      InetSocketAddress hostAddress = InetSocketAddress (hostIpv4Address, 10);
      hostAddress.SetTos (Ipv4Header::DscpType::DscpDefault << 2);
      Ptr<BulkSendApplication> m_bulkSendUp = CreateObject<BulkSendApplication> ();
      m_bulkSendUp->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_bulkSendUp->SetAttribute ("Remote", AddressValue (hostAddress));
      m_bulkSendUp->SetAttribute ("MaxBytes", UintegerValue (0));
      m_node->AddApplication (m_bulkSendUp);
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
      m_bulkSendUp->TraceConnectWithoutContext ("Tx", MakeBoundCallback (&TraceSentPacket, &m_bytesSent[0]));
      Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSamplingUpload, this, "TCP upload BE", 0);
      Address sinkAddressUp (InetSocketAddress (Ipv4Address::GetAny (), 10));
      Ptr<PacketSink> m_packetSinkUp = CreateObject<PacketSink> ();
      m_packetSinkUp->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_packetSinkUp->SetAttribute ("Local", AddressValue (sinkAddressUp));
      m_hostNode->AddApplication (m_packetSinkUp);
      ApplicationContainer sinkAppUp;
      sinkAppUp.Add (m_packetSinkUp);
      sinkAppUp.Start (m_startTime + Seconds(5));
      sinkAppUp.Stop (m_stopTime - Seconds(5));

      // Download BK
      Address sinkAddress2 (InetSocketAddress (Ipv4Address::GetAny (), 9));
      Ptr<PacketSink> m_packetSink2 = CreateObject<PacketSink> ();
      m_packetSink2->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_packetSink2->SetAttribute ("Local", AddressValue (sinkAddress2));
      m_node->AddApplication (m_packetSink2);
      ApplicationContainer sinkApp2;
      sinkApp2.Add (m_packetSink2);
      sinkApp2.Start (m_startTime + Seconds(5));
      sinkApp2.Stop (m_stopTime - Seconds(5));
      m_packetSink2->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&TraceReceivedPacket, &m_bytesReceived[1]));
      m_output["results"]["TCP download BK"] = Json::Value (Json::arrayValue);
      m_output["raw_values"]["TCP download BK"] = Json::Value (Json::arrayValue);
      Json::Value data2;
      data2["dur"] = m_stepSize.GetSeconds ();
      data2["t"] = (Simulator::Now ().GetSeconds ()+m_currTime);
      data2["val"] = 0;
      m_output["raw_values"]["TCP download BK"].append (data2);
      m_output["results"]["TCP download BK"].append (data2["val"]);
      Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSamplingDownload, this, "TCP download BK", 1);
      InetSocketAddress localBindAddress2 = InetSocketAddress (localIpv4Address, 9);
      localBindAddress2.SetTos (Ipv4Header::DscpType::DSCP_CS1 << 2);
      Ptr<BulkSendApplication> m_bulkSend2 = CreateObject<BulkSendApplication> ();
      m_bulkSend2->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_bulkSend2->SetAttribute ("Remote", AddressValue (localBindAddress2));
      m_bulkSend2->SetAttribute ("MaxBytes", UintegerValue (0));
      m_hostNode->AddApplication (m_bulkSend2);
      ApplicationContainer sourceApp2;
      sourceApp2.Add(m_bulkSend2);
      sourceApp2.Start (m_startTime + Seconds(5));
      sourceApp2.Stop (m_stopTime - Seconds(5));
      
      //Upload BK
      hostAddress = InetSocketAddress (hostIpv4Address, 11);
      hostAddress.SetTos (Ipv4Header::DscpType::DSCP_CS1 << 2);
      Ptr<BulkSendApplication> m_bulkSendUp2 = CreateObject<BulkSendApplication> ();
      m_bulkSendUp2->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_bulkSendUp2->SetAttribute ("Remote", AddressValue (hostAddress));
      m_bulkSendUp2->SetAttribute ("MaxBytes", UintegerValue (0));
      m_node->AddApplication (m_bulkSendUp2);
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
      m_bulkSendUp->TraceConnectWithoutContext ("Tx", MakeBoundCallback (&TraceSentPacket, &m_bytesSent[1]));
      Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSamplingUpload, this, "TCP upload BK", 1);
      Address sinkAddressUp2 (InetSocketAddress (Ipv4Address::GetAny (), 11));
      Ptr<PacketSink> m_packetSinkUp2 = CreateObject<PacketSink> ();
      m_packetSinkUp2->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_packetSinkUp2->SetAttribute ("Local", AddressValue (sinkAddressUp2));
      m_hostNode->AddApplication (m_packetSinkUp2);
      ApplicationContainer sinkAppUp2;
      sinkAppUp2.Add (m_packetSinkUp2);
      sinkAppUp2.Start (m_startTime + Seconds(5));
      sinkAppUp2.Stop (m_stopTime - Seconds(5));

      //Download CS5
      Address sinkAddress3 (InetSocketAddress (Ipv4Address::GetAny (), 11));
      Ptr<PacketSink> m_packetSink3 = CreateObject<PacketSink> ();
      m_packetSink3->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_packetSink3->SetAttribute ("Local", AddressValue (sinkAddress3));
      m_node->AddApplication (m_packetSink3);
      ApplicationContainer sinkApp3;
      sinkApp3.Add (m_packetSink3);
      sinkApp3.Start (m_startTime + Seconds(5));
      sinkApp3.Stop (m_stopTime - Seconds(5));
      m_packetSink3->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&TraceReceivedPacket, &m_bytesReceived[2]));
      m_output["results"]["TCP download CS5"] = Json::Value (Json::arrayValue);
      m_output["raw_values"]["TCP download CS5"] = Json::Value (Json::arrayValue);
      Json::Value data3;
      data3["dur"] = m_stepSize.GetSeconds ();
      data3["t"] = (Simulator::Now ().GetSeconds ()+m_currTime);
      data3["val"] = 0;
      m_output["raw_values"]["TCP download CS5"].append (data3);
      m_output["results"]["TCP download CS5"].append (data3["val"]);
      Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSamplingDownload, this, "TCP download CS5", 2);
      InetSocketAddress localBindAddress3 = InetSocketAddress (localIpv4Address, 11);
      localBindAddress3.SetTos (Ipv4Header::DscpType::DSCP_CS5 << 2);
      Ptr<BulkSendApplication> m_bulkSend3 = CreateObject<BulkSendApplication> ();
      m_bulkSend3->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_bulkSend3->SetAttribute ("Remote", AddressValue (localBindAddress3));
      m_bulkSend3->SetAttribute ("MaxBytes", UintegerValue (0));
      m_hostNode->AddApplication (m_bulkSend3);
      ApplicationContainer sourceApp3;
      sourceApp3.Add(m_bulkSend3);
      sourceApp3.Start (m_startTime + Seconds(5));
      sourceApp3.Stop (m_stopTime - Seconds(5));

      //Upload CS5
      hostAddress = InetSocketAddress (hostIpv4Address, 12);
      hostAddress.SetTos (Ipv4Header::DscpType::DSCP_CS5 << 2);
      Ptr<BulkSendApplication> m_bulkSendUp3 = CreateObject<BulkSendApplication> ();
      m_bulkSendUp3->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_bulkSendUp3->SetAttribute ("Remote", AddressValue (hostAddress));
      m_bulkSendUp3->SetAttribute ("MaxBytes", UintegerValue (0));
      m_node->AddApplication (m_bulkSendUp3);
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
      m_bulkSendUp3->TraceConnectWithoutContext ("Tx", MakeBoundCallback (&TraceSentPacket, &m_bytesSent[2]));
      Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSamplingUpload, this, "TCP upload CS5", 2);
      Address sinkAddressUp3 (InetSocketAddress (Ipv4Address::GetAny (), 12));
      Ptr<PacketSink> m_packetSinkUp3 = CreateObject<PacketSink> ();
      m_packetSinkUp3->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_packetSinkUp3->SetAttribute ("Local", AddressValue (sinkAddressUp3));
      m_hostNode->AddApplication (m_packetSinkUp3);
      ApplicationContainer sinkAppUp3;
      sinkAppUp3.Add (m_packetSinkUp3);
      sinkAppUp3.Start (m_startTime + Seconds(5));
      sinkAppUp3.Stop (m_stopTime - Seconds(5));

      //Download EF
      Address sinkAddress4 (InetSocketAddress (Ipv4Address::GetAny (), 12));
      Ptr<PacketSink> m_packetSink4 = CreateObject<PacketSink> ();
      m_packetSink4->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_packetSink4->SetAttribute ("Local", AddressValue (sinkAddress4));
      m_node->AddApplication (m_packetSink4);
      ApplicationContainer sinkApp4;
      sinkApp4.Add (m_packetSink4);
      sinkApp4.Start (m_startTime + Seconds(5));
      sinkApp4.Stop (m_stopTime - Seconds(5));
      m_packetSink4->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&TraceReceivedPacket, &m_bytesReceived[3]));
      m_output["results"]["TCP download EF"] = Json::Value (Json::arrayValue);
      m_output["raw_values"]["TCP download EF"] = Json::Value (Json::arrayValue);
      Json::Value data4;
      data4["dur"] = m_stepSize.GetSeconds ();
      data4["t"] = (Simulator::Now ().GetSeconds ()+m_currTime);
      data4["val"] = 0;
      m_output["raw_values"]["TCP download EF"].append (data4);
      m_output["results"]["TCP download EF"].append (data4["val"]);
      Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSamplingDownload, this, "TCP download EF", 3);
      InetSocketAddress localBindAddress4 = InetSocketAddress (localIpv4Address, 12);
      localBindAddress4.SetTos (Ipv4Header::DscpType::DSCP_EF << 2);
      Ptr<BulkSendApplication> m_bulkSend4 = CreateObject<BulkSendApplication> ();
      m_bulkSend4->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_bulkSend4->SetAttribute ("Remote", AddressValue (localBindAddress4));
      m_bulkSend4->SetAttribute ("MaxBytes", UintegerValue (0));
      m_hostNode->AddApplication (m_bulkSend4);
      ApplicationContainer sourceApp4;
      sourceApp4.Add(m_bulkSend4);
      sourceApp4.Start (m_startTime + Seconds(5));
      sourceApp4.Stop (m_stopTime - Seconds(5));

      //Upload EF
      hostAddress = InetSocketAddress (hostIpv4Address, 13);
      hostAddress.SetTos (Ipv4Header::DscpType::DSCP_EF << 2);
      Ptr<BulkSendApplication> m_bulkSendUp4 = CreateObject<BulkSendApplication> ();
      m_bulkSendUp4->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_bulkSendUp4->SetAttribute ("Remote", AddressValue (hostAddress));
      m_bulkSendUp4->SetAttribute ("MaxBytes", UintegerValue (0));
      m_node->AddApplication (m_bulkSendUp4);
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
      m_bulkSendUp4->TraceConnectWithoutContext ("Tx", MakeBoundCallback (&TraceSentPacket, &m_bytesSent[3]));
      Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSamplingUpload, this, "TCP upload EF", 3);
      Address sinkAddressUp4 (InetSocketAddress (Ipv4Address::GetAny (), 13));
      Ptr<PacketSink> m_packetSinkUp4 = CreateObject<PacketSink> ();
      m_packetSinkUp4->SetAttribute ("Protocol", StringValue ("ns3::TcpSocketFactory"));
      m_packetSinkUp4->SetAttribute ("Local", AddressValue (sinkAddressUp4));
      m_hostNode->AddApplication (m_packetSinkUp4);
      ApplicationContainer sinkAppUp4;
      sinkAppUp4.Add (m_packetSinkUp4);
      sinkAppUp4.Start (m_startTime + Seconds(5));
      sinkAppUp4.Stop (m_stopTime - Seconds(5));
    }

}

void FlentApplication::StopApplication (void) // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);
  AsciiTraceHelper ascii;
  if (m_testName.compare ("ping") == 0)
    {
      //m_v4ping->TraceDisconnectWithoutContext ("Rx", MakeCallback (&FlentApplication::ReceivePing, this));
      //m_packetSink->TraceDisconnectWithoutContext ("Rx", MakeCallback (&FlentApplication::ReceiveData1, this));
      Ptr<OutputStreamWrapper> streamOutput = ascii.CreateFileStream (m_testName + ".flent");
      *streamOutput->GetStream () << m_output << std::endl;
    }
  else if (m_testName.compare ("tcp_upload") == 0)
    {
      //m_v4ping->TraceDisconnectWithoutContext ("Rx", MakeCallback (&FlentApplication::ReceivePing, this));
      //m_bulkSend->TraceDisconnectWithoutContext ("Tx", MakeBoundCallback (&TraceSentPacket, &g_bytesSent[0]));
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
