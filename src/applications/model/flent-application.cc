/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Georgia Institute of Technology
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
 * Author: George F. Riley <riley@ece.gatech.edu>
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
#include "ns3/internet-apps-module.h"
#include "ns3/trace-helper.h"
#include "ns3/applications-module.h"
#include "ns3/packet-sink.h"
#include "ns3/packet-sink-helper.h"
//#include "bulk-send-application.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FlentApplication");

NS_OBJECT_ENSURE_REGISTERED (FlentApplication);

uint32_t g_bytesSent = 0;

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
    .AddAttribute ("ImageText", "Text to be included in the plot",
		  StringValue (""),
		  MakeStringAccessor (&FlentApplication::m_imageText),
		  MakeStringChecker ())
    .AddAttribute ("ImageName", "Name of the image to save the output plot",
		  StringValue (""),
		  MakeStringAccessor (&FlentApplication::m_imageName),
		  MakeStringChecker ())
    .AddAttribute ("StepSize", "Measurment data point size",
		  TimeValue (Seconds(1)),
		  MakeTimeAccessor (&FlentApplication::m_stepSize),
		  MakeTimeChecker ())
    .AddAttribute ("Delay", "Time to delay parts of test",
		  TimeValue (Seconds(1)),
		  MakeTimeAccessor (&FlentApplication::m_delay),
		  MakeTimeChecker ())
  ;
  return tid;
}


FlentApplication::FlentApplication ()
  : m_server (false)
{
  NS_LOG_FUNCTION (this);
}

FlentApplication::~FlentApplication ()
{
  NS_LOG_FUNCTION (this);
}

void
FlentApplication::SetTest (std::string testname)
{
  m_testName = testname;
}

void FlentApplication::SetDuration (Time duration)
{
  m_duration = duration;
  m_stopTime = Seconds (m_startTime.GetSeconds () + m_duration.GetSeconds () + 10);
}

void
FlentApplication::SetServerAddress (Address serverAddress)
{
  m_serverAddress = serverAddress;
}

void
FlentApplication::SetServer ()
{
  m_server = true;
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

void FlentApplication::SetDelay (Time delay)
{
  m_delay = delay;
}

std::string FlentApplication::GetUTCFormatTime (int sec) {
  time_t     now = time (0) + sec;
  struct tm  tstruct;
  char       buf[80];
  tstruct = *localtime (&now);
  strftime (buf, sizeof (buf), "%Y-%m-%dT%X.004275Z", &tstruct);

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
  j["metadata"]["NAME"] = "tcp_upload";
  j["metadata"]["NOTE"] = Json::Value::null;
  j["metadata"]["REMOTE_METADATA"] = Json::Value::null;
  j["metadata"]["STEP_SIZE"] = m_stepSize.GetSeconds ();
  j["metadata"]["TIME"] = GetUTCFormatTime (0);
  j["metadata"]["T0"] = GetUTCFormatTime (-19800);
  j["metadata"]["TEST_PARAMETERS"] = Json::objectValue;
  j["metadata"]["TITLE"] = Json::Value::null;
  j["metadata"]["TOTAL_LENGTH"] = m_stopTime.GetSeconds ();
  j["version"] = 4;
}

void
FlentApplication::ReceivePing (const Address &address, uint16_t seq, uint8_t ttl, Time t)
{
  Json::Value data;
  double rtt = t.GetNanoSeconds ()/1e6;
  data["seq"] = seq;
  data["t"] = (Simulator::Now ().GetSeconds ()+ m_currTime);
  data["val"] = rtt; 
  m_output["raw_values"]["Ping (ms) ICMP"].append(data);
  m_output["results"]["Ping (ms) ICMP"].append(rtt);
  m_output["x_values"].append (Simulator::Now ().GetSeconds ());
}

void 
FlentApplication::SendData (Ptr<const Packet> packet)
{
  g_bytesSent += packet->GetSize ();
}

void
FlentApplication::GoodputSampling () {
  Json::Value data;
  Json::Int64 throughput = (g_bytesSent * 8 / m_stepSize.GetSeconds () / 1e6);
  data["dur"] = m_stepSize.GetSeconds ();
  data["t"] = (Simulator::Now ().GetSeconds ()+m_currTime);
  data["val"] = throughput;
  m_output["raw_values"]["TCP upload"].append (data);
  m_output["results"]["TCP upload"].append (throughput);
  g_bytesSent = 0;
  Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSampling, this);
}

//Application Methods
void FlentApplication::StartApplication (void) //Called at time specified by Start
{
  NS_LOG_FUNCTION (this);
  //Intital timestamp t0 is recorded in UTC Format in json output
  //and is substracted from all timestamp values so we add currTime to all
  //timestamp values written to json output
  m_currTime = (std::chrono::duration_cast<std::chrono::nanoseconds> (std::chrono::system_clock::now ().time_since_epoch ()).count ()/1000000000);
  FlentApplication::AddMetadata (m_output);

  if (m_testName.compare ("ping") == 0) {
      Ipv4Address serverAddr = Ipv4Address::ConvertFrom (m_serverAddress);
      m_v4ping = CreateObject<V4Ping> ();
      m_v4ping->SetAttribute ("Remote", Ipv4AddressValue (serverAddr));
      m_v4ping->SetAttribute ("Interval", TimeValue (m_stepSize));
      GetNode ()->AddApplication (m_v4ping);
      m_output["raw_values"]["Ping (ms) ICMP"] = Json::Value (Json::arrayValue);
      m_output["results"]["Ping (ms) ICMP"] = Json::Value (Json::arrayValue);
      m_output["x_values"] = Json::Value (Json::arrayValue);
      
      m_v4ping->TraceConnectWithoutContext ("Rx", MakeCallback (&FlentApplication::ReceivePing, this));

  } else if (m_testName.compare ("tcp_upload") == 0) {
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
      m_bulkSend->TraceConnectWithoutContext ("Tx", MakeCallback (&FlentApplication::SendData, this));
      Simulator::Schedule (m_stepSize, &FlentApplication::GoodputSampling, this);

    } else if (m_testName.compare ("netserver") == 0) {
      Address sinkAddress (InetSocketAddress (Ipv4Address::GetAny (), 9));
      PacketSinkHelper netserver ("ns3::TcpSocketFactory", sinkAddress);
      netserver.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
      ApplicationContainer sinkApp = netserver.Install (GetNode ());
      sinkApp.Start (m_startTime);
      sinkApp.Stop (m_stopTime);
      m_packetSink = DynamicCast<PacketSink> (sinkApp.Get (0));

    }

}

void FlentApplication::StopApplication (void) // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);
  Simulator::Schedule (MilliSeconds (0.01), &Simulator::Stop);
  m_v4ping->TraceDisconnectWithoutContext ("Rx", MakeCallback (&FlentApplication::ReceivePing, this));
  m_bulkSend->TraceDisconnectWithoutContext ("Tx", MakeCallback (&FlentApplication::SendData, this));
  AsciiTraceHelper ascii;
  Ptr<OutputStreamWrapper> streamOutput = ascii.CreateFileStream (m_testName + ".flent");
  *streamOutput->GetStream () << m_output << std::endl;
}

} // Namespace ns3
