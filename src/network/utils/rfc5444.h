/*
 * Copyright (c) 2009 Drexel University
 *
 * SPDX-License-Identifier: GPL-2.0-only AND NIST-Software
 *
 * Authors: Tom Wambold <tom5760@gmail.com> (original ns-3 PacketBB implementation and tests)
 *          Tom Henderson <tomh@tomh.org> (rewrite assisted by Claude Fable 5)
 */

#ifndef RFC5444_H
#define RFC5444_H

#include "ipv4-address.h"
#include "ipv4-network-address.h"
#include "ipv6-address.h"
#include "ipv6-network-address.h"

#include "ns3/buffer.h"
#include "ns3/header.h"

#include <cstdint>
#include <optional>
#include <ostream>
#include <span>
#include <type_traits>
#include <variant>
#include <vector>

namespace ns3
{

/**
 * @defgroup rfc5444 RFC 5444 Generalized MANET Packet/Message Format
 * @ingroup network
 *
 * Classes implementing the Generalized Mobile Ad Hoc Network (MANET)
 * Packet/Message Format, RFC 5444: a packet contains zero or more packet
 * TLVs and zero or more messages; a message contains zero or more message
 * TLVs and zero or more address blocks; an address block contains addresses,
 * each carrying a prefix length, and zero or more address TLVs.
 *
 * These classes supersede the PacketBB (Pbb*) classes, which carry the
 * pre-publication draft name of RFC 5444 and a 2009-era API.
 *
 * Aggregates expose their contents as std::vector, and optional protocol
 * fields use std::optional. Example of building and sending an NHDP-style
 * HELLO message:
 *
 * @code
 * Rfc5444AddressBlockIpv4 neighbors;
 * neighbors.Addresses() = {Ipv4Address("10.0.0.2"), Ipv4Address("10.0.0.3")};
 *
 * Rfc5444AddressTlv linkStatus;
 * linkStatus.SetType(3);            // LINK_STATUS (RFC 6130)
 * linkStatus.SetValues(0, {{2},     // one octet per address: HEARD
 *                          {1}});   //                        SYMMETRIC
 * neighbors.Tlvs().push_back(linkStatus);
 *
 * Rfc5444MessageIpv4 hello;
 * hello.SetType(1);                 // HELLO (RFC 6130)
 * hello.SetOriginatorAddress(Ipv4Address("10.0.0.1"));
 * hello.SetHopLimit(1);
 * hello.AddressBlocks().push_back(neighbors);
 *
 * Rfc5444Packet packet;
 * packet.SetSequenceNumber(37);
 * packet.Messages().push_back(hello);
 *
 * Ptr<Packet> p = Create<Packet>();
 * p->AddHeader(packet);
 * @endcode
 *
 * On reception, messages are retrieved from the packet as a variant over the
 * two address families:
 *
 * @code
 * Rfc5444Packet packet;
 * p->RemoveHeader(packet);
 * for (const auto& messageVariant : packet.Messages())
 * {
 *     if (const auto* hello = std::get_if<Rfc5444MessageIpv4>(&messageVariant))
 *     {
 *         ...
 *     }
 * }
 * @endcode
 */

/**
 * @ingroup rfc5444
 *
 * A packet or message TLV (RFC 5444 Section 5.4.1).
 *
 * A TLV has a mandatory type, an optional type extension, and an optional
 * value. Address TLVs, which additionally carry index and multivalue
 * information, are represented by the derived class Rfc5444AddressTlv.
 */
class Rfc5444Tlv
{
  public:
    /**
     * Set the type of this TLV.
     *
     * @param type The type value to set.
     */
    void SetType(uint8_t type);

    /**
     * @return The type of this TLV.
     */
    uint8_t GetType() const;

    /**
     * Set or clear the type extension of this TLV.
     *
     * The full type of a TLV is the pair (type, type extension); see RFC 5444
     * Section 5.4.1.
     *
     * @param typeExt The type extension to set, or std::nullopt to clear it.
     */
    void SetTypeExt(std::optional<uint8_t> typeExt);

