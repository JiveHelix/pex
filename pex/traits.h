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


template<typename T>
concept IsSignalModel = T::isSignalModel;

template<typename T>
concept IsSignalControl = T::isSignalControl;


template<typename T>
concept IsSignal = IsSignalModel<T> || IsSignalControl<T>;


template<typename T>
concept IsPexCopyable = T::isPexCopyable;


template<typename T, typename = void>
struct FilterIsNoneOrFree_: std::true_type {};


template<typename T>
struct FilterIsNoneOrFree_
<
    T,
    std::enable_if_t
    <
        detail::FilterIsMember
        <
            typename T::UpstreamType,
            typename T::Filter
        >
    >
>: std::false_type {};


template<typename T>
inline constexpr bool FilterIsNoneOrFree = FilterIsNoneOrFree_<T>::value;


/** If T is a pex::model::Value, it cannot be copied. Also, if it
 ** has a Filter with member functions, then allowing it to be copied
 ** breaks the ability to change the Filter instance. These values must not
 ** be copied.
 **/
template<typename T>
concept IsCopyable =
    !IsModel<T>
    && !IsSignalModel<T>
    && FilterIsNoneOrFree<T>
    && IsPexCopyable<T>;



#if 0
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
        !IsSignalModel<Pex>
        &&
        !detail::FilterIsMember
        <
            typename Pex::UpstreamType,
            typename Pex::Filter
        >
        && IsPexCopyable<Pex>
    >
>: std::true_type {};


template<typename Pex>
inline constexpr bool IsCopyable = IsCopyable_<Pex>::value;
#endif


template<typename T>
concept IsValueContainer = std::remove_cvref_t<T>::isValueContainer;

template<typename T>
concept IsKeyValueContainer = T::isKeyValueContainer;


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
    std::enable_if_t
    <
        !IsCopyable<T>
        && !IsValueContainer<T>
        && !IsKeyValueContainer<T>
    >
>
{
    using Type = ::pex::model::Direct<T>;
};


template<typename T>
struct UpstreamHolder_
<
    T,
    std::enable_if_t
    <
        !IsCopyable<T>
        && IsValueContainer<T>
    >
>
{
    using Type = ::pex::model::DirectValueContainer<T>;
};

template<typename T>
struct UpstreamHolder_
<
    T,
    std::enable_if_t
    <
        !IsCopyable<T>
        && IsKeyValueContainer<T>
    >
>
{
    using Type = ::pex::model::DirectKeyValueContainer<T>;
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



template<typename T>
concept IsGroupModel = T::isGroupModel;

template<typename T>
concept IsGroupControl = T::isGroupControl;

template<typename T>
concept IsGroupMux = T::isGroupMux;


template<typename T>
inline constexpr bool IsGroupNode =
    IsGroupModel<T>
    || IsGroupControl<T>
    || IsGroupMux<T>;


template<typename T>
concept IsGroup = T::isGroup;


template<typename T>
concept IsList = T::isList;

template<typename T>
concept IsListControl = T::isListControl;

template<typename T>
concept IsListModel = T::isListModel;

template<typename T>
concept IsListMux = T::isListMux;

template<typename T>
concept IsListNode = IsListModel<T> || IsListControl<T> || IsListMux<T>;

template<typename T>
concept IsDerivedGroup = T::isDerivedGroup;

template<typename T>
concept IsControlWrapper = T::isControlWrapper;

template<typename T>
concept IsModelWrapper = T::isModelWrapper;

template<typename T>
concept IsRangeModel = T::isRangeModel;

template<typename T>
concept IsRangeControl = T::isRangeControl;

template<typename T>
concept IsRangeMux = T::isRangeMux;

template<typename T>
concept IsRangeNode =
    IsRangeModel<T>
    || IsRangeControl<T>
    || IsRangeMux<T>;

template<typename T>
concept IsSelectModel = T::isSelectModel;

template<typename T>
concept IsSelectControl = T::isSelectControl;

template<typename T>
concept IsSelectMux = T::isSelectMux;

template<typename T>
concept IsSelectNode =
    IsSelectModel<T>
    || IsSelectControl<T>
    || IsSelectMux<T>;

template<typename T>
concept IsAggregate = T::isAggregate;

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
