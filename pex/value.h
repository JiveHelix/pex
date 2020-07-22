/**
  * @file value.h
  * 
  * @brief Implements model and interface Value nodes.
  * 
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 22 Jul 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/
#pragma once
#include <type_traits>
#include "pex/notify.h"

namespace pex
{

template<typename Observer, typename T>
using UnboundValueCallable = void (*)(Observer * const observer, T value);

template<typename Observer>
using UnboundSignalCallable = void (*)(Observer * const observer);


template<typename Observer, typename T>
using BoundValueCallable = void (Observer::*)(T value);

template<typename Observer>
using BoundSignalCallable = void (Observer::*)();


namespace detail
{

template<
    typename Observer,
    typename T,
    template<typename, typename> typename ValueCallable>
class ValueNotify: public Notify_<Observer, ValueCallable<Observer, T>>
{
public:
    using Type = T;
    using Base = Notify_<Observer, ValueCallable<Observer, T>>;
    using Base::Base;

    void operator()(T value)
    {
        if constexpr(Base::IsMemberFunction)
        {
            static_assert(
                !std::is_same_v<Observer, void>,
                "Cannot call member function on void type.");

            (this->observer_->*(this->callable_))(value);
        }
        else
        {
            this->callable_(this->observer_, value);
        }
    }
};


template<typename Observer, template<typename> typename SignalCallable>
class SignalNotify: public Notify_<Observer, SignalCallable<Observer>>
{
public:
    using Base = Notify_<Observer, SignalCallable<Observer>>;
    using Base::Base;

    void operator()()
    {
        if constexpr(Base::IsMemberFunction)
        {
            static_assert(
                !std::is_same_v<Observer, void>,
                "Cannot call member function on void type.");

            (this->observer_->*(this->callable_))();
        }
        else
        {
            this->callable_(this->observer_);
        }
    }
};

} // namespace detail

namespace model
{

template<typename T, typename Filter, typename = void>
struct FilterIsValid_: std::false_type {};


/** Filter can be void **/
template<typename T, typename Filter>
struct FilterIsValid_
<
    T,
    Filter,
    std::enable_if_t<std::is_same_v<Filter, void>>
> : std::true_type {};


/** Filter::Set can be normal function, or a function that takes a pointer
 ** to Filter.
 **/
template<typename T, typename Filter>
struct FilterIsValid_
<
    T,
    Filter,
    std::enable_if_t
    <
        std::is_invocable_r_v<T, decltype(Filter::Set), T>
        || std::is_invocable_r_v<T, decltype(Filter::Set), Filter *, T>
    >
> : std::true_type {};


template<typename T, typename Filter>
constexpr auto FilterIsValid = FilterIsValid_<T, Filter>::value;


// Model must use unbound callbacks so it can notify disparate types.
// All observers are stored as void *.
template<typename T, typename Filter = void>
class Value
    : public detail::NotifyMany<
        detail::ValueNotify<void, T, UnboundValueCallable>>
{
    static_assert(FilterIsValid<T, Filter>);

public:
    using Type = T;

    void Set(T value)
    {
        if constexpr (std::is_void_v<Filter>)
        {
            this->value_ = value;
        }
        else if constexpr (
            std::is_invocable_r_v<T, decltype(Filter::Set), Filter *, T>)
        {
            this->value_ = Filter::Set(this->filterContext_, value);
        }
        else
        {
            // The filter is not void
            // and the filter doesn't accept a Filter * argument.
            this->value_ = Filter::Set(value);
        }

        this->Notify_(this->value_);
    }

    void SetFilterContext(Filter * const filterContext)
    {
        static_assert(!std::is_void_v<Filter>, "Filter must be specified");
        this->filterContext_ = filterContext;
    }

    T Get() const
    {
        return this->value_;
    }

    Value(): value_{}, filterContext_{nullptr}
    {

    }

    Value(T value): value_(value)
    {

    }

    Value(const Value<T, Filter> &) = delete;
    Value(Value<T, Filter> &&) = delete;

private:

    T value_;

    Filter * filterContext_;
};

} // namespace model


