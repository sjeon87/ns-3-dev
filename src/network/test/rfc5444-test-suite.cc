/*
 * Copyright (c) 2009 Drexel University
 *
 * SPDX-License-Identifier: GPL-2.0-only AND NIST-Software
 *
 * Authors: Tom Wambold <tom5760@gmail.com> (original ns-3 PacketBB implementation and tests)
 *          Tom Henderson <tomh@tomh.org> (rewrite assisted by Claude Fable 5)
 */

#include "ns3/buffer.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/rfc5444.h"
#include "ns3/test.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

using namespace ns3;

namespace
{

/**
 * Serialize an element (TLV or address block) and return its raw octets.
 *
 * @tparam ElementT The element class under test.
 * @param element The element to serialize.
 * @return The serialized octets.
 */
template <typename ElementT>
std::vector<uint8_t>
SerializeElement(const ElementT& element)
{
    Buffer buffer;
    buffer.AddAtStart(element.GetSerializedSize());
    auto it = buffer.Begin();
    element.Serialize(it);
    std::vector<uint8_t> bytes(buffer.GetSize());
    buffer.Begin().Read(bytes.data(), bytes.size());
    return bytes;
}

/**
 * Deserialize an element (TLV or address block) from raw octets.
 *
 * @tparam ElementT The element class under test.
 * @param bytes The octets to deserialize from.
 * @param element The element deserialized into.
 * @param consumed The number of octets consumed.
 * @return Whether deserialization succeeded.
 */
template <typename ElementT>
bool
DeserializeElement(const std::vector<uint8_t>& bytes, ElementT& element, uint32_t& consumed)
{
    Buffer buffer;
    buffer.AddAtStart(bytes.size());
    if (!bytes.empty())
    {
        buffer.Begin().Write(bytes.data(), bytes.size());
    }
    auto it = buffer.Begin();
    const bool ok = element.Deserialize(it);
    consumed = buffer.GetSize() - it.GetRemainingSize();
    return ok;
}

} // namespace

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * @brief Serialization and deserialization test of a single element.
 *
 * Checks that the object serializes to exactly the reference octets and that
 * the reference octets deserialize to an equal object.
 *
 * @tparam ElementT The element class under test, e.g., Rfc5444Tlv,
 *         Rfc5444AddressTlv, or Rfc5444AddressBlockIpv4.
 */
template <typename ElementT>
class Rfc5444ElementSerializationTestCase : public TestCase
{
  public:
    /**
     * Constructor.
     *
     * @param name Test name.
     * @param element Reference element.
     * @param bytes Reference serialized form.
     */
    Rfc5444ElementSerializationTestCase(std::string name,
                                        ElementT element,
                                        std::vector<uint8_t> bytes)
        : TestCase(name),
          m_refElement(element),
          m_refBytes(bytes)
    {
    }

  protected:
    void DoRun() override;

  private:
    ElementT m_refElement;           //!< Reference element
    std::vector<uint8_t> m_refBytes; //!< Reference serialized form
};

template <typename ElementT>
void
Rfc5444ElementSerializationTestCase<ElementT>::DoRun()
{
    NS_TEST_ASSERT_MSG_EQ(m_refElement.GetSerializedSize(),
                          m_refBytes.size(),
                          "wrong serialized size");

    const auto bytes = SerializeElement(m_refElement);
    NS_TEST_ASSERT_MSG_EQ((bytes == m_refBytes), true, "serialization produced different octets");

    ElementT newElement;
    uint32_t consumed = 0;
    const bool ok = DeserializeElement(m_refBytes, newElement, consumed);
    NS_TEST_ASSERT_MSG_EQ(ok, true, "deserialization failed");
    NS_TEST_ASSERT_MSG_EQ(consumed, m_refBytes.size(), "deserialization did not use all octets");
    NS_TEST_ASSERT_MSG_EQ((newElement == m_refElement),
                          true,
                          "deserialized element does not match");
}

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * @brief Deserialize-only test of TLV flag-rule and truncation handling.
 *
 * The same octets are offered to both Rfc5444Tlv (packet/message TLV rules)
 * and Rfc5444AddressTlv (address TLV rules), with an expected accept/reject
 * outcome for each.
 */
class Rfc5444TlvDeserializeTestCase : public TestCase
{
  public:
    /**
     * Constructor.
     *
     * @param name Test name.
     * @param bytes Octets to deserialize.
     * @param tlvOk Whether Rfc5444Tlv::Deserialize should succeed.
     * @param addressTlvOk Whether Rfc5444AddressTlv::Deserialize should succeed.
     */
    Rfc5444TlvDeserializeTestCase(std::string name,
                                  std::vector<uint8_t> bytes,
                                  bool tlvOk,
                                  bool addressTlvOk)
        : TestCase(name),
          m_bytes(bytes),
          m_tlvOk(tlvOk),
          m_addressTlvOk(addressTlvOk)
    {
    }

  protected:
    void DoRun() override;

  private:
    std::vector<uint8_t> m_bytes; //!< Octets to deserialize
    bool m_tlvOk;                 //!< Expected Rfc5444Tlv outcome
    bool m_addressTlvOk;          //!< Expected Rfc5444AddressTlv outcome
};

void
Rfc5444TlvDeserializeTestCase::DoRun()
{
    Rfc5444Tlv tlv;
    uint32_t consumed = 0;
    NS_TEST_ASSERT_MSG_EQ(DeserializeElement(m_bytes, tlv, consumed),
                          m_tlvOk,
                          "unexpected Rfc5444Tlv::Deserialize outcome");

    Rfc5444AddressTlv addressTlv;
    NS_TEST_ASSERT_MSG_EQ(DeserializeElement(m_bytes, addressTlv, consumed),
                          m_addressTlvOk,
                          "unexpected Rfc5444AddressTlv::Deserialize outcome");
}

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * @brief Deserialize-only test of address block handling of malformed input.
 *
 * @tparam AddrT The address family of the block: Ipv4Address or Ipv6Address.
 */
template <typename AddrT>
class Rfc5444AddressBlockDeserializeTestCase : public TestCase
{
  public:
    /**
     * Constructor.
     *
     * @param name Test name.
     * @param bytes Octets to deserialize.
     * @param ok Whether deserialization should succeed.
     */
    Rfc5444AddressBlockDeserializeTestCase(std::string name, std::vector<uint8_t> bytes, bool ok)
        : TestCase(name),
          m_bytes(bytes),
          m_ok(ok)
    {
    }

  protected:
    void DoRun() override;

  private:
    std::vector<uint8_t> m_bytes; //!< Octets to deserialize
    bool m_ok;                    //!< Expected outcome
};

template <typename AddrT>
void
Rfc5444AddressBlockDeserializeTestCase<AddrT>::DoRun()
{
    Rfc5444AddressBlock<AddrT> block;
    uint32_t consumed = 0;
    NS_TEST_ASSERT_MSG_EQ(DeserializeElement(m_bytes, block, consumed),
                          m_ok,
                          "unexpected Rfc5444AddressBlock::Deserialize outcome");
}

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * @brief Serialization and deserialization test of a whole Rfc5444Packet.
 *
 * Checks that the packet serializes to exactly the reference octets and that
 * the reference octets deserialize to an equal packet.
 */
class Rfc5444PacketSerializationTestCase : public TestCase
{
  public:
    /**
     * Constructor.
     *
     * @param name Test name.
     * @param packet Reference packet.
     * @param bytes Reference serialized form.
     */
    Rfc5444PacketSerializationTestCase(std::string name,
                                       Rfc5444Packet packet,
                                       std::vector<uint8_t> bytes)
        : TestCase(name),
          m_refPacket(packet),
          m_refBytes(bytes)
    {
    }

  protected:
    void DoRun() override;

  private:
    Rfc5444Packet m_refPacket;       //!< Reference packet
    std::vector<uint8_t> m_refBytes; //!< Reference serialized form
};

