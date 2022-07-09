/**
  * @file async.h
  * 
  * @brief Asynchronous communication between worker threads and wxWidgets
  * event loop.
  * 
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 22 Mar 2022
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once


#include <mutex>
#include "pex/wx/wxshim.h"
#include "pex/value.h"
#include "pex/interface.h"


namespace pex
{


namespace wx
{


template<typename T>
class Async: public wxEvtHandler
{
public:
    using Type = T;
    using ThreadSafe = model::Value<Type>;

    template<typename Observer>
    friend struct Control;

    template<typename>
    friend class pex::Reference;

    template<typename Observer>
    struct Control: public control::Value<Observer, ThreadSafe>
    {
        using Base = control::Value<Observer, ThreadSafe>;

        using Base::Base;

        // Constructed from an instance of Async, the control will always be
        // the user-side control.
        //
        // To create a worker control, explicitly call `GetWorkerControl`.
        Control(Async &async)
            :
            control::Value<Observer, ThreadSafe>(async.wxModel_)
        {

        }
    };

    Async(Argument<Type> initialValue = Type{})
        :
        mutex_(),
        wxModel_(),
        wxInternal_(this, this->wxModel_),
        ignoreWxEcho_(false),
        workerModel_(),
        workerInternal_(this, this->workerModel_),
        ignoreWorkerEcho_(false),
        value_(initialValue)
    {
        this->Bind(wxEVT_THREAD, &Async::OnWxEventLoop_, this);

        PEX_LOG("Connect");
        this->wxInternal_.Connect(&Async::OnWxChanged_);

        PEX_LOG("Connect");
        this->workerInternal_.Connect(&Async::OnWorkerChanged_);
    }

    Control<void> GetWorkerControl()
    {
        return Control<void>(this->workerModel_);
    }

    Control<void> GetWxControl()
    {
        return Control<void>(this->wxModel_);
    }

    Type Get() const
    {
        std::lock_guard<std::mutex> lock(this->mutex_);
        return this->value_;
    }

    explicit operator Type () const
    {
        std::lock_guard<std::mutex> lock(this->mutex_);
        return this->value_;
    }

    // The defaut control is for the wx event loop.
    template<typename Observer>
    explicit operator Control<Observer> ()
    {
        return Control<Observer>(this->wxModel_);
    }

    void Connect(void * observer, typename ThreadSafe::Callable callable)
    {
        this->wxModel_.Connect(observer, callable);
    }

    void Disconnect(void * observer)
    {
        this->wxModel_.Disconnect(observer);
    }

private:
    void OnWorkerChanged_(Argument<Type> value)
    {
        if (this->ignoreWorkerEcho_)
        {
            this->ignoreWorkerEcho_ = false;
            return;
        }

        {
            std::lock_guard<std::mutex> lock(this->mutex_);
            this->value_ = value;
        }

        // Queue the event for the wxWidgets event loop.
        this->QueueEvent(new wxThreadEvent());
    }

    void OnWxEventLoop_(wxThreadEvent &)
    {
        Type value;

        {
            std::lock_guard<std::mutex> lock(this->mutex_);
            value = this->value_;
        }
        
        this->ignoreWxEcho_ = true;

        this->wxModel_.Set(value);
    }

    void OnWxChanged_(Argument<Type> value)
    {
        if (this->ignoreWxEcho_)
        {
            this->ignoreWxEcho_ = false;
            return;
        }

        {
            std::lock_guard<std::mutex> lock(this->mutex_);
            this->value_ = value;
        }

        this->ignoreWorkerEcho_ = true;

        this->workerModel_.Set(value);
    }


    class ReferenceSetter: public Reference<ThreadSafe>
    {
    public:
        using Reference<ThreadSafe>::Reference;

        void SetWithoutNotify(Argument<Type> value)
        {
            this->SetWithoutNotify_(value);
        }

        void DoNotify()
        {
            this->DoNotify_();
        }
    };

    void SetWithoutNotify_(Argument<Type> value)
    {
        {
            std::lock_guard<std::mutex> lock(this->mutex_);
            this->value_ = value;
        }

        ReferenceSetter(this->workerModel_).SetWithoutNotify(value);
    }

    void DoNotify_()
    {
        // This will trigger OnWorkerChanged_, which will notify wxModel_ in
        // the wx event loop.
        ReferenceSetter(this->workerModel_).DoNotify();
    }

private:
    using Internal = pex::Terminus<Async, Control<Async>>;

    mutable std::mutex mutex_;
    ThreadSafe wxModel_;
    Internal wxInternal_;
    std::atomic_bool ignoreWxEcho_;
    ThreadSafe workerModel_;
    Internal workerInternal_;
    std::atomic_bool ignoreWorkerEcho_;
    Type value_;
};


template<typename T>
using MakeAsync = pex::MakeCustom<Async<T>>;


} // end namespace wx


} // end namespace pex
