/**
  * @file notify.h
  * 
  * @brief Provides Connect, Disconnect, and Notify_ methods for notification
  * callbacks.
  * 
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 15 Jul 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/
#pragma once

#include <vector>
#include <type_traits>
#include "jive/compare.h"
#include <optional>

namespace pex
{

namespace detail
{

template<typename Observer_, typename Callable_>
class Notify_: jive::Compare<Notify_<Observer_, Callable_>>
{
public:
    using Observer = Observer_;
    using Callable = Callable_;

    static constexpr auto IsMemberFunction =
        std::is_member_function_pointer_v<Callable>;

    Notify_(Observer * const observer, Callable callable)
        :
        observer_(observer),
        callable_(callable)
    {

    }

    /** Conversion from observer pointer for comparisons. **/
    explicit Notify_(Observer * const observer)
        :
        observer_(observer),
        callable_{}
    {

    }

    Notify_(const Notify_ &other)
        :
        observer_(other.observer_),
        callable_(other.callable_)
    {

    }

    Notify_ & operator=(const Notify_ &other)
    {
        this->observer_ = other.observer_;
        this->callable_ = other.callable_;

        return *this;
    }

    /** Compare using the memory address of observer_ **/
    template<typename Operator>
    bool Compare(const Notify_ &other) const
    {
        return Operator::Call(this->observer_, other.observer_);
    }

protected:
    Observer * observer_;
    Callable callable_;
};


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


template<typename Notify>
class NotifyOne_
{
public:
    void Connect(
        typename Notify::Observer * const observer,
        typename Notify::Callable callable)
    {
        this->notify_ = Notify(observer, callable);
    }

    /** Remove all registered callbacks for the observer. **/
    void Disconnect(typename Notify::Observer * const)
    {
        this->notify_.reset();
    }

    std::optional<Notify> notify_;
};


template<typename Notify, typename = std::void_t<>>
class NotifyOne : public NotifyOne_<Notify>
{
protected:
    void Notify_()
    {
        if (this->notify_)
        {
            (*this->notify_)();
        }
    }
};


template<typename Notify>
class NotifyOne<Notify, std::void_t<typename Notify::Type>>
    : public NotifyOne_<Notify>
{
protected:
    void Notify_(typename Notify::argumentType value)
    {
        if (this->notify_)
        {
            (*this->notify_)(value);
        }
    }
};


} // namespace detail

} // namespace pex
