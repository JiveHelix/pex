#pragma once


#include <tau/point.h>


namespace pex
{


template<typename T>
using PointGroup = pex::Group
    <
        tau::PointFields,
        tau::PointTemplate<T>::template Template,
        tau::Point<T>
    >;


} // end namespace pex
