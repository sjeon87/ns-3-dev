/*
 * Copyright (c) 2021
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "wifi-phy-common.h"

#include "wifi-mode.h"
#include "wifi-net-device.h"

#include "ns3/he-configuration.h"
#include "ns3/ht-configuration.h"

namespace ns3
{

Time
GetGuardIntervalForMode(WifiMode mode, const Ptr<WifiNetDevice> device)
{
    auto gi = NanoSeconds(800);
    if (mode.GetModulationClass() >= WIFI_MOD_CLASS_HE)
    {
        Ptr<HeConfiguration> heConfiguration = device->GetHeConfiguration();
        NS_ASSERT(heConfiguration); // If HE/EHT modulation is used, we should have a HE
                                    // configuration attached
        gi = heConfiguration->GetGuardInterval();
    }
    else if (mode.GetModulationClass() == WIFI_MOD_CLASS_HT ||
             mode.GetModulationClass() == WIFI_MOD_CLASS_VHT)
    {
        Ptr<HtConfiguration> htConfiguration = device->GetHtConfiguration();
        NS_ASSERT(htConfiguration); // If HT/VHT modulation is used, we should have a HT
                                    // configuration attached
        gi = NanoSeconds(htConfiguration->m_sgiSupported ? 400 : 800);
    }
    return gi;
}

Time
GetGuardIntervalForMode(WifiMode mode, bool htShortGuardInterval, Time heGuardInterval)
{
    auto gi = NanoSeconds(800);
    if (mode.GetModulationClass() >= WIFI_MOD_CLASS_HE)
    {
        gi = heGuardInterval;
    }
    else if (mode.GetModulationClass() == WIFI_MOD_CLASS_HT ||
             mode.GetModulationClass() == WIFI_MOD_CLASS_VHT)
    {
        gi = NanoSeconds(htShortGuardInterval ? 400 : 800);
    }
    return gi;
}

WifiPreamble
GetPreambleForTransmission(WifiModulationClass modulation, bool useShortPreamble /* = false */)
{
    if (modulation == WIFI_MOD_CLASS_EHT)
    {
        return WIFI_PREAMBLE_EHT_MU;
    }
    else if (modulation == WIFI_MOD_CLASS_HE)
    {
        return WIFI_PREAMBLE_HE_SU;
    }
    else if (modulation == WIFI_MOD_CLASS_DMG_CTRL)
    {
        return WIFI_PREAMBLE_DMG_CTRL;
    }
    else if (modulation == WIFI_MOD_CLASS_DMG_SC)
    {
        return WIFI_PREAMBLE_DMG_SC;
    }
    else if (modulation == WIFI_MOD_CLASS_DMG_OFDM)
    {
        return WIFI_PREAMBLE_DMG_OFDM;
    }
    else if (modulation == WIFI_MOD_CLASS_VHT)
    {
        return WIFI_PREAMBLE_VHT_SU;
    }
    else if (modulation == WIFI_MOD_CLASS_HT)
    {
        return WIFI_PREAMBLE_HT_MF; // HT_GF has been removed
    }
    else if (modulation == WIFI_MOD_CLASS_HR_DSSS &&
             useShortPreamble) // ERP_DSSS is modeled through HR_DSSS (since same preamble and
                               // modulation)
    {
        return WIFI_PREAMBLE_SHORT;
    }
    else
    {
        return WIFI_PREAMBLE_LONG;
    }
}

WifiModulationClass
GetModulationClassForPreamble(WifiPreamble preamble)
{
    switch (preamble)
    {
    case WIFI_PREAMBLE_HT_MF:
        return WIFI_MOD_CLASS_HT;
    case WIFI_PREAMBLE_VHT_SU:
    case WIFI_PREAMBLE_VHT_MU:
        return WIFI_MOD_CLASS_VHT;
    case WIFI_PREAMBLE_HE_SU:
    case WIFI_PREAMBLE_HE_ER_SU:
    case WIFI_PREAMBLE_HE_MU:
    case WIFI_PREAMBLE_HE_TB:
        return WIFI_MOD_CLASS_HE;
    case WIFI_PREAMBLE_EHT_MU:
    case WIFI_PREAMBLE_EHT_TB:
        return WIFI_MOD_CLASS_EHT;
    default:
        NS_ABORT_MSG("Unsupported preamble type: " << preamble);
    }
    return WIFI_MOD_CLASS_UNKNOWN;
}

bool
IsAllowedControlAnswerModulationClass(WifiModulationClass modClassReq,
                                      WifiModulationClass modClassAnswer)
{
    switch (modClassReq)
    {
    case WIFI_MOD_CLASS_DSSS:
        return (modClassAnswer == WIFI_MOD_CLASS_DSSS);
    case WIFI_MOD_CLASS_HR_DSSS:
        return (modClassAnswer == WIFI_MOD_CLASS_DSSS || modClassAnswer == WIFI_MOD_CLASS_HR_DSSS);
    case WIFI_MOD_CLASS_ERP_OFDM:
        return (modClassAnswer == WIFI_MOD_CLASS_DSSS || modClassAnswer == WIFI_MOD_CLASS_HR_DSSS ||
                modClassAnswer == WIFI_MOD_CLASS_ERP_OFDM);
    case WIFI_MOD_CLASS_OFDM:
        return (modClassAnswer == WIFI_MOD_CLASS_OFDM);
    case WIFI_MOD_CLASS_HT:
    case WIFI_MOD_CLASS_VHT:
    case WIFI_MOD_CLASS_HE:
    case WIFI_MOD_CLASS_EHT:
        return true;
    default:
        NS_FATAL_ERROR("Modulation class not defined");
        return false;
    }
}

