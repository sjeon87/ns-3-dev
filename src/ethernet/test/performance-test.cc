/*
 * Copyright (c) 2026 Somaiya Vidyavihar University, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manmita Das <das.manmita12@gmail.com>
 */

#include "ns3/bulk-send-helper.h"
#include "ns3/config.h"
#include "ns3/csma-helper.h"
#include "ns3/data-rate.h"
#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/ethernet-helper.h"
#include "ns3/ethernet-mac.h"
#include "ns3/ethernet-net-device.h"
#include "ns3/inet-socket-address.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/log.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/ping-helper.h"
#include "ns3/ping.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/pointer.h"
#include "ns3/queue-disc.h"
#include "ns3/queue.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/tcp-header.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/test.h"
#include "ns3/traffic-control-layer.h"
#include "ns3/uinteger.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace ns3;
using namespace ns3::ethernet;

NS_LOG_COMPONENT_DEFINE("EthernetPerformanceTest");

using namespace ns3;
using namespace ns3::ethernet;

/**
 * @ingroup ethernet
 * @ingroup tests
 *
 * @brief The link technology whose performance is being measured.
 */
enum class LinkTechnology
{
    POINT_TO_POINT, //!< PointToPointNetDevice over a PointToPointChannel
    CSMA,           //!< CsmaNetDevice over a CsmaChannel
    ETHERNET        //!< EthernetNetDevice over an EthernetChannel
};

/**
 * @brief Get the human readable name of a link technology.
 *
 * @param technology The link technology.
 * @return The name used in the reports and in the CSV files.
 */
std::string
TechnologyName(LinkTechnology technology)
{
    switch (technology)
    {
    case LinkTechnology::POINT_TO_POINT:
        return "PointToPoint";

    case LinkTechnology::CSMA:
        return "Csma";

    case LinkTechnology::ETHERNET:
        return "Ethernet";
    }

    return "unknown";
}

/// Data rate of each of the two links.
const DataRate LINK_RATE("100Mbps");

/// One-way propagation delay of each of the two links.
const Time LINK_DELAY = NanoSeconds(100);

/// Cable length used to obtain LINK_DELAY on an EthernetChannel, in metres.
constexpr double ETHERNET_CABLE_LENGTH = 20.0;

/// Propagation speed used to obtain LINK_DELAY on an EthernetChannel, in m/s.
constexpr double ETHERNET_PROPAGATION_SPEED = 200000000.0;

/// MTU configured on every net device.
constexpr uint16_t DEVICE_MTU = 1500;

/// Transmit queue size configured on every net device.
constexpr const char* DEVICE_QUEUE_SIZE = "100p";

/// TCP port the BulkSendApplication connects to.
constexpr uint16_t TCP_PORT = 5000;

/// Payload handed to the socket on each BulkSendApplication write.
constexpr uint32_t TCP_SEND_SIZE = 1448;

/// Total number of bytes the BulkSendApplication transfers.
constexpr uint64_t TCP_TOTAL_BYTES = 10000000;

/// Time at which the PacketSink starts listening.
const Time TCP_SINK_START = Seconds(0.5);

/// Time at which the BulkSendApplication starts.
const Time TCP_SOURCE_START = Seconds(1.0);

/// Upper bound on the TCP simulation; the transfer is expected to end earlier.
const Time TCP_SIMULATION_STOP = Seconds(30.0);

/// Number of echo requests whose replies are measured.
constexpr uint32_t PING_COUNT = 20;

/// Number of echo requests sent, but not measured, ahead of the measured ones.
///
/// CSMA and Ethernet have to resolve the address of the next hop before the
/// first echo request can leave, and ns-3 spreads ARP requests over a random
/// jitter of up to 10 ms per hop. PointToPoint needs no address resolution at
/// all, so counting the first exchange would compare address resolution rather
/// than forwarding latency.
constexpr uint32_t PING_WARMUP_COUNT = 2;

/// Interval between two echo requests.
const Time PING_INTERVAL = MilliSeconds(50);

/// Number of payload bytes in each echo request.
constexpr uint32_t PING_SIZE = 56;

/// Time at which the Ping application starts.
const Time PING_START = Seconds(1.0);

/// Per-technology summary of every metric, one row per technology.
constexpr const char* SUMMARY_CSV = "ethernet-performance-summary.csv";

/// Congestion window against time, one row per congestion window update.
constexpr const char* CWND_CSV = "ethernet-performance-cwnd.csv";

// ---------------------------------------------------------------------------
// Measurement collectors
// ---------------------------------------------------------------------------

/**
 * @ingroup ethernet
 * @ingroup tests
 *
 * @brief Collects the round trip times and the loss of a Ping application.
 *
 * The report emitted by Ping when it stops rounds every round trip time to a
 * whole millisecond, which is far coarser than the latency of the links under
 * test, so the samples are gathered from the per-packet traces instead.
 */
class PingCollector
{
  public:
    /**
     * Constructor.
     *
     * @param firstMeasured The sequence number of the first measured request;
     *                      everything before it is treated as a warm-up.
     */
    PingCollector(uint16_t firstMeasured)
        : m_firstMeasured(firstMeasured)
    {
    }

