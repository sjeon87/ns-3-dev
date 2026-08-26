/*
 * SPDX-License-Identifier: NIST-Software
 *
 * Author: Tom Henderson <tomh@tomh.org> (assisted by Claude Fable 5)
 */

/*
 * This example shows how to build and parse a MANET routing protocol
 * control packet with the RFC 5444 classes (Rfc5444Packet and related),
 * using an AODVv2 Route Request (RREQ) as the worked example. The message
 * layout follows draft-perkins-manet-aodvv2-06 Section 8.1 ("Route Request
 * Message Representation") with the type numbers proposed in its Section 13;
 * NHDP (RFC 6130) and OLSRv2 (RFC 7181) messages are built the same way.
 *
 * In the scenario, the router that owns 192.0.2.1 (OrigPrefix) initiates
 * route discovery for 192.0.2.99 (TargPrefix). Its own sequence number
 * (OrigSeqNum) is 5000, and the metric of the route back to it, in Hop Count
 * metric units, is 3. The RREQ carried inside the RFC 5444 packet is:
 *
 * +---------------------------------------------------------------------+
 * | RFC 5444 packet (version 0, no packet TLVs, no sequence number)     |
 * |                                                                     |
 * |  +---------------------------------------------------------------+  |
 * |  | Message: msg-type = RREQ (10), msg-addr-length = 3 (IPv4),    |  |
 * |  |          msg-hop-limit = 20 (MAX_HOPCOUNT)                    |  |
 * |  |                                                               |  |
 * |  |  Message TLV block: (empty; AODVv2 defines no RREQ msg TLVs)  |  |
 * |  |  +---------------------------------------------------------+  |  |
 * |  |  | Address block:                                          |  |  |
 * |  |  |   index 0:  192.0.2.1/32    (OrigPrefix)                |  |  |
 * |  |  |   index 1:  192.0.2.99/32   (TargPrefix)                |  |  |
 * |  |  +---------------------------------------------------------+  |  |
 * |  |  | Address TLV block:                                      |  |  |
 * |  |  |   ADDRESS_TYPE (131) multivalue, indexes 0-1:           |  |  |
 * |  |  |       index 0 -> ORIGPREFIX (0)                         |  |  |
 * |  |  |       index 1 -> TARGPREFIX (1)                         |  |  |
 * |  |  |   SEQ_NUM (130), index 0:  OrigSeqNum = 5000            |  |  |
 * |  |  |   PATH_METRIC (129), type ext = 1 (Hop Count metric),   |  |  |
 * |  |  |       index 0:  OrigMetric = 3                          |  |  |
 * |  |  +---------------------------------------------------------+  |  |
 * |  +---------------------------------------------------------------+  |
 * +---------------------------------------------------------------------+
 *
 * The roles of the two addresses are not implied by their positions; the
 * ADDRESS_TYPE TLV names the role of each covered address, carrying one
 * one-octet value per address (a "multivalue" TLV, RFC 5444 Section 5.4.2).
 * The SEQ_NUM and PATH_METRIC TLVs apply to index 0 only, and PATH_METRIC
 * uses its type extension to carry the AODVv2 MetricType.
 *
 * The packet serializes to these 37 octets (the address block head/mid
 * compression stores the shared prefix 192.0.2 only once):
 *
 *   00                    packet header: version 0, no flags
 *   0a                    msg-type = 10 (RREQ)
 *   43                    msg-flags/msg-addr-length: mhashoplimit | 3
 *   00 24                 msg-size = 36 octets
 *   14                    msg-hop-limit = 20
 *   00 00                 message TLV block, length 0
 *   02                    address block: 2 addresses
 *   80                    addr-flags: ahashead
 *   03 c0 00 02           head-length 3, head = 192.0.2
 *   01 63                 mids: .1 and .99
 *   00 13                 address TLV block, length 19
 *   83 34 00 01 02 00 01  ADDRESS_TYPE, multivalue over 0-1: {ORIG, TARG}
 *   82 50 00 02 13 88     SEQ_NUM, index 0: 5000
 *   81 d0 01 00 01 03     PATH_METRIC, type ext 1, index 0: 3
 *
 * The program builds the packet, prints its fields, serializes it, prints
 * the octets, and parses it back as a receiving AODVv2 router would,
 * producing this output:
 *
 *   RREQ sent:
 *     msg-hop-limit = 20
 *     address 0: 192.0.2.1/32 (OrigPrefix)
 *       SEQ_NUM = 5000
 *       PATH_METRIC = 3 (MetricType 1)
 *     address 1: 192.0.2.99/32 (TargPrefix)
 *
 *   Serialized RREQ packet (37 octets):
 *   00 0a 43 00 24 14 00 00 02 80 03 c0 00 02 01 63
 *   00 13 83 34 00 01 02 00 01 82 50 00 02 13 88 81
 *   d0 01 00 01 03
 *
 *   RREQ received:
 *     msg-hop-limit = 20
 *     address 0: 192.0.2.1/32 (OrigPrefix)
 *       SEQ_NUM = 5000
 *       PATH_METRIC = 3 (MetricType 1)
 *     address 1: 192.0.2.99/32 (TargPrefix)
 *
 * Run it with:
 *
 *   ./ns3 run rfc5444-example
 */

