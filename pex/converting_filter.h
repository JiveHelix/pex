#pragma once

#include <cmath>
#include <jive/overflow.h>
#include <jive/platform.h>
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


template<typename F, unsigned Base, unsigned Divisor>
struct LogarithmicFilter
{
    static_assert(
        std::is_floating_point_v<F>,
        "Expected a floating point type.");

    static constexpr auto base = static_cast<F>(Base);
    static constexpr auto divisor = static_cast<F>(Divisor);

    static int Get(F value)
    {
        /**
        v = b^(x/d)

        log_b(v) = (x/d)

        x = d * log_b(v)
        **/

        if constexpr (Base == 2)
        {
            return static_cast<int>(
                std::round(divisor * std::log2(value)));
        }
        else if constexpr (Base == 10)
        {
            return static_cast<int>(std::round(divisor * std::log10(value)));
        }
        else
        {
            /*

            Change-of-base
            x = d * log(v) / log(b)

            */

            return static_cast<int>(
                std::round(
                    divisor * std::log(value) / std::log(base)));
        }
    }

    static F Set(int value)
    {
        return std::pow(base, static_cast<F>(value) / divisor);
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
 ** Maps between a model value and the integer values of a control, like
 ** a slider. Slope determines the number of possible adjustment
 ** steps per integral value of the model.
 **
 ** Examples:
 ** If model ranges from 0 to 360, and the slope is 1, there will
 ** be 360 discrete adjustment steps in the slider. A slope of 2 produces 720
 ** steps, or two steps per whole number.
 **
 ** If model ranges from 0 to 1 and the slope is 100, there will be 100
 ** discrete adjustment steps.
 **
 **/
template<typename T, ssize_t slope>
struct LinearFilter
{
    static_assert(slope != 0, "Cannot divide by zero!");
    static_assert(std::is_floating_point_v<T>);

    static int Get(T value)
    {
        T result = value * static_cast<T>(slope);

        if constexpr (std::is_floating_point_v<T>)
        {
            result = round(result);
        }

        return static_cast<int>(result);
    }

    static T Set(int value)
    {
        return static_cast<T>(value) / static_cast<T>(slope);
    }
};


} // end namespace control


} // end namespace pex
