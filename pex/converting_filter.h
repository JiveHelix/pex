#pragma once

#include <cmath>
#include <jive/overflow.h>
#include <jive/platform.h>
#include <jive/optional.h>
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


template<typename SetType_, typename GetType_>
struct ConvertingFilter
{
    using GetType = GetType_;
    using SetType = SetType_;

    static GetType Get(Argument<SetType> value)
    {
        if constexpr (jive::IsOptional<SetType>)
        {
            if (!value)
            {
                return std::nullopt;
            }

            CHECK_RANGE(jive::RemoveOptional<GetType>, *value);

            return static_cast<GetType>(*value);
        }
        else
        {
            CHECK_RANGE(GetType, value);

            return static_cast<GetType>(value);
        }
    }

    static SetType Set(Argument<GetType> value)
    {
        if constexpr (jive::IsOptional<GetType>)
        {
            if (!value)
            {
                return std::nullopt;
            }

            CHECK_RANGE(jive::RemoveOptional<SetType>, *value);

            return static_cast<SetType>(*value);
        }
        else
        {
            CHECK_RANGE(SetType, value);

            return static_cast<SetType>(value);
        }
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
    typename Upstream,
    typename Converted,
    typename Access = pex::GetAndSetTag
>
using ConvertingValue = Value_<
    Upstream,
    ConvertingFilter<typename Upstream::Type, Converted>,
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
template<typename T>
struct LinearFilter
{
    using PlainType = jive::RemoveOptional<T>;
    using Type = T;
    using PlainFilter = LinearFilter<PlainType>;

    static_assert(std::is_floating_point_v<PlainType>);
    using IntType = jive::MatchOptional<T, int>;

    LinearFilter()
        :
        slope_(1)
    {

    }

    LinearFilter(PlainType slope)
        :
        slope_(slope)
    {
        if (slope == 0)
        {
            throw std::logic_error("Cannot divide by zero");
        }
    }

    IntType Get(Type value) const
    {
        if constexpr (jive::IsOptional<Type>)
        {
            if (!value)
            {
                return value;
            }

            PlainType result = (*value) * this->slope_;

            if constexpr (std::is_floating_point_v<PlainType>)
            {
                result = round(result);
            }

            return static_cast<int>(result);
        }
        else
        {
            PlainType result = value * this->slope_;

            if constexpr (std::is_floating_point_v<PlainType>)
            {
                result = round(result);
            }

            return static_cast<int>(result);
        }
    }

    Type Set(IntType value) const
    {
        if constexpr (jive::IsOptional<Type>)
        {
            if (!value)
            {
                return value;
            }

            return static_cast<PlainType>(*value) / this->slope_;
        }
        else
        {
            return static_cast<PlainType>(value) / this->slope_;
        }
    }

    void SetSlope(PlainType slope)
    {
        this->slope_ = slope;
    }

    PlainType GetSlope() const
    {
        return this->slope_;
    }

private:
    PlainType slope_;
};


template<typename T, ssize_t slope>
struct StaticLinearFilter
{
    using Type = T;
    using PlainType = jive::RemoveOptional<T>;
    using IntType = jive::MatchOptional<T, int>;

    static_assert(std::is_floating_point_v<PlainType>);
    static_assert(slope != 0, "Cannot divide by zero");

    static IntType Get(Type value)
    {
        if constexpr (jive::IsOptional<Type>)
        {
            if (!value)
            {
                return value;
            }

            PlainType result = (*value) * static_cast<PlainType>(slope);

            if constexpr (std::is_floating_point_v<PlainType>)
            {
                result = round(result);
            }

            return static_cast<int>(result);
        }
        else
        {
            PlainType result = value * static_cast<PlainType>(slope);

            if constexpr (std::is_floating_point_v<PlainType>)
            {
                result = round(result);
            }

            return static_cast<int>(result);
        }
    }

    static Type Set(IntType value)
    {
        if constexpr (jive::IsOptional<Type>)
        {
            if (!value)
            {
                return value;
            }

            return static_cast<PlainType>(value)
                / static_cast<PlainType>(slope);
        }
        else
        {
            return static_cast<PlainType>(value)
                / static_cast<PlainType>(slope);
        }
    }
};


} // end namespace control


} // end namespace pex
