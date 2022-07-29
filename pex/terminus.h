#pragma once


#include "pex/traits.h"
#include "pex/control_value.h"
#include "pex/reference.h"


namespace pex
{


template<typename Observer, typename Pex_>
class Terminus_
{
    template<typename P, typename enable = void>
    struct PexHelper
    {
        template<typename O>
        using Type = control::Value_<O, P>;

        using Upstream = P;
    };

    template<typename P>
    struct PexHelper<P, std::enable_if_t<IsControl<P>>>
    {
        template<typename O>
        using Type = control::ChangeObserver<O, P>;

        using Upstream = typename P::Pex;
    };

    template<typename P>
    struct PexHelper<P, std::enable_if_t<IsControlSignal<P>>>
    {
        template<typename O>
        using Type = control::Signal<O>;

        using Upstream = typename P::Pex;
    };

    template<typename P>
    struct PexHelper<P, std::enable_if_t<IsModelSignal<P>>>
    {
        template<typename O>
        using Type = control::Signal<O>;

        using Upstream = P;
    };

public:
    template<typename O>
    using Pex = typename PexHelper<Pex_>::template Type<O>;

    using Callable = typename Pex<Observer>::Callable;

    Terminus_()
        :
        observer_(nullptr),
        pex_{}
    {
        PEX_LOG("Terminus default: ", this);
    }

    Terminus_(Observer *observer, const Pex<void> &pex)
        :
        observer_(observer),
        pex_(pex)
    {
        PEX_LOG("Terminus copy(pex) ctor: ", this);
    }

    Terminus_(Observer *observer, Pex<void> &&pex)
        :
        observer_(observer),
        pex_(std::move(pex))
    {
        PEX_LOG("Terminus move(pex) ctor: ", this);
    }

    Terminus_(
        Observer *observer,
        typename PexHelper<Pex_>::Upstream &upstream)
        :
        observer_(observer),
        pex_(upstream)
    {
        PEX_LOG("Terminus upstream ctor: ", this);
    }

    Terminus_(const Terminus_ &other) = delete;
    Terminus_(Terminus_ &&other) = delete;
    Terminus_ & operator=(const Terminus_ &) = delete;
    Terminus_ & operator=(Terminus_ &&) = delete;

    // Copy construct
    Terminus_(Observer *observer, const Terminus_ &other)
        :
        observer_(observer),
        pex_(other.pex_)
    {
        assert(this != &other);
        PEX_LOG("Terminus copy ctor: ", this, " with ", observer);
    }

    // Copy construct from other observer
    template<typename OtherObserver>
    Terminus_(
        Observer *observer,
        const Terminus_<OtherObserver, Pex_> &other)
        :
        observer_(observer),
        pex_(other.pex_)
    {
        PEX_LOG("Terminus copy ctor: ", this, " with ", observer);
    }

    // Move construct
    Terminus_(Observer *observer, Terminus_ &&other)
        :
        observer_(observer),
        pex_()
    {
        assert(this != &other);
        PEX_LOG("Terminus move ctor: ", this, " with ", observer);
        other.Disconnect();
        this->pex_ = std::move(other.pex_);
        other.observer_ = nullptr;
    }

    // Move construct from other observer
    template<typename OtherObserver>
    Terminus_(Observer *observer, Terminus_<OtherObserver, Pex_> &&other)
        :
        observer_(observer),
        pex_()
    {
        PEX_LOG("Terminus move ctor: ", this, " with ", observer);
        other.Disconnect();
        this->pex_ = std::move(other.pex_);
        other.observer_ = nullptr;
    }

    // Copy assign
    template<typename OtherObserver>
    Terminus_ & Assign(
        Observer *observer,
        const Terminus_<OtherObserver, Pex_> &other)
    {
        assert(this != &other);

        PEX_LOG("Terminus copy assign: ", this);

        this->Disconnect();
        this->observer_ = observer;
        this->pex_ = other.pex_;

        return *this;
    }

