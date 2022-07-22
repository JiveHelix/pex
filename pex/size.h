#pragma once


#include <tau/size.h>


namespace pex
{


template<typename T>
using SizeGroup = pex::Group
    <
        tau::SizeFields,
        typename tau::SizeTemplate<T>::Template,
        tau::Size<T>
    >;


} // end namespace pex
