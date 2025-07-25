#pragma once


#include "pex/endpoint.h"


namespace pex
{


template<typename Observer, typename Upstream_>
class ListObserver
{
public:
    using ListControl = typename MakeControl<Upstream_>::Control;
    using ListItem = typename ListControl::ListItem;
    using Upstream = typename MakeControl<Upstream_>::Upstream;

    using MemberAddedEndpoint =
        Endpoint<Observer, typename ListControl::MemberAdded>;

    using MemberAddedCallable = typename MemberAddedEndpoint::Callable;

    using MemberRemovedEndpoint =
        Endpoint<Observer, typename ListControl::MemberRemoved>;

    using MemberRemovedCallable = typename MemberRemovedEndpoint::Callable;

    ListObserver()
        :
        memberAdded(),
        memberWillRemove(),
        memberRemoved()
    {

    }

    ListObserver(
        Observer *observer,
        ListControl listControl,
        MemberAddedCallable memberAddedCallable,
        MemberRemovedCallable memberWillRemoveCallable,
        MemberRemovedCallable memberRemovedCallable)
        :
        memberAdded(
            observer,
            listControl.memberAdded,
            memberAddedCallable),

        memberWillRemove(
            observer,
            listControl.memberWillRemove,
            memberWillRemoveCallable),

        memberRemoved(
            observer,
            listControl.memberRemoved,
            memberRemovedCallable)
    {

    }

    ListObserver(
        Observer *observer,
        Upstream &upstream,
        MemberAddedCallable memberAddedCallable,
        MemberRemovedCallable memberWillRemoveCallable,
        MemberRemovedCallable memberRemovedCallable)

        :
        ListObserver(
            observer,
            ListControl(upstream),
            memberAddedCallable,
            memberWillRemoveCallable,
            memberRemovedCallable)
    {

    }

    ListObserver(ListObserver &&other)
        :
        memberAdded(std::move(other.memberAdded)),
        memberWillRemove(std::move(other.memberWillRemove)),
        memberRemoved(std::move(other.memberRemoved))
    {

    }

    ListObserver & operator=(ListObserver &&other)
    {
        this->memberAdded = std::move(other.memberAdded);
        this->memberWillRemove = std::move(other.memberWillRemove);
        this->memberRemoved = std::move(other.memberRemoved);

        return *this;
    }

    ListObserver & Assign(Observer *observer, const ListObserver &other)
    {

        this->memberAdded.Assign(observer, other.memberAdded);
        this->memberWillRemove.Assign(observer, other.memberWillRemove);
        this->memberRemoved.Assign(observer, other.memberRemoved);

        return *this;
    }

    MemberAddedEndpoint memberAdded;
    MemberRemovedEndpoint memberWillRemove;
    MemberRemovedEndpoint memberRemoved;
};


} // end namespace pex
