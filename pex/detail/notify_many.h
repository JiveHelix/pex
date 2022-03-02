#pragma once

#include <algorithm>
#include <type_traits>
#include <vector>
#include "pex/access_tag.h"
#include "pex/detail/argument.h"


namespace pex
{

namespace detail
{

template<typename Notify, typename Access>
class NotifyMany_
{
public:
    void Connect(
        typename Notify::Observer * const observer,
        typename Notify::Callable callable)
    {
        static_assert(
            HasAccess<GetTag, Access>,
            "Cannot connect observer without read access.");

        auto callback = Notify(observer, callable);

        // sorted insert
        this->connections_.insert(
            std::upper_bound(
                this->connections_.begin(),
                this->connections_.end(),
                callback),
            callback);
    }

    /** Remove all registered callbacks for the observer. **/
    void Disconnect(typename Notify::Observer * const observer)
    {
        auto [first, last] = std::equal_range(
            this->connections_.begin(),
            this->connections_.end(),
            Notify(observer));

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
    void Notify_(ArgumentT<typename Notify::Type> value)
    {
        for (auto &connection: this->connections_)
        {
            connection(value);
        }
    }
};

} // namespace detail

} // namespace pex