    /**
     * @return The type extension of this TLV, if present.
     */
    std::optional<uint8_t> GetTypeExt() const;

    /**
     * Set the value of this TLV.
     *
     * The octets are serialized verbatim; the encoding of any multi-octet
     * quantities within is defined by the protocol using the TLV.
     *
     * @param value The value octets.
     */
    void SetValue(std::vector<uint8_t> value);

    /**
     * Set a single-octet value on this TLV.
     *
     * @param value The value octet.
     */
    void SetValue(uint8_t value);

    /**
     * Set a two-octet value on this TLV.
     *
     * @param value The value, in host byte order; it is serialized in
     *        network byte order.
     */
    void SetValue(uint16_t value);

    /**
     * Remove the value from this TLV.
     */
    void ClearValue();

    /**
     * Get the value of this TLV.
     *
     * The returned span references storage owned by this TLV and is valid
     * until the TLV is modified or destroyed.
     *
     * @return The value octets, if a value is present.
     */
    std::optional<std::span<const uint8_t>> GetValue() const;

    /**
     * Get the value of this TLV as a single octet.
     *
     * @return The value, if a value of exactly one octet is present.
     */
    std::optional<uint8_t> GetValueAsUint8() const;

    /**
     * Get the value of this TLV as a two-octet quantity.
     *
     * @return The value, in host byte order, if a value of exactly two
     *         octets is present.
     */
    std::optional<uint16_t> GetValueAsUint16() const;

    /**
     * @return The number of bytes this TLV occupies when serialized.
     */
    uint32_t GetSerializedSize() const;

    /**
     * Serialize this TLV into the specified buffer.
     *
     * Normally invoked while serializing the enclosing Rfc5444Packet rather
     * than directly.
     *
     * @param start Buffer iterator for serialization; advanced past the
     *        octets written.
     */
    void Serialize(Buffer::Iterator& start) const;

    /**
     * Deserialize a TLV from the specified buffer.
     *
     * Normally invoked while deserializing the enclosing Rfc5444Packet rather
     * than directly.
     *
     * @param start Buffer iterator for deserialization; advanced past the
     *        octets consumed.
     * @return true on success, false if the TLV is malformed (RFC 5444
     *         Section 5.5) or the buffer is truncated.
     */
    bool Deserialize(Buffer::Iterator& start);

    /**
     * Print this TLV in human-readable form.
     *
     * @param os A stream object to print to.
     * @param level Level of indentation, in double spaces.
     */
    void Print(std::ostream& os, int level = 0) const;

    /**
     * Equality operator.
     *
     * @param other TLV to compare this one to.
     * @return true if the TLVs are equal.
     */
    bool operator==(const Rfc5444Tlv& other) const = default;

  private:
    uint8_t m_type{0};                           //!< Type
    std::optional<uint8_t> m_typeExt;            //!< Type extension
    std::optional<std::vector<uint8_t>> m_value; //!< Value
};

/**
 * @ingroup rfc5444
 *
 * An address TLV (RFC 5444 Sections 5.4.1 and 5.4.2).
 *
 * In addition to the fields of Rfc5444Tlv, an address TLV may carry an index
 * range restricting it to a subset of the addresses in its address block, and
 * may be multivalue, in which case its value carries one equal-length entry
 * per covered address. Per RFC 5444, a multivalue TLV must have an explicit
 * index range.
 */
class Rfc5444AddressTlv : public Rfc5444Tlv
{
  public:
    /**
     * Set the range of address indices this TLV applies to.
     *
     * Indices are zero-based positions in the containing address block; the
     * range is inclusive at both ends.
     *
     * @param indexStart The index of the first covered address.
     * @param indexStop The index of the last covered address; must be greater
     *        than or equal to @p indexStart.
     */
    void SetIndexRange(uint8_t indexStart, uint8_t indexStop);

    /**
     * Remove the index range from this TLV, making it apply to every address
     * in the containing address block. Also clears the multivalue flag.
     */
    void ClearIndexRange();

    /**
     * @return The index of the first covered address, if an index range is
     *         present.
     */
    std::optional<uint8_t> GetIndexStart() const;

