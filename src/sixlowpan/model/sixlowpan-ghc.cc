/*
 * Copyright (c) 2026 SRM Institute of Science and Technology
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Usham Roy <ushamroy80@gmail.com>
 *
 * 6LoWPAN-GHC: Generic Header Compression - RFC 7400
 */

#include "sixlowpan-ghc.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/log.h"

#include <algorithm>
#include <cstring>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SixLowPanGhc");

// ============================================================================
//  Static Dictionary (RFC 7400 Section 3.1)
// ============================================================================

const uint8_t SixLowPanGhcEngine::STATIC_DICTIONARY[16] = {0x16,
                                                           0xfe,
                                                           0xfd,
                                                           0x17,
                                                           0xfe,
                                                           0xfd,
                                                           0x00,
                                                           0x01,
                                                           0x00,
                                                           0x00,
                                                           0x00,
                                                           0x00,
                                                           0x00,
                                                           0x01,
                                                           0x00,
                                                           0x00};

// ============================================================================
//  GHC Compression/Decompression Engine
// ============================================================================

void
SixLowPanGhcEngine::InitDictionary(uint8_t* dict,
                                   const Ipv6Address& srcAddr,
                                   const Ipv6Address& dstAddr)
{
    // Bytes 0-15: Source IPv6 address
    uint8_t srcBuf[16];
    srcAddr.GetBytes(srcBuf);
    std::memcpy(dict, srcBuf, 16);

    // Bytes 16-31: Destination IPv6 address
    uint8_t dstBuf[16];
    dstAddr.GetBytes(dstBuf);
    std::memcpy(dict + 16, dstBuf, 16);

    // Bytes 32-47: Static dictionary
    std::memcpy(dict + 32, STATIC_DICTIONARY, 16);
}

GhcBytecodeType
SixLowPanGhcEngine::ClassifyBytecode(uint8_t byte)
{
    if ((byte & 0x80) == 0)
    {
        // 0kkkkkkk - Literal copy
        return GhcBytecodeType::LITERAL;
    }
    else if ((byte & 0xF0) == 0x80)
    {
        // 1000nnnn - Zero insertion (nnnn > 0) or reserved
        if (byte == 0x80)
        {
            // 10000000 is reserved (n=0 would mean insert 2 zeros but spec
            // says n=0 in this pattern is reserved - actually per RFC section 3
            // "1000nnnn" with n ranging 0-15 inserts n+2 zeros.
            // Re-reading RFC: 10000000 inserts 0+2 = 2 zeros. Valid.
            return GhcBytecodeType::ZERO_INSERT;
        }
        return GhcBytecodeType::ZERO_INSERT;
    }
    else if (byte == 0x90)
    {
        // 10010000 - Stop code
        return GhcBytecodeType::STOP_CODE;
    }
    else if ((byte & 0xF0) == 0x90 && byte != 0x90)
    {
        // 1001nnnn where nnnn > 0 - Reserved
        return GhcBytecodeType::RESERVED;
    }
    else if ((byte & 0xE0) == 0xA0)
    {
        // 101nssss - Extended arguments
        return GhcBytecodeType::EXTENDED_ARGS;
    }
    else if ((byte & 0xC0) == 0xC0)
    {
        // 11nnnkkk - Backreference
        return GhcBytecodeType::BACKREF;
    }
    else
    {
        // 011xxxxx - Reserved
        return GhcBytecodeType::RESERVED;
    }
}

