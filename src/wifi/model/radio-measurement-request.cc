/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Devansh Gupta <devanshg308@gmail.com>
 */

#include "radio-measurement-request.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RadioMeasurementRequestHeader");
NS_OBJECT_ENSURE_REGISTERED(RadioMeasurementRequestHeader);

TypeId
RadioMeasurementRequestHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RadioMeasurementRequestHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<RadioMeasurementRequestHeader>();
    return tid;
}

TypeId
RadioMeasurementRequestHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
RadioMeasurementRequestHeader::Print(std::ostream& os) const
{
    os << "DialogToken=" << +m_dialogToken << ", NumberOfRepetitions=" << m_numberOfRepetitions
       << ", MeasurementRequestElements=" << m_measurementRequestElements.size();
}

uint32_t
RadioMeasurementRequestHeader::GetSerializedSize() const
{
    uint32_t size = 1 + 2; // Dialog Token (1) + Number of Repetitions (2)
    for (const auto& elem : m_measurementRequestElements)
    {
        size += elem.GetSerializedSize();
    }
    return size;
}

void
RadioMeasurementRequestHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(m_dialogToken);
    i.WriteU16(m_numberOfRepetitions);
    for (const auto& elem : m_measurementRequestElements)
    {
        i = elem.Serialize(i);
    }
}

uint32_t
RadioMeasurementRequestHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_dialogToken = i.ReadU8();
    m_numberOfRepetitions = i.ReadU16();

    m_measurementRequestElements.clear();
    while (true)
    {
        auto tmp = i;
        MeasurementRequestElement elem;
        i = elem.DeserializeIfPresent(i);
        if (i.GetDistanceFrom(tmp) == 0)
        {
            break;
        }
        m_measurementRequestElements.push_back(std::move(elem));
    }

    return i.GetDistanceFrom(start);
}

void
RadioMeasurementRequestHeader::SetDialogToken(uint8_t token)
{
    m_dialogToken = token;
}

uint8_t
RadioMeasurementRequestHeader::GetDialogToken() const
{
    return m_dialogToken;
}

void
RadioMeasurementRequestHeader::SetNumberOfRepetitions(uint16_t repetitions)
{
    m_numberOfRepetitions = repetitions;
}

uint16_t
RadioMeasurementRequestHeader::GetNumberOfRepetitions() const
{
    return m_numberOfRepetitions;
}

void
RadioMeasurementRequestHeader::AddMeasurementRequestElement(const MeasurementRequestElement& elem)
{
    m_measurementRequestElements.push_back(elem);
}

const std::vector<MeasurementRequestElement>&
RadioMeasurementRequestHeader::GetMeasurementRequestElements() const
{
    return m_measurementRequestElements;
}

} // namespace ns3
