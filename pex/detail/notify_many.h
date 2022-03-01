#pragma once

#include <algorithm>
#include <type_traits>
#include <vector>


namespace pex
{

namespace detail
{

template<typename Notify>
class NotifyMany_
{
public:

    void Connect(
        typename Notify::Observer * const observer,
        typename Notify::Callable callable)
    {
        auto callback = Notify(observer, callable);

        // sorted insert
        this->notifiers_.insert(
            std::upper_bound(
                this->notifiers_.begin(),
                this->notifiers_.end(),
                callback),
            callback);
    }

    /** Remove all registered callbacks for the observer. **/
    void Disconnect(typename Notify::Observer * const observer)
    {
        auto [first, last] = std::equal_range(
            this->notifiers_.begin(),
            this->notifiers_.end(),
            Notify(observer));

        this->notifiers_.erase(first, last);
    }

    size_t GetNotifierCount() const
    {
        return this->notifiers_.size();
    }

protected:
    std::vector<Notify> notifiers_;
};


template<typename Notify, typename = std::void_t<>>
class NotifyMany : public NotifyMany_<Notify>
{
protected:
    void Notify_()
    {
        for (auto &notifier: this->notifiers_)
        {
            notifier();
        }
    }
};


template<typename Notify>
class NotifyMany<Notify, std::void_t<typename Notify::Type>>
    : public NotifyMany_<Notify>
{
protected:
    void Notify_(typename Notify::argumentType value)
    {
        for (auto &notifier: this->notifiers_)
        {
            notifier(value);
        }
    }
};

} // namespace detail

} // namespace pex
