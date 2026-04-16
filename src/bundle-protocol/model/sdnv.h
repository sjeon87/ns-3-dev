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
#ifndef SDNV_H
#define SDNV_H

#include "ns3/buffer.h"

#include <stdint.h>
#include <vector>

namespace ns3
{

/**
 * @brief an implementation class of self-delimiting numeric values based on RFC 6256
 */
class Sdnv
{
  public:
    /**
     * Constructor
     */
    Sdnv();

    /**
     * Destroy
     */
    virtual ~Sdnv();

    /**
     * @brief SDNV encoding algorithm
     *
     * The encoding algorithm is based on section 3.1, RFC 6256
     *
     * @param val value need to be encoded
     * @return a uint8_t vector
     */
    std::vector<uint8_t> Encode(uint64_t val);

    /**
     * @brief SDNV decoding algorithm for an integer
     *
     * The decoding algorithm is based on section 3.2, RFC 6256
     *
     * @param val value need to be decoded
     * @return uint64_t decoded integer; It is user's responsibility to
     *         convert the return type to the type of variables in use
     */
    uint64_t Decode(std::vector<uint8_t> val);

    /**
     * @brief SDNV decoding algorithm for a Buffer
     *
     * This method read an integer from the Buffer and decodes it by
     * Decode (std::vector<uint8_t val>)
     *
     * @param start buffer start iterator reference
     * @return uint64_t decoded integer; It is user's responsibility to
     *         convert the return type to the type of variables in use
     */
    uint64_t Decode(Buffer::Iterator& start);

    /**
     * @brief is this the bolder of an encoded integer?
     *
     * @param val an entry of encoded vector
     * @return return true if it is the bolder
     */
    bool IsLast(uint8_t& val);
};

} // namespace ns3

#endif /* SDNV_H */
