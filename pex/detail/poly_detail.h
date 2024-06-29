#pragma once


#include <memory>
#include "pex/detail/traits.h"


namespace pex
{


namespace poly
{


namespace detail
{


/**
 ** ControlBase_ declares virtual methods that allow its derived classes to be
 ** in a pex::List. (These are mostly used internally by pex.)
 ** A user can add their own virtual interface with ControlUserBase.
 **/
template<typename Value_, typename ControlUserBase>
class ControlBase_: public ControlUserBase
{
public:
    using Value = Value_;
    using ValueBase = typename Value_::ValueBase;

    virtual ~ControlBase_() {}
    virtual Value GetValue() const = 0;
    virtual void SetValue(const Value &) = 0;
    virtual std::string_view GetTypeName() const = 0;

    using Callable = std::function<void(void *, const Value &)>;
    virtual void Connect(void *observer, Callable callable) = 0;
    virtual void Disconnect(void *observer) = 0;

    virtual void SetValueWithoutNotify(const Value &) = 0;
    virtual void DoValueNotify() = 0;

    virtual std::shared_ptr<ControlBase_> Copy() const = 0;
};


/**
 ** ModelBase_ declares virtual methods that allow its derived classes to be
 ** in a pex::List. (These are mostly used internally by pex.)
 ** A user can add their own virtual interface with ModelUserBase.
 **/
template<typename Value_, typename ModelUserBase, typename ControlBase>
class ModelBase_: public ModelUserBase
{
public:
    using Value = Value_;
    using ValueBase = typename Value_::ValueBase;
    using ControlPtr = std::shared_ptr<ControlBase>;

    virtual ~ModelBase_() {}
    virtual Value GetValue() const = 0;
    virtual void SetValue(const Value &) = 0;
    virtual std::string_view GetTypeName() const = 0;
    virtual ControlPtr MakeControl() = 0;
    virtual void SetValueWithoutNotify(const Value &) = 0;
    virtual void DoValueNotify() = 0;
};


template<typename T, typename = void>
struct BaseHasDescribe_: std::false_type {};


template<typename T>
struct BaseHasDescribe_
<
    T,
    std::enable_if_t
    <
        std::is_same_v
        <
            std::ostream &,
            decltype(
                std::declval<T>().Describe(
                    std::declval<std::ostream &>(),
                    std::declval<fields::Style>(),
                    std::declval<int>()))
        >
    >
>: std::true_type {};


template<typename T, typename = void>
struct BaseHasUnstructure_: std::false_type {};

template<typename T>
struct BaseHasUnstructure_
<
    T,
    std::enable_if_t
    <
        std::is_same_v
        <
            typename T::Json,
            decltype(std::declval<T>().Unstructure())
        >
    >
>: std::true_type {};


template<typename T, typename = void>
struct BaseHasOperatorEquals_: std::false_type {};

template<typename T>
struct BaseHasOperatorEquals_
<
    T,
    std::enable_if_t
    <
        std::is_same_v
        <
            bool,
            decltype(std::declval<T>().operator==(std::declval<T>()))
        >
    >
>: std::true_type {};


template<typename T, typename = void>
struct BaseHasGetTypeName_: std::false_type {};

template<typename T>
struct BaseHasGetTypeName_
<
    T,
    std::enable_if_t
    <
        std::is_same_v
        <
            std::string_view,
            decltype(std::declval<T>().GetTypeName())
        >
    >
>: std::true_type {};


template<typename T, typename = void>
struct BaseHasCopy_: std::false_type {};

template<typename T>
struct BaseHasCopy_
<
    T,
    std::enable_if_t
    <
        std::is_same_v
        <
            std::shared_ptr<T>,
            decltype(std::declval<T>().Copy())
        >
    >
>: std::true_type {};


template<typename T, typename Enable = void>
struct IsCompatibleBase_: std::false_type {};


template<typename T>
struct IsCompatibleBase_
<
    T,
    std::enable_if_t
    <
        std::is_polymorphic_v<T>
        && BaseHasDescribe_<T>::value
        && BaseHasUnstructure_<T>::value
        && BaseHasOperatorEquals_<T>::value
        && BaseHasGetTypeName_<T>::value
        && BaseHasCopy_<T>::value
    >
>: std::true_type {};


template<typename T>
inline constexpr bool IsCompatibleBase = IsCompatibleBase_<T>::value;


template<typename T, typename Enable = void>
struct VirtualBase_
{
    using Type = T;
};


template<typename T>
struct VirtualBase_<T, std::void_t<typename T::Base>>
{
    using Type = typename T::Base;
};


struct Empty {};


template<typename Supers, typename = void>
struct MakeControlUserBase_
{
    using Type = Empty;
};


template<typename Supers>
struct MakeControlUserBase_
<
    Supers,
    std::enable_if_t<::pex::detail::HasControlUserBase<Supers>>
>
{
    using Type = Supers::ControlUserBase;
};


template<typename Supers>
using MakeControlUserBase = typename MakeControlUserBase_<Supers>::Type;


template<typename Supers, typename = void>
struct MakeModelUserBase_
{
    using Type = Empty;
};


template<typename Supers>
struct MakeModelUserBase_
<
    Supers,
    std::enable_if_t<::pex::detail::HasModelUserBase<Supers>>
>
{
    using Type = Supers::ModelUserBase;
};


template<typename Supers>
using MakeModelUserBase = typename MakeModelUserBase_<Supers>::Type;


} // end namespace detail


} // end namespace poly


} // end namespace pex
