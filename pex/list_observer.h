#pragma once


#include "pex/endpoint.h"


namespace pex
{


template<typename Observer, typename Upstream_>
class ListObserver
{
public:
    using ListControl = typename PromoteControl<Upstream_>::Type;
    using ListItem = typename ListControl::ListItem;
    using Upstream = typename PromoteControl<Upstream_>::Upstream;

    using MemberAddedEndpoint =
        Endpoint<Observer, typename ListControl::MemberAdded>;

    using MemberAddedCallable = typename MemberAddedEndpoint::Callable;

    using MemberWillRemoveEndpoint =
        Endpoint<Observer, typename ListControl::MemberWillRemove>;

    using MemberWillRemoveCallable =
        typename MemberWillRemoveEndpoint::Callable;

    using MemberRemovedEndpoint =
        Endpoint<Observer, typename ListControl::MemberRemoved>;

    using MemberRemovedCallable = typename MemberRemovedEndpoint::Callable;

    using MemberWillReplaceEndpoint =
        Endpoint<Observer, typename ListControl::MemberWillReplace>;

    using MemberWillReplaceCallable =
        typename MemberWillReplaceEndpoint::Callable;

    using MemberReplacedEndpoint =
        Endpoint<Observer, typename ListControl::MemberReplaced>;

    using MemberReplacedCallable =
        typename MemberReplacedEndpoint::Callable;

    ListObserver()
        :
        memberAdded(),
        memberWillRemove(),
        memberRemoved(),
        memberWillReplace(),
        memberReplaced()
    {

    }

    ListObserver(
        Observer *observer,
        ListControl listControl,
        MemberAddedCallable memberAddedCallable,
        MemberWillRemoveCallable memberWillRemoveCallable,
        MemberRemovedCallable memberRemovedCallable,
        MemberWillReplaceCallable memberWillReplaceCallable,
        MemberReplacedCallable memberReplacedCallable)
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
            memberRemovedCallable),

        memberWillReplace(
            observer,
            listControl.memberWillReplace,
            memberWillReplaceCallable),

        memberReplaced(
            observer,
            listControl.memberReplaced,
            memberReplacedCallable)
    {

    }

    ListObserver(
        Observer *observer,
        Upstream &upstream,
        MemberAddedCallable memberAddedCallable,
        MemberWillRemoveCallable memberWillRemoveCallable,
        MemberRemovedCallable memberRemovedCallable,
        MemberWillReplaceCallable memberWillReplaceCallable,
        MemberReplacedCallable memberReplacedCallable)

        :
        ListObserver(
            observer,
            ListControl(upstream),
            memberAddedCallable,
            memberWillRemoveCallable,
            memberRemovedCallable,
            memberWillReplaceCallable,
            memberReplacedCallable)
    {

    }

    ListObserver(ListObserver &&other)
        :
        memberAdded(std::move(other.memberAdded)),
        memberWillRemove(std::move(other.memberWillRemove)),
        memberRemoved(std::move(other.memberRemoved)),
        memberWillReplace(std::move(other.memberWillReplace)),
        memberReplaced(std::move(other.memberReplaced))
    {

    }

    ListObserver & operator=(ListObserver &&other)
    {
        this->memberAdded = std::move(other.memberAdded);
        this->memberWillRemove = std::move(other.memberWillRemove);
        this->memberRemoved = std::move(other.memberRemoved);
        this->memberWillReplace = std::move(other.memberWillReplace);
        this->memberReplaced = std::move(other.memberReplaced);

        return *this;
    }

    ListObserver & Assign(Observer *observer, const ListObserver &other)
    {

        this->memberAdded.Assign(observer, other.memberAdded);
        this->memberWillRemove.Assign(observer, other.memberWillRemove);
        this->memberRemoved.Assign(observer, other.memberRemoved);
        this->memberWillReplace.Assign(observer, other.memberWillReplace);
        this->memberReplaced.Assign(observer, other.memberReplaced);

        return *this;
    }

    MemberAddedEndpoint memberAdded;
    MemberWillRemoveEndpoint memberWillRemove;
    MemberRemovedEndpoint memberRemoved;
    MemberWillReplaceEndpoint memberWillReplace;
    MemberReplacedEndpoint memberReplaced;
};


} // end namespace pex
