#pragma once


#include "pex/traits.h"
#include "pex/control_value.h"
#include "pex/detail/value_connection.h"
#include "pex/detail/signal_connection.h"
#include "pex/signal.h"
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
    using Control = control::Value_<Pex>;
    using Upstream = Pex;
};


template<typename Pex>
struct MakeControl
<
    Pex,
    std::enable_if_t<IsControl<Pex>>
>
{
    using Control = Pex;
    using Upstream = typename Pex::Upstream;
};


template<typename Pex>
struct MakeControl<Pex, std::enable_if_t<IsControlSignal<Pex>>>
{
    using Control = Pex; // control::Signal<>;
    using Upstream = typename Pex::Upstream;
};


template<typename Pex>
struct MakeControl<Pex, std::enable_if_t<IsModelSignal<Pex>>>
{
    using Control = control::Signal<>;
    using Upstream = Pex;
};


template<typename Notifier>
class SignalNotifier: public Notifier
{
public:
    using Callable = typename Notifier::Callable;
    using Observer = typename Notifier::Observer;

    void ClearConnections()
    {
        this->ClearConnections_();
    }

    Callable GetCallable() const
    {
        return this->GetCallable_();
    }

    static void OnUpstream(void * observer)
    {
        // The upstream has signaled.
        // Notify our observer.
        auto self = static_cast<SignalNotifier<Notifier> *>(observer);
        self->Notify_();
    }
};


template<typename Notifier>
class ValueNotifier: public Notifier
{
public:
    using Type = typename Notifier::Type;
    using Callable = typename Notifier::Callable;
    using Observer = typename Notifier::Observer;

    void ClearConnections()
    {
        this->ClearConnections_();
    }

    Callable GetCallable() const
    {
        return this->GetCallable_();
    }

    static void OnUpstream(
        void * observer,
        Argument<Type> value)
    {
        // The upstream value has changed.
        // Update our observer.
        auto self = static_cast<ValueNotifier<Notifier> *>(observer);
        self->Notify_(value);
    }
};


template<typename Observer, typename Control, typename enable = void>
struct MakeConnection
{
    using Connection =
        ValueConnection
        <
            Observer,
            typename Control::Type,
            typename Control::Filter
        >;

    using Notifier =
        ValueNotifier
            <
                detail::NotifyOne<Connection, typename Control::Access>
            >;

    using Callable = typename Connection::Callable;
};


template<typename Observer, typename Control>
struct MakeConnection
    <
        Observer,
        Control,
        std::enable_if_t<IsControlSignal<Control>>
    >
{
    using Connection = detail::SignalConnection<Observer>;

    using Notifier =
        SignalNotifier<detail::NotifyOne<Connection, GetAndSetTag>>;

    using Callable = typename Connection::Callable;
};


template<typename Observer, typename Upstream_>
class Terminus_
{

public:
    using ControlType =
        typename MakeControl<Upstream_>::Control;

    using Connection = MakeConnection<Observer, ControlType>;
    using Notifier = typename Connection::Notifier;
    using Callable = typename Connection::Callable;

    static constexpr bool isPexCopyable = true;

    // Make any template specialization of Terminus a friend class.
    template <typename, typename>
    friend class ::pex::Terminus_;

    Terminus_()
        :
        observer_(nullptr),
        notifier_{},
        upstreamControl_{}
    {
        PEX_LOG("Terminus default: ", this);
    }

    Terminus_(Observer *observer, const ControlType &control)
        :
        observer_(observer),
        notifier_{},
        upstreamControl_(control)
    {
        this->upstreamControl_.ClearConnections();
        PEX_LOG("Terminus copy(control) ctor: ", this);
    }

    Terminus_(Observer *observer, const ControlType &control, Callable callable)
        :
        observer_(observer),
        notifier_{},
        upstreamControl_(control)
    {
        this->upstreamControl_.ClearConnections();
        this->Connect(callable);
    }

    Terminus_(Observer *observer, ControlType &&control)
        :
        observer_(observer),
        notifier_{},
        upstreamControl_(std::move(control))
    {
        this->upstreamControl_.ClearConnections();
        PEX_LOG("Terminus move(control) ctor: ", this);
    }

    Terminus_(Observer *observer, ControlType &&control, Callable callable)
        :
        observer_(observer),
        notifier_{},
        upstreamControl_(std::move(control))
    {
        this->upstreamControl_.ClearConnections();
        this->Connect(callable);
    }

    Terminus_(
        Observer *observer,
        typename MakeControl<Upstream_>::Upstream &upstream)
        :
        observer_(observer),
        notifier_{},
        upstreamControl_(upstream)
    {
        this->upstreamControl_.ClearConnections();
        PEX_LOG("Terminus upstream ctor: ", this);
    }

    Terminus_(
        Observer *observer,
        typename MakeControl<Upstream_>::Upstream &upstream,
        Callable callable)
        :
        observer_(observer),
        notifier_{},
        upstreamControl_(upstream)
    {
        this->upstreamControl_.ClearConnections();
        this->Connect(callable);
    }

