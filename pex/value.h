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
    using Type = T;
};

template<typename T>
struct Argument<T, std::enable_if_t<!std::is_arithmetic_v<T>>>
{
    using Type = const T &;
};



template<typename Observer, typename T>
using UnboundValueCallable =
    void (*)(Observer * const observer, typename Argument<T>::Type value);

template<typename Observer, typename T>
using BoundValueCallable =
    void (Observer::*)(typename Argument<T>::Type value);

// Use bound notification methods for all Observers except void.
template<typename Observer, typename T, typename = void>
struct CallableStyle;

template<typename Observer, typename T>
struct CallableStyle<Observer, T, std::enable_if_t<std::is_void_v<Observer>>>
{
    using Type = UnboundValueCallable<Observer, T>;
};

template<typename Observer, typename T>
struct CallableStyle<Observer, T, std::enable_if_t<!std::is_void_v<Observer>>>
{
    using Type = BoundValueCallable<Observer, T>;
};

template<typename Observer, typename T>
using ValueCallable = typename CallableStyle<Observer, T>::Type;

template<typename Observer, typename T>
class ValueNotify: public Notify_<Observer, ValueCallable<Observer, T>>
{
public:
    using Type = T;
    using Base = Notify_<Observer, ValueCallable<Observer, T>>;
    using argumentType = typename Argument<T>::Type;
    using Base::Base;

    void operator()(typename Argument<T>::Type value)
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

    void Set(typename detail::Argument<T>::Type value)
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

    explicit Value_(T value)
        :
        value_{value},
        filter_{}
    {

    }

    T value_;
    Filter * filter_;
};


template<typename T, typename Filter = void, typename = void>
class FilteredValue;


/** Filter provides static methods or Filter is void **/
template<typename T, typename Filter>
class FilteredValue
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
    explicit FilteredValue(T value): Base(value) {}
    FilteredValue(): Base() {}
};


/**
 ** Filter provides member methods.
 ** Constructor must get a reference to the filter instance.
 ** **/
template<typename T, typename Filter>
class FilteredValue
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
    explicit FilteredValue(Filter &filter)
        :
        Value_<T, Filter>()
    {
        this->filter_ = &filter;
    }

    explicit FilteredValue(Filter &filter, T value)
        :
        Value_<T, Filter>(value)
    {
        this->filter_ = &filter;
    }
};


template<typename T>
struct Value: public FilteredValue<T, void> {};


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


template<typename Model, typename = void>
struct ModelImplementsConnect: std::false_type {};


template<typename Model>
struct ModelImplementsConnect
<
    Model,
    std::enable_if_t
    <
        std::is_invocable_v
        <
            decltype(&Model::Connect),
            Model,
            void * const,
            detail::UnboundValueCallable<void, typename Model::Type>
        >
    >
> : std::true_type {};


template<typename Model, typename = void>
struct ModelImplementsDisconnect: std::false_type {};

template<typename Model>
struct ModelImplementsDisconnect
<
    Model,
    std::enable_if_t
    <
        std::is_invocable_v
        <
            decltype(&Model::Disconnect),
            Model,
            void * const
        >
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
    Value_(): model_(nullptr), filter_(nullptr) {}

    explicit Value_(Model * const model)
        :
        model_(model),
        filter_(nullptr)
    {
        if constexpr (ModelImplementsConnect<Model>::value)
        {
            this->model_->Connect(this, &Value_::OnModelChanged_);
        }
    }

public:
    ~Value_()
    {
        if constexpr (ModelImplementsDisconnect<Model>::value)
        {
            if (this->model_)
            {
                this->model_->Disconnect(this);
            }
        }
    }

    explicit Value_(const Value_ &other)
        :
        model_(other.model_),
        filter_(nullptr)
    {
        if constexpr (ModelImplementsConnect<Model>::value)
        {
            if (this->model_)
            {
                this->model_->Connect(this, &Value_::OnModelChanged_);
            }
        }
    }

    Value_ & operator=(const Value_ &other)
    {
        this->model_ = other.model_;
        this->filter_ = other.filter_;

        if constexpr (ModelImplementsConnect<Model>::value)
        {
            if (this->model_)
            {
                this->model_->Connect(this, &Value_::OnModelChanged_);
            }
        }

        return *this;
    }

    /** Implicit bool conversion returns true if the interface is currently
     ** tracking a Model and a Filter, if it is not void.
     **/
    operator bool () const
    {
        if constexpr (std::is_void_v<Filter>)
        {
            return this->model_ != nullptr;
        }
        else
        {
            // Filter has been specified.
            return (this->model_ != nullptr) && (this->filter_ != nullptr);
        }
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

    void Set(typename detail::Argument<T>::Type value)
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
    T FilterSet_(typename detail::Argument<T>::Type value) const
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

    T FilterGet_(typename detail::Argument<T>::Type value) const
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
        typename detail::Argument<T>::Type value)
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


template<typename Model, typename = void>
struct ModelDefinesType: std::false_type {};

template<typename Model>
struct ModelDefinesType<Model, std::void_t<typename Model::Type>>
    : std::true_type {};


template<
    typename Observer,
    typename Model,
    typename Filter,
    typename = void>
class FilteredValue
{
    static_assert(
        ModelDefinesType<Model>::value,
        "Any class used as the Model must define Model::Type");
};


/** Filter provides static methods or Filter is void **/
template
<
    typename Observer,
    typename Model,
    typename Filter
>
class FilteredValue
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

    FilteredValue(): Base()
    {

    }

    explicit FilteredValue(Model * const model): Base(model)
    {

    }

    explicit FilteredValue(const FilteredValue &other): Base(other)
    {

    }

    FilteredValue & operator=(const FilteredValue &other)
    {
        Base::operator=(other);
        return *this;
    }
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
class FilteredValue
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

    FilteredValue(): Base() {}

    explicit FilteredValue(Model * const model, Filter &filter)
        :
        Base(model)
    {
        this->filter_ = &filter;
    }

    explicit FilteredValue(const FilteredValue &other): Base(other)
    {

    }

    FilteredValue & operator=(const FilteredValue &other)
    {
        Base::operator=(other);
        return *this;
    }
};


template<typename Observer, typename ModelType>
using Value = FilteredValue<Observer, ModelType, void>;


template<typename Observer>
struct BoundFilteredValue
{
    template<typename ModelType, typename Filter>
    using Type = FilteredValue<Observer, ModelType, Filter>;
};


template<typename Observer>
struct BoundValue
{
    template<typename ModelType>
    using Type = FilteredValue<Observer, ModelType, void>;
};


} // namespace interface

} // namespace pex