    /**
     * Record an echo request transmission.
     *
     * @param sequenceNumber The sequence number of the request.
     * @param packet The transmitted packet.
     */
    void Tx(uint16_t sequenceNumber, Ptr<Packet> packet)
    {
        if (sequenceNumber < m_firstMeasured)
        {
            return;
        }

        ++m_tx;
    }

    /**
     * Record the round trip time of an echo reply.
     *
     * @param sequenceNumber The sequence number of the reply.
     * @param rtt The round trip time.
     */
    void Rtt(uint16_t sequenceNumber, Time rtt)
    {
        if (sequenceNumber < m_firstMeasured)
        {
            return;
        }

        if (m_rtts.empty())
        {
            m_min = rtt;
            m_max = rtt;
        }
        else
        {
            m_min = std::min(m_min, rtt);
            m_max = std::max(m_max, rtt);
        }

        m_rtts.push_back(rtt);
    }

    /**
     * Record an unanswered echo request.
     *
     * @param sequenceNumber The sequence number of the request.
     * @param reason The reason the request went unanswered.
     */
    void Drop(uint16_t sequenceNumber, Ping::DropReason reason)
    {
        if (sequenceNumber < m_firstMeasured)
        {
            return;
        }

        ++m_drops;
    }

    /**
     * @return The number of echo requests sent.
     */
    uint32_t GetTx() const
    {
        return m_tx;
    }

    /**
     * @return The number of echo replies received.
     */
    uint32_t GetRx() const
    {
        return m_rtts.size();
    }

    /**
     * @return The number of echo requests reported as unanswered.
     */
    uint32_t GetDrops() const
    {
        return m_drops;
    }

    /**
     * @return The fraction of echo requests left unanswered, as a percentage.
     */
    double GetLossPercent() const
    {
        return m_tx ? static_cast<double>(m_tx - GetRx()) * 100.0 / static_cast<double>(m_tx) : 0.0;
    }

    /**
     * @return The smallest round trip time, in ms.
     */
    double GetMinMs() const
    {
        return m_rtts.empty() ? 0.0 : ToMs(m_min);
    }

    /**
     * @return The average round trip time, in ms.
     */
    double GetAvgMs() const
    {
        if (m_rtts.empty())
        {
            return 0.0;
        }

        double sum = 0.0;

        for (const auto& rtt : m_rtts)
        {
            sum += ToMs(rtt);
        }

        return sum / static_cast<double>(m_rtts.size());
    }

    /**
     * @return The largest round trip time, in ms.
     */
    double GetMaxMs() const
    {
        return m_rtts.empty() ? 0.0 : ToMs(m_max);
    }

    /**
     * @return The mean deviation of the round trip time, in ms.
     */
    double GetMdevMs() const
    {
        if (m_rtts.empty())
        {
            return 0.0;
        }

        const double average = GetAvgMs();
        double squaredSum = 0.0;

        for (const auto& rtt : m_rtts)
        {
            const double deviation = ToMs(rtt) - average;
            squaredSum += deviation * deviation;
        }

        return std::sqrt(squaredSum / static_cast<double>(m_rtts.size()));
    }

  private:
    /**
     * Convert a duration to milliseconds without losing sub-millisecond detail.
     *
     * @param time The duration.
     * @return The duration in ms.
     */
    static double ToMs(Time time)
    {
        return time.GetSeconds() * 1e3;
    }

    uint16_t m_firstMeasured; //!< Sequence number of the first measured request
    uint32_t m_tx{0};         //!< Echo requests sent
    uint32_t m_drops{0};      //!< Echo requests reported as unanswered
    Time m_min{0};            //!< Smallest round trip time
    Time m_max{0};            //!< Largest round trip time
    std::vector<Time> m_rtts; //!< Every round trip time sample
};

/**
 * @ingroup ethernet
 * @ingroup tests
 *
 * @brief Collects the bytes and the timing of a TCP bulk transfer.
 *
 * The transfer time is measured at the receiver, from the arrival of the first
 * payload byte to the arrival of the last one, so that it is not polluted by
 * the connection establishment handshake.
 */
class TcpFlowCollector
{
  public:
    /**
     * Record a payload reception at the PacketSink.
     *
     * @param packet The received packet.
     * @param from The sender address.
     */
    void Rx(Ptr<const Packet> packet, const Address& from)
    {
        if (m_bytes == 0)
        {
            m_firstRx = Simulator::Now();
        }

        m_bytes += packet->GetSize();
        m_lastRx = Simulator::Now();
    }

    /**
     * Record a retransmission by the sending TCP socket.
     *
     * @param packet The retransmitted packet.
     * @param header The TCP header.
     * @param localAddress The local address.
     * @param peerAddress The peer address.
     * @param socket The sending socket.
     */
    void Retransmission(Ptr<const Packet> packet,
                        const TcpHeader& header,
                        const Address& localAddress,
                        const Address& peerAddress,
                        Ptr<const TcpSocketBase> socket)
    {
        ++m_retransmissions;
    }

    /**
     * @return The total number of payload bytes received.
     */
    uint64_t GetBytes() const
    {
        return m_bytes;
    }

    /**
     * @return The elapsed time between the first and the last payload reception.
     */
    Time GetTransferTime() const
    {
        return m_bytes ? m_lastRx - m_firstRx : Time(0);
    }

