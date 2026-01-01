#ifndef UNITS_FREQUENCY_NSTIME_H
#define UNITS_FREQUENCY_NSTIME_H

// Include si-units from third-party library
#include <si-units>

#include "ns3/nstime.h"

namespace si_units {

/// Multiplication operator
/// @param lhs The Hz_t object to multiply
/// @param nstime time
/// @return Unitless value
inline double
operator*(const Hz_t& lhs, ns3::Time nstime)
{
    // 64-bit double storage type is large enough to support Time's storage type and its
    // metric prefixes. 64-bit double supports upto 1.7e308. Time supports up to 2^63 - 1,
    // regardless of its metric prefix. The supported range of the return value large enough
    // for ns-3 This means upto 1.8e289 Hz_t is supported for this multiplication operation
    // This range does not qualify anything about the precision and accuracy.
    return (lhs.val * nstime.GetNanoSeconds()) / ONE_GIGA;
}

/// Multiply Hz_t by Time
/// @param nstime The Time to multiply by
/// @param rhs The Hz_t object to multiply
/// @return unitless value
inline double
operator*(ns3::Time nstime, const Hz_t& rhs)
{
    return rhs * nstime;
}

} // namespace si_units

#endif // UNITS_FREQUENCY_NSTIME_H
