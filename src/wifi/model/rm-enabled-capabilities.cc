/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Devansh Gupta <devanshg308@gmail.com>
 */

#include "rm-enabled-capabilities.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RmEnabledCapabilities");

WifiInformationElementId
RmEnabledCapabilities::ElementId() const
{
    return IE_RM_ENABLED_CAPACITIES;
}

bool
RmEnabledCapabilities::GetBit(uint8_t bitPos) const
{
    return (m_capabilities[bitPos / 8] >> (bitPos % 8)) & 1;
}

void
RmEnabledCapabilities::SetBit(uint8_t bitPos, bool val)
{
    uint8_t mask = 1 << (bitPos % 8);
    if (val)
    {
        m_capabilities[bitPos / 8] |= mask;
    }
    else
    {
        m_capabilities[bitPos / 8] &= ~mask;
    }
}

uint8_t
RmEnabledCapabilities::Get3BitField(uint8_t startBit) const
{
    return (m_capabilities[startBit / 8] >> (startBit % 8)) & 0x7;
}

void
RmEnabledCapabilities::Set3BitField(uint8_t startBit, uint8_t val)
{
    NS_ASSERT_MSG(val <= 7, "3-bit field value must be 0-7, got " << +val);
    uint8_t shift = startBit % 8;
    m_capabilities[startBit / 8] =
        (m_capabilities[startBit / 8] & ~(0x7 << shift)) | ((val & 0x7) << shift);
}

void
RmEnabledCapabilities::SetLinkMeasurement(bool val)
{
    SetBit(0, val);
}

bool
RmEnabledCapabilities::GetLinkMeasurement() const
{
    return GetBit(0);
}

void
RmEnabledCapabilities::SetNeighborReport(bool val)
{
    SetBit(1, val);
}

bool
RmEnabledCapabilities::GetNeighborReport() const
{
    return GetBit(1);
}

void
RmEnabledCapabilities::SetParallelMeasurements(bool val)
{
    SetBit(2, val);
}

bool
RmEnabledCapabilities::GetParallelMeasurements() const
{
    return GetBit(2);
}

void
RmEnabledCapabilities::SetRepeatedMeasurements(bool val)
{
    SetBit(3, val);
}

bool
RmEnabledCapabilities::GetRepeatedMeasurements() const
{
    return GetBit(3);
}

void
RmEnabledCapabilities::SetBeaconPassiveMeasurement(bool val)
{
    SetBit(4, val);
}

bool
RmEnabledCapabilities::GetBeaconPassiveMeasurement() const
{
    return GetBit(4);
}

void
RmEnabledCapabilities::SetBeaconActiveMeasurement(bool val)
{
    SetBit(5, val);
}

bool
RmEnabledCapabilities::GetBeaconActiveMeasurement() const
{
    return GetBit(5);
}

void
RmEnabledCapabilities::SetBeaconTableMeasurement(bool val)
{
    SetBit(6, val);
}

bool
RmEnabledCapabilities::GetBeaconTableMeasurement() const
{
    return GetBit(6);
}

void
RmEnabledCapabilities::SetBeaconMeasurementReportingConditions(bool val)
{
    SetBit(7, val);
}

bool
RmEnabledCapabilities::GetBeaconMeasurementReportingConditions() const
{
    return GetBit(7);
}

void
RmEnabledCapabilities::SetFrameMeasurement(bool val)
{
    SetBit(8, val);
}

bool
RmEnabledCapabilities::GetFrameMeasurement() const
{
    return GetBit(8);
}

void
RmEnabledCapabilities::SetChannelLoadMeasurement(bool val)
{
    SetBit(9, val);
}

bool
RmEnabledCapabilities::GetChannelLoadMeasurement() const
{
    return GetBit(9);
}

void
RmEnabledCapabilities::SetNoiseHistogramMeasurement(bool val)
{
    SetBit(10, val);
}

bool
RmEnabledCapabilities::GetNoiseHistogramMeasurement() const
{
    return GetBit(10);
}

void
RmEnabledCapabilities::SetStatisticsMeasurement(bool val)
{
    SetBit(11, val);
}

bool
RmEnabledCapabilities::GetStatisticsMeasurement() const
{
    return GetBit(11);
}

