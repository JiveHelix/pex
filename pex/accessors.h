#pragma once

#include <optional>
#include <utility>
#include <fields/assign.h>
#include "pex/reference.h"
#include "pex/selectors.h"


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


// Signals do not have an underlying type, so they are not part of the
// conversion to a plain-old data structure.
template<typename Pex, typename Enable = void>
struct ConvertsToPlain_: std::true_type {};

// Detects model and control signals.
template<typename Pex>
struct ConvertsToPlain_
<
    Pex,
    std::enable_if_t<IsSignal<Pex>>
>: std::false_type {};

// Detects a Terminus signal.
template<typename Pex>
struct ConvertsToPlain_
<
    Pex,
    std::enable_if_t<IsSignal<typename Pex::Pex>>
>: std::false_type {};

template<typename Pex>
struct ConvertsToPlain_
<
    Pex,
    std::enable_if_t<std::is_same_v<DescribeSignal, Pex>>
>: std::false_type {};

template<typename Pex>
inline constexpr bool ConvertsToPlain = ConvertsToPlain_<Pex>::value;


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



template<typename Target, typename Source>
std::enable_if_t<!IsSignal<Target>>
SetWithoutNotify(Target &target, const Source &source)
{
    detail::AccessReference<Target>(target).SetWithoutNotify(source);
}

template<typename Target, typename Source>
std::enable_if_t<IsSignal<Target>>
SetWithoutNotify(Target &, const Source &)
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
        // using PlainType = std::remove_reference_t<
        //     decltype(target.*(plainField.member))>;

        // using SourceType = std::remove_reference_t<
        //     decltype(source.*(sourceField.member))>;

        AssignSourceToTarget(
            target.*(plainField.member),
            source.*(sourceField.member));
                // static_cast<PlainType>(source.*(sourceField.member));
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


template<template<typename> typename Fields, typename T>
bool HasModel(const T &group)
{
    bool result = true;

    auto modelChecker = [&group, &result](auto field)
    {
        if (result)
        {
            result = (group.*(field.member)).HasModel();
        }
    };

    jive::ForEach(Fields<T>::fields, modelChecker);

    return result;
}


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


namespace detail
{


// Internal helper to allow observation of aggregate types.
template
<
    typename Plain,
    template<typename> typename Fields,
    template<template<typename> typename> typename Template
>
struct Aggregate:
    // Create Terminus members. Connect happens in MakeConnections_, Disconnect
    // is called by Terminus on destruction.
    public Template
    <
        pex::TerminusSelector
        <
            Aggregate<Plain, Fields, Template>
        >::template Type
    >,
    public Getter<Plain, Fields, Aggregate<Plain, Fields, Template>>,
    public detail::NotifyMany
    <
        ::pex::ValueConnection<void, Plain, NoFilter>,
        GetAndSetTag
    >
{
public:
    static constexpr auto observerName = "Aggregate";

    using Connector = detail::NotifyMany
        <
            ::pex::ValueConnection<void, Plain, NoFilter>,
            GetAndSetTag
        >;

    template<typename Upstream>
    Aggregate(Upstream &upstream)
        :
        isMuted_(false)
    {
        InitializeTerminus<Fields>(this, *this, upstream);
        this->MakeConnections_();

#ifdef ENABLE_PEX_LOG
        auto logger = [this](const auto &field) -> void
        {
            PEX_LOG(
                "Aggregate.",
                field.name,
                ": ",
                &(this->*(field.member)));
        };

        jive::ForEach(Fields<Aggregate>::fields, logger);
#endif

    }

    Aggregate(const Aggregate &) = delete;
    Aggregate(Aggregate &&) = delete;
    Aggregate & operator=(const Aggregate &) = delete;
    Aggregate & operator=(Aggregate &&) = delete;

    ~Aggregate()
    {
        this->ClearConnections_();
        assert(this->connections_.empty());
    }

    using Callable = detail::ValueFunctionStyle<void, Plain>;

    void Connect(void * observer, Callable callable)
    {
        PEX_LOG("Connect ", observer, " to ", this);
        Connector::Connect(observer, callable);
    }

    void Mute()
    {
        this->isMuted_ = true;
    }

    void Unmute()
    {
        this->isMuted_ = false;
    }

    void Notify(const Plain &plain)
    {
        this->Notify_(plain);
    }

private:
    void MakeConnections_()
    {
        auto connector = [this](const auto &field) -> void
        {
            using MemberPex = typename std::remove_reference_t<
                decltype(this->*(field.member))>::Pex;

            if constexpr (!IsSignal<MemberPex>)
            {
                // Aggregate observers ignore signals.
                // Notifications are only forwarded when there is data.
                using MemberType = typename std::remove_reference_t<
                    decltype(this->*(field.member))>::Type;

                PEX_LOG(
                    "Connect ",
                    this,
                    " to ",
                    field.name,
                    " (",
                    &(this->*(field.member)),
                    ")");

                (this->*(field.member)).Connect(
                    &Aggregate::template OnMemberChanged_<MemberType>);
            }
        };

        jive::ForEach(Fields<Aggregate>::fields, connector);
    }

    template<typename T>
    void OnMemberChanged_(Argument<T>)
    {
        if (this->isMuted_)
        {
            return;
        }

        this->Notify_(this->Get());
    }

private:
    bool isMuted_;
};


} // end namespace detail