uint32_t
SixLowPanGhcEngine::Decompress(const Ipv6Address& srcAddr,
                               const Ipv6Address& dstAddr,
                               const uint8_t* compressed,
                               uint32_t compressedLen,
                               uint8_t* output,
                               uint32_t outputMaxLen,
                               bool useStopCode)
{
    NS_LOG_FUNCTION_NOARGS();

    // Allocate decompression buffer: dictionary + output space
    // Maximum buffer = dictionary(48) + MTU(1280)
    static constexpr uint32_t BUFFER_SIZE = DICTIONARY_SIZE + MAX_OUTPUT_SIZE;
    uint8_t buffer[BUFFER_SIZE];

    // Initialize dictionary in buffer[0..47]
    InitDictionary(buffer, srcAddr, dstAddr);

    uint32_t outputPos = DICTIONARY_SIZE; // Current write position in buffer
    uint32_t inputPos = 0;                // Current read position in compressed
    uint32_t sa = 0;                      // Start adjust accumulator
    uint32_t na = 0;                      // Number adjust accumulator

    while (inputPos < compressedLen)
    {
        uint8_t codeByte = compressed[inputPos++];
        GhcBytecodeType type = ClassifyBytecode(codeByte);

        switch (type)
        {
        case GhcBytecodeType::LITERAL: {
            // 0kkkkkkk: copy k literal bytes from bytecode stream
            uint32_t k = codeByte & 0x7F;

            if (k == 0)
            {
                // No-op
                break;
            }
            if (k >= 96)
            {
                NS_LOG_WARN("GHC: Literal count k=" << k << " >= 96, reserved");
                return 0;
            }
            if (inputPos + k > compressedLen)
            {
                NS_LOG_WARN("GHC: Literal overruns compressed data");
                return 0;
            }
            if (outputPos + k > BUFFER_SIZE)
            {
                NS_LOG_WARN("GHC: Literal would exceed buffer");
                return 0;
            }

            std::memcpy(buffer + outputPos, compressed + inputPos, k);
            outputPos += k;
            inputPos += k;
            break;
        }

        case GhcBytecodeType::ZERO_INSERT: {
            // 1000nnnn: insert (n+2) zero bytes
            uint32_t n = (codeByte & 0x0F) + 2;

            if (outputPos + n > BUFFER_SIZE)
            {
                NS_LOG_WARN("GHC: Zero insert would exceed buffer");
                return 0;
            }

            std::memset(buffer + outputPos, 0, n);
            outputPos += n;
            break;
        }

        case GhcBytecodeType::STOP_CODE: {
            // 10010000: terminate decompression (extension headers only)
            if (useStopCode)
            {
                NS_LOG_DEBUG("GHC: Stop code encountered");
                goto done;
            }
            else
            {
                NS_LOG_WARN("GHC: Unexpected stop code in non-extension context");
                return 0;
            }
        }

        case GhcBytecodeType::EXTENDED_ARGS: {
            // 101nssss: sa += ssss << 3, na += n << 3
            //   bit layout: 101[n][ssss]
            //   n  = bit 4 (one bit)
            //   ssss = bits 0-3
            uint32_t ssss = codeByte & 0x0F;
            uint32_t nBit = (codeByte >> 4) & 0x01;

            sa += (ssss << 3);
            na += (nBit << 3);

            NS_LOG_DEBUG("GHC: ExtArgs sa=" << sa << " na=" << na);
            break;
        }

        case GhcBytecodeType::BACKREF: {
            // 11nnnkkk: copy n bytes from s bytes back
            //   n = na + (nnn) + 2
            //   s = (kkk) + sa + n
            uint32_t nnn = (codeByte >> 3) & 0x07;
            uint32_t kkk = codeByte & 0x07;

            uint32_t copyLen = na + nnn + 2;
            uint32_t offset = kkk + sa + copyLen;

            // Reset extended arg accumulators
            sa = 0;
            na = 0;

            if (offset > outputPos)
            {
                NS_LOG_WARN("GHC: Backref offset " << offset << " exceeds buffer position "
                                                   << outputPos);
                return 0;
            }
            if (outputPos + copyLen > BUFFER_SIZE)
            {
                NS_LOG_WARN("GHC: Backref would exceed buffer");
                return 0;
            }

            // Copy byte-by-byte (overlapping copies are valid in LZ77)
            uint32_t srcPos = outputPos - offset;
            for (uint32_t i = 0; i < copyLen; i++)
            {
                buffer[outputPos + i] = buffer[srcPos + i];
            }
            outputPos += copyLen;
            break;
        }

        case GhcBytecodeType::RESERVED:
            NS_LOG_WARN("GHC: Reserved bytecode 0x" << std::hex << (uint32_t)codeByte);
            return 0;
        }
    }

done:
    // Output is everything after the dictionary
    uint32_t decompressedLen = outputPos - DICTIONARY_SIZE;

    if (decompressedLen > outputMaxLen)
    {
        NS_LOG_WARN("GHC: Decompressed size " << decompressedLen << " exceeds output buffer "
                                              << outputMaxLen);
        return 0;
    }

    std::memcpy(output, buffer + DICTIONARY_SIZE, decompressedLen);

    NS_LOG_DEBUG("GHC: Decompressed " << compressedLen << " bytes to " << decompressedLen
                                      << " bytes");

    return decompressedLen;
}

