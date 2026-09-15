/*
 * Copyright (c) 2009 Drexel University
 *
 * SPDX-License-Identifier: GPL-2.0-only AND NIST-Software
 *
 * Authors: Tom Wambold <tom5760@gmail.com> (original ns-3 PacketBB implementation and tests)
 *          Tom Henderson <tomh@tomh.org> (rewrite assisted by Claude Fable 5)
 */

#include "rfc5444.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/log.h"

#include <algorithm>
#include <array>
#include <string>
#include <utility>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Rfc5444");

NS_OBJECT_ENSURE_REGISTERED(Rfc5444Packet);

namespace
{

/* TLV flags (RFC 5444 Section 5.4.1) */
constexpr uint8_t THAS_TYPE_EXT = 0x80;     ///< TLV has a type extension
constexpr uint8_t THAS_SINGLE_INDEX = 0x40; ///< TLV has a single index
constexpr uint8_t THAS_MULTI_INDEX = 0x20;  ///< TLV has index-start and index-stop
constexpr uint8_t THAS_VALUE = 0x10;        ///< TLV has a value
constexpr uint8_t THAS_EXT_LEN = 0x08;      ///< TLV length field is 16 bits
constexpr uint8_t TIS_MULTIVALUE = 0x04;    ///< TLV value is multivalue

/* Address block flags (RFC 5444 Section 5.3) */
constexpr uint8_t AHAS_HEAD = 0x80;           ///< Address block has a head
constexpr uint8_t AHAS_FULL_TAIL = 0x40;      ///< Address block has a full tail
constexpr uint8_t AHAS_ZERO_TAIL = 0x20;      ///< Address block has a zero tail
constexpr uint8_t AHAS_SINGLE_PRE_LEN = 0x10; ///< Address block has a single prefix length
constexpr uint8_t AHAS_MULTI_PRE_LEN = 0x08;  ///< Address block has per-address prefix lengths

/* Packet header: version and packet flags (RFC 5444 Section 5.1) */
constexpr uint8_t VERSION = 0;         ///< RFC 5444 version number
constexpr uint8_t PHAS_SEQ_NUM = 0x08; ///< Packet has a sequence number
constexpr uint8_t PHAS_TLV = 0x04;     ///< Packet has a TLV block

/* Message flags (RFC 5444 Section 5.2) */
constexpr uint8_t MHAS_ORIG = 0x80;      ///< Message has an originator address
constexpr uint8_t MHAS_HOP_LIMIT = 0x40; ///< Message has a hop limit
constexpr uint8_t MHAS_HOP_COUNT = 0x20; ///< Message has a hop count
constexpr uint8_t MHAS_SEQ_NUM = 0x10;   ///< Message has a sequence number

/**
 * Serialized size of the \<length\> and \<value\> TLV fields.
 *
 * @param valueSize The value size in octets.
 * @return The field size in octets.
 */
uint32_t
ValueFieldSize(uint32_t valueSize)
{
    return (valueSize > 255 ? 2 : 1) + valueSize;
}

/**
 * Write the \<length\> and \<value\> TLV fields.
 *
 * @param start Buffer iterator; advanced past the octets written.
 * @param value The value octets.
 */
void
WriteValueField(Buffer::Iterator& start, std::span<const uint8_t> value)
{
    if (value.size() > 255)
    {
        start.WriteHtonU16(static_cast<uint16_t>(value.size()));
    }
    else
    {
        start.WriteU8(static_cast<uint8_t>(value.size()));
    }
    start.Write(value.data(), static_cast<uint32_t>(value.size()));
}

/**
 * Read the \<length\> and \<value\> TLV fields.
 *
 * @param start Buffer iterator; advanced past the octets consumed.
 * @param extLen Whether the length field is 16 bits (thasextlen).
 * @param value The value octets read.
 * @return false if the buffer is truncated.
 */
bool
ReadValueField(Buffer::Iterator& start, bool extLen, std::vector<uint8_t>& value)
{
    const uint32_t lengthOctets = extLen ? 2 : 1;
    if (start.GetRemainingSize() < lengthOctets)
    {
        NS_LOG_DEBUG("TLV truncated in length field");
        return false;
    }
    const uint16_t length = extLen ? start.ReadNtohU16() : start.ReadU8();
    if (start.GetRemainingSize() < length)
    {
        NS_LOG_DEBUG("TLV truncated in value field");
        return false;
    }
    value.resize(length);
    start.Read(value.data(), length);
    return true;
}

/**
 * Whether an address TLV with an index range serializes with the
 * thasmultiindex flag rather than the thassingleindex flag (the RFC 5444
 * Section 5.4.1 names for bits of the <tlv-flags> octet).
 *
 * A multivalue TLV always uses the thasmultiindex flag, because RFC 5444
 * Section 5.4.1 requires the tismultivalue flag to be cleared when the
 * thasmultiindex flag is cleared.
 *
 * @param tlv The address TLV; must have an index range.
 * @return Whether thasmultiindex is used.
 */
bool
UsesMultiIndex(const Rfc5444AddressTlv& tlv)
{
    return tlv.IsMultivalue() || tlv.GetIndexStart() != tlv.GetIndexStop();
}

/**
 * Serialized size of a TLV block: a 16-bit length followed by TLVs
 * (RFC 5444 Section 5.4).
 *
 * @tparam TlvT The TLV type of the block.
 * @param tlvs The TLVs of the block.
 * @return The block size in octets.
 */
template <typename TlvT>
uint32_t
TlvBlockSize(const std::vector<TlvT>& tlvs)
{
    uint32_t size = 2;
    for (const auto& tlv : tlvs)
    {
        size += tlv.GetSerializedSize();
    }
    return size;
}

/**
 * Write a TLV block.
 *
 * @tparam TlvT The TLV type of the block.
 * @param start Buffer iterator; advanced past the octets written.
 * @param tlvs The TLVs of the block.
 */
template <typename TlvT>
void
WriteTlvBlock(Buffer::Iterator& start, const std::vector<TlvT>& tlvs)
{
    uint32_t length = 0;
    for (const auto& tlv : tlvs)
    {
        length += tlv.GetSerializedSize();
    }
    NS_ABORT_MSG_IF(length > 65535, "TLV block exceeds the 16-bit length field");
    start.WriteHtonU16(static_cast<uint16_t>(length));
    for (const auto& tlv : tlvs)
    {
        tlv.Serialize(start);
    }
}

/**
 * Read a TLV block.
 *
 * @tparam TlvT The TLV type of the block.
 * @param start Buffer iterator; advanced past the octets consumed.
 * @param tlvs The TLVs read.
 * @return false if the block is malformed or the buffer is truncated.
 */
template <typename TlvT>
bool
ReadTlvBlock(Buffer::Iterator& start, std::vector<TlvT>& tlvs)
{
    tlvs.clear();
    if (start.GetRemainingSize() < 2)
    {
        NS_LOG_DEBUG("TLV block truncated in length field");
        return false;
    }
    const uint16_t tlvsLength = start.ReadNtohU16();
    if (start.GetRemainingSize() < tlvsLength)
    {
        NS_LOG_DEBUG("TLV block truncated");
        return false;
    }
    const uint32_t remainingAfterBlock = start.GetRemainingSize() - tlvsLength;
    while (start.GetRemainingSize() > remainingAfterBlock)
    {
        TlvT tlv;
        if (!tlv.Deserialize(start))
        {
            return false;
        }
        tlvs.push_back(std::move(tlv));
    }
    if (start.GetRemainingSize() != remainingAfterBlock)
    {
        NS_LOG_DEBUG("TLV overruns its TLV block");
        return false;
    }
    return true;
}

/// Maximum serialized address length in octets (IPv6); sizes stack buffers.
constexpr uint32_t MAX_ADDRESS_LENGTH = 16;

/**
 * Serialized address length in octets of the address family AddrT.
 *
 * @tparam AddrT The address family.
 * @return The address length in octets.
 */
template <typename AddrT>
constexpr uint32_t
AddressLength()
{
    return std::is_same_v<AddrT, Ipv4Address> ? 4 : 16;
}

/**
 * Wire layout of an address block (RFC 5444 Section 5.3), computed once and
 * shared by GetSerializedSize() and Serialize().
 */
struct AddressBlockLayout
{
    uint32_t headLength{0};         //!< Octets common to the front of all addresses
    uint32_t tailLength{0};         //!< Octets common to the back of all addresses
    bool zeroTail{false};           //!< Whether the common tail is all zero octets
    bool singlePrefixLength{false}; //!< Whether one <prefix-length> octet is emitted
    bool multiPrefixLength{false};  //!< Whether one <prefix-length> octet per address is emitted
    std::array<uint8_t, MAX_ADDRESS_LENGTH> first{}; //!< Serialized first address
};

/**
 * Compute the wire layout of an address block.
 *
 * The head (tail) is the longest common prefix (suffix) of the serialized
 * addresses, found by comparing every address against the first. Both can
 * only shrink as addresses are examined, so a later address pair cannot
 * resurrect a tail that an earlier address eliminated (the corruption behind
 * ns-3 issue #1340). A single-address block uses no compression.
 * <prefix-length> octets are emitted only when some prefix length differs
 * from the full address length.
 *
 * @tparam AddrT The address family.
 * @tparam NetworkAddressT The paired address and prefix length type.
 * @param addresses The addresses of the block; must not be empty.
 * @return The layout.
 */
template <typename AddrT, typename NetworkAddressT>
AddressBlockLayout
ComputeAddressBlockLayout(const std::vector<NetworkAddressT>& addresses)
{
    constexpr uint32_t addressLength = AddressLength<AddrT>();
    AddressBlockLayout layout;
    addresses.front().GetAddress().Serialize(layout.first.data());

    if (addresses.size() > 1)
    {
        uint32_t head = addressLength;
        uint32_t tail = addressLength;
        std::array<uint8_t, MAX_ADDRESS_LENGTH> current;
        for (std::size_t i = 1; i < addresses.size(); i++)
        {
            addresses[i].GetAddress().Serialize(current.data());
            uint32_t common = 0;
            while (common < head && current[common] == layout.first[common])
            {
                common++;
            }
            head = common;
            common = 0;
            while (common < tail &&
                   current[addressLength - 1 - common] == layout.first[addressLength - 1 - common])
            {
                common++;
            }
            tail = common;
        }
        layout.headLength = head;
        layout.tailLength = std::min(tail, addressLength - head);
        layout.zeroTail = layout.tailLength > 0 &&
                          std::all_of(layout.first.begin() + addressLength - layout.tailLength,
                                      layout.first.begin() + addressLength,
                                      [](uint8_t octet) { return octet == 0; });
    }

    constexpr uint8_t fullLength = 8 * addressLength;
    const bool allFull = std::all_of(addresses.begin(), addresses.end(), [](const auto& address) {
        return address.GetNetworkLength() == fullLength;
    });
    if (!allFull)
    {
        const uint8_t firstLength = addresses.front().GetNetworkLength();
        const bool allEqual =
            std::all_of(addresses.begin(), addresses.end(), [firstLength](const auto& address) {
                return address.GetNetworkLength() == firstLength;
            });
        layout.singlePrefixLength = allEqual;
        layout.multiPrefixLength = !allEqual;
    }
    return layout;
}

} // namespace

