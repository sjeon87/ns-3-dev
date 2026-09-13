/*
 * Copyright (c) 2026 Tom Henderson
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

/**
 * @file
 * @brief ns-3 extensions for mp-units: logarithmic unit wrappers and type aliases
 *
 * mp-units (https://mpusz.github.io/mp-units/) is a C++20 quantities and units library
 * targeting C++29 standardization. It provides compile-time dimensional analysis for
 * linear SI units but does NOT support logarithmic units (dB, dBm, dBW).
 *
 * This header provides:
 * 1. Wrapper classes (dB_t, dBm_t, dBW_t, dBm_per_Hz_t, dBm_per_MHz_t) for logarithmic
 *    quantities with type-safe arithmetic (e.g., dBm + dB = dBm, dBm + dBm = deleted)
 * 2. Type aliases in namespace ns3 for mp-units linear quantities (MHz_t, Watt_t, etc.)
 * 3. Explicit conversion between linear mp-units quantities and logarithmic wrappers
 * 4. Multiplication-literal construction (e.g., 3.0 * dB, -82.0 * dBm) matching mp-units
 *    style (20.0 * MHz)
 * 5. Stream I/O operators for all types
 *
 * Construction: Like mp-units, bare numeric construction is not allowed. Use
 * multiplication literals: 3.0 * dB, -82.0 * dBm, -10.0 * dBW. For linear-to-log
 * conversion, use the public constructors: dB_t{scalar_t{ratio}}, dBm_t{mWatt_t{...}}.
 *
 * Value extraction: Use .numerical_value_in(unit) with the matching unit tag:
 * x.numerical_value_in(dB), x.numerical_value_in(dBm), etc.
 *
 * If mp-units adds native logarithmic unit support in a future version, these wrapper
 * classes can be replaced with type aliases to the native types without changing the
 * ns3 API surface.
 */

#ifndef NS3_UNITS_H
#define NS3_UNITS_H

#include <cmath>
#include <compare>
#include <iostream>
#include <mp-units/systems/isq.h>
#include <mp-units/systems/si.h>
#include <string>

namespace ns3
{

// ============================================================================
// Linear unit type aliases using mp-units
// ============================================================================

/// Watt quantity (double)
using Watt_t = mp_units::quantity<mp_units::si::watt, double>;
/// Milliwatt quantity (double)
using mWatt_t = mp_units::quantity<mp_units::si::milli<mp_units::si::watt>, double>;
/// Hertz quantity (double)
using Hz_t = mp_units::quantity<mp_units::si::hertz, double>;
/// Megahertz quantity (double)
using MHz_t = mp_units::quantity<mp_units::si::mega<mp_units::si::hertz>, double>;
/// Metre quantity (double)
using meter_t = mp_units::quantity<mp_units::si::metre, double>;
/// Ampere quantity (double)
using ampere_t = mp_units::quantity<mp_units::si::ampere, double>;
/// Volt quantity (double)
using volt_t = mp_units::quantity<mp_units::si::volt, double>;
/// Joule quantity (double)
using joule_t = mp_units::quantity<mp_units::si::joule, double>;
/// Dimensionless scalar quantity (double)
using scalar_t = mp_units::quantity<mp_units::one, double>;
/// Angle in degrees (double)
using degree_t = mp_units::quantity<mp_units::si::degree, double>;
/// Milliwatt per Hertz -- linear power spectral density (double)
using mW_per_Hz_t =
    mp_units::quantity<mp_units::si::milli<mp_units::si::watt> / mp_units::si::hertz, double>;
/// Milliwatt per Megahertz -- linear power spectral density (double)
using mW_per_MHz_t = mp_units::quantity<mp_units::si::milli<mp_units::si::watt> /
                                            mp_units::si::mega<mp_units::si::hertz>,
                                        double>;

// ============================================================================
// Unit symbols for use in headers
// ============================================================================
//
// Headers should not use 'using namespace mp_units::si::unit_symbols' because
// it leaks the entire symbol set into every includer.  The selective imports
// below make the unit symbols most commonly needed in ns-3 model library
// headers available to any header that includes units.h, so that default
// argument values and constexpr initializers can be written concisely:
//
//     MHz_t bw = 20.0 * MHz        // instead of 20.0 * mp_units::si::mega<mp_units::si::hertz>
//
// If a needed SI unit symbol is not provided here, authors may add a selective
// 'using' declaration in their own header, or use the fully qualified form
// (e.g., mp_units::si::unit_symbols::kHz).
//
// .cc files may continue to use 'using namespace mp_units::si::unit_symbols;'
// for the full set of symbols.

using mp_units::si::unit_symbols::GHz;
using mp_units::si::unit_symbols::Hz;
using mp_units::si::unit_symbols::MHz;
using mp_units::si::unit_symbols::mW;
using mp_units::si::unit_symbols::W;

// ============================================================================
// Logarithmic unit tag types and constants
// ============================================================================
//
// These serve as both:
// 1. Arguments to numerical_value_in(): x.numerical_value_in(dB)
// 2. Multiplication-literal operands: 3.0 * dB, -82.0 * dBm
//
// This mirrors the mp-units pattern where Hz, MHz, etc. serve the same dual
// role for linear quantities.

/// Tag type for decibel unit
struct dB_unit
{
};

/// Tag type for dBm unit
struct dBm_unit
{
};

/// Tag type for dBW unit
struct dBW_unit
{
};

/// Tag type for dBm/Hz unit
struct dBm_per_Hz_unit
{
};

/// Tag type for dBm/MHz unit
struct dBm_per_MHz_unit
{
};

/// Unit symbol constant for dB
inline constexpr dB_unit dB{};
/// Unit symbol constant for dBm
inline constexpr dBm_unit dBm{};
/// Unit symbol constant for dBW
inline constexpr dBW_unit dBW{};
/// Unit symbol constant for dBm/Hz
inline constexpr dBm_per_Hz_unit dBm_per_Hz{};
/// Unit symbol constant for dBm/MHz
inline constexpr dBm_per_MHz_unit dBm_per_MHz{};

// Forward declarations for logarithmic wrapper types
class dB_t;
class dBm_t;
class dBW_t;
class dBm_per_Hz_t;
class dBm_per_MHz_t;

/// dBr strong type: relative level in dB (alias for dB_t)
using dBr_t = dB_t;

// ============================================================================
// dB_t -- dimensionless relative logarithmic quantity
// ============================================================================

/**
 * @brief Wrapper for decibel (dB) values -- dimensionless, relative quantity
 *
 * dB represents a ratio on a logarithmic scale. Arithmetic rules:
 * - dB + dB = dB (adding relative quantities)
 * - dB - dB = dB
 * - dBm + dB = dBm (adding relative to absolute)
 * - dBm - dBm = dB (difference of absolutes = relative)
 *
 * Construction: use multiplication literals (3.0 * dB) or linear conversion
 * (dB_t{scalar_t{ratio}}). Direct construction from numeric types is private.
 *
 * Value extraction: use .numerical_value_in(dB).
 */
class dB_t
{
    double m_value{0}; ///< Stored numeric value, interpreted as a logarithmic ratio in decibels

