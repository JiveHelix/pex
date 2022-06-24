#pragma once


#include "pex/traits.h"
#include "pex/control_value.h"


namespace pex
{


template<typename Observer, typename Pex_>
class Terminus_
{
public:
    using Pex = control::ChangeObserver<Observer, Pex_>;
    using Callable = typename Pex::Callable;

    template<typename ... PexArgs>
    Terminus_(Observer *observer, PexArgs && ... pexArgs)
        :
        observer_(observer),
        pex_(std::forward<PexArgs>(pexArgs)...)
    {

    }

    Terminus_(const Terminus_ &) = delete;
    Terminus_(Terminus_ &&) = delete;
    Terminus_ & operator=(const Terminus_ &) = delete;
    Terminus_ & operator=(Terminus_ &&) = delete;
    
    ~Terminus_()
    {
        PEX_LOG("Disconnect");
        if constexpr (IsSignal<Pex>)
        {
            this->pex_.Disconnect();
        }
        else
        {
            this->pex_.Disconnect(this->observer_);
        }
    }

    void Connect(Callable callable)
    {
        PEX_LOG("Connect");
        this->pex_.Connect(this->observer_, callable);
    }

private:
    Observer *observer_;

protected:
    Pex pex_;
};


template<typename Observer, typename Pex_, typename enable = void>
class Terminus: public Terminus_<Observer, Pex_>
{
public:
    using Base = Terminus_<Observer, Pex_>;
    using Base::Base;

    using Type = typename Pex_::Type;

    Type Get() const
    {
        return this->pex_.Get();
    }

    void Set(pex::Argument<Type> value)
    {
        this->pex_.Set(value);
    }
};


template<typename Observer, typename Pex_>
class Terminus<Observer, Pex_, std::enable_if_t<IsSignal<Pex_>>>
    : public Terminus_<Observer, Pex_>
{
public:
    using Base = Terminus_<Observer, Pex_>;
    using Base::Base;

    void Trigger()
    {
        this->pex_.Trigger();
    }
};


} // end namespace