/* Rfc5444Tlv */

void
Rfc5444Tlv::SetType(uint8_t type)
{
    m_type = type;
}

uint8_t
Rfc5444Tlv::GetType() const
{
    return m_type;
}

void
Rfc5444Tlv::SetTypeExt(std::optional<uint8_t> typeExt)
{
    m_typeExt = typeExt;
}

std::optional<uint8_t>
Rfc5444Tlv::GetTypeExt() const
{
    return m_typeExt;
}

void
Rfc5444Tlv::SetValue(std::vector<uint8_t> value)
{
    NS_ABORT_MSG_IF(value.size() > 65535, "TLV value exceeds the 16-bit length field");
    m_value = std::move(value);
}

void
Rfc5444Tlv::SetValue(uint8_t value)
{
    m_value = std::vector<uint8_t>{value};
}

void
Rfc5444Tlv::SetValue(uint16_t value)
{
    m_value =
        std::vector<uint8_t>{static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value & 0xff)};
}

void
Rfc5444Tlv::ClearValue()
{
    m_value.reset();
}

std::optional<std::span<const uint8_t>>
Rfc5444Tlv::GetValue() const
{
    if (!m_value)
    {
        return std::nullopt;
    }
    return std::span<const uint8_t>(*m_value);
}

std::optional<uint8_t>
Rfc5444Tlv::GetValueAsUint8() const
{
    if (!m_value || m_value->size() != 1)
    {
        return std::nullopt;
    }
    return m_value->front();
}

