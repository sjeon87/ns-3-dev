/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#ifndef BUNDLE_AGENT_H
#define BUNDLE_AGENT_H

#include <vector>

namespace ns3
{

enum PBB_PROC_FLAGS
{
    IS_FRG = 0,
    ADMIN_RECORD,
    NO_FRAGMENT,
    CT_REQ,
    SINGLETON,
    ACK_REQ,
    PRIORITY_LSB = 7,
    PRIORITY_MSB = 8
};

enum PBB_STATUS_FLAGS
{
    BUNDLE_RECEPTION = 14,
    CUSTODY_ACCEPT,
    BUNDLE_FORWARD,
    BUNDLE_DELIVERY,
    BUNDLE_DELETION
};

enum PDB_PROC_FLAGS
{
    BLOCK_REPLICATE = 0,
    TRANSMIT_REPORT_ON_ERROR,
    DELETE_BUNDLE,
    LAST_BLOCK,
    DISCARD_BLOCK,
    BLOCK_FWD_WITHOUT_PROC,
    BLOCK_HAS_EID
};

enum STATUS_REPORT_FLAGS
{
    RECVD_BUNDLE = 0,
    CUSTODY_ACCEPTED,
    FWD_BUNDLE,
    DELIVER_BUNDLE,
    DEL_BUNDLE
};

enum STATUS_REPORT_REASON
{
    SR_NO_INFO = 0,
    SR_LIFE_EXPIRE,
    SR_FWD_OVER_LINK,
    SR_CANCEL_TX,
    SR_STORAGE_FULL,
    SR_EID_CORRUPT,
    SR_NO_ROUTE,
    SR_NO_CONTACT,
    SR_CORRUPT_BLK
};

enum CUSTODY_FLAGS
{
    CT_NO_INFO = 0,
    CT_REDUNDANT = 3,
    CT_STORAGE_FULL,
    CT_EID_CORRUPT,
    CT_NO_ROUTE,
    CT_NO_CONTACT,
    CT_CORRUPT_BLK
};

/**
 *
 * Each bundle stores a set of blocks. However in this implementation,
 * I'm going to assume that these blocks are just packets. I can't figure out what
 * the difference between a block and packet would be apart from the CBOR encoding
 * in RFC 9171, which I'm not sure yet is relevant to a simulation for complexity.
 *
 */
class Bundle : public Object
{
    Bundle();

  private:
    std::vector<Packet> m_blockList;
};

} // namespace ns3

#endif /* BUNDLE_AGENT_H */