    /**
     * @return The time at which the last payload byte was received.
     */
    Time GetLastRxTime() const
    {
        return m_lastRx;
    }

    /**
     * @return The throughput of the transfer, in Mbit/s.
     */
    double GetThroughputMbps() const
    {
        const Time transferTime = GetTransferTime();

        if (!transferTime.IsStrictlyPositive())
        {
            return 0.0;
        }

        return static_cast<double>(m_bytes) * 8.0 / transferTime.GetSeconds() / 1e6;
    }

    /**
     * @return The number of segments retransmitted by the sender.
     */
    uint32_t GetRetransmissions() const
    {
        return m_retransmissions;
    }

  private:
    uint64_t m_bytes{0};           //!< Payload bytes received by the PacketSink
    Time m_firstRx{0};             //!< Time of the first payload reception
    Time m_lastRx{0};              //!< Time of the last payload reception
    uint32_t m_retransmissions{0}; //!< Segments retransmitted by the sender
};

/**
 * @ingroup ethernet
 * @ingroup tests
 *
 * @brief Collects the congestion window of the sending TCP socket over time.
 */
class CwndCollector
{
  public:
    /**
     * Record a congestion window update.
     *
     * @param oldCwnd The previous congestion window, in bytes.
     * @param newCwnd The new congestion window, in bytes.
     */
    void CwndChange(uint32_t oldCwnd, uint32_t newCwnd)
    {
        if (m_samples.empty())
        {
            m_min = newCwnd;
            m_max = newCwnd;
        }
        else
        {
            m_min = std::min(m_min, newCwnd);
            m_max = std::max(m_max, newCwnd);
        }

        if (newCwnd < oldCwnd)
        {
            ++m_reductions;
        }

        m_sum += newCwnd;
        m_samples.emplace_back(Simulator::Now(), newCwnd);
    }

    /**
     * @return The smallest congestion window observed, in bytes.
     */
    double GetMinBytes() const
    {
        return m_samples.empty() ? 0.0 : static_cast<double>(m_min);
    }

    /**
     * @return The time average of the congestion window, in bytes.
     */
    double GetAverageBytes() const
    {
        return m_samples.empty()
                   ? 0.0
                   : static_cast<double>(m_sum) / static_cast<double>(m_samples.size());
    }

    /**
     * @return The largest congestion window observed, in bytes.
     */
    double GetMaxBytes() const
    {
        return m_samples.empty() ? 0.0 : static_cast<double>(m_max);
    }

    /**
     * @return The number of times the congestion window was reduced.
     */
    uint32_t GetReductions() const
    {
        return m_reductions;
    }

    /**
     * @return Every recorded (time, congestion window) pair.
     */
    const std::vector<std::pair<Time, uint32_t>>& GetSamples() const
    {
        return m_samples;
    }

  private:
    uint32_t m_min{0};                                //!< Smallest congestion window
    uint32_t m_max{0};                                //!< Largest congestion window
    uint64_t m_sum{0};                                //!< Sum of every sample
    uint32_t m_reductions{0};                         //!< Number of window reductions
    std::vector<std::pair<Time, uint32_t>> m_samples; //!< The recorded samples
};

/**
 * @ingroup ethernet
 * @ingroup tests
 *
 * @brief Counts the packets dropped along the forwarding path.
 *
 * Drops are gathered at three places that every technology has in common: the
 * transmit queue inside each net device, the root queue disc installed on each
 * net device by the traffic control layer, and the IPv4 layer of each node.
 */
class DropCollector
{
  public:
    /**
     * Remember the transmit queue of each net device so that its drop counters
     * can be read once the simulation has ended.
     *
     * The transmit queue of an EthernetNetDevice belongs to its MAC, while the
     * other two technologies expose it directly on the device.
     *
     * @param devices The net devices of the topology.
     * @param technology The link technology under test.
     */
    void TrackDeviceQueues(const NetDeviceContainer& devices, LinkTechnology technology)
    {
        for (auto it = devices.Begin(); it != devices.End(); ++it)
        {
            PointerValue queue;

            if (technology == LinkTechnology::ETHERNET)
            {
                DynamicCast<EthernetNetDevice>(*it)->GetMac()->GetAttribute("TxQueue", queue);
            }
            else
            {
                (*it)->GetAttribute("TxQueue", queue);
            }

            m_deviceQueues.push_back(queue.Get<QueueBase>());
        }
    }

    /**
     * Remember the root queue disc of each net device and connect to the IPv4
     * drop trace of each node.
     *
     * @param nodes The nodes of the topology.
     * @param devices The net devices of the topology.
     */
    void TrackNodes(const NodeContainer& nodes, const NetDeviceContainer& devices)
    {
        for (auto it = nodes.Begin(); it != nodes.End(); ++it)
        {
            (*it)->GetObject<Ipv4L3Protocol>()->TraceConnectWithoutContext(
                "Drop",
                MakeCallback(&DropCollector::Ipv4Drop, this));
        }

        for (auto it = devices.Begin(); it != devices.End(); ++it)
        {
            Ptr<TrafficControlLayer> tc = (*it)->GetNode()->GetObject<TrafficControlLayer>();
            Ptr<QueueDisc> disc = tc->GetRootQueueDiscOnDevice(*it);

            if (disc)
            {
                m_queueDiscs.push_back(disc);
            }
        }
    }

