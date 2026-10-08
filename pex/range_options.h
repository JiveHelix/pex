#pragma once


#include <pex/limit.h>


namespace pex
{


template
<
    typename T,
    IsLimit initialMinimum,
    IsLimit initialMaximum,
    typename Filter_ = ::pex::NoFilter,
    template<typename, typename, typename>
    typename ValueNode_ = ::pex::DefaultValueNode
>
struct RangeOptions
{
    using Type = T;
    using LimitType = jive::RemoveOptional<Type>;

    // Defaults to numeric_limits<T>::lowest() and ::max()
    static constexpr auto minimum = Minimum<LimitType, initialMinimum>::value;
    static constexpr auto maximum = Maximum<LimitType, initialMaximum>::value;

    using Filter = Filter_;

    template<typename U, typename V, typename W>
    using ValueNode = ValueNode_<U, V, W>;
};


template<typename T>
using DefaultRangeOptions = RangeOptions<T, DefaultLimit, DefaultLimit>;


} // end namespace pex