    Terminus_(const Terminus_ &other) = delete;
    Terminus_(Terminus_ &&other) = delete;
    Terminus_ & operator=(const Terminus_ &) = delete;
    Terminus_ & operator=(Terminus_ &&) = delete;

    // Copy construct
    Terminus_(Observer *observer, const Terminus_ &other)
        :
        observer_(observer),
        notifier_{},
        upstreamControl_(other.upstreamControl_)
    {
        assert(this != &other);
        assert(observer);

        this->upstreamControl_.ClearConnections();

        PEX_LOG("Terminus copy ctor: ", this, " with ", observer);

        if (other.notifier_.HasConnection())
        {
            this->Connect(other.notifier_.GetCallable());
        }
    }

    // Copy construct from other observer
    template<typename O>
    Terminus_(
        Observer *observer,
        const Terminus_<O, Upstream_> &other)
        :
        observer_(observer),
        notifier_{},
        upstreamControl_(other.upstreamControl_)
    {
        PEX_LOG("Terminus copy ctor: ", this, " with ", observer);
        assert(observer);

        this->upstreamControl_.ClearConnections();

        // There is no way to copy the callable from a different observer.
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
        this->upstreamControl_ = other.upstreamControl_;
        this->upstreamControl_.ClearConnections();

        if constexpr (std::is_same_v<Observer, O>)
        {
            if (other.notifier_.HasConnection())
            {
                this->Connect(other.notifier_.GetCallable());
            }
        }
        // else
        // There is no way to copy the callable from a different observer.

        return *this;

    }

    ~Terminus_()
    {
        this->Disconnect();
    }

    void Disconnect()
    {
        this->upstreamControl_.Disconnect(&this->notifier_);
        this->notifier_.ClearConnections();
    }

    bool HasModel() const
    {
        return this->upstreamControl_.HasModel();
    }

    explicit operator ControlType () const
    {
        // return ControlType(this->upstreamControl_);
        return this->upstreamControl_;
    }

    const ControlType & GetUpstream() const
    {
        return this->upstreamControl_;
    }

    Observer * GetObserver()
    {
        return this->observer_;
    }

    void Connect(Callable callable)
    {
        if (!this->observer_)
        {
            throw std::runtime_error("Terminus has no observer.");
        }

        if (!this->upstreamControl_.HasObserver(&this->notifier_))
        {
            // Connect ourselves to the upstream.
            this->upstreamControl_.Connect(
                &this->notifier_,
                &Notifier::OnUpstream);
        }

        this->notifier_.Connect(this->observer_, callable);
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
    Notifier notifier_;

protected:
    ControlType upstreamControl_;
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
    using Upstream = typename MakeControl<Upstream_>::Control;
    using Type = typename Upstream::Type;
    using Filter = typename Upstream::Filter;
    using Access = typename Upstream::Access;

    Type Get() const
    {
        return static_cast<const Derived *>(this)->upstreamControl_.Get();
    }

    void Set(pex::Argument<Type> value)
    {
        static_cast<Derived *>(this)->upstreamControl_.Set(value);
    }

    explicit operator Type () const
    {
        return Type(static_cast<const Derived *>(this)->upstreamControl_);
    }

protected:
    void SetWithoutNotify_(Argument<Type> value)
    {
        detail::AccessReference<Upstream>(
            static_cast<Derived *>(this)->upstreamControl_).SetWithoutNotify(value);
    }

    void DoNotify_()
    {
        detail::AccessReference<Upstream>(
            static_cast<Derived *>(this)->upstreamControl_).DoNotify();
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
    using Upstream = typename MakeControl<Upstream_>::Control;
    using Connection = detail::SignalConnection<Observer>;
    using Callable = typename Connection::Callable;

    void Trigger()
    {
        static_cast<Derived *>(this)->upstreamControl_.Trigger();
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

    using Callable = typename Base::Callable;

    using ControlType = typename Base::ControlType;

    using UpstreamControl =
        typename MakeControl<Upstream_>::Control;

    Terminus(Observer *observer, const Terminus &other)
        :
        Base(observer, other)
    {

    }

    Terminus(Observer *observer, const ControlType &pex, Callable callable)
        :
        Base(observer, pex, callable)
    {

    }


    Terminus(Observer *observer, ControlType &&pex)
        :
        Base(observer, std::move(pex))
    {

    }

    Terminus(Observer *observer, ControlType &&pex, Callable callable)
        :
        Base(observer, std::move(pex), callable)
    {

    }

    Terminus(
        Observer *observer,
        typename MakeControl<Upstream_>::Upstream &upstream)
        :
        Base(observer, upstream)
    {

    }

    Terminus(
        Observer *observer,
        typename MakeControl<Upstream_>::Upstream &upstream,
        Callable callable)
        :
        Base(observer, upstream, callable)
    {

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
