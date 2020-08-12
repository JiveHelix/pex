#pragma once

#include <vector>
#include "wxshim.h"


namespace pex
{

namespace wx
{


template<typename Converter, typename Vector>
wxArrayString MakeArrayString(Vector &&items)
{
    wxArrayString result;
    result.Alloc(items.size());

    for (auto &it: items)
    {
        result.Add(Converter::ToString(it));
    }

    return result;
}


} // namespace wx

} // namespace pex