Time
GetPpduMaxTime(WifiPreamble preamble)
{
    Time duration;

    switch (preamble)
    {
    case WIFI_PREAMBLE_HT_MF:
    case WIFI_PREAMBLE_VHT_SU:
    case WIFI_PREAMBLE_VHT_MU:
    case WIFI_PREAMBLE_HE_SU:
    case WIFI_PREAMBLE_HE_ER_SU:
    case WIFI_PREAMBLE_HE_MU:
    case WIFI_PREAMBLE_HE_TB:
    case WIFI_PREAMBLE_EHT_MU:
    case WIFI_PREAMBLE_EHT_TB:
        duration = MicroSeconds(5484);
        break;
    default:
        duration = MicroSeconds(0);
        break;
    }
    return duration;
}

bool
IsMu(WifiPreamble preamble)
{
    return (IsDlMu(preamble) || IsUlMu(preamble));
}

bool
IsDlMu(WifiPreamble preamble)
{
    return ((preamble == WIFI_PREAMBLE_HE_MU) || (preamble == WIFI_PREAMBLE_EHT_MU));
}

bool
IsUlMu(WifiPreamble preamble)
{
    return ((preamble == WIFI_PREAMBLE_HE_TB) || (preamble == WIFI_PREAMBLE_EHT_TB));
}

WifiModulationClass
GetModulationClassForStandard(WifiStandard standard)
{
    WifiModulationClass modulationClass{WIFI_MOD_CLASS_UNKNOWN};
    switch (standard)
    {
    case WIFI_STANDARD_80211a:
        [[fallthrough]];
    case WIFI_STANDARD_80211p:
        modulationClass = WIFI_MOD_CLASS_OFDM;
        break;
    case WIFI_STANDARD_80211b:
        // Although two modulation classes are supported in 802.11b, return the
        // numerically greater one defined in the WifiModulationClass enum.
        // See issue #1095 for more explanation.
        modulationClass = WIFI_MOD_CLASS_HR_DSSS;
        break;
    case WIFI_STANDARD_80211g:
        modulationClass = WIFI_MOD_CLASS_ERP_OFDM;
        break;
    case WIFI_STANDARD_80211n:
        modulationClass = WIFI_MOD_CLASS_HT;
        break;
    case WIFI_STANDARD_80211ac:
        modulationClass = WIFI_MOD_CLASS_VHT;
        break;
    case WIFI_STANDARD_80211ax:
        modulationClass = WIFI_MOD_CLASS_HE;
        break;
    case WIFI_STANDARD_80211be:
        modulationClass = WIFI_MOD_CLASS_EHT;
        break;
    case WIFI_STANDARD_UNSPECIFIED:
        [[fallthrough]];
    default:
        NS_ASSERT_MSG(false, "Unsupported standard " << standard);
        break;
    }
    return modulationClass;
}

std::set<MHz_u>
GetSupportedChannelWidthSet(WifiStandard standard, WifiPhyBand band)
{
    switch (standard)
    {
    case WIFI_STANDARD_80211p:
        return {MHz_u{5}};
    case WIFI_STANDARD_80211a:
    case WIFI_STANDARD_80211g:
        return {MHz_u{20}};
    case WIFI_STANDARD_80211b:
        return {MHz_u{22}};
    case WIFI_STANDARD_80211n:
        return {MHz_u{20}, MHz_u{40}};
    case WIFI_STANDARD_80211ac:
        return {MHz_u{80}, MHz_u{160}};
    case WIFI_STANDARD_80211ax:
        return (band == WifiPhyBand::WIFI_PHY_BAND_2_4GHZ)
                   ? std::set<MHz_u>{MHz_u{20}, MHz_u{40}}
                   : std::set<MHz_u>{MHz_u{20}, MHz_u{80}, MHz_u{160}};
    case WIFI_STANDARD_80211be:
        switch (band)
        {
        case WifiPhyBand::WIFI_PHY_BAND_2_4GHZ:
            return {MHz_u{20}, MHz_u{40}};
        case WifiPhyBand::WIFI_PHY_BAND_5GHZ:
            return {MHz_u{20}, MHz_u{80}, MHz_u{160}};
        case WifiPhyBand::WIFI_PHY_BAND_6GHZ:
            return {MHz_u{20}, MHz_u{80}, MHz_u{160}, MHz_u{320}};
        default:
            NS_ABORT_MSG("Unknown band: " << band);
            return {};
        }
    default:
        NS_ABORT_MSG("Unknown standard: " << standard);
        return {};
    }
}