std::optional<uint16_t>
Rfc5444Tlv::GetValueAsUint16() const
{
    if (!m_value || m_value->size() != 2)
    {
        return std::nullopt;
    }
    return static_cast<uint16_t>(((*m_value)[0] << 8) | (*m_value)[1]);
}

uint32_t
Rfc5444Tlv::GetSerializedSize() const
{
    uint32_t size = 2; // type + flags
    if (m_typeExt)
    {
        size++;
    }
    if (m_value)
    {
        size += ValueFieldSize(m_value->size());
    }
    return size;
}

void
Rfc5444Tlv::Serialize(Buffer::Iterator& start) const
{
    start.WriteU8(m_type);

    uint8_t flags = 0;
    if (m_typeExt)
    {
        flags |= THAS_TYPE_EXT;
    }
    if (m_value)
    {
        flags |= THAS_VALUE;
        if (m_value->size() > 255)
        {
            flags |= THAS_EXT_LEN;
        }
    }
    start.WriteU8(flags);

    if (m_typeExt)
    {
        start.WriteU8(*m_typeExt);
    }
    if (m_value)
    {
        WriteValueField(start, *m_value);
    }
}

bool
Rfc5444Tlv::Deserialize(Buffer::Iterator& start)
{
    if (start.GetRemainingSize() < 2)
    {
        NS_LOG_DEBUG("TLV truncated before flags");
        return false;
    }
    m_type = start.ReadU8();
    const uint8_t flags = start.ReadU8();

    // RFC 5444 Section 5.4.1: index and multivalue flags MUST be cleared in
    // packet and message TLVs.
    if (flags & (THAS_SINGLE_INDEX | THAS_MULTI_INDEX | TIS_MULTIVALUE))
    {
        NS_LOG_DEBUG("index or multivalue flag in a packet or message TLV");
        return false;
    }
    if ((flags & THAS_EXT_LEN) && !(flags & THAS_VALUE))
    {
        NS_LOG_DEBUG("thasextlen without thasvalue");
        return false;
    }

    m_typeExt.reset();
    if (flags & THAS_TYPE_EXT)
    {
        if (start.GetRemainingSize() < 1)
        {
            NS_LOG_DEBUG("TLV truncated in type extension");
            return false;
        }
        m_typeExt = start.ReadU8();
    }

    m_value.reset();
    if (flags & THAS_VALUE)
    {
        std::vector<uint8_t> value;
        if (!ReadValueField(start, flags & THAS_EXT_LEN, value))
        {
            return false;
        }
        m_value = std::move(value);
    }
    return true;
}

void
Rfc5444Tlv::Print(std::ostream& os, int level) const
{
    const std::string prefix(level * 2, ' ');
    os << prefix << "Rfc5444Tlv {" << std::endl;
    os << prefix << "  type = " << static_cast<int>(m_type) << std::endl;
    if (m_typeExt)
    {
        os << prefix << "  typeExt = " << static_cast<int>(*m_typeExt) << std::endl;
    }
    if (m_value)
    {
        os << prefix << "  value size = " << m_value->size() << std::endl;
    }
    os << prefix << "}" << std::endl;
}

/* Rfc5444AddressTlv */

void
Rfc5444AddressTlv::SetIndexRange(uint8_t indexStart, uint8_t indexStop)
{
    NS_ASSERT_MSG(indexStop >= indexStart, "indexStop must be >= indexStart");
    m_indexStart = indexStart;
    m_indexStop = indexStop;
}

void
Rfc5444AddressTlv::ClearIndexRange()
{
    m_indexStart.reset();
    m_indexStop.reset();
    m_multivalue = false;
}

std::optional<uint8_t>
Rfc5444AddressTlv::GetIndexStart() const
{
    return m_indexStart;
}

std::optional<uint8_t>
Rfc5444AddressTlv::GetIndexStop() const
{
    return m_indexStop;
}

bool
Rfc5444AddressTlv::IsMultivalue() const
{
    return m_multivalue;
}

void
Rfc5444AddressTlv::SetValues(uint8_t indexStart, const std::vector<std::vector<uint8_t>>& values)
{
    NS_ASSERT_MSG(!values.empty(), "SetValues() requires at least one value");
    NS_ABORT_MSG_IF(indexStart + values.size() - 1 > 255, "index range exceeds 8-bit index field");
    const std::size_t singleLength = values.front().size();
    NS_ASSERT_MSG(singleLength > 0, "SetValues() entries must have nonzero length");

    std::vector<uint8_t> concatenation;
    concatenation.reserve(singleLength * values.size());
    for (const auto& value : values)
    {
        NS_ASSERT_MSG(value.size() == singleLength, "SetValues() entries must have equal length");
        concatenation.insert(concatenation.end(), value.begin(), value.end());
    }

    SetIndexRange(indexStart, static_cast<uint8_t>(indexStart + values.size() - 1));
    m_multivalue = true;
    SetValue(std::move(concatenation));
}

