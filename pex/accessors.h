#pragma once

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
inline constexpr bool ConvertsToPlain = ConvertsToPlain_<Pex>::value;


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
        using PlainType = std::remove_reference_t<
            decltype(target.*(plainField.member))>;

        using SourceType = std::remove_reference_t<
            decltype(source.*(sourceField.member))>;

        if constexpr (ConvertsToPlain<SourceType>)
        {
            target.*(plainField.member) =
                PlainType(source.*(sourceField.member));
        }
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
        using MemberType = typename std::remove_reference_t
            <
                decltype(terminus.*(terminusField.member))
            >;

        (terminus.*(terminusField.member)).Assign(
            observer,
            MemberType(observer, upstream.*(upstreamField.member)));
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
    public Template
    <
        pex::TerminusSelector
        <
            Aggregate<Plain, Fields, Template>
        >::template Type
    >,
    public Getter<Plain, Fields, Aggregate<Plain, Fields, Template>>,
    public detail::NotifyConnector
    <
        ::pex::ValueConnection<void, Plain, NoFilter>,
        GetAndSetTag
    >
{
public:
    using Connector = detail::NotifyConnector
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
            using MemberType = typename std::remove_reference_t<
                decltype(this->*(field.member))>::Type;

            PEX_LOG("Connect ", this, " to ", &(this->*(field.member)));

            (this->*(field.member)).Connect(
                &Aggregate::template OnMemberChanged_<MemberType>);
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
private:
    using Aggregate = detail::Aggregate<Plain, Fields, Template>;

    std::unique_ptr<Aggregate> aggregate_;
protected:
    void ResetAccessors_()
    {
        this->aggregate_.reset();
    }

public:
    Accessors() = default;

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

    void Set(const Plain &plain)
    {
        if (this->aggregate_)
        {
            // Suppress multiple repeated notifications from aggregate
            // observers.
            this->aggregate_->Mute();
        }

        {
            DeferGroup<Plain, Fields, Template, Selector> deferGroup(
                static_cast<Derived &>(*this));

            deferGroup.Set(plain);
        }

        if (this->aggregate_)
        {
            this->aggregate_->Unmute();
            this->aggregate_->Notify(plain);
        }
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

        PEX_LOG("Connect ", observer, " to ", this);
        Connector::Connect(observer, callable);
    }

    void SetWithoutNotify_(const Plain &plain)
    {
        auto derived = static_cast<Derived *>(this);

        auto setWithoutNotify = [derived, &plain]
            (auto thisField, auto plainField)
        {
            using Member = typename std::remove_reference_t<
                decltype(derived->*(thisField.member))>;

            detail::AccessReference<Member>(
                derived->*(thisField.member))
                    .SetWithoutNotify(plain.*(plainField.member));
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
};


} // end namespace pex