uint32_t
SixLowPanGhcEngine::CountZeros(const uint8_t* input, uint32_t remaining)
{
    uint32_t count = 0;
    while (count < remaining && input[count] == 0)
    {
        count++;
    }
    return count;
}

bool
SixLowPanGhcEngine::FindLongestMatch(const uint8_t* buffer,
                                     uint32_t bufLen,
                                     const uint8_t* input,
                                     uint32_t inputRemaining,
                                     uint32_t& matchOffset,
                                     uint32_t& matchLength)
{
    matchOffset = 0;
    matchLength = 0;

    // Maximum copy length with extended args: na_max(8) + nnn_max(7) + 2 = 17
    // But we can have multiple extended args: each adds up to 8.
    // Practical limit: keep it reasonable for encoding cost vs benefit.
    // Without extended args: max copyLen = 7 + 2 = 9, max offset = 7 + 9 = 16
    // With one extended arg: max copyLen = 8+7+2 = 17, max offset = 120+8+17 = 145

    // Maximum window to search backward
    // With extended args: sa can be at most 15*8 = 120 per byte, plus kkk=7, plus n
    // Practical: search back up to 256 bytes (covers dictionary and recent output)
    uint32_t maxSearchBack = std::min(bufLen, (uint32_t)256);

    // Maximum match length we'll encode (keep encoding overhead reasonable)
    uint32_t maxMatchLen = std::min(inputRemaining, (uint32_t)17);

    for (uint32_t back = 2; back <= maxSearchBack; back++)
    {
        uint32_t searchPos = bufLen - back;
        uint32_t len = 0;

        while (len < maxMatchLen && buffer[searchPos + len] == input[len])
        {
            len++;
        }

        if (len >= 2 && len > matchLength)
        {
            matchLength = len;
            matchOffset = back;

            if (matchLength == maxMatchLen)
            {
                break; // Can't do better
            }
        }
    }

    return (matchLength >= 2);
}

bool
SixLowPanGhcEngine::EmitBackref(uint8_t* output,
                                uint32_t& outPos,
                                uint32_t outputMaxLen,
                                uint32_t matchLength,
                                uint32_t matchOffset)
{
    NS_ASSERT(matchLength >= 2);
    // GHC bytecode 11nnnkkk requires offset >= length;
    // caller (Compress) clamps overlapping matches before calling us.

    // Backref encoding: 11nnnkkk
    //   n = na + nnn + 2  =>  nnn = n - na - 2
    //   s = kkk + sa + n  =>  kkk = s - sa - n  (where s = matchOffset)
    //
    // We need: nnn in [0,7], kkk in [0,7]
    // If they don't fit, we need Extended Args (101nssss) to add to sa/na.

    uint32_t n = matchLength; // Total copy length
    uint32_t s = matchOffset; // Total offset

    // Determine how much sa and na we need
    // nnn = n - na - 2, must be in [0,7]  =>  na = n - 2 - nnn
    // kkk = s - sa - n, must be in [0,7]  =>  sa = s - n - kkk

    // First try without extended args
    uint32_t nBase = n - 2; // Need nnn = nBase - na
    uint32_t kBase = s - n; // Need kkk = kBase - sa

    uint32_t na = 0;
    uint32_t sa = 0;

    // Calculate required na (to make nnn fit in [0,7])
    if (nBase > 7)
    {
        na = nBase - 7; // Minimum na to make nnn <= 7
        // na must be a multiple of 8 (since extended args add in units of 8)
        na = ((na + 7) / 8) * 8;
    }

    // Calculate required sa (to make kkk fit in [0,7])
    if (kBase > 7)
    {
        sa = kBase - 7;
        sa = ((sa + 7) / 8) * 8;
    }

    // Verify encoding is feasible
    uint32_t nnn = nBase - na;
    uint32_t kkk = kBase - sa;

    if (nnn > 7 || kkk > 7)
    {
        NS_LOG_WARN("GHC: Cannot encode backref len=" << n << " offset=" << s);
        return false;
    }

    // Emit extended argument bytes for sa and na
    uint32_t extArgBytes = 0;

    // We need to emit extended arg bytes to build up sa and na
    uint32_t remainingSa = sa;
    uint32_t remainingNa = na;

    // Count how many extended arg bytes we need
    while (remainingSa > 0 || remainingNa > 0)
    {
        extArgBytes++;
        uint32_t sChunk = std::min(remainingSa, (uint32_t)(15 * 8));
        uint32_t nChunk = std::min(remainingNa, (uint32_t)8);
        remainingSa -= std::min(sChunk, remainingSa);
        remainingNa -= std::min(nChunk, remainingNa);
    }

    // Check output space: extArgBytes + 1 (backref byte)
    if (outPos + extArgBytes + 1 > outputMaxLen)
    {
        return false;
    }

    // Emit extended args
    remainingSa = sa;
    remainingNa = na;

    while (remainingSa > 0 || remainingNa > 0)
    {
        // Extended arg: 101[n][ssss]
        //   adds ssss*8 to sa, n*8 to na
        uint32_t ssss = std::min(remainingSa / 8, (uint32_t)15);
        uint32_t nBit = (remainingNa >= 8) ? 1 : 0;

        uint8_t extByte = 0xA0 | (nBit << 4) | (ssss & 0x0F);
        output[outPos++] = extByte;

        remainingSa -= ssss * 8;
        if (nBit)
        {
            remainingNa -= 8;
        }
    }

    // Emit backref: 11nnnkkk
    uint8_t backrefByte = 0xC0 | ((nnn & 0x07) << 3) | (kkk & 0x07);
    output[outPos++] = backrefByte;

    return true;
}

