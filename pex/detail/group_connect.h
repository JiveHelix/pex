#pragma once


#include <fields/describe.h>
#include "pex/promote_control.h"
#include "pex/detail/ensemble.h"
#include "pex/type_tester.h"


namespace pex
{


namespace detail
{


template
<
    typename Observer,
    typename Upstream_
>
class GroupConnect
{
public:
    static constexpr bool isGroupConnect = true;

    static_assert(IsGroupNode<Upstream_>);

    template<typename U>
    using Tailor = typename PromoteControl<Upstream_>::template Tailor<U>;

    using UpstreamControl = typename PromoteControl<Upstream_>::Type;
    using Upstream = typename PromoteControl<Upstream_>::Upstream;

    using Plain = typename UpstreamControl::Plain;
    using Type = Plain;

    template<template<typename> typename T>
    using Schema = typename UpstreamControl::template GroupSchema<T>;

    using Ensemble = detail::Ensemble<Plain, Schema, Tailor>;

    using ValueConnection = detail::ValueConnection<Observer, Plain>;
    using Callable = typename ValueConnection::Callable;

    GroupConnect()
        :
        upstreamControl_(),
        ensemble_(),
        observer_(nullptr),
        valueConnection_()
    {
        PEX_NAME("GroupConnect for NULL");
        PEX_MEMBER(ensemble_);
    }

    explicit GroupConnect(const UpstreamControl &upstreamControl)
        :
        upstreamControl_(upstreamControl),
        ensemble_(this->upstreamControl_),
        observer_(nullptr),
        valueConnection_()
    {
        PEX_NAME("GroupConnect for NULL");
        PEX_MEMBER(ensemble_);
    }

    GroupConnect(
        Observer *observer,
        const UpstreamControl &upstreamControl,
        Callable callable)
        :
        upstreamControl_(upstreamControl),
        ensemble_(this->upstreamControl_),
        observer_(observer),
        valueConnection_(std::in_place_t{}, observer, callable)
    {
        PEX_NAME(
            fmt::format(
                "GroupConnect ({}) for {}",
                PromoteControl<Upstream_>::selectorName,
                pex::LookupPexName(observer)));

        PEX_MEMBER(ensemble_);

        this->ensemble_.Connect(this, &GroupConnect::OnEnsemble_);

        PEX_LINK_OBSERVER(this, observer);
    }

    explicit GroupConnect(Upstream &upstream)
        :
        GroupConnect(UpstreamControl(upstream))
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
        ensemble_(this->upstreamControl_),
        observer_(nullptr),
        valueConnection_()
    {
        PEX_NAME(
            fmt::format("GroupConnect for {}", pex::LookupPexName(observer)));

        PEX_MEMBER(ensemble_);

        if (other.valueConnection_)
        {
            this->observer_ = observer;

            this->valueConnection_.emplace(
                observer,
                other.valueConnection_->GetCallable());

            this->ensemble_.Connect(this, &GroupConnect::OnEnsemble_);

            PEX_LINK_OBSERVER(this, observer);
        }
    }

    GroupConnect & operator=(const GroupConnect &other)
    {
        return this->Assign(other.observer_, other);
    }

    GroupConnect(const GroupConnect &other)
        :
        upstreamControl_(other.upstreamControl_),
        ensemble_(this->upstreamControl_),
        observer_(nullptr),
        valueConnection_()
    {
        if (other.valueConnection_)
        {
            assert(other.observer_);
            this->observer_ = other.observer_;

            this->valueConnection_.emplace(
                this->observer_,
                other.valueConnection_->GetCallable());

            this->ensemble_.Connect(this, &GroupConnect::OnEnsemble_);

            PEX_NAME(
                fmt::format(
                    "GroupConnect for {}",
                    pex::LookupPexName(this->observer_)));

            PEX_LINK_OBSERVER(this, this->observer_);
        }
        else
        {
            PEX_NAME("GroupConnect for nullptr");
        }

        PEX_MEMBER(ensemble_);
    }

