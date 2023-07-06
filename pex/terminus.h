#pragma once


#include "pex/traits.h"
#include "pex/control_value.h"
#include "pex/reference.h"


namespace pex
{


/** MakeControl
 **
 ** Converts model values/signals into controls.
 ** Preserves control values/signals.
 **
 **/
template<typename Pex, typename enable = void>
struct MakeControl
{
    template<typename O>
    using Control = control::Value_<O, Pex>;

    using Upstream = Pex;
};


template<typename Pex>
struct MakeControl
<
    Pex,
    std::enable_if_t<IsControl<Pex>>
>
{
    template<typename O>
    using Control = control::ChangeObserver<O, Pex>;

    using Upstream = typename Pex::Upstream;
};


template<typename Pex>
struct MakeControl<Pex, std::enable_if_t<IsControlSignal<Pex>>>
{
    template<typename O>
    using Control = control::Signal<O>;

    using Upstream = typename Pex::Upstream;
};


template<typename Pex>
struct MakeControl<Pex, std::enable_if_t<IsModelSignal<Pex>>>
{
    template<typename O>
    using Control = control::Signal<O>;

    using Upstream = Pex;
};


template<typename Observer, typename Upstream_>
class Terminus_
{

public:
    template<typename O>
    using ControlTemplate =
        typename MakeControl<Upstream_>::template Control<O>;

    using Callable = typename ControlTemplate<Observer>::Callable;

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

    Terminus_(Observer *observer, const ControlTemplate<void> &pex)
        :
        observer_(observer),
        pex_(pex)
    {
        PEX_LOG("Terminus copy(pex) ctor: ", this);
    }

    Terminus_(Observer *observer, ControlTemplate<void> &&pex)
        :
        observer_(observer),
        pex_(std::move(pex))
    {
        PEX_LOG("Terminus move(pex) ctor: ", this);
    }

    Terminus_(
        Observer *observer,
        typename MakeControl<Upstream_>::Upstream &upstream)
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
        const Terminus_<O, Upstream_> &other)
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
        const Terminus_<O, control::ChangeObserver<O, Upstream_>> &other)
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
        const Terminus_<O, Upstream_> &&other)
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
        const Terminus_<O, control::ChangeObserver<O, Upstream_>> &&other)
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
        const Terminus_<O, Upstream_> &other)
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
        const Terminus_<O, control::ChangeObserver<O, Upstream_>> &other)
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
        Terminus_<O, Upstream_> &&other)
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
        Terminus_<O, control::ChangeObserver<O, Upstream_>> &&other)
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

        if constexpr (!std::is_void_v<Observer>)
        {

            PEX_LOG(
                "Connect to: ",
                &this->pex_,
                " with observer ",
                Observer::observerName,
                ": ",
                this->observer_);
        }
        else
        {
            PEX_LOG(
                "Connect to: ",
                &this->pex_,
                " with observer: ",
                this->observer_);
        }

        this->pex_.Connect(this->observer_, callable);
    }

    bool HasModel() const
    {
        return this->pex_.HasModel();
    }

    explicit operator ControlTemplate<void> () const
    {
        return ControlTemplate<void>(this->pex_);
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
    ControlTemplate<Observer> pex_;
};


template
<
    template<typename, typename> typename Derived_,
    typename Observer,
    typename Upstream_,
    typename enable = void
>
struct ImplementInterface
{
    using Derived = Derived_<Observer, Upstream_>;
    using Pex = typename MakeControl<Upstream_>::template Control<Observer>;
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
    typename Upstream_
>
struct ImplementInterface
<
    Derived_,
    Observer,
    Upstream_,
    std::enable_if_t<IsSignal<Upstream_>>
>
{
public:
    using Derived = Derived_<Observer, Upstream_>;
    using Pex = typename MakeControl<Upstream_>::template Control<Observer>;

    void Trigger()
    {
        static_cast<Derived *>(this)->pex_.Trigger();
    }
};



template<typename Observer, typename Upstream_>
class Terminus:
    public Terminus_<Observer, Upstream_>,
    public ImplementInterface<Terminus, Observer, Upstream_>
{
public:
    using Base = Terminus_<Observer, Upstream_>;
    using Base::Base;

    Terminus(Observer *observer, const Terminus &other)
        : Base(observer, other)
    {
    }

    Terminus(Observer *observer, Terminus &&other)
        : Base(observer, std::move(other))
    {
    }

    template <typename O>
    Terminus & Assign(Observer *observer, const Terminus<O, Upstream_> &other)
    {
        Base::Assign(observer, other);
        return *this;
    }

    template <typename O>
    Terminus & Assign(
        Observer *observer,
        const Terminus<O, control::ChangeObserver<O, Upstream_>> &other)
    {
        Base::Assign(observer, other);
        return *this;
    }

    template <typename O>
    Terminus & Assign(Observer *observer, Terminus<O, Upstream_> &&other)
    {
        Base::Assign(observer, std::move(other));
        return *this;
    }

    template <typename O>
    Terminus & Assign(
        Observer *observer,
        Terminus<O, control::ChangeObserver<O, Upstream_>> &&other)
    {
        Base::Assign(observer, std::move(other));
        return *this;
    }

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
