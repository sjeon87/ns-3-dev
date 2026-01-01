#ifndef SI_UNITS_VECTOR_H
#define SI_UNITS_VECTOR_H

#include <iostream>
#include <vector>

namespace si_units
{

/// @brief A vector container for SI unit types
///
/// This template struct provides a wrapper around std::vector for SI unit types,
/// offering specialized construction, comparison, and I/O operations.
///
/// @tparam SiUnitType The SI unit type to be stored in the vector
template<typename SiUnitType>
struct SiUnitsVec
{
    std::vector<SiUnitType> vals; ///< The underlying vector container

    /// @brief Default constructor: Creates an empty vector
    SiUnitsVec() = default;

    /// @brief Constructor: Generate vector with specified size and default values
    /// @param size Number of elements to create
    SiUnitsVec(size_t size)
    {
        vals.assign(size, SiUnitType{0});
    }

    /// @brief Constructor: Create vector from existing std::vector
    /// @param inp Input vector to move from
    SiUnitsVec(std::vector<SiUnitType> inp) : vals(std::move(inp))
    {
    }

    /// @brief Convert vector contents to string representation
    /// @return String representation of all elements separated by spaces
    std::string str() const // NOLINT(readability-identifier-naming)
    {
        std::string res;
        for (const auto& x : vals)
        {
            res += x.str();
            res += " ";
        }
        return res;
    }

    /// @brief get size of vectors
    /// @return size of vector
    size_t size() const // NOLINT(readability-identifier-naming)
    {
        return vals.size();
    }

    /// @brief Equality operator
    /// @param rhs object being compared to
    /// @return true if objects are equal
    inline bool operator==(const SiUnitsVec<SiUnitType>& rhs) const
    {
        return vals == rhs.vals;
    }

    /// @brief Inequality operator
    /// @param rhs object being compared to
    /// @return true if objects are not equal
    inline bool operator!=(const SiUnitsVec<SiUnitType>& rhs) const
    {
        return !(operator==(rhs));
    }
};

/// @brief Output stream operator for SiUnitsVec
/// @tparam SiUnitType The SI unit type contained in the vector
/// @param os Output stream
/// @param vec The SiUnitsVec to output
/// @return Reference to the output stream
template<typename SiUnitType>
inline std::ostream&
operator<<(std::ostream& os, const SiUnitsVec<SiUnitType>& vec)
{
    return os << vec.str();
}

/// @brief Input stream operator for SiUnitsVec
/// @tparam SiUnitType The SI unit type contained in the vector
/// @param is Input stream
/// @param vec The SiUnitsVec to read into
/// @return Reference to the input stream
template<typename SiUnitType>
inline std::istream&
operator>>(std::istream& is, SiUnitsVec<SiUnitType>& vec)
{
    for (auto& x : vec.vals)
    {
        if (is.eof())
        {
            return is;
        }
        is >> x.val;
    }
    return is;
}

} // namespace si_units
#endif // SI_UNITS_VECTOR_H
