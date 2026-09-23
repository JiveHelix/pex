#pragma once

#ifdef ENABLE_PEX_LOG
#include <fmt/core.h>
#endif

#include <fields/assign.h>
#include <fields/describe.h>
#include "pex/tailors.h"
#include "pex/traits.h"
#include "pex/detail/mute.h"
#include "pex/detail/forward.h"
#include "pex/detail/signal_connection.h"


namespace pex
{

namespace detail
{


template<typename Target, typename Source>
std::enable_if_t<ConvertsToPlain<Target>>
AssignSourceToTarget(Target &target, const Source &source)
{
    target = source.Get();
}

template<typename Target, typename Source>
std::enable_if_t<!ConvertsToPlain<Target>>
AssignSourceToTarget(Target &, const Source &)
{

}


template
<
    typename Plain,
    typename Source
>
void PlainConvert(Plain &target, Source &source)
{
    if constexpr (fields::HasFields<Plain>)
    {
        static_assert(
            fields::HasFields<Source>,
            "Source must also have fields");

        auto doAssign = [&target, &source](
            const auto &plainField,
            const auto &sourceField) -> void
        {
            AssignSourceToTarget(
                target.*(plainField.member),
                source.*(sourceField.member));
        };

        jive::ZipApply(doAssign, Plain::fields, Source::fields);
    }
    else
    {
        auto doAssign = [](
            auto &targetMember,
            const auto &sourceMember) -> void
        {
            AssignSourceToTarget(targetMember, sourceMember);
        };

        fields::ForEachZip(target, source, doAssign);
    }
}


template
<
    typename Plain,
    typename Derived
>
struct Getter
{
    static constexpr auto observerName = "Getter";

    Plain Get() const
    {
        Plain result;
        PlainConvert(result, static_cast<const Derived &>(*this));

        return result;
    }