#include "ns3/command-line.h"
#include "ns3/ipv4-address.h"
#include "ns3/packet.h"
#include "ns3/rfc5444.h"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <variant>
#include <vector>

using namespace ns3;

/* AODVv2 RFC 5444 allocations (draft-perkins-manet-aodvv2-06 Section 13).
 * A real implementation would define these in its own header. */
constexpr uint8_t RREQ = 10;                 //!< Route Request message type (Table 20)
constexpr uint8_t PATH_METRIC = 129;         //!< Route metric address TLV (Table 22)
constexpr uint8_t SEQ_NUM = 130;             //!< Sequence number address TLV (Table 22)
constexpr uint8_t ADDRESS_TYPE = 131;        //!< Address role address TLV (Table 22)
constexpr uint8_t ORIGPREFIX = 0;            //!< ADDRESS_TYPE value (Table 24)
constexpr uint8_t TARGPREFIX = 1;            //!< ADDRESS_TYPE value (Table 24)
constexpr uint8_t METRIC_TYPE_HOP_COUNT = 1; //!< MetricType number (Table 23)
constexpr uint8_t MAX_HOPCOUNT = 20;         //!< msg-hop-limit for a new RREQ

/**
 * Build an AODVv2 RREQ packet (draft-perkins-manet-aodvv2-06 Section 8.1).
 *
 * @param origPrefix The prefix of the router initiating route discovery.
 * @param origSeqNum The sequence number of the initiating router.
 * @param origMetric The route metric back to origPrefix, in Hop Count units.
 * @param targPrefix The prefix a route is requested for.
 * @return The RFC 5444 packet carrying the RREQ.
 */
Rfc5444Packet
BuildRreq(Ipv4Address origPrefix, uint16_t origSeqNum, uint8_t origMetric, Ipv4Address targPrefix)
{
    // Address block (Table 3): OrigPrefix first, TargPrefix second. A plain
    // Ipv4Address converts to an address-block entry with the full /32
    // prefix length; to advertise a shorter prefix, use
    // Ipv4NetworkAddress(address, prefixLength) instead.
    Rfc5444AddressBlockIpv4 addressBlock;
    addressBlock.Addresses() = {origPrefix, targPrefix};

    // ADDRESS_TYPE (Tables 4 and 5) names the role of each address. It is a
    // multivalue TLV: SetValues() takes one equal-length value per covered
    // address, starting at the given index.
    Rfc5444AddressTlv addressType;
    addressType.SetType(ADDRESS_TYPE);
    addressType.SetValues(0, {{ORIGPREFIX}, {TARGPREFIX}});
    addressBlock.Tlvs().push_back(addressType);

    // SEQ_NUM (Table 4) applies to OrigPrefix only, so restrict it to index
    // 0. The two-octet setter serializes in network byte order.
    Rfc5444AddressTlv seqNum;
    seqNum.SetType(SEQ_NUM);
    seqNum.SetIndexRange(0, 0);
    seqNum.SetValue(origSeqNum);
    addressBlock.Tlvs().push_back(seqNum);

    // PATH_METRIC (Table 4) also applies to index 0 only; the AODVv2
    // MetricType travels in the TLV type extension, and a Hop Count metric
    // value occupies one octet (Table 23).
    Rfc5444AddressTlv pathMetric;
    pathMetric.SetType(PATH_METRIC);
    pathMetric.SetTypeExt(METRIC_TYPE_HOP_COUNT);
    pathMetric.SetIndexRange(0, 0);
    pathMetric.SetValue(origMetric);
    addressBlock.Tlvs().push_back(pathMetric);

    // Message header (Table 2): an RREQ uses only the type and hop limit.
    // The address family (msg-addr-length) is fixed by the class.
    Rfc5444MessageIpv4 rreq;
    rreq.SetType(RREQ);
    rreq.SetHopLimit(MAX_HOPCOUNT);
    rreq.AddressBlocks().push_back(addressBlock);

    Rfc5444Packet packet;
    packet.Messages().emplace_back(rreq);
    return packet;
}

/**
 * Print the RREQ fields of an RFC 5444 packet, walking its messages, address
 * blocks, and TLVs as an AODVv2 router would on reception.
 *
 * @param rfc5444Packet The packet to walk.
 * @param heading The heading line to print.
 */