    /**
     * @return The total number of packets dropped by the device transmit queues.
     */
    uint32_t GetDeviceQueueDrops() const
    {
        uint32_t drops = 0;

        for (const auto& queue : m_deviceQueues)
        {
            drops += queue->GetTotalDroppedPackets();
        }

        return drops;
    }

    /**
     * @return The total number of packets dropped by the root queue discs.
     */
    uint32_t GetQueueDiscDrops() const
    {
        uint32_t drops = 0;

        for (const auto& disc : m_queueDiscs)
        {
            drops += disc->GetStats().nTotalDroppedPackets;
        }

        return drops;
    }

    /**
     * @return The total number of packets dropped by the IPv4 layers.
     */
    uint32_t GetIpv4Drops() const
    {
        return m_ipv4Drops;
    }

    /**
     * @return The total number of packets dropped anywhere in the topology.
     */
    uint32_t GetTotalDrops() const
    {
        return GetDeviceQueueDrops() + GetQueueDiscDrops() + GetIpv4Drops();
    }

  private:
    /**
     * Record a drop by the IPv4 layer of a node.
     *
     * @param header The IPv4 header of the dropped packet.
     * @param packet The dropped packet.
     * @param reason The reason the packet was dropped.
     * @param ipv4 The IPv4 stack that dropped the packet.
     * @param interface The interface the packet was dropped on.
     */
    void Ipv4Drop(const Ipv4Header& header,
                  Ptr<const Packet> packet,
                  Ipv4L3Protocol::DropReason reason,
                  Ptr<Ipv4> ipv4,
                  uint32_t interface)
    {
        ++m_ipv4Drops;
    }

    std::vector<Ptr<QueueBase>> m_deviceQueues; //!< Transmit queue of each net device
    std::vector<Ptr<QueueDisc>> m_queueDiscs;   //!< Root queue disc of each net device
    uint32_t m_ipv4Drops{0};                    //!< Packets dropped by the IPv4 layers
};

// ---------------------------------------------------------------------------
// Results
// ---------------------------------------------------------------------------

/**
 * @ingroup ethernet
 * @ingroup tests
 *
 * @brief Every metric measured for one link technology.
 */
struct TechnologyResult
{
    std::string name; //!< Name of the link technology

    uint32_t pingTx{0};        //!< Echo requests sent
    uint32_t pingRx{0};        //!< Echo replies received
    uint32_t pingDrops{0};     //!< Echo requests reported as unanswered
    double pingLossPercent{0}; //!< Echo reply loss, as a percentage
    double pingRttMinMs{0};    //!< Smallest round trip time, in ms
    double pingRttAvgMs{0};    //!< Average round trip time, in ms
    double pingRttMaxMs{0};    //!< Largest round trip time, in ms
    double pingRttMdevMs{0};   //!< Mean deviation of the round trip time, in ms

    uint64_t tcpBytes{0};           //!< Payload bytes received
    double tcpTransferTimeMs{0};    //!< Duration of the transfer, in ms
    double tcpThroughputMbps{0};    //!< Throughput of the transfer, in Mbit/s
    uint32_t tcpRetransmissions{0}; //!< Segments retransmitted by the sender

    double cwndMinBytes{0};     //!< Smallest congestion window, in bytes
    double cwndAvgBytes{0};     //!< Average congestion window, in bytes
    double cwndMaxBytes{0};     //!< Largest congestion window, in bytes
    uint32_t cwndSamples{0};    //!< Number of congestion window updates
    uint32_t cwndReductions{0}; //!< Number of congestion window reductions

    uint32_t deviceQueueDrops{0}; //!< Packets dropped by the device transmit queues
    uint32_t queueDiscDrops{0};   //!< Packets dropped by the root queue discs
    uint32_t ipv4Drops{0};        //!< Packets dropped by the IPv4 layers
};

/**
 * @ingroup ethernet
 * @ingroup tests
 *
 * @brief Accumulates the results of every technology and reports them.
 *
 * The test cases of the suite run one after the other, each measuring a single
 * technology. The collected rows are written to CSV files and, once every
 * technology of the suite has reported, printed as a side by side comparison.
 */
class ComparisonReport
{
  public:
    /**
     * @return The single instance of the report.
     */
    static ComparisonReport& Get()
    {
        static ComparisonReport report;
        return report;
    }

    /**
     * Declare how many technologies the suite is going to measure, so that the
     * comparison table can be printed as soon as the last one has reported.
     *
     * @param count The number of technologies.
     */
    void SetExpectedCount(std::size_t count)
    {
        m_expected = count;
    }

    /**
     * Add the results of one technology.
     *
     * @param result The measured metrics.
     * @param cwnd The congestion window samples of the technology.
     */
    void Add(const TechnologyResult& result, const CwndCollector& cwnd)
    {
        WriteSummaryRow(result);
        WriteCwndRows(result.name, cwnd);

        m_results.push_back(result);

        if (m_results.size() == m_expected)
        {
            PrintComparison();
        }
    }

