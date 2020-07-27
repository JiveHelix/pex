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

namespace detail
{


// Set and Notify methods will use pass-by-value when T is an integral or
// floating-point type. Other types, like std::string will be passed by const
// reference to avoid unnecessary copying. 
template<typename T, typename = void>
struct Argument;

template<typename T>
struct Argument<T, std::enable_if_t<std::is_arithmetic_v<T>>>
{
    using type = T;
};

template<typename T>
struct Argument<T, std::enable_if_t<!std::is_arithmetic_v<T>>>
{
    using type = const T &;
};



template<typename Observer, typename T>
using UnboundValueCallable =
    void (*)(Observer * const observer, typename Argument<T>::type value);

template<typename Observer, typename T>
using BoundValueCallable =
    void (Observer::*)(typename Argument<T>::type value);

// Use bound notification methods for all Observers except void.
template<typename Observer, typename T, typename = void>
struct CallableStyle;

template<typename Observer, typename T>
struct CallableStyle<Observer, T, std::enable_if_t<std::is_void_v<Observer>>>
{
    using type = UnboundValueCallable<Observer, T>;
};

template<typename Observer, typename T>
struct CallableStyle<Observer, T, std::enable_if_t<!std::is_void_v<Observer>>>
{
    using type = BoundValueCallable<Observer, T>;
};

template<typename Observer, typename T>
using ValueCallable = typename CallableStyle<Observer, T>::type;

template<typename Observer, typename T>
class ValueNotify: public Notify_<Observer, ValueCallable<Observer, T>>
{
public:
    using Type = T;
    using Base = Notify_<Observer, ValueCallable<Observer, T>>;
    using argumentType = typename Argument<T>::type;
    using Base::Base;

    void operator()(typename Argument<T>::type value)
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


template<typename T, typename Filter, typename = void>
struct GetterIsStatic: std::false_type {};

template<typename T>
struct GetterIsStatic<T, void, void>: std::false_type {};

template<typename T, typename Filter>
struct GetterIsStatic
<
    T,
    Filter,
    std::enable_if_t<std::is_invocable_r_v<T, decltype(&Filter::Get), T>>
> : std::true_type {};


template<typename T, typename Filter, typename = void>
struct GetterIsMember: std::false_type {};


template<typename T>
struct GetterIsMember<T, void, void>: std::false_type {};


template<typename T, typename Filter>
struct GetterIsMember
<
    T,
    Filter,
    std::enable_if_t
    <
        std::is_invocable_r_v<T, decltype(&Filter::Set), Filter, T>
    >
> : std::true_type {};


template<typename T, typename Filter, typename = void>
struct SetterIsStatic: std::false_type {};

template<typename T>
struct SetterIsStatic<T, void, void>: std::false_type {};

template<typename T, typename Filter>
struct SetterIsStatic
<
    T,
    Filter,
    std::enable_if_t<std::is_invocable_r_v<T, decltype(&Filter::Set), T>>
> : std::true_type {};


template<typename T, typename Filter, typename = void>
struct SetterIsMember: std::false_type {};

template<typename T>
struct SetterIsMember<T, void, void>: std::false_type {};

template<typename T, typename Filter>
struct SetterIsMember
<
    T,
    Filter,
    std::enable_if_t
    <
        std::is_invocable_r_v<T, decltype(&Filter::Set), Filter, T>
#if 0
        std::is_same_v
        <
            T,
            decltype(std::declval<Filter>().Set(std::declval<T>()))
        >
        && std::is_member_function_pointer_v<decltype(&Filter::Set)>
#endif
    >
> : std::true_type {};


/** Filter::Get can be normal function, or a function that takes a pointer
 ** to Filter.
 **/
template<typename T, typename Filter, typename = void>
struct GetterIsValid : std::false_type {};

template<typename T>
struct GetterIsValid<T, void, void> : std::false_type {};

template<typename T, typename Filter>
struct GetterIsValid
<
    T,
    Filter,
    std::enable_if_t
    <
        GetterIsStatic<T, Filter>::value || GetterIsMember<T, Filter>::value
    >
> : std::true_type {};


/** Filter::Set can be normal function, or a function that takes a pointer
 ** to Filter.
 **/
template<typename T, typename Filter, typename = void>
struct SetterIsValid : std::false_type {};


template<typename T>
struct SetterIsValid<T, void, void> : std::false_type {};


template<typename T, typename Filter>
struct SetterIsValid
<
    T,
    Filter,
    std::enable_if_t
    <
        SetterIsStatic<T, Filter>::value || SetterIsMember<T, Filter>::value
    >
> : std::true_type {};


} // namespace detail

namespace model
{

template<typename T, typename Filter, typename = void>
struct FilterIsVoidOrValid: std::false_type {};


/** Filter can be void **/
template<typename T, typename Filter>
struct FilterIsVoidOrValid
<
    T,
    Filter,
    std::enable_if_t<std::is_same_v<Filter, void>>
> : std::true_type {};


/** Filter::Set can be normal function, or a function that takes a pointer
 ** to Filter.
 **/
template<typename T, typename Filter>
struct FilterIsVoidOrValid
<
    T,
    Filter,
    std::enable_if_t<detail::SetterIsValid<T, Filter>::value>
> : std::true_type {};


// Model must use unbound callbacks so it can notify disparate types.
// All observers are stored as void *.
template<typename T, typename Filter = void>
class Value_ : public detail::NotifyMany<detail::ValueNotify<void, T>>
{
    static_assert(FilterIsVoidOrValid<T, Filter>::value);

public:
    using Type = T;

