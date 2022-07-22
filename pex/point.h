#pragma once


#include <tau/point.h>


namespace pex
{


template<typename T>
using PointGroup = pex::Group
    <
        tau::PointFields,
        typename tau::PointTemplate<T>::Template,
        tau::Point<T>
    >;


} // end namespace pex
