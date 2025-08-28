#pragma once


#include <jive/for_each.h>


namespace pex
{


namespace detail
{


template<template<typename> typename Fields, typename T>
bool HasModel(const T &group)
{
    bool result = true;

    auto modelChecker = [&group, &result](auto field)
    {
        if (result)
        {
            result = (group.*(field.member)).HasModel();
            assert(result);
        }
    };

    jive::ForEach(Fields<T>::fields, modelChecker);

    return result;
}


} // end namespace detail


} // end namespace pex
