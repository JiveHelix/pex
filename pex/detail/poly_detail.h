#pragma once


#include <memory>


namespace pex
{


namespace poly
{


namespace detail
{


template<typename Value_>
struct ControlBase_
{
    using Value = Value_;

    virtual ~ControlBase_() {}
    virtual Value GetValue() const = 0;
    virtual void SetValue(const Value &) = 0;
    virtual std::string_view GetTypeName() const = 0;
};


template<typename Value_>
struct ModelBase_
{
    using Value = Value_;
    using ControlPtr = std::shared_ptr<ControlBase_<Value>>;

    virtual ~ModelBase_() {}
    virtual Value GetValue() const = 0;
    virtual void SetValue(const Value &) = 0;
    virtual std::string_view GetTypeName() const = 0;
    virtual ControlPtr MakeControl() = 0;
};


template<typename ...T>
struct FirstType_ {};


template<typename First, typename ...T>
struct FirstType_<First, T...>
{
    using Type = First;
};


template<typename ...T>
using FirstType = typename FirstType_<T...>::Type;


template<typename ...Derived>
struct Creator_
{
    using Base = typename FirstType<Derived...>::Base;
    static_assert((std::is_same_v<Base, typename Derived::Base> && ...));

private:
    template<typename T, typename Json>
    static bool MakeDerived(std::shared_ptr<Base> &result, const Json &json)
    {
        if (json["type"] == T::fieldsTypeName)
        {
            result = std::make_shared<T>(fields::Structure<T>(json));
            return true;
        }

        return false;
    }

    template<typename Json>
    static std::shared_ptr<Base> MakeDerived(const Json &json)
    {
        std::shared_ptr<Base> result{};

        // Call MakeDerived with each derived type until a match is found.
        static_cast<void>(
            ((MakeDerived<Derived>(result, json) ? false : true) && ...));

        return result;
    }

public:
    template<typename Json>
    static std::shared_ptr<Base> Structure(const Json &jsonValues)
    {
        if (!jsonValues.contains("type"))
        {
            throw std::runtime_error(
                "Cannot structure an Aircraft without type information.");
        }

        auto result = MakeDerived<Json>(jsonValues);

        if (!result)
        {
            throw std::runtime_error("Unknown type");
        }

        return result;
    }
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


} // end namespace detail


} // end namespace poly


} // end namespace pex