uint32_t
SixLowPanGhcEngine::Compress(const Ipv6Address& srcAddr,
                             const Ipv6Address& dstAddr,
                             const uint8_t* input,
                             uint32_t inputLen,
                             uint8_t* output,
                             uint32_t outputMaxLen,
                             bool emitStopCode)
{
    NS_LOG_FUNCTION_NOARGS();

    if (inputLen == 0)
    {
        return 0;
    }

    // Build the working buffer with dictionary pre-loaded
    static constexpr uint32_t BUFFER_SIZE = DICTIONARY_SIZE + MAX_OUTPUT_SIZE;
    uint8_t buffer[BUFFER_SIZE];
    InitDictionary(buffer, srcAddr, dstAddr);

    uint32_t bufPos = DICTIONARY_SIZE; // Next write position in buffer
    uint32_t inPos = 0;                // Current position in input
    uint32_t outPos = 0;               // Current position in output

    // Accumulate literal bytes to emit as a group
    std::vector<uint8_t> literalBuf;

    // Lambda to flush accumulated literals
    auto flushLiterals = [&]() -> bool {
        while (!literalBuf.empty())
        {
            uint32_t chunk = std::min((uint32_t)literalBuf.size(), (uint32_t)95);

            // Need 1 byte (count) + chunk bytes (data)
            if (outPos + 1 + chunk > outputMaxLen)
            {
                return false;
            }

            output[outPos++] = static_cast<uint8_t>(chunk); // 0kkkkkkk where k < 96
            std::memcpy(output + outPos, literalBuf.data(), chunk);
            outPos += chunk;

            literalBuf.erase(literalBuf.begin(), literalBuf.begin() + chunk);
        }
        return true;
    };

    while (inPos < inputLen)
    {
        // Strategy 1: Check for zero runs
        uint32_t zeros = CountZeros(input + inPos, inputLen - inPos);
        if (zeros >= 2)
        {
            // Flush any pending literals first
            if (!flushLiterals())
            {
                return 0;
            }

            // Emit zero insertion instructions
            while (zeros >= 2)
            {
                uint32_t emit = std::min(zeros, (uint32_t)17); // Max per instruction
                if (outPos + 1 > outputMaxLen)
                {
                    return 0;
                }

                // 1000nnnn where n = emit - 2
                output[outPos++] = 0x80 | ((emit - 2) & 0x0F);

                // Also advance the buffer (for future backreferences)
                std::memset(buffer + bufPos, 0, emit);
                bufPos += emit;
                inPos += emit;
                zeros -= emit;
            }
            continue;
        }

        // Strategy 2: Check for backreference match in dictionary + output
        uint32_t matchOffset = 0;
        uint32_t matchLength = 0;

        if (FindLongestMatch(buffer,
                             bufPos,
                             input + inPos,
                             inputLen - inPos,
                             matchOffset,
                             matchLength))
        {
            // GHC backreference encoding requires offset >= length.
            // Clamp overlapping matches from the LZ77 match finder.
            if (matchOffset < matchLength)
            {
                matchLength = matchOffset;
            }
            if (matchLength < 2)
            {
                // Too short after clamping — treat current byte as literal
                literalBuf.push_back(input[inPos]);
                buffer[bufPos++] = input[inPos];
                inPos++;
                continue;
            }
            // Flush any pending literals first
            if (!flushLiterals())
            {
                return 0;
            }

            // Try to emit backref
            uint32_t savedOutPos = outPos;
            if (EmitBackref(output, outPos, outputMaxLen, matchLength, matchOffset))
            {
                // Advance buffer with matched data
                std::memcpy(buffer + bufPos, input + inPos, matchLength);
                bufPos += matchLength;
                inPos += matchLength;
                continue;
            }
            else
            {
                // Backref encoding failed, fall through to literal
                outPos = savedOutPos;
            }
        }

        // Strategy 3: Accumulate as literal
        literalBuf.push_back(input[inPos]);
        buffer[bufPos++] = input[inPos];
        inPos++;

        // Check buffer overflow
        if (bufPos >= BUFFER_SIZE)
        {
            NS_LOG_WARN("GHC: Compression buffer overflow");
            return 0;
        }
    }

    // Flush remaining literals
    if (!flushLiterals())
    {
        return 0;
    }

    // Emit stop code if requested (for extension headers)
    if (emitStopCode)
    {
        if (outPos + 1 > outputMaxLen)
        {
            return 0;
        }
        output[outPos++] = 0x90; // Stop code
    }

    // Only return compressed data if it's actually smaller
    uint32_t overhead = emitStopCode ? 1 : 0;
    if (outPos >= inputLen + overhead)
    {
        NS_LOG_DEBUG("GHC: No compression benefit (" << outPos << " >= " << inputLen << ")");
        return 0; // No compression benefit
    }

    NS_LOG_DEBUG("GHC: Compressed " << inputLen << " bytes to " << outPos << " bytes (ratio "
                                    << (float)inputLen / outPos << "x)");

    return outPos;
}

