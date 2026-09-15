/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#ifndef EID_H
#define EID_H

#include "ns3/buffer.h"

#include <stdint.h>
#include <string>

namespace ns3
{

/**
 * @ingroup BundleProtocol
 * @brief Helper class to handle serialization and deserialization of BPv7 EIDs.
 */
class Eid
{
  public:
    /**
     * @brief Serializes an EID string into the provided buffer iterator.
     * @param i The buffer iterator to write to.
     * @param eid The EID string to serialize.
     */
    static void Write(Buffer::Iterator& i, const std::string& eid);

    /**
     * @brief Deserializes an EID string from the provided buffer iterator.
     * @param i The buffer iterator to read from.
     * @return The deserialized EID string.
     */
    static std::string Read(Buffer::Iterator& i);

    /**
     * @brief Calculates the size of an EID string in bytes.
     * @param eid The EID string to evaluate.
     * @return The number of bytes required to serialize the EID.
     */
    static uint32_t GetSize(const std::string& eid);
};

} // namespace ns3

#endif /* EID_H */
