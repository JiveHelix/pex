#pragma once

#include <algorithm>
#include <type_traits>
#include <vector>
#include <cassert>
#include <iostream>
#include <atomic>

#include <jive/create_exception.h>

#ifndef NDEBUG
#include <jive/scope_flag.h>
#endif

#include "pex/access_tag.h"
#include "pex/argument.h"
#include "pex/detail/log.h"


namespace pex
{


CREATE_EXCEPTION(PexError, std::runtime_error);


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
        }

        assert(this->connections_.empty());
    }

    void Connect(Observer * const observer, Callable callable)
    {
        static_assert(
            HasAccess<GetTag, Access>,
            "Cannot connect observer without read access.");

        THROW_IF_NOTIFYING

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

        auto callback = ConnectionType(observer, callable);

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

    /** Remove all registered callbacks for the observer. **/
    void Disconnect(typename ConnectionType::Observer * const observer)
    {
        THROW_IF_NOTIFYING

        if (this->connections_.empty())
        {
            return;
        }

        auto [first, last] = std::equal_range(
            this->connections_.begin(),
            this->connections_.end(),
            ConnectionType(observer));

        if (first == this->connections_.end())
        {
            // This observer has no connections.
            return;
        }

        PEX_LOG(observer, " disconnecting from ", this);

        this->connections_.erase(first, last);
    }

    size_t GetNotifierCount() const
    {
        return this->connections_.size();
    }

    bool HasConnections() const
    {
        return !this->connections_.empty();
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
