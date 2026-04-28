/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "eid.h"

#include "cbor.h"

namespace ns3
{

void
Eid::Write(Buffer::Iterator& i, const std::string& eid)
{
    if (eid == "dtn:none")
    {
        Cbor::WriteArray(i, 2);
        Cbor::WriteUint(i, 1);
        Cbor::WriteUint(i, 0);
    }
    else if (eid.substr(0, 4) == "dtn:")
    {
        Cbor::WriteArray(i, 2);
        Cbor::WriteUint(i, 1);
        Cbor::WriteTextString(i, eid.substr(4));
    }
    else if (eid.substr(0, 4) == "ipn:")
    {
        std::string ssp = eid.substr(4);
        uint64_t nodeNum = std::stoull(ssp.substr(0, ssp.find('.')));
        uint64_t serviceNum = std::stoull(ssp.substr(ssp.find('.') + 1));
        Cbor::WriteArray(i, 2);
        Cbor::WriteUint(i, 2);
        Cbor::WriteArray(i, 2);
        Cbor::WriteUint(i, nodeNum);
        Cbor::WriteUint(i, serviceNum);
    }
}

std::string
Eid::Read(Buffer::Iterator& i)
{
    Cbor::ReadArray(i);
    uint64_t scheme = Cbor::ReadUint(i);

    if (scheme == 1)
    {
        uint8_t peek = i.PeekU8();
        if ((peek & 0xE0) == 0x00)
        {
            Cbor::ReadUint(i);
            return "dtn:none";
        }
        return "dtn:" + Cbor::ReadTextString(i);
    }
    else if (scheme == 2)
    {
        uint8_t peek = i.PeekU8();
        uint64_t nodeNum, serviceNum;
        if ((peek & 0xE0) == 0x80)
        {
            Cbor::ReadArray(i);
            nodeNum = Cbor::ReadUint(i);
            serviceNum = Cbor::ReadUint(i);
        }
        else
        {
            nodeNum = Cbor::ReadUint(i);
            serviceNum = Cbor::ReadUint(i);
        }
        return "ipn:" + std::to_string(nodeNum) + "." + std::to_string(serviceNum);
    }

    return "dtn:none";
}

uint32_t
Eid::GetSize(const std::string& eid)
{
    uint32_t size = 0;
    if (eid == "dtn:none")
    {
        size += Cbor::GetArraySize(2);
        size += Cbor::GetUintSize(1);
        size += Cbor::GetUintSize(0);
    }
    else if (eid.substr(0, 4) == "dtn:")
    {
        size += Cbor::GetArraySize(2);
        size += Cbor::GetUintSize(1);
        size += Cbor::GetTextStringSize(eid.substr(4));
    }
    else if (eid.substr(0, 4) == "ipn:")
    {
        std::string ssp = eid.substr(4);
        uint64_t nodeNum = std::stoull(ssp.substr(0, ssp.find('.')));
        uint64_t serviceNum = std::stoull(ssp.substr(ssp.find('.') + 1));
        size += Cbor::GetArraySize(2);
        size += Cbor::GetUintSize(2);
        size += Cbor::GetArraySize(2);
        size += Cbor::GetUintSize(nodeNum);
        size += Cbor::GetUintSize(serviceNum);
    }
    return size;
}

} // namespace ns3
