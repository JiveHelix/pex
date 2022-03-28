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


template<typename WorkerControl>
class Async: public wxEvtHandler
{
public:
    using Type = typename WorkerControl::Type;
    using ThreadSafe = model::Value<Type>;

    // Read-only control.
    // These values only propagate from a worker to the wxWidgets event loop.
    using WxControl = control::Value_<void, ThreadSafe, NoFilter, GetTag>;

    Async()
        :
        mutex_(),
        workerControl_(),
        threadSafe_(),
        value_()
    {
        this->Bind(wxEVT_THREAD, &Async::OnWxEventLoop_, this);
    }

    void SetWorkerControl(WorkerControl workerControl)
    {
        this->workerControl_ = workerControl;

        static_assert(HasAccess<GetTag, typename WorkerControl::Access>);

        this->workerControl_.Connect(this, &Async::OnWorker_);
    }

    WxControl GetControl()
    {
        return WxControl(this->threadSafe_);
    }

    void OnWorker_(ArgumentT<Type> value)
    {
        {
            std::lock_guard<std::mutex> lock(this->mutex_);
            this->value_ = value;
        }

        // Queue the event for the wxWidgets event loop.
        this->QueueEvent(new wxThreadEvent());
    }

    void OnWxEventLoop_(wxThreadEvent &)
    {
        {
            std::lock_guard<std::mutex> lock(this->mutex_);
            this->threadSafe_.Set(this->value_);
        }
    }
    
private:
    using Observed =
        typename control::ChangeObserver<Async, WorkerControl>::Type;

    std::mutex mutex_;
    Observed workerControl_;
    ThreadSafe threadSafe_;
    Type value_;
};


} // end namespace wx


} // end namespace pex