    /**
     * Private: construct from a raw numeric value (used by friends only).
     * @param v Numeric value, interpreted as a logarithmic ratio in decibels.
     */
    constexpr explicit dB_t(double v)
        : m_value(v)
    {
    }

    // Friends that need private constructor
    friend constexpr dB_t operator*(double v, dB_unit);
    friend constexpr dB_t operator*(int v, dB_unit);
    friend class dBm_t;
    friend class dBW_t;
    friend class DbChecker; ///< attribute checker needs Type(double)

  public:
    constexpr dB_t() = default;

    /**
     * Construct from linear power ratio (linear to logarithmic conversion): 10*log10(ratio).
     * @param s The linear ratio as a dimensionless mp-units quantity.
     */
    explicit dB_t(scalar_t s)
        : m_value(10.0 * std::log10(s.numerical_value_in(mp_units::one)))
    {
    }

    /**
     * Get the numeric value in dB.
     * @return The stored numeric value (logarithmic ratio in decibels).
     */
    [[nodiscard]] constexpr double numerical_value_in(dB_unit) const
    {
        return m_value;
    }

    /// Convert to linear power ratio: 10^(value/10)
    [[nodiscard]] explicit operator scalar_t() const
    {
        return std::pow(10.0, m_value / 10.0) * mp_units::one;
    }

    /**
     * Convert to linear power ratio: 10^(value/10).
     * @return The linear power ratio as a plain double.
     */
    [[nodiscard]] double to_linear_power() const
    {
        return std::pow(10.0, m_value / 10.0);
    }

    /**
     * Convert to linear amplitude ratio: 10^(value/20).
     * @return The linear amplitude ratio as a plain double.
     */
    [[nodiscard]] double to_linear_amplitude() const
    {
        return std::pow(10.0, m_value / 20.0);
    }

    /**
     * dB + dB yields dB.
     * @param a Left operand.
     * @param b Right operand.
     * @return Sum in dB.
     */
    friend constexpr dB_t operator+(dB_t a, dB_t b)
    {
        return dB_t(a.m_value + b.m_value);
    }

    /**
     * dB - dB yields dB.
     * @param a Left operand.
     * @param b Right operand.
     * @return Difference in dB.
     */
    friend constexpr dB_t operator-(dB_t a, dB_t b)
    {
        return dB_t(a.m_value - b.m_value);
    }

    /**
     * Compound assignment: add a dB value.
     * @param rhs Right-hand side.
     * @return Reference to this.
     */
    constexpr dB_t& operator+=(dB_t rhs)
    {
        m_value += rhs.m_value;
        return *this;
    }

    /**
     * Compound assignment: subtract a dB value.
     * @param rhs Right-hand side.
     * @return Reference to this.
     */
    constexpr dB_t& operator-=(dB_t rhs)
    {
        m_value -= rhs.m_value;
        return *this;
    }

    /**
     * Unary minus.
     * @return Negated value in dB.
     */
    constexpr dB_t operator-() const
    {
        return dB_t(-m_value);
    }