namespace interface
{

template<typename T, typename Filter, typename = void>
struct GetterIsValid : std::false_type {};

template<typename T, typename Filter>
struct GetterIsValid
<
    T,
    Filter,
    std::enable_if_t
    <
        std::is_invocable_r_v<T, decltype(Filter::Get), T>
        || std::is_invocable_r_v<T, decltype(Filter::Get), Filter *, T>
    >
> : std::true_type {};


template<typename T, typename Filter, typename = void>
struct SetterIsValid : std::false_type {};

template<typename T, typename Filter>
struct SetterIsValid
<
    T,
    Filter,
    std::enable_if_t
    <
        std::is_invocable_r_v<void, decltype(Filter::Set), T>
        || std::is_invocable_r_v<void, decltype(Filter::Set), Filter *, T>
    >
> : std::true_type {};


template<typename T, typename Filter, typename = void>
struct FilterIsValid_ : std::false_type {};


/** Filter can be void **/
template<typename T, typename Filter>
struct FilterIsValid_
<
    T,
    Filter,
    std::enable_if_t<std::is_same_v<Filter, void>>
> : std::true_type {};


template<typename T, typename Filter>
struct FilterIsValid_
<
    T,
    Filter,
    std::enable_if_t
    <
        GetterIsValid<T, Filter>::value
        && SetterIsValid<T, Filter>::value
    >
> : std::true_type {};


template<typename T, typename Filter>
constexpr auto FilterIsValid = FilterIsValid_<T, Filter>::value;


template<
    typename Observer,
    typename ModelValue,
    typename Filter = void>
class Value
#ifdef ALLOW_MULTIPLE_CALLBACKS
    : public detail::NotifyMany<
#else
    : public detail::NotifyOne<
#endif
        detail::ValueNotify<
            Observer,
            typename ModelValue::Type,
            BoundValueCallable>>
{
public:
    using T = typename ModelValue::Type;

    static_assert(FilterIsValid<T, Filter>);

    Value(ModelValue * const model)
        :
        model_(model)
    {
        this->model_->Connect(this, &Value::OnModelChanged_);
    }

    ~Value()
    {
        this->model_->Disconnect(this);
    }

    Value(const Value &other)
        :
        model_(other.model_)
    {
        this->model_->Connect(this, &Value::OnModelChanged_);
    }

    /**
     ** Assignment could cause an interface::Value to track a different model
     ** value.
     **/
    Value & operator=(const Value &) = delete;
    Value & operator=(Value &&) = delete;

    void SetFilterContext(Filter * const filterContext)
    {
        static_assert(
            !std::is_void_v<Filter>,
            "Filter must be specified");

        this->filterContext_ = filterContext;
    }

    T Get() const
    {
        if constexpr (std::is_void_v<Filter>)
        {
            return this->model_->Get();
        }
        else
        {
            return this->FilterGet_(this->model_->Get());
        }
    }

    void Set(T value)
    {
        if constexpr (std::is_void_v<Filter>)
        {
            this->model_->Set(value);
        }
        else
        {
            this->model_->Set(this->FilterSet_(value));
        }
    }

private:
    T FilterSet_(T value) const
    {
        if constexpr (
            std::is_invocable_r_v<T, decltype(Filter::Set), Filter *, T>)
        {
            return Filter::Set(this->filterContext_, value);
        }
        else
        {
            // The filter doesn't accept a Filter * argument.
            return Filter::Set(value);
        }
    }

    T FilterGet_(T value) const
    {
        if constexpr (
            std::is_invocable_r_v<T, decltype(Filter::Get), Filter *, T>)
        {
            return Filter::Get(this->filterContext_, value);
        }
        else
        {
            // The filter doesn't accept a Filter * argument.
            return Filter::Get(value);
        }
    }

    static void OnModelChanged_(void * observer, T value)
    {
        // The model value has changed.
        // Update our observers.
        auto self = static_cast<Value *>(observer);

        if constexpr (!std::is_void_v<Filter>)
        {
            value = self->FilterGet_(value);
        }

        self->Notify_(value);
    }

    ModelValue *model_;

    Filter * filterContext_;
};

} // namespace interface

} // namespace pex
