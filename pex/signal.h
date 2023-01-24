/**
  * @file value.h
  *
  * @brief Implements model and control Signal nodes.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 22 Jul 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include <type_traits>
#include <ostream>
#include <stdexcept>
#include "pex/detail/notify_one.h"
#include "pex/detail/notify_many.h"
#include "pex/detail/signal_connection.h"
#include "pex/detail/require_has_value.h"
#include "pex/detail/log.h"

namespace pex
{


struct DescribeSignal
{

};


inline
std::ostream & operator<<(std::ostream &output, const DescribeSignal &)
{
    return output << "Signal";
}


namespace model
{

class Signal
    :
    public detail::NotifyMany<detail::SignalConnection<void>, GetAndSetTag>
{
public:
    using Callable =
        typename detail::SignalConnection<void>::Callable;

    void Trigger()
    {
        this->Notify_();
    }

    explicit operator DescribeSignal () const
    {
        return DescribeSignal{};
    }
};

} // namespace model


namespace control
{

template<typename Observer_, typename Access = GetAndSetTag>
class Signal
    :
    public detail::NotifyOne<detail::SignalConnection<Observer_>, Access>
{
public:
    using Observer = Observer_;
    using Pex = pex::model::Signal;

    using Callable =
        typename detail::SignalConnection<Observer_>::Callable;

    Signal(): model_(nullptr) {}

    Signal(model::Signal &model)
        : model_(&model)
    {
        PEX_LOG("Connect ", this);
        this->model_->Connect(this, &Signal::OnModelSignaled_);
    }

    ~Signal()
    {
        if (this->model_)
        {
            PEX_LOG("Disconnect ", this);
            this->model_->Disconnect(this);
        }
    }

    /** Signals the model node, which echoes the signal back to all of the
     ** interfaces, including this one.
     **/
    void Trigger()
    {
        REQUIRE_HAS_VALUE(this->model_);
        this->model_->Trigger();
    }

    Signal(const Signal &other)
        :
        model_(other.model_)
    {
        if (!other.model_)
        {
            throw std::logic_error("other.model_ must be set!");
        }

        PEX_LOG("Connect ", this);
        this->model_->Connect(this, &Signal::OnModelSignaled_);
    }

    template<typename OtherObserver>
    explicit Signal(const Signal<OtherObserver> &other)
        :
        model_(other.model_)
    {
        if (!other.model_)
        {
            throw std::logic_error("other.model_ must be set!");
        }

        PEX_LOG("Connect ", this);
        this->model_->Connect(this, &Signal::OnModelSignaled_);
    }

    Signal & operator=(const Signal &other)
    {
        if (!other.model_)
        {
            throw std::logic_error("other.model_ must be set!");
        }

        if (this->model_)
        {
            PEX_LOG("Disconnect ", this);
            this->model_->Disconnect(this);
        }

        this->model_ = other.model_;

        PEX_LOG("Connect ", this);
        this->model_->Connect(this, &Signal::OnModelSignaled_);

        return *this;
    }

    template<typename OtherObserver>
    Signal<Observer> & operator=(const Signal<OtherObserver> &other)
    {
        if (!other.model_)
        {
            throw std::logic_error("other.model_ must be set!");
        }

        if (this->model_)
        {
            PEX_LOG("Disconnect ", this);
            this->model_->Disconnect(this);
        }

        this->model_ = other.model_;

        PEX_LOG("Connect", this);
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
        return this->HasModel();
    }

    bool HasModel() const
    {
        return (this->model_ != nullptr);
    }

    explicit operator DescribeSignal () const
    {
        return DescribeSignal{};
    }

    template<typename U, typename V>
    friend class Signal;

private:
    model::Signal * model_;
};


} // namespace control


} // namespace pex