void
Rfc5444PacketSerializationTestCase::DoRun()
{
    NS_TEST_ASSERT_MSG_EQ(m_refPacket.GetSerializedSize(),
                          m_refBytes.size(),
                          "wrong serialized size");

    Buffer buffer;
    buffer.AddAtStart(m_refPacket.GetSerializedSize());
    m_refPacket.Serialize(buffer.Begin());
    std::vector<uint8_t> bytes(buffer.GetSize());
    buffer.Begin().Read(bytes.data(), bytes.size());
    NS_TEST_ASSERT_MSG_EQ((bytes == m_refBytes), true, "serialization produced different octets");

    Buffer refBuffer;
    refBuffer.AddAtStart(m_refBytes.size());
    refBuffer.Begin().Write(m_refBytes.data(), m_refBytes.size());
    Rfc5444Packet newPacket;
    const uint32_t consumed = newPacket.Deserialize(refBuffer.Begin());
    NS_TEST_ASSERT_MSG_EQ(consumed, m_refBytes.size(), "deserialization did not use all octets");
    NS_TEST_ASSERT_MSG_EQ((newPacket == m_refPacket), true, "deserialized packet does not match");
}

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * @brief Deserialize-only test of Rfc5444Packet handling of malformed or
 * non-canonical input.
 */
class Rfc5444PacketDeserializeTestCase : public TestCase
{
  public:
    /**
     * Constructor.
     *
     * @param name Test name.
     * @param bytes Octets to deserialize.
     * @param expectedConsumed The expected Deserialize() return value: the
     *        octet count on success, or zero for a malformed packet.
     */
    Rfc5444PacketDeserializeTestCase(std::string name,
                                     std::vector<uint8_t> bytes,
                                     uint32_t expectedConsumed)
        : TestCase(name),
          m_bytes(bytes),
          m_expectedConsumed(expectedConsumed)
    {
    }

  protected:
    void DoRun() override;

  private:
    std::vector<uint8_t> m_bytes; //!< Octets to deserialize
    uint32_t m_expectedConsumed;  //!< Expected Deserialize() return value
};

void
Rfc5444PacketDeserializeTestCase::DoRun()
{
    Buffer buffer;
    buffer.AddAtStart(m_bytes.size());
    if (!m_bytes.empty())
    {
        buffer.Begin().Write(m_bytes.data(), m_bytes.size());
    }
    Rfc5444Packet packet;
    NS_TEST_ASSERT_MSG_EQ(packet.Deserialize(buffer.Begin()),
                          m_expectedConsumed,
                          "unexpected Rfc5444Packet::Deserialize outcome");
}

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * @brief Tests of the typed value accessors of Rfc5444Tlv.
 */
class Rfc5444TlvValueAccessorTestCase : public TestCase
{
  public:
    Rfc5444TlvValueAccessorTestCase()
        : TestCase("Rfc5444Tlv value accessors")
    {
    }

  protected:
    void DoRun() override;
};

void
Rfc5444TlvValueAccessorTestCase::DoRun()
{
    Rfc5444Tlv tlv;
    NS_TEST_ASSERT_MSG_EQ(tlv.GetValue().has_value(), false, "unexpected value on fresh TLV");
    NS_TEST_ASSERT_MSG_EQ(tlv.GetValueAsUint8().has_value(),
                          false,
                          "unexpected uint8 value on fresh TLV");
    NS_TEST_ASSERT_MSG_EQ(tlv.GetValueAsUint16().has_value(),
                          false,
                          "unexpected uint16 value on fresh TLV");

    tlv.SetValue(uint8_t{0x2a});
    NS_TEST_ASSERT_MSG_EQ(tlv.GetValue()->size(), 1, "wrong one-octet value size");
    NS_TEST_ASSERT_MSG_EQ(*tlv.GetValueAsUint8(), 0x2a, "wrong uint8 value");
    NS_TEST_ASSERT_MSG_EQ(tlv.GetValueAsUint16().has_value(),
                          false,
                          "uint16 accessor accepted a one-octet value");

    tlv.SetValue(uint16_t{0x1234});
    NS_TEST_ASSERT_MSG_EQ(tlv.GetValue()->size(), 2, "wrong two-octet value size");
    NS_TEST_ASSERT_MSG_EQ((*tlv.GetValue())[0], 0x12, "wrong network byte order");
    NS_TEST_ASSERT_MSG_EQ((*tlv.GetValue())[1], 0x34, "wrong network byte order");
    NS_TEST_ASSERT_MSG_EQ(*tlv.GetValueAsUint16(), 0x1234, "wrong uint16 value");
    NS_TEST_ASSERT_MSG_EQ(tlv.GetValueAsUint8().has_value(),
                          false,
                          "uint8 accessor accepted a two-octet value");

    tlv.ClearValue();
    NS_TEST_ASSERT_MSG_EQ(tlv.GetValue().has_value(), false, "value present after ClearValue()");
}

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * @brief Tests of Rfc5444AddressTlv per-address value semantics
 * (RFC 5444 Section 5.4.2).
 */
class Rfc5444AddressTlvValueForIndexTestCase : public TestCase
{
  public:
    Rfc5444AddressTlvValueForIndexTestCase()
        : TestCase("Rfc5444AddressTlv GetValueForIndex")
    {
    }

  protected:
    void DoRun() override;
};

void
Rfc5444AddressTlvValueForIndexTestCase::DoRun()
{
    // No index range: the whole value applies to every address.
    Rfc5444AddressTlv tlv;
    tlv.SetType(1);
    tlv.SetValue(std::vector<uint8_t>{1, 2, 3});
    for (uint8_t index : {0, 7, 255})
    {
        const auto value = tlv.GetValueForIndex(index);
        NS_TEST_ASSERT_MSG_EQ(value.has_value(), true, "no value without an index range");
        NS_TEST_ASSERT_MSG_EQ(value->size(), 3, "expected the whole value");
    }

    // No value: nothing applies, even inside an index range.
    tlv.ClearValue();
    tlv.SetIndexRange(1, 3);
    NS_TEST_ASSERT_MSG_EQ(tlv.GetValueForIndex(2).has_value(),
                          false,
                          "value reported for a TLV without a value");

    // Single value with an index range: the whole value applies to each
    // covered address, and nothing applies outside the range.
    tlv.SetValue(std::vector<uint8_t>{9, 9});
    NS_TEST_ASSERT_MSG_EQ(tlv.GetValueForIndex(0).has_value(), false, "index below range covered");
    NS_TEST_ASSERT_MSG_EQ(tlv.GetValueForIndex(4).has_value(), false, "index above range covered");
    for (uint8_t index : {1, 2, 3})
    {
        const auto value = tlv.GetValueForIndex(index);
        NS_TEST_ASSERT_MSG_EQ(value.has_value(), true, "covered index has no value");
        NS_TEST_ASSERT_MSG_EQ(value->size(), 2, "expected the whole value");
        NS_TEST_ASSERT_MSG_EQ(tlv.IsMultivalue(), false, "single value reported as multivalue");
    }

    // Multivalue: each covered address gets its own slice.
    Rfc5444AddressTlv multivalue;
    multivalue.SetType(2);
    multivalue.SetValues(1, {{0x0a, 0x0b}, {0x0c, 0x0d}, {0x0e, 0x0f}});
    NS_TEST_ASSERT_MSG_EQ(multivalue.IsMultivalue(), true, "SetValues() did not set multivalue");
    NS_TEST_ASSERT_MSG_EQ(*multivalue.GetIndexStart(), 1, "wrong index start");
    NS_TEST_ASSERT_MSG_EQ(*multivalue.GetIndexStop(), 3, "wrong index stop");
    NS_TEST_ASSERT_MSG_EQ(multivalue.GetValue()->size(), 6, "wrong concatenated value size");
    NS_TEST_ASSERT_MSG_EQ(multivalue.GetValueForIndex(0).has_value(),
                          false,
                          "index below range covered");
    for (uint8_t index : {1, 2, 3})
    {
        const auto value = multivalue.GetValueForIndex(index);
        NS_TEST_ASSERT_MSG_EQ(value.has_value(), true, "covered index has no value");
        NS_TEST_ASSERT_MSG_EQ(value->size(), 2, "wrong single-length");
        NS_TEST_ASSERT_MSG_EQ((*value)[0], 0x0a + 2 * (index - 1), "wrong value slice");
        NS_TEST_ASSERT_MSG_EQ((*value)[1], 0x0b + 2 * (index - 1), "wrong value slice");
    }
    NS_TEST_ASSERT_MSG_EQ(multivalue.GetValueForIndex(4).has_value(),
                          false,
                          "index above range covered");

    // ClearIndexRange() also clears the multivalue flag; the concatenated
    // value then applies whole to every address.
    multivalue.ClearIndexRange();
    NS_TEST_ASSERT_MSG_EQ(multivalue.IsMultivalue(),
                          false,
                          "multivalue set after ClearIndexRange()");
    NS_TEST_ASSERT_MSG_EQ(multivalue.GetIndexStart().has_value(),
                          false,
                          "index range present after ClearIndexRange()");
    NS_TEST_ASSERT_MSG_EQ(multivalue.GetValueForIndex(200)->size(), 6, "expected the whole value");
}

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * @brief RFC 5444 TestSuite
 */
