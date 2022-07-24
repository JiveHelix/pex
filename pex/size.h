#pragma once


#include <tau/size.h>
#include "pex/group.h"


namespace pex
{


template<typename T>
using SizeGroup = pex::Group
    <
        tau::SizeFields,
        tau::SizeTemplate<T>::template Template,
        tau::Size<T>
    >;


} // end namespace pex