    /**
     * dB * scalar yields dB (scale the dB value; e.g., n steps * dB/step = dB).
     * @param a The dB value.
     * @param n The scalar factor.
     * @return Scaled dB value.
     */
    friend constexpr dB_t operator*(dB_t a, double n)
    {
        return dB_t(a.m_value * n);
    }

    /**
     * scalar * dB yields dB (commutative).
     * @param n The scalar factor.
     * @param a The dB value.
     * @return Scaled dB value.
     */
    friend constexpr dB_t operator*(double n, dB_t a)
    {
        return dB_t(n * a.m_value);
    }

    /**
     * dB / scalar yields dB (e.g., total_dB / num_steps = dB/step).
     * @param a The dB value.
     * @param n The scalar divisor.
     * @return Divided dB value.
     */
    friend constexpr dB_t operator/(dB_t a, double n)
    {
        return dB_t(a.m_value / n);
    }

    /**
     * dB / dB yields scalar_t (ratio of two dB values; e.g., range / step = number of steps).
     * @param a Numerator in dB.
     * @param b Denominator in dB.
     * @return Dimensionless ratio.
     */
    friend scalar_t operator/(dB_t a, dB_t b)
    {
        return (a.m_value / b.m_value) * mp_units::one;
    }

    /**
     * Three-way comparison.
     * @param a Left operand.
     * @param b Right operand.
     * @return Ordering result.
     */
    friend constexpr auto operator<=>(dB_t a, dB_t b) = default;

    /**
     * Equality comparison.
     * @param a Left operand.
     * @param b Right operand.
     * @return True if equal.
     */
    friend constexpr bool operator==(dB_t a, dB_t b) = default;

    /**
     * Less-than-or-equal comparison.
     * @param a Left operand.
     * @param b Right operand.
     * @return True if a <= b.
     */
    friend constexpr bool operator<=(dB_t a, dB_t b)
    {
        return a.m_value <= b.m_value;
    }

    /**
     * Greater-than-or-equal comparison.
     * @param a Left operand.
     * @param b Right operand.
     * @return True if a >= b.
     */
    friend constexpr bool operator>=(dB_t a, dB_t b)
    {
        return a.m_value >= b.m_value;
    }

    /// Stream insertion: prints as "3 dB"
    friend std::ostream& operator<<(std::ostream& os, dB_t v)
    {
        os << v.m_value << " dB";
        return os;
    }

    /// Stream extraction: accepts "10.0_dB" or "10.0"
    friend std::istream& operator>>(std::istream& is, dB_t& v)
    {
        std::string token;
        is >> token;
        const auto pos = token.find('_');
        std::string number;
        if (pos == std::string::npos)
        {
            number = token;
        }
        else
        {
            auto unit = token.substr(pos + 1);
            if (unit != "dB")
            {
                is.setstate(std::ios_base::failbit);
                return is;
            }
            number = token.substr(0, pos);
        }
        v = dB_t(std::strtod(number.c_str(), nullptr));
        return is;
    }
};

/**
 * mp-units style literal for a dB_t. Example: dB_t value = 3.0 * dB;
 * @param v Numeric value, interpreted as a logarithmic ratio in decibels.
 * @return dB_t wrapping v.
 */
constexpr dB_t
operator*(double v, dB_unit)
{
    return dB_t(v);
}

/**
 * mp-units style literal for a dB_t (integer overload). Example: dB_t value = 3 * dB;
 * @param v Numeric value, interpreted as a logarithmic ratio in decibels.
 * @return dB_t wrapping v as a double.
 */
constexpr dB_t
operator*(int v, dB_unit)
{
    return dB_t(static_cast<double>(v));
}

// ============================================================================
// dBm_t -- absolute power in dBm (decibels relative to 1 milliwatt)
// ============================================================================

/**
 * @brief Wrapper for dBm values -- absolute power on a logarithmic scale
 *
 * dBm represents absolute power: 0 dBm = 1 mW. Arithmetic rules:
 * - dBm + dB = dBm
 * - dBm - dB = dBm
 * - dBm - dBm = dB (difference of absolute = relative)
 * - dBm + dBm = DELETED (not physically meaningful)
 *
 * Construction: use multiplication literals (-82.0 * dBm) or linear conversion
 * (dBm_t{mWatt_t{...}}, dBm_t{Watt_t{...}}). Direct construction from numeric
 * types is private.
 *
 * Value extraction: use .numerical_value_in(dBm).
 */
class dBm_t
{
    double m_value{0}; ///< Stored numeric value, interpreted as power in dBm

    /**
     * Private: construct from a raw numeric value (used by friends only).
     * @param v Numeric value, interpreted as power in dBm.
     */
    constexpr explicit dBm_t(double v)
        : m_value(v)
    {
    }

    // Friends that need private constructor
    friend constexpr dBm_t operator*(double v, dBm_unit);
    friend constexpr dBm_t operator*(int v, dBm_unit);
    friend class dBW_t;
    friend class DbmChecker; ///< attribute checker needs Type(double)

  public:
    constexpr dBm_t() = default;

    /**
     * Construct from milliwatt (linear to logarithmic conversion).
     * @param mw Power in milliwatts.
     */
    explicit dBm_t(mWatt_t mw)
        : m_value(10.0 *
                  std::log10(mw.numerical_value_ref_in(mp_units::si::milli<mp_units::si::watt>)))
    {
    }

