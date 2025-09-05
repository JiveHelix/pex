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
    static constexpr bool isSignalModel = true;

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


template<typename Upstream_, typename Access = GetAndSetTag>
class Signal
    :
    public detail::NotifyOne<detail::SignalConnection<void>, Access>
{
public:
    using Base = detail::NotifyOne<detail::SignalConnection<void>, Access>;

    static constexpr bool isSignalControl = true;
    static constexpr bool isPexCopyable = true;

    using Upstream = Upstream_;

    using Callable =
        typename detail::SignalConnection<void>::Callable;

    class UpstreamConnection
    {
        Upstream *upstream_;
        Signal *observer_;

    public:
        using FunctionPointer = void (*)(void *);

        UpstreamConnection(
            Upstream *upstream,
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

    Signal(): Base{}, upstream_(nullptr)
    {
        PEX_NAME_UNIQUE("control::Signal");
    }

    Signal(Upstream &upstream)
        :
        Base{},
        upstream_(&upstream),
        upstreamConnection_()
    {
        PEX_NAME_UNIQUE("control::Signal");
    }

    ~Signal()
    {
        PEX_CLEAR_NAME(this);
        PEX_LOG("control::Signal::~Signal : ", LookupPexName(this));
    }

    Signal(void *observer, Upstream &upstream, Callable callable)
        :
        Signal(upstream)
    {
        this->Connect(observer, callable);
    }

    Signal(void *observer, const Signal &other, Callable callable)
        :
        Signal(other)
    {
        this->Connect(observer, callable);
    }

    /** Signals the upstream node, which echoes the signal back to all of the
     ** interfaces, including this one.
     **/
    void Trigger()
    {
        REQUIRE_HAS_VALUE(this->upstream_);
        this->upstream_->Trigger();
    }

    Signal(const Signal &other)
        :
        Base(other),
        upstream_(other.upstream_),
        upstreamConnection_()
    {
        PEX_NAME_UNIQUE("control::Signal");

        if (this->HasConnection())
        {
            this->upstreamConnection_.emplace(
                this->upstream_,
                this,
                &Signal::OnModelSignaled_);
        }
    }

    Signal(Signal &&other)
        :
        Base(std::move(other)),
        upstream_(std::move(other.upstream_)),
        upstreamConnection_()
    {
        other.upstreamConnection_.reset();

        PEX_NAME_UNIQUE("control::Signal");

        if (this->HasConnection())
        {
            this->upstreamConnection_.emplace(
                this->upstream_,
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

        this->upstream_ = other.upstream_;

        if (this->HasConnection())
        {
            this->upstreamConnection_.emplace(
                this->upstream_,
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

        this->upstream_ = std::move(other.upstream_);

        if (this->HasConnection())
        {
            this->upstreamConnection_.emplace(
                this->upstream_,
                this,
                &Signal::OnModelSignaled_);
        }

        return *this;
    }

    static void OnModelSignaled_(void * observer)
    {
        // The upstream value has changed.
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
        return (this->upstream_ != nullptr);
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
                this->upstream_,
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
                this->upstream_,
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

    template<typename, typename>
    friend class Signal;

protected:
    void ChangeUpstream_(Upstream &upstream)
    {
        this->upstream_ = &upstream;

        if (this->HasConnection())
        {
            this->upstreamConnection_.emplace(
                this->upstream_,
                this,
                &Signal::OnModelSignaled_);
        }
    }

private:
    Upstream *upstream_;
    std::optional<UpstreamConnection> upstreamConnection_;
};


using DefaultSignal = Signal<::pex::model::Signal>;


class SignalMux: public Signal<::pex::model::Signal>
{
public:
    using Base = Signal<::pex::model::Signal>;
    using Upstream = typename Base::Upstream;
    static constexpr bool isPexCopyable = false;

    SignalMux(const SignalMux &) = delete;
    SignalMux(SignalMux &&) = delete;

    SignalMux & operator=(const SignalMux &) = delete;
    SignalMux & operator=(SignalMux &&) = delete;

    using Base::Base;

    void ChangeUpstream(Upstream &upstream)
    {
        this->ChangeUpstream_(upstream);
    }
};


} // namespace control


} // namespace pex
