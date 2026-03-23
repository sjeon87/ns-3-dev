/*
 * Copyright (c) 2012 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#ifndef HASH_MURMUR3_H
#define HASH_MURMUR3_H

#include "hash-function.h"

/**
 * @file
 * @ingroup hash
 * @brief ns3::Hash::Function::Murmur3 and ns3::Hash::Function::Murmur3_x64
 * declarations.
 */

namespace ns3
{

namespace Hash
{

namespace Function
{

/**
 *  @ingroup hash
 *
 *  @brief Murmur3 hash function implementation
 *
 *  Adapted from http://code.google.com/p/smhasher/
 *
 *  MurmurHash3 was written by Austin Appleby, and is placed in the public
 *  domain. The author hereby disclaims copyright to this source code.

 *  Note - The x86 and x64 versions do _not_ produce the same results, as the
 *  algorithms are optimized for their respective platforms. You can still
 *  compile and run any of them on any platform, but your performance with the
 *  non-native version will be less than optimal.
 */
class Murmur3 : public Implementation
{
  public:
    /**
     * Constructor, clears internal state
     */
    Murmur3();
    /**
     * Compute 32-bit hash of a byte buffer
     *
     * Call clear () between calls to GetHash32() to reset the
     * internal state and hash each buffer separately.
     *
     * If you don't call clear() between calls to GetHash32,
     * you can hash successive buffers.  The final return value
     * will be the cumulative hash across all calls.
     *
     * @param [in] buffer pointer to the beginning of the buffer
     * @param [in] size length of the buffer, in bytes
     * @return 32-bit hash of the buffer
     */
    uint32_t GetHash32(const char* buffer, const std::size_t size) override;
    /**
     * Compute 64-bit hash of a byte buffer.
     *
     * Call clear () between calls to GetHash64() to reset the
     * internal state and hash each buffer separately.
     *
     * If you don't call clear() between calls to GetHash64,
     * you can hash successive buffers.  The final return value
     * will be the cumulative hash across all calls.
     *
     * @param [in] buffer pointer to the beginning of the buffer
     * @param [in] size length of the buffer, in bytes
     * @return 64-bit hash of the buffer
     */
    uint64_t GetHash64(const char* buffer, const std::size_t size) override;
    /**
     * Restore initial state
     */
    void clear() override;

  private:
    /**
     * Seed value
     *
     * This has to be a constant for all MPI ranks to generate
     * the same hash from the same string.
     */
    static constexpr auto SEED{0x8BADF00D}; // Ate bad food

    /**
     * Cache last hash value, and total bytes hashed (needed to finalize),
     * for incremental hashing
     */
    /**@{*/
    uint32_t m_hash32;
    std::size_t m_size32;
    /**@}*/

    /** murmur3 produces 128-bit hash and state; we use just the first 64-bits. */
    /**@{*/
    uint64_t m_hash64[2];
    std::size_t m_size64;
    /**@}*/

    // end of class Murmur3
};

/**
 *  @ingroup hash
 *
 *  @brief Murmur3 hash function, x64-optimised variant (opt-in).
 *
 *  Wraps MurmurHash3_x64_128.  On 64-bit hosts it is typically faster
 *  than the default Murmur3 (x86) variant because its inner loop operates on
 *  64-bit words natively.  However, it produces different hash values
 *  for the same input, so it is an explicit opt-in to avoid silently
 *  invalidating existing simulation results.
 *
 *  The default hash implementation used by ns3::Hasher remains Murmur3.
 *
 *  Usage example:
 *  @code
 *    Hasher h(Create<Hash::Function::Murmur3_x64>());
 *    uint64_t v = h.GetHash64(buffer, size);
 *  @endcode
 *
 *  The 32-bit result is the lower 32 bits of the 64-bit hash, so the two
 *  are mutually consistent for the same input.
 *
 *  @note Each call to GetHash64() or GetHash32() is a complete one-shot
 *  hash of its argument.  Successive calls without clear() do not
 *  accumulate input.
 */
class Murmur3_x64 : public Implementation
{
  public:
    /**
     * Constructor, clears internal state.
     */
    Murmur3_x64();
    /**
     * Compute 32-bit hash of a byte buffer.
     *
     * Returns the lower 32 bits of the 64-bit hash.
     *
     * @param [in] buffer Pointer to the beginning of the buffer.
     * @param [in] size Length of the buffer, in bytes.
     * @return 32-bit hash of the buffer.
     */
    uint32_t GetHash32(const char* buffer, const std::size_t size) override;
    /**
     * Compute 64-bit hash of a byte buffer.
     *
     * @param [in] buffer Pointer to the beginning of the buffer.
     * @param [in] size Length of the buffer, in bytes.
     * @return 64-bit hash of the buffer.
     */
    uint64_t GetHash64(const char* buffer, const std::size_t size) override;
    /**
     * Restore initial state.
     */
    void clear() override;

  private:
    static constexpr auto SEED{0x8BADF00D}; // Ate bad food

    uint64_t m_hash64[2]; /**< 128-bit state from MurmurHash3_x64_128_incr. */
    std::size_t m_size64;

}; // end of class Murmur3_x64

} // namespace Function

} // namespace Hash

} // namespace ns3

#endif /* HASH_MURMUR3_H */
