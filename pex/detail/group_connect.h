#pragma once


#include <fields/describe.h>
#include "pex/make_control.h"
#include "pex/detail/aggregate.h"


namespace pex
{


// Specializations of MakeControl for Groups.
template<typename P>
struct MakeControl<P, std::enable_if_t<IsGroupModel<P>>>
{
    using Control = typename P::ControlType;
    using Upstream = P;
};


template<typename P>
struct MakeControl<P, std::enable_if_t<IsGroupControl<P>>>
{
    using Control = P;
    using Upstream = typename P::Upstream;
};


namespace detail
{


template<typename Observer, typename Upstream_>
class GroupConnect
{
public:
    static constexpr bool isGroupConnect = true;

    static_assert(IsGroupNode<Upstream_>);

    using UpstreamControl = typename MakeControl<Upstream_>::Control;
    using Upstream = typename MakeControl<Upstream_>::Upstream;

    using Plain = typename UpstreamControl::Plain;
    using Type = Plain;

    template< typename T>
    using Fields = typename UpstreamControl::template Fields<T>;

    template<template<typename> typename T>
    using Template = typename UpstreamControl::template GroupTemplate<T>;

    using Aggregate = detail::Aggregate<Plain, Fields, Template>;

    using ValueConnection = detail::ValueConnection<Observer, Plain>;
    using Callable = typename ValueConnection::Callable;

    GroupConnect()
        :
        upstreamControl_(),
        aggregate_(),
        observer_(nullptr),
        valueConnection_()
    {
        REGISTER_PEX_NAME(
            this,
            "GroupConnect for NULL");

        REGISTER_PEX_PARENT(aggregate_);
    }

    GroupConnect(
        Observer *observer,
        const UpstreamControl &upstreamControl)
        :
        upstreamControl_(upstreamControl),
        aggregate_(this->upstreamControl_),
        observer_(observer),
        valueConnection_()
    {
        REGISTER_PEX_NAME(
            this,
            fmt::format("GroupConnect for {}", pex::LookupPexName(observer)));

        REGISTER_PEX_PARENT(aggregate_);
    }

    GroupConnect(
        Observer *observer,
        const UpstreamControl &upstreamControl,
        Callable callable)
        :
        upstreamControl_(upstreamControl),
        aggregate_(this->upstreamControl_),
        observer_(observer),
        valueConnection_(std::in_place_t{}, observer, callable)
    {
        REGISTER_PEX_NAME(
            this,
            fmt::format("GroupConnect for {}", pex::LookupPexName(observer)));

        REGISTER_PEX_PARENT(aggregate_);

        this->aggregate_.Connect(this, &GroupConnect::OnAggregate_);
    }

    GroupConnect(
        Observer *observer,
        Upstream &upstream)
        :
        GroupConnect(observer, UpstreamControl(upstream))
    {

    }

    GroupConnect(
        Observer *observer,
        Upstream &upstream,
        Callable callable)
        :
        GroupConnect(observer, UpstreamControl(upstream), callable)
    {

    }

    GroupConnect(Observer *observer, const GroupConnect &other)
        :
        upstreamControl_(other.upstreamControl_),
        aggregate_(this->upstreamControl_),
        observer_(observer),
        valueConnection_()
    {
        REGISTER_PEX_NAME(
            this,
            fmt::format("GroupConnect for {}", pex::LookupPexName(observer)));

        REGISTER_PEX_PARENT(aggregate_);

        if (other.valueConnection_)
        {
            this->valueConnection_.emplace(
                observer,
                other.valueConnection_->GetCallable());

            this->aggregate_.Connect(this, &GroupConnect::OnAggregate_);
        }
    }

    GroupConnect & operator=(const GroupConnect &other)
    {
        return this->Assign(other.observer_, other);
    }

    GroupConnect(const GroupConnect &other)
        :
        upstreamControl_(other.upstreamControl_),
        aggregate_(this->upstreamControl_),
        observer_(other.observer_),
        valueConnection_()
    {
        REGISTER_PEX_NAME(
            this,
            fmt::format(
                "GroupConnect for {}",
                pex::LookupPexName(this->observer_)));

        REGISTER_PEX_PARENT(aggregate_);

        if (other.valueConnection_)
        {
            this->valueConnection_.emplace(
                this->observer_,
                other.valueConnection_->GetCallable());

            this->aggregate_.Connect(this, &GroupConnect::OnAggregate_);
        }
    }

    GroupConnect(GroupConnect &&other) noexcept
        :
        upstreamControl_(std::move(other.upstreamControl_)),
        aggregate_(this->upstreamControl_),
        observer_(std::move(other.observer_)),
        valueConnection_()
    {
        REGISTER_PEX_NAME(
            this,
            fmt::format(
                "GroupConnect for {}",
                pex::LookupPexName(this->observer_)));

        REGISTER_PEX_PARENT(aggregate_);

        if (other.valueConnection_)
        {
            this->valueConnection_.emplace(
                this->observer_,
                other.valueConnection_->GetCallable());

            this->aggregate_.Connect(this, &GroupConnect::OnAggregate_);
        }
    }

    GroupConnect & Assign(Observer *observer, const GroupConnect &other)
    {
        this->aggregate_.ClearConnections();
        this->upstreamControl_ = other.upstreamControl_;

        this->aggregate_.AssignUpstream(this->upstreamControl_);

        this->observer_ = observer;

        REGISTER_PEX_NAME(
            this,
            fmt::format(
                "GroupConnect for {}",
                pex::LookupPexName(this->observer_)));

        if (other.valueConnection_)
        {
            this->valueConnection_.emplace(
                observer,
                other.valueConnection_->GetCallable());

            this->aggregate_.Connect(this, &GroupConnect::OnAggregate_);
        }

        return *this;
    }

    void Connect(Callable callable)
    {
        if (!this->observer_)
        {
            throw std::runtime_error("GroupConnect has no observer.");
        }

        this->valueConnection_.emplace(this->observer_, callable);
        this->aggregate_.Connect(this, &GroupConnect::OnAggregate_);
    }

    void Disconnect(Observer *)
    {
        this->Disconnect();
    }

    void Disconnect()
    {
        this->aggregate_.Disconnect(this);
        this->valueConnection_.reset();
    }

    ~GroupConnect()
    {
        this->aggregate_.ClearConnections();
    }

    static void OnAggregate_(void * context, const Plain &value)
    {
        auto self = static_cast<GroupConnect *>(context);
        assert(self->valueConnection_.has_value());
        (*self->valueConnection_)(value);
    }

    const UpstreamControl & GetControl() const
    {
        return this->upstreamControl_;
    }

    UpstreamControl & GetControl()
    {
        return this->upstreamControl_;
    }

    explicit operator UpstreamControl () const
    {
        return this->upstreamControl_;
    }

    Plain Get() const
    {
        return this->upstreamControl_.Get();
    }

    void Set(const Plain &plain)
    {
        this->upstreamControl_.Set(plain);
    }

private:
    UpstreamControl upstreamControl_;
    Aggregate aggregate_;
    Observer *observer_;
    std::optional<ValueConnection> valueConnection_;
};


} // end namespace detail


template<typename T>
concept IsGroupConnect = requires { T::isGroupConnect; };


} // end namespace pex