std::optional<std::span<const uint8_t>>
Rfc5444AddressTlv::GetValueForIndex(uint8_t addressIndex) const
{
    const auto value = GetValue();
    if (!value)
    {
        return std::nullopt;
    }
    if (!m_indexStart)
    {
        return value;
    }
    if (addressIndex < *m_indexStart || addressIndex > *m_indexStop)
    {
        return std::nullopt;
    }
    if (!m_multivalue)
    {
        return value;
    }
    const uint32_t numberValues = *m_indexStop - *m_indexStart + 1;
    const std::size_t singleLength = value->size() / numberValues;
    return value->subspan((addressIndex - *m_indexStart) * singleLength, singleLength);
}

uint32_t
Rfc5444AddressTlv::GetSerializedSize() const
{
    uint32_t size = Rfc5444Tlv::GetSerializedSize();
    if (m_indexStart)
    {
        size += UsesMultiIndex(*this) ? 2 : 1;
    }
    return size;
}

void
Rfc5444AddressTlv::Serialize(Buffer::Iterator& start) const
{
    start.WriteU8(GetType());

    const auto typeExt = GetTypeExt();
    const auto value = GetValue();

    uint8_t flags = 0;
    if (typeExt)
    {
        flags |= THAS_TYPE_EXT;
    }
    if (m_indexStart)
    {
        flags |= UsesMultiIndex(*this) ? THAS_MULTI_INDEX : THAS_SINGLE_INDEX;
    }
    if (value)
    {
        flags |= THAS_VALUE;
        if (value->size() > 255)
        {
            flags |= THAS_EXT_LEN;
        }
        if (m_multivalue)
        {
            flags |= TIS_MULTIVALUE;
        }
    }
    start.WriteU8(flags);

    if (typeExt)
    {
        start.WriteU8(*typeExt);
    }
    if (m_indexStart)
    {
        start.WriteU8(*m_indexStart);
        if (UsesMultiIndex(*this))
        {
            start.WriteU8(*m_indexStop);
        }
    }
    if (value)
    {
        WriteValueField(start, *value);
    }
}

bool
Rfc5444AddressTlv::Deserialize(Buffer::Iterator& start)
{
    if (start.GetRemainingSize() < 2)
    {
        NS_LOG_DEBUG("address TLV truncated before flags");
        return false;
    }
    SetType(start.ReadU8());
    const uint8_t flags = start.ReadU8();

    // Flag combinations RFC 5444 Section 5.4.1 says MUST NOT be used.
    if ((flags & THAS_SINGLE_INDEX) && (flags & THAS_MULTI_INDEX))
    {
        NS_LOG_DEBUG("thassingleindex and thasmultiindex both set");
        return false;
    }
    if ((flags & THAS_EXT_LEN) && !(flags & THAS_VALUE))
    {
        NS_LOG_DEBUG("thasextlen without thasvalue");
        return false;
    }
    if ((flags & TIS_MULTIVALUE) && (!(flags & THAS_MULTI_INDEX) || !(flags & THAS_VALUE)))
    {
        NS_LOG_DEBUG("tismultivalue without thasmultiindex or thasvalue");
        return false;
    }

    SetTypeExt(std::nullopt);
    if (flags & THAS_TYPE_EXT)
    {
        if (start.GetRemainingSize() < 1)
        {
            NS_LOG_DEBUG("address TLV truncated in type extension");
            return false;
        }
        SetTypeExt(start.ReadU8());
    }

    m_indexStart.reset();
    m_indexStop.reset();
    if (flags & THAS_SINGLE_INDEX)
    {
        if (start.GetRemainingSize() < 1)
        {
            NS_LOG_DEBUG("address TLV truncated in index field");
            return false;
        }
        const uint8_t index = start.ReadU8();
        m_indexStart = index;
        m_indexStop = index;
    }
    else if (flags & THAS_MULTI_INDEX)
    {
        if (start.GetRemainingSize() < 2)
        {
            NS_LOG_DEBUG("address TLV truncated in index fields");
            return false;
        }
        m_indexStart = start.ReadU8();
        m_indexStop = start.ReadU8();
        if (*m_indexStop < *m_indexStart)
        {
            NS_LOG_DEBUG("index-stop less than index-start");
            return false;
        }
    }

    m_multivalue = (flags & TIS_MULTIVALUE) != 0;
    ClearValue();
    if (flags & THAS_VALUE)
    {
        std::vector<uint8_t> value;
        if (!ReadValueField(start, flags & THAS_EXT_LEN, value))
        {
            return false;
        }
        if (m_multivalue)
        {
            // RFC 5444 Section 5.4.1: <length> MUST be an integral multiple
            // of number-values.
            const uint32_t numberValues = *m_indexStop - *m_indexStart + 1;
            if (value.size() % numberValues != 0)
            {
                NS_LOG_DEBUG("multivalue length not a multiple of number-values");
                return false;
            }
        }
        SetValue(std::move(value));
    }
    return true;
}

void
Rfc5444AddressTlv::Print(std::ostream& os, int level) const
{
    const std::string prefix(level * 2, ' ');
    os << prefix << "Rfc5444AddressTlv {" << std::endl;
    os << prefix << "  type = " << static_cast<int>(GetType()) << std::endl;
    if (const auto typeExt = GetTypeExt())
    {
        os << prefix << "  typeExt = " << static_cast<int>(*typeExt) << std::endl;
    }
    if (m_indexStart)
    {
        os << prefix << "  indexStart = " << static_cast<int>(*m_indexStart) << std::endl;
        os << prefix << "  indexStop = " << static_cast<int>(*m_indexStop) << std::endl;
    }
    os << prefix << "  isMultivalue = " << m_multivalue << std::endl;
    if (const auto value = GetValue())
    {
        os << prefix << "  value size = " << value->size() << std::endl;
    }
    os << prefix << "}" << std::endl;
}

/* Rfc5444AddressBlock */

