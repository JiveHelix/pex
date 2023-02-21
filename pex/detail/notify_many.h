#pragma once

#include <algorithm>
#include <type_traits>
#include <vector>
#include <cassert>
#include "pex/access_tag.h"
#include "pex/argument.h"
#include "pex/detail/log.h"
#include <iostream>

#define USE_OBSERVER_NAME

namespace pex
{

namespace detail
{


template<typename Notify, typename Access>
class NotifyConnector_
{
public:
    using Observer = typename Notify::Observer;
    using Callable = typename Notify::Callable;

    NotifyConnector_()
    {

    }

    ~NotifyConnector_()
    {
        if (!this->connections_.empty())
        {
            std::cout << "ERROR: Active connections destroyed: ";

#ifdef USE_OBSERVER_NAME
            if constexpr (std::is_void_v<Observer>)
            {
                std::cout << "void ";
            }
            else
            {
                std::cout << Observer::observerName << " ";
            }
#endif
            std::cout << this;
            std::cout << std::endl;

            std::cout << "Was your model destroyed before your controls?"
                << std::endl;

            for (auto &connection: this->connections_)
            {
                std::cout << "  " << connection.GetObserver() << std::endl;
            }
        }

        assert(this->connections_.empty());
    }

    void Connect(Observer * const observer, Callable callable)
    {
        static_assert(
            HasAccess<GetTag, Access>,
            "Cannot connect observer without read access.");

#ifdef USE_OBSERVER_NAME
        if constexpr (std::is_void_v<Observer>)
        {
            PEX_LOG("void (", observer, ") connecting to ", this);
        }
        else
        {
            PEX_LOG(
                Observer::observerName,
                " (",
                observer,
                ") connecting to ",
                this);
        }
#endif

        auto callback = Notify(observer, callable);

        // sorted insert
        auto insertion = std::upper_bound(
            this->connections_.begin(),
            this->connections_.end(),
            callback);

#ifdef ENABLE_PEX_LOG
        if (!this->connections_.empty() && *(insertion - 1) == callback)
        {
            PEX_LOG("Info: ", observer, " is already observing.");
        }
#endif

        this->connections_.insert(insertion, callback);
    }

    size_t GetNotifierCount() const
    {
        return this->connections_.size();
    }

protected:
    void ClearConnections_()
    {
        this->connections_.clear();
    }

protected:
    std::vector<Notify> connections_;
};


template<typename Notify, typename Access>
class NotifyMany_: public NotifyConnector_<Notify, Access>
{
public:
    /** Remove all registered callbacks for the observer. **/
    void Disconnect(typename Notify::Observer * const observer)
    {
        if (this->connections_.empty())
        {
            return;
        }

        auto [first, last] = std::equal_range(
            this->connections_.begin(),
            this->connections_.end(),
            Notify(observer));

        if (first == this->connections_.end())
        {
            // This observer has no connections.
            return;
        }

        PEX_LOG(observer, " disconnecting from ", this);

        this->connections_.erase(first, last);
    }
};


/* Specialized for notify callbacks that do not accept an argument */
template<typename Notify, typename Access, typename = std::void_t<>>
class NotifyConnector: public NotifyConnector_<Notify, Access>
{
protected:
    void Notify_()
    {
        for (auto &connection: this->connections_)
        {
            connection();
        }
    }
};

template<typename Notify, typename Access, typename = std::void_t<>>
class NotifyMany : public NotifyMany_<Notify, Access>
{
protected:
    void Notify_()
    {
        for (auto &connection: this->connections_)
        {
            connection();
        }
    }
};


/* Specialized for notify callbacks that accept an argument. */
template<typename Notify, typename Access>
class NotifyConnector<Notify, Access, std::void_t<typename Notify::Type>>
    : public NotifyConnector_<Notify, Access>
{
protected:
    void Notify_(Argument<typename Notify::Type> value)
    {
        for (auto &connection: this->connections_)
        {
            connection(value);
        }
    }
};

template<typename Notify, typename Access>
class NotifyMany<Notify, Access, std::void_t<typename Notify::Type>>
    : public NotifyMany_<Notify, Access>
{
protected:
    void Notify_(Argument<typename Notify::Type> value)
    {
        for (auto &connection: this->connections_)
        {
            connection(value);
        }
    }
};

} // namespace detail

} // namespace pex
