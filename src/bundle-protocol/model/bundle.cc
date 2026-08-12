/*
 * Copyright (c) 2008 INRIA
 *                  2013 University of New Brunswick
 *                  2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *           Dizhi Zhou <dizhi.zhou@gmail.com>
 *           Gerard Garcia <ggarcia@deic.uab.cat>
 *           Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "bundle.h"

#include "bundle-protocol-flags.h"

#include "ns3/log.h"

#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Bundle");

NS_OBJECT_ENSURE_REGISTERED(Bundle);

TypeId
Bundle::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Bundle")
                            .SetParent<Object>()
                            .SetGroupName("BundleProtocol")
                            .AddConstructor<Bundle>();
    return tid;
}

Bundle::Bundle()
{
    NS_LOG_FUNCTION(this);
}

Bundle::~Bundle()
{
    NS_LOG_FUNCTION(this);
}

void
Bundle::AddBlock(Ptr<BundleBlock> block)
{
    NS_LOG_FUNCTION(this << block);
    m_blocks.push_back(block);
}

Ptr<BundleBlock>
Bundle::GetBlock(uint32_t index) const
{
    NS_LOG_FUNCTION(this << index);
    NS_ASSERT_MSG(index < m_blocks.size(), "Block index out of range");
    return m_blocks[index];
}

uint32_t
Bundle::GetBlockCount() const
{
    NS_LOG_FUNCTION(this);
    return m_blocks.size();
}

const std::vector<Ptr<BundleBlock>>&
Bundle::GetBlocks() const
{
    NS_LOG_FUNCTION(this);
    return m_blocks;
}

Ptr<PrimaryBlock>
Bundle::GetPrimaryBlock() const
{
    NS_LOG_FUNCTION(this);
    if (!m_blocks.empty())
    {
        return m_blocks[0]->GetObject<PrimaryBlock>();
    }
    return nullptr;
}

Ptr<PayloadBlock>
Bundle::GetPayloadBlock() const
{
    NS_LOG_FUNCTION(this);
    for (const auto& block : m_blocks)
    {
        if (block->GetBlockType() == 1)
        {
            return block->GetObject<PayloadBlock>();
        }
    }
    return nullptr;
}

uint32_t
Bundle::GetTotalSize() const
{
    NS_LOG_FUNCTION(this);
    uint32_t size = 0;
    for (const auto& block : m_blocks)
    {
        size += block->SerializeToPacket()->GetSize();
    }
    return size;
}

Ptr<Packet>
Bundle::Serialize() const
{
    NS_LOG_FUNCTION(this);
    Ptr<Packet> bundle = Create<Packet>();

    uint8_t indefiniteOpen = 0x9F;
    bundle->AddAtEnd(Create<Packet>(&indefiniteOpen, 1));

    for (const auto& block : m_blocks)
    {
        Ptr<Packet> blockPacket = block->SerializeToPacket();
        bundle->AddAtEnd(blockPacket);
    }

    uint8_t breakCode = 0xFF;
    bundle->AddAtEnd(Create<Packet>(&breakCode, 1));

    return bundle;
}

void
Bundle::Deserialize(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this << p);
    m_blocks.clear();

    Ptr<Packet> copy = p->Copy();
    copy->RemoveAtStart(1);

    Ptr<PrimaryBlock> primary = CreateObject<PrimaryBlock>();
    primary->Deserialize(copy);
    m_blocks.emplace_back(primary);

    while (copy->GetSize() > 1)
    {
        Ptr<PayloadBlock> payload = CreateObject<PayloadBlock>();
        payload->Deserialize(copy);
        m_blocks.emplace_back(payload);
    }
    copy->RemoveAtStart(1);
}

Time
Bundle::GetExpiry() const
{
    NS_LOG_FUNCTION(this);
    Ptr<PrimaryBlock> primary = GetPrimaryBlock();
    NS_ASSERT_MSG(primary, "Bundle has no primary block");
    const PrimaryBlockHeader& h = primary->GetHeader();
    return h.GetCreationTime() + h.GetLifetime();
}

std::string
Bundle::GetDestinationEID() const
{
    NS_LOG_FUNCTION(this);
    Ptr<PrimaryBlock> primary = GetPrimaryBlock();
    NS_ASSERT_MSG(primary, "Bundle has no primary block");
    return primary->GetHeader().GetDestinationEID();
}

std::string
Bundle::GetSourceEID() const
{
    NS_LOG_FUNCTION(this);
    Ptr<PrimaryBlock> primary = GetPrimaryBlock();
    NS_ASSERT_MSG(primary, "Bundle has no primary block");
    return primary->GetHeader().GetSourceEID();
}

std::string
Bundle::GetReportToEID() const
{
    NS_LOG_FUNCTION(this);
    Ptr<PrimaryBlock> primary = GetPrimaryBlock();
    NS_ASSERT_MSG(primary, "Bundle has no primary block");
    return primary->GetHeader().GetReportToEID();
}

bool
Bundle::IsAdminRecord() const
{
    NS_LOG_FUNCTION(this);
    Ptr<PrimaryBlock> primary = GetPrimaryBlock();
    NS_ASSERT_MSG(primary, "Bundle has no primary block");
    return (primary->GetHeader().GetProcFlags() & (1 << ADMIN_RECORD)) != 0;
}

uint32_t
Bundle::GetHopCount() const
{
    NS_LOG_FUNCTION(this);
    Ptr<PrimaryBlock> primary = GetPrimaryBlock();
    NS_ASSERT_MSG(primary, "Bundle has no primary block");
    return primary->GetHeader().GetHopCount();
}

std::vector<Ptr<Bundle>>
Bundle::Fragment(Ptr<Bundle> original, uint32_t maxPayloadSize)
{
    NS_LOG_FUNCTION(original << maxPayloadSize);
    NS_ASSERT_MSG(maxPayloadSize > 0, "maxPayloadSize must be positive");

    std::vector<Ptr<Bundle>> fragments;

    Ptr<PrimaryBlock> origPrimary = original->GetPrimaryBlock();
    Ptr<PayloadBlock> origPayload = original->GetPayloadBlock();
    NS_ASSERT_MSG(origPrimary && origPayload,
                  "Cannot fragment a bundle without primary and payload blocks");

    const PrimaryBlockHeader& origHeader = origPrimary->GetHeader();
    Ptr<Packet> fullPayload = origPayload->GetPayload();
    uint32_t totalLength = fullPayload ? fullPayload->GetSize() : 0;

    bool alreadyFragment = (origHeader.GetProcFlags() & (1 << IS_FRG)) != 0;
    uint32_t baseOffset = alreadyFragment ? origHeader.GetFragmentOffset() : 0;
    uint32_t totalAppDataLength =
        alreadyFragment ? origHeader.GetTotalAppDataLength() : totalLength;

    for (uint32_t offset = 0; offset < totalLength; offset += maxPayloadSize)
    {
        uint32_t chunkSize = std::min(maxPayloadSize, totalLength - offset);
        Ptr<Packet> chunk = fullPayload->CreateFragment(offset, chunkSize);

        PrimaryBlockHeader fragHeader = origHeader;
        fragHeader.SetProcFlags(origHeader.GetProcFlags() | (1 << IS_FRG));
        fragHeader.SetFragmentOffset(baseOffset + offset);
        fragHeader.SetTotalAppDataLength(totalAppDataLength);

        Ptr<PrimaryBlock> fragPrimary = CreateObject<PrimaryBlock>();
        fragPrimary->GetHeader() = fragHeader;

        PayloadBlockHeader fragPayloadHeader = origPayload->GetHeader();

        Ptr<PayloadBlock> fragPayload = CreateObject<PayloadBlock>();
        fragPayload->GetHeader() = fragPayloadHeader;
        fragPayload->SetPayload(chunk);

        Ptr<Bundle> fragmentBundle = CreateObject<Bundle>();
        fragmentBundle->AddBlock(fragPrimary);
        fragmentBundle->AddBlock(fragPayload);
        fragments.push_back(fragmentBundle);
    }

    return fragments;
}

} // namespace ns3