template <typename AddrT>
auto
Rfc5444AddressBlock<AddrT>::Addresses() -> std::vector<NetworkAddress>&
{
    return m_addresses;
}

template <typename AddrT>
auto
Rfc5444AddressBlock<AddrT>::Addresses() const -> const std::vector<NetworkAddress>&
{
    return m_addresses;
}

template <typename AddrT>
std::vector<Rfc5444AddressTlv>&
Rfc5444AddressBlock<AddrT>::Tlvs()
{
    return m_tlvs;
}

template <typename AddrT>
const std::vector<Rfc5444AddressTlv>&
Rfc5444AddressBlock<AddrT>::Tlvs() const
{
    return m_tlvs;
}

template <typename AddrT>
uint32_t
Rfc5444AddressBlock<AddrT>::GetSerializedSize() const
{
    NS_ABORT_MSG_IF(m_addresses.empty(),
                    "an address block must contain at least one address (RFC 5444 Section 5.3)");
    constexpr uint32_t addressLength = AddressLength<AddrT>();
    const auto layout = ComputeAddressBlockLayout<AddrT>(m_addresses);

    uint32_t size = 2; // num-addr + addr-flags
    if (layout.headLength > 0)
    {
        size += 1 + layout.headLength;
    }
    if (layout.tailLength > 0)
    {
        size += 1 + (layout.zeroTail ? 0 : layout.tailLength);
    }
    size += (addressLength - layout.headLength - layout.tailLength) *
            static_cast<uint32_t>(m_addresses.size());
    if (layout.singlePrefixLength)
    {
        size += 1;
    }
    else if (layout.multiPrefixLength)
    {
        size += static_cast<uint32_t>(m_addresses.size());
    }
    return size + TlvBlockSize(m_tlvs);
}

template <typename AddrT>
void
Rfc5444AddressBlock<AddrT>::Serialize(Buffer::Iterator& start) const
{
    NS_ABORT_MSG_IF(m_addresses.empty(),
                    "an address block must contain at least one address (RFC 5444 Section 5.3)");
    NS_ABORT_MSG_IF(m_addresses.size() > 255,
                    "an address block cannot hold more than 255 addresses");
    constexpr uint32_t addressLength = AddressLength<AddrT>();
    const auto layout = ComputeAddressBlockLayout<AddrT>(m_addresses);

    start.WriteU8(static_cast<uint8_t>(m_addresses.size()));

    uint8_t flags = 0;
    if (layout.headLength > 0)
    {
        flags |= AHAS_HEAD;
    }
    if (layout.tailLength > 0)
    {
        flags |= layout.zeroTail ? AHAS_ZERO_TAIL : AHAS_FULL_TAIL;
    }
    if (layout.singlePrefixLength)
    {
        flags |= AHAS_SINGLE_PRE_LEN;
    }
    else if (layout.multiPrefixLength)
    {
        flags |= AHAS_MULTI_PRE_LEN;
    }
    start.WriteU8(flags);

    if (layout.headLength > 0)
    {
        start.WriteU8(static_cast<uint8_t>(layout.headLength));
        start.Write(layout.first.data(), layout.headLength);
    }
    if (layout.tailLength > 0)
    {
        start.WriteU8(static_cast<uint8_t>(layout.tailLength));
        if (!layout.zeroTail)
        {
            start.Write(layout.first.data() + addressLength - layout.tailLength, layout.tailLength);
        }
    }

    const uint32_t midLength = addressLength - layout.headLength - layout.tailLength;
    if (midLength > 0)
    {
        std::array<uint8_t, MAX_ADDRESS_LENGTH> octets;
        for (const auto& networkAddress : m_addresses)
        {
            networkAddress.GetAddress().Serialize(octets.data());
            start.Write(octets.data() + layout.headLength, midLength);
        }
    }

    if (layout.singlePrefixLength)
    {
        start.WriteU8(m_addresses.front().GetNetworkLength());
    }
    else if (layout.multiPrefixLength)
    {
        for (const auto& networkAddress : m_addresses)
        {
            start.WriteU8(networkAddress.GetNetworkLength());
        }
    }

    WriteTlvBlock(start, m_tlvs);
}