    GroupConnect(GroupConnect &&other) noexcept
        :
        upstreamControl_(std::move(other.upstreamControl_)),
        ensemble_(this->upstreamControl_),
        observer_(nullptr),
        valueConnection_()
    {
        if (other.valueConnection_)
        {
            assert(other.observer_);
            this->observer_ = other.observer_;

            PEX_NAME(
                fmt::format(
                    "GroupConnect for {}",
                    pex::LookupPexName(this->observer_)));

            PEX_MEMBER(ensemble_);
            PEX_LINK_OBSERVER(this, this->observer_);

            this->valueConnection_.emplace(
                this->observer_,
                other.valueConnection_->GetCallable());

            this->ensemble_.Connect(this, &GroupConnect::OnEnsemble_);
        }
        else
        {
            PEX_NAME("GroupConnect for nullptr");
            PEX_MEMBER(ensemble_);
        }

        other.Disconnect();
    }

    void Emplace(const UpstreamControl &upstreamControl)
    {
        this->Disconnect();

        this->upstreamControl_ = upstreamControl;
        this->ensemble_.AssignUpstream(this->upstreamControl_);
    }

    void Emplace(
        Observer *observer,
        const UpstreamControl &upstreamControl,
        Callable callable)
    {
        assert(observer != nullptr);

        this->Disconnect();

        PEX_NAME(
            fmt::format("GroupConnect for {}", pex::LookupPexName(observer)));

        this->upstreamControl_ = upstreamControl;
        this->ensemble_.AssignUpstream(this->upstreamControl_);
        this->ensemble_.Connect(this, &GroupConnect::OnEnsemble_);
        this->valueConnection_.emplace(observer, callable);
        this->observer_ = observer;

        PEX_LINK_OBSERVER(this, this->observer_);
    }

    GroupConnect & Assign(Observer *observer, const GroupConnect &other)
    {
        this->Disconnect();

        this->upstreamControl_ = other.upstreamControl_;
        this->ensemble_.AssignUpstream(this->upstreamControl_);

        PEX_NAME(
            fmt::format(
                "GroupConnect for {}",
                pex::LookupPexName(observer)));

        if (other.valueConnection_)
        {
            this->valueConnection_.emplace(
                observer,
                other.valueConnection_->GetCallable());

            this->ensemble_.Connect(this, &GroupConnect::OnEnsemble_);

            this->observer_ = observer;

            PEX_LINK_OBSERVER(this, this->observer_);
        }

        return *this;
    }

    void Connect(Observer *observer, Callable callable)
    {
        if (this->observer_)
        {
            // Already connected.
            assert(this->valueConnection_.has_value());
            assert(this->ensemble_.HasObserver(this));

        }
        else
        {
            assert(!this->ensemble_.HasObserver(this));
            this->ensemble_.Connect(this, &GroupConnect::OnEnsemble_);
        }

        this->observer_ = observer;
        this->valueConnection_.emplace(this->observer_, callable);

        PEX_LINK_OBSERVER(this, this->observer_);
    }

    void Disconnect(Observer *)
    {
        this->Disconnect();
    }

    void Disconnect()
    {
        if (!this->observer_)
        {
            assert(!this->ensemble_.HasConnection());
            assert(!this->valueConnection_.has_value());

            return;
        }

        this->ensemble_.Disconnect(this);
        this->valueConnection_.reset();
        this->observer_ = nullptr;
    }

    ~GroupConnect()
    {
        this->Disconnect();

        PEX_CLEAR_NAME(this);
        PEX_CLEAR_NAME(&this->ensemble_);
    }

    static void OnEnsemble_(void * context, const Plain &value)
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
    Ensemble ensemble_;
    Observer *observer_;
    std::optional<ValueConnection> valueConnection_;
};


} // end namespace detail


template<typename T>
concept IsGroupConnect = requires { T::isGroupConnect; };


} // end namespace pex
