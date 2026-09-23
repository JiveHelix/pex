#pragma once

#include <optional>
#include <utility>
#include <fields/assign.h>
#include <jive/for_each.h>
#include "pex/reference.h"
#include "pex/tailors.h"
#include "pex/detail/value_connection.h"
#include "pex/detail/ensemble.h"


namespace pex
{


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
    target.Notify();
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
    template<template<typename> typename> typename Schema_,
    template<typename> typename Tailor,
    typename Derived
>
class GroupAccessors
    :
    public detail::Getter<Plain_, Derived>
{
public:
    static constexpr bool isGroupAccessor = true;

    using Plain = Plain_;

    template<template<typename> typename T>
    using GroupSchema = Schema_<T>;

public:
#ifdef ENABLE_PEX_NAMES
    void RegisterPexNames(void *groupAddress)
    {
        auto derived = static_cast<Derived *>(this);

        // Iterate over members, and register names and addresses.

        if constexpr (fields::HasFields<Derived>)
        {
            auto doRegisterNames = [derived, groupAddress] (auto thisField)
            {
                PexName(
                    &(derived->*(thisField.member)),
                    groupAddress,
                    thisField.name);
            };

            jive::ForEach(Derived::fields, doRegisterNames);
        }
        else
        {
            auto doRegisterNames = [groupAddress] (
                const auto &name,
                const auto &member)
            {
                PexName(&member, groupAddress, name);
            };

            fields::ForEach(
                *derived,
                doRegisterNames);
        }
    }

    void UnregisterPexNames()
    {
        auto derived = static_cast<Derived *>(this);

        // Iterate over members, and register names and addresses.

        if constexpr (fields::HasFields<Derived>)
        {
            auto doUnregisterNames = [derived] (auto thisField)
            {
                ClearPexName(&(derived->*(thisField.member)));
            };

            jive::ForEach(Derived::fields, doUnregisterNames);
        }
        else
        {
            auto doUnregisterNames = [] (const auto &member)
            {
                ClearPexName(&member);
            };

            fields::ForEach(*derived, doUnregisterNames);
        }
    }

#endif // ENABLE_PEX_NAMES

    void Mute()
    {
        auto derived = static_cast<Derived *>(this);

        if (derived->IsMuted())
        {
            return;
        }

        derived->DoMute();

        // Iterate over members, muting those that support it.

        if constexpr (fields::HasFields<Derived>)
        {
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
                Derived::fields,
                doMute);
        }
        else
        {
            auto doMute = [] (auto &member)
            {
                using Member = std::remove_reference_t<decltype(member)>;

                if constexpr (HasMute<Member>::value)
                {
                    member.Mute();
                }
            };

            fields::ForEach(*derived, doMute);
        }
    }

    void Unmute()
    {
        auto derived = static_cast<Derived *>(this);

        if (!derived->IsMuted())
        {
            return;
        }

        // Iterate over members, unmuting those that support it.
        if constexpr (fields::HasFields<Derived>)
        {
            auto doUnmute = [derived] (auto thisField)
            {
                using Member = typename std::remove_reference_t<
                    decltype(derived->*(thisField.member))>;

                if constexpr (HasMute<Member>::value)
                {
                    (derived->*(thisField.member)).Unmute();
                }
            };

            jive::ForEach(Derived::fields, doUnmute);
        }
        else
        {
            auto doUnmute = [] (auto &member)
            {
                using Member = std::remove_reference_t<decltype(member)>;

                if constexpr (HasMute<Member>::value)
                {
                    member.Unmute();
                }
            };

            fields::ForEach(*derived, doUnmute);
        }

        derived->DoUnmute();
    }

    void Set(const Plain &plain)
    {
        // DeferGroup will notify members of changes after all values have been
        // set.
        // The ensemble notification will follow.
        DeferGroup<Schema_, Tailor, Derived> deferGroup(
            static_cast<Derived &>(*this));

        deferGroup.Set(plain);
    }

    void Set(const Plain &plain) const
    {
        const_cast<GroupAccessors *>(this)->Set(plain);
    }

    // Initialize values without sending notifications.
    void SetInitial(const Plain &plain)
    {
        auto derived = static_cast<Derived *>(this);

        if constexpr (fields::HasFields<Derived>)
        {
            auto setInitial = [derived, &plain]
                (auto thisField, auto plainField)
            {
                DoSetInitial(
                    derived->*(thisField.member),
                    plain.*(plainField.member));
            };

            jive::ZipApply(setInitial, Derived::fields, Plain::fields);
        }
        else
        {
            static_assert(
                fields::CanReflect<Derived>,
                "Without fields, Derived must support reflection");

            auto setInitial = [] (auto &targetMember, const auto &plainMember)
            {
                DoSetInitial(targetMember, plainMember);
            };

            fields::ForEachZip(*derived, plain, setInitial);
        }
    }

    void Notify()
    {
        auto derived = static_cast<Derived *>(this);

        if constexpr (fields::HasFields<Derived>)
        {
            auto doNotify = [derived] (auto thisField)
            {
                DoNotify(derived->*(thisField.member));
            };

            jive::ForEach(Derived::fields, doNotify);
        }
        else
        {
            auto doNotify = [] (auto &member)
            {
                DoNotify(member);
            };

            fields::ForEach(*derived, doNotify);
        }
    }

    template<typename>
    friend class Reference;

protected:
    void SetWithoutNotify_(const Plain &plain)
    {
        auto derived = static_cast<Derived *>(this);

        if constexpr (fields::HasFields<Derived>)
        {
            auto setWithoutNotify = [derived, &plain]
                (auto thisField, auto plainField)
            {
                SetWithoutNotify(
                    derived->*(thisField.member),
                    plain.*(plainField.member));
            };

            jive::ZipApply(setWithoutNotify, Derived::fields, Plain::fields);
        }
        else
        {
            auto setWithoutNotify = []
                (auto &thisMember, const auto &plainMember)
            {
                SetWithoutNotify(thisMember, plainMember);
            };

            fields::ForEachZip(*derived, plain, setWithoutNotify);
        }
    }

    void SetWithoutNotify_(const Plain &plain) const
    {
        const_cast<GroupAccessors *>(this)->SetWithoutNotify_(plain);
    }
};


#ifdef ENABLE_PEX_NAMES

#define PEX_NAMES(groupAddress) this->RegisterPexNames(groupAddress)
#define CLEAR_PEX_NAMES this->UnregisterPexNames()

#else

#define PEX_NAMES(groupAddress)
#define CLEAR_PEX_NAMES

#endif


} // end namespace pex