void
RmEnabledCapabilities::SetLciMeasurement(bool val)
{
    SetBit(12, val);
}

bool
RmEnabledCapabilities::GetLciMeasurement() const
{
    return GetBit(12);
}

void
RmEnabledCapabilities::SetLciAzimuth(bool val)
{
    SetBit(13, val);
}

bool
RmEnabledCapabilities::GetLciAzimuth() const
{
    return GetBit(13);
}

void
RmEnabledCapabilities::SetTransmitStreamCategoryMeasurement(bool val)
{
    SetBit(14, val);
}

bool
RmEnabledCapabilities::GetTransmitStreamCategoryMeasurement() const
{
    return GetBit(14);
}

void
RmEnabledCapabilities::SetTriggeredTransmitStreamCategoryMeasurement(bool val)
{
    SetBit(15, val);
}

bool
RmEnabledCapabilities::GetTriggeredTransmitStreamCategoryMeasurement() const
{
    return GetBit(15);
}

void
RmEnabledCapabilities::SetApChannelReport(bool val)
{
    SetBit(16, val);
}

bool
RmEnabledCapabilities::GetApChannelReport() const
{
    return GetBit(16);
}

void
RmEnabledCapabilities::SetRmMib(bool val)
{
    SetBit(17, val);
}

bool
RmEnabledCapabilities::GetRmMib() const
{
    return GetBit(17);
}

void
RmEnabledCapabilities::SetOperatingChannelMaxMeasurementDuration(uint8_t val)
{
    Set3BitField(18, val);
}

uint8_t
RmEnabledCapabilities::GetOperatingChannelMaxMeasurementDuration() const
{
    return Get3BitField(18);
}

void
RmEnabledCapabilities::SetNonoperatingChannelMaxMeasurementDuration(uint8_t val)
{
    Set3BitField(21, val);
}

uint8_t
RmEnabledCapabilities::GetNonoperatingChannelMaxMeasurementDuration() const
{
    return Get3BitField(21);
}

void
RmEnabledCapabilities::SetMeasurementPilotCapability(uint8_t val)
{
    Set3BitField(24, val);
}

uint8_t
RmEnabledCapabilities::GetMeasurementPilotCapability() const
{
    return Get3BitField(24);
}

void
RmEnabledCapabilities::SetMeasurementPilotTransmissionInformation(bool val)
{
    SetBit(27, val);
}

bool
RmEnabledCapabilities::GetMeasurementPilotTransmissionInformation() const
{
    return GetBit(27);
}

void
RmEnabledCapabilities::SetNeighborReportTsfOffset(bool val)
{
    SetBit(28, val);
}

bool
RmEnabledCapabilities::GetNeighborReportTsfOffset() const
{
    return GetBit(28);
}

void
RmEnabledCapabilities::SetRcpiMeasurement(bool val)
{
    SetBit(29, val);
}

bool
RmEnabledCapabilities::GetRcpiMeasurement() const
{
    return GetBit(29);
}

void
RmEnabledCapabilities::SetRsniMeasurement(bool val)
{
    SetBit(30, val);
}

bool
RmEnabledCapabilities::GetRsniMeasurement() const
{
    return GetBit(30);
}

void
RmEnabledCapabilities::SetBssAverageAccessDelay(bool val)
{
    SetBit(31, val);
}

bool
RmEnabledCapabilities::GetBssAverageAccessDelay() const
{
    return GetBit(31);
}

void
RmEnabledCapabilities::SetBssAvailableAdmissionCapacity(bool val)
{
    SetBit(32, val);
}

bool
RmEnabledCapabilities::GetBssAvailableAdmissionCapacity() const
{
    return GetBit(32);
}

void
RmEnabledCapabilities::SetAntenna(bool val)
{
    SetBit(33, val);
}

bool
RmEnabledCapabilities::GetAntenna() const
{
    return GetBit(33);
}

void
RmEnabledCapabilities::SetFtmRangeReport(bool val)
{
    SetBit(34, val);
}

bool
RmEnabledCapabilities::GetFtmRangeReport() const
{
    return GetBit(34);
}

void
RmEnabledCapabilities::SetCivicLocationMeasurement(bool val)
{
    SetBit(35, val);
}

