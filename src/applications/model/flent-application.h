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

#ifndef FLENT_APPLICATION_H
#define FLENT_APPLICATION_H

#include <string>
#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/json.h"

namespace ns3 {

class V4Ping;
class PacketSink;
class BulkSendApplication;

/**
 * \ingroup applications
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

  void SetTest (std::string testname);
  void SetDuration (Time duration);
  void SetServerAddress (Address serverAddress);
  void SetIncludeText (std::string textInImage);
  void SetOutput (std::string imagename);
  void SetStepSize (Time stepsize);
  void SetDelay (Time delay);
  void AddMetadata (Json::Value &j);
  void SetServer ();
  void SetServerApp (Ptr<FlentApplication> serverApp);

protected:
  virtual void DoDispose (void);
private:

  // inherited from Application base class.
  virtual void StartApplication (void);    //Called at time specified by Start
  virtual void StopApplication (void);     //Called at time specified by Stop

  void ReceivePing (const Address &address, uint16_t seq, uint8_t ttl, Time t);
  void SendData (Ptr<const Packet> packet);
  void ReceiveData (Ptr<const Packet> packet, const Address &address);
  std::string GetUTCFormatTime (int sec);
  void GoodputSampling ();
  void GoodputSamplingDownload ();


  bool            m_server;        //!< Bool client or server
  double          m_currTime;      //!< Current time
  Json::Value     m_output;        //!< Json output
  Ptr<V4Ping>     m_v4ping;        //!< V4Ping application
  Ptr<PacketSink> m_packetSink;    //!< Packet sink application
  Ptr<BulkSendApplication> m_bulkSend; //!< Bulk send application
  Ptr<FlentApplication> m_serverApp; //!< Point to ns-3 flent server application
  Time            m_duration;      //!< Test duration
  std::string     m_testName;      //!< Flent test name
  Address         m_serverAddress; //!< Server address
  std::string     m_imageText;     //!< Text to be included in plot
  std::string     m_imageName;     //!< Name of the image to which plot is saved
  Time            m_stepSize;      //!< Measurment data point step size
  Time            m_delay;         //!< Number of Seconds to delay parts of test

};

} // namespace ns3

#endif /* FLENT_APPLICATION_H */