template <typename AddrT>
bool
Rfc5444AddressBlock<AddrT>::Deserialize(Buffer::Iterator& start)
{
    constexpr uint32_t addressLength = AddressLength<AddrT>();

    m_addresses.clear();
    m_tlvs.clear();

    if (start.GetRemainingSize() < 2)
    {
        NS_LOG_DEBUG("address block truncated before flags");
        return false;
    }
    const uint8_t numAddr = start.ReadU8();
    const uint8_t flags = start.ReadU8();

    // Combinations RFC 5444 Section 5.3 says MUST NOT be used.
    if (numAddr == 0)
    {
        NS_LOG_DEBUG("num-addr is zero");
        return false;
    }
    if ((flags & AHAS_FULL_TAIL) && (flags & AHAS_ZERO_TAIL))
    {
        NS_LOG_DEBUG("ahasfulltail and ahaszerotail both set");
        return false;
    }
    if ((flags & AHAS_SINGLE_PRE_LEN) && (flags & AHAS_MULTI_PRE_LEN))
    {
        NS_LOG_DEBUG("ahassingleprelen and ahasmultiprelen both set");
        return false;
    }

    std::array<uint8_t, MAX_ADDRESS_LENGTH> octets{};
    uint32_t headLength = 0;
    uint32_t tailLength = 0;

    if (flags & AHAS_HEAD)
    {
        if (start.GetRemainingSize() < 1)
        {
            NS_LOG_DEBUG("address block truncated in head length");
            return false;
        }
        headLength = start.ReadU8();
        if (headLength > addressLength)
        {
            NS_LOG_DEBUG("head-length exceeds the address length");
            return false;
        }
        if (start.GetRemainingSize() < headLength)
        {
            NS_LOG_DEBUG("address block truncated in head");
            return false;
        }
        start.Read(octets.data(), headLength);
    }

    if (flags & (AHAS_FULL_TAIL | AHAS_ZERO_TAIL))
    {
        if (start.GetRemainingSize() < 1)
        {
            NS_LOG_DEBUG("address block truncated in tail length");
            return false;
        }
        tailLength = start.ReadU8();
        if (headLength + tailLength > addressLength)
        {
            NS_LOG_DEBUG("head-length plus tail-length exceeds the address length");
            return false;
        }
        if (flags & AHAS_FULL_TAIL)
        {
            if (start.GetRemainingSize() < tailLength)
            {
                NS_LOG_DEBUG("address block truncated in tail");
                return false;
            }
            start.Read(octets.data() + addressLength - tailLength, tailLength);
        }
        // A zero tail has no octets on the wire; octets was zero-initialized.
    }

    const uint32_t midLength = addressLength - headLength - tailLength;
    if (start.GetRemainingSize() < numAddr * midLength)
    {
        NS_LOG_DEBUG("address block truncated in mid fields");
        return false;
    }
    std::vector<AddrT> addresses;
    addresses.reserve(numAddr);
    for (uint32_t i = 0; i < numAddr; i++)
    {
        if (midLength > 0)
        {
            start.Read(octets.data() + headLength, midLength);
        }
        addresses.push_back(AddrT::Deserialize(octets.data()));
    }

    constexpr uint8_t fullLength = 8 * addressLength;
    std::vector<uint8_t> prefixLengths;
    if (flags & AHAS_SINGLE_PRE_LEN)
    {
        if (start.GetRemainingSize() < 1)
        {
            NS_LOG_DEBUG("address block truncated in prefix length");
            return false;
        }
        const uint8_t prefixLength = start.ReadU8();
        if (prefixLength > fullLength)
        {
            NS_LOG_DEBUG("prefix-length exceeds the address length in bits");
            return false;
        }
        prefixLengths.assign(numAddr, prefixLength);
    }
    else if (flags & AHAS_MULTI_PRE_LEN)
    {
        if (start.GetRemainingSize() < numAddr)
        {
            NS_LOG_DEBUG("address block truncated in prefix lengths");
            return false;
        }
        prefixLengths.reserve(numAddr);
        for (uint32_t i = 0; i < numAddr; i++)
        {
            const uint8_t prefixLength = start.ReadU8();
            if (prefixLength > fullLength)
            {
                NS_LOG_DEBUG("prefix-length exceeds the address length in bits");
                return false;
            }
            prefixLengths.push_back(prefixLength);
        }
    }
    else
    {
        prefixLengths.assign(numAddr, fullLength);
    }

    m_addresses.reserve(numAddr);
    for (uint32_t i = 0; i < numAddr; i++)
    {
        m_addresses.emplace_back(addresses[i], prefixLengths[i]);
    }

    return ReadTlvBlock(start, m_tlvs);
}

template <typename AddrT>
void
Rfc5444AddressBlock<AddrT>::Print(std::ostream& os, int level) const
{
    const std::string prefix(level * 2, ' ');
    os << prefix << "Rfc5444AddressBlock {" << std::endl;
    for (const auto& networkAddress : m_addresses)
    {
        os << prefix << "  " << networkAddress << std::endl;
    }
    for (const auto& tlv : m_tlvs)
    {
        tlv.Print(os, level + 1);
    }
    os << prefix << "}" << std::endl;
}

/* Rfc5444Message */

template <typename AddrT>
void
Rfc5444Message<AddrT>::SetType(uint8_t type)
{
    m_type = type;
}

template <typename AddrT>
uint8_t
Rfc5444Message<AddrT>::GetType() const
{
    return m_type;
}

template <typename AddrT>
void
Rfc5444Message<AddrT>::SetOriginatorAddress(std::optional<AddrT> address)
{
    m_originatorAddress = address;
}

template <typename AddrT>
std::optional<AddrT>
Rfc5444Message<AddrT>::GetOriginatorAddress() const
{
    return m_originatorAddress;
}

template <typename AddrT>
void
Rfc5444Message<AddrT>::SetHopLimit(std::optional<uint8_t> hopLimit)
{
    m_hopLimit = hopLimit;
}

template <typename AddrT>
std::optional<uint8_t>
Rfc5444Message<AddrT>::GetHopLimit() const
{
    return m_hopLimit;
}

template <typename AddrT>
void
Rfc5444Message<AddrT>::SetHopCount(std::optional<uint8_t> hopCount)
{
    m_hopCount = hopCount;
}

template <typename AddrT>
std::optional<uint8_t>
Rfc5444Message<AddrT>::GetHopCount() const
{
    return m_hopCount;
}

template <typename AddrT>
void
Rfc5444Message<AddrT>::SetSequenceNumber(std::optional<uint16_t> sequenceNumber)
{
    m_sequenceNumber = sequenceNumber;
}

template <typename AddrT>
std::optional<uint16_t>
Rfc5444Message<AddrT>::GetSequenceNumber() const
{
    return m_sequenceNumber;
}

template <typename AddrT>
std::vector<Rfc5444Tlv>&
Rfc5444Message<AddrT>::Tlvs()
{
    return m_tlvs;
}

template <typename AddrT>
const std::vector<Rfc5444Tlv>&
Rfc5444Message<AddrT>::Tlvs() const
{
    return m_tlvs;
}

template <typename AddrT>
std::vector<Rfc5444AddressBlock<AddrT>>&
Rfc5444Message<AddrT>::AddressBlocks()
{
    return m_addressBlocks;
}

template <typename AddrT>
const std::vector<Rfc5444AddressBlock<AddrT>>&
Rfc5444Message<AddrT>::AddressBlocks() const
{
    return m_addressBlocks;
}

