#ifndef UNITS_TIME_H
#define UNITS_TIME_H

#include "si-units-parser.h"
#include "units-aliases.h"
#include "si-units-vector.h"

#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <iostream>
#include <math.h> // To support Linux. <cmath> lacks log10l().

namespace si_units
{

/// Nanoseconds
struct nSEC_t
{
    long long v{}; ///< Value in nanoseconds

    nSEC_t() = default; ///< Default constructor

    /// Constructor
    /// @param val Value in percent
    constexpr explicit nSEC_t(long long val)
        : v(val)
    {
    }

    /// Represents in a human-readable string
    /// @param space Insert or omit space between a number and a unit measurement, with SI standard as default
    /// @returns A string representation of the value in the most easy to read unit scale
    std::string str(bool space=true) const // NOLINT(readability-identifier-naming)
    {
        const std::vector<std::string> SUBUNIT_PREFIX = {"n", "u", "m", ""};

        size_t idx{};
        auto val = v;
        while (((idx + 1) < SUBUNIT_PREFIX.size()) && ((val % ONE_KILO) == 0))
        {
            val /= ONE_KILO;
            ++idx;
        }

        return sformat(space ? "%lld %sSEC" : "%lld%sSEC", val, SUBUNIT_PREFIX[idx].c_str());
    }

    /// Converts from integers represented in nanoseconds to a vector of nSEC_t
    /// @param input A vector of integers represented in nanoseconds
    /// @returns A vector of nSEC_t
    static std::vector<nSEC_t> from_ints(
        std::vector<int64_t>& input) // NOLINT(readability-identifier-naming)
    {
        std::vector<nSEC_t> output(input.size());
        std::transform(input.begin(), input.end(), output.begin(), [](int64_t f) {
            return nSEC_t{f};
        });
        return output;
    }

    /// Converts from vectors of nSEC_t to integers represented in nanoseconds
    /// @param input A vector of nSEC_t
    /// @returns A vector of integers represented in nanoseconds
    static std::vector<int64_t> to_ints(
        std::vector<nSEC_t>& input) // NOLINT(readability-identifier-naming)
    {
        std::vector<int64_t> output(input.size());
        std::transform(input.begin(), input.end(), output.begin(), [](nSEC_t f) {
            return static_cast<int64_t>(f.v);
        });
        return output;
    }

    /// Three-way comparison
    /// @param rhs right hand side
    /// @return deduced comparison type
    auto operator<=>(const nSEC_t& rhs) const = default;

    /// Negation operator
    /// @returns Negated value
    inline nSEC_t operator-() const
    {
        return nSEC_t{-v};
    }

    /// Addition operators
    /// @param rhs value to add
    /// @returns Sum of values
    inline nSEC_t operator+(const nSEC_t& rhs) const
    {
        return nSEC_t{v + rhs.v};
    }

    /// Subtraction operators
    /// @param rhs value to subtract
    /// @returns Difference of values
    inline nSEC_t operator-(const nSEC_t& rhs) const
    {
        return nSEC_t{v - rhs.v};
    }

    /// Addition assignment operators
    /// @param rhs value to add
    /// @returns Reference to this
    inline nSEC_t& operator+=(const nSEC_t& rhs)
    {
        v += rhs.v;
        return *this;
    }

    /// Subtraction assignment operators
    /// @param rhs value to subtract
    /// @returns Reference to this
    inline nSEC_t& operator-=(const nSEC_t& rhs)
    {
        v -= rhs.v;
        return *this;
    }

    /// Get the value in microseconds
    /// @returns Value in microseconds
    inline long long in_usec() const // NOLINT(readability-identifier-naming)
    {
        return v / ONE_KILO;
    }

    /// Get the value in milliseconds
    /// @returns Value in milliseconds
    inline long long in_msec() const // NOLINT(readability-identifier-naming)
    {
        return v / ONE_MEGA;
    }

    /// Get the value in seconds
    /// @returns Value in seconds
    inline long long in_sec() const // NOLINT(readability-identifier-naming)
    {
        return v / ONE_GIGA;
    }
};


/// User defined literals for nanoseconds
/// @param v value in nanoseconds
/// @return nSEC_t object
inline nSEC_t
operator""_nSEC(unsigned long long v)
{
    return nSEC_t{static_cast<long long>(v)};
}

/// User defined literals for nanoseconds
/// @param v value in nanoseconds
/// @return nSEC_t object
inline nSEC_t
operator""_nSEC(long double v)
{
    return nSEC_t{static_cast<long long>(v)};
}

/// User defined literals for microseconds
/// @param v value in microseconds
/// @return nSEC_t object
inline nSEC_t
operator""_uSEC(unsigned long long v)
{
    return nSEC_t{static_cast<long long>(v * ONE_KILO)};
}

/// User defined literals for microseconds
/// @param v value in microseconds
/// @return nSEC_t object
inline nSEC_t
operator""_uSEC(long double v)
{
    return nSEC_t{static_cast<long long>(v * ONE_KILO)};
}

/// User defined literals for milliseconds
/// @param v value in milliseconds
/// @return nSEC_t object
inline nSEC_t
operator""_mSEC(unsigned long long v)
{
    return nSEC_t{static_cast<long long>(v * ONE_MEGA)};
}

/// User defined literals for milliseconds
/// @param v value in milliseconds
/// @return nSEC_t object
inline nSEC_t
operator""_mSEC(long double v)
{
    return nSEC_t{static_cast<long long>(v * ONE_MEGA)};
}

/// User defined literals for seconds
/// @param v value in seconds
/// @return nSEC_t object
inline nSEC_t
operator""_SEC(unsigned long long v)
{
    return nSEC_t{static_cast<long long>(v * ONE_GIGA)};
}

/// User defined literals for seconds
/// @param v value in seconds
/// @return nSEC_t object
inline nSEC_t
operator""_SEC(long double v)
{
    return nSEC_t{static_cast<long long>(v * ONE_GIGA)};
}

/// Output stream operator for nSEC_t
/// @param os output stream
/// @param q nSEC_t object
/// @return output stream
inline std::ostream&
operator<<(std::ostream& os, const nSEC_t& q)
{
    return os << q.str();
}

// Note other strong types use istream
// TODO(porce): Explore if this type also converge to the same handling, including input argument
// with suffix string
/// Input stream operator for nSEC_t
/// @param is input stream
/// @param q nSEC_t object
/// @return input stream
inline std::istream&
operator>>(std::istream& is, nSEC_t& q)
{
    is >> q.v;
    return is;
}

// Vector support
using nSECVec_t = SiUnitsVec<nSEC_t>;

} // namespace si_units

#endif // UNITS_TIME_H
