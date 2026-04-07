/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#include "bundle.h"

#include "ns3/log.h"

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
    for (auto it = m_blocks.rbegin(); it != m_blocks.rend(); ++it)
    {
        Ptr<Packet> blockPacket = (*it)->SerializeToPacket();
        bundle->AddAtEnd(blockPacket);
    }
    return bundle;
}

void
Bundle::Deserialize(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this << p);
    m_blocks.clear();

    Ptr<Packet> copy = p->Copy();

    Ptr<PrimaryBlock> primary = CreateObject<PrimaryBlock>();
    primary->Deserialize(copy);
    m_blocks.push_back(primary);

    if (copy->GetSize() > 0)
    {
        Ptr<PayloadBlock> payload = CreateObject<PayloadBlock>();
        payload->Deserialize(copy);
        m_blocks.push_back(payload);
    }
}

Time
Bundle::GetExpiry() const
{
    NS_LOG_FUNCTION(this);
    Ptr<PrimaryBlock> primary = GetPrimaryBlock();
    NS_ASSERT_MSG(primary, "Bundle has no primary block");
    const PrimaryBlockHeader& h = primary->GetHeader();
    return h.GetCreationTime() + h.GetTTL();
}

std::string
Bundle::GetDestinationEID() const
{
    NS_LOG_FUNCTION(this);
    Ptr<PrimaryBlock> primary = GetPrimaryBlock();
    NS_ASSERT_MSG(primary, "Bundle has no primary block");
    const PrimaryBlockHeader& h = primary->GetHeader();
    const std::string& dict = h.GetDictionary();
    std::string scheme = dict.c_str() + h.GetDestinationSchemeOffset();
    std::string ssp = dict.c_str() + h.GetDestinationSSPOffset();
    return scheme + ":" + ssp;
}

std::string
Bundle::GetSourceEID() const
{
    NS_LOG_FUNCTION(this);
    Ptr<PrimaryBlock> primary = GetPrimaryBlock();
    NS_ASSERT_MSG(primary, "Bundle has no primary block");
    const PrimaryBlockHeader& h = primary->GetHeader();
    const std::string& dict = h.GetDictionary();
    std::string scheme = dict.c_str() + h.GetSourceSchemeOffset();
    std::string ssp = dict.c_str() + h.GetSourceSSPOffset();
    return scheme + ":" + ssp;
}

std::string
Bundle::GetReportToEID() const
{
    NS_LOG_FUNCTION(this);
    Ptr<PrimaryBlock> primary = GetPrimaryBlock();
    NS_ASSERT_MSG(primary, "Bundle has no primary block");
    const PrimaryBlockHeader& h = primary->GetHeader();
    const std::string& dict = h.GetDictionary();
    std::string scheme = dict.c_str() + h.GetReportToSchemeOffset();
    std::string ssp = dict.c_str() + h.GetReportToSSPOffset();
    return scheme + ":" + ssp;
}

bool
Bundle::IsAdminRecord() const
{
    NS_LOG_FUNCTION(this);
    Ptr<PrimaryBlock> primary = GetPrimaryBlock();
    NS_ASSERT_MSG(primary, "Bundle has no primary block");
    return (primary->GetHeader().GetProcFlags() & 0x02) != 0;
}

} // namespace ns3
