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
#include "pex/detail/signal_detail.h"

namespace pex
{

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
    Signal() = default;

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

    template<typename OtherObserver>
    explicit Signal(const Signal<OtherObserver> &other)
        :
        model_(other.model_)
    {
        this->model_->Connect(this, &Signal::OnModelSignaled_);
    }

    template<typename OtherObserver>
    Signal<Observer> & operator=(const Signal<OtherObserver> &other)
    {
        if (this->model_)
        {
            this->model_->Disconnect();
        }

        this->model_ = other.model_;
        this->model_->Connect(this, &Signal::OnModelSignaled_);
        return *this;
    }

    static void OnModelSignaled_(void * observer)
    {
        // The model value has changed.
        // Update our observers.
        auto self = static_cast<Signal *>(observer);
        self->Notify_();
    }

    operator bool () const
    {
        return (this->model_ != nullptr);
    }

    template<typename T>
    friend class Signal;

private:
    model::Signal * model_;
};

} // namespace interface



} // namespace pex
