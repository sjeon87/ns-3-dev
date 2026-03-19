/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Devansh Gupta <devanshg308@gmail.com>
 */

#include "measurement-report-element.h"

#include "ns3/address-utils.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MeasurementReportElement");

// --- BeaconReport ---

uint16_t
BeaconReport::GetSerializedSize() const
{
    return 26;
}

void
BeaconReport::Serialize(Buffer::Iterator& start) const
{
    start.WriteU8(m_operatingClass);
    start.WriteU8(m_channelNumber);
    start.WriteU64(m_actualMeasurementStartTime);
    start.WriteU16(m_measurementDuration);
    start.WriteU8(m_reportedFrameInfo);
    start.WriteU8(m_rcpi);
    start.WriteU8(m_rsni);
    WriteTo(start, m_bssid);
    start.WriteU8(m_antennaId);
    start.WriteU32(m_parentTsf);
}

uint16_t
BeaconReport::Deserialize(Buffer::Iterator& start)
{
    m_operatingClass = start.ReadU8();
    m_channelNumber = start.ReadU8();
    m_actualMeasurementStartTime = start.ReadU64();
    m_measurementDuration = start.ReadU16();
    m_reportedFrameInfo = start.ReadU8();
    m_rcpi = start.ReadU8();
    m_rsni = start.ReadU8();
    ReadFrom(start, m_bssid);
    m_antennaId = start.ReadU8();
    m_parentTsf = start.ReadU32();
    return 26;
}

void
BeaconReport::SetOperatingClass(uint8_t operatingClass)
{
    m_operatingClass = operatingClass;
}

uint8_t
BeaconReport::GetOperatingClass() const
{
    return m_operatingClass;
}

void
BeaconReport::SetChannelNumber(uint8_t channel)
{
    m_channelNumber = channel;
}

uint8_t
BeaconReport::GetChannelNumber() const
{
    return m_channelNumber;
}

void
BeaconReport::SetActualMeasurementStartTime(uint64_t startTime)
{
    m_actualMeasurementStartTime = startTime;
}

uint64_t
BeaconReport::GetActualMeasurementStartTime() const
{
    return m_actualMeasurementStartTime;
}

void
BeaconReport::SetMeasurementDuration(uint16_t duration)
{
    m_measurementDuration = duration;
}

uint16_t
BeaconReport::GetMeasurementDuration() const
{
    return m_measurementDuration;
}

void
BeaconReport::SetReportedFrameInformation(uint8_t condensedPhyType, bool reportedFrameType)
{
    m_reportedFrameInfo = (condensedPhyType & 0x7F) | (reportedFrameType ? (1 << 7) : 0);
}

uint8_t
BeaconReport::GetCondensedPhyType() const
{
    return m_reportedFrameInfo & 0x7F;
}

bool
BeaconReport::GetReportedFrameType() const
{
    return (m_reportedFrameInfo & (1 << 7)) != 0;
}

void
BeaconReport::SetRcpi(uint8_t rcpi)
{
    m_rcpi = rcpi;
}

uint8_t
BeaconReport::GetRcpi() const
{
    return m_rcpi;
}

void
BeaconReport::SetRsni(uint8_t rsni)
{
    m_rsni = rsni;
}

uint8_t
BeaconReport::GetRsni() const
{
    return m_rsni;
}

void
BeaconReport::SetBssid(Mac48Address bssid)
{
    m_bssid = bssid;
}

Mac48Address
BeaconReport::GetBssid() const
{
    return m_bssid;
}

void
BeaconReport::SetAntennaId(uint8_t antennaId)
{
    m_antennaId = antennaId;
}

uint8_t
BeaconReport::GetAntennaId() const
{
    return m_antennaId;
}

void
BeaconReport::SetParentTsf(uint32_t parentTsf)
{
    m_parentTsf = parentTsf;
}

uint32_t
BeaconReport::GetParentTsf() const
{
    return m_parentTsf;
}

// --- MeasurementReportElement ---

WifiInformationElementId
MeasurementReportElement::ElementId() const
{
    return IE_MEASUREMENT_REPORT;
}

void
MeasurementReportElement::SetMeasurementToken(uint8_t token)
{
    m_measurementToken = token;
}

uint8_t
MeasurementReportElement::GetMeasurementToken() const
{
    return m_measurementToken;
}

void
MeasurementReportElement::SetLate(bool late)
{
    if (late)
    {
        NS_ABORT_MSG_IF(m_measurementReportMode & 0x06,
                        "No more than one bit is set to 1 within a Measurement Report Mode field");
        m_measurementReportMode |= (1 << 0);
    }
    else
    {
        m_measurementReportMode &= ~(1 << 0);
    }
}

bool
MeasurementReportElement::GetLate() const
{
    return (m_measurementReportMode & (1 << 0)) != 0;
}