// ============================================================================
//  SixLowPanGhcExtension Implementation
// ============================================================================

NS_OBJECT_ENSURE_REGISTERED(SixLowPanGhcExtension);

SixLowPanGhcExtension::SixLowPanGhcExtension()
    : m_nhcByte(GHC_EXT_BASE),
      m_nextHeader(0),
      m_blobLength(0)
{
    std::memset(m_blob, 0, sizeof(m_blob));
}

TypeId
SixLowPanGhcExtension::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SixLowPanGhcExtension")
                            .SetParent<Header>()
                            .SetGroupName("SixLowPan")
                            .AddConstructor<SixLowPanGhcExtension>();
    return tid;
}

TypeId
SixLowPanGhcExtension::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
SixLowPanGhcExtension::Print(std::ostream& os) const
{
    os << "GHC Ext Header: EID=" << (uint32_t)GetEid() << " NH=" << GetNh() << " blob["
       << (uint32_t)m_blobLength << "]";
}

uint32_t
SixLowPanGhcExtension::GetSerializedSize() const
{
    // 1 byte NHC + optional 1 byte Next Header + blob length
    uint32_t size = 1; // NHC byte
    if (!GetNh())
    {
        size += 1; // Next header inline
    }
    size += m_blobLength;
    return size;
}

void
SixLowPanGhcExtension::Serialize(Buffer::Iterator start) const
{
    start.WriteU8(m_nhcByte);
    if (!GetNh())
    {
        start.WriteU8(m_nextHeader);
    }
    start.Write(m_blob, m_blobLength);
}

uint32_t
SixLowPanGhcExtension::Deserialize(Buffer::Iterator start)
{
    m_nhcByte = start.ReadU8();
    if (!GetNh())
    {
        m_nextHeader = start.ReadU8();
    }

    // For GHC extension headers, we need to scan for Stop Code to find blob end.
    // The deserializer must read bytecodes until Stop Code is found.
    m_blobLength = 0;
    while (start.GetRemainingSize() > 0 && m_blobLength < 255)
    {
        uint8_t byte = start.ReadU8();
        m_blob[m_blobLength++] = byte;

        if (byte == 0x90) // Stop code
        {
            break;
        }

        // If it's a literal (0kkkkkkk), skip k bytes
        if ((byte & 0x80) == 0)
        {
            uint32_t k = byte & 0x7F;
            for (uint32_t i = 0; i < k && start.GetRemainingSize() > 0 && m_blobLength < 255; i++)
            {
                m_blob[m_blobLength++] = start.ReadU8();
            }
        }
    }

    return GetSerializedSize();
}

