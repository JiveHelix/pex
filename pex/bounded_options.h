#pragma once


#include <pex/limit.h>


namespace pex
{


template
<
    typename T,
    IsLimit initialMinimum_,
    IsLimit initialMaximum_,
    typename Filter_ = ::pex::NoFilter,
    template<typename, typename, typename>
    typename ValueNode_ = ::pex::DefaultValueNode
>
struct BoundedOptions
{
    using Type = T;
    using LimitType = jive::RemoveOptional<Type>;
    using initialMinimum = initialMinimum_;
    using initialMaximum = initialMaximum_;

    // Defaults to numeric_limits<T>::lowest() and ::max()
    static constexpr auto minimum = Minimum<LimitType, initialMinimum>::value;
    static constexpr auto maximum = Maximum<LimitType, initialMaximum>::value;

    using Filter = Filter_;

    template<typename U, typename V, typename W>
    using ValueNode = ValueNode_<U, V, W>;
};


template<typename T>
using DefaultBoundedOptions = BoundedOptions<T, DefaultLimit, DefaultLimit>;


} // end namespace pex