    /**
     * Construct from watt (linear to logarithmic conversion).
     * @param w Power in watts.
     */
    explicit dBm_t(Watt_t w)
        : m_value(10.0 * std::log10(w.numerical_value_ref_in(mp_units::si::watt) * 1000.0))
    {
    }

    /**
     * Get the numeric value in dBm.
     * @return The stored numeric value (power in dBm).
     */
    [[nodiscard]] constexpr double numerical_value_in(dBm_unit) const
    {
        return m_value;
    }

    /// Convert to milliwatts
    [[nodiscard]] explicit operator mWatt_t() const
    {
        return std::pow(10.0, m_value / 10.0) * mp_units::si::milli<mp_units::si::watt>;
    }

    /// Convert to watts
    [[nodiscard]] explicit operator Watt_t() const
    {
        return std::pow(10.0, (m_value - 30.0) / 10.0) * mp_units::si::watt;
    }

    /**
     * dBm + dB yields dBm.
     * @param a dBm operand.
     * @param b dB operand.
     * @return Sum in dBm.
     */
    friend constexpr dBm_t operator+(dBm_t a, dB_t b)
    {
        return dBm_t(a.m_value + b.numerical_value_in(dB));
    }

    /**
     * dB + dBm yields dBm (commutative).
     * @param a dB operand.
     * @param b dBm operand.
     * @return Sum in dBm.
     */
    friend constexpr dBm_t operator+(dB_t a, dBm_t b)
    {
        return dBm_t(a.numerical_value_in(dB) + b.m_value);
    }

    /**
     * dBm - dB yields dBm.
     * @param a dBm operand.
     * @param b dB operand.
     * @return Difference in dBm.
     */
    friend constexpr dBm_t operator-(dBm_t a, dB_t b)
    {
        return dBm_t(a.m_value - b.numerical_value_in(dB));
    }

    /**
     * dBm - dBm yields dB (difference of absolute powers becomes a relative ratio).
     * @param a Left dBm operand.
     * @param b Right dBm operand.
     * @return Difference in dB.
     */
    friend constexpr dB_t operator-(dBm_t a, dBm_t b)
    {
        return (a.m_value - b.m_value) * dB;
    }

    // dBm + dBm = DELETED (not physically meaningful)
    friend constexpr void operator+(dBm_t, dBm_t) = delete;

    /**
     * Compound assignment: add a dB value.
     * @param rhs Right-hand side in dB.
     * @return Reference to this.
     */
    constexpr dBm_t& operator+=(dB_t rhs)
    {
        m_value += rhs.m_value;
        return *this;
    }

    /**
     * Compound assignment: subtract a dB value.
     * @param rhs Right-hand side in dB.
     * @return Reference to this.
     */
    constexpr dBm_t& operator-=(dB_t rhs)
    {
        m_value -= rhs.m_value;
        return *this;
    }

    /**
     * Three-way comparison.
     * @param a Left operand.
     * @param b Right operand.
     * @return Ordering result.
     */
    friend constexpr auto operator<=>(dBm_t a, dBm_t b) = default;

    /**
     * Equality comparison.
     * @param a Left operand.
     * @param b Right operand.
     * @return True if equal.
     */
    friend constexpr bool operator==(dBm_t a, dBm_t b) = default;

    /**
     * Less-than-or-equal comparison.
     * @param a Left operand.
     * @param b Right operand.
     * @return True if a <= b.
     */
    friend constexpr bool operator<=(dBm_t a, dBm_t b)
    {
        return a.m_value <= b.m_value;
    }

    /**
     * Greater-than-or-equal comparison.
     * @param a Left operand.
     * @param b Right operand.
     * @return True if a >= b.
     */
    friend constexpr bool operator>=(dBm_t a, dBm_t b)
    {
        return a.m_value >= b.m_value;
    }

    /// Stream insertion: prints as "20 dBm"
    friend std::ostream& operator<<(std::ostream& os, dBm_t v)
    {
        os << v.m_value << " dBm";
        return os;
    }

