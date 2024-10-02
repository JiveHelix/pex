#pragma once

#include <algorithm>
#include <type_traits>
#include <vector>
#include <cassert>
#include <iostream>
#include <atomic>


#ifndef NDEBUG
#include <jive/scope_flag.h>
#endif

#include "pex/access_tag.h"
#include "pex/argument.h"
#include "pex/error.h"
#include "pex/detail/log.h"


namespace pex
{


namespace detail
{


#ifndef NDEBUG
#define THROW_IF_NOTIFYING                                           \
    if (this->isNotifying_)                                          \
    {                                                                \
        throw PexError(                                              \
            "Cannot modify connections from notification callback"); \
    }                                                                \

#define REPORT_NOTIFYING                                             \
    auto isNotifying = jive::ScopedCountFlag(this->isNotifying_);

#else
#define THROW_IF_NOTIFYING
#define REPORT_NOTIFYING
#endif


template<typename ConnectionType, typename Access>
class NotifyConnector_
{
public:
    using Observer = typename ConnectionType::Observer;
    using Callable = typename ConnectionType::Callable;

    NotifyConnector_()
        :
#ifndef NDEBUG
        isNotifying_{},
#endif
        connections_{}
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

            assert(false);
        }

        assert(this->connections_.empty());
    }

    void Connect(Observer *observer, Callable callable)
    {
        static_assert(
            HasAccess<GetTag, Access>,
            "Cannot connect observer without read access.");

        THROW_IF_NOTIFYING

#ifdef USE_OBSERVER_NAME
        if constexpr (std::is_void_v<Observer>)
        {
            PEX_LOG(
                "void (",
                LookupPexName(observer),
                ") connecting to ",
                LookupPexName(this));
        }
        else
        {
            PEX_LOG(
                Observer::observerName,
                " (",
                LookupPexName(observer),
                ") connecting to ",
                LookupPexName(this));
        }
#endif

        // Callbacks will be executed in the order the connections are made.
        this->connections_.emplace_back(observer, callable);
    }

    /** Remove all registered callbacks for the observer. **/
    void Disconnect(typename ConnectionType::Observer *observer)
    {
        THROW_IF_NOTIFYING

#ifdef USE_OBSERVER_NAME
        if constexpr (std::is_void_v<Observer>)
        {
            PEX_LOG(
                "void (",
                LookupPexName(observer),
                ") disconnecting from ",
                LookupPexName(this));
        }
        else
        {
            PEX_LOG(
                Observer::observerName,
                " (",
                LookupPexName(observer),
                ") disconnecting from ",
                LookupPexName(this));
        }
#endif

        std::erase(
            this->connections_,
            ConnectionType(observer));
    }

    bool IsConnected(typename ConnectionType::Observer *observer)
    {
        return std::end(this->connections_) !=
            std::find(
                std::begin(this->connections_),
                std::end(this->connections_),
                ConnectionType(observer));
    }

    // Only make the connection if the observer is not already connected.
    void ConnectOnce(Observer *observer, Callable callable)
    {
        if (this->IsConnected(observer))
        {
            // This observer has already been added to the connections_.
            return;
        }

        this->Connect(observer, callable);
    }

    size_t GetNotifierCount() const
    {
        return this->connections_.size();
    }

    bool HasConnections() const
    {
        return !this->connections_.empty();
    }

    bool HasObserver(Observer *observer)
    {
        auto found = std::find(
            this->connections_.begin(),
            this->connections_.end(),
            ConnectionType(observer));

        return (found != this->connections_.end());
    }

protected:
    void ClearConnections_()
    {
        THROW_IF_NOTIFYING
        this->connections_.clear();
    }

protected:
#ifndef NDEBUG
    jive::CountFlag<size_t> isNotifying_;
#endif
    std::vector<ConnectionType> connections_;
};


// For callbacks without an argument (signals)
template<typename ConnectionType, typename Access, typename = std::void_t<>>
class NotifyMany: public NotifyConnector_<ConnectionType, Access>
{
protected:
    void Notify_()
    {
        REPORT_NOTIFYING

        for (auto &connection: this->connections_)
        {
            connection();
        }
    }
};


// Iterates over connections, passing the value to each callback.
template<typename ConnectionType, typename Access>
class NotifyMany
<
    ConnectionType,
    Access,
    std::void_t<typename ConnectionType::Type>
>
    : public NotifyConnector_<ConnectionType, Access>
{
public:
    using Type = typename ConnectionType::Type;

protected:
    void Notify_(Argument<typename ConnectionType::Type> value)
    {
        REPORT_NOTIFYING

        for (auto &connection: this->connections_)
        {
            connection(value);
        }
    }
};

} // namespace detail

} // namespace pex
