/**
  * @file value.h
  * 
  * @brief Implements model and interface Signal nodes.
  * 
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 22 Jul 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/
#pragma once
#include <type_traits>
#include "pex/notify.h"

namespace pex
{

namespace detail
{

template<typename Observer>
using UnboundSignalCallable = void (*)(Observer * const observer);

template<typename Observer>
using BoundSignalCallable = void (Observer::*)();

// Use bound notification methods for all Observers except void.
template<typename Observer, typename = void>
struct CallableStyle;

template<typename Observer>
struct CallableStyle<Observer, std::enable_if_t<std::is_void_v<Observer>>>
{
    using type = detail::UnboundSignalCallable<Observer>;
};

template<typename Observer>
struct CallableStyle<Observer, std::enable_if_t<!std::is_void_v<Observer>>>
{
    using type = detail::BoundSignalCallable<Observer>;
};

template<typename Observer>
using SignalCallable = typename CallableStyle<Observer>::type;


template<typename Observer>
class SignalNotify: public Notify_<Observer, SignalCallable<Observer>>
{
public:
    using Base = Notify_<Observer, SignalCallable<Observer>>;
    using Base::Base;

    void operator()()
    {
        if constexpr(Base::IsMemberFunction)
        {
            static_assert(
                !std::is_same_v<Observer, void>,
                "Cannot call member function on void type.");

            (this->observer_->*(this->callable_))();
        }
        else
        {
            this->callable_(this->observer_);
        }
    }
};

} // namespace detail

namespace model
{

class Signal
    :
    public detail::NotifyMany<detail::SignalNotify<void>>
{
public:
    void Trigger()
    {
        this->Notify_();
    }
};

} // namespace model


namespace interface
{

template<typename Observer>
class Signal
    :
    public detail::NotifyOne<detail::SignalNotify<Observer>>
{
public:
    Signal(model::Signal * const model)
        : model_(model)
    {
        this->model_->Connect(this, &Signal::OnModelSignaled_);
    }

    ~Signal()
    {
        this->model_->Disconnect(this);
    }

    /** Signals the model node, which echoes the signal back to all of the
     ** interfaces, including this one.
     **/
    void Trigger()
    {
        this->model_->Trigger();
    }

    Signal(const Signal &other)
        :
        model_(other.model_)
    {
        this->model_->Connect(this, &Signal::OnModelSignaled_);
    }

    /**
     ** Assignment could cause an interface::Signal to track a different model
     ** value. By design, an interface value tracks the same model value for
     ** the duration of its lifetime.
     **/
    Signal & operator=(const Signal &) = delete;
    Signal & operator=(Signal &&) = delete;

    static void OnModelSignaled_(void * observer)
    {
        // The model value has changed.
        // Update our observers.
        auto self = static_cast<Signal *>(observer);
        self->Notify_();
    }

private:
    model::Signal * model_;
};

} // namespace interface



} // namespace pex
