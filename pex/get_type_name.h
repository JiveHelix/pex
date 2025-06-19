#pragma once


#include <jive/describe_type.h>
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

        if constexpr (fields::HasFieldsTypeName<TemplateBase>)
        {
            return TemplateBase::fieldsTypeName;
        }
        else
        {
            return jive::GetTypeName<Templates>();
        }
    }
}


} // end namespace poly


} // end namespace pex