SixLowPanDispatch::NhcDispatch_e
SixLowPanGhcExtension::GetNhcDispatchType() const
{
    return SixLowPanDispatch::LOWPAN_GHC_EXT;
}

void
SixLowPanGhcExtension::SetEid(Eid_e eid)
{
    m_nhcByte = GHC_EXT_BASE | ((static_cast<uint8_t>(eid) & 0x07) << 1) | (m_nhcByte & NH_MASK);
}

SixLowPanGhcExtension::Eid_e
SixLowPanGhcExtension::GetEid() const
{
    return static_cast<Eid_e>((m_nhcByte & EID_MASK) >> 1);
}

void
SixLowPanGhcExtension::SetNextHeader(uint8_t nextHeader)
{
    m_nextHeader = nextHeader;
}

uint8_t
SixLowPanGhcExtension::GetNextHeader() const
{
    return m_nextHeader;
}

void
SixLowPanGhcExtension::SetNh(bool nhField)
{
    if (nhField)
    {
        m_nhcByte |= NH_MASK;
    }
    else
    {
        m_nhcByte &= ~NH_MASK;
    }
}

bool
SixLowPanGhcExtension::GetNh() const
{
    return (m_nhcByte & NH_MASK) != 0;
}

void
SixLowPanGhcExtension::SetBlob(const uint8_t* blob, uint32_t size)
{
    NS_ASSERT(size <= 255);
    m_blobLength = size;
    std::memcpy(m_blob, blob, size);
}

uint32_t
SixLowPanGhcExtension::CopyBlob(uint8_t* blob, uint32_t size) const
{
    uint32_t copyLen = std::min((uint32_t)m_blobLength, size);
    std::memcpy(blob, m_blob, copyLen);
    return copyLen;
}

uint32_t
SixLowPanGhcExtension::GetBlobLength() const
{
    return m_blobLength;
}

std::ostream&
operator<<(std::ostream& os, const SixLowPanGhcExtension& header)
{
    header.Print(os);
    return os;
}

// ============================================================================
//  SixLowPanGhcUdp Implementation
// ============================================================================

NS_OBJECT_ENSURE_REGISTERED(SixLowPanGhcUdp);

SixLowPanGhcUdp::SixLowPanGhcUdp()
    : m_baseFormat(GHC_UDP_BASE),
      m_checksum(0),
      m_srcPort(0),
      m_dstPort(0)
{
}

TypeId
SixLowPanGhcUdp::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SixLowPanGhcUdp")
                            .SetParent<Header>()
                            .SetGroupName("SixLowPan")
                            .AddConstructor<SixLowPanGhcUdp>();
    return tid;
}

TypeId
SixLowPanGhcUdp::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
SixLowPanGhcUdp::Print(std::ostream& os) const
{
    os << "GHC UDP: src=" << m_srcPort << " dst=" << m_dstPort << " C=" << GetC()
       << " P=" << (uint32_t)GetPorts();
}

uint32_t
SixLowPanGhcUdp::GetSerializedSize() const
{
    uint32_t size = 1; // NHC byte

    switch (GetPorts())
    {
    case PORTS_INLINE:
        size += 4; // 2 + 2
        break;
    case PORTS_ALL_SRC_LAST_DST:
        size += 3; // 2 + 1
        break;
    case PORTS_LAST_SRC_ALL_DST:
        size += 3; // 1 + 2
        break;
    case PORTS_LAST_SRC_LAST_DST:
        size += 1; // 4-bit + 4-bit packed
        break;
    }

    if (!GetC())
    {
        size += 2; // Checksum inline
    }

    return size;
}

void
SixLowPanGhcUdp::Serialize(Buffer::Iterator start) const
{
    start.WriteU8(m_baseFormat);

    switch (GetPorts())
    {
    case PORTS_INLINE:
        start.WriteHtonU16(m_srcPort);
        start.WriteHtonU16(m_dstPort);
        break;
    case PORTS_ALL_SRC_LAST_DST:
        start.WriteHtonU16(m_srcPort);
        start.WriteU8(m_dstPort & 0xFF);
        break;
    case PORTS_LAST_SRC_ALL_DST:
        start.WriteU8(m_srcPort & 0xFF);
        start.WriteHtonU16(m_dstPort);
        break;
    case PORTS_LAST_SRC_LAST_DST:
        start.WriteU8(((m_srcPort & 0x0F) << 4) | (m_dstPort & 0x0F));
        break;
    }

    if (!GetC())
    {
        start.WriteHtonU16(m_checksum);
    }
}