    void Set(typename detail::Argument<T>::type value)
    {
        if constexpr (std::is_void_v<Filter>)
        {
            this->value_ = value;
        }
        else if constexpr (
            std::is_invocable_r_v<T, decltype(&Filter::Set), Filter, T>)
        {
            this->value_ = std::invoke(
                &Filter::Set,
                *this->filter_,
                value);
        }
        else
        {
            // The filter is not void
            // and the filter doesn't accept a Filter * argument.
            this->value_ = Filter::Set(value);
        }

        this->Notify_(this->value_);
    }

    T Get() const
    {
        return this->value_;
    }

    Value_(const Value_<T, Filter> &) = delete;
    Value_(Value_<T, Filter> &&) = delete;

protected:
    Value_()
        :
        value_{},
        filter_{}
    {

    }

    Value_(T value)
        :
        value_{value},
        filter_{}
    {

    }

    T value_;
    Filter * filter_;
};


template<typename T, typename Filter = void, typename = void>
class Value;


/** Filter provides static methods or Filter is void **/
template<typename T, typename Filter>
class Value
<
    T,
    Filter,
    std::enable_if_t
    <
        detail::SetterIsStatic<T, Filter>::value || std::is_void_v<Filter>
    >
>
    :
    public Value_<T, Filter>
{
public:
    using Base = Value_<T, Filter>;
    Value(T value): Base(value) {}
    Value(): Base() {}
};


/** 
 ** Filter provides member methods.
 ** Constructor must get a reference to the filter instance.
 ** **/
template<typename T, typename Filter>
class Value
<
    T,
    Filter,
    std::enable_if_t
    <
        detail::SetterIsMember<T, Filter>::value
    >
>
    :
    public Value_<T, Filter>
{
public:
    Value(Filter &filter)
        :
        Value_<T, Filter>()
    {
        this->filter_ = &filter;
    }

    Value(Filter &filter, T value)
        :
        Value_<T, Filter>(value)
    {
        this->filter_ = &filter;
    }
};


} // namespace model


namespace interface
{

template<typename T, typename Filter, typename = void>
struct FilterIsVoidOrValid : std::false_type {};


/** Filter can be void **/
template<typename T, typename Filter>
struct FilterIsVoidOrValid
<
    T,
    Filter,
    std::enable_if_t<std::is_same_v<Filter, void>>
> : std::true_type {};


template<typename T, typename Filter>
struct FilterIsVoidOrValid
<
    T,
    Filter,
    std::enable_if_t
    <
        detail::GetterIsValid<T, Filter>::value
        && detail::SetterIsValid<T, Filter>::value
    >
> : std::true_type {};


template<
    typename Observer,
    typename Model,
    typename Filter = void>
class Value_
#ifdef ALLOW_MULTIPLE_CALLBACKS
    : public detail::NotifyMany<
#else
    : public detail::NotifyOne<
#endif
        detail::ValueNotify<Observer, typename Model::Type>>
{
public:
    using T = typename Model::Type;

    static_assert(FilterIsVoidOrValid<T, Filter>::value);

protected:
    Value_(Model * const model)
        :
        model_(model)
    {
        this->model_->Connect(this, &Value_::OnModelChanged_);
    }

public:
    ~Value_()
    {
        this->model_->Disconnect(this);
    }

    Value_(const Value_ &other)
        :
        model_(other.model_)
    {
        this->model_->Connect(this, &Value_::OnModelChanged_);
    }

    /**
     ** Assignment could cause an interface::Value_ to track a different model
     ** value. By design, an interface value tracks the same model value for
     ** the duration of its lifetime.
     **/
    Value_ & operator=(const Value_ &) = delete;
    Value_ & operator=(Value_ &&) = delete;

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

    void Set(typename detail::Argument<T>::type value)
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
    T FilterSet_(typename detail::Argument<T>::type value) const
    {
        if constexpr (
            std::is_invocable_r_v<T, decltype(&Filter::Set), Filter, T>)
        {
            return std::invoke(&Filter::Set, *this->filter_, value);
        }
        else
        {
            // The filter doesn't accept a Filter * argument.
            return Filter::Set(value);
        }
    }

    T FilterGet_(typename detail::Argument<T>::type value) const
    {
        if constexpr (
            std::is_invocable_r_v<T, decltype(&Filter::Get), Filter, T>)
        {
            return std::invoke(&Filter::Get, *this->filter_, value);
        }
        else
        {
            // The filter doesn't accept a Filter * argument.
            return Filter::Get(value);
        }
    }

    static void OnModelChanged_(
        void * observer,
        typename detail::Argument<T>::type value)
    {
        // The model value has changed.
        // Update our observers.
        auto self = static_cast<Value_ *>(observer);

        if constexpr (!std::is_void_v<Filter>)
        {
            value = self->FilterGet_(value);
        }

        self->Notify_(value);
    }

    Model *model_;
    Filter *filter_;
};


template<
    typename Observer,
    typename Model,
    typename Filter = void,
    typename = void>
class Value;


/** Filter provides static methods or Filter is void **/
template
<
    typename Observer,
    typename Model,
    typename Filter
>
class Value
<
    Observer,
    Model,
    Filter, 
    std::enable_if_t
    <
        std::is_void_v<Filter>
        || (detail::GetterIsStatic<typename Model::Type, Filter>::value
            && detail::SetterIsStatic<typename Model::Type, Filter>::value)
    >
>
    :
    public Value_<Observer, Model, Filter>
{
public:
    using Base = Value_<Observer, Model, Filter>;
    Value(Model * const model): Base(model) {}
    Value(const Value &other): Base(other) {}
};
        

/** 
 ** Filter provides member methods, so,
 ** Constructor must get a reference to the filter instance.
 **/
template
<
    typename Observer,
    typename Model,
    typename Filter
>
class Value
<
    Observer,
    Model,
    Filter, 
    std::enable_if_t
    <
        detail::GetterIsMember<typename Model::Type, Filter>::value
        || detail::SetterIsMember<typename Model::Type, Filter>::value
    >
>
    :
    public Value_<Observer, Model, Filter>
{
public:
    using Base = Value_<Observer, Model, Filter>;

    Value(Model * const model, Filter &filter)
        :
        Base(model)
    {
        this->filter_ = &filter;
    }

    Value(const Value &other): Base(other)
    {
        
    }
};


} // namespace interface

} // namespace pex
