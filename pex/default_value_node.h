#pragma once

#include <pex/model_value.h>
#include <pex/control_value.h>


namespace pex
{


template<typename T, typename Filter, typename Access>
struct DefaultValueNode
{
    using Type = T;
    using Model = model::Value_<T, Filter, Access>;

    template
    <
        typename Upstream,
        typename ControlFilter,
        typename ControlAccess
    >
    using Control = control::Value_<Upstream, ControlFilter, ControlAccess>;

    using Mux = control::Mux<Model>;

    template<typename ControlFilter, typename ControlAccess>
    using Follow = control::Value_<Mux, ControlFilter, ControlAccess>;
};


} // end namespace pex
