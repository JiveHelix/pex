#pragma once

#include <cassert>
#include <optional>
#include "pex/access_tag.h"
#include "pex/argument.h"
#include "pex/detail/log.h"


#if defined(__GNUG__) && !defined(__clang__) && !defined(_WIN32)
// Avoid bogus -Werror=maybe-uninitialized
#ifndef DO_PRAGMA
#define DO_PRAGMA_(arg) _Pragma (#arg)
#define DO_PRAGMA(arg) DO_PRAGMA_(arg)
#endif

#define GNU_IGNORE_MAYBE_UNINITIALIZED \
    DO_PRAGMA(GCC diagnostic push) \
    DO_PRAGMA(GCC diagnostic ignored "-Wmaybe-uninitialized")

#define GNU_IGNORE_MAYBE_UNINITIALIZED_POP \
    DO_PRAGMA(GCC diagnostic pop)

#else

#define GNU_IGNORE_MAYBE_UNINITIALIZED
#define GNU_IGNORE_MAYBE_UNINITIALIZED_POP

#endif // defined __GNUG__


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

    void Connect(Observer *observer, Callable callable)
    {
        static_assert(
            HasAccess<GetTag, Access>,
            "Cannot connect observer without read access.");

#ifdef USE_OBSERVER_NAME
        if constexpr (std::is_void_v<Observer>)
        {
            PEX_LOG("void (", observer, ") connecting to ", this);
        }
        else
        {
            PEX_LOG(
                Observer::observerName,
                " (",
                observer,
                ") connecting to ",
                this);
        }
#endif

        this->connection_ = ConnectionType(observer, callable);
    }

    ~NotifyOne_()
    {
        if (this->connection_)
        {
            std::cout << "ERROR: Active connection destroyed: ";

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

            assert(false);
        }
    }

    /** Remove all registered callbacks for the observer. **/
    void Disconnect(
        [[maybe_unused]] typename ConnectionType::Observer *observer)
    {

#ifndef NDEBUG
        if (this->connection_)
        {
            assert(this->connection_->GetObserver() == observer);
        }
#endif

#ifdef USE_OBSERVER_NAME
        if constexpr (std::is_void_v<Observer>)
        {
            PEX_LOG("void (", observer, ") disconnecting from ", this);
        }
        else
        {
            PEX_LOG(
                Observer::observerName,
                " (",
                observer,
                ") disconnecting from ",
                this);
        }
#endif

        this->connection_.reset();
    }

    bool HasObserver(Observer *observer)
    {
        if (!this->connection_)
        {
            return false;
        }

        GNU_IGNORE_MAYBE_UNINITIALIZED
        return (this->connection_->GetObserver() == observer);
        GNU_IGNORE_MAYBE_UNINITIALIZED_POP
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
