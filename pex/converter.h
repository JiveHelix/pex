/**
  * @file converter.h
  *
  * @brief Convert between values and string representations.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 10 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include <string>
#include <iostream>
#include <jive/auto_format.h>
#include <jive/formatter.h>
#include <jive/to_integer.h>
#include <jive/to_float.h>
#include <jive/type_traits.h>
#include <jive/optional.h>


namespace pex
{


/** Selects the larger of width, precision, or minimumBufferSize. **/
template<typename T, typename Traits>
struct BufferSize
{
    static constexpr int minimumBufferSize = 32;
    static constexpr int maximumSpecified =
        (Traits::width > Traits::precision)
        ? Traits::width
        : Traits::precision;

    static constexpr int value =
        (maximumSpecified > minimumBufferSize)
        ? maximumSpecified
        : minimumBufferSize;
};


template<int base_, int width_, int precision_, typename Flag_>
struct ConverterTraits
{
    static constexpr int base = base_;
    static constexpr int width = width_;
    static constexpr int precision = precision_;
    using Flag = Flag_;
};


using DefaultConverterTraits =
    ConverterTraits<10, 0, -1, jive::flag::Alternate>;


template<typename T, typename Traits, typename = void>
struct ValueToString
{
    static constexpr auto format =
        jive::FixedFormat
        <
            jive::RemoveOptional<T>,
            Traits::base,
            typename Traits::Flag
        >::value.data();

    static std::string Call(T value)
    {
        if constexpr (std::is_same_v<bool, T>)
        {
            if (value)
            {
                return "true";
            }

            return "false";
        }
        else
        {

#ifndef _WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#endif
            return jive::Formatter<BufferSize<T, Traits>::value>(
                format,
                Traits::width,
                Traits::precision,
                value);

#ifndef _WIN32
#pragma GCC diagnostic pop
#endif
        }
    }
};


/** Specialization for std::string that doesn't create an extra copy. **/
template<typename T, typename Traits>
struct ValueToString
<
    T,
    Traits,
    std::enable_if_t<std::is_same_v<T, std::string>>
>
{
    template<typename U>
    static U Call(U &&value)
    {
        return std::forward<U>(value);
    }
};


/** Specialization for std::bitset **/
template<typename T, typename Traits>
struct ValueToString
<
    T,
    Traits,
    std::enable_if_t<jive::IsBitset<T>::value>
>
{
    template<typename U>
    static std::string Call(U &&value)
    {
        return value.to_string();
    }
};


/**
 ** StringToValue with a floating-point or integral value may throw
 ** std::out_of_range or std::invalid_argument if the conversion is not
 ** possible.
 **
 ** Additional specializations a create a direct pass-through for std::string
 ** and conversions to std::bitset<N>
 **/
template<typename T, int base, typename = void>
struct StringToValue
{
    template<typename U>
    static T Call(U &&asString)
    {
        return std::forward<U>(asString);
    }
};


template<typename T, int base>
struct StringToValue
<
    T,
    base,
    std::enable_if_t
    <
        std::is_integral_v<T>
        && !std::is_same_v<bool, T>
    >
>
{
    static T Call(const std::string &asString)
    {
        return jive::ToInteger<T, base>(asString);
    }
};


template<typename T, int base>
struct StringToValue<T, base, std::enable_if_t<std::is_same_v<bool, T>>>
{
    static std::string Upper(const std::string &value)
    {
        std::string result;
        result.reserve(value.size());

        std::transform(
            std::begin(value),
            std::end(value),
            std::back_inserter(result),
            [](char c)
            {
                return static_cast<char>(
                    std::toupper(static_cast<unsigned char>(c)));
            });

        return result;
    }

    static T Call(const std::string &asString)
    {
        // This is not the most efficient way, but it is easier to read.
        return Upper(jive::strings::Trim(asString)) == "TRUE";
    }
};


template<typename T, int base>
struct StringToValue<T, base, std::enable_if_t<std::is_floating_point_v<T>>>
{
    static T Call(const std::string &asString)
    {
        return jive::ToFloat<T>(asString);
    }
};


template<typename T, int base>
struct StringToValue<T, base, std::enable_if_t<jive::IsBitset<T>::value>>
{
    static T Call(const std::string &asString)
    {
        // std::bitset is constructed from a std::string of 1's and 0's.
        return T(std::string(asString));
    }
};


template<typename T, typename Traits = DefaultConverterTraits>
struct Converter
{
    using ConvertToString = ValueToString<jive::RemoveOptional<T>, Traits>;

    using ConvertToValue = StringToValue<jive::RemoveOptional<T>, Traits::base>;

    template<typename U>
    static std::string ToString(U &&value)
    {
        static_assert(std::is_same_v<std::remove_cvref_t<U>, T>);

        if constexpr (jive::IsOptional<T>)
        {
            if (!value)
            {
                return {};
            }

            return ConvertToString::Call(*value);
        }
        else
        {
            return ConvertToString::Call(std::forward<U>(value));
        }
    }

    template<typename U>
    static T ToValue(U &&asString)
    {
        if constexpr (jive::IsOptional<T>)
        {
            if (asString.empty())
            {
                return {};
            }

            return ConvertToValue::Call(asString);
        }
        else
        {
            return ConvertToValue::Call(std::forward<U>(asString));
        }
    }
};


template<typename Converter, typename T, typename = void>
struct HasToString_: std::false_type {};

template<typename Converter, typename T>
struct HasToString_
<
    Converter,
    T,
    std::enable_if_t
    <
        std::is_invocable_r_v
        <
            std::string,
            decltype(&Converter::template ToString<T>),
            T
        >
    >
>: std::true_type {};


template<typename Converter, typename T>
static constexpr bool HasToString = HasToString_<Converter, T>::value;


} // namespace pex
