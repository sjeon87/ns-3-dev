#ifndef UNITS_RATIO_H
#define UNITS_RATIO_H

// SI Derived Unit allows a dimensionless ratio.

#include "si-units-logger.h"
#include "si-units-parser.h"
#include "si-units-vector.h"
#include "units-aliases.h"
#include "units-frequency.h"
#include "units-time.h"

#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <iostream>
#include <optional>

// TODO(porce): Implement Per mille

namespace si_units
{

const double CENTUM{100.0}; ///< "By a hundred". Latin

struct percent_t;

/// Percentage
struct percent_t
{
    double val{}; ///< Value in percent

    percent_t() = default; ///< Default constructor

    /// Constructor
    /// @param val Value in degree
    constexpr explicit percent_t(int32_t val)
        : val(static_cast<double>(val))
    {
    }

    /// Constructor
    /// @param val Value in percent
    constexpr explicit percent_t(double val)
        : val(val)
    {
    }

    /// Constructor
    /// @param str a string representation of the percent
    explicit percent_t(const std::string& str)
    {
        auto res = from_str(str);
        UNITS_ASRT(res.has_value(), GetParseErrMsg(str, "percent"));
        val = res.value().val;
    }

    /// Represent in string
    /// @param space Insert or omit space between a number and a unit measurement, with SI standard as default
    /// @return String representation
    std::string str(bool space=true) const // NOLINT(readability-identifier-naming)
    {
        return sformat(space ? "%.1f %%" : "%.1f%%", val); // ISO 31-0 requires a non-breaking space
    }

    /// Constructor from ratio
    /// @param input Ratio
    /// @return Percentage
    static percent_t from_ratio(double input) // NOLINT(readability-identifier-naming)
    {
        return percent_t{input * CENTUM};
    }

    /// Convert to ratio
    /// @return Ratio
    double to_ratio() const // NOLINT(readability-identifier-naming)
    {
        return val / CENTUM;
    }

    /// Converts from a vector of doubles represented in ratio to a vector of percent_t
    /// @param input Vector of ratios
    /// @return Vector of percent_t
    static std::vector<percent_t> from_ratios(
        std::vector<double>& input) // NOLINT(readability-identifier-naming)
    {
        std::vector<percent_t> output(input.size());
        std::transform(input.cbegin(), input.cend(), output.begin(), [](double f) {
            return percent_t{f * CENTUM};
        });
        return output;
    }

    /// Converts from a vector of percent_t to a vector of doubles represented in ratio
    /// @param input Vector of percent_t
    /// @return Vector of ratios
    static std::vector<double> to_ratios(
        std::vector<percent_t>& input) // NOLINT(readability-identifier-naming)
    {
        std::vector<double> output(input.size());
        std::transform(input.cbegin(), input.cend(), output.begin(), [](percent_t f) {
            return static_cast<double>(f.val / CENTUM);
        });
        return output;
    }

    /// Three-way comparison
    /// @param rhs right hand side
    /// @return deduced comparison type
    auto operator<=>(const percent_t& rhs) const = default;

    /// Negation operator
    /// @return Negated value
    inline percent_t operator-() const
    {
        return percent_t{-val};
    }

    /// Addition operators
    /// @param rhs value to add
    /// @return Sum of values
    inline percent_t operator+(const percent_t& rhs) const
    {
        return percent_t{val + rhs.val};
    }

    /// Subtraction operators
    /// @param rhs value to subtract
    /// @return Difference of values
    inline percent_t operator-(const percent_t& rhs) const
    {
        return percent_t{val - rhs.val};
    }

    // In order not to cause a confusion, percent_t * percent_t is undefined.
    // TODO(porce): Enable percent_t * percent_t only after users are confident not to be confused.
    /// Multiplication operators
    /// @param rhs unitless value to multiply
    /// @return multiplied percent_t
    inline percent_t operator*(double rhs) const
    {
        return percent_t{val * rhs};
    }

    // In order not to cause a confusion, percent_t / percent_t is undefined.
    // TODO(porce): Enable percent_t / percent_t only after users are confident not to be confused.
    /// Division operators
    /// @param rhs unitless value to divide
    /// @return divided percent_t
    inline percent_t operator/(double rhs) const
    {
        return percent_t{val / rhs};
    }

    /// Addition assignment operators
    /// @param rhs value to add
    /// @return reference to this object
    inline percent_t& operator+=(const percent_t& rhs)
    {
        val += rhs.val;
        return *this;
    }

    /// Subtraction assignment operators
    /// @param rhs value to subtract
    /// @return reference to this object
    inline percent_t& operator-=(const percent_t& rhs)
    {
        val -= rhs.val;
        return *this;
    }

    // In order not to cause a confusion, *= percent_t is undefined.
    // TODO(porce): Enable *= percent_t only after users are confident not to be confused.
    /// Multiplication assignment operators
    /// @param rhs unitless value to multiply
    /// @return reference to this object
    inline percent_t& operator*=(double rhs)
    {
        val *= rhs;
        return *this;
    }

    // In order not to cause a confusion, /= percent_t is undefined.
    // TODO(porce): Enable /= percent_t only after users are confident not to be confused.
    /// Division assignment operators
    /// @param rhs unitless value to divide
    /// @return reference to this object
    inline percent_t& operator/=(double rhs)
    {
        val /= rhs;
        return *this;
    }

    /// Converts from a string
    /// @param input string to convert
    /// @return converted percent_t, or std::nullopt if conversion failed
    static std::optional<percent_t> from_str( // NOLINT(readability-identifier-naming)
        const std::string& input)
    {
        auto res = StringToDouble(input, "percent");
        if (res.has_value())
        {
            return std::optional(percent_t{res.value()});
        }
        res = StringToDouble(input, "%");
        if (res.has_value())
        {
            return std::optional(percent_t{res.value()});
        }
        return std::nullopt;
    }

};

/// Multiply operation
/// @param lhs a unitless value
/// @param rhs percent
/// @return multiplied percent
inline percent_t
operator*(const double& lhs, const percent_t& rhs)
{
    return percent_t{lhs * rhs.val};
}

/// User defined literals for percent
/// @param val percent value
/// @return percent_t object
inline percent_t
operator""_percent(long double val)
{
    return percent_t{static_cast<double>(val)};
}

/// User defined literals for percent
/// @param val percent value
/// @return percent_t object
inline percent_t
operator""_percent(unsigned long long val)
{
    return percent_t{static_cast<double>(val)};
}

/// Output stream operator for percent
/// @param os output stream
/// @param rhs percent
/// @return output stream
inline std::ostream&
operator<<(std::ostream& os, const percent_t& rhs)
{
    return os << rhs.str();
}

/// Input stream operator for percent
/// @param is input stream
/// @param rhs percent
/// @return input stream
inline std::istream&
operator>>(std::istream& is, percent_t& rhs)
{
    return ParseSIString(is, rhs, "percent");
}

// Vector support
using percentVec_t = SiUnitsVec<percent_t>;

} // namespace si_units

#endif // UNITS_RATIO_H