    explicit operator Plain () const
    {
        return this->Get();
    }
};


template<template<typename> typename Tailor>
struct EnsembleTailor_
{
    template<typename T, typename Enable = void>
    struct TailorType
    {
        using Type = Tailor<T>;
    };
};

template<template<typename> typename Tailor>
template<typename T>
struct EnsembleTailor_<Tailor>::TailorType
<
    T,
    std::enable_if_t<IsGroup<T>>
>
{
    using Type = typename T::template Ensemble<Tailor>;
};


template<template<typename> typename Tailor>
template<typename T>
struct EnsembleTailor_<Tailor>::TailorType
<
    T,
    std::enable_if_t<IsList<T>>
>
{
    using Type = ListConnect<void, Tailor<T>>;
};


template<template<typename> typename Tailor>
struct EnsembleTailor
{
    template<typename T>
    using TailorType =
        typename EnsembleTailor_<Tailor>::template TailorType<T>::Type;
};



template<typename T>
concept HasGetValue = requires(T t)
{
    { t.GetValue() };
};


template<typename T, typename = void>
struct CallbackType_
{
    using Type = typename T::Type;
};


template<typename T>
struct CallbackType_
<
    T,
    std::enable_if_t<HasGetValue<T>>
>
{
    using Type = std::remove_cvref_t<decltype(std::declval<T>().GetValue())>;
};


template<typename T>
using CallbackType = typename CallbackType_<T>::Type;


// Internal helper to allow observation of group types.
template
<
    typename Plain,
    template<template<typename> typename> typename Schema,
    template<typename> typename Tailor
>
struct Ensemble
    :
    public detail::NotifyOne
    <
        ::pex::ValueConnection<void, Plain, NoFilter>,
        GetAndSetTag
    >,
    Separator,
    public Schema<EnsembleTailor<Tailor>::template TailorType>,
    public
        Getter
        <
            Plain,
            Ensemble<Plain, Schema, Tailor>
        >
{
    using SignalConnection_ = SignalConnection<void>;
    using SignalCallable = typename SignalConnection_::Callable;

public:
    static constexpr auto observerName = "Ensemble";
    static constexpr bool isEnsemble = true;

    using Base = detail::NotifyOne
        <
            ::pex::ValueConnection<void, Plain, NoFilter>,
            GetAndSetTag
        >;

    using ValueCallable = typename Base::Callable;

#ifdef ENABLE_PEX_NAMES
    void RegisterPexNames()
    {
        // Iterate over members, and register names and addresses.

        if constexpr (fields::HasFields<Ensemble>)
        {
            auto doRegisterName = [this] (auto thisField)
            {
                PexName(
                    &(this->*(thisField.member)),
                    this,
                    fmt::format("Ensemble::{}", thisField.name));
            };

            jive::ForEach(Ensemble::fields, doRegisterName);
        }
        else
        {
            auto doRegisterName = [this] (const auto &name, const auto &member)
            {
                PexName(
                    &member,
                    this,
                    fmt::format("Ensemble::{}", name));
            };

            fields::ForEach(
                *this,
                doRegisterName);
        }
    }

    void ClearPexNames()
    {
        // Iterate over members, and register names and addresses.

        if constexpr (fields::HasFields<Ensemble>)
        {
            auto doClearName = [this] (auto thisField)
            {
                ClearPexName(&(this->*(thisField.member)));
            };

            jive::ForEach(
                Ensemble::fields,
                doClearName);
        }
        else
        {
            auto doClearName = [this] (const auto &member)
            {
                ClearPexName(&member);
            };

            fields::ForEach(*this, doClearName);
        }
    }
#endif

    Ensemble()
        :
        isMuted_(),
        muteTerminus_(),
        isModified_(false),
        memberChanged_(),
        madeConnections_(false)
    {
#ifdef ENABLE_PEX_NAMES
        PEX_NAME(fmt::format("Ensemble {}", jive::GetTypeName<Plain>()));
        this->RegisterPexNames();

        PEX_MEMBER(muteTerminus_);
#endif
    }

    template<typename Upstream>
    Ensemble(Upstream &upstream)
        :
        isMuted_(),
        muteTerminus_(),
        isModified_(false),
        memberChanged_(),
        madeConnections_(false)
    {
#ifdef ENABLE_PEX_NAMES
        PEX_NAME(fmt::format("Ensemble {}", jive::GetTypeName<Plain>()));
        this->RegisterPexNames();

        PEX_MEMBER(muteTerminus_);
#endif
        this->AssignUpstream(upstream);
    }

    template<typename Upstream>
    void AssignUpstream(Upstream &upstream)
    {
        this->UnmakeConnections_();

        this->muteTerminus_.Emplace(upstream.CloneMuteNode());

        if constexpr (fields::HasFields<Ensemble>)
        {
            auto doAssign = [this, &upstream](
                const auto &EnsembleField,
                const auto &upstreamField) -> void
            {
                this->AssignUpstream_(
                    this->*(EnsembleField.member),
                    upstream.*(upstreamField.member));
            };

            jive::ZipApply(
                doAssign,
                Ensemble::fields,
                Upstream::fields);
        }
        else
        {
            auto doAssign = [this](
                auto &EnsembleMember,
                const auto &upstreamMember) -> void
            {
                this->AssignUpstream_(EnsembleMember, upstreamMember);
            };

            fields::ForEachZip(*this, upstream, doAssign);
        }
    }

    Ensemble(const Ensemble &) = delete;
    Ensemble(Ensemble &&) = delete;
    Ensemble & operator=(const Ensemble &) = delete;
    Ensemble & operator=(Ensemble &&) = delete;

    ~Ensemble()
    {
        this->UnmakeConnections_();
        this->ClearConnections();

#ifdef ENABLE_PEX_NAMES
        this->ClearPexNames();

        PEX_CLEAR_NAME(&this->muteTerminus_);
#endif
    }

    void Connect(void *observer, ValueCallable callable)
    {
        if (!this->madeConnections_)
        {
            this->MakeConnections_();
        }

        this->Base::Connect(observer, callable);
    }

    void Disconnect(void *observer)
    {
        this->UnmakeConnections_();

        if (this->memberChanged_)
        {
            this->memberChanged_.reset();
        }

        this->Base::Disconnect(observer);
    }

    void ClearConnections()
    {
        // Calls Disconnect for all observers.
        // This is a NotifyOne, so there is at most one connection to clear.
        this->ClearConnections_();
    }

    void Notify(const Plain &plain)
    {
        this->Notify_(plain);
    }

private:
    template<typename Member, typename Upstream>
    void AssignUpstream_(Member &member, Upstream &upstream)
    {
        if constexpr (IsEnsemble<Member>)
        {
            member.AssignUpstream(upstream);
        }
        else
        {
            member = upstream;
        }
    }

    template<typename Member>
    void Connector_(Member &member)
    {
        if constexpr (!IsSignal<Member>)
        {
            using MemberType = CallbackType<Member>;

            member.Connect(
                this,
                &Ensemble::template OnMemberChanged_<MemberType>);

            if constexpr (IsEnsemble<MemberType>)
            {
                member.ConnectEnsemble_(
                    this,
                    &Ensemble::OnEnsembleMemberChanged_);
            }
        }
    }

    void ConnectEnsemble_(void *observer, SignalCallable callable)
    {
        this->memberChanged_.emplace(observer, callable);
    }

    void MakeConnections_()
    {
        this->muteTerminus_.Connect(this, &Ensemble::OnMute_);

        if constexpr (fields::HasFields<Ensemble>)
        {
            auto connector = [this](const auto &field) -> void
            {
#ifdef ENABLE_PEX_NAMES
                assert(pex::HasPexName(&(this->*(field.member))));
#endif
                this->Connector_(this->*(field.member));
            };

            jive::ForEach(Ensemble::fields, connector);
        }
        else
        {
            auto connector = [this](auto &member) -> void
            {
#ifdef ENABLE_PEX_NAMES
                assert(pex::HasPexName(&(member)));
#endif
                this->Connector_(member);
            };

            fields::ForEach(*this, connector);
        }

        this->madeConnections_ = true;
    }

    template<typename Member>
    void Disconnector_(Member &member)
    {
        if constexpr (!IsSignal<Member>)
        {
            member.Disconnect(this);
        }
    }

    void UnmakeConnections_()
    {
        if (!this->madeConnections_)
        {
            return;
        }

        this->muteTerminus_.Disconnect();

        if constexpr (fields::HasFields<Ensemble>)
        {
            auto disconnector = [this](const auto &field) -> void
            {
                this->Disconnector_(this->*(field.member));
            };

            jive::ForEach(Ensemble::fields, disconnector);
        }
        else
        {
            auto disconnector = [this](auto &member) -> void
            {
                this->Disconnector_(member);
            };

            fields::ForEach(*this, disconnector);
        }

        this->madeConnections_ = false;
    }

    template<typename T>
    static void OnMemberChanged_(void *observer, Argument<T>)
    {
        auto self = static_cast<Ensemble *>(observer);
        self->isModified_ = true;

        PEX_LOG(
            LookupPexName(self),
            " OnMemberChanged_");

        if (self->memberChanged_)
        {
            PEX_LOG(
                LookupPexName(self),
                " sending member changed notice.");

            (*self->memberChanged_)();
        }

        if (self->isMuted_)
        {
            return;
        }

        self->Notify_(self->Get());
    }

    template<typename T>
    static void OnEnsembleMemberChanged_(void *observer)
    {
        auto self = static_cast<Ensemble *>(observer);

        PEX_LOG(
            LookupPexName(self),
            " received Ensemble member changed notice.");

        self->isModified_ = true;

        if (self->memberChanged_)
        {
            PEX_LOG(
                LookupPexName(self),
                " sending member changed notice.");

            (*self->memberChanged_)();
        }
    }

    void OnMute_(const Mute_ &muteState)
    {
        if (!muteState.isMuted && !muteState.isSilenced)
        {
            // Notify observers of changed groups when unmuted.
            if (this->isModified_)
            {
                PEX_LOG(
                    LookupPexName(this),
                    " is modifified. Notifying.");

                this->isModified_ = false;
                this->Notify_(this->Get());
            }
            else
            {
                PEX_LOG(
                    LookupPexName(this),
                    " is unchanged. Skipping notification.");
            }
        }

        if (muteState.isMuted && !this->isMuted_.isMuted)
        {
            // The group has been newly muted.

            // Initialize isModified_ to false so we only notify for members
            // that have changed.
            this->isModified_ = false;
        }

        this->isMuted_ = muteState;
    }

private:
    Mute_ isMuted_;

    using MuteNode = Tailor<MakeMute>;
    using MuteTerminus = pex::Terminus<Ensemble, MuteNode>;
    MuteTerminus muteTerminus_;

    bool isModified_;
    std::optional<SignalConnection_> memberChanged_;
    bool madeConnections_;
};


} // end namespace detail


} // end namespace pex
