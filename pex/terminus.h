#pragma once


#include "pex/traits.h"
#include "pex/control_value.h"
#include "pex/reference.h"


namespace pex
{


template<typename P, typename enable = void>
struct ManagedControl
{
    template<typename O>
    using Type = control::Value_<O, P>;

    using Upstream = P;
};

template<typename P>
struct ManagedControl<P, std::enable_if_t<IsControl<P>>>
{
    template<typename O>
    using Type = control::ChangeObserver<O, P>;

    using Upstream = typename P::Pex;
};

template<typename P>
struct ManagedControl<P, std::enable_if_t<IsControlSignal<P>>>
{
    template<typename O>
    using Type = control::Signal<O>;

    using Upstream = typename P::Pex;
};

template<typename P>
struct ManagedControl<P, std::enable_if_t<IsModelSignal<P>>>
{
    template<typename O>
    using Type = control::Signal<O>;

    using Upstream = P;
};


template<typename Observer, typename Pex_>
class Terminus_
{

public:
    template<typename O>
    using Managed = typename ManagedControl<Pex_>::template Type<O>;

    using Callable = typename Managed<Observer>::Callable;

    static constexpr bool isPexCopyable = true;

    // Make any template specialization of Terminus a friend class.
    template <typename, typename>
    friend class ::pex::Terminus_;

    Terminus_()
        :
        observer_(nullptr),
        pex_{}
    {
        PEX_LOG("Terminus default: ", this);
    }

    Terminus_(Observer *observer, const Managed<void> &pex)
        :
        observer_(observer),
        pex_(pex)
    {
        PEX_LOG("Terminus copy(pex) ctor: ", this);
    }

    Terminus_(Observer *observer, Managed<void> &&pex)
        :
        observer_(observer),
        pex_(std::move(pex))
    {
        PEX_LOG("Terminus move(pex) ctor: ", this);
    }

    Terminus_(
        Observer *observer,
        typename ManagedControl<Pex_>::Upstream &upstream)
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
    template<typename O>
    Terminus_(
        Observer *observer,
        const Terminus_<O, Pex_> &other)
        :
        observer_(observer),
        pex_(other.pex_)
    {
        PEX_LOG("Terminus copy ctor: ", this, " with ", observer);
    }

    // Copy construct from other observer
    template<typename O>
    Terminus_(
        Observer *observer,
        const Terminus_<O, control::ChangeObserver<O, Pex_>> &other)
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
    template<typename O>
    Terminus_(
        Observer *observer,
        const Terminus_<O, Pex_> &&other)
        :
        observer_(observer),
        pex_()
    {
        PEX_LOG("Terminus move ctor: ", this, " with ", observer);
        other.Disconnect();
        this->pex_ = std::move(other.pex_);
        other.observer_ = nullptr;
    }

    // Move construct from other observer
    template<typename O>
    Terminus_(
        Observer *observer,
        const Terminus_<O, control::ChangeObserver<O, Pex_>> &&other)
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
    template<typename O>
    Terminus_ & Assign(
        Observer *observer,
        const Terminus_<O, Pex_> &other)
    {
        if constexpr (std::is_same_v<Observer, O>)
        {
            assert(this != &other);
        }

        PEX_LOG("Terminus copy assign: ", this);

        this->Disconnect();
        this->observer_ = observer;
        this->pex_ = other.pex_;

        return *this;
    }

    // Copy assign
    template<typename O>
    Terminus_ & Assign(
        Observer *observer,
        const Terminus_<O, control::ChangeObserver<O, Pex_>> &other)
    {
        if constexpr (std::is_same_v<Observer, O>)
        {
            assert(this != &other);
        }

        PEX_LOG("Terminus copy assign: ", this);

        this->Disconnect();
        this->observer_ = observer;
        this->pex_ = other.pex_;

        return *this;
    }

    // Move assign
    template<typename O>
    Terminus_ & Assign(
        Observer *observer,
        Terminus_<O, Pex_> &&other)
    {
        if constexpr (std::is_same_v<Observer, O>)
        {
            assert(this != &other);
        }

        PEX_LOG("Terminus move assign: ", this);

        this->Disconnect();
        other.Disconnect();

        this->observer_ = observer;
        other.observer_ = nullptr;

        this->pex_ = std::move(other.pex_);

        PEX_LOG("pex_: ", &this->pex_);

        return *this;
    }

