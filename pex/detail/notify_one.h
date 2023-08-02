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

template<typename ConnectionType, typename Access>
class NotifyOne_
{
public:
    using Observer = typename ConnectionType::Observer;
    using Callable = typename ConnectionType::Callable;

    void Connect(Observer * const observer, Callable callable)
    {
        PEX_LOG(observer);

        static_assert(
            HasAccess<GetTag, Access>,
            "Cannot connect observer without read access.");

        this->connection_ = ConnectionType(observer, callable);
    }

    ~NotifyOne_()
    {
        if (this->connection_)
        {
            std::cout << "ERROR: Active connections destroyed: ";

#ifdef USE_OBSERVER_NAME
            if constexpr (std::is_void_v<Observer>)
            {
                std::cout << "void ";
            }
            else
            {
                std::cout << Observer::observerName << " ";
            }
#endif
            std::cout << this;
            std::cout << std::endl;

            std::cout << "Was your model destroyed before your controls?"
                << std::endl;

            std::cout << "  " << this->connection_->GetObserver() << std::endl;
        }
    }

    /** Remove all registered callbacks for the observer. **/
    void Disconnect(
        [[maybe_unused]] typename ConnectionType::Observer * const observer)
    {

#ifndef NDEBUG
        if (this->connection_)
        {
            assert(this->connection_->GetObserver() == observer);
        }
#endif

        PEX_LOG(observer);
        this->connection_.reset();
    }

    bool HasObserver(Observer *observer)
    {
        if (!this->connection_)
        {
            return false;
        }

        return (this->connection_->GetObserver() == observer);
    }

    bool HasConnection() const
    {
        return this->connection_.has_value();
    }

protected:
    void ClearConnections_()
    {
        this->connection_.reset();
    }

    Callable GetCallable_() const
    {
        if (!this->connection_)
        {
            throw std::logic_error("There is no connection.");
        }

        return this->connection_->GetCallable();
    }


    std::optional<ConnectionType> connection_;
};


template<typename ConnectionType, typename Access, typename = std::void_t<>>
class NotifyOne : public NotifyOne_<ConnectionType, Access>
{
protected:
    void Notify_()
    {
        if (this->connection_)
        {
            (*this->connection_)();
        }
    }
};


// Selected if ConnectionType has the member 'Type'.
// This Notify_ method takes an argument.
template<typename ConnectionType, typename Access>
class NotifyOne
<
    ConnectionType,
    Access,
    std::void_t<typename ConnectionType::Type>
>
    : public NotifyOne_<ConnectionType, Access>
{
public:
    using Type = typename ConnectionType::Type;

protected:
    void Notify_(Argument<typename ConnectionType::Type> value)
    {
        if (this->connection_)
        {
            (*this->connection_)(value);
        }
    }
};


} // namespace detail

} // namespace pex