uint32_t
SixLowPanGhcUdp::Deserialize(Buffer::Iterator start)
{
    m_baseFormat = start.ReadU8();

    switch (GetPorts())
    {
    case PORTS_INLINE:
        m_srcPort = start.ReadNtohU16();
        m_dstPort = start.ReadNtohU16();
        break;
    case PORTS_ALL_SRC_LAST_DST:
        m_srcPort = start.ReadNtohU16();
        m_dstPort = 0xF000 | start.ReadU8();
        break;
    case PORTS_LAST_SRC_ALL_DST:
        m_srcPort = 0xF000 | start.ReadU8();
        m_dstPort = start.ReadNtohU16();
        break;
    case PORTS_LAST_SRC_LAST_DST: {
        uint8_t packed = start.ReadU8();
        m_srcPort = 0xF0B0 | ((packed >> 4) & 0x0F);
        m_dstPort = 0xF0B0 | (packed & 0x0F);
        break;
    }
    }

    if (!GetC())
    {
        m_checksum = start.ReadNtohU16();
    }

    return GetSerializedSize();
}

SixLowPanDispatch::NhcDispatch_e
SixLowPanGhcUdp::GetNhcDispatchType() const
{
    return SixLowPanDispatch::LOWPAN_GHC_UDP;
}

void
SixLowPanGhcUdp::SetPorts(Ports_e port)
{
    m_baseFormat = GHC_UDP_BASE | (m_baseFormat & C_MASK) | (static_cast<uint8_t>(port) & P_MASK);
}

SixLowPanGhcUdp::Ports_e
SixLowPanGhcUdp::GetPorts() const
{
    return static_cast<Ports_e>(m_baseFormat & P_MASK);
}

void
SixLowPanGhcUdp::SetSrcPort(uint16_t port)
{
    m_srcPort = port;
}

uint16_t
SixLowPanGhcUdp::GetSrcPort() const
{
    return m_srcPort;
}

void
SixLowPanGhcUdp::SetDstPort(uint16_t port)
{
    m_dstPort = port;
}

uint16_t
SixLowPanGhcUdp::GetDstPort() const
{
    return m_dstPort;
}

void
SixLowPanGhcUdp::SetC(bool cField)
{
    if (cField)
    {
        m_baseFormat |= C_MASK;
    }
    else
    {
        m_baseFormat &= ~C_MASK;
    }
}

bool
SixLowPanGhcUdp::GetC() const
{
    return (m_baseFormat & C_MASK) != 0;
}

void
SixLowPanGhcUdp::SetChecksum(uint16_t checksum)
{
    m_checksum = checksum;
}

uint16_t
SixLowPanGhcUdp::GetChecksum() const
{
    return m_checksum;
}

std::ostream&
operator<<(std::ostream& os, const SixLowPanGhcUdp& header)
{
    header.Print(os);
    return os;
}

// ============================================================================
//  SixLowPanGhcIcmpv6 Implementation
// ============================================================================

NS_OBJECT_ENSURE_REGISTERED(SixLowPanGhcIcmpv6);

SixLowPanGhcIcmpv6::SixLowPanGhcIcmpv6()
    : m_blobLength(0)
{
    std::memset(m_blob, 0, sizeof(m_blob));
}

TypeId
SixLowPanGhcIcmpv6::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SixLowPanGhcIcmpv6")
                            .SetParent<Header>()
                            .SetGroupName("SixLowPan")
                            .AddConstructor<SixLowPanGhcIcmpv6>();
    return tid;
}

TypeId
SixLowPanGhcIcmpv6::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
SixLowPanGhcIcmpv6::Print(std::ostream& os) const
{
    os << "GHC ICMPv6: blob[" << (uint32_t)m_blobLength << "]";
}

uint32_t
SixLowPanGhcIcmpv6::GetSerializedSize() const
{
    return 1 + m_blobLength; // NHC byte + compressed ICMPv6 body
}

