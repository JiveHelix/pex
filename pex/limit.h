#pragma once


#include <type_traits>


namespace pex
{


template<
    signed integral,
    unsigned fractional = 0,
    unsigned denominator = 1000000>
struct Limit
{
    template<typename T = double>
    static constexpr T Get()
    {
        if constexpr (std::is_integral_v<T>)
        {
            return static_cast<T>(integral);
        }
        else
        {
            return static_cast<T>(integral)
                + (static_cast<T>(fractional)
                    / static_cast<T>(denominator));
        }
    }
};


struct DefaultLimit {};


template<typename T>
struct IsLimit_: std::false_type {};


template<signed i, unsigned f, unsigned d>
struct IsLimit_<Limit<i, f, d>>: std::true_type {};

template<>
struct IsLimit_<DefaultLimit>: std::true_type {};


template<typename T>
concept IsLimit = IsLimit_<T>::value;


template<typename T>
concept IsDefaultLimit = std::same_as<T, DefaultLimit>;


template<typename T, IsLimit Value, typename Enable = void>
struct Minimum
{
    static constexpr auto value = Value::template Get<T>();
};

template<typename T, IsLimit Value>
struct Minimum<T, Value, std::enable_if_t<IsDefaultLimit<Value>>>
{
    static constexpr auto value = std::numeric_limits<T>::lowest();
};

template<typename T, IsLimit Value, typename Enable = void>
struct Maximum
{
    static constexpr auto value = Value::template Get<T>();
};

template<typename T, IsLimit Value>
struct Maximum<T, Value, std::enable_if_t<IsDefaultLimit<Value>>>
{
    static constexpr auto value = std::numeric_limits<T>::max();
};


} // end namespace pex