    // Move assign
    template<typename O>
    Terminus_ & Assign(
        Observer *observer,
        Terminus_<O, control::ChangeObserver<O, Pex_>> &&other)
    {
        if constexpr (std::is_same_v<Observer, O>)
        {
            assert(this != &other);
        }

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

    explicit operator Managed<void> () const
    {
        return this->pex_;
    }

    Observer * GetObserver()
    {
        return this->observer_;
    }

    template
    <
        template<typename, typename> typename,
        typename,
        typename,
        typename
    >
    friend struct ImplementInterface;

private:
    Observer *observer_;

protected:
    Managed<Observer> pex_;
};


#define INHERIT_TERMINUS_CONSTRUCTORS                                    \
    using Base = Terminus_<Observer, Pex_>;                              \
    using Base::Base;                                                    \
                                                                         \
    Terminus(Observer *observer, const Terminus &other)                  \
        : Base(observer, other)                                          \
    {                                                                    \
    }                                                                    \
                                                                         \
    Terminus(Observer *observer, Terminus &&other)                       \
        : Base(observer, std::move(other))                               \
    {                                                                    \
    }                                                                    \
                                                                         \
    template <typename O>                                                \
    Terminus &Assign(Observer *observer, const Terminus<O, Pex_> &other) \
    {                                                                    \
        Base::Assign(observer, other);                                   \
        return *this;                                                    \
    }                                                                    \
                                                                         \
    template <typename O>                                                \
    Terminus &Assign(                                                    \
        Observer *observer,                                              \
        const Terminus<O, control::ChangeObserver<O, Pex_>> &other)      \
    {                                                                    \
        Base::Assign(observer, other);                                   \
        return *this;                                                    \
    }                                                                    \
                                                                         \
    template <typename O>                                                \
    Terminus &Assign(Observer *observer, Terminus<O, Pex_> &&other)      \
    {                                                                    \
        Base::Assign(observer, std::move(other));                        \
        return *this;                                                    \
    }                                                                    \
                                                                         \
    template <typename O>                                                \
    Terminus &Assign(                                                    \
        Observer *observer,                                              \
        Terminus<O, control::ChangeObserver<O, Pex_>> &&other)           \
    {                                                                    \
        Base::Assign(observer, std::move(other));                        \
        return *this;                                                    \
    }



template
<
    template<typename, typename> typename Derived_,
    typename Observer,
    typename Pex_,
    typename enable = void
>
struct ImplementInterface
{
    using Derived = Derived_<Observer, Pex_>;
    using Pex = typename ManagedControl<Pex_>::template Type<Observer>;
    using Type = typename Pex::Type;
    using Filter = typename Pex::Filter;

    Type Get() const
    {
        return static_cast<const Derived *>(this)->pex_.Get();
    }

    void Set(pex::Argument<Type> value)
    {
        static_cast<Derived *>(this)->pex_.Set(value);
    }

    explicit operator Type () const
    {
        return Type(static_cast<const Derived *>(this)->pex_);
    }

protected:
    void SetWithoutNotify_(Argument<Type> value)
    {
        detail::AccessReference<Pex>(
            static_cast<Derived *>(this)->pex_).SetWithoutNotify(value);
    }

    void DoNotify_()
    {
        detail::AccessReference<Pex>(
            static_cast<Derived *>(this)->pex_).DoNotify();
    }
};


template
<
    template<typename, typename> typename Derived_,
    typename Observer,
    typename Pex_
>
struct ImplementInterface
<
    Derived_,
    Observer,
    Pex_,
    std::enable_if_t<IsSignal<Pex_>>
>
{
public:
    using Derived = Derived_<Observer, Pex_>;
    using Pex = typename ManagedControl<Pex_>::template Type<Observer>;

    void Trigger()
    {
        static_cast<Derived *>(this)->pex_.Trigger();
    }
};



template<typename Observer, typename Pex_>
class Terminus:
    public Terminus_<Observer, Pex_>,
    public ImplementInterface<Terminus, Observer, Pex_>
{
public:
    INHERIT_TERMINUS_CONSTRUCTORS

    template<typename>
    friend class Reference;
};


template<typename ...T>
struct IsTerminus_: std::false_type {};

template<typename ...T>
struct IsTerminus_<Terminus<T...>>: std::true_type {};

template<typename ...T>
inline constexpr bool IsTerminus = IsTerminus_<T...>::value;


} // end namespace pex
