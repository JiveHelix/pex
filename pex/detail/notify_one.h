#pragma once

#include <optional>


namespace pex
{

namespace detail
{

template<typename Notify>
class NotifyOne_
{
public:
    void Connect(
        typename Notify::Observer * const observer,
        typename Notify::Callable callable)
    {
        this->notify_ = Notify(observer, callable);
    }

    /** Remove all registered callbacks for the observer. **/
    void Disconnect()
    {
        this->notify_.reset();
    }

    std::optional<Notify> notify_;
};


template<typename Notify, typename = std::void_t<>>
class NotifyOne : public NotifyOne_<Notify>
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
template<typename Notify>
class NotifyOne<Notify, std::void_t<typename Notify::Type>>
    : public NotifyOne_<Notify>
{
protected:
    void Notify_(typename Notify::argumentType value)
    {
        if (this->notify_)
        {
            (*this->notify_)(value);
        }
    }
};


} // namespace detail

} // namespace pex
