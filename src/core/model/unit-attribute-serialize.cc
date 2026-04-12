/*
 * Copyright (c) 2026 Tom Henderson
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include "unit-attribute-serialize.h"

#include <sstream>

namespace ns3
{

std::string
SerializeUnitValue(double numericValue, const char* suffix)
{
    std::ostringstream oss;
    oss << numericValue << "_" << suffix;
    return oss.str();
}

bool
DeserializeUnitValue(const std::string& str, const char* expectedSuffix, double& outValue)
{
    if (str.empty())
    {
        outValue = 0.0;
        return true;
    }
    std::istringstream iss(str);
    iss >> outValue;
    if (iss.fail())
    {
        return false;
    }
    if (iss.eof())
    {
        // Bare number -- assumed to be in the expected unit.
        return true;
    }
    // Optional suffix: "_dBm" (token format) or "dBm" (space-separated).
    std::string suffix;
    iss >> suffix;
    if (!iss.eof())
    {
        return false; // unexpected trailing content
    }
    if (!suffix.empty() && suffix[0] == '_')
    {
        suffix = suffix.substr(1);
    }
    return suffix == expectedSuffix;
}

} // namespace ns3
