#pragma once


#include <fields/describe.h>
#include "pex/detail/aggregate.h"


namespace pex
{


namespace detail
{


template<typename Observer, typename GroupControl>
class GroupConnect
{
public:
    static_assert(IsAccessor<GroupControl>);

    using Plain = typename GroupControl::Plain;

    template< typename T>
    using Fields = typename GroupControl::template Fields<T>;

    template<template<typename> typename T>
    using Template = typename GroupControl::template GroupTemplate<T>;

    using Aggregate = detail::Aggregate<Plain, Fields, Template>;

    using ValueConnection = detail::ValueConnection<Observer, Plain>;
    using Callable = typename ValueConnection::Callable;

    GroupConnect()
        :
        groupControl_(),
        aggregate_(),
        observer_(nullptr),
        valueConnection_()
    {

    }

    GroupConnect(
        Observer *observer,
        const GroupControl &groupControl)
        :
        groupControl_(groupControl),
        aggregate_(this->groupControl_),
        observer_(observer),
        valueConnection_()
    {

    }

    GroupConnect(
        Observer *observer,
        const GroupControl &groupControl,
        Callable callable)
        :
        groupControl_(groupControl),
        aggregate_(this->groupControl_),
        observer_(observer),
        valueConnection_(std::in_place_t{}, observer, callable)
    {
        this->aggregate_.Connect(this, &GroupConnect::OnAggregate_);
    }

    GroupConnect(Observer *observer, const GroupConnect &other)
        :
        groupControl_(other.groupControl),
        aggregate_(this->groupControl_),
        observer_(other.observer_),
        valueConnection_()
    {
        if (other.valueConnection_)
        {
            this->valueConnection_.emplace(
                observer,
                other.valueConnection_->GetCallable());

            this->aggregate_.Connect(this, &GroupConnect::OnAggregate_);
        }
    }

    GroupConnect &operator=(const GroupConnect &) = delete;
    GroupConnect(const GroupConnect &) = delete;

    GroupConnect & Assign(Observer *observer, const GroupConnect &other)
    {
        this->aggregate_.ClearConnections();
        this->groupControl_ = other.groupControl_;

        this->aggregate_.AssignUpstream(this->groupControl_);

        this->observer_ = observer;

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

    const GroupControl & GetUpstream() const
    {
        return this->groupControl_;
    }

    explicit operator GroupControl () const
    {
        return this->groupControl_;
    }

private:
    GroupControl groupControl_;
    Aggregate aggregate_;
    Observer *observer_;
    std::optional<ValueConnection> valueConnection_;
};


} // end namespace detail


} // end namespace pex