MHz_u
GetMaximumChannelWidth(WifiModulationClass modulation)
{
    switch (modulation)
    {
    case WIFI_MOD_CLASS_DSSS:
    case WIFI_MOD_CLASS_HR_DSSS:
        return MHz_u{22};
    case WIFI_MOD_CLASS_OFDM:
    case WIFI_MOD_CLASS_ERP_OFDM:
        return MHz_u{20};
    case WIFI_MOD_CLASS_HT:
        return MHz_u{40};
    case WIFI_MOD_CLASS_VHT:
    case WIFI_MOD_CLASS_HE:
        return MHz_u{160};
    case WIFI_MOD_CLASS_EHT:
        return MHz_u{320};
    default:
        NS_ABORT_MSG("Unknown modulation class: " << modulation);
        return MHz_u{0};
    }
}

std::optional<WifiTonePlan>
GetTonePlan(WifiModulationClass modulation, MHz_u channelWidth)
{
    const auto bw = static_cast<std::size_t>(channelWidth);

    switch (modulation)
    {
    case WIFI_MOD_CLASS_OFDM:
    case WIFI_MOD_CLASS_ERP_OFDM:
        // Table 17-5 "Timing-related parameters" of 802.11-2020
        switch (bw)
        {
        case 20:
        case 10: // same layout as 20 MHz with halved subcarrier spacing
        case 5:  // same layout as 20 MHz with quartered subcarrier spacing
            return WifiTonePlan{64, 52, 48, 4, 6, 1};
        default:
            return std::nullopt;
        }
    case WIFI_MOD_CLASS_HT:
        // Table 19-6 "Timing-related constants" of 802.11-2020
        switch (bw)
        {
        case 20:
            return WifiTonePlan{64, 56, 52, 4, 4, 1};
        case 40:
            return WifiTonePlan{128, 114, 108, 6, 6, 3};
        default:
            return std::nullopt;
        }
    case WIFI_MOD_CLASS_VHT:
        // Table 21-5 "Timing-related constants" of 802.11-2020
        switch (bw)
        {
        case 20:
            return WifiTonePlan{64, 56, 52, 4, 4, 1};
        case 40:
            return WifiTonePlan{128, 114, 108, 6, 6, 3};
        case 80:
            return WifiTonePlan{256, 242, 234, 8, 6, 3};
        case 160:
            return WifiTonePlan{512, 484, 468, 16, 6, 3};
        default:
            return std::nullopt;
        }
    case WIFI_MOD_CLASS_HE:
    case WIFI_MOD_CLASS_EHT:
        // Subcarrier allocation per full-bandwidth RU (Table 27-14 "Subcarrier allocation related
        // constants for RUs in an OFDMA HE PPDU" of 802.11ax-2021, Table 36-21 "Subcarrier
        // allocation related constants for RUs in an OFDMA EHT PPDU" of 802.11be-2024)
        switch (bw)
        {
        case 20: // 242-tone RU
            return WifiTonePlan{256, 242, 234, 8, 6, 3};
        case 40: // 484-tone RU
            return WifiTonePlan{512, 484, 468, 16, 12, 5};
        case 80: // 996-tone RU
            return WifiTonePlan{1024, 996, 980, 16, 12, 5};
        case 160: // 2x996-tone RU
            return WifiTonePlan{2048, 1992, 1960, 32, 12, 5};
        case 320:
            if (modulation == WIFI_MOD_CLASS_EHT)
            {
                // 4x996-tone RU (Table 36-21 of 802.11be-2024)
                return WifiTonePlan{4096, 3984, 3920, 64, 12, 5};
            }
            return std::nullopt;
        default:
            return std::nullopt;
        }
    default:
        return std::nullopt;
    }
}

MHz_u
GetChannelWidthInMhz(WifiChannelWidthType width)
{
    switch (width)
    {
    case WifiChannelWidthType::UNKNOWN:
        return MHz_u{0};
    case WifiChannelWidthType::CW_20MHZ:
        return MHz_u{20};
    case WifiChannelWidthType::CW_22MHZ:
        return MHz_u{22};
    case WifiChannelWidthType::CW_5MHZ:
        return MHz_u{5};
    case WifiChannelWidthType::CW_10MHZ:
        return MHz_u{10};
    case WifiChannelWidthType::CW_40MHZ:
        return MHz_u{40};
    case WifiChannelWidthType::CW_80MHZ:
        return MHz_u{80};
    case WifiChannelWidthType::CW_160MHZ:
    case WifiChannelWidthType::CW_80_PLUS_80MHZ:
        return MHz_u{160};
    case WifiChannelWidthType::CW_320MHZ:
        return MHz_u{320};
    case WifiChannelWidthType::CW_2160MHZ:
        return MHz_u{2160};
    default:
        NS_FATAL_ERROR("Unknown wifi channel width type " << width);
        return MHz_u{0};
    }
}

bool
IsEht(WifiPreamble preamble)
{
    return ((preamble == WIFI_PREAMBLE_EHT_MU) || (preamble == WIFI_PREAMBLE_EHT_TB));
}

} // namespace ns3
