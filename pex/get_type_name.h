#pragma once


#include <fields/core.h>


namespace pex
{


namespace poly
{


template<typename Templates>
std::string_view GetTypeName()
{
    if constexpr (fields::HasFieldsTypeName<Templates>)
    {
        return Templates::fieldsTypeName;
    }
    else
    {
        using TemplateBase =
            typename Templates::template Template<fields::Identity>;

        return TemplateBase::fieldsTypeName;
    }
}


} // end namespace poly


} // end namespace pex
