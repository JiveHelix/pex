#pragma once

#include <algorithm>
#include <type_traits>
#include <vector>
#include "pex/access_tag.h"
#include "pex/detail/argument.h"
#include "pex/detail/log.h"


namespace pex
{

namespace detail
{

template<typename Notify, typename Access>
class NotifyMany_
{
public:
    ~NotifyMany_()
    {

    }

    void Connect(
        typename Notify::Observer * const observer,
        typename Notify::Callable callable)
    {
        static_assert(
            HasAccess<GetTag, Access>,
            "Cannot connect observer without read access.");

        PEX_LOG(observer, " connecting to ", this);

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

#ifdef ENABLE_PEX_LOG
        for (const auto &connection: this->connections_)
        {
            std::cout << connection.GetObserver() << " is observing " << this
                << std::endl;
        }
#endif

    }

    /** Remove all registered callbacks for the observer. **/
    void Disconnect(typename Notify::Observer * const observer)
    {
        PEX_LOG(observer, " disconnecting from ", this);

        auto [first, last] = std::equal_range(
            this->connections_.begin(),
            this->connections_.end(),
            Notify(observer));

        if (first == this->connections_.end())
        {
            // This observer has no connections.
            return;
        }

        this->connections_.erase(first, last);
    }

    size_t GetNotifierCount() const
    {
        return this->connections_.size();
    }

protected:
    std::vector<Notify> connections_;
};


/* Specialized for notify callbacks that do not accept an argument */
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
