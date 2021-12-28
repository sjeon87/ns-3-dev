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
 *         (adapted from bulk-send-application.h and
 *          packet-sink-application.h written by George F. Riley)
 *
 * Modified by: Ameya Deshpande <ameyanrd@outlook.com>
 *              Bhaskar Kataria <bhaskar.k7920@gmail.com> (Post processing of raw data)
 */

#ifndef FLENT_APPLICATION_H
#define FLENT_APPLICATION_H

#include <string>
#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/node-list.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/json.h"
#include "ns3/v4ping.h"
#include "ns3/bulk-send-application.h"
#include "ns3/packet-sink.h"
#include "ns3/udp-echo-server.h"
#include "ns3/udp-echo-client.h"

namespace ns3 {

class V4Ping;
class PacketSink;
class BulkSendApplication;
class SeqTsEchoHeader;

/**
 * \ingroup applications
 * \defgroup flent FlentApplication
 *
 * This application provides a basic model of
 * the flent traffic generator. In practice,
 * Flent is a wrapper around three traffic
 * generation tools: netperf(for TCP),
 * iperf (for UDP) and ping (for ICMP).
 * Flent also provides output in a
 * JSON-formatted data file, and plotting
 * support via matplotlib.
 */

/**
 * \ingroup flent
 *
 * \brief Flent is a network benchmarking tool
 *
 * This application provides a basic model of
 * the flent traffic generator. In practice,
 * Flent is a wrapper around three traffic
 * generation tools: netperf(for TCP),
 * iperf (for UDP) and ping (for ICMP).
 * Flent also provides output in a
 * JSON-formatted data file, and plotting
 * support via matplotlib.
 */

class FlentApplication : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  FlentApplication ();

  virtual ~FlentApplication ();

  /**
   * \brief Add Flent Meta Data
   *
   * \param [out] j Json output object
   */
  void AddMetadata (Json::Value &j);

protected:

  /**
   * In this method, the following tasks are performed:
   * 1) set the host node, which is derived from
   *    the host address,
   * 2) put the local bind address as the first
   *    address of the first non-loopback interface
   *    (if not set by the user explicitly), and
   * 3) override the application stop time from the
   *    Length attribute set by the user.
   *
   * \sa Application::DoInitialize
   */
  virtual void DoInitialize (void);

  /**
   * \sa Application::DoDispose
   */
  virtual void DoDispose (void);
private:

  // inherited from Application base class.
  virtual void StartApplication (void);    //Called at time specified by Start
  virtual void StopApplication (void);     //Called at time specified by Stop

  /**
   * \brief Iterates through the node list and finds the node
   * pointer for the given hostAddress
   * \param [in] hostAddress host node IP
   * \return The node with the specified IP.
   *
   */
  Ptr<Node> GetHostNode (Ipv4Address hostAddress) const;

  /**
   * \brief Ping RTT Trace function
   * \param address Destination address
   * \param seq Echo sequence number
   * \param ttl Time-To-Live field from IP header
   * \param t Time delta from the sent time
   */
  void TraceReceivedPing (const Address &address, uint16_t seq, uint8_t ttl, Time t);

  /**
   * \brief UDP Ping RTT Trace function
   * \param packet UDP Ping Packet
   * \param address Address of the destination
   * \param localAddress Local address of the sender
   * \param header SeqTsEchoHeader header
   */
  void TraceReceivedUdpPing1 (Ptr<const Packet> packet, const Address &address, const Address &localAddress, const SeqTsEchoHeader &header);

  /**
   * \brief UDP Ping RTT Trace function
   * \param packet UDP Ping Packet
   * \param address Address of the destination
   * \param localAddress Local address of the sender
   * \param header SeqTsEchoHeader header
   */
  void TraceReceivedUdpPing2 (Ptr<const Packet> packet, const Address &address, const Address &localAddress, const SeqTsEchoHeader &header);

  /**
   * \brief UDP Ping RTT Trace function
   * \param packet UDP Ping Packet
   * \param address Address of the destination
   * \param localAddress Local address of the sender
   * \param header SeqTsEchoHeader header
   */
  void TraceReceivedUdpPing3 (Ptr<const Packet> packet, const Address &address, const Address &localAddress, const SeqTsEchoHeader &header);

  /**
   * \brief Calculate the Time in UTC time format
   * from the number of seconds.
   * \param [in] sec Seconds
   * \return The time in UTC format
   *
   */
  std::string GetUtcFormatTime (int sec) const;

  /**
   * \brief Goodput calculation for the i-th flow
   * based on the bytes sent.
   *
   * Index i can have various values from 0 to 3 for
   * the four upload flows in case of RRUL test.
   *
   * @param [in] name Name of the flow
   * @param [in] i index of the flow
   */
  void GoodputSamplingUpload (std::string name, int i);

  /**
   * \brief Goodput calculation for the i-th flow
   * based on the bytes received.
   *
   * Index i can have various values from 0 to 3 for
   * the four download flows in case of RRUL test.
   *
   * @param [in] name Name of the flow
   * @param [in] i index of the flow
   */
  void GoodputSamplingDownload (std::string name, int i);

  /**
   * \brief Fill x_values parameter in flent file.
   * Adds x parameter values with the differece of stepSize
   */
  void FillXValues (void);

  /**
   * \brief Process raw values and add the processed result in the flent file.
   */
  void ProcessRawValues (void);


  double          m_currTime;         //!< Current time
  Json::Value     m_output;           //!< Json output
  Time            m_length;           //!< Test duration
  std::string     m_testName;         //!< Flent test name
  Ptr<Node>       m_hostNode;         //!< Host Node
  Address         m_hostAddress;      //!< Host address
  Address         m_localBindAddress; //!< Local bind address
  std::string     m_imageText;        //!< Text to be included in plot
  std::string     m_imageName;        //!< Name of the image to which plot is saved
  Time            m_stepSize;         //!< Measurment data point step size
  Time            m_delay;            //!< Number of Seconds to delay parts of test
  std::vector<uint32_t> m_bytesSent {std::vector<uint32_t> (4, 0)}; //!< sent data counters
  std::vector<uint32_t> m_bytesReceived {std::vector<uint32_t> (4, 0)}; //!< receive data counters

  /* Applications */
  Ptr<V4Ping>               m_v4ping;            //!< V4Ping Application
  Ptr<PacketSink>           m_packetSinkUp[4];   //!< PacketSink Applications for Upload flows
  Ptr<PacketSink>           m_packetSinkDown[4]; //!< PacketSink Applications for Download flows
  Ptr<BulkSendApplication>  m_bulkSendUp[4];     //!< BulkSend Applications for Upload flows
  Ptr<BulkSendApplication>  m_bulkSendDown[4];   //!< BulkSend Applications for Download flows
  Ptr<UdpEchoServer>        m_udpserver[3];      //!< UdpEchoServer Applications
  Ptr<UdpEchoClient>        m_udpclient[3];      //!< UdpEchoClient Applications
};

} // namespace ns3

#endif /* FLENT_APPLICATION_H */