void
MeasurementReportElement::SetIncapable(bool incapable)
{
    if (incapable)
    {
        NS_ABORT_MSG_IF(m_measurementReportMode & 0x05,
                        "No more than one bit is set to 1 within a Measurement Report Mode field");
        m_measurementReportMode |= (1 << 1);
    }
    else
    {
        m_measurementReportMode &= ~(1 << 1);
    }
}

bool
MeasurementReportElement::GetIncapable() const
{
    return (m_measurementReportMode & (1 << 1)) != 0;
}

void
MeasurementReportElement::SetRefused(bool refused)
{
    if (refused)
    {
        NS_ABORT_MSG_IF(m_measurementReportMode & 0x03,
                        "No more than one bit is set to 1 within a Measurement Report Mode field");
        m_measurementReportMode |= (1 << 2);
    }
    else
    {
        m_measurementReportMode &= ~(1 << 2);
    }
}

bool
MeasurementReportElement::GetRefused() const
{
    return (m_measurementReportMode & (1 << 2)) != 0;
}

void
MeasurementReportElement::SetMeasurementType(MeasurementReportType type)
{
    m_measurementType = static_cast<uint8_t>(type);
}

uint8_t
MeasurementReportElement::GetMeasurementType() const
{
    return m_measurementType;
}

void
MeasurementReportElement::SetBeaconReport(const BeaconReport& report)
{
    m_measurementType = static_cast<uint8_t>(MeasurementReportType::BEACON);
    m_report = report;
}

std::optional<BeaconReport>
MeasurementReportElement::GetBeaconReport() const
{
    if (std::holds_alternative<BeaconReport>(m_report))
    {
        return std::get<BeaconReport>(m_report);
    }
    return std::nullopt;
}

bool
MeasurementReportElement::HasModeSet() const
{
    return (m_measurementReportMode & 0x07) != 0;
}

uint16_t
MeasurementReportElement::GetInformationFieldSize() const
{
    uint16_t size = 3; // token(1) + mode(1) + type(1)
    if (!HasModeSet() && std::holds_alternative<BeaconReport>(m_report))
    {
        size += std::get<BeaconReport>(m_report).GetSerializedSize();
    }
    return size;
}

void
MeasurementReportElement::SerializeInformationField(Buffer::Iterator start) const
{
    start.WriteU8(m_measurementToken);
    start.WriteU8(m_measurementReportMode);
    start.WriteU8(m_measurementType);

    if (!HasModeSet() && std::holds_alternative<BeaconReport>(m_report))
    {
        BeaconReport br = std::get<BeaconReport>(m_report);
        br.Serialize(start);
    }
}

uint16_t
MeasurementReportElement::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    Buffer::Iterator i = start;
    m_measurementToken = i.ReadU8();
    m_measurementReportMode = i.ReadU8();
    m_measurementType = i.ReadU8();

    uint16_t bytesRead = 3;

    if (!HasModeSet() && bytesRead < length)
    {
        if (m_measurementType == static_cast<uint8_t>(MeasurementReportType::BEACON))
        {
            BeaconReport br;
            bytesRead += br.Deserialize(i);
            m_report = br;
        }
        else
        {
            // Skip unknown report body
            uint16_t remaining = length - bytesRead;
            for (uint16_t j = 0; j < remaining; j++)
            {
                i.ReadU8();
            }
            bytesRead += remaining;
            m_report = std::monostate{};
        }
    }

    return bytesRead;
}

void
MeasurementReportElement::Print(std::ostream& os) const
{
    os << "MeasurementReport=[Token=" << +m_measurementToken << ", Mode=0x" << std::hex
       << +m_measurementReportMode << std::dec << ", Type=" << +m_measurementType;
    if (HasModeSet())
    {
        if (GetLate())
        {
            os << " Late";
        }
        if (GetIncapable())
        {
            os << " Incapable";
        }
        if (GetRefused())
        {
            os << " Refused";
        }
    }
    else if (std::holds_alternative<BeaconReport>(m_report))
    {
        const auto& br = std::get<BeaconReport>(m_report);
        os << ", BeaconReport=[OpClass=" << +br.GetOperatingClass()
           << ", Channel=" << +br.GetChannelNumber()
           << ", StartTime=" << br.GetActualMeasurementStartTime()
           << ", Duration=" << br.GetMeasurementDuration()
           << ", PHYType=" << +br.GetCondensedPhyType()
           << ", FrameType=" << br.GetReportedFrameType() << ", RCPI=" << +br.GetRcpi()
           << ", RSNI=" << +br.GetRsni() << ", BSSID=" << br.GetBssid()
           << ", AntennaId=" << +br.GetAntennaId() << ", ParentTSF=0x" << std::hex
           << br.GetParentTsf() << std::dec << "]";
    }
    os << "]";
}

} // namespace ns3
