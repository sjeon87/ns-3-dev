/*
 * Copyright (c) 2013 University of New Brunswick
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 *
 * Author: Dizhi Zhou        <dizhi.zhou@gmail.com>
 *         Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "sdnv.h"

#include "ns3/log.h"

#include <algorithm>
#include <stdint.h>

NS_LOG_COMPONENT_DEFINE("Sdnv");

namespace ns3
{

Sdnv::Sdnv()
{
    NS_LOG_FUNCTION(this);
}

Sdnv::~Sdnv()
{
    NS_LOG_FUNCTION(this);
}

std::vector<uint8_t>
Sdnv::Encode(uint64_t val)
{
    NS_LOG_FUNCTION(this << " " << val);
    std::vector<uint8_t> data;

    if (val == 0)
    {
        data.push_back(0);
        return data;
    }

    uint8_t encoded;

    encoded = val & 0x7F;
    val >>= 7;
    data.push_back(encoded);
    while (val)
    {
        encoded = val & 0x7F;
        val >>= 7;
        encoded |= 0x80;
        data.push_back(encoded);
    }
    std::reverse(data.begin(), data.end());
    return data;
}

uint64_t
Sdnv::Decode(std::vector<uint8_t> val)
{
    NS_LOG_FUNCTION(this);
    uint64_t decoded = 0;
    std::vector<uint8_t>::iterator iter;
    for (iter = val.begin(); iter != val.end(); iter++)
    {
        decoded <<= 7;
        decoded += (*iter) & 0x7F;
        if (((*iter) & 0x80) == 0)
        {
            return decoded;
        }
    }
    return decoded;
}

uint64_t
Sdnv::Decode(Buffer::Iterator& start)
{
    NS_LOG_FUNCTION(this);
    std::vector<uint8_t> vec;

    // check the last byte of a variable in the buffer
    do
    {
        uint8_t val = start.ReadU8();
        vec.push_back(val);

        if (IsLast(val))
        {
            break;
        }
    } while (1);

    return Decode(vec);
}

bool
Sdnv::IsLast(uint8_t& val)
{
    NS_LOG_FUNCTION(this << " " << (uint16_t)val);
    if ((val & 0x80) == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

} // namespace ns3
