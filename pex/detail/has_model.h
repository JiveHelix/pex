#pragma once


#include <jive/for_each.h>
#include <fields/reflect.h>
#include <fields/has_fields.h>


namespace pex
{


namespace detail
{


template<typename T>
bool HasModel(const T &group)
{
    bool result = true;

    if constexpr (fields::HasFields<T>)
    {
        auto modelChecker = [&group, &result](auto field)
        {
            if (result)
            {
                result = (group.*(field.member)).HasModel();
            }
        };

        jive::ForEach(T::fields, modelChecker);
    }
    else
    {
        auto modelChecker = [&result](const auto &member)
        {
            if (result)
            {
                result = member.HasModel();
            }
        };

        fields::ForEachZip(group, modelChecker);
    }

    return result;
}


} // end namespace detail


} // end namespace pex
