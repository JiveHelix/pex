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

    using CountWillChangeEndpoint =
        Endpoint<Observer, typename ListControl::CountWillChange>;

    using CountWillChangeCallable = typename CountWillChangeEndpoint::Callable;

    using CountEndpoint =
        Endpoint<Observer, typename ListControl::Count>;

    using CountCallable = typename CountEndpoint::Callable;

    ListObserver()
        :
        countWillChange(),
        count()
    {

    }

    ListObserver(
        Observer *observer,
        ListControl listControl,
        CountWillChangeCallable countWillChangeCallable,
        CountCallable countCallable)
        :
        countWillChange(
            observer,
            listControl.countWillChange,
            countWillChangeCallable),
        count(
            observer,
            listControl.count,
            countCallable)
    {

    }

    ListObserver(
        Observer *observer,
        Upstream &upstream,
        CountWillChangeCallable countWillChangeCallable,
        CountCallable countCallable)

        :
        ListObserver(
            observer,
            ListControl(upstream),
            countWillChangeCallable,
            countCallable)
    {

    }

    ListObserver(ListObserver &&other)
        :
        countWillChange(std::move(other.countWillChange)),
        count(std::move(other.count))
    {

    }

    ListObserver & operator=(ListObserver &&other)
    {
        this->countWillChange = std::move(other.countWillChange);
        this->count = std::move(other.count);

        return *this;
    }

    ListObserver & Assign(Observer *observer, const ListObserver &other)
    {
        this->countWillChange.Assign(observer, other.countWillChange);
        this->count.Assign(observer, other.count);

        return *this;
    }

    CountWillChangeEndpoint countWillChange;
    CountEndpoint count;
};


} // end namespace pex