    /// Stream extraction: accepts "-72.0_dBm" or "-72.0"
    friend std::istream& operator>>(std::istream& is, dBm_t& v)
    {
        std::string token;
        is >> token;
        const auto pos = token.find('_');
        std::string number;
        if (pos == std::string::npos)
        {
            number = token;
        }
        else
        {
            auto unit = token.substr(pos + 1);
            if (unit != "dBm")
            {
                is.setstate(std::ios_base::failbit);
                return is;
            }
            number = token.substr(0, pos);
        }
        v = dBm_t(std::strtod(number.c_str(), nullptr));
        return is;
    }
};

/**
 * mp-units style literal for a dBm_t. Example: dBm_t p = -82.0 * dBm;
 * @param v Numeric value, interpreted as power in dBm (decibels relative to 1 mW).
 * @return dBm_t wrapping v.
 */
constexpr dBm_t
operator*(double v, dBm_unit)
{
    return dBm_t(v);
}

/**
 * mp-units style literal for a dBm_t (integer overload). Example: dBm_t p = 0 * dBm;
 * @param v Numeric value, interpreted as power in dBm (decibels relative to 1 mW).
 * @return dBm_t wrapping v as a double.
 */
constexpr dBm_t
operator*(int v, dBm_unit)
{
    return dBm_t(static_cast<double>(v));
}

// ============================================================================
// dBW_t -- absolute power in dBW (decibels relative to 1 watt)
// ============================================================================

/**
 * @brief Wrapper for dBW values -- absolute power on a logarithmic scale
 *
 * dBW represents absolute power: 0 dBW = 1 W = 30 dBm.
 * Same arithmetic rules as dBm_t.
 *
 * Construction: use multiplication literals (-10.0 * dBW) or linear conversion
 * (dBW_t{Watt_t{...}}, dBW_t{mWatt_t{...}}, dBW_t{dBm_t{...}}).
 * Direct construction from numeric types is private.
 *
 * Value extraction: use .numerical_value_in(dBW).
 */
class dBW_t
{
    double m_value{0}; ///< Stored numeric value, interpreted as power in dBW

    /**
     * Private: construct from a raw numeric value (used by friends only).
     * @param v Numeric value, interpreted as power in dBW.
     */
    constexpr explicit dBW_t(double v)
        : m_value(v)
    {
    }

    // Friends that need private constructor
    friend constexpr dBW_t operator*(double v, dBW_unit);
    friend constexpr dBW_t operator*(int v, dBW_unit);

  public:
    constexpr dBW_t() = default;

    /**
     * Construct from dBm (dBW = dBm - 30).
     * @param dbm Power in dBm.
     */
    constexpr explicit dBW_t(dBm_t dbm)
        : m_value(dbm.m_value - 30.0)
    {
    }

    /**
     * Construct from watt (linear to logarithmic conversion).
     * @param w Power in watts.
     */
    explicit dBW_t(Watt_t w)
        : m_value(10.0 * std::log10(w.numerical_value_ref_in(mp_units::si::watt)))
    {
    }

    /**
     * Construct from milliwatt (linear to logarithmic conversion).
     * @param mw Power in milliwatts.
     */
    explicit dBW_t(mWatt_t mw)
        : m_value(10.0 *
                  std::log10(mw.numerical_value_ref_in(mp_units::si::milli<mp_units::si::watt>) /
                             1000.0))
    {
    }

    /**
     * Get the numeric value in dBW.
     * @return The stored numeric value (power in dBW).
     */
    [[nodiscard]] constexpr double numerical_value_in(dBW_unit) const
    {
        return m_value;
    }

    /**
     * Convert to dBm.
     * @return Equivalent power in dBm (dBm = dBW + 30).
     */
    [[nodiscard]] constexpr explicit operator dBm_t() const
    {
        return dBm_t(m_value + 30.0);
    }

    /// Convert to watts
    [[nodiscard]] explicit operator Watt_t() const
    {
        return std::pow(10.0, m_value / 10.0) * mp_units::si::watt;
    }

    /// Convert to milliwatts
    [[nodiscard]] explicit operator mWatt_t() const
    {
        return std::pow(10.0, (m_value + 30.0) / 10.0) * mp_units::si::milli<mp_units::si::watt>;
    }

    /**
     * dBW + dB yields dBW.
     * @param a dBW operand.
     * @param b dB operand.
     * @return Sum in dBW.
     */
    friend constexpr dBW_t operator+(dBW_t a, dB_t b)
    {
        return dBW_t(a.m_value + b.numerical_value_in(dB));
    }

    /**
     * dB + dBW yields dBW (commutative).
     * @param a dB operand.
     * @param b dBW operand.
     * @return Sum in dBW.
     */
    friend constexpr dBW_t operator+(dB_t a, dBW_t b)
    {
        return dBW_t(a.numerical_value_in(dB) + b.m_value);
    }

    /**
     * dBW - dB yields dBW.
     * @param a dBW operand.
     * @param b dB operand.
     * @return Difference in dBW.
     */
    friend constexpr dBW_t operator-(dBW_t a, dB_t b)
    {
        return dBW_t(a.m_value - b.numerical_value_in(dB));
    }

    /**
     * dBW - dBW yields dB (difference of absolute powers becomes a relative ratio).
     * @param a Left dBW operand.
     * @param b Right dBW operand.
     * @return Difference in dB.
     */
    friend constexpr dB_t operator-(dBW_t a, dBW_t b)
    {
        return (a.m_value - b.m_value) * dB;
    }

    // dBW + dBW = DELETED
    friend constexpr void operator+(dBW_t, dBW_t) = delete;

    // dBm + dBW = DELETED
    friend constexpr void operator+(dBm_t, dBW_t) = delete;

    // dBW + dBm = DELETED
    friend constexpr void operator+(dBW_t, dBm_t) = delete;

    /**
     * Compound assignment: add a dB value.
     * @param rhs Right-hand side in dB.
     * @return Reference to this.
     */
    constexpr dBW_t& operator+=(dB_t rhs)
    {
        m_value += rhs.m_value;
        return *this;
    }