using MuteModel = typename model::Value<bool>;
using MuteControl = typename control::Value<void, MuteModel>;

template<typename Observer>
using MuteTerminus = Terminus<Observer, MuteModel>;


class MuteOwner
{
public:
    MuteOwner()
        :
        mute_(false)
    {

    }

    MuteOwner(const MuteOwner &) = delete;
    MuteOwner(MuteOwner &&) = delete;
    MuteOwner & operator=(const MuteOwner &) = delete;
    MuteOwner & operator=(MuteOwner &&) = delete;

    MuteControl GetMuteControl()
    {
        return MuteControl(this->mute_);
    }

private:
    MuteModel mute_;
};


template<typename Derived>
class MuteGroup
{
public:
    static constexpr auto observerName = "MuteGroup";

    template<typename Other>
    friend class MuteGroup;

    using MuteGroupTerminus = std::optional<MuteTerminus<MuteGroup<Derived>>>;

    MuteGroup()
        :
        muteTerminus_()
    {

    }

    MuteGroup(MuteControl muteControl)
        :
        muteTerminus_(std::in_place_t{}, this, muteControl)
    {
        this->muteTerminus_->Connect(&MuteGroup<Derived>::OnMute_);
    }

    template<typename Other>
    MuteGroup(const MuteGroup<Other> &other)
        :
        muteTerminus_()
    {
        if (other.muteTerminus_)
        {
            this->muteTerminus_.emplace();
            this->muteTerminus_->Assign(this, *other.muteTerminus_);
            this->muteTerminus_->Connect(&MuteGroup<Derived>::OnMute_);
        }
    }

    MuteGroup(const MuteGroup &other)
        :
        muteTerminus_()
    {
        if (other.muteTerminus_)
        {
            this->muteTerminus_.emplace();
            this->muteTerminus_->Assign(this, *other.muteTerminus_);
            this->muteTerminus_->Connect(&MuteGroup<Derived>::OnMute_);
        }
    }

    MuteGroup & operator=(const MuteGroup &other)
    {
        if (other.muteTerminus_)
        {
            this->muteTerminus_.emplace();
            this->muteTerminus_->Assign(this, *other.muteTerminus_);
            this->muteTerminus_->Connect(&MuteGroup<Derived>::OnMute_);
        }

        return *this;
    }

    bool IsMuted() const
    {
        if (!this->muteTerminus_)
        {
            throw std::logic_error("Uninitialized");
        }

        return this->muteTerminus_->Get();
    }

    void DoMute()
    {
        if (!this->muteTerminus_)
        {
            throw std::logic_error("Uninitialized");
        }

        this->muteTerminus_->Set(true);
    }

    void DoUnmute()
    {
        if (!this->muteTerminus_)
        {
            throw std::logic_error("Uninitialized");
        }

        this->muteTerminus_->Set(false);
    }

private:
    void OnMute_(bool isMuted)
    {
        if (isMuted)
        {
            static_cast<Derived *>(this)->Mute();
        }
        else
        {
            static_cast<Derived *>(this)->Unmute();
        }
    }

private:
    MuteGroupTerminus muteTerminus_;
};



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
    typename Plain,
    template<typename> typename Fields,
    template<template<typename> typename> typename Template,
    template<typename> typename Selector,
    typename Derived,
    typename Connector
