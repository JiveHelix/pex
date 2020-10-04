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
    using ValueInterface = typename ::pex::interface::Value<void, Value>;

    template<typename Access>
    using BoundInterface =
        typename ::pex::interface::Value<void, Bound, Access>;

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
        auto changeMinimum = ::pex::Reference<Bound>(this->minimum_);
        *changeMinimum = minimum;

        if (this->value_.Get() < minimum)
        {
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

    void Connect(void * context, typename Value::Notify::Callable callable)
    {
        this->value_.Connect(context, callable);
    }

    ValueInterface GetValueInterface()
    {
        return ValueInterface(&this->value_);
    }

    template<typename Access = GetTag>
    BoundInterface<Access> GetMinimumInterface()
    {
        return BoundInterface<Access>(&this->minimum_);
    }

    template<typename Access = GetTag>
    BoundInterface<Access> GetMaximumInterface()
    {
        return BoundInterface<Access>(&this->maximum_);
    }

private:
    Value value_;
    Bound minimum_;
    Bound maximum_;
};

} // namespace model


namespace interface
{

template
<
    typename Observer,
    typename RangeModel,
    typename Filter_ = void
>
class Range
{
public:
    using Filter = Filter_;

    using Value =
        ::pex::interface::FilteredValue
        <
            Observer,
            typename RangeModel::Value,
            Filter,
            pex::GetAndSetTag
        >;

    using Bound =
        ::pex::interface::FilteredValue
        <
            Observer,
            typename RangeModel::Bound,
            Filter,
            pex::GetTag
        >;


    Range(RangeModel * model)
        :
        value(model->GetValueInterface()),
        minimum(model->GetMinimumInterface()),
        maximum(model->GetMaximumInterface())
    {

    }

    Value value;
    Bound minimum;
    Bound maximum;
};


} // namespace interface

} // namespace pex
