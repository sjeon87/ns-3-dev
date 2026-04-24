/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#ifndef CBOR_H
#define CBOR_H

#include "ns3/buffer.h"

#include <stdint.h>
#include <string>

namespace ns3
{

/**
 * @ingroup BundleProtocol
 * @brief A CBOR encoder for BPv7 serialization directly into ns3::Buffer
 */
class Cbor
{
  public:
    static void WriteUint(Buffer::Iterator& i, uint64_t val);

    static void WriteArray(Buffer::Iterator& i, uint64_t size);

    static void WriteTextString(Buffer::Iterator& i, const std::string& text);

    static uint64_t ReadUint(Buffer::Iterator& i);

    static uint64_t ReadArray(Buffer::Iterator& i);

    static std::string ReadTextString(Buffer::Iterator& i);

    static uint32_t GetUintSize(uint64_t val);

    static uint32_t GetArraySize(uint64_t size);

    static uint32_t GetTextStringSize(const std::string& text);
};

} // namespace ns3

#endif /* CBOR_H */