>
class Accessors
    :
    public Getter<Plain, Fields, Derived>,
    public Connector
{
public:
    static constexpr auto observerName = "Accessors";

private:
    using Aggregate = detail::Aggregate<Plain, Fields, Template>;

    std::unique_ptr<Aggregate> aggregate_;

protected:
    void ResetAccessors_()
    {
        this->aggregate_.reset();
    }

public:
    Accessors()
        :
        aggregate_(nullptr),
        isMuted_(false)
    {

    }

    Accessors(const Accessors &)
        :
        aggregate_(nullptr)
    {
        // Allows copy, but never copies the aggregate observer.
        // Re-connect is required after a copy.
        PEX_LOG("Accessors: ", this);
    }

    Accessors(Accessors &&)
        :
        aggregate_(nullptr)
    {
        PEX_LOG("Accessors: ", this);
    }

    template<typename ...Others>
    Accessors(Accessors<Others...> &&)
        :
        aggregate_(nullptr)
    {
        PEX_LOG("Accessors: ", this);
    }

#if 0
    Accessors & operator=(const Accessors &)
    {
        // Allows copy, but never copies the aggregate observer.
        // Re-connect is required after a copy.
        this->aggregate_.reset();
        PEX_LOG("Accessors: ", this);
        return *this;
    }

    template<typename ...Others>
    Accessors & operator=(const Accessors<Others...> &)
    {
        // Allows copy, but never copies the aggregate observer.
        // Re-connect is required after a copy.
        this->aggregate_.reset();
        PEX_LOG("Accessors: ", this);
        return *this;
    }

    Accessors & operator=(Accessors &&)
    {
        this->aggregate_.reset();
        PEX_LOG("Accessors: ", this);
        return *this;
    }

    template<typename ...Others>
    Accessors & operator=(Accessors<Others...> &&)
    {
        this->aggregate_.reset();
        PEX_LOG("Accessors: ", this);
        return *this;
    }
#endif

    ~Accessors()
    {
#ifdef ENABLE_PEX_LOG
        if (!this->connections_.empty())
        {
            for (auto &connection: this->connections_)
            {
                PEX_LOG(
                    "Warning: ",
                    connection.GetObserver(),
                    " is still connected to Accessors ",
                    this);
            }
        }
#endif
    }

    using Callable = typename Connector::Callable;
    using Observer = typename Connector::Observer;

    void Mute()
    {
        if (this->isMuted_)
        {
            return;
        }

        this->isMuted_ = true;

        if (this->aggregate_)
        {
            // Suppress multiple repeated notifications from aggregate
            // observers.
            this->aggregate_->Mute();
        }

        auto derived = static_cast<Derived *>(this);
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
        if (!this->isMuted_)
        {
            return;
        }

        this->isMuted_ = false;

        auto derived = static_cast<Derived *>(this);
        derived->DoUnmute();

        // Iterate over members, muting those that support it.
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

        // Allow all members to notify before unmuting our own aggregate
        // observers.
        if (this->aggregate_)
        {
            this->aggregate_->Notify(this->Get());
            this->aggregate_->Unmute();
        }
    }

    void Set(const Plain &plain)
    {
        this->Mute();

        {
            DeferGroup<Fields, Template, Selector> deferGroup(
                static_cast<Derived &>(*this));

            deferGroup.Set(plain);
        }

        this->Unmute();
    }

    template<typename>
    friend class Reference;

protected:
    void Connect_(Observer *observer, Callable callable)
    {
        if (!this->aggregate_)
        {
            auto derived = static_cast<Derived *>(this);

            this->aggregate_ =
                std::make_unique<Aggregate>(*derived);

            this->aggregate_->Connect(derived, &Accessors::OnAggregate_);
        }

        if constexpr (std::is_void_v<Observer>)
        {
            PEX_LOG(
                "Connect void (",
                observer,
                ") to ",
                this);
        }
        else
        {
            PEX_LOG(
                "Connect ",
                Observer::observerName,
                " (",
                observer,
                ") to ",
                this);
        }

        Connector::Connect(observer, callable);
    }

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
            using Member = typename std::remove_reference_t<
                decltype(derived->*(thisField.member))>;

            detail::AccessReference<Member>(
                (derived->*(thisField.member)))
                    .DoNotify();
        };

        jive::ForEach(
            Fields<Derived>::fields,
            doNotify);
    }

private:
    static void OnAggregate_(void * context, const Plain &value)
    {
        auto derived = static_cast<Derived *>(context);
        derived->Notify_(value);
    }

private:
    bool isMuted_;
};


} // end namespace pex
