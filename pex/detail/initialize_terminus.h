#pragma once


#include <jive/for_each.h>
#include <jive/zip_apply.h>


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

#if 0

template<typename Observer, typename Terminus, typename Upstream>
void DoAssign(Observer *observer, Terminus &terminus, Upstream &upstream)
{
    terminus.Assign(observer, Terminus(observer, upstream));
}


template
<
    template<typename> typename Fields,
    typename Observer,
    typename T,
    typename Upstream
>
void InitializeTerminus(
    Observer *observer,
    T &terminus,
    Upstream &upstream)
{
    assert(HasModel<Fields>(upstream));

    auto initializer = [&terminus, observer, &upstream](
        auto terminusField,
        auto upstreamField)
    {
        DoAssign(
            observer,
            terminus.*(terminusField.member),
            upstream.*(upstreamField.member));
    };

    jive::ZipApply(
        initializer,
        Fields<T>::fields,
        Fields<Upstream>::fields);
}


template
<
    template<typename> typename Fields,
    typename T,
    typename U,
    typename Observer
>
void CopyTerminus(Observer *observer, T &terminus, const U &other)
{
    auto initializer = [&terminus, &other, observer](
        auto leftField,
        auto rightField)
    {
        PEX_LOG("Move assign ", leftField.name);

        (terminus.*(leftField.member)).Assign(
            observer,
            other.*(rightField.member));
    };

    jive::ZipApply(
        initializer,
        Fields<T>::fields,
        Fields<U>::fields);
}


template
<
    template<typename> typename Fields,
    typename T,
    typename U,
    typename Observer
>
void MoveTerminus(Observer *observer, T &terminus, U &other)
{
    auto initializer = [&terminus, &other, observer](
        auto leftField,
        auto rightField)
    {
        PEX_LOG("Move assign ", leftField.name);

        (terminus.*(leftField.member)).Assign(
            observer,
            std::move(other.*(rightField.member)));
    };

    jive::ZipApply(
        initializer,
        Fields<T>::fields,
        Fields<U>::fields);
}

#endif

} // end namespace detail


} // end namespace pex