class Rfc5444TestSuite : public TestSuite
{
  public:
    Rfc5444TestSuite();
};

Rfc5444TestSuite::Rfc5444TestSuite()
    : TestSuite("rfc5444", Type::UNIT)
{
    /* Serialize/deserialize cases. The byte vectors are TLV slices of the
     * packet-level vectors in packetbb-test-suite.cc (the old test number is
     * noted). Flag names in comments (thassingleindex, thasmultiindex,
     * tismultivalue, ...) are the <tlv-flags> bit names defined in RFC 5444
     * Section 5.4.1. */

    /* Minimal TLV: type only. [old test 4] */
    {
        Rfc5444Tlv tlv;
        tlv.SetType(1);
        AddTestCase(new Rfc5444ElementSerializationTestCase("tlv-minimal", tlv, {0x01, 0x00}),
                    TestCase::Duration::QUICK);
    }

    /* Type extension, no value. [old test 5] */
    {
        Rfc5444Tlv tlv;
        tlv.SetType(2);
        tlv.SetTypeExt(100);
        AddTestCase(
            new Rfc5444ElementSerializationTestCase("tlv-type-ext", tlv, {0x02, 0x80, 0x64}),
            TestCase::Duration::QUICK);
    }

    /* Type extension and a four-octet value. [old test 6] */
    {
        Rfc5444Tlv tlv;
        tlv.SetType(2);
        tlv.SetTypeExt(100);
        tlv.SetValue(std::vector<uint8_t>{1, 2, 3, 4});
        AddTestCase(new Rfc5444ElementSerializationTestCase(
                        "tlv-value",
                        tlv,
                        {0x02, 0x90, 0x64, 0x04, 0x01, 0x02, 0x03, 0x04}),
                    TestCase::Duration::QUICK);
    }

    /* A 300-octet value, exercising the 16-bit extended length. [old test 7] */
    {
        std::vector<uint8_t> value(300);
        for (std::size_t i = 0; i < value.size(); i++)
        {
            value[i] = static_cast<uint8_t>(i % 255);
        }

        Rfc5444Tlv tlv;
        tlv.SetType(2);
        tlv.SetTypeExt(100);
        tlv.SetValue(value);

        std::vector<uint8_t> bytes{0x02, 0x98, 0x64, 0x01, 0x2c};
        bytes.insert(bytes.end(), value.begin(), value.end());
        AddTestCase(new Rfc5444ElementSerializationTestCase("tlv-ext-len", tlv, bytes),
                    TestCase::Duration::QUICK);
    }

    /* One- and two-octet convenience values. */
    {
        Rfc5444Tlv tlv;
        tlv.SetType(3);
        tlv.SetValue(uint8_t{0x2a});
        AddTestCase(new Rfc5444ElementSerializationTestCase("tlv-value-uint8",
                                                            tlv,
                                                            {0x03, 0x10, 0x01, 0x2a}),
                    TestCase::Duration::QUICK);
    }
    {
        Rfc5444Tlv tlv;
        tlv.SetType(3);
        tlv.SetValue(uint16_t{0x1234});
        AddTestCase(new Rfc5444ElementSerializationTestCase("tlv-value-uint16",
                                                            tlv,
                                                            {0x03, 0x10, 0x02, 0x12, 0x34}),
                    TestCase::Duration::QUICK);
    }

    /* Address TLV without an index range serializes like a plain TLV. */
    {
        Rfc5444AddressTlv tlv;
        tlv.SetType(1);
        tlv.SetValue(uint8_t{0x01});
        AddTestCase(new Rfc5444ElementSerializationTestCase("address-tlv-no-index",
                                                            tlv,
                                                            {0x01, 0x10, 0x01, 0x01}),
                    TestCase::Duration::QUICK);
    }

    /* Single-index address TLV (thassingleindex flag). [old test 25] */
    {
        Rfc5444AddressTlv tlv;
        tlv.SetType(1);
        tlv.SetIndexRange(1, 1);
        AddTestCase(new Rfc5444ElementSerializationTestCase("address-tlv-single-index",
                                                            tlv,
                                                            {0x01, 0x40, 0x01}),
                    TestCase::Duration::QUICK);
    }

    /* Multi-index address TLV (thasmultiindex flag). [old test 26] */
    {
        Rfc5444AddressTlv tlv;
        tlv.SetType(1);
        tlv.SetIndexRange(1, 3);
        AddTestCase(new Rfc5444ElementSerializationTestCase("address-tlv-multi-index",
                                                            tlv,
                                                            {0x01, 0x20, 0x01, 0x03}),
                    TestCase::Duration::QUICK);
    }

    /* Multivalue address TLV, one octet per address. [old test 27] */
    {
        Rfc5444AddressTlv tlv;
        tlv.SetType(1);
        tlv.SetValues(1, {{1}, {2}, {3}});
        AddTestCase(new Rfc5444ElementSerializationTestCase(
                        "address-tlv-multivalue",
                        tlv,
                        {0x01, 0x34, 0x01, 0x03, 0x03, 0x01, 0x02, 0x03}),
                    TestCase::Duration::QUICK);
    }

    /* Multivalue address TLV, two octets per address. */
    {
        Rfc5444AddressTlv tlv;
        tlv.SetType(1);
        tlv.SetValues(0, {{0x00, 0x01}, {0x00, 0x02}});
        AddTestCase(new Rfc5444ElementSerializationTestCase(
                        "address-tlv-multivalue-two-octet",
                        tlv,
                        {0x01, 0x34, 0x00, 0x01, 0x04, 0x00, 0x01, 0x00, 0x02}),
                    TestCase::Duration::QUICK);
    }

    /* RFC 5444 Section 5.4.1 requires the tismultivalue flag to be cleared
     * when the thasmultiindex flag is cleared, so a single-entry multivalue
     * TLV serializes with the thasmultiindex flag and index-start equal to
     * index-stop, and deserializes back to an equal TLV. */
    {
        Rfc5444AddressTlv tlv;
        tlv.SetType(1);
        tlv.SetValues(2, {{7}});
        AddTestCase(new Rfc5444ElementSerializationTestCase("address-tlv-multivalue-single-entry",
                                                            tlv,
                                                            {0x01, 0x34, 0x02, 0x02, 0x01, 0x07}),
                    TestCase::Duration::QUICK);
    }

    /* Deserialize-only cases: truncation and the flag combinations RFC 5444
     * Section 5.4.1 forbids, offered to both TLV classes. */
    AddTestCase(new Rfc5444TlvDeserializeTestCase("truncated-empty", {}, false, false),
                TestCase::Duration::QUICK);
    AddTestCase(new Rfc5444TlvDeserializeTestCase("truncated-flags", {0x01}, false, false),
                TestCase::Duration::QUICK);
    AddTestCase(new Rfc5444TlvDeserializeTestCase("truncated-type-ext", {0x01, 0x80}, false, false),
                TestCase::Duration::QUICK);
    AddTestCase(new Rfc5444TlvDeserializeTestCase("truncated-index", {0x01, 0x40}, false, false),
                TestCase::Duration::QUICK);
    AddTestCase(new Rfc5444TlvDeserializeTestCase("truncated-length", {0x01, 0x10}, false, false),
                TestCase::Duration::QUICK);
    AddTestCase(
        new Rfc5444TlvDeserializeTestCase("truncated-ext-length", {0x01, 0x18, 0x01}, false, false),
        TestCase::Duration::QUICK);
    AddTestCase(new Rfc5444TlvDeserializeTestCase("truncated-value",
                                                  {0x01, 0x10, 0x02, 0xaa},
                                                  false,
                                                  false),
                TestCase::Duration::QUICK);
    AddTestCase(new Rfc5444TlvDeserializeTestCase("both-index-flags",
                                                  {0x01, 0x60, 0x01, 0x02},
                                                  false,
                                                  false),
                TestCase::Duration::QUICK);
    AddTestCase(
        new Rfc5444TlvDeserializeTestCase("ext-len-without-value", {0x01, 0x08}, false, false),
        TestCase::Duration::QUICK);
    /* Index flags are valid only in address TLVs. */
    AddTestCase(
        new Rfc5444TlvDeserializeTestCase("single-index-kind", {0x01, 0x40, 0x01}, false, true),
        TestCase::Duration::QUICK);
    AddTestCase(new Rfc5444TlvDeserializeTestCase("multi-index-kind",
                                                  {0x01, 0x20, 0x01, 0x03},
                                                  false,
                                                  true),
                TestCase::Duration::QUICK);
    AddTestCase(new Rfc5444TlvDeserializeTestCase("multivalue-without-multi-index",
                                                  {0x01, 0x14, 0x01, 0xaa},
                                                  false,
                                                  false),
                TestCase::Duration::QUICK);
    AddTestCase(new Rfc5444TlvDeserializeTestCase("multivalue-with-single-index",
                                                  {0x01, 0x54, 0x01, 0x01, 0xaa},
                                                  false,
                                                  false),
                TestCase::Duration::QUICK);
    AddTestCase(new Rfc5444TlvDeserializeTestCase("multivalue-without-value",
                                                  {0x01, 0x24, 0x01, 0x03},
                                                  false,
                                                  false),
                TestCase::Duration::QUICK);
    AddTestCase(new Rfc5444TlvDeserializeTestCase("multivalue-not-divisible",
                                                  {0x01, 0x34, 0x00, 0x02, 0x02, 0xaa, 0xbb},
                                                  false,
                                                  false),
                TestCase::Duration::QUICK);
    AddTestCase(new Rfc5444TlvDeserializeTestCase("index-stop-less-than-start",
                                                  {0x01, 0x20, 0x03, 0x01},
                                                  false,
                                                  false),
                TestCase::Duration::QUICK);
    /* Reserved flag bits 6-7 are ignored on reception. */
    AddTestCase(
        new Rfc5444TlvDeserializeTestCase("reserved-flags-ignored", {0x01, 0x03}, true, true),
        TestCase::Duration::QUICK);

    /* Address block cases. Byte vectors are address-block slices of the
     * packet-level vectors in packetbb-test-suite.cc where noted; each block
     * includes its trailing address TLV block (tlvs-length octets and TLVs).
     * Flag names are the <addr-flags> bit names of RFC 5444 Section 5.3. */

    /* Two IPv4 addresses sharing one head and one tail octet (ahashead and
     * ahasfulltail flags), no TLVs. [old test 25] */
    {
        Rfc5444AddressBlockIpv4 block;
        block.Addresses() = {Ipv4Address("10.0.0.2"), Ipv4Address("10.1.1.2")};
        AddTestCase(new Rfc5444ElementSerializationTestCase(
                        "address-block-head-tail",
                        block,
                        {0x02, 0xc0, 0x01, 0x0a, 0x01, 0x02, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00}),
                    TestCase::Duration::QUICK);
    }

    /* Four IPv4 address prefixes with per-address prefix lengths
     * (ahasmultiprelen flag) and one single-index TLV. [old test 25] */
    {
        Rfc5444AddressBlockIpv4 block;
        block.Addresses() = {{Ipv4Address("10.0.0.0"), 32},
                             {Ipv4Address("11.0.0.0"), 32},
                             {Ipv4Address("10.0.0.5"), 16},
                             {Ipv4Address("10.0.0.6"), 24}};
        Rfc5444AddressTlv tlv;
        tlv.SetType(1);
        tlv.SetIndexRange(1, 1);
        block.Tlvs().push_back(tlv);
        AddTestCase(
            new Rfc5444ElementSerializationTestCase(
                "address-block-multi-prefix",
                block,
                {0x04, 0x08, 0x0a, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x05,
                 0x0a, 0x00, 0x00, 0x06, 0x20, 0x20, 0x10, 0x18, 0x00, 0x03, 0x01, 0x40, 0x01}),
            TestCase::Duration::QUICK);
    }

    /* A single address: no compression, and no prefix-length octet because
     * the prefix length is the full address length. */
    {
        Rfc5444AddressBlockIpv4 block;
        block.Addresses() = {Ipv4Address("10.0.0.2")};
        AddTestCase(new Rfc5444ElementSerializationTestCase(
                        "address-block-single-address",
                        block,
                        {0x01, 0x00, 0x0a, 0x00, 0x00, 0x02, 0x00, 0x00}),
                    TestCase::Duration::QUICK);
    }

    /* All prefixes share length 16 (ahassingleprelen flag); the common tail
     * octet is zero (ahaszerotail flag), so no tail octets are emitted. */
    {
        Rfc5444AddressBlockIpv4 block;
        block.Addresses() = {{Ipv4Address("10.0.0.0"), 16}, {Ipv4Address("10.0.1.0"), 16}};
        AddTestCase(new Rfc5444ElementSerializationTestCase(
                        "address-block-single-prefix-zero-tail",
                        block,
                        {0x02, 0xb0, 0x02, 0x0a, 0x00, 0x01, 0x00, 0x01, 0x10, 0x00, 0x00}),
                    TestCase::Duration::QUICK);
    }

    /* IPv6 block with a common head and an all-zero common tail. */
    {
        Rfc5444AddressBlockIpv6 block;
        block.Addresses() = {Ipv6Address("fe80::1:0:0"), Ipv6Address("fe80::2:0:0")};
        AddTestCase(new Rfc5444ElementSerializationTestCase("address-block-ipv6-zero-tail",
                                                            block,
                                                            {0x02,
                                                             0xa0,
                                                             0x0b,
                                                             0xfe,
                                                             0x80,
                                                             0x00,
                                                             0x00,
                                                             0x00,
                                                             0x00,
                                                             0x00,
                                                             0x00,
                                                             0x00,
                                                             0x00,
                                                             0x00,
                                                             0x04,
                                                             0x01,
                                                             0x02,
                                                             0x00,
                                                             0x00}),
                    TestCase::Duration::QUICK);
    }

    /* Mixed-suffix IPv6 addresses (regression for the head/tail corruption of
     * ns-3 issue #1340): the pair (fc00::1, fe80::3) reduces the common tail
     * to zero and the trailing pair (fe80::3, fc00::3) must not resurrect it,
     * so all four addresses serialize in full. */
    {
        const std::vector<Ipv6Address> addresses = {Ipv6Address("fe80::1"),
                                                    Ipv6Address("fc00::1"),
                                                    Ipv6Address("fe80::3"),
                                                    Ipv6Address("fc00::3")};
        Rfc5444AddressBlockIpv6 block;
        std::vector<uint8_t> bytes{0x04, 0x00};
        for (const auto& address : addresses)
        {
            block.Addresses().emplace_back(address);
            uint8_t octets[16];
            address.Serialize(octets);
            bytes.insert(bytes.end(), octets, octets + 16);
        }
        bytes.insert(bytes.end(), {0x00, 0x00});
        AddTestCase(new Rfc5444ElementSerializationTestCase("address-block-mixed-suffix-1340",
                                                            block,
                                                            bytes),
                    TestCase::Duration::QUICK);
    }

    /* Deserialize-only address block cases: truncation and the combinations
     * RFC 5444 Section 5.3 forbids. */
    AddTestCase(
        new Rfc5444AddressBlockDeserializeTestCase<Ipv4Address>("address-block-empty", {}, false),
        TestCase::Duration::QUICK);
    AddTestCase(
        new Rfc5444AddressBlockDeserializeTestCase<Ipv4Address>("address-block-num-addr-zero",
                                                                {0x00, 0x00, 0x00, 0x00},
                                                                false),
        TestCase::Duration::QUICK);
    AddTestCase(
        new Rfc5444AddressBlockDeserializeTestCase<Ipv4Address>("address-block-both-tail-flags",
                                                                {0x02, 0x60},
                                                                false),
        TestCase::Duration::QUICK);
    AddTestCase(
        new Rfc5444AddressBlockDeserializeTestCase<Ipv4Address>("address-block-both-prelen-flags",
                                                                {0x02, 0x18},
                                                                false),
        TestCase::Duration::QUICK);
    AddTestCase(
        new Rfc5444AddressBlockDeserializeTestCase<Ipv4Address>("address-block-head-too-long",
                                                                {0x02, 0x80, 0x05},
                                                                false),
        TestCase::Duration::QUICK);
    AddTestCase(new Rfc5444AddressBlockDeserializeTestCase<Ipv4Address>(
                    "address-block-negative-mid",
                    {0x02, 0xc0, 0x03, 0xaa, 0xbb, 0xcc, 0x02},
                    false),
                TestCase::Duration::QUICK);
    AddTestCase(new Rfc5444AddressBlockDeserializeTestCase<Ipv4Address>(
                    "address-block-prefix-too-large",
                    {0x01, 0x10, 0x0a, 0x00, 0x00, 0x02, 0x21},
                    false),
                TestCase::Duration::QUICK);
    AddTestCase(new Rfc5444AddressBlockDeserializeTestCase<Ipv4Address>(
                    "address-block-truncated-mid",
                    {0x02, 0x00, 0x0a, 0x00, 0x00, 0x02},
                    false),
                TestCase::Duration::QUICK);
    AddTestCase(new Rfc5444AddressBlockDeserializeTestCase<Ipv4Address>(
                    "address-block-truncated-tlv-block",
                    {0x01, 0x00, 0x0a, 0x00, 0x00, 0x02, 0x00},
                    false),
                TestCase::Duration::QUICK);
    AddTestCase(new Rfc5444AddressBlockDeserializeTestCase<Ipv4Address>(
                    "address-block-tlv-block-length-mismatch",
                    {0x01, 0x00, 0x0a, 0x00, 0x00, 0x02, 0x00, 0x03, 0x01, 0x00, 0xff},
                    false),
                TestCase::Duration::QUICK);
    AddTestCase(new Rfc5444AddressBlockDeserializeTestCase<Ipv4Address>(
                    "address-block-tlv-overruns-block",
                    {0x01, 0x00, 0x0a, 0x00, 0x00, 0x02, 0x00, 0x02, 0x01, 0x10, 0x01, 0xaa},
                    false),
                TestCase::Duration::QUICK);

    /* Packet cases ported from packetbb-test-suite.cc; the old test number is
     * noted and the byte vectors are unchanged. Old tests 1, 2, 8, and 9 are
     * the four cases directly below; the remaining ports follow the
     * deserialize-only packet cases. */

    /* Empty packet. [old test 1] */
    {
        Rfc5444Packet packet;
        AddTestCase(new Rfc5444PacketSerializationTestCase("packet-minimal", packet, {0x00}),
                    TestCase::Duration::QUICK);
    }

    /* Sequence number only (phasseqnum flag). [old test 2] */
    {
        Rfc5444Packet packet;
        packet.SetSequenceNumber(2);
        AddTestCase(
            new Rfc5444PacketSerializationTestCase("packet-seq-num", packet, {0x08, 0x00, 0x02}),
            TestCase::Duration::QUICK);
    }

    /* Packet TLV and a minimal IPv4 message. [old test 8] */
    {
        Rfc5444Packet packet;
        packet.SetSequenceNumber(8);

        Rfc5444Tlv tlv;
        tlv.SetType(1);
        packet.Tlvs().push_back(tlv);

        Rfc5444MessageIpv4 message;
        message.SetType(1);
        packet.Messages().emplace_back(message);

        AddTestCase(
            new Rfc5444PacketSerializationTestCase(
                "packet-message-minimal",
                packet,
                {0x0c, 0x00, 0x08, 0x00, 0x02, 0x01, 0x00, 0x01, 0x03, 0x00, 0x06, 0x00, 0x00}),
            TestCase::Duration::QUICK);
    }

    /* Two IPv4 messages, the second carrying an originator address.
     * [old test 9] */
    {
        Rfc5444Packet packet;
        packet.SetSequenceNumber(9);

        Rfc5444Tlv tlv;
        tlv.SetType(1);
        packet.Tlvs().push_back(tlv);

        Rfc5444MessageIpv4 message1;
        message1.SetType(1);
        packet.Messages().emplace_back(message1);

        Rfc5444MessageIpv4 message2;
        message2.SetType(2);
        message2.SetOriginatorAddress(Ipv4Address("10.0.0.1"));
        packet.Messages().emplace_back(message2);

        AddTestCase(new Rfc5444PacketSerializationTestCase(
                        "packet-message-originator",
                        packet,
                        {0x0c, 0x00, 0x09, 0x00, 0x02, 0x01, 0x00, 0x01, 0x03, 0x00, 0x06, 0x00,
                         0x00, 0x02, 0x83, 0x00, 0x0a, 0x0a, 0x00, 0x00, 0x01, 0x00, 0x00}),
                    TestCase::Duration::QUICK);
    }

    /* Deserialize-only packet cases. */

    /* A present-but-empty packet TLV block (the old suite's test 3, built
     * with ForceTlv()) is questionable but not malformed; it is accepted on
     * input, while our canonical serialization omits the empty block. */
    AddTestCase(new Rfc5444PacketDeserializeTestCase("packet-empty-tlv-block",
                                                     {0x0c, 0x00, 0x03, 0x00, 0x00},
                                                     5),
                TestCase::Duration::QUICK);
    AddTestCase(new Rfc5444PacketDeserializeTestCase("packet-version-nonzero", {0x10}, 0),
                TestCase::Duration::QUICK);
    AddTestCase(new Rfc5444PacketDeserializeTestCase("packet-truncated-seq-num", {0x08, 0x00}, 0),
                TestCase::Duration::QUICK);
    AddTestCase(new Rfc5444PacketDeserializeTestCase("packet-bad-msg-addr-length",
                                                     {0x00, 0x01, 0x05, 0x00, 0x06, 0x00, 0x00},
                                                     0),
                TestCase::Duration::QUICK);
    AddTestCase(new Rfc5444PacketDeserializeTestCase("packet-msg-size-overruns",
                                                     {0x00, 0x01, 0x03, 0x00, 0x08, 0x00, 0x00},
                                                     0),
                TestCase::Duration::QUICK);

    /* The remaining ports from packetbb-test-suite.cc. The old mixed-suffix
     * IPv6 regression test (#1340) is not re-ported here: the byte-exact
     * address-block-mixed-suffix-1340 case above covers it. */

    /* The 300-octet TLV value of old tests 7, 28, 36, and 37. */
    std::vector<uint8_t> longValue(300);
    for (std::size_t i = 0; i < longValue.size(); i++)
    {
        longValue[i] = static_cast<uint8_t>(i % 255);
    }

    /* Packet TLVs only. [old tests 4-7] */
    {
        Rfc5444Packet packet;
        packet.SetSequenceNumber(4);
        Rfc5444Tlv tlv;
        tlv.SetType(1);
        packet.Tlvs().push_back(tlv);
        AddTestCase(
            new Rfc5444PacketSerializationTestCase("packet-old-test-4",
                                                   packet,
                                                   {0x0c, 0x00, 0x04, 0x00, 0x02, 0x01, 0x00}),
            TestCase::Duration::QUICK);
    }
    {
        Rfc5444Packet packet;
        packet.SetSequenceNumber(5);
        Rfc5444Tlv tlv1;
        tlv1.SetType(1);
        packet.Tlvs().push_back(tlv1);
        Rfc5444Tlv tlv2;
        tlv2.SetType(2);
        tlv2.SetTypeExt(100);
        packet.Tlvs().push_back(tlv2);
        AddTestCase(new Rfc5444PacketSerializationTestCase(
                        "packet-old-test-5",
                        packet,
                        {0x0c, 0x00, 0x05, 0x00, 0x05, 0x01, 0x00, 0x02, 0x80, 0x64}),
                    TestCase::Duration::QUICK);
    }
    {
        Rfc5444Packet packet;
        packet.SetSequenceNumber(6);
        Rfc5444Tlv tlv1;
        tlv1.SetType(1);
        packet.Tlvs().push_back(tlv1);
        Rfc5444Tlv tlv2;
        tlv2.SetType(2);
        tlv2.SetTypeExt(100);
        tlv2.SetValue(std::vector<uint8_t>{1, 2, 3, 4});
        packet.Tlvs().push_back(tlv2);
        AddTestCase(new Rfc5444PacketSerializationTestCase("packet-old-test-6",
                                                           packet,
                                                           {0x0c,
                                                            0x00,
                                                            0x06,
                                                            0x00,
                                                            0x0a,
                                                            0x01,
                                                            0x00,
                                                            0x02,
                                                            0x90,
                                                            0x64,
                                                            0x04,
                                                            0x01,
                                                            0x02,
                                                            0x03,
                                                            0x04}),
                    TestCase::Duration::QUICK);
    }
    {
        Rfc5444Packet packet;
        packet.SetSequenceNumber(7);
        Rfc5444Tlv tlv1;
        tlv1.SetType(1);
        packet.Tlvs().push_back(tlv1);
        Rfc5444Tlv tlv2;
        tlv2.SetType(2);
        tlv2.SetTypeExt(100);
        tlv2.SetValue(longValue);
        packet.Tlvs().push_back(tlv2);
        std::vector<uint8_t>
            bytes{0x0c, 0x00, 0x07, 0x01, 0x33, 0x01, 0x00, 0x02, 0x98, 0x64, 0x01, 0x2c};
        bytes.insert(bytes.end(), longValue.begin(), longValue.end());
        AddTestCase(new Rfc5444PacketSerializationTestCase("packet-old-test-7", packet, bytes),
                    TestCase::Duration::QUICK);
    }

    /* Message header field combinations. [old tests 10-12] */
    {
        Rfc5444Packet packet;
        packet.SetSequenceNumber(10);
        Rfc5444Tlv tlv;
        tlv.SetType(1);
        packet.Tlvs().push_back(tlv);
        Rfc5444MessageIpv4 message1;
        message1.SetType(1);
        packet.Messages().emplace_back(message1);
        Rfc5444MessageIpv4 message2;
        message2.SetType(2);
        message2.SetOriginatorAddress(Ipv4Address("10.0.0.1"));
        message2.SetHopCount(1);
        packet.Messages().emplace_back(message2);
        AddTestCase(new Rfc5444PacketSerializationTestCase(
                        "packet-old-test-10",
                        packet,
                        {0x0c, 0x00, 0x0a, 0x00, 0x02, 0x01, 0x00, 0x01, 0x03, 0x00, 0x06, 0x00,
                         0x00, 0x02, 0xa3, 0x00, 0x0b, 0x0a, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00}),
                    TestCase::Duration::QUICK);
    }
    {
        Rfc5444Packet packet;
        packet.SetSequenceNumber(11);
        Rfc5444Tlv tlv;
        tlv.SetType(1);
        packet.Tlvs().push_back(tlv);
        Rfc5444MessageIpv4 message1;
        message1.SetType(1);
        packet.Messages().emplace_back(message1);
        Rfc5444MessageIpv4 message2;
        message2.SetType(2);
        message2.SetOriginatorAddress(Ipv4Address("10.0.0.1"));
        message2.SetHopLimit(255);
        message2.SetHopCount(1);
        packet.Messages().emplace_back(message2);
        AddTestCase(
            new Rfc5444PacketSerializationTestCase(
                "packet-old-test-11",
                packet,
                {0x0c, 0x00, 0x0b, 0x00, 0x02, 0x01, 0x00, 0x01, 0x03, 0x00, 0x06, 0x00, 0x00,
                 0x02, 0xe3, 0x00, 0x0c, 0x0a, 0x00, 0x00, 0x01, 0xff, 0x01, 0x00, 0x00}),
            TestCase::Duration::QUICK);
    }
    {
        Rfc5444Packet packet;
        packet.SetSequenceNumber(12);
        Rfc5444Tlv tlv;
        tlv.SetType(1);
        packet.Tlvs().push_back(tlv);
        Rfc5444MessageIpv4 message1;
        message1.SetType(1);
        packet.Messages().emplace_back(message1);
        Rfc5444MessageIpv4 message2;
        message2.SetType(2);
        message2.SetOriginatorAddress(Ipv4Address("10.0.0.1"));
        message2.SetHopLimit(255);
        message2.SetHopCount(1);
        message2.SetSequenceNumber(12345);
        packet.Messages().emplace_back(message2);
        AddTestCase(
            new Rfc5444PacketSerializationTestCase(
                "packet-old-test-12",
                packet,
                {0x0c, 0x00, 0x0c, 0x00, 0x02, 0x01, 0x00, 0x01, 0x03, 0x00, 0x06, 0x00, 0x00, 0x02,
                 0xf3, 0x00, 0x0e, 0x0a, 0x00, 0x00, 0x01, 0xff, 0x01, 0x30, 0x39, 0x00, 0x00}),
            TestCase::Duration::QUICK);
    }

    /* Old tests 14-28 share a common prelude (a packet TLV, a first message
     * carrying one message TLV, and a second message using all four header
     * fields) and a common byte prefix; these helpers build both. */
    auto makePreludePacket = [](uint8_t seq) {
        Rfc5444Packet packet;
        packet.SetSequenceNumber(seq);
        Rfc5444Tlv packetTlv;
        packetTlv.SetType(1);
        packet.Tlvs().push_back(packetTlv);
        Rfc5444MessageIpv4 message1;
        message1.SetType(1);
        Rfc5444Tlv messageTlv;
        messageTlv.SetType(1);
        message1.Tlvs().push_back(messageTlv);
        packet.Messages().emplace_back(message1);
        return packet;
    };
    auto makeFullHeaderMessage = []() {
        Rfc5444MessageIpv4 message;
        message.SetType(2);
        message.SetOriginatorAddress(Ipv4Address("10.0.0.1"));
        message.SetHopLimit(255);
        message.SetHopCount(1);
        message.SetSequenceNumber(12345);
        return message;
    };
    auto makePreludeBytes =
        [](uint8_t seq, uint16_t messageSize, const std::vector<uint8_t>& blocks) {
            std::vector<uint8_t> bytes{0x0c,
                                       0x00,
                                       seq,
                                       0x00,
                                       0x02,
                                       0x01,
                                       0x00,
                                       0x01,
                                       0x03,
                                       0x00,
                                       0x08,
                                       0x00,
                                       0x02,
                                       0x01,
                                       0x00,
                                       0x02,
                                       0xf3,
                                       static_cast<uint8_t>(messageSize >> 8),
                                       static_cast<uint8_t>(messageSize & 0xff),
                                       0x0a,
                                       0x00,
                                       0x00,
                                       0x01,
                                       0xff,
                                       0x01,
                                       0x30,
                                       0x39,
                                       0x00,
                                       0x00};
            bytes.insert(bytes.end(), blocks.begin(), blocks.end());
            return bytes;
        };
    /* The two-address head/tail block of old tests 21-28 and its bytes. */
    auto makeHeadTailBlock = []() {
        Rfc5444AddressBlockIpv4 block;
        block.Addresses() = {Ipv4Address("10.0.0.2"), Ipv4Address("10.1.1.2")};
        return block;
    };
    const std::vector<uint8_t> headTailBlockBytes =
        {0x02, 0xc0, 0x01, 0x0a, 0x01, 0x02, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00};
    /* The four-prefix block of old tests 23-28 and its bytes (without the
     * trailing TLV block, which varies per test). */
    auto makeFourPrefixBlock = []() {
        Rfc5444AddressBlockIpv4 block;
        block.Addresses() = {{Ipv4Address("10.0.0.0"), 32},
                             {Ipv4Address("11.0.0.0"), 32},
                             {Ipv4Address("10.0.0.5"), 16},
                             {Ipv4Address("10.0.0.6"), 24}};
        return block;
    };
    const std::vector<uint8_t> fourPrefixBlockBytes = {
        0x04, 0x08, 0x0a, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x0a,
        0x00, 0x00, 0x05, 0x0a, 0x00, 0x00, 0x06, 0x20, 0x20, 0x10, 0x18};

    /* Prelude only, no address blocks. [old test 14] */
    {
        Rfc5444Packet packet = makePreludePacket(14);
        packet.Messages().emplace_back(makeFullHeaderMessage());
        AddTestCase(new Rfc5444PacketSerializationTestCase("packet-old-test-14",
                                                           packet,
                                                           makePreludeBytes(14, 0x0e, {})),
                    TestCase::Duration::QUICK);
    }

    /* Single-address blocks with various addresses. [old tests 15-19] */
    {
        const std::vector<std::pair<uint8_t, Ipv4Address>> singles = {
            {15, Ipv4Address("0.0.0.0")},
            {16, Ipv4Address("255.255.255.255")},
            {17, Ipv4Address("0.0.0.1")},
            {18, Ipv4Address("10.0.0.0")},
            {19, Ipv4Address("10.0.0.1")},
        };
        for (const auto& [oldNumber, address] : singles)
        {
            Rfc5444Packet packet = makePreludePacket(oldNumber);
            Rfc5444MessageIpv4 message2 = makeFullHeaderMessage();
            Rfc5444AddressBlockIpv4 block;
            block.Addresses() = {address};
            message2.AddressBlocks().push_back(block);
            packet.Messages().emplace_back(message2);

            std::vector<uint8_t> blockBytes{0x01, 0x00};
            uint8_t octets[4];
            address.Serialize(octets);
            blockBytes.insert(blockBytes.end(), octets, octets + 4);
            blockBytes.insert(blockBytes.end(), {0x00, 0x00});
            AddTestCase(new Rfc5444PacketSerializationTestCase(
                            "packet-old-test-" + std::to_string(oldNumber),
                            packet,
                            makePreludeBytes(oldNumber, 0x16, blockBytes)),
                        TestCase::Duration::QUICK);
        }
    }

    /* Two addresses sharing a head (ahashead flag). [old test 20] */
    {
        Rfc5444Packet packet = makePreludePacket(20);
        Rfc5444MessageIpv4 message2 = makeFullHeaderMessage();
        Rfc5444AddressBlockIpv4 block;
        block.Addresses() = {Ipv4Address("10.0.0.1"), Ipv4Address("10.0.0.2")};
        message2.AddressBlocks().push_back(block);
        packet.Messages().emplace_back(message2);
        AddTestCase(
            new Rfc5444PacketSerializationTestCase(
                "packet-old-test-20",
                packet,
                makePreludeBytes(20,
                                 0x18,
                                 {0x02, 0x80, 0x03, 0x0a, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00})),
            TestCase::Duration::QUICK);
    }

    /* Two addresses sharing a head and a tail. [old test 21] */
    {
        Rfc5444Packet packet = makePreludePacket(21);
        Rfc5444MessageIpv4 message2 = makeFullHeaderMessage();
        message2.AddressBlocks().push_back(makeHeadTailBlock());
        packet.Messages().emplace_back(message2);
        AddTestCase(
            new Rfc5444PacketSerializationTestCase("packet-old-test-21",
                                                   packet,
                                                   makePreludeBytes(21, 0x1a, headTailBlockBytes)),
            TestCase::Duration::QUICK);
    }

    /* A second block with an all-zero tail (ahaszerotail flag). [old test 22] */
    {
        Rfc5444Packet packet = makePreludePacket(22);
        Rfc5444MessageIpv4 message2 = makeFullHeaderMessage();
        message2.AddressBlocks().push_back(makeHeadTailBlock());
        Rfc5444AddressBlockIpv4 block2;
        block2.Addresses() = {Ipv4Address("10.0.0.0"), Ipv4Address("11.0.0.0")};
        message2.AddressBlocks().push_back(block2);
        packet.Messages().emplace_back(message2);
        std::vector<uint8_t> blocks = headTailBlockBytes;
        blocks.insert(blocks.end(), {0x02, 0x20, 0x03, 0x0a, 0x0b, 0x00, 0x00});
        AddTestCase(new Rfc5444PacketSerializationTestCase("packet-old-test-22",
                                                           packet,
                                                           makePreludeBytes(22, 0x21, blocks)),
                    TestCase::Duration::QUICK);
    }

    /* The four-prefix block with a varying address TLV block.
     * [old tests 23-27] */
    {
        struct FourPrefixCase
        {
            uint8_t oldNumber;                   //!< Old test number
            uint16_t messageSize;                //!< msg-size of the second message
            std::vector<uint8_t> tlvBlockBytes;  //!< Address TLV block bytes
            std::vector<Rfc5444AddressTlv> tlvs; //!< Address TLVs
        };

        Rfc5444AddressTlv plainTlv;
        plainTlv.SetType(1);
        Rfc5444AddressTlv singleIndexTlv = plainTlv;
        singleIndexTlv.SetIndexRange(1, 1);
        Rfc5444AddressTlv multiIndexTlv = plainTlv;
        multiIndexTlv.SetIndexRange(1, 3);
        Rfc5444AddressTlv multivalueTlv;
        multivalueTlv.SetType(1);
        multivalueTlv.SetValues(1, {{1}, {2}, {3}});

        const std::vector<FourPrefixCase> cases = {
            {23, 0x32, {0x00, 0x00}, {}},
            {24, 0x34, {0x00, 0x02, 0x01, 0x00}, {plainTlv}},
            {25, 0x35, {0x00, 0x03, 0x01, 0x40, 0x01}, {singleIndexTlv}},
            {26, 0x36, {0x00, 0x04, 0x01, 0x20, 0x01, 0x03}, {multiIndexTlv}},
            {27,
             0x3a,
             {0x00, 0x08, 0x01, 0x34, 0x01, 0x03, 0x03, 0x01, 0x02, 0x03},
             {multivalueTlv}},
        };
        for (const auto& c : cases)
        {
            Rfc5444Packet packet = makePreludePacket(c.oldNumber);
            Rfc5444MessageIpv4 message2 = makeFullHeaderMessage();
            message2.AddressBlocks().push_back(makeHeadTailBlock());
            Rfc5444AddressBlockIpv4 block2 = makeFourPrefixBlock();
            block2.Tlvs() = c.tlvs;
            message2.AddressBlocks().push_back(block2);
            packet.Messages().emplace_back(message2);

            std::vector<uint8_t> blocks = headTailBlockBytes;
            blocks.insert(blocks.end(), fourPrefixBlockBytes.begin(), fourPrefixBlockBytes.end());
            blocks.insert(blocks.end(), c.tlvBlockBytes.begin(), c.tlvBlockBytes.end());
            AddTestCase(new Rfc5444PacketSerializationTestCase(
                            "packet-old-test-" + std::to_string(c.oldNumber),
                            packet,
                            makePreludeBytes(c.oldNumber, c.messageSize, blocks)),
                        TestCase::Duration::QUICK);
        }
    }

    /* The second message of old tests 28, 36, and 37: the four-prefix block
     * carries a ranged TLV with the 300-octet extended-length value. */
    auto makeLongTlvMessage = [&]() {
        Rfc5444MessageIpv4 message = makeFullHeaderMessage();
        message.AddressBlocks().push_back(makeHeadTailBlock());
        Rfc5444AddressBlockIpv4 block = makeFourPrefixBlock();
        Rfc5444AddressTlv tlv;
        tlv.SetType(1);
        tlv.SetIndexRange(1, 3);
        tlv.SetValue(longValue);
        block.Tlvs().push_back(tlv);
        message.AddressBlocks().push_back(block);
        return message;
    };
    std::vector<uint8_t> longTlvMessageBytes = {
        0x02, 0xf3, 0x01, 0x64, 0x0a, 0x00, 0x00, 0x01, 0xff, 0x01, 0x30, 0x39, 0x00, 0x00,
        0x02, 0xc0, 0x01, 0x0a, 0x01, 0x02, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x04, 0x08,
        0x0a, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x05, 0x0a, 0x00,
        0x00, 0x06, 0x20, 0x20, 0x10, 0x18, 0x01, 0x32, 0x01, 0x38, 0x01, 0x03, 0x01, 0x2c};
    longTlvMessageBytes.insert(longTlvMessageBytes.end(), longValue.begin(), longValue.end());

    /* [old test 28] */
    {
        Rfc5444Packet packet = makePreludePacket(28);
        packet.Messages().emplace_back(makeLongTlvMessage());
        std::vector<uint8_t> bytes{0x0c,
                                   0x00,
                                   0x1c,
                                   0x00,
                                   0x02,
                                   0x01,
                                   0x00,
                                   0x01,
                                   0x03,
                                   0x00,
                                   0x08,
                                   0x00,
                                   0x02,
                                   0x01,
                                   0x00};
        bytes.insert(bytes.end(), longTlvMessageBytes.begin(), longTlvMessageBytes.end());
        AddTestCase(new Rfc5444PacketSerializationTestCase("packet-old-test-28", packet, bytes),
                    TestCase::Duration::QUICK);
    }

    /* IPv6 messages. [old tests 29-34] */
    {
        Rfc5444Packet packet;
        Rfc5444MessageIpv6 message;
        message.SetType(1);
        packet.Messages().emplace_back(message);
        AddTestCase(
            new Rfc5444PacketSerializationTestCase("packet-old-test-29",
                                                   packet,
                                                   {0x00, 0x01, 0x0f, 0x00, 0x06, 0x00, 0x00}),
            TestCase::Duration::QUICK);
    }
    {
        Rfc5444Packet packet;
        Rfc5444MessageIpv6 message;
        message.SetType(1);
        message.SetOriginatorAddress(Ipv6Address("abcd::1"));
        packet.Messages().emplace_back(message);
        AddTestCase(new Rfc5444PacketSerializationTestCase(
                        "packet-old-test-30",
                        packet,
                        {0x00, 0x01, 0x8f, 0x00, 0x16, 0xab, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00,
                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00}),
                    TestCase::Duration::QUICK);
    }
    {
        Rfc5444Packet packet;
        Rfc5444MessageIpv6 message;
        message.SetType(1);
        message.SetOriginatorAddress(Ipv6Address("abcd::1"));
        Rfc5444AddressBlockIpv6 block;
        block.Addresses() = {Ipv6Address("10::1")};
        message.AddressBlocks().push_back(block);
        packet.Messages().emplace_back(message);
        AddTestCase(new Rfc5444PacketSerializationTestCase(
                        "packet-old-test-31",
                        packet,
                        {0x00, 0x01, 0x8f, 0x00, 0x2a, 0xab, 0xcd, 0x00, 0x00, 0x00, 0x00,
                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
                         0x00, 0x01, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00}),
                    TestCase::Duration::QUICK);
    }
    {
        Rfc5444Packet packet;
        Rfc5444MessageIpv6 message;
        message.SetType(1);
        message.SetOriginatorAddress(Ipv6Address("abcd::1"));
        Rfc5444AddressBlockIpv6 block;
        block.Addresses() = {Ipv6Address("10::1"), Ipv6Address("10::2")};
        message.AddressBlocks().push_back(block);
        packet.Messages().emplace_back(message);
        AddTestCase(new Rfc5444PacketSerializationTestCase(
                        "packet-old-test-32",
                        packet,
                        {0x00, 0x01, 0x8f, 0x00, 0x2c, 0xab, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00,
                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x02,
                         0x80, 0x0f, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                         0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00}),
                    TestCase::Duration::QUICK);
    }
    {
        Rfc5444Packet packet;
        Rfc5444MessageIpv6 message;
        message.SetType(1);
        message.SetOriginatorAddress(Ipv6Address("abcd::1"));
        Rfc5444AddressBlockIpv6 block;
        block.Addresses() = {Ipv6Address("10::2"), Ipv6Address("10::11:2")};
        message.AddressBlocks().push_back(block);
        packet.Messages().emplace_back(message);
        AddTestCase(new Rfc5444PacketSerializationTestCase(
                        "packet-old-test-33",
                        packet,
                        {0x00, 0x01, 0x8f, 0x00, 0x2d, 0xab, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00,
                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x02,
                         0xc0, 0x0d, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                         0x00, 0x00, 0x00, 0x02, 0x00, 0x02, 0x00, 0x11, 0x00, 0x00}),
                    TestCase::Duration::QUICK);
    }
    {
        Rfc5444Packet packet;
        Rfc5444MessageIpv6 message;
        message.SetType(1);
        message.SetOriginatorAddress(Ipv6Address("abcd::1"));
        Rfc5444AddressBlockIpv6 block1;
        block1.Addresses() = {Ipv6Address("10::2"), Ipv6Address("10::11:2")};
        message.AddressBlocks().push_back(block1);
        Rfc5444AddressBlockIpv6 block2;
        block2.Addresses() = {Ipv6Address("10::"), Ipv6Address("11::")};
        message.AddressBlocks().push_back(block2);
        packet.Messages().emplace_back(message);
        AddTestCase(
            new Rfc5444PacketSerializationTestCase(
                "packet-old-test-34",
                packet,
                {0x00, 0x01, 0x8f, 0x00, 0x36, 0xab, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x02, 0xc0, 0x0d, 0x00, 0x10,
                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x02,
                 0x00, 0x11, 0x00, 0x00, 0x02, 0xa0, 0x01, 0x00, 0x0e, 0x10, 0x11, 0x00, 0x00}),
            TestCase::Duration::QUICK);
    }

    /* The IPv6 message of old tests 35-37 and its serialized form. */
    auto makeIpv6AddressMessage = []() {
        Rfc5444MessageIpv6 message;
        message.SetType(1);
        message.SetOriginatorAddress(Ipv6Address("abcd::1"));
        Rfc5444AddressBlockIpv6 block1;
        block1.Addresses() = {Ipv6Address("10::2"), Ipv6Address("10::11:2")};
        message.AddressBlocks().push_back(block1);
        Rfc5444AddressBlockIpv6 block2;
        block2.Addresses() = {{Ipv6Address("10::"), 128},
                              {Ipv6Address("11::"), 128},
                              {Ipv6Address("10::5"), 64},
                              {Ipv6Address("10::6"), 48}};
        message.AddressBlocks().push_back(block2);
        return message;
    };
    const std::vector<uint8_t> ipv6MessageBytes = {
        0x01, 0x8f, 0x00, 0x73, 0xab, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x02, 0xc0, 0x0d, 0x00, 0x10, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x02, 0x00, 0x11, 0x00, 0x00,
        0x04, 0x88, 0x01, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x05, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x06, 0x80, 0x80, 0x40, 0x30, 0x00, 0x00};

    /* [old test 35] */
    {
        Rfc5444Packet packet;
        packet.Messages().emplace_back(makeIpv6AddressMessage());
        std::vector<uint8_t> bytes{0x00};
        bytes.insert(bytes.end(), ipv6MessageBytes.begin(), ipv6MessageBytes.end());
        AddTestCase(new Rfc5444PacketSerializationTestCase("packet-old-test-35", packet, bytes),
                    TestCase::Duration::QUICK);
    }

    /* Packets mixing IPv6 and IPv4 messages. [old tests 36 and 37] */
    for (const auto& [oldNumber, seq] :
         std::vector<std::pair<std::string, uint8_t>>{{"36", 29}, {"37", 30}})
    {
        Rfc5444Packet packet;
        packet.SetSequenceNumber(seq);
        Rfc5444Tlv packetTlv;
        packetTlv.SetType(1);
        packet.Tlvs().push_back(packetTlv);
        Rfc5444MessageIpv6 message1;
        message1.SetType(1);
        Rfc5444Tlv messageTlv;
        messageTlv.SetType(1);
        message1.Tlvs().push_back(messageTlv);
        packet.Messages().emplace_back(message1);
        packet.Messages().emplace_back(makeLongTlvMessage());
        packet.Messages().emplace_back(makeIpv6AddressMessage());

        std::vector<uint8_t> bytes{0x0c,
                                   0x00,
                                   seq,
                                   0x00,
                                   0x02,
                                   0x01,
                                   0x00,
                                   0x01,
                                   0x0f,
                                   0x00,
                                   0x08,
                                   0x00,
                                   0x02,
                                   0x01,
                                   0x00};
        bytes.insert(bytes.end(), longTlvMessageBytes.begin(), longTlvMessageBytes.end());
        bytes.insert(bytes.end(), ipv6MessageBytes.begin(), ipv6MessageBytes.end());
        AddTestCase(
            new Rfc5444PacketSerializationTestCase("packet-old-test-" + oldNumber, packet, bytes),
            TestCase::Duration::QUICK);
    }

    AddTestCase(new Rfc5444TlvValueAccessorTestCase(), TestCase::Duration::QUICK);
    AddTestCase(new Rfc5444AddressTlvValueForIndexTestCase(), TestCase::Duration::QUICK);
}

static Rfc5444TestSuite g_rfc5444TestSuite; //!< Static variable for test initialization
