#pragma once

#ifdef ENABLE_PEX_LOG
#include <fmt/core.h>
#endif

#include <fields/assign.h>
#include <fields/describe.h>
#include "pex/selectors.h"
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
    template<typename> typename Fields,
    typename Plain,
    typename Source
>
void PlainConvert(Plain &target, Source &source)
{
    auto initializer = [&target, &source](
        const auto &plainField,
        const auto &sourceField) -> void
    {
        AssignSourceToTarget(
            target.*(plainField.member),
            source.*(sourceField.member));
    };

    jive::ZipApply(
        initializer,
        Fields<Plain>::fields,
        Fields<Source>::fields);
}


template
<
    typename Plain,
    template<typename> typename Fields,
    typename Derived
>
struct Getter
{
    static constexpr auto observerName = "Getter";

    Plain Get() const
    {
        Plain result;

        PlainConvert<Fields>(
            result,
            static_cast<const Derived &>(*this));

        return result;
    }

    explicit operator Plain () const
    {
        return this->Get();
    }
};


template<template<typename> typename Selector>
struct AggregateSelector_
{
    template<typename T, typename Enable = void>
    struct Template
    {
        using Type = Selector<T>;
    };
};

template<template<typename> typename Selector>
template<typename T>
struct AggregateSelector_<Selector>::Template
<
    T,
    std::enable_if_t<IsGroup<T>>
>
{
    using Type = typename T::template Aggregate<Selector>;
};


template<template<typename> typename Selector>
template<typename T>
struct AggregateSelector_<Selector>::Template
<
    T,
    std::enable_if_t<IsList<T>>
>
{
    using Type = ListConnect<void, Selector<T>>;
};


template<template<typename> typename Selector>
struct AggregateSelector
{
    template<typename T>
    using Template =
        typename AggregateSelector_<Selector>::template Template<T>::Type;
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


// Internal helper to allow observation of aggregate types.
template
<
    typename Plain,
    template<typename> typename Fields,
    template<template<typename> typename> typename Template,
    template<typename> typename Selector
>
struct Aggregate
    :
    public detail::NotifyOne
    <
        ::pex::ValueConnection<void, Plain, NoFilter>,
        GetAndSetTag
    >,
    Separator,
    public Template<AggregateSelector<Selector>::template Template>,
    public
        Getter
        <
            Plain,
            Fields,
            Aggregate<Plain, Fields, Template, Selector>
        >
{
    using SignalConnection_ = SignalConnection<void>;
    using SignalCallable = typename SignalConnection_::Callable;

public:
    static constexpr auto observerName = "Aggregate";
    static constexpr bool isAggregate = true;

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
        auto doRegisterName = [this] (auto thisField)
        {
            PexName(
                &(this->*(thisField.member)),
                this,
                fmt::format("Aggregate::{}", thisField.name));
        };

        jive::ForEach(
            Fields<Aggregate>::fields,
            doRegisterName);
    }

    void ClearPexNames()
    {
        // Iterate over members, and register names and addresses.
        auto doClearName = [this] (auto thisField)
        {
            ClearPexName(&(this->*(thisField.member)));
        };

        jive::ForEach(
            Fields<Aggregate>::fields,
            doClearName);
    }
#endif

    Aggregate()
        :
        isMuted_(),
        muteTerminus_(),
        isModified_(false),
        memberChanged_(),
        madeConnections_(false)
    {
#ifdef ENABLE_PEX_NAMES
        PEX_NAME(fmt::format("Aggregate {}", jive::GetTypeName<Plain>()));
        this->RegisterPexNames();

        PEX_MEMBER(muteTerminus_);
#endif
    }

    template<typename Upstream>
    Aggregate(Upstream &upstream)
        :
        isMuted_(),
        muteTerminus_(),
        isModified_(false),
        memberChanged_(),
        madeConnections_(false)
    {
#ifdef ENABLE_PEX_NAMES
        PEX_NAME(fmt::format("Aggregate {}", jive::GetTypeName<Plain>()));
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

        auto doAssign = [this, &upstream](
            const auto &aggregateField,
            const auto &upstreamField) -> void
        {
            this->AssignUpstream_(
                this->*(aggregateField.member),
                upstream.*(upstreamField.member));
        };

        jive::ZipApply(
            doAssign,
            Fields<Aggregate>::fields,
            Fields<Upstream>::fields);
    }

    Aggregate(const Aggregate &) = delete;
    Aggregate(Aggregate &&) = delete;
    Aggregate & operator=(const Aggregate &) = delete;
    Aggregate & operator=(Aggregate &&) = delete;

    ~Aggregate()
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
    void AssignUpstream_(Member &member, Upstream upstream)
    {
        if constexpr (IsAggregate<Member>)
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
                &Aggregate::template OnMemberChanged_<MemberType>);

            if constexpr (IsAggregate<MemberType>)
            {
                member.ConnectAggregate_(
                    this,
                    &Aggregate::OnAggregateMemberChanged_);
            }
        }
    }

    void ConnectAggregate_(void *observer, SignalCallable callable)
    {
        this->memberChanged_.emplace(observer, callable);
    }

    void MakeConnections_()
    {
        this->muteTerminus_.Connect(this, &Aggregate::OnMute_);

        auto connector = [this](const auto &field) -> void
        {
#ifdef ENABLE_PEX_NAMES
            assert(pex::HasPexName(&(this->*(field.member))));
#endif
            this->Connector_(this->*(field.member));
        };

        jive::ForEach(Fields<Aggregate>::fields, connector);

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

        auto disconnector = [this](const auto &field) -> void
        {
            this->Disconnector_(this->*(field.member));
        };

        jive::ForEach(Fields<Aggregate>::fields, disconnector);

        this->madeConnections_ = false;
    }

    template<typename T>
    static void OnMemberChanged_(void *observer, Argument<T>)
    {
        auto self = static_cast<Aggregate *>(observer);
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
    static void OnAggregateMemberChanged_(void *observer)
    {
        auto self = static_cast<Aggregate *>(observer);

        PEX_LOG(
            LookupPexName(self),
            " received aggregate member changed notice.");

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

    using MuteNode = Selector<MakeMute>;
    using MuteTerminus = pex::Terminus<Aggregate, MuteNode>;
    MuteTerminus muteTerminus_;

    bool isModified_;
    std::optional<SignalConnection_> memberChanged_;
    bool madeConnections_;
};


} // end namespace detail


} // end namespace pex
