/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "cbor.h"

namespace ns3
{

void
Cbor::WriteUint(Buffer::Iterator& i, uint64_t val)
{
    uint8_t majorType = 0x00;
    if (val <= 23)
    {
        i.WriteU8(majorType | static_cast<uint8_t>(val));
    }
    else if (val <= 0xFF)
    {
        i.WriteU8(majorType | 24);
        i.WriteU8(static_cast<uint8_t>(val));
    }
    else if (val <= 0xFFFF)
    {
        i.WriteU8(majorType | 25);
        i.WriteHtonU16(static_cast<uint16_t>(val));
    }
    else if (val <= 0xFFFFFFFF)
    {
        i.WriteU8(majorType | 26);
        i.WriteHtonU32(static_cast<uint32_t>(val));
    }
    else
    {
        i.WriteU8(majorType | 27);
        i.WriteHtonU64(val);
    }
}

void
Cbor::WriteArray(Buffer::Iterator& i, uint64_t size)
{
    uint8_t majorType = 0x80; // 0b100_00000
    if (size <= 23)
    {
        i.WriteU8(majorType | static_cast<uint8_t>(size));
    }
    else if (size <= 0xFF)
    {
        i.WriteU8(majorType | 24);
        i.WriteU8(static_cast<uint8_t>(size));
    }
    else if (size <= 0xFFFF)
    {
        i.WriteU8(majorType | 25);
        i.WriteHtonU16(static_cast<uint16_t>(size));
    }
    else
    {
        i.WriteU8(majorType | 26);
        i.WriteHtonU32(static_cast<uint32_t>(size));
    }
}

void
Cbor::WriteTextString(Buffer::Iterator& i, const std::string& text)
{
    uint8_t majorType = 0x60; // 0b011_00000
    uint64_t size = text.length();

    if (size <= 23)
    {
        i.WriteU8(majorType | static_cast<uint8_t>(size));
    }
    else if (size <= 0xFF)
    {
        i.WriteU8(majorType | 24);
        i.WriteU8(static_cast<uint8_t>(size));
    }
    else if (size <= 0xFFFF)
    {
        i.WriteU8(majorType | 25);
        i.WriteHtonU16(static_cast<uint16_t>(size));
    }
    else
    {
        i.WriteU8(majorType | 26);
        i.WriteHtonU32(static_cast<uint32_t>(size));
    }

    i.Write(reinterpret_cast<const uint8_t*>(text.data()), size);
}

void
Cbor::WriteByteStringHeader(Buffer::Iterator& i, uint64_t len)
{
    uint8_t majorType = 0x40;
    if (len <= 23)
    {
        i.WriteU8(majorType | static_cast<uint8_t>(len));
    }
    else if (len <= 0xFF)
    {
        i.WriteU8(majorType | 24);
        i.WriteU8(static_cast<uint8_t>(len));
    }
    else if (len <= 0xFFFF)
    {
        i.WriteU8(majorType | 25);
        i.WriteHtonU16(static_cast<uint16_t>(len));
    }
    else
    {
        i.WriteU8(majorType | 26);
        i.WriteHtonU32(static_cast<uint32_t>(len));
    }
}

uint64_t
Cbor::ReadByteStringHeader(Buffer::Iterator& i)
{
    uint8_t initialByte = i.ReadU8();
    uint8_t additionalInfo = initialByte & 0x1F;
    if (additionalInfo <= 23)
    {
        return additionalInfo;
    }
    else if (additionalInfo == 24)
    {
        return i.ReadU8();
    }
    else if (additionalInfo == 25)
    {
        return i.ReadNtohU16();
    }
    else if (additionalInfo == 26)
    {
        return i.ReadNtohU32();
    }
    return 0;
}

uint64_t
Cbor::ReadUint(Buffer::Iterator& i)
{
    uint8_t initialByte = i.ReadU8();
    uint8_t additionalInfo = initialByte & 0x1F;

    if (additionalInfo <= 23)
    {
        return additionalInfo;
    }
    else if (additionalInfo == 24)
    {
        return i.ReadU8();
    }
    else if (additionalInfo == 25)
    {
        return i.ReadNtohU16();
    }
    else if (additionalInfo == 26)
    {
        return i.ReadNtohU32();
    }
    else if (additionalInfo == 27)
    {
        return i.ReadNtohU64();
    }
    return 0;
}

uint64_t
Cbor::ReadArray(Buffer::Iterator& i)
{
    return ReadUint(i);
}

std::string
Cbor::ReadTextString(Buffer::Iterator& i)
{
    uint64_t length = ReadUint(i);

    std::string result;
    if (length > 0)
    {
        auto* buf = new uint8_t[length];
        i.Read(buf, length);
        result.assign(reinterpret_cast<char*>(buf), length);
        delete[] buf;
    }
    return result;
}

uint32_t
Cbor::GetUintSize(uint64_t val)
{
    if (val <= 23)
    {
        return 1;
    }
    if (val <= 0xFF)
    {
        return 2;
    }
    if (val <= 0xFFFF)
    {
        return 3;
    }
    if (val <= 0xFFFFFFFF)
    {
        return 5;
    }
    return 9;
}

uint32_t
Cbor::GetArraySize(uint64_t size)
{
    return GetUintSize(size);
}

uint32_t
Cbor::GetTextStringSize(const std::string& text)
{
    return GetUintSize(text.length()) + text.length();
}

uint32_t
Cbor::GetByteStringHeaderSize(uint64_t len)
{
    if (len <= 23)
    {
        return 1;
    }
    if (len <= 0xFF)
    {
        return 2;
    }
    if (len <= 0xFFFF)
    {
        return 3;
    }
    return 5;
}

uint16_t
Cbor::ComputeCrc16(uint8_t* data, uint32_t length)
{
    uint16_t crc = 0xFFFF;
    for (uint32_t i = 0; i < length; i++)
    {
        crc ^= static_cast<uint16_t>(data[i]);
        for (int bit = 0; bit < 8; bit++)
        {
            if (crc & 0x0001)
            {
                crc = (crc >> 1) ^ 0x8408;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc ^ 0xFFFF;
}

} // namespace ns3