  private:
    /**
     * Append the summary row of one technology, creating the file and its
     * header on the first call.
     *
     * @param result The measured metrics.
     */
    void WriteSummaryRow(const TechnologyResult& result)
    {
        std::ofstream file(SUMMARY_CSV, m_results.empty() ? std::ios::trunc : std::ios::app);

        if (!file.is_open())
        {
            NS_FATAL_ERROR("Unable to open " << SUMMARY_CSV);
        }

        if (m_results.empty())
        {
            file << "technology,ping_tx,ping_rx,ping_drops,ping_loss_percent,ping_rtt_min_ms,"
                    "ping_rtt_avg_ms,ping_rtt_max_ms,ping_rtt_mdev_ms,tcp_bytes,"
                    "tcp_transfer_time_ms,tcp_throughput_mbps,tcp_retransmissions,"
                    "cwnd_min_bytes,cwnd_avg_bytes,cwnd_max_bytes,cwnd_samples,"
                    "cwnd_reductions,device_queue_drops,queue_disc_drops,ipv4_drops\n";
        }

        file << result.name << ',' << result.pingTx << ',' << result.pingRx << ','
             << result.pingDrops << ',' << result.pingLossPercent << ',' << result.pingRttMinMs
             << ',' << result.pingRttAvgMs << ',' << result.pingRttMaxMs << ','
             << result.pingRttMdevMs << ',' << result.tcpBytes << ',' << result.tcpTransferTimeMs
             << ',' << result.tcpThroughputMbps << ',' << result.tcpRetransmissions << ','
             << result.cwndMinBytes << ',' << result.cwndAvgBytes << ',' << result.cwndMaxBytes
             << ',' << result.cwndSamples << ',' << result.cwndReductions << ','
             << result.deviceQueueDrops << ',' << result.queueDiscDrops << ',' << result.ipv4Drops
             << '\n';
    }

    /**
     * Append every congestion window sample of one technology, creating the
     * file and its header on the first call.
     *
     * @param name The name of the technology.
     * @param cwnd The congestion window samples.
     */
    void WriteCwndRows(const std::string& name, const CwndCollector& cwnd)
    {
        std::ofstream file(CWND_CSV, m_results.empty() ? std::ios::trunc : std::ios::app);

        if (!file.is_open())
        {
            NS_FATAL_ERROR("Unable to open " << CWND_CSV);
        }

        if (m_results.empty())
        {
            file << "technology,time_s,cwnd_bytes\n";
        }

        for (const auto& [time, bytes] : cwnd.GetSamples())
        {
            file << name << ',' << std::setprecision(12) << time.GetSeconds() << ',' << bytes
                 << '\n';
        }
    }

    /**
     * Print the metrics of every technology side by side.
     */
    void PrintComparison() const
    {
        constexpr int LABEL_WIDTH = 26;
        constexpr int VALUE_WIDTH = 16;

        const int tableWidth = LABEL_WIDTH + VALUE_WIDTH * static_cast<int>(m_results.size());
        const std::string rule(tableWidth, '=');
        const std::string thinRule(tableWidth, '-');

        std::ostringstream setup;
        setup << LINK_RATE.GetBitRate() / 1000000 << " Mbps and " << LINK_DELAY.As(Time::NS)
              << " per link, MTU " << DEVICE_MTU << ", device queue " << DEVICE_QUEUE_SIZE << ", "
              << TCP_TOTAL_BYTES << " byte transfer";

        std::cout << '\n'
                  << rule << '\n'
                  << "Link technology comparison\n"
                  << setup.str() << '\n'
                  << rule << '\n';

        std::cout << std::left << std::setw(LABEL_WIDTH) << "Metric";

        for (const auto& result : m_results)
        {
            std::cout << std::setw(VALUE_WIDTH) << result.name;
        }

        std::cout << '\n' << thinRule << '\n';

        auto row = [this](const std::string& label, auto&& extract, int precision) {
            std::cout << std::left << std::setw(LABEL_WIDTH) << label;

            for (const auto& result : m_results)
            {
                std::ostringstream value;
                value << std::fixed << std::setprecision(precision) << extract(result);
                std::cout << std::setw(VALUE_WIDTH) << value.str();
            }

            std::cout << '\n';
        };

        std::cout << "PING\n";
        row("  requests sent", [](const auto& r) { return r.pingTx; }, 0);
        row("  replies received", [](const auto& r) { return r.pingRx; }, 0);
        row("  unanswered", [](const auto& r) { return r.pingDrops; }, 0);
        row("  loss (%)", [](const auto& r) { return r.pingLossPercent; }, 2);
        row("  rtt min (ms)", [](const auto& r) { return r.pingRttMinMs; }, 4);
        row("  rtt avg (ms)", [](const auto& r) { return r.pingRttAvgMs; }, 4);
        row("  rtt max (ms)", [](const auto& r) { return r.pingRttMaxMs; }, 4);
        row("  rtt mdev (ms)", [](const auto& r) { return r.pingRttMdevMs; }, 4);

        std::cout << "TCP BULK TRANSFER\n";
        row("  bytes received", [](const auto& r) { return r.tcpBytes; }, 0);
        row("  transfer time (ms)", [](const auto& r) { return r.tcpTransferTimeMs; }, 3);
        row("  throughput (Mbps)", [](const auto& r) { return r.tcpThroughputMbps; }, 3);
        row("  retransmissions", [](const auto& r) { return r.tcpRetransmissions; }, 0);

        std::cout << "CONGESTION WINDOW\n";
        row("  min (bytes)", [](const auto& r) { return r.cwndMinBytes; }, 0);
        row("  avg (bytes)", [](const auto& r) { return r.cwndAvgBytes; }, 0);
        row("  max (bytes)", [](const auto& r) { return r.cwndMaxBytes; }, 0);
        row("  updates", [](const auto& r) { return r.cwndSamples; }, 0);
        row("  reductions", [](const auto& r) { return r.cwndReductions; }, 0);

        std::cout << "PACKET DROPS (TCP RUN)\n";
        row("  device tx queue", [](const auto& r) { return r.deviceQueueDrops; }, 0);
        row("  queue disc", [](const auto& r) { return r.queueDiscDrops; }, 0);
        row("  ipv4 layer", [](const auto& r) { return r.ipv4Drops; }, 0);

        std::cout << rule << '\n'
                  << "Summary written to " << SUMMARY_CSV << '\n'
                  << "Congestion window trace written to " << CWND_CSV << '\n'
                  << '\n';
    }