template <typename AddrT>
uint32_t
Rfc5444Message<AddrT>::GetSerializedSize() const
{
    uint32_t size = 4; // msg-type + msg-flags/msg-addr-length + msg-size
    if (m_originatorAddress)
    {
        size += AddressLength<AddrT>();
    }
    if (m_hopLimit)
    {
        size++;
    }
    if (m_hopCount)
    {
        size++;
    }
    if (m_sequenceNumber)
    {
        size += 2;
    }
    size += TlvBlockSize(m_tlvs);
    for (const auto& block : m_addressBlocks)
    {
        size += block.GetSerializedSize();
    }
    return size;
}

template <typename AddrT>
void
Rfc5444Message<AddrT>::Serialize(Buffer::Iterator& start) const
{
    const uint32_t size = GetSerializedSize();
    NS_ABORT_MSG_IF(size > 65535, "message exceeds the 16-bit msg-size field");

    start.WriteU8(m_type);

    // msg-flags in the high nibble, msg-addr-length (address length minus
    // one) in the low nibble.
    auto flags = static_cast<uint8_t>(AddressLength<AddrT>() - 1);
    if (m_originatorAddress)
    {
        flags |= MHAS_ORIG;
    }
    if (m_hopLimit)
    {
        flags |= MHAS_HOP_LIMIT;
    }
    if (m_hopCount)
    {
        flags |= MHAS_HOP_COUNT;
    }
    if (m_sequenceNumber)
    {
        flags |= MHAS_SEQ_NUM;
    }
    start.WriteU8(flags);
    start.WriteHtonU16(static_cast<uint16_t>(size));

    if (m_originatorAddress)
    {
        std::array<uint8_t, MAX_ADDRESS_LENGTH> octets;
        m_originatorAddress->Serialize(octets.data());
        start.Write(octets.data(), AddressLength<AddrT>());
    }
    if (m_hopLimit)
    {
        start.WriteU8(*m_hopLimit);
    }
    if (m_hopCount)
    {
        start.WriteU8(*m_hopCount);
    }
    if (m_sequenceNumber)
    {
        start.WriteHtonU16(*m_sequenceNumber);
    }

    WriteTlvBlock(start, m_tlvs);
    for (const auto& block : m_addressBlocks)
    {
        block.Serialize(start);
    }
}

template <typename AddrT>
bool
Rfc5444Message<AddrT>::Deserialize(Buffer::Iterator& start)
{
    constexpr uint32_t addressLength = AddressLength<AddrT>();

    m_originatorAddress.reset();
    m_hopLimit.reset();
    m_hopCount.reset();
    m_sequenceNumber.reset();
    m_tlvs.clear();
    m_addressBlocks.clear();

    const uint32_t remainingAtStart = start.GetRemainingSize();
    if (remainingAtStart < 4)
    {
        NS_LOG_DEBUG("message truncated in header");
        return false;
    }
    m_type = start.ReadU8();
    const uint8_t flags = start.ReadU8();
    if ((flags & 0x0f) != addressLength - 1)
    {
        NS_LOG_DEBUG("msg-addr-length does not match the address family");
        return false;
    }
    const uint16_t messageSize = start.ReadNtohU16();
    if (messageSize < 4 || messageSize > remainingAtStart)
    {
        NS_LOG_DEBUG("msg-size smaller than the message header or larger than the buffer");
        return false;
    }
    const uint32_t remainingAfterMessage = remainingAtStart - messageSize;
    const auto remainingInMessage = [&start, remainingAfterMessage]() {
        return start.GetRemainingSize() - remainingAfterMessage;
    };

    if (flags & MHAS_ORIG)
    {
        if (remainingInMessage() < addressLength)
        {
            NS_LOG_DEBUG("message truncated in originator address");
            return false;
        }
        std::array<uint8_t, MAX_ADDRESS_LENGTH> octets;
        start.Read(octets.data(), addressLength);
        m_originatorAddress = AddrT::Deserialize(octets.data());
    }
    if (flags & MHAS_HOP_LIMIT)
    {
        if (remainingInMessage() < 1)
        {
            NS_LOG_DEBUG("message truncated in hop limit");
            return false;
        }
        m_hopLimit = start.ReadU8();
    }
    if (flags & MHAS_HOP_COUNT)
    {
        if (remainingInMessage() < 1)
        {
            NS_LOG_DEBUG("message truncated in hop count");
            return false;
        }
        m_hopCount = start.ReadU8();
    }
    if (flags & MHAS_SEQ_NUM)
    {
        if (remainingInMessage() < 2)
        {
            NS_LOG_DEBUG("message truncated in sequence number");
            return false;
        }
        m_sequenceNumber = start.ReadNtohU16();
    }

    if (!ReadTlvBlock(start, m_tlvs))
    {
        return false;
    }
    if (start.GetRemainingSize() < remainingAfterMessage)
    {
        NS_LOG_DEBUG("message TLV block overruns msg-size");
        return false;
    }

    while (start.GetRemainingSize() > remainingAfterMessage)
    {
        Rfc5444AddressBlock<AddrT> block;
        if (!block.Deserialize(start))
        {
            return false;
        }
        if (start.GetRemainingSize() < remainingAfterMessage)
        {
            NS_LOG_DEBUG("address block overruns msg-size");
            return false;
        }
        m_addressBlocks.push_back(std::move(block));
    }
    return true;
}

template <typename AddrT>
void
Rfc5444Message<AddrT>::Print(std::ostream& os, int level) const
{
    const std::string prefix(level * 2, ' ');
    os << prefix << "Rfc5444Message {" << std::endl;
    os << prefix << "  type = " << static_cast<int>(m_type) << std::endl;
    if (m_originatorAddress)
    {
        os << prefix << "  originatorAddress = " << *m_originatorAddress << std::endl;
    }
    if (m_hopLimit)
    {
        os << prefix << "  hopLimit = " << static_cast<int>(*m_hopLimit) << std::endl;
    }
    if (m_hopCount)
    {
        os << prefix << "  hopCount = " << static_cast<int>(*m_hopCount) << std::endl;
    }
    if (m_sequenceNumber)
    {
        os << prefix << "  sequenceNumber = " << *m_sequenceNumber << std::endl;
    }
    for (const auto& tlv : m_tlvs)
    {
        tlv.Print(os, level + 1);
    }
    for (const auto& block : m_addressBlocks)
    {
        block.Print(os, level + 1);
    }
    os << prefix << "}" << std::endl;
}

