#pragma once


#include <string>
#include <type_traits>
#include <jive/describe_type.h>


namespace pex
{


namespace detail
{


template<typename T>
concept HasObserverName =
    requires
    {
        { std::string{ T::observerName } } -> std::same_as<std::string>;
    };


template<typename T, typename = void>
struct ObserverName_
{
    static constexpr auto value = jive::GetTypeName<T>();
};


template<typename T>
struct ObserverName_<T, std::enable_if_t<std::is_void_v<T>>>
{
    static constexpr auto value = "void";
};


template<typename T>
struct ObserverName_<T, std::enable_if_t<HasObserverName<T>>>
{
    static constexpr auto value = T::observerName;
};


template<typename T>
inline constexpr auto ObserverName = ObserverName_<T>::value;


} // end namespace detail


} // end namespace pex
