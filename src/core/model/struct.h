/*
 * Copyright (c) 2026 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef STRUCT_H
#define STRUCT_H

#include "tuple.h"

#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace ns3
{

/**
 * @ingroup attributes
 * @defgroup attribute_Struct Struct Attribute
 * AttributeValue implementation for structures.
 */

/**
 * Describe a structure field and the AttributeValue used to represent it.
 *
 * @tparam Value
 *         The AttributeValue type used to represent the field.
 * @tparam Member
 *         Pointer to the structure member represented by the field.
 */
template <typename Value, auto Member>
struct StructField
{
    /** AttributeValue type used to represent the field. */
    using value_type = Value;

    /** Pointer to the represented structure member. */
    static constexpr auto member = Member;
};

/**
 * @ingroup attribute_Struct
 *
 * AttributeValue implementation for structures.
 *
 * The fields of the structure are stored internally as a tuple of AttributeValue
 * objects. StructField instances associate every structure member with the
 * AttributeValue type used to represent it.
 *
 * @code
 * struct Data
 * {
 *     uint16_t count;
 *     double value;
 * };
 *
 * using DataValue =
 *     StructValue<Data,
 *                 StructField<UintegerValue, &Data::count>,
 *                 StructField<DoubleValue, &Data::value>>;
 * @endcode
 *
 * @tparam T
 *         The structure represented by this AttributeValue.
 * @tparam Fields
 *         StructField types describing the structure fields in declaration order.
 *
 * @see AttributeValue
 */
template <typename T, typename... Fields>
class StructValue : public TupleValue<typename Fields::value_type...>
{
  public:
    /** The represented structure type. */
    using result_type = T;
    /** Tuple of the values returned by the field AttributeValue objects. */
    using tuple_type = typename TupleValue<typename Fields::value_type...>::result_type;
    /** Tuple of the field AttributeValue objects. */
    using value_type = typename TupleValue<typename Fields::value_type...>::value_type;

    /** Construct a StructValue containing default-initialized field values. */
    StructValue() = default;

    /**
     * Construct a StructValue from a structure.
     *
     * @param value Value with which to construct this object.
     */
    StructValue(const result_type& value);

    Ptr<AttributeValue> Copy() const override;

    /**
     * Get the stored fields as a structure.
     *
     * @return the structure containing the stored field values
     */
    result_type Get() const;

    /**
     * Set the stored fields from a structure.
     *
     * @param value the structure containing the field values to store
     */
    void Set(const result_type& value);

    /**
     * Set the given variable to the structure represented by this object.
     *
     * @tparam U
     *         The type of the given variable.
     * @param value The variable to set.
     * @return true if the given variable was set
     */
    template <typename U>
    bool GetAccessor(U& value) const;

  private:
    /** TupleValue type used to store the structure fields. */
    using Base = TupleValue<typename Fields::value_type...>;

    static_assert((std::is_member_object_pointer_v<decltype(Fields::member)> && ...),
                  "StructField requires pointers to data members");
    static_assert((requires(const T& value) { value.*Fields::member; } && ...),
                  "Every StructField member must belong to the represented structure");
};

/**
 * @ingroup attribute_Struct
 *
 * Checker for attribute values representing structures.
 */
class StructChecker : public TupleChecker
{
};

/**
 * @ingroup attribute_Struct
 *
 * Create a StructChecker from the AttributeCheckers associated with the fields.
 *
 * @tparam T
 *         The StructValue type to check.
 * @tparam Ts
 *         The AttributeChecker types.
 * @param checkers AttributeCheckers for the individual fields.
 * @return pointer to the StructChecker instance
 */
template <typename T, typename... Ts>
Ptr<const AttributeChecker> MakeStructChecker(Ts... checkers);

/**
 * @ingroup attribute_Struct
 *
 * Create an AttributeAccessor for a structure data member, or for a lone class
 * get functor or set method.
 *
 * @tparam T
 *         The StructValue type used by the attribute.
 * @tparam T1
 *         The type of the class data member, get functor, or set method.
 * @param a1 The address of the data member, get functor, or set method.
 * @return the AttributeAccessor
 */
template <typename T, typename T1>
Ptr<const AttributeAccessor> MakeStructAccessor(T1 a1);

