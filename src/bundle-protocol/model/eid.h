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
 * @brief Helper class to handle serialization and deserialization of BPv7 EIDs
 */
class Eid
{
  public:
    static void Write(Buffer::Iterator& i, const std::string& eid);
    static std::string Read(Buffer::Iterator& i);
    static uint32_t GetSize(const std::string& eid);
};

} // namespace ns3

#endif /* EID_H */