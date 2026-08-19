/*
 * Copyright (c) 2010 Georgia Institute of Technology
 * Copyright (c) 2020 Harsha Sharma : Flent application
 * Copyright (c) 2021 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
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

#include "bulk-send-application.h"
#include "nlohmann/json.hpp"
#include "packet-sink.h"
#include "udp-echo-client.h"
#include "udp-echo-server.h"

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/node-list.h"
#include "ns3/ping.h"
#include "ns3/ptr.h"

#include <string>

namespace ns3
{

class SeqTsEchoHeader;

/**
 * @ingroup applications
 * @defgroup flent FlentApplication
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
 * @ingroup flent
 *
 * @brief Flent is a network benchmarking tool
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
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    FlentApplication();

    ~FlentApplication() override;

    /**
     * @brief Add Flent Meta Data
     *
     * @param [out] j Json output object
     */
    void AddMetadata(nlohmann::json& j);

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
     * @sa Application::DoInitialize
     */
    void DoInitialize() override;

    /**
     * @sa Application::DoDispose
     */
    void DoDispose() override;

  private:
    // inherited from Application base class.
    void StartApplication() override; // Called at time specified by Start
    void StopApplication() override;  // Called at time specified by Stop

    /**
     * @brief Iterates through the node list and finds the node
     * pointer for the given hostAddress
     * @param [in] hostAddress host node IP
     * @return The node with the specified IP.
     *
     */
    Ptr<Node> GetHostNode(Ipv4Address hostAddress) const;

    /**
     * @brief Ping RTT Trace function
     * @param seq Echo sequence number
     * @param rtt Round Trip Time delta from the sent time
     */
    void TraceReceivedPing(uint16_t seq, Time rtt);

    /**
     * @brief UDP Ping RTT Trace function
     * @param packet UDP Ping Packet
     * @param address Address of the destination
     * @param localAddress Local address of the sender
     * @param header SeqTsEchoHeader header
     */
    void TraceReceivedUdpPing1(Ptr<const Packet> packet,
                               const Address& address,
                               const Address& localAddress,
                               const SeqTsEchoHeader& header);

    /**
     * @brief UDP Ping RTT Trace function
     * @param packet UDP Ping Packet
     * @param address Address of the destination
     * @param localAddress Local address of the sender
     * @param header SeqTsEchoHeader header
     */
    void TraceReceivedUdpPing2(Ptr<const Packet> packet,
                               const Address& address,
                               const Address& localAddress,
                               const SeqTsEchoHeader& header);

    /**
     * @brief UDP Ping RTT Trace function
     * @param packet UDP Ping Packet
     * @param address Address of the destination
     * @param localAddress Local address of the sender
     * @param header SeqTsEchoHeader header
     */
    void TraceReceivedUdpPing3(Ptr<const Packet> packet,
                               const Address& address,
                               const Address& localAddress,
                               const SeqTsEchoHeader& header);

    /**
     * @brief Calculate the Time in UTC time format
     * from the number of seconds.
     * @return The time in UTC format
     *
     */
    std::string GetUtcFormatTime() const;

    /**
     * @brief Goodput calculation for the i-th flow
     * based on the bytes sent.
     *
     * Index i can have various values from 0 to 3 for
     * the four upload flows in case of RRUL test.
     *
     * @param [in] name Name of the flow
     * @param [in] i index of the flow
     */
    void GoodputSamplingUpload(std::string name, int i);

    /**
     * @brief Goodput calculation for the i-th flow
     * based on the bytes received.
     *
     * Index i can have various values from 0 to 3 for
     * the four download flows in case of RRUL test.
     *
     * @param [in] name Name of the flow
     * @param [in] i index of the flow
     */
    void GoodputSamplingDownload(std::string name, int i);

    /**
     * @brief Fill x_values parameter in flent file.
     * Adds x parameter values with the difference of stepSize
     */
    void FillXValues();

    /**
     * @brief Process raw values and add the processed result in the flent file.
     */
    void ProcessRawValues();

    double m_currTime;            //!< Derived epoch anchor in seconds
    nlohmann::json m_output;      //!< Json output
    Time m_length;                //!< Test duration
    std::string m_outputFilename; ///< Output file path (see OutputFilename attribute)
    std::string m_testName;       //!< Flent test name
    Time m_t0;                    ///< Epoch anchor for output timestamps (see T0 attribute)
    bool m_useWallClockT0; ///< Use the wall clock instead of m_t0 (see UseWallClockT0 attribute)
    Ptr<Node> m_hostNode;  //!< Host Node
    Address m_hostAddress; //!< Host address
    Address m_localBindAddress; //!< Local bind address
    std::string m_imageText;    //!< Text to be included in plot
    Time m_stepSize;            //!< Measurement data point step size
    std::array<uint32_t, 4> m_bytesSent{};     //!< sent data counters
    std::array<uint32_t, 4> m_bytesReceived{}; //!< receive data counters

    /* Applications */
    Ptr<Ping> m_ping;                           //!< Ping Application for latency measurement (ping, rrul tests)

    // The RRUL (Realtime Response Under Load) test specification (rrul.conf from real flent) defines 
    // 4 concurrent TCP streams in each direction, assigned to different QoS markings (BE, BK, CS5, EF). 
    // This helps test how different traffic classes are handled when the link is under heavy load
    std::array<Ptr<PacketSink>, 4> m_packetSinkUp;          //!< PacketSink Applications for Upload flows (tcp_upload, rrul tests)
    std::array<Ptr<PacketSink>, 4> m_packetSinkDown;        //!< PacketSink Applications for Download flows (tcp_download, rrul tests)
    std::array<Ptr<BulkSendApplication>, 4> m_bulkSendUp;   //!< BulkSend Applications for Upload flows (tcp_upload, rrul tests)
    std::array<Ptr<BulkSendApplication>, 4> m_bulkSendDown; //!< BulkSend Applications for Download flows (tcp_download, rrul tests)

    // The RRUL specification defines 3 UDP ping streams marked with different QoS
    // markings (EF, BK, and BE) to measure latency and jitter for different
    // traffic classes under heavy load.
    std::array<Ptr<UdpEchoServer>, 3> m_udpserver;          //!< UdpEchoServer Applications
    std::array<Ptr<UdpEchoClient>, 3> m_udpclient;          //!< UdpEchoClient Applications
};

} // namespace ns3

#endif /* FLENT_APPLICATION_H */
