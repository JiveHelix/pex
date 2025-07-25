#pragma once

#include <algorithm>
#include <type_traits>
#include <vector>
#include <cassert>
#include <iostream>
#include <atomic>


#ifndef NDEBUG
#include <jive/scope_flag.h>
#include <fields/describe.h>
#endif

#include "pex/access_tag.h"
#include "pex/argument.h"
#include "pex/error.h"
#include "pex/detail/log.h"
#include "pex/detail/observer_name.h"


#ifndef NDEBUG
#include <pex/detail/logs_observers.h>
#endif


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
class NotifyMany_
#ifndef NDEBUG
    :
    public LogsObservers
#endif
{
public:
    using Observer = typename ConnectionType::Observer;
    using Callable = typename ConnectionType::Callable;

    NotifyMany_()
        :
#ifndef NDEBUG
        LogsObservers{},
        isNotifying_{},
#endif
        connections_{}
    {

    }

    ~NotifyMany_()
    {
        if (!this->connections_.empty())
        {
            std::cout << "WARNING: Active connections destroyed: ";

            std::cout << (ObserverName<Observer>) << " "
                << LookupPexName(this) << std::endl;

            std::cout << "Was your model destroyed before your controls?"
                << std::endl;

#ifndef NDEBUG
            static_assert(std::derived_from<NotifyMany_, LogsObservers>);
            this->PrintObservers(1);
#endif

            assert(false);
        }

        assert(this->connections_.empty());

        UNREGISTER_PEX_NAME(this);
    }

    NotifyMany_(const NotifyMany_ &other) = default;
    NotifyMany_(NotifyMany_ &&) noexcept = default;
    NotifyMany_ & operator=(const NotifyMany_ &other) = default;

    template<typename T>
    void Connect(T *observer, Callable callable)
    {
        static_assert(
            HasAccess<GetTag, Access>,
            "Cannot connect observer without read access.");

        THROW_IF_NOTIFYING

#ifdef ENABLE_REGISTER_NAME
        if (!HasPexName(observer))
        {
            throw std::runtime_error("All observers must be labeled");
        }

        if (!HasPexName(this))
        {
            throw std::runtime_error("All nodes must be labeled");
        }
#endif

        PEX_LOG(
            ObserverName<Observer>,
            " (",
            LookupPexName(observer),
            ") connecting to ",
            LookupPexName(this));

#ifndef NDEBUG
        this->RegisterObserver(observer);
#endif

        // Callbacks will be executed in the order the connections are made.
        this->connections_.emplace_back(observer, callable);
    }

    /** Remove all registered callbacks for the observer. **/
    void Disconnect(Observer *observer)
    {
        THROW_IF_NOTIFYING

        PEX_LOG(
            ObserverName<Observer>,
            " (",
            LookupPexName(observer),
            ") disconnecting from ",
            LookupPexName(this));

#ifndef NDEBUG
        auto found = std::find(
            std::begin(this->connections_),
            std::end(this->connections_),
            ConnectionType(observer));

        if (found == std::end(this->connections_))
        {
            throw std::logic_error(
                "Attempted disconnection from wrong model");
        }
#endif

        std::erase(
            this->connections_,
            ConnectionType(observer));

#ifndef NDEBUG
        this->RemoveObserver(observer);

        for (auto &connection: this->connections_)
        {
            if (connection.GetObserver() == observer)
            {
                throw std::logic_error(
                    "Expected all references to Observer to have been removed");
            }
        }
#endif
    }

    size_t GetNotificationOrder(Observer *observer)
    {
        if (!this->HasObserver(observer))
        {
            throw std::logic_error("Observer not found");
        }

        auto found = std::find(
            this->connections_.begin(),
            this->connections_.end(),
            ConnectionType(observer));

        auto result = std::distance(this->connections_.begin(), found);

        assert(result >= 0);

        return static_cast<size_t>(result);
    }

    // Only make the connection if the observer is not already connected.
    template<typename T>
    void ConnectOnce(T *observer, Callable callable)
    {
        if (this->HasObserver(observer))
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
class NotifyMany: public NotifyMany_<ConnectionType, Access>
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
    : public NotifyMany_<ConnectionType, Access>
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