    /**
     * Compound assignment: subtract a dB value.
     * @param rhs Right-hand side in dB.
     * @return Reference to this.
     */
    constexpr dBW_t& operator-=(dB_t rhs)
    {
        m_value -= rhs.m_value;
        return *this;
    }

    /**
     * Three-way comparison.
     * @param a Left operand.
     * @param b Right operand.
     * @return Ordering result.
     */
    friend constexpr auto operator<=>(dBW_t a, dBW_t b) = default;

    /**
     * Equality comparison.
     * @param a Left operand.
     * @param b Right operand.
     * @return True if equal.
     */
    friend constexpr bool operator==(dBW_t a, dBW_t b) = default;

    /**
     * Less-than-or-equal comparison.
     * @param a Left operand.
     * @param b Right operand.
     * @return True if a <= b.
     */
    friend constexpr bool operator<=(dBW_t a, dBW_t b)
    {
        return a.m_value <= b.m_value;
    }

    /**
     * Greater-than-or-equal comparison.
     * @param a Left operand.
     * @param b Right operand.
     * @return True if a >= b.
     */
    friend constexpr bool operator>=(dBW_t a, dBW_t b)
    {
        return a.m_value >= b.m_value;
    }

    /// Stream insertion: prints as "-10 dBW"
    friend std::ostream& operator<<(std::ostream& os, dBW_t v)
    {
        os << v.m_value << " dBW";
        return os;
    }

    /// Stream extraction: accepts "30.0_dBW" or "30.0"
    friend std::istream& operator>>(std::istream& is, dBW_t& v)
    {
        std::string token;
        is >> token;
        const auto pos = token.find('_');
        std::string number;
        if (pos == std::string::npos)
        {
            number = token;
        }
        else
        {
            auto unit = token.substr(pos + 1);
            if (unit != "dBW")
            {
                is.setstate(std::ios_base::failbit);
                return is;
            }
            number = token.substr(0, pos);
        }
        v = dBW_t(std::strtod(number.c_str(), nullptr));
        return is;
    }
};

/**
 * mp-units style literal for a dBW_t. Example: dBW_t p = -10.0 * dBW;
 * @param v Numeric value, interpreted as power in dBW (decibels relative to 1 W).
 * @return dBW_t wrapping v.
 */
constexpr dBW_t
operator*(double v, dBW_unit)
{
    return dBW_t(v);
}

/**
 * mp-units style literal for a dBW_t (integer overload). Example: dBW_t p = 0 * dBW;
 * @param v Numeric value, interpreted as power in dBW (decibels relative to 1 W).
 * @return dBW_t wrapping v as a double.
 */
constexpr dBW_t
operator*(int v, dBW_unit)
{
    return dBW_t(static_cast<double>(v));
}

// ============================================================================
// dBm_per_Hz_t -- power spectral density in dBm/Hz
// ============================================================================

/**
 * @brief Wrapper for power spectral density in dBm/Hz
 *
 * Supports multiplication by frequency to yield total power:
 * - dBm_per_Hz * Hz = dBm (in logarithmic domain: PSD_linear * BW_linear yields power_linear,
 *   expressed as dBm)
 *
 * Construction: use multiplication literals (-50.0 * dBm_per_Hz).
 * Value extraction: use .numerical_value_in(dBm_per_Hz).
 */
class dBm_per_Hz_t
{
    double m_value{0}; ///< Stored numeric value, interpreted as power spectral density in dBm/Hz

    /**
     * Private: construct from a raw numeric value (used by friends only).
     * @param v Numeric value, interpreted as power spectral density in dBm/Hz.
     */
    constexpr explicit dBm_per_Hz_t(double v)
        : m_value(v)
    {
    }

    // Friends that need private constructor
    friend constexpr dBm_per_Hz_t operator*(double v, dBm_per_Hz_unit);
    friend constexpr dBm_per_Hz_t operator*(int v, dBm_per_Hz_unit);

  public:
    constexpr dBm_per_Hz_t() = default;

    /**
     * Get the numeric value in dBm/Hz.
     * @return The stored numeric value (power spectral density in dBm/Hz).
     */
    [[nodiscard]] constexpr double numerical_value_in(dBm_per_Hz_unit) const
    {
        return m_value;
    }

    /**
     * Three-way comparison.
     * @param a Left operand.
     * @param b Right operand.
     * @return Ordering result.
     */
    friend constexpr auto operator<=>(dBm_per_Hz_t a, dBm_per_Hz_t b) = default;

    /**
     * Equality comparison.
     * @param a Left operand.
     * @param b Right operand.
     * @return True if equal.
     */
    friend constexpr bool operator==(dBm_per_Hz_t a, dBm_per_Hz_t b) = default;

    /// dBm_per_Hz * Hz = dBm (linear domain multiplication, result in dBm)
    friend dBm_t operator*(dBm_per_Hz_t psd, Hz_t bw)
    {
        double psd_linear_mw_per_hz = std::pow(10.0, psd.m_value / 10.0);
        double bw_hz = bw.numerical_value_ref_in(mp_units::si::hertz);
        double power_mw = psd_linear_mw_per_hz * bw_hz;
        return 10.0 * std::log10(power_mw) * dBm;
    }

