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

#include <iostream>
#include <string>
#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/node.h"
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

namespace ns3 {

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
		  MakeAddressAccessor (&FlentApplication::m_server),
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
  m_stopTime = Seconds (m_startTime.GetSeconds() + m_duration.GetSeconds() + 10);
}

void
FlentApplication::SetServer (Address serverAddress)
{
  m_server = serverAddress;
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
 
void FlentApplication::AddMetadata (Json::Value &j)
{
  j["metadata"]["BATCH_NAME"] = Json::Value::null;
  j["metadata"]["BATCH_TIME"] = Json::Value::null;
  j["metadata"]["BATCH_TITLE"] = Json::Value::null;
  j["metadata"]["BATCH_UUID"] = Json::Value::null;
  j["metadata"]["DATA_FILENAME"] = Json::Value::null;
  j["metadata"]["EGRESS_INFO"]["bql"]["tx-0"] = Json::Value::null;
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
  j["metadata"]["HOST"] = Json::Value::null;
  j["metadata"]["HOSTS"] = Json::Value::null;
  j["metadata"]["HTTP_GETTER_DNS"] = Json::Value::null;
  j["metadata"]["HTTP_GETTER_URLLIST"] = Json::Value::null;
  j["metadata"]["HTTP_GETTER_WORKERS"] = Json::Value::null;
  j["metadata"]["IP_VERSION"] = Json::Value::null;
  j["metadata"]["KERNEL_NAME"] = Json::Value::null;
  j["metadata"]["KERNEL_RELEASE"] = Json::Value::null;
  j["metadata"]["LENGTH"] = Json::Value::null;
  j["metadata"]["LOCAL_HOST"] = Json::Value::null;
  j["metadata"]["MODULE_VERSIONS"] = Json::Value::null;
  j["metadata"]["NAME"] = Json::Value::null;
  j["metadata"]["NOTE"] = Json::Value::null;
  j["metadata"]["REMOTE_METADATA"] = Json::Value::null;
  j["metadata"]["STEP_SIZE"] = Json::Value::null;
  j["metadata"]["T0"] = Json::Value::null;
  j["metadata"]["TEST_PARAMETERS"] = Json::Value::null;
  j["metadata"]["TIME"] = Json::Value::null;
  j["metadata"]["TITLE"] = Json::Value::null;
  j["metadata"]["TOTAL_LENGTH"] = Json::Value::null;
}

static void
ReceivePing (Json::Value &pingData,  const Address &address, uint16_t seq, uint8_t ttl, Time t)
{
  Json::Value data;
  Json::Int64 rtt = t.GetMilliSeconds ();
  data["seq"] = seq;
  data["t"] = Simulator::Now ().GetSeconds ();
  data["val"] = rtt; 
  pingData.append(data);
  //std::cout << pingData << std::endl;
}

//Application Methods
void FlentApplication::StartApplication (void) //Called at time specified by Start
{
  NS_LOG_FUNCTION (this);
  FlentApplication::AddMetadata (m_output);

  if (m_testName.compare ("ping") == 0) {
      Ipv4Address serverAddr = Ipv4Address::ConvertFrom (m_server);
      Ptr<V4Ping> m_v4ping = CreateObject<V4Ping> ();
      m_v4ping->SetAttribute ("Remote", Ipv4AddressValue(serverAddr));
      m_v4ping->SetAttribute ("Interval", TimeValue (m_stepSize));
      GetNode ()->AddApplication (m_v4ping);
      m_output["raw_values"]["Ping (ms) ICMP"] = Json::Value (Json::arrayValue);
      
      m_v4ping->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&ReceivePing, m_output["raw_values"]["Ping (ms) ICMP"]));

  } 
}

void FlentApplication::StopApplication (void) // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);
  Simulator::Schedule (MilliSeconds (0.01), &Simulator::Stop);
  std::cout << m_output << std::endl;
  //AsciiTraceHelper ascii;
  //Ptr<OutputStreamWrapper> streamOutput = ascii.CreateFileStream (m_testName + "-" + m_imageText + ".flent");
  //streamOutput->GetStream () << m_output << std::endl;
}

} // Namespace ns3
