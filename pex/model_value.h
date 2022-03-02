/**
  * @file model_value.h
  *
  * @brief Implements model Value nodes.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 22 Jul 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include <type_traits>
#include <optional>

#include "pex/no_filter.h"
#include "pex/detail/notify_one.h"
#include "pex/detail/notify_many.h"
#include "pex/detail/value_connection.h"
#include "pex/detail/filters.h"
#include "pex/detail/value_detail.h"
#include "pex/access_tag.h"
#include "pex/reference.h"
#include "pex/transaction.h"
#include "pex/detail/require_has_value.h"


namespace pex
{

template<typename Observer, typename T, typename Filter>
using ValueConnection =
    detail::ValueConnection
    <
        Observer,
        typename detail::FilteredType<T, Filter>::Type
    >;

namespace model
{

// Model must use unbound callbacks so it can send notifications to
// different observer types.
// All observers are stored as void *.
template<typename T, typename Filter_>
class Value_
    :
    //Callback values will be the type returned by the Filter, or T if
    // the filter is void.
    public detail::NotifyMany<ValueConnection<void, T, Filter_>, GetAndSetTag>
{
    static_assert(detail::FilterIsNoneOrValid<T, Filter_, SetTag>::value);

public:
    using Type = T;
    using Filter = Filter_;
    using Callable = typename ValueConnection<void, T, Filter>::Callable;

    // All model nodes have writable access.
    using Access = GetAndSetTag;

    template<typename Pex>
    friend class ::pex::Transaction;

    template<typename Pex>
    friend class ::pex::Reference;

    template<typename Pex>
    friend class ::pex::ConstReference;

    Value_()
        :
        filter_{},
        value_{}
    {

    }

    explicit Value_(T value)
        :
        filter_{},
        value_{this->FilterOnSet_(value)}
    {
        static_assert(
            detail::FilterIsNoneOrStatic<T, Filter, SetTag>::value,
            "A filter with member functions requires a pointer.");
    }

    Value_(T value, Filter filter)
        :
        filter_{filter},
        value_{this->FilterOnSet_(value)}
    {
        static_assert(
            detail::FilterIsMember<T, Filter>::value,
            "Static or void Filter does not require a filter instance.");
    }

    Value_(Filter filter)
        :
        filter_{filter},
        value_{}
    {
        static_assert(
            detail::FilterIsMember<T, Filter>::value,
            "Static or void Filter does not require a filter instance.");
    }

    Value_(const Value_<T, Filter> &) = delete;
    Value_(Value_<T, Filter> &&) = delete;

    /** Set the value and notify interfaces **/
    void Set(ArgumentT<T> value)
    {
        this->SetWithoutNotify_(value);
        this->DoNotify_();
    }

    T Get() const
    {
        return this->value_;
    }

    explicit operator T () const
    {
        return this->value_;
    }

    Value_ & operator=(ArgumentT<T> value)
    {
        this->Set(value);
        return *this;
    }

    void SetFilter(Filter filter)
    {
        static_assert(
            detail::FilterIsMember<T, Filter>::value,
            "Static or void Filter does not require a filter instance.");

        this->filter_ = filter;
    }

private:
    void SetWithoutNotify_(ArgumentT<T> value)
    {
        if constexpr (std::is_same_v<NoFilter, Filter>)
        {
            this->value_ = value;
        }
        else
        {
            this->value_ = this->FilterOnSet_(value);
        }
    }

    void DoNotify_()
    {
        this->Notify_(this->value_);
    }

    T FilterOnSet_(ArgumentT<T> value) const
    {
        if constexpr (std::is_same_v<NoFilter, Filter>)
        {
            return value;
        }
        else if constexpr (detail::SetterIsMember<T, Filter>::value)
        {
            REQUIRE_HAS_VALUE(this->filter_);
            return this->filter_->Set(value);
        }
        else
        {
            // The filter is not a member method.
            return Filter::Set(value);
        }
    }

    T FilterOnGet_(ArgumentT<T> value) const
    {
        if constexpr (std::is_same_v<NoFilter, Filter>)
        {
            return value;
        }
        else if constexpr (detail::GetterIsMember<T, Filter>::value)
        {
            REQUIRE_HAS_VALUE(this->filter_);
            return this->filter_->Get(value);
        }
        else
        {
            // The filter doesn't accept a Filter * argument.
            return Filter::Get(value);
        }
    }

    std::optional<Filter> filter_;
    T value_;
};


template<typename T>
using Value = Value_<T, NoFilter>;

template<typename T, typename Filter>
using FilteredValue = Value_<T, Filter>;


} // namespace model


template<typename ...T>
struct IsModel: std::false_type {};

template<typename ...T>
struct IsModel<pex::model::Value_<T...>>: std::true_type {};


} // namespace pex