/**
 * @ingroup attribute_Struct
 *
 * Create an AttributeAccessor using a pair of get functor and set methods.
 *
 * @tparam T
 *         The StructValue type used by the attribute.
 * @tparam T1
 *         The type of the first method.
 * @tparam T2
 *         The type of the second method.
 * @param a1 The address of the first method.
 * @param a2 The address of the second method.
 * @return the AttributeAccessor
 */
template <typename T, typename T1, typename T2>
Ptr<const AttributeAccessor> MakeStructAccessor(T1 a1, T2 a2);

} // namespace ns3

/*****************************************************************************
 * Implementation below
 *****************************************************************************/

namespace ns3
{

template <typename T, typename... Fields>
StructValue<T, Fields...>::StructValue(const result_type& value)
{
    Set(value);
}

template <typename T, typename... Fields>
Ptr<AttributeValue>
StructValue<T, Fields...>::Copy() const
{
    return Create<StructValue<T, Fields...>>(Get());
}

template <typename T, typename... Fields>
typename StructValue<T, Fields...>::result_type
StructValue<T, Fields...>::Get() const
{
    return std::make_from_tuple<result_type>(Base::Get());
}

template <typename T, typename... Fields>
void
StructValue<T, Fields...>::Set(const result_type& value)
{
    Base::Set(tuple_type(value.*Fields::member...));
}

template <typename T, typename... Fields>
template <typename U>
bool
StructValue<T, Fields...>::GetAccessor(U& value) const
{
    value = U(Get());
    return true;
}

namespace internal
{

/**
 * @ingroup attribute_Struct
 *
 * Internal StructChecker implementation.
 *
 * @tparam T
 *         The StructValue type to check.
 */
template <typename T>
class StructChecker;

/**
 * @ingroup attribute_Struct
 *
 * StructChecker specialization for a StructValue.
 *
 * @tparam T
 *         The represented structure type.
 * @tparam Fields
 *         The fields represented by the StructValue.
 */
template <typename T, typename... Fields>
class StructChecker<StructValue<T, Fields...>> : public ns3::StructChecker
{
  public:
    /**
     * Construct a checker from the checkers for the individual fields.
     *
     * @tparam Ts
     *         The AttributeChecker types.
     * @param checkers The checkers for the individual fields.
     */
    template <typename... Ts>
    StructChecker(Ts... checkers)
        : m_checkers{checkers...}
    {
        static_assert(sizeof...(Ts) == sizeof...(Fields),
                      "A checker must be provided for every structure field");
    }

    const std::vector<Ptr<const AttributeChecker>>& GetCheckers() const override
    {
        return m_checkers;
    }

    bool Check(const AttributeValue& value) const override
    {
        const auto structValue = dynamic_cast<const StructValue<T, Fields...>*>(&value);
        if (structValue == nullptr)
        {
            return false;
        }

        std::size_t n{0};
        return std::apply(
            [this, &n](const auto&... values) { return (m_checkers[n++]->Check(values) && ...); },
            structValue->GetValue());
    }

    std::string GetValueTypeName() const override
    {
        return "ns3::StructValue";
    }

    bool HasUnderlyingTypeInformation() const override
    {
        return false;
    }

    std::string GetUnderlyingTypeInformation() const override
    {
        return "";
    }

    Ptr<AttributeValue> Create() const override
    {
        return ns3::Create<StructValue<T, Fields...>>();
    }

    bool Copy(const AttributeValue& source, AttributeValue& destination) const override
    {
        const auto src = dynamic_cast<const StructValue<T, Fields...>*>(&source);
        auto dst = dynamic_cast<StructValue<T, Fields...>*>(&destination);
        if (src == nullptr || dst == nullptr)
        {
            return false;
        }
        *dst = *src;
        return true;
    }

  private:
    std::vector<Ptr<const AttributeChecker>> m_checkers; //!< Field checkers
};

} // namespace internal

template <typename T, typename... Ts>
Ptr<const AttributeChecker>
MakeStructChecker(Ts... checkers)
{
    return Create<internal::StructChecker<T>>(checkers...);
}

template <typename T, typename T1>
Ptr<const AttributeAccessor>
MakeStructAccessor(T1 a1)
{
    return MakeAccessorHelper<T>(a1);
}

template <typename T, typename T1, typename T2>
Ptr<const AttributeAccessor>
MakeStructAccessor(T1 a1, T2 a2)
{
    return MakeAccessorHelper<T>(a1, a2);
}

} // namespace ns3

#endif // STRUCT_H