    std::vector<TechnologyResult> m_results; //!< The results reported so far
    std::size_t m_expected{0};               //!< Number of technologies to expect
};

// ---------------------------------------------------------------------------
// Test case
// ---------------------------------------------------------------------------

/**
 * @ingroup ethernet
 * @ingroup tests
 *
 * @brief Measure the performance of one link technology on a fixed topology.
 *
 * Three nodes are connected by two point to point links of identical data rate
 * and propagation delay:
 *
 * @verbatim
     10.1.1.0/24            10.1.2.0/24
   n0 ------------- n1 (router) ------------- n2
   sender          two interfaces          consumer
   @endverbatim
 *
 * Two independent simulations are run on that topology. The first one pings the
 * consumer from the sender, measuring the round trip time and the loss of an
 * otherwise idle network. The second one runs a TCP bulk transfer of a fixed
 * size from the sender to the consumer, measuring the throughput, the transfer
 * time, the congestion window over time, the retransmissions, and the drops.
 *
 * Keeping the two apart means the round trip time reflects the latency of the
 * technology rather than the queueing delay caused by the bulk transfer.
 */
class LinkTechnologyPerformanceTestCase : public TestCase
{
  public:
    /**
     * Constructor.
     *
     * @param technology The link technology to measure.
     */
    LinkTechnologyPerformanceTestCase(LinkTechnology technology)
        : TestCase(TechnologyName(technology) + " performance"),
          m_technology(technology)
    {
    }

  private:
    /**
     * @ingroup ethernet
     * @ingroup tests
     *
     * @brief The three node topology shared by the two simulations.
     */
    struct Topology
    {
        NodeContainer nodes;         //!< Sender, router and consumer
        NetDeviceContainer devices;  //!< Devices, in installation order
        Ipv4Address consumerAddress; //!< Address of the consumer, the traffic destination
    };

    /**
     * Create the two links of the topology using the technology under test.
     *
     * All three technologies are configured for the same data rate, the same
     * one-way propagation delay, the same MTU and the same queue size. Note
     * that an EthernetChannel derives its delay from a cable length and a
     * propagation speed rather than taking it directly, and that an
     * EthernetNetDevice derives its data rate from the link type negotiated
     * with its peer.
     *
     * @param nodes The three nodes of the topology.
     * @return The four created devices, in installation order.
     */
    NetDeviceContainer InstallDevices(NodeContainer nodes) const
    {
        NetDeviceContainer devices;

        switch (m_technology)
        {
        case LinkTechnology::POINT_TO_POINT: {
            PointToPointHelper p2p;
            p2p.SetDeviceAttribute("DataRate", DataRateValue(LINK_RATE));
            p2p.SetDeviceAttribute("Mtu", UintegerValue(DEVICE_MTU));
            p2p.SetChannelAttribute("Delay", TimeValue(LINK_DELAY));
            p2p.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue(DEVICE_QUEUE_SIZE));

            devices.Add(p2p.Install(nodes.Get(0), nodes.Get(1)));
            devices.Add(p2p.Install(nodes.Get(1), nodes.Get(2)));
            break;
        }

        case LinkTechnology::CSMA: {
            CsmaHelper csma;
            csma.SetChannelAttribute("DataRate", DataRateValue(LINK_RATE));
            csma.SetChannelAttribute("Delay", TimeValue(LINK_DELAY));
            csma.SetDeviceAttribute("Mtu", UintegerValue(DEVICE_MTU));
            csma.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue(DEVICE_QUEUE_SIZE));

            devices.Add(csma.Install(NodeContainer(nodes.Get(0), nodes.Get(1))));
            devices.Add(csma.Install(NodeContainer(nodes.Get(1), nodes.Get(2))));
            break;
        }

        case LinkTechnology::ETHERNET: {
            EthernetHelper ethernet;
            ethernet.SetDeviceAttribute("MaxSupportedEthernetLinkType",
                                        EnumValue<EthernetLinkType>(EthernetLinkType::BASE100_TX));
            ethernet.SetChannelAttribute("Length", DoubleValue(ETHERNET_CABLE_LENGTH));
            ethernet.SetChannelAttribute("Speed", DoubleValue(ETHERNET_PROPAGATION_SPEED));
            ethernet.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue(DEVICE_QUEUE_SIZE));