    /**
     * @return The index of the last covered address, if an index range is
     *         present.
     */
    std::optional<uint8_t> GetIndexStop() const;

    /**
     * Whether this TLV is multivalue (RFC 5444 Section 5.4.2).
     *
     * If set, the value carries one equal-length entry for each covered
     * address; if cleared, the single value applies to each covered address
     * in its entirety. The flag is not set directly: SetValues() sets it,
     * ClearIndexRange() clears it, and deserialization takes it from the
     * wire.
     *
     * @return Whether this TLV is multivalue.
     */
    bool IsMultivalue() const;

    /**
     * Set one value per covered address (RFC 5444 Section 5.4.2).
     *
     * Sets the index range to [@p indexStart, @p indexStart + values.size() - 1],
     * sets the multivalue flag, and stores the concatenation of @p values as
     * the TLV value. All entries of @p values must have the same, nonzero
     * length.
     *
     * @param indexStart The index of the first covered address.
     * @param values One value per covered address, in address order.
     */
    void SetValues(uint8_t indexStart, const std::vector<std::vector<uint8_t>>& values);

    /**
     * Get the value applying to a single address (RFC 5444 Section 5.4.2).
     *
     * If this TLV has no index range, it covers every address and the whole
     * value is returned for any index. If an index range is present and
     * @p addressIndex falls outside it, std::nullopt is returned. Otherwise,
     * for a multivalue TLV the corresponding slice of the value is returned;
     * for a single-value TLV the whole value is returned.
     *
     * The returned span references storage owned by this TLV and is valid
     * until the TLV is modified or destroyed.
     *
     * @param addressIndex Zero-based index of the address in the containing
     *        address block.
     * @return The value octets applying to that address, if any.
     */
    std::optional<std::span<const uint8_t>> GetValueForIndex(uint8_t addressIndex) const;

    /**
     * @return The number of bytes this TLV occupies when serialized.
     */
    uint32_t GetSerializedSize() const;

    /**
     * Serialize this TLV into the specified buffer.
     *
     * Normally invoked while serializing the enclosing Rfc5444Packet rather
     * than directly.
     *
     * @param start Buffer iterator for serialization; advanced past the
     *        octets written.
     */
    void Serialize(Buffer::Iterator& start) const;

    /**
     * Deserialize an address TLV from the specified buffer.
     *
     * Normally invoked while deserializing the enclosing Rfc5444Packet rather
     * than directly.
     *
     * @param start Buffer iterator for deserialization; advanced past the
     *        octets consumed.
     * @return true on success, false if the TLV is malformed (RFC 5444
     *         Section 5.5) or the buffer is truncated.
     */
    bool Deserialize(Buffer::Iterator& start);

    /**
     * Print this TLV in human-readable form.
     *
     * @param os A stream object to print to.
     * @param level Level of indentation, in double spaces.
     */
    void Print(std::ostream& os, int level = 0) const;

    /**
     * Equality operator.
     *
     * @param other Address TLV to compare this one to.
     * @return true if the TLVs are equal.
     */
    bool operator==(const Rfc5444AddressTlv& other) const = default;

  private:
    std::optional<uint8_t> m_indexStart; //!< First covered address index
    std::optional<uint8_t> m_indexStop;  //!< Last covered address index
    bool m_multivalue{false};            //!< Multivalue flag
};

/**
 * @ingroup rfc5444
 *
 * An address block and its associated address TLVs (RFC 5444 Section 5.3).
 *
 * Each address is stored together with its prefix length as an
 * Ipv4NetworkAddress or Ipv6NetworkAddress; a plain Ipv4Address or
 * Ipv6Address converts implicitly, taking the full address length as its
 * prefix length.
 *
 * Head/tail/zero-tail address compression and single/multiple prefix-length
 * encoding are wire-format details handled transparently: serialization
 * emits no prefix-length octets when every prefix length is the full address
 * length, a single one when all are equal, and one per address otherwise;
 * deserialization presents an absent prefix length as the full address
 * length. This class always presents fully expanded addresses.
 *
 * @tparam AddrT The address family of this block: Ipv4Address or Ipv6Address.
 */
template <typename AddrT>
class Rfc5444AddressBlock
{
    static_assert(std::is_same_v<AddrT, Ipv4Address> || std::is_same_v<AddrT, Ipv6Address>,
                  "Rfc5444AddressBlock supports Ipv4Address and Ipv6Address only");

