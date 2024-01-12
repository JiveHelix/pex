#pragma once

#include <fields/assign.h>
#include <fields/describe.h>
#include "pex/selectors.h"
#include "pex/traits.h"
#include "pex/detail/mute.h"
#include "pex/detail/forward.h"


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

template<typename T, typename Enable = void>
struct IsAggregate_: std::false_type {};

template<typename T>
struct IsAggregate_
<
    T,
    std::enable_if_t<T::isAggregate>
>
: std::true_type {};

template<typename T>
inline constexpr bool IsAggregate = IsAggregate_<T>::value;



// AggregateSelector selects group members that are themselves groups.
template<typename T, typename Enable = void>
struct AggregateSelector_
{
    using Type = ControlSelector<T>;
};

template<typename T>
struct AggregateSelector_<T, std::enable_if_t<IsGroup<T>>>
{
    using Type = typename T::Aggregate;
};

template<typename T>
struct AggregateSelector_<T, std::enable_if_t<IsMakeList<T>>>
{
    using Type = ListConnect<void, ControlSelector<T>>;
};


template<typename T>
using AggregateSelector = typename AggregateSelector_<T>::Type;


// Internal helper to allow observation of aggregate types.
template
<
    typename Plain,
    template<typename> typename Fields,
    template<template<typename> typename> typename Template
>
struct Aggregate:
    public Template<AggregateSelector>,
    public Getter<Plain, Fields, Aggregate<Plain, Fields, Template>>,
    public detail::NotifyOne
    <
        ::pex::ValueConnection<void, Plain, NoFilter>,
        GetAndSetTag
    >
{
public:
    static constexpr auto observerName = "Aggregate";
    static constexpr bool isAggregate = true;

    using Base = detail::NotifyOne
        <
            ::pex::ValueConnection<void, Plain, NoFilter>,
            GetAndSetTag
        >;

    Aggregate()
        :
        isMuted_(false)
    {

    }

    template<typename Upstream>
    Aggregate(Upstream &upstream)
        :
        isMuted_(false)
    {
        this->AssignUpstream(upstream);
    }

    template<typename Upstream>
    void AssignUpstream(Upstream &upstream)
    {
        this->UnmakeConnections_();

        this->muteTerminus_.Assign(
            this,
            MuteTerminus<Aggregate>(this, upstream.CloneMuteControl()));

        this->muteTerminus_.Connect(&Aggregate::OnMute_);

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

        this->MakeConnections_();
    }

    Aggregate(const Aggregate &) = delete;
    Aggregate(Aggregate &&) = delete;
    Aggregate & operator=(const Aggregate &) = delete;
    Aggregate & operator=(Aggregate &&) = delete;

    ~Aggregate()
    {
        this->UnmakeConnections_();
        this->ClearConnections();
    }

    void ClearConnections()
    {
        // Clears downstream connections.
        this->ClearConnections_();
    }

    // template<typename Member>
    // static void MuteMember_(Member &member, bool isMuted)
    // {
    //     if constexpr (IsAggregate<Member>)
    //     {
    //         member.SetMuted(isMuted);
    //     }
    // }

    // void SetMuted(bool isMuted)
    // {
    //     this->isMuted_ = isMuted;

    //     auto doMute = [this, isMuted](const auto &field) -> void
    //     {
    //         MuteMember_(this->*(field.member), isMuted);
    //     };

    //     jive::ForEach(Fields<Aggregate>::fields, doMute);
    // }

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
            using MemberType = typename Member::Type;

            member.Connect(
                this,
                &Aggregate::template OnMemberChanged_<MemberType>);
        }
    }

    void MakeConnections_()
    {
        auto connector = [this](const auto &field) -> void
        {
            this->Connector_(this->*(field.member));
        };

        jive::ForEach(Fields<Aggregate>::fields, connector);
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
        auto disconnector = [this](const auto &field) -> void
        {
            this->Disconnector_(this->*(field.member));
        };

        jive::ForEach(Fields<Aggregate>::fields, disconnector);
    }

    template<typename T>
    static void OnMemberChanged_(void *observer, Argument<T>)
    {
        auto self = static_cast<Aggregate *>(observer);

        if (self->isMuted_)
        {
            return;
        }

        self->Notify_(self->Get());
    }

    void OnMute_(bool isMuted)
    {
        if (!isMuted)
        {
            // Notify group observers when unmuted.
            this->Notify_(this->Get());
        }

        this->isMuted_ = isMuted;
    }

private:
    bool isMuted_;
    MuteTerminus<Aggregate> muteTerminus_;
};


} // end namespace detail


} // end namespace pex