    // Move assign
    template<typename OtherObserver>
    Terminus_ & Assign(
        Observer *observer,
        Terminus_<OtherObserver, Pex_> &&other)
    {
        assert(this != &other);

        PEX_LOG("Terminus move assign: ", this);

        this->Disconnect();
        other.Disconnect();

        this->observer_ = observer;
        other.observer_ = nullptr;

        this->pex_ = std::move(other.pex_);

        PEX_LOG("pex_: ", &this->pex_);

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
            PEX_LOG(
                "Terminus_ Disconnect: ",
                this->observer_,
                " from ",
                &this->pex_);

            this->pex_.Disconnect(this->observer_);
        }
    }

    void Connect(Callable callable)
    {
        if (!this->observer_)
        {
            throw std::runtime_error("Terminus has no observer.");
        }

        PEX_LOG(
            "Connect to: ",
            &this->pex_,
            " with observer: ",
            this->observer_);

        this->pex_.Connect(this->observer_, callable);
    }

    bool HasModel() const
    {
        return this->pex_.HasModel();
    }

    explicit operator Pex<void> () const
    {
        return this->pex_;
    }

    Observer * GetObserver()
    {
        return this->observer_;
    }

private:
    Observer *observer_;

protected:
    Pex<Observer> pex_;
};


#define INHERIT_TERMINUS_CONSTRUCTORS                                        \
    using Base = Terminus_<Observer, Pex_>;                                  \
    using Base::Base;                                                        \
                                                                             \
    Terminus(Observer *observer, const Terminus &other)                      \
        : Base(observer, other)                                              \
    {                                                                        \
    }                                                                        \
                                                                             \
    Terminus(Observer *observer, Terminus &&other)                           \
        : Base(observer, std::move(other))                                   \
    {                                                                        \
    }                                                                        \
                                                                             \
    template <typename OtherObserver>                                        \
    Terminus(Observer *observer, const Terminus<OtherObserver, Pex_> &other) \
        : Base(observer, other)                                              \
    {                                                                        \
    }                                                                        \
                                                                             \
    template <typename OtherObserver>                                        \
    Terminus(Observer *observer, Terminus<OtherObserver, Pex_> &&other)      \
        : Base(observer, std::move(other))                                   \
    {                                                                        \
    }                                                                        \
                                                                             \
    template <typename OtherObserver>                                        \
    Terminus &Assign(                                                        \
        Observer *observer, const Terminus<OtherObserver, Pex_> &other)      \
    {                                                                        \
        Base::Assign(observer, other);                                       \
        return *this;                                                        \
    }                                                                        \
                                                                             \
    template <typename OtherObserver>                                        \
    Terminus &Assign(                                                        \
        Observer *observer, Terminus<OtherObserver, Pex_> &&other)           \
    {                                                                        \
        Base::Assign(observer, std::move(other));                            \
        return *this;                                                        \
    }


template<typename Observer, typename Pex_, typename enable = void>
class Terminus: public Terminus_<Observer, Pex_>
{
public:
    INHERIT_TERMINUS_CONSTRUCTORS

    using Pex = typename Base::template Pex<Observer>;
    using Type = typename Pex::Type;
    using Filter = typename Pex::Filter;

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

    template<typename>
    friend class Reference;

protected:
    void SetWithoutNotify_(Argument<Type> value)
    {
        internal::AccessReference<Pex>(this->pex_).SetWithoutNotify(value);
    }

    void DoNotify_()
    {
        internal::AccessReference<Pex>(this->pex_).DoNotify();
    }
};


template<typename Observer, typename Pex_>
class Terminus<Observer, Pex_, std::enable_if_t<IsSignal<Pex_>>>
    : public Terminus_<Observer, Pex_>
{
public:
    INHERIT_TERMINUS_CONSTRUCTORS

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