/* Rfc5444Packet */

uint8_t
Rfc5444Packet::GetVersion() const
{
    return VERSION;
}

void
Rfc5444Packet::SetSequenceNumber(std::optional<uint16_t> sequenceNumber)
{
    m_sequenceNumber = sequenceNumber;
}

std::optional<uint16_t>
Rfc5444Packet::GetSequenceNumber() const
{
    return m_sequenceNumber;
}

std::vector<Rfc5444Tlv>&
Rfc5444Packet::Tlvs()
{
    return m_tlvs;
}

const std::vector<Rfc5444Tlv>&
Rfc5444Packet::Tlvs() const
{
    return m_tlvs;
}

std::vector<Rfc5444MessageVariant>&
Rfc5444Packet::Messages()
{
    return m_messages;
}

const std::vector<Rfc5444MessageVariant>&
Rfc5444Packet::Messages() const
{
    return m_messages;
}

TypeId
Rfc5444Packet::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Rfc5444Packet")
                            .SetParent<Header>()
                            .SetGroupName("Network")
                            .AddConstructor<Rfc5444Packet>();
    return tid;
}

TypeId
Rfc5444Packet::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
Rfc5444Packet::GetSerializedSize() const
{
    uint32_t size = 1; // version + pkt-flags
    if (m_sequenceNumber)
    {
        size += 2;
    }
    if (!m_tlvs.empty())
    {
        size += TlvBlockSize(m_tlvs);
    }
    for (const auto& message : m_messages)
    {
        size += std::visit([](const auto& m) { return m.GetSerializedSize(); }, message);
    }
    return size;
}

void
Rfc5444Packet::Serialize(Buffer::Iterator start) const
{
    // Version 0 in the high nibble; a TLV block is emitted only when packet
    // TLVs are present.
    uint8_t versionAndFlags = VERSION << 4;
    if (m_sequenceNumber)
    {
        versionAndFlags |= PHAS_SEQ_NUM;
    }
    if (!m_tlvs.empty())
    {
        versionAndFlags |= PHAS_TLV;
    }
    start.WriteU8(versionAndFlags);

    if (m_sequenceNumber)
    {
        start.WriteHtonU16(*m_sequenceNumber);
    }
    if (!m_tlvs.empty())
    {
        WriteTlvBlock(start, m_tlvs);
    }
    for (const auto& message : m_messages)
    {
        std::visit([&start](const auto& m) { m.Serialize(start); }, message);
    }
}

uint32_t
Rfc5444Packet::Deserialize(Buffer::Iterator start)
{
    m_sequenceNumber.reset();
    m_tlvs.clear();
    m_messages.clear();

    const uint32_t initialRemaining = start.GetRemainingSize();
    if (initialRemaining < 1)
    {
        NS_LOG_DEBUG("packet truncated before header");
        return 0;
    }
    const uint8_t versionAndFlags = start.ReadU8();
    if ((versionAndFlags >> 4) != VERSION)
    {
        NS_LOG_DEBUG("version is not 0");
        return 0;
    }
    if (versionAndFlags & PHAS_SEQ_NUM)
    {
        if (start.GetRemainingSize() < 2)
        {
            NS_LOG_DEBUG("packet truncated in sequence number");
            return 0;
        }
        m_sequenceNumber = start.ReadNtohU16();
    }
    if (versionAndFlags & PHAS_TLV)
    {
        if (!ReadTlvBlock(start, m_tlvs))
        {
            return 0;
        }
    }
    while (start.GetRemainingSize() > 0)
    {
        auto message = DeserializeMessage(start);
        if (!message)
        {
            return 0;
        }
        m_messages.push_back(std::move(*message));
    }
    return initialRemaining;
}

std::optional<Rfc5444MessageVariant>
Rfc5444Packet::DeserializeMessage(Buffer::Iterator& start)
{
    if (start.GetRemainingSize() < 2)
    {
        NS_LOG_DEBUG("message truncated in header");
        return std::nullopt;
    }

    // Peek at the msg-addr-length nibble to select the address family.
    Buffer::Iterator peek = start;
    peek.Next(1);
    const uint8_t addrLengthField = peek.ReadU8() & 0x0f;

    if (addrLengthField == AddressLength<Ipv4Address>() - 1)
    {
        Rfc5444MessageIpv4 message;
        if (!message.Deserialize(start))
        {
            return std::nullopt;
        }
        return message;
    }
    if (addrLengthField == AddressLength<Ipv6Address>() - 1)
    {
        Rfc5444MessageIpv6 message;
        if (!message.Deserialize(start))
        {
            return std::nullopt;
        }
        return message;
    }
    NS_LOG_DEBUG("unsupported msg-addr-length");
    return std::nullopt;
}

void
Rfc5444Packet::Print(std::ostream& os) const
{
    os << "Rfc5444Packet {" << std::endl;
    if (m_sequenceNumber)
    {
        os << "  sequenceNumber = " << *m_sequenceNumber << std::endl;
    }
    for (const auto& tlv : m_tlvs)
    {
        tlv.Print(os, 1);
    }
    for (const auto& message : m_messages)
    {
        std::visit([&os](const auto& m) { m.Print(os, 1); }, message);
    }
    os << "}" << std::endl;
}

bool
Rfc5444Packet::operator==(const Rfc5444Packet& other) const
{
    return m_sequenceNumber == other.m_sequenceNumber && m_tlvs == other.m_tlvs &&
           m_messages == other.m_messages;
}

template class Rfc5444AddressBlock<Ipv4Address>;
template class Rfc5444AddressBlock<Ipv6Address>;
template class Rfc5444Message<Ipv4Address>;
template class Rfc5444Message<Ipv6Address>;

} // namespace ns3
