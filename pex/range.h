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

namespace pex
{


namespace model
{


template<typename T>
class Range
{
    static_assert(
        std::is_arithmetic_v<T>,
        "Designed only for arithmetic types.");

public:
    using Type = T;
    using Value = typename ::pex::model::Value<T>;
    using Bound = typename ::pex::model::Value<T>;
    using ValueControl = typename ::pex::control::Value<void, Value>;

    template<typename Access>
    using BoundControl =
        typename ::pex::control::Value<void, Bound, Access>;

public:
    Range(T value, T minimum, T maximum)
        :
        value_(value),
        minimum_(minimum),
        maximum_(maximum)
    {

    }

    void SetMinimum(T minimum)
    {
        minimum = std::min(minimum, this->maximum_.Get());

        // Use a pex::Reference to delay notifying of the bounds change until
        // the value has been (maybe) adjusted.
        auto changeMinimum = ::pex::Reference<Bound>(this->minimum_);
        *changeMinimum = minimum;

        if (this->value_.Get() < minimum)
        {
            // The current value is less than the new minimum.
            // Adjust the value to the minimum.
            this->value_.Set(minimum);
        }
    }

    void SetMaximum(T maximum)
    {
        maximum = std::max(maximum, this->minimum_.Get());
        auto changeMaximum = ::pex::Reference<Bound>(this->maximum_);
        *changeMaximum = maximum;

        if (this->value_.Get() > maximum)
        {
            this->value_.Set(maximum);
        }
    }

    void SetValue(T value)
    {
        value = std::max(
            this->minimum_.Get(),
            std::min(value, this->maximum_.Get()));

        this->value_.Set(value);
    }

    void Connect(void * context, typename Value::Callable callable)
    {
        this->value_.Connect(context, callable);
    }

    ValueControl GetValueControl()
    {
        return ValueControl(this->value_);
    }

    template<typename Access = GetTag>
    BoundControl<Access> GetMinimumControl()
    {
        return BoundControl<Access>(this->minimum_);
    }

    template<typename Access = GetTag>
    BoundControl<Access> GetMaximumControl()
    {
        return BoundControl<Access>(this->maximum_);
    }

private:
    Value value_;
    Bound minimum_;
    Bound maximum_;
};

} // namespace model


namespace control
{

template
<
    typename Observer,
    typename RangeModel,
    typename Filter_ = NoFilter
>
class Range
{
public:
    using Filter = Filter_;
    using ModelType = typename RangeModel::Type;

    using Value =
        ::pex::control::FilteredValue
        <
            Observer,
            typename RangeModel::Value,
            Filter,
            pex::GetAndSetTag
        >;

    // Read-only Bound type for accessing range bounds.
    using Bound =
        ::pex::control::FilteredValue
        <
            Observer,
            typename RangeModel::Bound,
            Filter,
            pex::GetTag
        >;

    Range(RangeModel &model)
        :
        value(model.GetValueControl()),
        minimum(model.GetMinimumControl()),
        maximum(model.GetMaximumControl())
    {

    }

    Value value;
    Bound minimum;
    Bound maximum;
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
