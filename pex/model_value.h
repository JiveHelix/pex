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
#include <stdexcept>

#include "pex/no_filter.h"
#include "pex/detail/notify_one.h"
#include "pex/detail/notify_many.h"
#include "pex/detail/value_connection.h"
#include "pex/detail/filters.h"
#include "pex/detail/value.h"
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
        detail::FilteredType<T, Filter>
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
    static_assert(!std::is_void_v<T>);
    static_assert(detail::FilterIsNoneOrValid<T, Filter_, SetTag>);

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
        PEX_LOG(this);
    }

    explicit Value_(Type value)
        :
        filter_{},
        value_{this->FilterOnSet_(value)}
    {
        static_assert(
            detail::FilterIsNoneOrStatic<Type, Filter, SetTag>,
            "A filter with member functions requires a pointer.");

        PEX_LOG(this);
    }

    Value_(Type value, Filter filter)
        :
        filter_{filter},
        value_{this->FilterOnSet_(value)}
    {
        static_assert(
            detail::FilterIsMember<Type, Filter>,
            "Static or void Filter does not require a filter instance.");

        PEX_LOG(this);
    }

    Value_(Filter filter)
        :
        filter_{filter},
        value_{}
    {
        static_assert(
            detail::FilterIsMember<Type, Filter>,
            "Static or void Filter does not require a filter instance.");

        PEX_LOG(this);
    }

    Value_(const Value_<Type, Filter> &) = delete;
    Value_(Value_<Type, Filter> &&) = delete;

    ~Value_()
    {
#ifdef ENABLE_PEX_LOG
        if (!this->connections_.empty())
        {
            for (auto &connection: this->connections_)
            {
                PEX_LOG(
                    "Warning: ",
                    connection.GetObserver(),
                    " is still connected to Model ",
                    this);
            }
        }
#endif
    }

    /** Set the value and notify interfaces **/
    void Set(Argument<Type> value)
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

    Value_ & operator=(Argument<Type> value)
    {
        this->Set(value);
        return *this;
    }

    void SetFilter(Filter filter)
    {
        static_assert(
            detail::FilterIsMember<Type, Filter>,
            "Static or void Filter does not require a filter instance.");

        this->filter_ = filter;
    }

    Filter & GetFilter()
    {
        return *this->filter_;
    }

    const Filter & GetFilter() const
    {
        return *this->filter_;
    }

    // This function is used in debug assertions to check that other entities
    // hold a reference to a model value.
    bool HasModel() const { return true; }

private:
    void SetWithoutNotify_(Argument<Type> value)
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

    Type FilterOnSet_(Argument<Type> value) const
    {
        if constexpr (std::is_same_v<NoFilter, Filter>)
        {
            return value;
        }
        else if constexpr (detail::SetterIsMember<Type, Filter>)
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

    Type FilterOnGet_(Argument<Type> value) const
    {
        if constexpr (std::is_same_v<NoFilter, Filter>)
        {
            return value;
        }
        else if constexpr (detail::GetterIsMember<Type, Filter>)
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
    using Callable = typename Model::Callable;
    static constexpr bool isPexCopyable = true;

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

    Direct(const Direct &other)
        :
        model_(other.model_)
    {
        if (!other.model_)
        {
            throw std::logic_error("other.model_ must be set!");
        }
    }

    Direct & operator=(const Direct &other)
    {
        if (!other.model_)
        {
            throw std::logic_error("other.model_ must be set!");
        }

        this->model_ = other.model_;

        return *this;
    }

    Type Get() const
    {
        REQUIRE_HAS_VALUE(this->model_);
        return this->model_->Get();
    }

    void Set(Argument<Type> value)
    {
        REQUIRE_HAS_VALUE(this->model_);
        this->model_->Set(value);
    }

    void Connect(void * const observer, Callable callable)
    {
        if (this->model_)
        {
            PEX_LOG("Connect ", observer, " to ", this->model_);
            this->model_->Connect(observer, callable);
        }
    }

    void Disconnect(void * const observer)
    {
        if (this->model_)
        {
            PEX_LOG("Disconnect observer: ", observer);
            this->model_->Disconnect(observer);
        }
    }

    bool HasModel() const
    {
        return (this->model_ != nullptr);
    }

    template<typename, typename, typename, typename>
    friend class ::pex::control::Value_;

    template<typename>
    friend class ::pex::Reference;

private:
    void SetWithoutNotify_(Argument<Type> value)
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
