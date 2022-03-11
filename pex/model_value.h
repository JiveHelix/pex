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
#include "pex/transaction.h"
#include "pex/detail/require_has_value.h"


namespace pex
{


template<typename>
class Reference;

template<typename>
class ConstReference;


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

    template<typename>
    friend class ::pex::Transaction;

    template<typename>
    friend class ::pex::Reference;

    template<typename>
    friend class ::pex::ConstReference;

    template<typename>
    friend class Direct;

    Value_()
        :
        filter_{},
        value_{}
    {

    }

    explicit Value_(Type value)
        :
        filter_{},
        value_{this->FilterOnSet_(value)}
    {
        static_assert(
            detail::FilterIsNoneOrStatic<Type, Filter, SetTag>::value,
            "A filter with member functions requires a pointer.");
    }

    Value_(Type value, Filter filter)
        :
        filter_{filter},
        value_{this->FilterOnSet_(value)}
    {
        static_assert(
            detail::FilterIsMember<Type, Filter>::value,
            "Static or void Filter does not require a filter instance.");
    }

    Value_(Filter filter)
        :
        filter_{filter},
        value_{}
    {
        static_assert(
            detail::FilterIsMember<Type, Filter>::value,
            "Static or void Filter does not require a filter instance.");
    }

    Value_(const Value_<Type, Filter> &) = delete;
    Value_(Value_<Type, Filter> &&) = delete;

    /** Set the value and notify interfaces **/
    void Set(ArgumentT<Type> value)
    {
        this->SetWithoutNotify_(value);
        this->DoNotify_();
    }

    Type Get() const
    {
        return this->value_;
    }

    explicit operator Type () const
    {
        return this->value_;
    }

    Value_ & operator=(ArgumentT<Type> value)
    {
        this->Set(value);
        return *this;
    }

    void SetFilter(Filter filter)
    {
        static_assert(
            detail::FilterIsMember<Type, Filter>::value,
            "Static or void Filter does not require a filter instance.");

        this->filter_ = filter;
    }

private:
    void SetWithoutNotify_(ArgumentT<Type> value)
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

    Type FilterOnSet_(ArgumentT<Type> value) const
    {
        if constexpr (std::is_same_v<NoFilter, Filter>)
        {
            return value;
        }
        else if constexpr (detail::SetterIsMember<Type, Filter>::value)
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

    Type FilterOnGet_(ArgumentT<Type> value) const
    {
        if constexpr (std::is_same_v<NoFilter, Filter>)
        {
            return value;
        }
        else if constexpr (detail::GetterIsMember<Type, Filter>::value)
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
    Type value_;
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


namespace control
{


template<typename, typename, typename, typename> class Value_;


} // end namespace control


namespace model
{


template<typename Model>
class Direct
{
public:
    using Type = typename Model::Type;

    Direct()
        :
        model_(nullptr)
    {

    }

    Direct(Model &model)
        :
        model_(&model)
    {

    }

    Type Get() const
    {
        REQUIRE_HAS_VALUE(this->model_);
        return this->model_->Get(); 
    }

    void Set(ArgumentT<Type> value)
    {
        REQUIRE_HAS_VALUE(this->model_);
        this->model_->Set(value);
    }

    void Connect(
        void * const observer,
        typename Model::Callable callable)
    {
        if (this->model_)
        {
            this->model_->Connect(observer, callable);
        }
    }

    void Disconnect(void * const observer)
    {
        if (this->model_)
        {
            this->model_->Disconnect(observer);
        }
    }

    bool HasModel()
    {
        return (this->model_ != nullptr);
    }
    
    template<typename, typename, typename, typename>
    friend class ::pex::control::Value_;

private:
    void SetWithoutNotify_(ArgumentT<Type> value)
    {
        this->model_->SetWithoutNotify_(value);
    }

    void DoNotify_()
    {
        this->model_->DoNotify_();
    }

private:
    Model *model_;
};


} // end namespace model


} // namespace pex