            devices.Add(ethernet.Install(nodes.Get(0), nodes.Get(1)));
            devices.Add(ethernet.Install(nodes.Get(1), nodes.Get(2)));

            for (auto it = devices.Begin(); it != devices.End(); ++it)
            {
                (*it)->SetMtu(DEVICE_MTU);
            }
            break;
        }
        }

        return devices;
    }

    /**
     * Build the topology, install the internet stack and populate the routes.
     *
     * @return The topology.
     */
    Topology BuildTopology() const
    {
        Topology topology;

        topology.nodes.Create(3);
        topology.devices = InstallDevices(topology.nodes);

        InternetStackHelper internet;
        internet.Install(topology.nodes);

        NetDeviceContainer senderSubnet(topology.devices.Get(0), topology.devices.Get(1));
        NetDeviceContainer consumerSubnet(topology.devices.Get(2), topology.devices.Get(3));

        Ipv4AddressHelper ipv4;

        ipv4.SetBase("10.1.1.0", "255.255.255.0");
        ipv4.Assign(senderSubnet);

        ipv4.SetBase("10.1.2.0", "255.255.255.0");
        Ipv4InterfaceContainer consumerInterfaces = ipv4.Assign(consumerSubnet);

        topology.consumerAddress = consumerInterfaces.GetAddress(1);

        Ipv4GlobalRoutingHelper::PopulateRoutingTables();

        return topology;
    }

    /**
     * Ping the consumer from the sender on an otherwise idle network.
     *
     * @param [out] result The round trip time and loss metrics.
     */
    void RunPingSimulation(TechnologyResult& result)
    {
        Topology topology = BuildTopology();

        PingHelper ping(topology.consumerAddress);
        ping.SetAttribute("Count", UintegerValue(0));
        ping.SetAttribute("Interval", TimeValue(PING_INTERVAL));
        ping.SetAttribute("Size", UintegerValue(PING_SIZE));
        ping.SetAttribute("VerboseMode", EnumValue(Ping::VerboseMode::SILENT));

        ApplicationContainer pingApps = ping.Install(topology.nodes.Get(0));

        PingCollector collector(PING_WARMUP_COUNT);
        Ptr<Application> pingApp = pingApps.Get(0);

        pingApp->TraceConnectWithoutContext("Tx", MakeCallback(&PingCollector::Tx, &collector));
        pingApp->TraceConnectWithoutContext("Rtt", MakeCallback(&PingCollector::Rtt, &collector));
        pingApp->TraceConnectWithoutContext("Drop", MakeCallback(&PingCollector::Drop, &collector));

        // The last request goes out one interval before the stop time, leaving
        // half an interval for its reply to come back.
        const uint32_t totalRequests = PING_WARMUP_COUNT + PING_COUNT;
        const Time pingStop = PING_START + PING_INTERVAL * (totalRequests - 1) + PING_INTERVAL / 2;

        pingApps.Start(PING_START);
        pingApps.Stop(pingStop);

        Simulator::Stop(pingStop + PING_INTERVAL);
        Simulator::Run();
        Simulator::Destroy();

        result.pingTx = collector.GetTx();
        result.pingRx = collector.GetRx();
        result.pingDrops = collector.GetDrops();
        result.pingLossPercent = collector.GetLossPercent();
        result.pingRttMinMs = collector.GetMinMs();
        result.pingRttAvgMs = collector.GetAvgMs();
        result.pingRttMaxMs = collector.GetMaxMs();
        result.pingRttMdevMs = collector.GetMdevMs();
    }

    /**
     * Transfer a fixed number of bytes from the sender to the consumer over TCP.
     *
     * @param [out] result The throughput, transfer time, congestion window and
     *                     drop metrics.
     * @param [out] cwnd The congestion window samples.
     */
    void RunTcpSimulation(TechnologyResult& result, CwndCollector& cwnd)
    {
        Topology topology = BuildTopology();

        DropCollector drops;
        drops.TrackDeviceQueues(topology.devices, m_technology);
        drops.TrackNodes(topology.nodes, topology.devices);

        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",
                                    InetSocketAddress(Ipv4Address::GetAny(), TCP_PORT));

        ApplicationContainer sinkApps = sinkHelper.Install(topology.nodes.Get(2));
        sinkApps.Start(TCP_SINK_START);

        BulkSendHelper sourceHelper("ns3::TcpSocketFactory",
                                    InetSocketAddress(topology.consumerAddress, TCP_PORT));
        sourceHelper.SetAttribute("MaxBytes", UintegerValue(TCP_TOTAL_BYTES));
        sourceHelper.SetAttribute("SendSize", UintegerValue(TCP_SEND_SIZE));

        ApplicationContainer sourceApps = sourceHelper.Install(topology.nodes.Get(0));
        sourceApps.Start(TCP_SOURCE_START);

        TcpFlowCollector flow;
        sinkApps.Get(0)->TraceConnectWithoutContext("Rx",
                                                    MakeCallback(&TcpFlowCollector::Rx, &flow));

        // BulkSendApplication creates its socket when it starts, and the sender
        // has no other TCP socket, so the socket of the transfer is the only
        // entry of the socket list just after the application has started.
        Simulator::Schedule(TCP_SOURCE_START + NanoSeconds(1), [&flow, &cwnd]() {
            const std::string socketPath = "/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/";

            NS_ASSERT_MSG(Config::ConnectWithoutContextFailSafe(
                              socketPath + "CongestionWindow",
                              MakeCallback(&CwndCollector::CwndChange, &cwnd)),
                          "Could not connect to the congestion window of the sending socket");

            NS_ASSERT_MSG(Config::ConnectWithoutContextFailSafe(
                              socketPath + "Retransmission",
                              MakeCallback(&TcpFlowCollector::Retransmission, &flow)),
                          "Could not connect to the retransmissions of the sending socket");
        });

        Simulator::Stop(TCP_SIMULATION_STOP);
        Simulator::Run();
        Simulator::Destroy();

        result.tcpBytes = flow.GetBytes();
        result.tcpTransferTimeMs = flow.GetTransferTime().GetMilliSeconds();
        result.tcpThroughputMbps = flow.GetThroughputMbps();
        result.tcpRetransmissions = flow.GetRetransmissions();

        result.cwndMinBytes = cwnd.GetMinBytes();
        result.cwndAvgBytes = cwnd.GetAverageBytes();
        result.cwndMaxBytes = cwnd.GetMaxBytes();
        result.cwndSamples = cwnd.GetSamples().size();
        result.cwndReductions = cwnd.GetReductions();

        result.deviceQueueDrops = drops.GetDeviceQueueDrops();
        result.queueDiscDrops = drops.GetQueueDiscDrops();
        result.ipv4Drops = drops.GetIpv4Drops();
    }

    void DoRun() override
    {
        TechnologyResult result;
        result.name = TechnologyName(m_technology);

        CwndCollector cwnd;

        RunPingSimulation(result);
        RunTcpSimulation(result, cwnd);

        ComparisonReport::Get().Add(result, cwnd);

        // The network is idle during the ping simulation and has no error
        // model, so every echo request must be answered.
        NS_TEST_ASSERT_MSG_EQ(result.pingTx, PING_COUNT, "Ping did not send every request");
        NS_TEST_ASSERT_MSG_EQ(result.pingRx, PING_COUNT, "Ping lost an echo reply");
        NS_TEST_ASSERT_MSG_EQ(result.pingDrops, 0, "Ping reported an unanswered request");
        NS_TEST_ASSERT_MSG_EQ(result.pingLossPercent, 0.0, "Ping reported loss on an idle network");
        NS_TEST_ASSERT_MSG_GT(result.pingRttMinMs, 0.0, "Ping reported a zero round trip time");
        NS_TEST_ASSERT_MSG_GT_OR_EQ(result.pingRttMaxMs,
                                    result.pingRttAvgMs,
                                    "Ping reported an average above its maximum");

        // The transfer is bounded and the simulation is given more than enough
        // time, so all of the requested bytes must be delivered.
        NS_TEST_ASSERT_MSG_EQ(result.tcpBytes,
                              TCP_TOTAL_BYTES,
                              "TCP transfer did not complete before the simulation ended");

        // The payload cannot travel faster than the line rate, and must be
        // slower than it because of the header and inter-frame overhead.
        const double linkRateMbps = static_cast<double>(LINK_RATE.GetBitRate()) / 1e6;

        NS_TEST_ASSERT_MSG_GT(result.tcpThroughputMbps, 0.0, "TCP throughput is zero");
        NS_TEST_ASSERT_MSG_LT(result.tcpThroughputMbps,
                              linkRateMbps,
                              "TCP payload throughput exceeded the link data rate");

        const double minimumTransferTimeMs =
            static_cast<double>(TCP_TOTAL_BYTES) * 8.0 / linkRateMbps / 1e3;

        NS_TEST_ASSERT_MSG_GT(result.tcpTransferTimeMs,
                              minimumTransferTimeMs,
                              "TCP transfer was faster than the link data rate allows");

        NS_TEST_ASSERT_MSG_GT(result.cwndSamples, 0, "No congestion window update was recorded");
        NS_TEST_ASSERT_MSG_GT_OR_EQ(result.cwndMaxBytes,
                                    result.cwndAvgBytes,
                                    "Average congestion window above its maximum");
    }

    LinkTechnology m_technology; //!< The link technology being measured
};

/**
 * @ingroup ethernet
 * @ingroup tests
 *
 * @brief Compare CSMA, PointToPoint and EthernetNetDevice on the same topology.
 */
class EthernetPerformanceTestSuite : public TestSuite
{
  public:
    EthernetPerformanceTestSuite()
        : TestSuite("ethernet-performance-test", Type::UNIT)
    {
        const std::vector<LinkTechnology> technologies{LinkTechnology::POINT_TO_POINT,
                                                       LinkTechnology::CSMA,
                                                       LinkTechnology::ETHERNET};

        ComparisonReport::Get().SetExpectedCount(technologies.size());

        for (const auto technology : technologies)
        {
            AddTestCase(new LinkTechnologyPerformanceTestCase(technology),
                        TestCase::Duration::QUICK);
        }
    }
};

static EthernetPerformanceTestSuite g_ethernetPerformanceTestSuite; //!< The testsuite