void
PrintRreq(const Rfc5444Packet& rfc5444Packet, const std::string& heading)
{
    // A packet may carry several messages of either address family; each
    // element of Messages() is a variant over the two families.
    for (const auto& messageVariant : rfc5444Packet.Messages())
    {
        const auto* message = std::get_if<Rfc5444MessageIpv4>(&messageVariant);
        if (!message || message->GetType() != RREQ)
        {
            // A router would dispatch RREP, RERR, and other-family messages
            // here; this example handles only the IPv4 RREQ.
            continue;
        }

        std::cout << heading << std::endl;
        if (const auto hopLimit = message->GetHopLimit())
        {
            // Before forwarding, a router would decrement this and drop the
            // message when it reaches zero.
            std::cout << "  msg-hop-limit = " << static_cast<int>(*hopLimit) << std::endl;
        }

        for (const auto& block : message->AddressBlocks())
        {
            // Locate the TLVs of interest by type.
            const Rfc5444AddressTlv* addressType = nullptr;
            const Rfc5444AddressTlv* seqNum = nullptr;
            const Rfc5444AddressTlv* pathMetric = nullptr;
            for (const auto& tlv : block.Tlvs())
            {
                switch (tlv.GetType())
                {
                case ADDRESS_TYPE:
                    addressType = &tlv;
                    break;
                case SEQ_NUM:
                    seqNum = &tlv;
                    break;
                case PATH_METRIC:
                    pathMetric = &tlv;
                    break;
                default:
                    break;
                }
            }

            // Walk the addresses; GetValueForIndex() returns the value slice
            // applying to one address, or nothing if the address is outside
            // the TLV's index range.
            for (std::size_t i = 0; i < block.Addresses().size(); i++)
            {
                const auto index = static_cast<uint8_t>(i);
                std::cout << "  address " << static_cast<int>(index) << ": "
                          << block.Addresses()[i];

                if (addressType)
                {
                    if (const auto role = addressType->GetValueForIndex(index);
                        role && role->size() == 1)
                    {
                        switch (role->front())
                        {
                        case ORIGPREFIX:
                            std::cout << " (OrigPrefix)";
                            break;
                        case TARGPREFIX:
                            std::cout << " (TargPrefix)";
                            break;
                        default:
                            std::cout << " (unknown role)";
                            break;
                        }
                    }
                }
                std::cout << std::endl;

                if (seqNum)
                {
                    if (const auto value = seqNum->GetValueForIndex(index);
                        value && value->size() == 2)
                    {
                        const uint16_t sequenceNumber = ((*value)[0] << 8) | (*value)[1];
                        std::cout << "    SEQ_NUM = " << sequenceNumber << std::endl;
                    }
                }
                if (pathMetric)
                {
                    if (const auto value = pathMetric->GetValueForIndex(index);
                        value && value->size() == 1)
                    {
                        const int metricType = pathMetric->GetTypeExt().value_or(0);
                        std::cout << "    PATH_METRIC = " << static_cast<int>(value->front())
                                  << " (MetricType " << metricType << ")" << std::endl;
                    }
                }
            }
        }
    }
}

/**
 * Remove the RFC 5444 header from a received packet and print the RREQ
 * fields recovered from it.
 *
 * @param packet The received packet.
 */
void
ParseRreq(Ptr<Packet> packet)
{
    Rfc5444Packet rfc5444Packet;
    packet->RemoveHeader(rfc5444Packet);
    PrintRreq(rfc5444Packet, "RREQ received:");
}

/**
 * Print the serialized octets of a packet, sixteen per line.
 *
 * @param packet The packet to dump.
 */
void
PrintHexDump(Ptr<Packet> packet)
{
    std::vector<uint8_t> bytes(packet->GetSize());
    packet->CopyData(bytes.data(), bytes.size());
    std::cout << std::hex << std::setfill('0');
    for (std::size_t i = 0; i < bytes.size(); i++)
    {
        std::cout << std::setw(2) << static_cast<int>(bytes[i]);
        std::cout << ((i + 1) % 16 == 0 || i + 1 == bytes.size() ? "\n" : " ");
    }
    std::cout << std::dec << std::setfill(' ');
}

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    // Originating router side: build the RREQ and serialize it into a
    // Packet, as would be handed to a UDP socket bound to the RFC 5444
    // well-known port (269).
    Rfc5444Packet rreq = BuildRreq(Ipv4Address("192.0.2.1"), 5000, 3, Ipv4Address("192.0.2.99"));
    PrintRreq(rreq, "RREQ sent:");
    std::cout << std::endl;

    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(rreq);

    std::cout << "Serialized RREQ packet (" << packet->GetSize() << " octets):" << std::endl;
    PrintHexDump(packet);
    std::cout << std::endl;

    // Receiving router side.
    ParseRreq(packet);

    return 0;
}
