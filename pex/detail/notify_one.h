#pragma once

#include <optional>
#include "pex/access_tag.h"
#include "pex/detail/argument.h"
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
    void Disconnect()
    {

#ifdef ENABLE_PEX_LOG
        if (this->notify_)
        {
            PEX_LOG(this->notify_.GetObserver());
        }
#endif

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
    void Notify_(ArgumentT<typename Notify::Type> value)
    {
        if (this->notify_)
        {
            (*this->notify_)(value);
        }
    }
};


} // namespace detail

} // namespace pex