    /// Hz * dBm_per_Hz = dBm (commutative)
    friend dBm_t operator*(Hz_t bw, dBm_per_Hz_t psd)
    {
        return psd * bw;
    }

    /**
     * Stream insertion: prints as "X dBm/Hz".
     * @param os Output stream.
     * @param v The dBm_per_Hz_t value.
     * @return Reference to the stream.
     */
    friend std::ostream& operator<<(std::ostream& os, dBm_per_Hz_t v)
    {
        os << v.m_value << " dBm/Hz";
        return os;
    }

    /**
     * Stream extraction: accepts "X_dBm_per_Hz" or "X".
     * @param is Input stream.
     * @param v The dBm_per_Hz_t to populate.
     * @return Reference to the stream.
     */
    friend std::istream& operator>>(std::istream& is, dBm_per_Hz_t& v)
    {
        std::string token;
        is >> token;
        const auto pos = token.find('_');
        std::string number;
        if (pos == std::string::npos)
        {
            number = token;
        }
        else
        {
            auto unit = token.substr(pos + 1);
            if (unit != "dBm_per_Hz")
            {
                is.setstate(std::ios_base::failbit);
                return is;
            }
            number = token.substr(0, pos);
        }
        v = dBm_per_Hz_t(std::strtod(number.c_str(), nullptr));
        return is;
    }
};

/**
 * mp-units style literal for a dBm_per_Hz_t. Example: dBm_per_Hz_t psd = -174.0 * dBm_per_Hz;
 * @param v Numeric value, interpreted as power spectral density in dBm/Hz.
 * @return dBm_per_Hz_t wrapping v.
 */
constexpr dBm_per_Hz_t
operator*(double v, dBm_per_Hz_unit)
{
    return dBm_per_Hz_t(v);
}

/**
 * mp-units style literal for a dBm_per_Hz_t (integer overload).
 * Example: dBm_per_Hz_t psd = -174 * dBm_per_Hz;
 * @param v Numeric value, interpreted as power spectral density in dBm/Hz.
 * @return dBm_per_Hz_t wrapping v as a double.
 */
constexpr dBm_per_Hz_t
operator*(int v, dBm_per_Hz_unit)
{
    return dBm_per_Hz_t(static_cast<double>(v));
}

// ============================================================================
// dBm_per_MHz_t -- power spectral density in dBm/MHz
// ============================================================================

/**
 * @brief Wrapper for power spectral density in dBm/MHz
 *
 * Supports multiplication by frequency to yield total power:
 * - dBm_per_MHz * MHz = dBm
 *
 * Construction: use multiplication literals (-20.0 * dBm_per_MHz).
 * Value extraction: use .numerical_value_in(dBm_per_MHz).
 */
class dBm_per_MHz_t
{
    double m_value{0}; ///< Stored numeric value, interpreted as power spectral density in dBm/MHz

    /**
     * Private: construct from a raw numeric value (used by friends only).
     * @param v Numeric value, interpreted as power spectral density in dBm/MHz.
     */
    constexpr explicit dBm_per_MHz_t(double v)
        : m_value(v)
    {
    }

    // Friends that need private constructor
    friend constexpr dBm_per_MHz_t operator*(double v, dBm_per_MHz_unit);
    friend constexpr dBm_per_MHz_t operator*(int v, dBm_per_MHz_unit);
    friend class DbmPerMhzChecker; ///< attribute checker needs Type(double)

  public:
    constexpr dBm_per_MHz_t() = default;

    /**
     * Get the numeric value in dBm/MHz.
     * @return The stored numeric value (power spectral density in dBm/MHz).
     */
    [[nodiscard]] constexpr double numerical_value_in(dBm_per_MHz_unit) const
    {
        return m_value;
    }

    /**
     * Three-way comparison.
     * @param a Left operand.
     * @param b Right operand.
     * @return Ordering result.
     */
    friend constexpr auto operator<=>(dBm_per_MHz_t a, dBm_per_MHz_t b) = default;

    /**
     * Equality comparison.
     * @param a Left operand.
     * @param b Right operand.
     * @return True if equal.
     */
    friend constexpr bool operator==(dBm_per_MHz_t a, dBm_per_MHz_t b) = default;

    /// dBm_per_MHz * MHz = dBm (linear domain multiplication, result in dBm)
    friend dBm_t operator*(dBm_per_MHz_t psd, MHz_t bw)
    {
        double psd_linear_mw_per_mhz = std::pow(10.0, psd.m_value / 10.0);
        double bw_mhz = bw.numerical_value_ref_in(mp_units::si::mega<mp_units::si::hertz>);
        double power_mw = psd_linear_mw_per_mhz * bw_mhz;
        return 10.0 * std::log10(power_mw) * dBm;
    }

    /// MHz * dBm_per_MHz = dBm (commutative)
    friend dBm_t operator*(MHz_t bw, dBm_per_MHz_t psd)
    {
        return psd * bw;
    }

