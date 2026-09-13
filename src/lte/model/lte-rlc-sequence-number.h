/*
 * Copyright (c) 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#ifndef LTE_RLC_SEQUENCE_NUMBER_H
#define LTE_RLC_SEQUENCE_NUMBER_H

#include "ns3/assert.h"

#include <compare>
#include <iostream>
#include <limits>
#include <stdint.h>

namespace ns3
{

/// SequenceNumber10 class
class SequenceNumber10
{
  public:
    SequenceNumber10()
        : m_value(0),
          m_modulusBase(0)
    {
    }

    /**
     * Constructor
     *
     * @param value the value
     */
    explicit SequenceNumber10(uint16_t value)
        : m_value(value % 1024),
          m_modulusBase(0)
    {
    }

    /**
     * Constructor
     *
     * @param value the value
     */
    SequenceNumber10(const SequenceNumber10& value)
        : m_value(value.m_value),
          m_modulusBase(value.m_modulusBase)
    {
    }

    /**
     * Assignment operator
     *
     * @param value the value
     * @returns SequenceNumber10
     */
    SequenceNumber10& operator=(uint16_t value)
    {
        m_value = value % 1024;
        return *this;
    }

    /**
     * @brief Extracts the numeric value of the sequence number
     * @returns the sequence number value
     */
    uint16_t GetValue() const
    {
        return m_value;
    }

    /**
     * @brief Set modulus base
     * @param modulusBase the modulus
     */
    void SetModulusBase(SequenceNumber10 modulusBase)
    {
        m_modulusBase = modulusBase.m_value;
    }

    /**
     * @brief Set modulus base
     * @param modulusBase the modulus
     */
    void SetModulusBase(uint16_t modulusBase)
    {
        m_modulusBase = modulusBase;
    }

    /**
     * postfix ++ operator
     * @returns SequenceNumber10
     */
    SequenceNumber10 operator++(int)
    {
        SequenceNumber10 retval(m_value);
        m_value = ((uint32_t)m_value + 1) % 1024;
        retval.SetModulusBase(m_modulusBase);
        return retval;
    }

    /**
     * addition operator
     * @param delta the amount to add
     * @returns SequenceNumber10
     */
    SequenceNumber10 operator+(uint16_t delta) const
    {
        SequenceNumber10 ret((m_value + delta) % 1024);
        ret.SetModulusBase(m_modulusBase);
        return ret;
    }

    /**
     * subtraction operator
     * @param delta the amount to subtract
     * @returns SequenceNumber10
     */
    SequenceNumber10 operator-(uint16_t delta) const
    {
        SequenceNumber10 ret((m_value - delta) % 1024);
        ret.SetModulusBase(m_modulusBase);
        return ret;
    }

    /**
     * subtraction operator
     * @param other the amount to subtract
     * @returns SequenceNumber10
     */
    uint16_t operator-(const SequenceNumber10& other) const
    {
        uint16_t diff = m_value - other.m_value;
        return diff;
    }

    /**
     * Three-way comparison (spaceship) operator.
     * @param other sequence number to compare to this one
     * @returns The result of the comparison.
     */
    constexpr std::strong_ordering operator<=>(const SequenceNumber10& other) const
    {
        NS_ASSERT(m_modulusBase == other.m_modulusBase);

        if (m_value == other.m_value)
        {
            return std::strong_ordering::equivalent;
        }

        uint16_t v1 = (m_value - m_modulusBase) % 1024;
        uint16_t v2 = (other.m_value - other.m_modulusBase) % 1024;

        if (v1 > v2)
        {
            return std::strong_ordering::greater;
        }

        return std::strong_ordering::less;
    }

    /**
     * @brief Equality comparison operator.
     *
     * Two sequence numbers are considered equal if their raw sequence values
     * are identical. The modulus base is assumed to be consistent between
     * compared objects and is not independently validated here.
     *
     * @param other The sequence number to compare with.
     * @return true if the sequence numbers are equal, false otherwise.
     */
    constexpr bool operator==(const SequenceNumber10& other) const
    {
        return (*this <=> other) == std::strong_ordering::equivalent;
    }

    friend std::ostream& operator<<(std::ostream& os, const SequenceNumber10& val);

  private:
    uint16_t m_value;       ///< the value
    uint16_t m_modulusBase; ///< the modulus base
};

} // namespace ns3

#endif // LTE_RLC_SEQUENCE_NUMBER_H