void
SixLowPanGhcIcmpv6::Serialize(Buffer::Iterator start) const
{
    start.WriteU8(GHC_ICMPV6_NHC);
    start.Write(m_blob, m_blobLength);
}

uint32_t
SixLowPanGhcIcmpv6::Deserialize(Buffer::Iterator start)
{
    uint8_t nhc = start.ReadU8();
    NS_ASSERT(nhc == GHC_ICMPV6_NHC);

    // ICMPv6 GHC: read remaining packet data as GHC bytecodes
    // (decompression terminates at packet boundary, no stop code)
    m_blobLength = 0;
    while (start.GetRemainingSize() > 0 && m_blobLength < 255)
    {
        m_blob[m_blobLength++] = start.ReadU8();
    }

    return GetSerializedSize();
}

SixLowPanDispatch::NhcDispatch_e
SixLowPanGhcIcmpv6::GetNhcDispatchType() const
{
    return SixLowPanDispatch::LOWPAN_GHC_ICMPV6;
}

void
SixLowPanGhcIcmpv6::SetBlob(const uint8_t* blob, uint32_t size)
{
    NS_ASSERT(size <= 255);
    m_blobLength = size;
    std::memcpy(m_blob, blob, size);
}

uint32_t
SixLowPanGhcIcmpv6::CopyBlob(uint8_t* blob, uint32_t size) const
{
    uint32_t copyLen = std::min((uint32_t)m_blobLength, size);
    std::memcpy(blob, m_blob, copyLen);
    return copyLen;
}

uint32_t
SixLowPanGhcIcmpv6::GetBlobLength() const
{
    return m_blobLength;
}

std::ostream&
operator<<(std::ostream& os, const SixLowPanGhcIcmpv6& header)
{
    header.Print(os);
    return os;
}

// ============================================================================
//  SixLowPan6Cio Implementation
// ============================================================================

NS_OBJECT_ENSURE_REGISTERED(SixLowPan6Cio);

SixLowPan6Cio::SixLowPan6Cio()
    : m_flags(0)
{
}

TypeId
SixLowPan6Cio::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SixLowPan6Cio")
                            .SetParent<Header>()
                            .SetGroupName("SixLowPan")
                            .AddConstructor<SixLowPan6Cio>();
    return tid;
}

TypeId
SixLowPan6Cio::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
SixLowPan6Cio::Print(std::ostream& os) const
{
    os << "6CIO: GHC=" << GetGhcCapable() << " flags=0x" << std::hex << m_flags << std::dec;
}

uint32_t
SixLowPan6Cio::GetSerializedSize() const
{
    // Type(1) + Length(1) + Flags(4 bytes = 32 bits)
    // Length field = 1 (in 8-octet units) => total 8 bytes
    // But we use minimum 6 bytes (Type + Len + 4 flag bytes)
    return 8; // Padded to 8-octet boundary per ND option rules
}

void
SixLowPan6Cio::Serialize(Buffer::Iterator start) const
{
    start.WriteU8(SIXLOWPAN_6CIO_TYPE);
    start.WriteU8(SIXLOWPAN_6CIO_LEN);
    start.WriteHtonU32(m_flags);
    start.WriteU16(0); // Padding to 8-byte boundary
}

uint32_t
SixLowPan6Cio::Deserialize(Buffer::Iterator start)
{
    uint8_t type = start.ReadU8();
    NS_ASSERT(type == SIXLOWPAN_6CIO_TYPE);
    uint8_t len = start.ReadU8();
    m_flags = start.ReadNtohU32();

    // Skip remaining bytes based on length field
    uint32_t remaining = (len * 8) - 6; // Total - (type + len + 4 flag bytes)
    for (uint32_t i = 0; i < remaining; i++)
    {
        start.ReadU8();
    }

    return len * 8;
}

void
SixLowPan6Cio::SetGhcCapable(bool ghcCapable)
{
    if (ghcCapable)
    {
        m_flags |= GHC_FLAG_BIT;
    }
    else
    {
        m_flags &= ~GHC_FLAG_BIT;
    }
}

bool
SixLowPan6Cio::GetGhcCapable() const
{
    return (m_flags & GHC_FLAG_BIT) != 0;
}

std::ostream&
operator<<(std::ostream& os, const SixLowPan6Cio& header)
{
    header.Print(os);
    return os;
}

} // namespace ns3