  public:
    /// The element type of Addresses(): an address paired with its prefix length.
    using NetworkAddress = std::
        conditional_t<std::is_same_v<AddrT, Ipv4Address>, Ipv4NetworkAddress, Ipv6NetworkAddress>;

    /**
     * Access the addresses in this block.
     *
     * @return A reference to the vector of addresses, each paired with its
     *         prefix length.
     */
    std::vector<NetworkAddress>& Addresses();

    /**
     * Access the addresses in this block (const version).
     *
     * @return A const reference to the vector of addresses, each paired with
     *         its prefix length.
     */
    const std::vector<NetworkAddress>& Addresses() const;

    /**
     * Access the address TLVs associated with this block.
     *
     * @return A reference to the vector of address TLVs.
     */
    std::vector<Rfc5444AddressTlv>& Tlvs();

    /**
     * Access the address TLVs associated with this block (const version).
     *
     * @return A const reference to the vector of address TLVs.
     */
    const std::vector<Rfc5444AddressTlv>& Tlvs() const;

    /**
     * @return The number of bytes this address block occupies when serialized.
     */
    uint32_t GetSerializedSize() const;

    /**
     * Serialize this address block into the specified buffer.
     *
     * Normally invoked while serializing the enclosing Rfc5444Packet rather
     * than directly.
     *
     * @param start Buffer iterator for serialization; advanced past the
     *        octets written.
     */
    void Serialize(Buffer::Iterator& start) const;

    /**
     * Deserialize an address block from the specified buffer.
     *
     * Normally invoked while deserializing the enclosing Rfc5444Packet rather
     * than directly.
     *
     * @param start Buffer iterator for deserialization; advanced past the
     *        octets consumed.
     * @return true on success, false if the address block is malformed
     *         (RFC 5444 Section 5.5) or the buffer is truncated.
     */
    bool Deserialize(Buffer::Iterator& start);

    /**
     * Print this address block in human-readable form.
     *
     * @param os A stream object to print to.
     * @param level Level of indentation, in double spaces.
     */
    void Print(std::ostream& os, int level = 0) const;

    /**
     * Equality operator.
     *
     * @param other Address block to compare this one to.
     * @return true if the address blocks are equal.
     */
    bool operator==(const Rfc5444AddressBlock& other) const = default;

  private:
    std::vector<NetworkAddress> m_addresses; //!< Addresses with prefix lengths
    std::vector<Rfc5444AddressTlv> m_tlvs;   //!< Address TLVs
};

/**
 * @ingroup rfc5444
 *
 * A message within an RFC 5444 packet (RFC 5444 Section 5.2).
 *
 * A message has a mandatory type, four optional header fields, zero or more
 * message TLVs, and zero or more address blocks. The address family is fixed
 * by the template parameter; a packet may carry messages of both families
 * side by side (see Rfc5444Packet).
 *
 * @tparam AddrT The address family of this message: Ipv4Address or
 *         Ipv6Address.
 */
template <typename AddrT>
class Rfc5444Message
{
    static_assert(std::is_same_v<AddrT, Ipv4Address> || std::is_same_v<AddrT, Ipv6Address>,
                  "Rfc5444Message supports Ipv4Address and Ipv6Address only");

  public:
    /**
     * Set the type of this message.
     *
     * @param type The type value to set.
     */
    void SetType(uint8_t type);

    /**
     * @return The type of this message.
     */
    uint8_t GetType() const;

    /**
     * Set or clear the originator address of this message.
     *
     * @param address The address of the node that created this message, or
     *        std::nullopt to clear it.
     */
    void SetOriginatorAddress(std::optional<AddrT> address);

