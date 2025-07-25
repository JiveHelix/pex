#pragma once

#include <optional>
#include <utility>
#include <fields/assign.h>
#include <jive/for_each.h>
#include "pex/reference.h"
#include "pex/selectors.h"
#include "pex/detail/value_connection.h"
#include "pex/detail/aggregate.h"


namespace pex
{

template
<
    template<typename> typename Fields,
    typename Target,
    typename Source
>
void Assign(Target &target, Source &source)
{
    auto initializer = [&target, &source](
        const auto &targetField,
        const auto &sourceField) -> void
    {
        // Target member must have a Set function.
        (target.*(targetField.member)).Set(source.*(sourceField.member));
    };

    jive::ZipApply(
        initializer,
        Fields<Target>::fields,
        Fields<Source>::fields);
}


template<typename Target, typename Source>
std::enable_if_t<detail::CanBeSet<Target>>
SetWithoutNotify(Target &target, const Source &source)
{
    detail::AccessReference(target).SetWithoutNotify(source);
}

template<typename Target, typename Source>
std::enable_if_t<!detail::CanBeSet<Target>>
SetWithoutNotify(Target &, const Source &)
{

}


template<typename Target, typename Source>
std::enable_if_t<HasSetInitial<Target>>
DoSetInitial(Target &target, const Source &source)
{
    target.SetInitial(source);
}

template<typename Target, typename Source>
std::enable_if_t<!HasSetInitial<Target>>
DoSetInitial(Target &target, const Source &source)
{
    if constexpr (!IsSignal<Target>)
    {
        detail::AccessReference(target).SetWithoutNotify(source);
    }
}

template<typename Target>
std::enable_if_t<detail::CanBeSet<Target>>
DoNotify(Target &target)
{
    detail::AccessReference(target).DoNotify();
}

template<typename Target>
std::enable_if_t<!detail::CanBeSet<Target>>
DoNotify(Target &)
{

}


template<typename T, typename Enable = void>
struct HasMute: std::false_type {};


template<typename T>
struct HasMute
<
    T,
    std::invoke_result_t<decltype(&T::Mute), T>
>: std::true_type {};


template
<
    typename Plain_,
    template<typename> typename Fields_,
    template<template<typename> typename> typename Template_,
    template<typename> typename Selector,
    typename Derived
>
class GroupAccessors
    :
    public detail::Getter<Plain_, Fields_, Derived>
{
public:
    static constexpr bool isGroupAccessor = true;

    using Plain = Plain_;

    template<typename T>
    using Fields = Fields_<T>;

    template<template<typename> typename T>
    using GroupTemplate = Template_<T>;

public:
    void RegisterPexNames(void *groupAddress)
    {
        auto derived = static_cast<Derived *>(this);

        // Iterate over members, and register names and addresses.
        auto doRegisterNames = [derived, groupAddress] (auto thisField)
        {
            RegisterPexName(
                &(derived->*(thisField.member)),
                groupAddress,
                thisField.name);
        };

        jive::ForEach(
            Fields<Derived>::fields,
            doRegisterNames);
    }

    void UnregisterPexNames()
    {
        auto derived = static_cast<Derived *>(this);

        // Iterate over members, and register names and addresses.
        auto doUnregisterNames = [derived] (auto thisField)
        {
            UnregisterPexName(
                &(derived->*(thisField.member)),
                thisField.name);
        };

        jive::ForEach(
            Fields<Derived>::fields,
            doUnregisterNames);
    }

    void Mute()
    {
        auto derived = static_cast<Derived *>(this);

        if (derived->IsMuted())
        {
            return;
        }

        derived->DoMute();

        // Iterate over members, muting those that support it.
        auto doMute = [derived] (auto thisField)
        {
            using Member = typename std::remove_reference_t<
                decltype(derived->*(thisField.member))>;

            if constexpr (HasMute<Member>::value)
            {
                (derived->*(thisField.member)).Mute();
            }
        };

        jive::ForEach(
            Fields<Derived>::fields,
            doMute);
    }

    void Unmute()
    {
        auto derived = static_cast<Derived *>(this);

        if (!derived->IsMuted())
        {
            return;
        }

        // Iterate over members, unmuting those that support it.
        auto doUnmute = [derived] (auto thisField)
        {
            using Member = typename std::remove_reference_t<
                decltype(derived->*(thisField.member))>;

            if constexpr (HasMute<Member>::value)
            {
                (derived->*(thisField.member)).Unmute();
            }
        };

        jive::ForEach(
            Fields<Derived>::fields,
            doUnmute);

        derived->DoUnmute();
    }

    void Set(const Plain &plain)
    {
        // DeferGroup will notify members of changes after all values have been
        // set.
        // The aggregate notification will follow.
        DeferGroup<Fields, Template_, Selector, Derived> deferGroup(
            static_cast<Derived &>(*this));

        deferGroup.Set(plain);
    }

    // Initialize values without sending notifications.
    void SetInitial(const Plain &plain)
    {
        auto derived = static_cast<Derived *>(this);

        auto setInitial = [derived, &plain]
            (auto thisField, auto plainField)
        {
            DoSetInitial(
                derived->*(thisField.member),
                plain.*(plainField.member));
        };

        jive::ZipApply(
            setInitial,
            Fields<Derived>::fields,
            Fields<Plain>::fields);

        // this->DoNotify_();
    }

    template<typename>
    friend class Reference;

protected:
    void SetWithoutNotify_(const Plain &plain)
    {
        auto derived = static_cast<Derived *>(this);

        auto setWithoutNotify = [derived, &plain]
            (auto thisField, auto plainField)
        {
            SetWithoutNotify(
                derived->*(thisField.member),
                plain.*(plainField.member));
        };

        jive::ZipApply(
            setWithoutNotify,
            Fields<Derived>::fields,
            Fields<Plain>::fields);
    }

    void DoNotify_()
    {
        auto derived = static_cast<Derived *>(this);

        auto doNotify = [derived] (auto thisField)
        {
            DoNotify(derived->*(thisField.member));
        };

        jive::ForEach(
            Fields<Derived>::fields,
            doNotify);
    }
};


#ifdef ENABLE_REGISTER_NAME

#define REGISTER_PEX_NAMES(groupAddress) this->RegisterPexNames(groupAddress)

#else

#define REGISTER_PEX_NAMES(groupAddress)

#endif


} // end namespace pex
