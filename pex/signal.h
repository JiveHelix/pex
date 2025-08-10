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

    void TriggerMayModify()
    {
        this->NotifyMayModify_();
    }

    explicit operator DescribeSignal () const
    {
        return DescribeSignal{};
    }
};

} // namespace model


namespace control
{

template<typename Access = GetAndSetTag>
class Signal
    :
    public detail::NotifyOne<detail::SignalConnection<void>, Access>
{
public:
    using Model = ::pex::model::Signal;
    using Base = detail::NotifyOne<detail::SignalConnection<void>, Access>;

    static constexpr bool isControlSignal = true;

    using Upstream = pex::model::Signal;

    using Callable =
        typename detail::SignalConnection<void>::Callable;

    class UpstreamConnection
    {
        Model *upstream_;
        Signal *observer_;

    public:
        using FunctionPointer = void (*)(void *);

        UpstreamConnection(
            Model *upstream,
            Signal *observer,
            FunctionPointer callable)
            :
            upstream_(upstream),
            observer_(observer)
        {
            this->upstream_->ConnectOnce(observer, callable);
        }

        ~UpstreamConnection()
        {
            PEX_LOG(
                "control::Signal Disconnect: ",
                LookupPexName(this->observer_),
                " from ",
                LookupPexName(this->upstream_));

            this->upstream_->Disconnect(this->observer_);
        }
    };

    Signal(): Base{}, model_(nullptr)
    {
        PEX_NAME_UNIQUE("control::Signal");
    }

    Signal(model::Signal &model)
        :
        Base{},
        model_(&model),
        upstreamConnection_()
    {
        PEX_NAME_UNIQUE("control::Signal");
    }

    ~Signal()
    {
        PEX_CLEAR_NAME(this);
        PEX_LOG("control::Signal::~Signal : ", LookupPexName(this));
    }

    Signal(void *observer, model::Signal &model, Callable callable)
        :
        Signal(model)
    {
        this->Connect(observer, callable);
    }

    Signal(void *observer, const Signal &other, Callable callable)
        :
        Signal(other)
    {
        this->Connect(observer, callable);
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
        Base(other),
        model_(other.model_),
        upstreamConnection_()
    {
        PEX_NAME_UNIQUE("control::Signal");

        if (this->HasConnection())
        {
            this->upstreamConnection_.emplace(
                this->model_,
                this,
                &Signal::OnModelSignaled_);
        }
    }

    Signal(Signal &&other)
        :
        Base(std::move(other)),
        model_(std::move(other.model_)),
        upstreamConnection_()
    {
        other.upstreamConnection_.reset();

        PEX_NAME_UNIQUE("control::Signal");

        if (this->HasConnection())
        {
            this->upstreamConnection_.emplace(
                this->model_,
                this,
                &Signal::OnModelSignaled_);
        }
    }

    Signal & operator=(const Signal &other)
    {
        assert(&other != this);

        this->Base::operator=(other);

        // Do not copy connections to other.
        this->upstreamConnection_.reset();

        this->model_ = other.model_;

        if (this->HasConnection())
        {
            this->upstreamConnection_.emplace(
                this->model_,
                this,
                &Signal::OnModelSignaled_);
        }

        return *this;
    }

    Signal & operator=(Signal &&other)
    {
        assert(&other != this);

        this->Base::operator=(std::move(other));
        other.upstreamConnection_.reset();

        // Do not copy connections to other.
        this->upstreamConnection_.reset();

        this->model_ = std::move(other.model_);

        if (this->HasConnection())
        {
            this->upstreamConnection_.emplace(
                this->model_,
                this,
                &Signal::OnModelSignaled_);
        }

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

    void ClearConnections()
    {
        this->ClearConnections_();
        this->upstreamConnection_.reset();
    }

    void Connect(void *observer, Callable callable)
    {
        if (!this->upstreamConnection_)
        {
            this->upstreamConnection_.emplace(
                this->model_,
                this,
                &Signal::OnModelSignaled_);
        }

        this->Base::Connect(observer, callable);
    }

    void ConnectOnce(void *observer, Callable callable)
    {
        if (!this->upstreamConnection_)
        {
            this->upstreamConnection_.emplace(
                this->model_,
                this,
                &Signal::OnModelSignaled_);
        }

        this->Base::ConnectOnce(observer, callable);
    }

    void Disconnect(void *observer)
    {
        this->Base::Disconnect(observer);

        if (!this->HasConnection())
        {
            // The last connection has been disconnected.
            // Remove ourselves from the upstream.
            this->upstreamConnection_.reset();
        }
    }

    template<typename>
    friend class Signal;

    const model::Signal & GetModel_() const
    {
        assert(this->model_);

        return *this->model_;
    }

private:
    model::Signal * model_;
    std::optional<UpstreamConnection> upstreamConnection_;
};


} // namespace control


} // namespace pex
