#pragma once

#include <optional>
#include <utility>
#include <fields/assign.h>
#include <jive/for_each.h>
#include "pex/reference.h"
#include "pex/selectors.h"
#include "pex/detail/value_connection.h"


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


template<typename T, typename Enable = void>
struct IsAccessor_: std::false_type {};

template<typename T>
struct IsAccessor_
<
    T,
    std::enable_if_t<T::isAccessor>
>
: std::true_type {};

template<typename T>
inline constexpr bool IsAccessor = IsAccessor_<T>::value;


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
        this->MakeConnections_<Fields<Aggregate>>(*this);

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
        this->ClearConnections();
    }

    void ClearConnections()
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

    void SetMuted(bool isMuted)
    {
        this->isMuted_ = isMuted;
    }

    void Notify(const Plain &plain)
    {
        this->Notify_(plain);
    }

private:
    template<typename Member>
    void Connector_(Member &member)
    {
        if constexpr (IsAccessor<Member>)
        {
            using MemberFields = typename Member::template Fields<Member>;

            this->MakeConnections_<MemberFields>(member);
        }
        else
        {
            if constexpr (!IsSignal<typename Member::Pex>)
            {
                using MemberType = typename Member::Type;

                member.Connect(
                    &Aggregate::template OnMemberChanged_<MemberType>);
            }
        }
    }

    template<typename MemberFields, typename Object>
    void MakeConnections_(Object &object)
    {
        auto connector = [this, &object](const auto &field) -> void
        {
            this->Connector_(object.*(field.member));
        };

        jive::ForEach(MemberFields::fields, connector);
    }

    template<typename T>
    void OnMemberChanged_(Argument<T>)
    {
        // std::cout << "Aggregate::OnMemberChanged_: " << value << " ...";

        if (this->isMuted_)
        {
            // std::cout << "muted" << std::endl;
            return;
        }

        // std::cout << "notify" << std::endl;

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


class MuteGroup
{
public:
    MuteGroup()
        :
        muteControl_()
    {

    }

    MuteGroup(MuteControl muteControl)
        :
        muteControl_(muteControl)
    {

    }

    MuteGroup(const MuteGroup &other)
        :
        muteControl_(other.muteControl_)
    {

    }

    MuteGroup & operator=(const MuteGroup &other)
    {
        this->muteControl_ = other.muteControl_;
        return *this;
    }

    MuteControl CloneMuteControl()
    {
        return this->muteControl_;
    }

    bool IsMuted() const
    {
        return this->muteControl_.Get();
    }

    void DoMute()
    {
        this->muteControl_.Set(true);
    }

    void DoUnmute()
    {
        this->muteControl_.Set(false);
    }

private:
    MuteControl muteControl_;
};


template<typename GroupSubType, typename Observer>
class GroupConnect
{
public:
    static_assert(IsAccessor<GroupSubType>);

    using Plain = typename GroupSubType::Plain;

    template< typename T>
    using Fields = typename GroupSubType::template Fields<T>;

    template<template<typename> typename T>
    using Template = typename GroupSubType::template GroupTemplate<T>;

    using Aggregate = detail::Aggregate<Plain, Fields, Template>;

    using ValueConnection = detail::ValueConnection<Observer, Plain>;
    using Callable = typename ValueConnection::Callable;

    GroupConnect(const GroupConnect &) = delete;
    GroupConnect &operator=(const GroupConnect &) = delete;

    GroupConnect(GroupSubType &group, Observer *observer, Callable callable)
        :
        muteTerminus_(this, group.CloneMuteControl()),
        aggregate_(group),
        valueConnection_(observer, callable)
    {
        this->aggregate_.Connect(this, &GroupConnect::OnAggregate_);
        this->muteTerminus_.Connect(&GroupConnect::OnMute_);
    }

    ~GroupConnect()
    {
        this->aggregate_.ClearConnections();
    }

    void OnMute_(bool isMuted)
    {
        if (!isMuted)
        {
            this->valueConnection_(this->aggregate_.Get());
        }

        this->aggregate_.SetMuted(isMuted);
    }

    static void OnAggregate_(void * context, const Plain &value)
    {
        auto self = static_cast<GroupConnect *>(context);
        self->valueConnection_(value);
    }

private:
    MuteTerminus<GroupConnect> muteTerminus_;
    Aggregate aggregate_;
    ValueConnection valueConnection_;
};


template
<
    typename Object,
    typename Observer,
    typename Enable = void
>
struct CallableHelper {};


template
<
    typename Object,
    typename Observer
>
struct CallableHelper
<
    Object,
    Observer,
    std::enable_if_t<IsSignal<Object>>
>
{
    using Callable = typename detail::SignalConnection<Observer>::Callable;
};


template
<
    typename Object,
    typename Observer
>
struct CallableHelper
<
    Object,
    Observer,
    std::enable_if_t<!IsSignal<Object>>
>
{
    using Callable =
        typename detail::ValueConnection<Observer, typename Object::Type>
            ::Callable;
};


template<typename Object, typename Observer>
class ObjectConnect
{
public:
    using ObservedObject = control::ChangeObserver<Observer, Object>;

    using Callable =
        typename CallableHelper<ObservedObject, Observer>::Callable;

    ObjectConnect(const ObjectConnect &) = delete;
    ObjectConnect &operator=(const ObjectConnect &) = delete;

    ObjectConnect(Object &object, Observer *observer, Callable callable)
        :
        object_(object),
        observed_(this->object_),
        observer_(observer)
    {
        this->observed_.Connect(observer, callable);
    }

    ~ObjectConnect()
    {
        this->observed_.Disconnect(this->observer_);
    }

    Object &object_;
    ObservedObject observed_;
    Observer *observer_;
};


template<typename T, typename Enable = void>
struct Connect_ {};


template<typename T>
struct Connect_<T, std::enable_if_t<IsAccessor<T>>>
{
    template<typename GroupSubType, typename Observer>
    using Type = GroupConnect<GroupSubType, Observer>;
};


template<typename T>
struct Connect_<T, std::enable_if_t<!IsAccessor<T>>>
{
    template<typename Object, typename Observer>
    using Type = ObjectConnect<Object, Observer>;
};


template<typename Object, typename Observer>
struct Connect: public Connect_<Object>::template Type<Object, Observer>
{
    using Base = typename Connect_<Object>::template Type<Object, Observer>;

    Connect(
        Object &object,
        Observer *observer,
        typename Base::Callable callable)
        :
        Base(object, observer, callable)
    {

    }
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
    typename Plain_,
    template<typename> typename Fields_,
    template<template<typename> typename> typename Template_,
    template<typename> typename Selector,
    typename Derived
>
class Accessors
    :
    public Getter<Plain_, Fields_, Derived>
{
public:
    static constexpr bool isAccessor = true;

    using Plain = Plain_;

    template<typename T>
    using Fields = Fields_<T>;

    template<template<typename> typename T>
    using GroupTemplate = Template_<T>;

    static constexpr auto observerName = "Accessors";

protected:
    void ResetAccessors_()
    {

    }

public:
    Accessors()
        :
        isMuted_(false)
    {

    }

    Accessors(const Accessors &)
    {
        // Allows copy, but never copies the aggregate observer.
        // Re-connect is required after a copy.
        PEX_LOG("Accessors: ", this);
    }

    Accessors(Accessors &&)
    {
        PEX_LOG("Accessors: ", this);
    }

    template<typename ...Others>
    Accessors(Accessors<Others...> &&)
    {
        PEX_LOG("Accessors: ", this);
    }

#if 0
    Accessors & operator=(const Accessors &)
    {
        // Allows copy, but never copies the aggregate observer.
        // Re-connect is required after a copy.
        PEX_LOG("Accessors: ", this);
        return *this;
    }

    template<typename ...Others>
    Accessors & operator=(const Accessors<Others...> &)
    {
        // Allows copy, but never copies the aggregate observer.
        // Re-connect is required after a copy.
        PEX_LOG("Accessors: ", this);
        return *this;
    }

    Accessors & operator=(Accessors &&)
    {
        PEX_LOG("Accessors: ", this);
        return *this;
    }

    template<typename ...Others>
    Accessors & operator=(Accessors<Others...> &&)
    {
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

    void Mute()
    {
        if (this->isMuted_)
        {
            return;
        }

        this->isMuted_ = true;

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

#if 0
        // TODO: Is this still necessary?

        // Allow all members to notify before unmuting our own aggregate
        // observers.
        if (this->aggregate_)
        {
            this->aggregate_->Notify(this->Get());
            this->aggregate_->Unmute();
        }
#endif
    }

    void Set(const Plain &plain)
    {
        DeferGroup<Fields, Template_, Selector, Derived> deferGroup(
            static_cast<Derived &>(*this));

        deferGroup.Set(plain);
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
    bool isMuted_;
};


} // end namespace pex
