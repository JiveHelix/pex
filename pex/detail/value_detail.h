#pragma once

#include <type_traits>
#include "pex/detail/notify.h"

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


template<typename T, typename Filter, typename = void>
struct FilterIsMember: std::false_type {};

template<typename T, typename Filter>
struct FilterIsMember
<
    T,
    Filter,
    std::enable_if_t
    <
        GetterIsMember<T, Filter>::value || SetterIsMember<T, Filter>::value
    >
> : std::true_type {};


template<typename T, typename = void>
struct ImplementsConnect: std::false_type {};

template<typename T>
struct ImplementsConnect
<
    T,
    std::enable_if_t
    <
        std::is_invocable_v
        <
            decltype(&T::Connect),
            T,
            void * const,
            UnboundValueCallable<void, typename T::Type>
        >
    >
> : std::true_type {};


template<typename T, typename = void>
struct ImplementsDisconnect: std::false_type {};

template<typename T>
struct ImplementsDisconnect
<
    T,
    std::enable_if_t
    <
        std::is_invocable_v
        <
            decltype(&T::Disconnect),
            T,
            void * const
        >
    >
> : std::true_type {};


template<typename T, typename = void>
struct DefinesType: std::false_type {};

template<typename T>
struct DefinesType<T, std::void_t<typename T::Type>>
    : std::true_type {};


template<typename T, typename Filter, typename = void>
struct ModelFilterIsVoidOrValid: std::false_type {};


/** Filter can be void **/
template<typename T, typename Filter>
struct ModelFilterIsVoidOrValid
<
    T,
    Filter,
    std::enable_if_t<std::is_same_v<Filter, void>>
> : std::true_type {};


/** Filter::Set can be normal function, or a function that takes a pointer
 ** to Filter.
 **/
template<typename T, typename Filter>
struct ModelFilterIsVoidOrValid
<
    T,
    Filter,
    std::enable_if_t<SetterIsValid<T, Filter>::value>
> : std::true_type {};


template<typename T, typename Filter, typename = void>
struct InterfaceFilterIsVoidOrValid : std::false_type {};


/** Filter can be void **/
template<typename T, typename Filter>
struct InterfaceFilterIsVoidOrValid
<
    T,
    Filter,
    std::enable_if_t<std::is_same_v<Filter, void>>
> : std::true_type {};


template<typename T, typename Filter>
struct InterfaceFilterIsVoidOrValid
<
    T,
    Filter,
    std::enable_if_t
    <
        GetterIsValid<T, Filter>::value
        && SetterIsValid<T, Filter>::value
    >
> : std::true_type {};


} // namespace detail

} // namespace pex


#ifndef NDEBUG
#define NOT_NULL(pointer)                                     \
    if (pointer == nullptr)                                   \
    {                                                         \
        throw std::logic_error(#pointer " must not be NULL"); \
    }

#else
#define NOT_NULL(pointer)
#endif




