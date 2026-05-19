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
    /**
     * Writes a uint64_t value to the buffer in CBOR encoding
     * @param i Iterator of the buffer to write to.
     * @param val The uint64_t value to write.
     */
    static void WriteUint(Buffer::Iterator& i, uint64_t val);

    /**
     * Writes an array to the buffer in CBOR encoding
     * @param i Iterator of the buffer to write to.
     * @param size The size of the array to write.
     */
    static void WriteArray(Buffer::Iterator& i, uint64_t size);

    /**
     * Writes a string of text to the buffer in CBOR encoding
     * @param i Iterator of the buffer to write to.
     * @param text The string to write.
     */
    static void WriteTextString(Buffer::Iterator& i, const std::string& text);

    /**
     * Writes a byte string header to the buffer in CBOR encoding
     * @param i Iterator of the buffer to write to.
     * @param len The length of the header.
     */
    static void WriteByteStringHeader(Buffer::Iterator& i, uint64_t len);

    /**
     * Reads a uint32_t value from the buffer.
     * @param i Iterator of the buffer to read from.
     * @return The parsed uint64_t value.
     */
    static uint64_t ReadUint(Buffer::Iterator& i);

    /**
     * Reads a byte-array from the buffer.
     * @param i Iterator of the buffer to read from.
     * @return The size of the array.
     */
    static uint64_t ReadArray(Buffer::Iterator& i);

    /**
     * Reads a string value from the buffer.
     * @param i Iterator of the buffer to read from.
     * @return The parsed string.
     */
    static std::string ReadTextString(Buffer::Iterator& i);

    /**
     * Reads a string header from the buffer.
     * @param i Iterator of the buffer to read from.
     * @return The length of the byte string.
     */
    static uint64_t ReadByteStringHeader(Buffer::Iterator& i);

    /**
     * Gets the size of the CBOR encoded uint64_t value
     * @param val Value to extract size from.
     * @return The number of bytes required to encode the value.
     */
    static uint32_t GetUintSize(uint64_t val);

    /**
     * Gets the size of a CBOR encoded array.
     * @param size Size of the array.
     * @return The number of bytes required to encode the array header.
     */
    static uint32_t GetArraySize(uint64_t size);

    /**
     * Gets the size of the CBOR string.
     * @param text String whose size is required.
     * @return The number of bytes required to encode the text string.
     */
    static uint32_t GetTextStringSize(const std::string& text);

    /**
     * Gets the size of the byte string header.
     * @param len Length of the byte string.
     * @return The number of bytes required to encode the byte string header.
     */
    static uint32_t GetByteStringHeaderSize(uint64_t len);

    /**
     * Computes the CRC-16/X-25 for a given data stream to return a checksum.
     * @param data Pointer to the array of bytes to check.
     * @param length Number of bytes in the data stream.
     * @return The 16-bit checksum value of the data stream.
     */
    static uint16_t ComputeCrc16(uint8_t* data, uint32_t length);
};

} // namespace ns3

#endif /* CBOR_H */