    /**
     * @return The originator address of this message, if present.
     */
    std::optional<AddrT> GetOriginatorAddress() const;

    /**
     * Set or clear the hop limit of this message.
     *
     * @param hopLimit The maximum number of hops this message should travel,
     *        or std::nullopt to clear it.
     */
    void SetHopLimit(std::optional<uint8_t> hopLimit);

    /**
     * @return The hop limit of this message, if present.
     */
    std::optional<uint8_t> GetHopLimit() const;

    /**
     * Set or clear the hop count of this message.
     *
     * @param hopCount The number of hops this message has traveled, or
     *        std::nullopt to clear it.
     */
    void SetHopCount(std::optional<uint8_t> hopCount);

    /**
     * @return The hop count of this message, if present.
     */
    std::optional<uint8_t> GetHopCount() const;

    /**
     * Set or clear the sequence number of this message.
     *
     * @param sequenceNumber The sequence number, or std::nullopt to clear it.
     */
    void SetSequenceNumber(std::optional<uint16_t> sequenceNumber);

    /**
     * @return The sequence number of this message, if present.
     */
    std::optional<uint16_t> GetSequenceNumber() const;

    /**
     * Access the message TLVs of this message.
     *
     * @return A reference to the vector of message TLVs.
     */
    std::vector<Rfc5444Tlv>& Tlvs();

    /**
     * Access the message TLVs of this message (const version).
     *
     * @return A const reference to the vector of message TLVs.
     */
    const std::vector<Rfc5444Tlv>& Tlvs() const;

    /**
     * Access the address blocks of this message.
     *
     * @return A reference to the vector of address blocks.
     */
    std::vector<Rfc5444AddressBlock<AddrT>>& AddressBlocks();

    /**
     * Access the address blocks of this message (const version).
     *
     * @return A const reference to the vector of address blocks.
     */
    const std::vector<Rfc5444AddressBlock<AddrT>>& AddressBlocks() const;

    /**
     * @return The number of bytes this message occupies when serialized.
     */
    uint32_t GetSerializedSize() const;

    /**
     * Serialize this message into the specified buffer.
     *
     * Normally invoked while serializing the enclosing Rfc5444Packet rather
     * than directly.
     *
     * @param start Buffer iterator for serialization; advanced past the
     *        octets written.
     */
    void Serialize(Buffer::Iterator& start) const;

    /**
     * Deserialize a message from the specified buffer.
     *
     * Normally invoked while deserializing the enclosing Rfc5444Packet, which
     * selects the address family from the msg-addr-length field.
     *
     * @param start Buffer iterator for deserialization; advanced past the
     *        octets consumed.
     * @return true on success, false if the message is malformed (RFC 5444
     *         Section 5.5) or the buffer is truncated.
     */
    bool Deserialize(Buffer::Iterator& start);

    /**
     * Print this message in human-readable form.
     *
     * @param os A stream object to print to.
     * @param level Level of indentation, in double spaces.
     */
    void Print(std::ostream& os, int level = 0) const;

    /**
     * Equality operator.
     *
     * @param other Message to compare this one to.
     * @return true if the messages are equal.
     */
    bool operator==(const Rfc5444Message& other) const = default;

