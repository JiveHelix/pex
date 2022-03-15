/**
  * @file range.h
  * 
  * @brief A class encapsulating a bounded value, with pex::Value's for value,
  * minimum, and maximum.
  * 
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 17 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include <type_traits>
#include <stdexcept>
#include <limits>
#include "pex/value.h"
#include "pex/detail/filters.h"
#include "pex/reference.h"


namespace pex
{


// Forward declare the control::Range
// Necessary for the friend declaration in model::Range
namespace control
{

template<typename, typename, typename, typename>
class Range;


} // end namespace control


namespace model
{


template<typename T>
struct RangeFilter
{
    RangeFilter(T minimum, T maximum)
        :
        minimum_(minimum),
        maximum_(maximum)
    {

    }

    T Get(T value) const
    {
        return value;
    }

    T Set(T value) const
    {
        return std::max(
            this->minimum_,
            std::min(value, this->maximum_));
    }

private:
    T minimum_;
    T maximum_;
};


template<typename Upstream>
class Range
{
    static_assert(
        std::is_arithmetic_v<typename Upstream::Type>,
        "Designed only for arithmetic types.");

public:
    // The Range value is a control to a model somewhere else.
    using Value =
        pex::control::FilteredValue
        <
            void,
            Upstream,
            RangeFilter<typename Upstream::Type>
        >;

    using Type = typename Value::Type;

    // The Range limits are stored here in the model::Range.
    using Limit = typename ::pex::model::Value<Type>;

public:
    Range() = default;

    Range(Upstream &upstream)
        :
        value_(upstream),
        minimum_(std::numeric_limits<Type>::lowest()),
        maximum_(std::numeric_limits<Type>::max())
    {
        this->value_.SetFilter(RangeFilter<Type>(
            this->minimum_.Get(),
            this->maximum_.Get()));
    }

    ~Range()
    {
        this->value_.Disconnect(this);
    }

    void SetLimits(Type minimum, Type maximum)
    {
        if (maximum < minimum)
        {
            // maximum may be equal to minimum, even though it doesn't seem
            // very useful.
            throw std::invalid_argument("requires maximum >= minimum");
        }

        // All model values will be changed, so any request to Get() will
        // return the new value, but notifications will not be sent until we
        // exit this scope.
        auto changeMinimum = ::pex::Defer<Limit>(this->minimum_);
        auto changeMaximum = ::pex::Defer<Limit>(this->maximum_);
        auto changeValue = ::pex::Defer<Value>(this->value_);

        changeMinimum.Set(minimum);
        changeMaximum.Set(maximum);

        this->value_.SetFilter(RangeFilter<Type>(
            this->minimum_.Get(),
            this->maximum_.Get()));

        if (this->value_.Get() < minimum)
        {
            changeValue.Set(minimum);
        }
        else if (this->value_.Get() > maximum)
        {
            changeValue.Set(maximum);
        }
    }

    void SetMinimum(Type minimum)
    {
        minimum = std::min(minimum, this->maximum_.Get());

        // Use a pex::Defer to delay notifying of the bounds change until
        // the value has been (maybe) adjusted.
        auto changeMinimum = ::pex::Defer<Limit>(this->minimum_);
        changeMinimum.Set(minimum);

        this->value_.SetFilter(RangeFilter<Type>(
            this->minimum_.Get(),
            this->maximum_.Get()));

        if (this->value_.Get() < minimum)
        {
            // The current value is less than the new minimum.
            // Adjust the value to the minimum.
            this->value_.Set(minimum);
        }
    }

    void SetMaximum(Type maximum)
    {
        maximum = std::max(maximum, this->minimum_.Get());
        auto changeMaximum = ::pex::Defer<Limit>(this->maximum_);
        changeMaximum.Set(maximum);

        this->value_.SetFilter(RangeFilter<Type>(
            this->minimum_.Get(),
            this->maximum_.Get()));

        if (this->value_.Get() > maximum)
        {
            this->value_.Set(maximum);
        }
    }

    void SetValue(Type value)
    {
        this->value_.Set(value);
    }

    template
    <
        typename,
        typename,
        typename,
        typename 
    >
    friend class ::pex::control::Range;

private:
    Value value_;
    Limit minimum_;
    Limit maximum_;
};

} // namespace model


template<typename T>
struct IsModelRange: std::false_type {};

template<typename T>
struct IsModelRange<model::Range<T>>: std::true_type {};


namespace control
{

template
<
    typename Observer,
    typename Upstream,
    typename Filter_ = NoFilter,
    typename Access = pex::GetAndSetTag
>
class Range
{
public:
    using Filter = Filter_;
    using Type = typename Upstream::Value::Type;

    static_assert(
        pex::detail::FilterIsNoneOrStatic<Type, Filter, Access>::value,
        "This class is designed for free function filters.");

    using Value =
        pex::control::FilteredValue
        <
            Observer,
            typename Upstream::Value,
            Filter,
            Access
        >;

    // Read-only Limit type for accessing range bounds.
    using Limit =
        pex::control::FilteredValue
        <
            Observer,
            typename Upstream::Limit,
            Filter,
            pex::GetTag
        >;

    Range(Upstream &upstream)
    {
        if constexpr (IsModelRange<Upstream>::value)
        {
            this->value = Value(upstream.value_);
            this->minimum = Limit(upstream.minimum_);
            this->maximum = Limit(upstream.maximum_);
        }
        else
        {
            this->value = Value(upstream.value);
            this->minimum = Limit(upstream.minimum);
            this->maximum = Limit(upstream.maximum);
        }
    }
    
    template<typename OtherObserver, typename OtherFilter, typename OtherAccess>
    Range(const Range<OtherObserver, Upstream, OtherFilter, OtherAccess> &other)
        :
        value(other.value),
        minimum(other.minimum),
        maximum(other.maximum)
    {

    }

    template<typename OtherObserver, typename OtherFilter, typename OtherAccess>
    Range & operator=(
        const Range<OtherObserver, Upstream, OtherFilter, OtherAccess> &other)
    {
        this->value = other.value;
        this->minimum = other.minimum;
        this->maximum = other.maximum;

        return *this;
    }

    Value value;
    Limit minimum;
    Limit maximum;
};

} // namespace control


} // namespace pex


#include "jive/overflow.h"

template<typename Target, typename T>
void RequireConvertible(T value)
{
    if (!jive::CheckConvertible<Target>(value))
    {
        throw std::range_error("value is not convertible to Target.");
    }
}


#ifndef NDEBUG
#define CHECK_TO_INT_RANGE(value) RequireConvertible<int>(value)
#define CHECK_FROM_INT_RANGE(T, value) RequireConvertible<T, int>(value)
#else
#define CHECK_TO_INT_RANGE(value)
#define CHECK_FROM_INT_RANGE(T, value)
#endif


template<typename T>
struct ExampleRangeFilter
{
    static int Get(T value)
    {
        CHECK_TO_INT_RANGE(value);
        return static_cast<int>(value);
    }

    static T Set(int value)
    {
        CHECK_FROM_INT_RANGE(T, value);
        return static_cast<T>(value);
    }
};
