#pragma once


#include <jive/overflow.h>
#include "pex/platform.h"
#include "pex/control_value.h"


namespace pex
{

namespace control
{

template<typename Target, typename Source>
void RequireConvertible(Source value)
{
    if (!jive::CheckConvertible<Target>(value))
    {
        throw std::range_error("value is not convertible to target");
    }
}


#ifndef NDEBUG
#define CHECK_RANGE(Target, value) RequireConvertible<Target>(value)
#else
// Release mode. No safety checks!
#define CHECK_RANGE(Target, value)
#endif


template<typename SetType, typename GetType>
struct ConvertingFilter
{
    static GetType Get(SetType value)
    {
        CHECK_RANGE(GetType, value);
        return static_cast<GetType>(value);
    }

    static SetType Set(GetType value)
    {
        CHECK_RANGE(SetType, value);
        return static_cast<SetType>(value);
    }
};


template
<
    typename Observer,
    typename Pex,
    typename Converted,
    typename Access = pex::GetAndSetTag
>
using ConvertingValue = Value_<
    Observer,
    Pex,
    ConvertingFilter<typename Pex::Type, Converted>,
    Access>;


/**
 ** Converts between a floating-point value and an integer.
 **/
template<typename T, typename I, ssize_t slope, ssize_t offset>
struct LinearFilter
{
    static_assert(slope != 0, "Cannot divide by zero!");
    static_assert(std::is_floating_point_v<T>);

    static I Get(T value)
    {
        T result = value * static_cast<T>(slope) + static_cast<T>(offset);

        if constexpr (std::is_integral_v<I>)
        {
            result = round(result);
        }

        return static_cast<I>(result);
    }

    static T Set(I value)
    {
        return static_cast<T>(value - static_cast<I>(offset))
            / static_cast<T>(slope);
    }
};


} // end namespace control


} // end namespace pex
