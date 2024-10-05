#pragma once


#include <type_traits>
#include "pex/model_value.h"
#include "pex/signal.h"
#include "pex/access_tag.h"


namespace pex
{


template<typename ...T>
struct IsModel_: std::false_type {};

template<typename ...T>
struct IsModel_<pex::model::Value_<T...>>: std::true_type {};

template<typename ...T>
inline constexpr bool IsModel = IsModel_<T...>::value;


template<typename Pex>
struct IsDirect_: std::false_type {};

template<typename Pex>
struct IsDirect_<pex::model::Direct<Pex>>: std::true_type {};

template<typename Pex>
inline constexpr bool IsDirect = IsDirect_<Pex>::value;


template<template<typename...> typename Base, typename Derived>
struct IsBaseOf_
{
    template<typename ...T>
    static constexpr std::true_type  DoTest_(const Base<T...> *);

    static constexpr std::false_type DoTest_(...);

    using Type = decltype(DoTest_(std::declval<Derived *>()));
};

template<template<typename...> class Base, typename Derived>
using IsBaseOf = typename IsBaseOf_<Base, Derived>::Type;


template<typename T, typename enable = void>
struct IsModelSignal_: std::false_type {};

template<typename T>
struct IsModelSignal_
<
    T,
    std::enable_if_t
    <
        std::is_same_v<T, pex::model::Signal>
    >
>: std::true_type {};

template<typename T>
inline constexpr bool IsModelSignal = IsModelSignal_<T>::value;

#if 0
template<typename ...T>
struct IsBaseControlSignal_: std::false_type {};

template<typename ...T>
struct IsBaseControlSignal_<pex::control::Signal<T...>>: std::true_type {};
#endif

template<typename T, typename Enable = void>
struct DefinesIsControlSignal_: std::false_type {};

template<typename T>
struct DefinesIsControlSignal_
<
    T,
    std::enable_if_t<T::isControlSignal>
>
: std::true_type {};

template<typename T>
inline constexpr bool DefinesIsControlSignal =
    DefinesIsControlSignal_<T>::value;

template<typename T, typename Enable = void>
struct IsControlSignal_: std::false_type {};

template<typename T>
struct IsControlSignal_
    <
        T,
        std::enable_if_t<DefinesIsControlSignal<T>>
    >: std::true_type {};

template<typename T>
inline constexpr bool IsControlSignal = IsControlSignal_<T>::value;


// TODO: Add IsTerminusSignal_ to all MakeDefer to operate on groups that
// contain a Signal.


template<typename T, typename enable = void>
struct IsSignal_: std::false_type {};

template<typename T>
struct IsSignal_
<
    T,
    std::enable_if_t
    <
        IsModelSignal<T> || IsControlSignal<T>
    >
>: std::true_type {};

template<typename T>
inline constexpr bool IsSignal = IsSignal_<T>::value;


template<typename T, typename Enable = void>
struct DefinesPexCopyable_: std::false_type {};

template<typename T>
struct DefinesPexCopyable_
<
    T,
    std::enable_if_t<T::isPexCopyable>
>
: std::true_type {};

template<typename T>
inline constexpr bool DefinesPexCopyable = DefinesPexCopyable_<T>::value;


/** If Pex is a pex::model::Value, it cannot be copied. Also, if it
 ** has a Filter with member functions, then allowing it to be copied
 ** breaks the ability to change the Filter instance. These Pex values must not
 ** be copied.
 **/
template<typename Pex, typename = void>
struct IsCopyable_: std::false_type {};


template<typename Pex>
struct IsCopyable_
<
    Pex,
    std::enable_if_t
    <
        !IsModel<Pex>
        &&
        !IsModelSignal<Pex>
        &&
        !detail::FilterIsMember
        <
            typename Pex::UpstreamType,
            typename Pex::Filter
        >
        && DefinesPexCopyable<Pex>
    >
>: std::true_type {};


template<typename Pex>
inline constexpr bool IsCopyable = IsCopyable_<Pex>::value;


/**
 ** Copyable Upstream may be stored directly, else use Direct.
 **/
template<typename T, typename Enable = void>
struct UpstreamHolder_
{
    using Type = T;
};


template<typename T>
struct UpstreamHolder_
<
    T,
    std::enable_if_t<!IsCopyable<T>>
>
{
    using Type = ::pex::model::Direct<T>;
};


template<typename T>
using UpstreamHolderT = typename UpstreamHolder_<T>::Type;

/** UpstreamHolderT **/


/**
 ** When passed as a constructor argument, non-copyable types will be
 ** passed by reference. Otherwise, a copy will be made. This allows
 ** a control::Value to be an rvalue.
 **/
template<typename T, typename = void>
struct PexArgument_
{
    using Type = T;
};

template<typename T>
struct PexArgument_<T, std::enable_if_t<!IsCopyable<T>>>
{
    using Type = T &;
};


template<typename T>
using PexArgument = typename PexArgument_<T>::Type;

/** PexArgument **/


template<typename T, typename = void>
struct HasDefault_: std::false_type {};

template<typename T>
struct HasDefault_
<
    T,
    std::enable_if_t
    <
        std::is_invocable_r_v
        <
            T,
            decltype(&T::Default)
        >
    >
>: std::true_type {};


template<typename T>
static constexpr bool HasDefault = HasDefault_<T>::value;


template<typename T, typename = void>
struct DefinesDefer: std::false_type {};


template<typename T>
struct DefinesDefer<T, std::void_t<typename T::Defer>>
    : std::true_type {};


template<typename T, typename Enable = void>
struct IsGroupAccessor_: std::false_type {};

template<typename T>
struct IsGroupAccessor_
<
    T,
    std::enable_if_t<T::isGroupAccessor>
>
: std::true_type {};

template<typename T>
inline constexpr bool IsGroupAccessor = IsGroupAccessor_<T>::value;


// Signals do not have an underlying type, so they are not part of the
// conversion to a plain-old data structure.
template<typename Pex, typename Enable = void>
struct ConvertsToPlain_: std::true_type {};

// Detects model and control signals.
template<typename Pex>
struct ConvertsToPlain_
<
    Pex,
    std::enable_if_t<IsSignal<Pex>>
>: std::false_type {};

// Detects a Terminus signal.
template<typename Pex>
struct ConvertsToPlain_
<
    Pex,
    std::enable_if_t<IsSignal<typename Pex::Pex>>
>: std::false_type {};

template<typename Pex>
struct ConvertsToPlain_
<
    Pex,
    std::enable_if_t<std::is_same_v<DescribeSignal, Pex>>
>: std::false_type {};

template<typename Pex>
inline constexpr bool ConvertsToPlain = ConvertsToPlain_<Pex>::value;



template<typename T, typename = void>
struct IsGroupModel_: std::false_type {};

template<typename T>
struct IsGroupModel_<T, std::enable_if_t<T::isGroupModel>>
    :
    std::true_type
{

};

template<typename T>
inline constexpr bool IsGroupModel = IsGroupModel_<T>::value;


template<typename T, typename = void>
struct IsGroupControl_: std::false_type {};

template<typename T>
struct IsGroupControl_<T, std::enable_if_t<T::isGroupControl>>
    :
    std::true_type
{

};

template<typename T>
inline constexpr bool IsGroupControl = IsGroupControl_<T>::value;


template<typename T>
inline constexpr bool IsGroupNode = IsGroupModel<T> || IsGroupControl<T>;

template<typename T>
concept IsGroup = T::isGroup;


template<typename T>
concept IsList = T::isList;

template<typename T>
concept IsListControl = T::isListControl;

template<typename T>
concept IsListModel = T::isListModel;

template<typename T>
concept IsListNode = IsListModel<T> || IsListControl<T>;


template<typename T>
concept IsPolyGroup = T::isPolyGroup;

template<typename T>
concept IsPolyControl = T::isPolyControl;

template<typename T>
concept IsPolyModel = T::isPolyModel;


template<typename T>
concept HasValueBase = requires { typename T::ValueBase; };

template<typename T>
concept HasSupers = requires { typename T::Supers; };


template<typename T>
concept HasMinimalSupers = HasSupers<T> && HasValueBase<typename T::Supers>;


template<typename T>
concept IsAccess = std::is_base_of_v<AccessTag, T>;


template<typename T>
concept DefinesAccess =
    requires { typename T::Access; } && IsAccess<typename T::Access>;


template<typename T, typename Enable = void>
struct GetAccess_
{
    using Type = GetAndSetTag;
};


template<typename T>
struct GetAccess_
<
    T,
    std::enable_if_t<DefinesAccess<T>>
>
{
    using Type = typename T::Access;
};


template<typename T>
using GetAccess = typename GetAccess_<T>::Type;


template<typename T>
concept IsPointer = std::is_pointer_v<T>;


template<typename T>
concept IsPolymorphic = std::is_polymorphic_v<std::remove_pointer_t<T>>;


template<typename T>
concept HasGetVirtual = requires(T t)
{
    { t.GetVirtual() } -> IsPointer;
    { t.GetVirtual() } -> IsPolymorphic;
};


template<typename T>
concept IsVoid = std::is_void_v<T>;


template<typename T>
concept HasSetInitial = requires(T t)
{
    { t.SetInitial(std::declval<typename T::Type>()) } -> IsVoid;
};


} // end namespace pex
