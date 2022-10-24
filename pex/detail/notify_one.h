#pragma once

#include <cassert>
#include <optional>
#include "pex/access_tag.h"
#include "pex/argument.h"
#include "pex/detail/log.h"


namespace pex
{

namespace detail
{

template<typename Notify, typename Access>
class NotifyOne_
{
public:
    void Connect(
        typename Notify::Observer * const observer,
        typename Notify::Callable callable)
    {
        PEX_LOG(observer);

        static_assert(
            HasAccess<GetTag, Access>,
            "Cannot connect observer without read access.");

        this->notify_ = Notify(observer, callable);
    }

    /** Remove all registered callbacks for the observer. **/
    void Disconnect(
        [[maybe_unused]] typename Notify::Observer * const observer)
    {

#ifndef NDEBUG
        if (this->notify_)
        {
            assert((*this->notify_).GetObserver() == observer);
        }
#endif

        PEX_LOG(observer);
        this->notify_.reset();
    }

    std::optional<Notify> notify_;
};


template<typename Notify, typename Access, typename = std::void_t<>>
class NotifyOne : public NotifyOne_<Notify, Access>
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


// Selected if Notify has the member 'Type'.
// This Notify_ method takes an argument.
template<typename Notify, typename Access>
class NotifyOne<Notify, Access, std::void_t<typename Notify::Type>>
    : public NotifyOne_<Notify, Access>
{
protected:
    void Notify_(Argument<typename Notify::Type> value)
    {
        if (this->notify_)
        {
            (*this->notify_)(value);
        }
    }
};


} // namespace detail

} // namespace pex
