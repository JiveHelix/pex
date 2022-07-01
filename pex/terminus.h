#pragma once


#include "pex/traits.h"
#include "pex/control_value.h"


namespace pex
{


template<typename Observer, typename Pex_>
class Terminus_
{
    template<typename P, typename enable = void>
    struct PexHelper
    {
        using Type = control::Value_<Observer, P>;
    };

    template<typename P>
    struct PexHelper<P, std::enable_if_t<IsControl<P>>>
    {
        using Type = control::ChangeObserver<Observer, P>;
    };

    template<typename P>
    struct PexHelper<P, std::enable_if_t<IsSignal<P>>>
    {
        using Type = control::Signal<Observer>;
    };

public:
    using Pex = typename PexHelper<Pex_>::Type;
    using Callable = typename Pex::Callable;

    Terminus_()
        :
        observer_(nullptr),
        pex_{}
    {
        PEX_LOG("Terminus default: ", this, ", pex_: ", &this->pex_);
    }

    template<typename ... PexArgs>
    Terminus_(Observer *observer, PexArgs && ... pexArgs)
        :
        observer_(observer),
        pex_(std::forward<PexArgs>(pexArgs)...)
    {
        PEX_LOG("Terminus ctor: ", this, ", pex_: ", &this->pex_);
    }

    Terminus_(const Terminus_ &other) = delete;

    Terminus_(Terminus_ &&other)
        :
        observer_(other.observer_),
        pex_(std::move(other.pex_))
    {
        PEX_LOG("Terminus move ctor: ", this, ", pex_: ", &this->pex_);
        other.observer_ = nullptr;
    }

    Terminus_ & operator=(const Terminus_ &) = delete;

    Terminus_ & operator=(Terminus_ &&other)
    {
        assert(this != &other);

        this->Disconnect();
        other.Disconnect();

        this->observer_ = other.observer_;
        other.observer_ = nullptr;

        this->pex_ = std::move(other.pex_);

        PEX_LOG("Terminus move assign: ", this, ", pex_: ", &this->pex_);

        return *this;
    }
    
    ~Terminus_()
    {
        PEX_LOG("Terminus destroy: ", this, ", pex_: ", &this->pex_);
        this->Disconnect();
    }

    void Disconnect()
    {
        if (this->observer_)
        {
            PEX_LOG("Disconnect: ", this->observer_, " from ", &this->pex_);
            this->pex_.Disconnect(this->observer_);
        }
    }

    void Connect(Callable callable)
    {
        if (!this->observer_)
        {
            throw std::runtime_error("Terminus has no observer.");
        }

        PEX_LOG("Connect to: ", &this->pex_);
        this->pex_.Connect(this->observer_, callable);
    }

    bool HasModel() const
    {
        return this->pex_.HasModel();
    }

    explicit operator pex::control::ChangeObserver<void, Pex> () const
    {
        return this->pex_;
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

    using Type = typename Base::Pex::Type;

    Terminus & operator=(Terminus &&other)
    {
        Base::operator=(std::move(other));
        return *this;
    }

    Type Get() const
    {
        return this->pex_.Get();
    }

    void Set(pex::Argument<Type> value)
    {
        this->pex_.Set(value);
    }

    explicit operator Type () const
    {
        return Type(this->pex_);
    }
};


template<typename Observer, typename Pex_>
class Terminus<Observer, Pex_, std::enable_if_t<IsSignal<Pex_>>>
    : public Terminus_<Observer, Pex_>
{
public:
    using Base = Terminus_<Observer, Pex_>;
    using Base::Base;

    Terminus & operator=(Terminus &&other)
    {
        Base::operator=(std::move(other));
        return *this;
    }

    void Trigger()
    {
        this->pex_.Trigger();
    }
};


template<typename ...T>
struct IsTerminus_: std::false_type {};

template<typename ...T>
struct IsTerminus_<Terminus<T...>>: std::true_type {};

template<typename ...T>
inline constexpr bool IsTerminus = IsTerminus_<T...>::value;


} // end namespace pex