bool
RmEnabledCapabilities::GetCivicLocationMeasurement() const
{
    return GetBit(35);
}

void
RmEnabledCapabilities::Print(std::ostream& os) const
{
    os << "RM Enabled Capabilities: [";
    for (uint8_t i = 0; i < 5; i++)
    {
        if (i > 0)
        {
            os << " ";
        }
        os << std::hex << std::uppercase << static_cast<int>(m_capabilities[i]) << std::dec;
    }
    os << "]";

    // Boolean capabilities
    if (GetLinkMeasurement())
    {
        os << " LinkMeasurement";
    }
    if (GetNeighborReport())
    {
        os << " NeighborReport";
    }
    if (GetParallelMeasurements())
    {
        os << " ParallelMeasurements";
    }
    if (GetRepeatedMeasurements())
    {
        os << " RepeatedMeasurements";
    }
    if (GetBeaconPassiveMeasurement())
    {
        os << " BeaconPassive";
    }
    if (GetBeaconActiveMeasurement())
    {
        os << " BeaconActive";
    }
    if (GetBeaconTableMeasurement())
    {
        os << " BeaconTable";
    }
    if (GetBeaconMeasurementReportingConditions())
    {
        os << " BeaconReportingConditions";
    }
    if (GetFrameMeasurement())
    {
        os << " FrameMeasurement";
    }
    if (GetChannelLoadMeasurement())
    {
        os << " ChannelLoad";
    }
    if (GetNoiseHistogramMeasurement())
    {
        os << " NoiseHistogram";
    }
    if (GetStatisticsMeasurement())
    {
        os << " Statistics";
    }
    if (GetLciMeasurement())
    {
        os << " LCI";
    }
    if (GetLciAzimuth())
    {
        os << " LciAzimuth";
    }
    if (GetTransmitStreamCategoryMeasurement())
    {
        os << " TransmitStreamCategory";
    }
    if (GetTriggeredTransmitStreamCategoryMeasurement())
    {
        os << " TriggeredTransmitStreamCategory";
    }
    if (GetApChannelReport())
    {
        os << " ApChannelReport";
    }
    if (GetRmMib())
    {
        os << " RmMib";
    }

    // 3-bit fields
    os << " OpChMaxDuration=" << +GetOperatingChannelMaxMeasurementDuration();
    os << " NonopChMaxDuration=" << +GetNonoperatingChannelMaxMeasurementDuration();
    os << " MeasPilotCap=" << +GetMeasurementPilotCapability();

    // Remaining boolean capabilities
    if (GetMeasurementPilotTransmissionInformation())
    {
        os << " MeasPilotTxInfo";
    }
    if (GetNeighborReportTsfOffset())
    {
        os << " NeighborReportTsfOffset";
    }
    if (GetRcpiMeasurement())
    {
        os << " RCPI";
    }
    if (GetRsniMeasurement())
    {
        os << " RSNI";
    }
    if (GetBssAverageAccessDelay())
    {
        os << " BssAvgAccessDelay";
    }
    if (GetBssAvailableAdmissionCapacity())
    {
        os << " BssAvailAdmCap";
    }
    if (GetAntenna())
    {
        os << " Antenna";
    }
    if (GetFtmRangeReport())
    {
        os << " FtmRangeReport";
    }
    if (GetCivicLocationMeasurement())
    {
        os << " CivicLocation";
    }
}

uint16_t
RmEnabledCapabilities::GetInformationFieldSize() const
{
    return 5;
}

void
RmEnabledCapabilities::SerializeInformationField(Buffer::Iterator start) const
{
    // Mask off reserved bits 36-39 (upper nibble of byte 4) before writing
    for (uint8_t i = 0; i < 4; i++)
    {
        start.WriteU8(m_capabilities[i]);
    }
    start.WriteU8(m_capabilities[4] & 0x0F);
}

uint16_t
RmEnabledCapabilities::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    NS_ABORT_MSG_IF(length != 5, "RM Enabled Capabilities length must be 5, got " << length);
    for (uint8_t i = 0; i < 4; i++)
    {
        m_capabilities[i] = start.ReadU8();
    }
    m_capabilities[4] = start.ReadU8() & 0x0F;
    return 5;
}

} // namespace ns3
