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


#include "pex/wx/wxshim.h"
#include "pex/value.h"


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
    using Control = control::Value<Observer, ThreadSafe>;

    Async(ArgumentT<Type> initialValue = Type{})
        :
        mutex_(),
        wxModel_(),
        wxInternal_(this->wxModel_),
        ignoreWxEcho_(false),
        workerModel_(),
        workerInternal_(this->workerModel_),
        ignoreWorkerEcho_(false),
        value_(initialValue)
    {
        this->Bind(wxEVT_THREAD, &Async::OnWxEventLoop_, this);
        this->wxInternal_.Connect(this, &Async::OnWxChanged_);
        this->workerInternal_.Connect(this, &Async::OnWorkerChanged_);
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

private:
    void OnWorkerChanged_(ArgumentT<Type> value)
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

    void OnWxChanged_(ArgumentT<Type> value)
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
    
private:
    mutable std::mutex mutex_;
    ThreadSafe wxModel_;
    Control<Async> wxInternal_;
    std::atomic_bool ignoreWxEcho_;
    ThreadSafe workerModel_;
    Control<Async> workerInternal_;
    std::atomic_bool ignoreWorkerEcho_;
    Type value_;
};


} // end namespace wx


} // end namespace pex
