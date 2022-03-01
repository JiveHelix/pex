#pragma once


#include <type_traits>


namespace pex
{


template<typename A, typename B, typename Enable = void>
struct IsCompatible: std::false_type {};


template<typename A, typename B>
struct IsCompatible
<
    A,
    B,
    std::enable_if_t
    <
        (std::is_same_v<typename A::Model, typename B::Model>
        && std::is_same_v<typename A::Access, typename B::Access>)
    >
>: std::true_type {};


template<typename A, typename B>
inline constexpr bool IsCompatibleV = IsCompatible<A, B>::value;


} // end namespace pex