  private:
    uint8_t m_type{0};                                       //!< Type
    std::optional<AddrT> m_originatorAddress;                //!< Originator address
    std::optional<uint8_t> m_hopLimit;                       //!< Hop limit
    std::optional<uint8_t> m_hopCount;                       //!< Hop count
    std::optional<uint16_t> m_sequenceNumber;                //!< Sequence number
    std::vector<Rfc5444Tlv> m_tlvs;                          //!< Message TLVs
    std::vector<Rfc5444AddressBlock<AddrT>> m_addressBlocks; //!< Address blocks
};

/// An RFC 5444 address block carrying IPv4 addresses.
using Rfc5444AddressBlockIpv4 = Rfc5444AddressBlock<Ipv4Address>;
/// An RFC 5444 address block carrying IPv6 addresses.
using Rfc5444AddressBlockIpv6 = Rfc5444AddressBlock<Ipv6Address>;
/// An RFC 5444 message carrying IPv4 addresses.
using Rfc5444MessageIpv4 = Rfc5444Message<Ipv4Address>;
/// An RFC 5444 message carrying IPv6 addresses.
using Rfc5444MessageIpv6 = Rfc5444Message<Ipv6Address>;

/// A message of either address family, as carried by Rfc5444Packet.
using Rfc5444MessageVariant = std::variant<Rfc5444MessageIpv4, Rfc5444MessageIpv6>;

extern template class Rfc5444AddressBlock<Ipv4Address>;
extern template class Rfc5444AddressBlock<Ipv6Address>;
extern template class Rfc5444Message<Ipv4Address>;
extern template class Rfc5444Message<Ipv6Address>;

/**
 * @ingroup rfc5444
 *
 * An RFC 5444 packet (RFC 5444 Section 5.1).
 *
 * A packet is made up of zero or more packet TLVs and zero or more messages,
 * which may mix address families. As a Header, it is added to and removed
 * from an ns3::Packet by value:
 *
 * @code
 * packet->AddHeader(rfc5444Packet);
 * @endcode
 */
class Rfc5444Packet : public Header
{
  public:
    /**
     * @return The RFC 5444 version of this packet; always 0, the only version
     *         defined by RFC 5444 and accepted by Deserialize().
     */
    uint8_t GetVersion() const;

    /**
     * Set or clear the sequence number of this packet.
     *
     * @param sequenceNumber The sequence number, or std::nullopt to clear it.
     */
    void SetSequenceNumber(std::optional<uint16_t> sequenceNumber);

    /**
     * @return The sequence number of this packet, if present.
     */
    std::optional<uint16_t> GetSequenceNumber() const;

    /**
     * Access the packet TLVs of this packet.
     *
     * @return A reference to the vector of packet TLVs.
     */
    std::vector<Rfc5444Tlv>& Tlvs();

    /**
     * Access the packet TLVs of this packet (const version).
     *
     * @return A const reference to the vector of packet TLVs.
     */
    const std::vector<Rfc5444Tlv>& Tlvs() const;

    /**
     * Access the messages of this packet.
     *
     * @return A reference to the vector of messages; each element holds a
     *         message of either address family.
     */
    std::vector<Rfc5444MessageVariant>& Messages();

    /**
     * Access the messages of this packet (const version).
     *
     * @return A const reference to the vector of messages.
     */
    const std::vector<Rfc5444MessageVariant>& Messages() const;

    /**
     * Get the type ID.
     *
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    TypeId GetInstanceTypeId() const override;

    uint32_t GetSerializedSize() const override;

    void Serialize(Buffer::Iterator start) const override;

    /**
     * Deserialize a packet from the specified buffer.
     *
     * All length fields are validated against the remaining buffer; a
     * truncated or malformed packet (RFC 5444 Section 5.5), including one
     * with a version other than 0, is rejected as a whole.
     *
     * @param start Buffer iterator at the beginning of the packet header.
     * @return The number of bytes deserialized, or 0 if the packet is
     *         malformed.
     */
    uint32_t Deserialize(Buffer::Iterator start) override;

    void Print(std::ostream& os) const override;

    /**
     * Equality operator.
     *
     * @param other Packet to compare this one to.
     * @return true if the packets are equal.
     */
    bool operator==(const Rfc5444Packet& other) const;

  private:
    /**
     * Deserialize one message, selecting the address family from the
     * msg-addr-length field.
     *
     * @param start Buffer iterator for deserialization; advanced past the
     *        octets consumed.
     * @return The deserialized message, or std::nullopt if it is malformed.
     */
    static std::optional<Rfc5444MessageVariant> DeserializeMessage(Buffer::Iterator& start);

    std::vector<Rfc5444Tlv> m_tlvs;                //!< Packet TLVs
    std::vector<Rfc5444MessageVariant> m_messages; //!< Messages
    std::optional<uint16_t> m_sequenceNumber;      //!< Sequence number
};

} // namespace ns3

#endif /* RFC5444_H */
