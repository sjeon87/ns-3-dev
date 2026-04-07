/*
 * Copyright (c) 2026 Michigan State University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ishaan Lagwankar <lagwanka@msu.edu>
 */

#ifndef BUNDLE_FLAGS_H
#define BUNDLE_FLAGS_H

namespace ns3
{

/**
 * @brief Primary Bundle Block Processing Control Flags (RFC 9171, Section 4.2.3)
 */
enum PBB_PROC_FLAGS
{
    IS_FRG = 0,               //!< Bundle is a fragment
    ADMIN_RECORD = 1,         //!< Payload is an administrative record
    NO_FRAGMENT = 2,          //!< Bundle must not be fragmented
    REQ_APP_ACK = 5,          //!< Acknowledgment by the user application is requested
    REQ_STATUS_TIME = 6,      //!< Status time is requested in all status reports
    REQ_REP_RECV = 14,        //!< Request reporting of bundle reception
    REQ_REP_FWD = 15,         //!< Request reporting of bundle forwarding
    REQ_REP_DELIV = 16,       //!< Request reporting of bundle delivery
    REQ_REP_DEL = 17          //!< Request reporting of bundle deletion
};

/**
 * @brief Status Report Request Flags (Mapped to Bit positions in PBB_PROC_FLAGS)
 */
enum PBB_STATUS_FLAGS
{
    BUNDLE_RECEPTION = 14,
    BUNDLE_FORWARD = 15,
    BUNDLE_DELIVERY = 16,
    BUNDLE_DELETION = 17
};

/**
 * @brief Block Processing Control Flags for Extension Blocks (RFC 9171, Section 4.2.4)
 */
enum PDB_PROC_FLAGS
{
    BLOCK_REPLICATE = 0,            //!< Block must be replicated in every fragment
    TRANSMIT_REPORT_ON_ERROR = 1,   //!< Transmit status report if block can't be processed
    DELETE_BUNDLE = 2,              //!< Delete bundle if block can't be processed
    DISCARD_BLOCK = 4,              //!< Discard block if it can't be processed
    BLOCK_FWD_WITHOUT_PROC = 5      //!< Block was forwarded without being processed
};

/**
 * @brief Status Report Administrative Record Flags (RFC 9171, Section 5.1)
 */
enum STATUS_REPORT_FLAGS
{
    RECVD_BUNDLE = 0,
    FWD_BUNDLE = 1,
    DELIVER_BUNDLE = 2,
    DEL_BUNDLE = 3
};

/**
 * @brief Bundle Status Report Reason Codes (RFC 9171, Section 9.5)
 */
enum STATUS_REPORT_REASON
{
    SR_NO_INFO = 0,                 //!< No additional information
    SR_LIFE_EXPIRE = 1,             //!< Lifetime expired
    SR_FWD_OVER_LINK = 2,           //!< Forwarded over unidirectional link
    SR_CANCEL_TX = 3,               //!< Transmission canceled
    SR_STORAGE_FULL = 4,            //!< Depleted storage
    SR_EID_UNINTELLIGIBLE = 5,      //!< Destination endpoint ID unintelligible
    SR_NO_ROUTE = 6,                //!< No known route to destination from here
    SR_NO_CONTACT = 7,              //!< No timely contact with next node on route
    SR_BLOCK_UNINTELLIGIBLE = 8,    //!< Block unintelligible
    SR_HOP_LIMIT_EXCEEDED = 9,      //!< Hop limit exceeded
    SR_TRAFFIC_PARED = 10,          //!< Traffic pared
    SR_BLOCK_UNSUPPORTED = 11       //!< Block unsupported
};

} // namespace ns3

#endif /* BUNDLE_FLAGS_H */