    /// dBm / MHz = dBm_per_MHz
    friend dBm_per_MHz_t operator/(dBm_t power, MHz_t bw)
    {
        double power_linear_mw = std::pow(10.0, power.numerical_value_in(dBm) / 10.0);
        double bw_mhz = bw.numerical_value_ref_in(mp_units::si::mega<mp_units::si::hertz>);
        double psd_linear_mw_per_mhz = power_linear_mw / bw_mhz;
        return dBm_per_MHz_t(10.0 * std::log10(psd_linear_mw_per_mhz));
    }

    /**
     * Stream insertion: prints as "X dBm/MHz".
     * @param os Output stream.
     * @param v The dBm_per_MHz_t value.
     * @return Reference to the stream.
     */
    friend std::ostream& operator<<(std::ostream& os, dBm_per_MHz_t v)
    {
        os << v.m_value << " dBm/MHz";
        return os;
    }

    /**
     * Stream extraction: accepts "X_dBm_per_MHz" or "X".
     * @param is Input stream.
     * @param v The dBm_per_MHz_t to populate.
     * @return Reference to the stream.
     */
    friend std::istream& operator>>(std::istream& is, dBm_per_MHz_t& v)
    {
        std::string token;
        is >> token;
        const auto pos = token.find('_');
        std::string number;
        if (pos == std::string::npos)
        {
            number = token;
        }
        else
        {
            auto unit = token.substr(pos + 1);
            if (unit != "dBm_per_MHz")
            {
                is.setstate(std::ios_base::failbit);
                return is;
            }
            number = token.substr(0, pos);
        }
        v = dBm_per_MHz_t(std::strtod(number.c_str(), nullptr));
        return is;
    }
};

/**
 * mp-units style literal for a dBm_per_MHz_t. Example: dBm_per_MHz_t psd = -50.0 * dBm_per_MHz;
 * @param v Numeric value, interpreted as power spectral density in dBm/MHz.
 * @return dBm_per_MHz_t wrapping v.
 */
constexpr dBm_per_MHz_t
operator*(double v, dBm_per_MHz_unit)
{
    return dBm_per_MHz_t(v);
}

/**
 * mp-units style literal for a dBm_per_MHz_t (integer overload).
 * Example: dBm_per_MHz_t psd = -50 * dBm_per_MHz;
 * @param v Numeric value, interpreted as power spectral density in dBm/MHz.
 * @return dBm_per_MHz_t wrapping v as a double.
 */
constexpr dBm_per_MHz_t
operator*(int v, dBm_per_MHz_unit)
{
    return dBm_per_MHz_t(static_cast<double>(v));
}

} // namespace ns3

// ============================================================================
// Stream I/O for mp-units linear quantities used in ns-3
// ============================================================================

// mp-units quantities already have operator<< built in (prints e.g. "100 mW").
// We provide operator>> (stream extraction) for types that need it for
// CommandLine and Attribute deserialization.  These must be in namespace mp_units
// so that argument-dependent lookup (ADL) can find them from template code.

namespace mp_units
{

/**
 * Stream extraction for Watt_t: accepts "1.0_W" or "1.0".
 * @param is Input stream.
 * @param watt The Watt_t to populate.
 * @return Reference to the stream.
 */
inline std::istream&
operator>>(std::istream& is, ns3::Watt_t& watt)
{
    std::string token;
    is >> token;
    const auto pos = token.find('_');
    std::string number;
    if (pos == std::string::npos)
    {
        number = token;
    }
    else
    {
        auto unit = token.substr(pos + 1);
        if (unit != "W")
        {
            is.setstate(std::ios_base::failbit);
            return is;
        }
        number = token.substr(0, pos);
    }
    watt = std::strtod(number.c_str(), nullptr) * si::watt;
    return is;
}

/**
 * Stream extraction for mWatt_t: accepts "100.0_mW" or "100.0".
 * @param is Input stream.
 * @param milliwatt The mWatt_t to populate.
 * @return Reference to the stream.
 */
inline std::istream&
operator>>(std::istream& is, ns3::mWatt_t& milliwatt)
{
    std::string token;
    is >> token;
    const auto pos = token.find('_');
    std::string number;
    if (pos == std::string::npos)
    {
        number = token;
    }
    else
    {
        auto unit = token.substr(pos + 1);
        if (unit != "mW")
        {
            is.setstate(std::ios_base::failbit);
            return is;
        }
        number = token.substr(0, pos);
    }
    milliwatt = std::strtod(number.c_str(), nullptr) * si::milli<si::watt>;
    return is;
}

/**
 * Stream extraction for MHz_t: accepts "2400.0_MHz" or "2400.0".
 * @param is Input stream.
 * @param megahertz The MHz_t to populate.
 * @return Reference to the stream.
 */
inline std::istream&
operator>>(std::istream& is, ns3::MHz_t& megahertz)
{
    std::string token;
    is >> token;
    const auto pos = token.find('_');
    std::string number;
    if (pos == std::string::npos)
    {
        number = token;
    }
    else
    {
        auto unit = token.substr(pos + 1);
        if (unit != "MHz")
        {
            is.setstate(std::ios_base::failbit);
            return is;
        }
        number = token.substr(0, pos);
    }
    megahertz = std::strtod(number.c_str(), nullptr) * si::mega<si::hertz>;
    return is;
}

} // namespace mp_units

#endif // NS3_UNITS_H
