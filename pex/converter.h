/**
  * @file converter.h
  *
  * @brief Converter between values and string representations.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 10 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include <bitset>
#include <string>
#include "jive/auto_format.h"
#include "jive/formatter.h"
#include "jive/to_integer.h"
#include "jive/to_float.h"


namespace pex
{


/** Selects the larger of width, precision, or minimumBufferSize. **/
template<typename T, int width, int precision>
struct BufferSize
{
    static constexpr int minimumBufferSize = 32;
    static constexpr int maximumSpecified =
        (width > precision)
        ? width
        : precision;

    static constexpr int value =
        (maximumSpecified > minimumBufferSize)
        ? maximumSpecified
        : minimumBufferSize;
};


template<typename T, int base, int width, int precision, typename = void>
struct ValueToString
{
    static constexpr auto format = jive::AutoFormat<T, base>::value.data();

    static std::string Call(T value)
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
        return jive::Formatter<BufferSize<T, width, precision>::value>(
            format,
            width,
            precision,
            value);
#pragma GCC diagnostic pop
    }
};


/** Specialization for std::string that doesn't create an extra copy. **/
template<typename T, int base, int width, int precision>
struct ValueToString
<
    T,
    base,
    width,
    precision,
    std::enable_if_t<std::is_same_v<T, std::string>>
>
{
    template<typename U>
    static U Call(U &&value)
    {
        return std::forward<U>(value);
    }
};


template<typename T>
struct IsBitset : std::false_type {};

template<size_t N>
struct IsBitset<std::bitset<N>> : std::true_type {};

/** Specialization for std::bitset **/
template<typename T, int base, int width, int precision>
struct ValueToString
<
    T,
    base,
    width,
    precision,
    std::enable_if_t<IsBitset<T>::value>
>
{
    template<typename U>
    static std::string Call(U &&value)
    {
        return value.to_string();
    }
};


template<typename T, int base, typename = void>
struct StringToValue
{
    static T Call(T &&asString)
    {
        return std::forward<T>(asString);
    }
};

template<typename T, int base>
struct StringToValue<T, base, std::enable_if_t<std::is_integral_v<T>>>
{
    static T Call(const std::string_view &asString)
    {
        return jive::ToInteger<T, base>(asString);
    }
};

template<typename T, int base>
struct StringToValue<T, base, std::enable_if_t<std::is_floating_point_v<T>>>
{
    static T Call(const std::string_view &asString)
    {
        return jive::ToFloat<T>(asString);
    }
};

template<typename T, int base>
struct StringToValue<T, base, std::enable_if_t<IsBitset<T>::value>>
{
    static T Call(const std::string_view &asString)
    {
        return T(std::string(asString));
    }
};


struct DefaultConverterTraits
{
    static constexpr int base = 10;
    static constexpr int width = -1;
    static constexpr int precision = -1;
};


template<typename T, typename Traits = DefaultConverterTraits>
struct Converter
{
    using ToStringFunctor =
        ValueToString<T, Traits::base, Traits::width, Traits::precision>;

    using ToValueFunctor = StringToValue<T, Traits::base>;

    template<typename U>
    static std::string ToString(U &&value)
    {
        return ToStringFunctor::Call(std::forward<U>(value));
    }

    template<typename U>
    static T ToValue(U &&asString)
    {
        return ToValueFunctor::Call(std::forward<U>(asString));
    }
};


} // namespace pex
