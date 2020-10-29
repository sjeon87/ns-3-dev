/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Universita' degli Studi di Napoli Federico II
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
 * Authors: Harsha Sharma <harshasha256@gmail.com>
 *          Tom Henderson <tomh@tomh.org>
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FlentExample");

int main (int argc, char *argv[])
{

  std::string testName = "rrul";
  Time rtt = MilliSeconds (10);
  DataRate bw ("50Mbps");
  Time length = Seconds (60);
  Time delay = Seconds (0);
  bool verbose = false;

  // 2 MB of TCP buffer
   Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1 << 21));
   Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1 << 21));
   Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpLinuxReno"));

  CommandLine cmd (__FILE__);
  cmd.AddValue ("test", "Type of ns-3 flent test", testName);
  cmd.AddValue ("rtt", "Delay value", rtt);
  cmd.AddValue ("bw", "Data Rate", bw);
  cmd.AddValue ("length", "Base test duration (--length in flent)", length);
  cmd.AddValue ("delay", "Time to delay test (--delay in flent)", delay);
  cmd.AddValue ("verbose", "Verbose output", verbose);
  cmd.Parse(argc, argv);

  // Check arguments
  if (testName != "rrul" && testName != "tcp_upload" && testName != "tcp_download" && testName != "ping")
    {
      NS_FATAL_ERROR ("Test name must be one of 'rrul', 'tcp_upload', 'tcp_download', or 'ping'");
    }
  
  if (verbose)
    {
      ShowProgress progress (Seconds (10));
    }  
  
  NodeContainer n;
  n.Create (4); // client <-> router1 <-> router2 <-> server
  // Create node containers for configuring individual links
  NodeContainer n0;  // Group the client and router1 together
  n0.Add (n.Get (0));
  n0.Add (n.Get (1));
  NodeContainer n1;  // Group the routers together
  n1.Add (n.Get (1));
  n1.Add (n.Get (2));
  NodeContainer n2;  // Group the router2 and server together
  n2.Add (n.Get (2));
  n2.Add (n.Get (3));

  PointToPointHelper deviceHelper;
  DataRate edgeRate (100 * bw.GetBitRate ());
  deviceHelper.SetDeviceAttribute ("DataRate", DataRateValue (edgeRate));
  deviceHelper.SetChannelAttribute ("Delay", TimeValue (MicroSeconds (1)));
  deviceHelper.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("3p"));
  NetDeviceContainer devices0;
  devices0 = deviceHelper.Install (n0);
  NetDeviceContainer devices2;
  devices2 = deviceHelper.Install (n2);
  // The middle link has the bandwidth and delay constraints
  NetDeviceContainer devices1;
  deviceHelper.SetDeviceAttribute ("DataRate", DataRateValue (bw));
  deviceHelper.SetChannelAttribute ("Delay", TimeValue (rtt / 2));
  devices1 = deviceHelper.Install (n1);
  
  // Configure the IP and traffic control layers
  InternetStackHelper stack;
  stack.InstallAll ();

  TrafficControlHelper tch;
  tch.SetRootQueueDisc ("ns3::FqCoDelQueueDisc");
  Config::SetDefault ("ns3::FqCoDelQueueDisc::MaxSize", QueueSizeValue (QueueSize ("200p")));
  tch.SetQueueLimits ("ns3::DynamicQueueLimits"); // enable BQL
  QueueDiscContainer qdiscs;
  qdiscs = tch.Install (devices0);
  qdiscs = tch.Install (devices1);
  qdiscs = tch.Install (devices2);
  
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces0 = address.Assign (devices0);
  address.NewNetwork ();
  Ipv4InterfaceContainer interfaces1 = address.Assign (devices1);
  address.NewNetwork ();
  Ipv4InterfaceContainer interfaces2 = address.Assign (devices2);
  
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  
  // Configure flent
  Ptr<FlentApplication> flent1 = CreateObject<FlentApplication> ();
  flent1->SetStartTime (delay);
  flent1->SetTest (testName);
  flent1->SetStepSize (Seconds (0.2));
  flent1->SetDuration (length);
  flent1->SetHostNode (n.Get(3));
  flent1->SetLocalBindAddress (interfaces0.GetAddress (0)); // local node
  flent1->SetHostAddress (interfaces2.GetAddress (1));  // remote node
  n.Get(0)->AddApplication (flent1); // add to local node only
  
  // Stop the simulation one second after flent ends
  // Flent ends at 'delay + length + Seconds (10)'
  Simulator::Stop (delay + length + Seconds (10) + Seconds (1));

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
