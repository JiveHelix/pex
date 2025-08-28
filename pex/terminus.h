#pragma once


#include <fmt/core.h>
#include "pex/detail/value_connection.h"
#include "pex/detail/signal_connection.h"
#include "pex/reference.h"
// #include "pex/promote_control.h"


namespace pex
{


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
        std::enable_if_t<IsSignalControl<Control>>
    >
{
    using Connection = detail::SignalConnection<Observer>;

    using Notifier =
        SignalNotifier<detail::NotifyOne<Connection, GetAndSetTag>>;

    using Callable = typename Connection::Callable;
};


template<typename Observer, typename Upstream_>
class Terminus;


template<typename Item>
class EndpointVector
{
private:
    template<typename, typename>
    friend class Terminus;

    class Wrapper: public Item
    {
    public:
        using Item::Item;

        Wrapper(Wrapper &&other) noexcept
            :
            Item(std::move(other))
        {

        }

        Wrapper & operator=(Wrapper &&other) noexcept
        {
            this->Item::operator=(std::move(other));

            return *this;
        }
    };

public:

    class EndpointRef
    {
        Wrapper & wrapper_;

        EndpointRef(Wrapper &wrapper)
            :
            wrapper_(wrapper)
        {

        }

    public:
        // EndpointRef cannot be copied or moved, so it always refers to
        // a member of this vector.
        //
        // Assignment is only allowed by move semantics, disabling copying of
        // the underlying endpoint.
        EndpointRef(const EndpointRef &) = delete;
        EndpointRef(EndpointRef &&) = delete;
        EndpointRef & operator=(const EndpointRef &) = delete;

        Item & operator=(Item &&other)
        {
            this->wrapper_ = Wrapper(std::move(other));

            return this->wrapper_;
        }

        operator const Item & () const
        {
            return this->wrapper_;
        }
    };

    EndpointVector()
        :
        items_{}
    {

    }

    void push_back(Item &&item)
    {
        this->items_.push_back(Wrapper(std::move(item)));
    }

    template<typename ...Args>
    void emplace_back(Args&&... args)
    {
        this->items_.emplace_back(std::forward<Args>(args)...);
    }

    void clear()
    {
        this->items_.clear();
    }

    void resize(size_t index)
    {
        this->items_.resize(index);
    }

    Item & at(size_t index)
    {
        return this->items_.at(index);
    }

    const Item & at(size_t index) const
    {
        return this->items_.at(index);
    }

    bool empty() const
    {
        return this->items_.empty();
    }

    size_t size() const
    {
        return this->items_.size();
    }

    EndpointRef operator[](size_t index)
    {
        return this->items_[index];
    }

    const EndpointRef operator[](size_t index) const
    {
        return this->items_[index];
    }

    EndpointVector(const EndpointVector &) = delete;
    EndpointVector(EndpointVector &&) = delete;
    EndpointVector & operator=(const EndpointVector &) = delete;
    EndpointVector & operator=(EndpointVector &&) = delete;


private:
    std::vector<Wrapper> items_;
};


template<typename Observer, typename Upstream_>
class Terminus_: Separator
{
    static_assert(IsCopyable<Upstream_>);

public:
    // using ControlType = typename PromoteControl<Upstream_>::Type;
    using ControlType = Upstream_;

    using UpstreamControl = ControlType;
    using Connection = MakeConnection<Observer, ControlType>;
    using Notifier = typename Connection::Notifier;
    using Callable = typename Connection::Callable;

    static constexpr bool isPexCopyable = true;

    // Make any template specialization of Terminus a friend class.
    template <typename, typename>
    friend class ::pex::Terminus_;

    template
    <
        template<typename, typename> typename,
        typename,
        typename,
        typename
    >
    friend struct ImplementInterface;

    template<typename>
    friend class Reference;

    template<typename>
    friend class ConstControlReference;

    template<typename, typename>
    friend class Terminus;

    Terminus_()
        :
        observer_(nullptr),
        notifier_{},
        upstreamControl_{}
    {
        PEX_LOG("Terminus default: ", this);

        PEX_NAME("Terminus_");
        PEX_MEMBER(notifier_);
        PEX_MEMBER(upstreamControl_);
    }

    explicit Terminus_(const ControlType &control)
        :
        observer_(nullptr),
        notifier_{},
        upstreamControl_(control)
    {
        PEX_NAME("Terminus_");
        PEX_MEMBER(notifier_);
        PEX_MEMBER(upstreamControl_);

        this->upstreamControl_.ClearConnections();
        PEX_LOG("Terminus copy(control) ctor: ", this);
    }

    Terminus_(Observer *observer, const ControlType &control, Callable callable)
        :
        observer_(nullptr),
        notifier_{},
        upstreamControl_(control)
    {
        PEX_NAME("Terminus_");
        PEX_MEMBER(notifier_);
        PEX_MEMBER(upstreamControl_);

        this->upstreamControl_.ClearConnections();
        this->Connect(observer, callable);
    }

    void Emplace(const ControlType &control)
    {
        this->Disconnect();
        this->upstreamControl_ = control;
        this->upstreamControl_.ClearConnections();
    }

    void Emplace(
        Observer *observer,
        const ControlType &control,
        Callable callable)
    {
        this->Disconnect();
        this->upstreamControl_ = control;
        this->upstreamControl_.ClearConnections();
        this->Connect(observer, callable);
    }

    explicit Terminus_(ControlType &&control)
        :
        observer_(nullptr),
        notifier_{},
        upstreamControl_(std::move(control))
    {
        PEX_NAME("Terminus_");
        PEX_MEMBER(notifier_);
        PEX_MEMBER(upstreamControl_);

        this->upstreamControl_.ClearConnections();
    }

    Terminus_(Observer *observer, ControlType &&control, Callable callable)
        :
        observer_(nullptr),
        notifier_{},
        upstreamControl_(std::move(control))
    {
        PEX_NAME("Terminus_");
        PEX_MEMBER(notifier_);
        PEX_MEMBER(upstreamControl_);

        this->upstreamControl_.ClearConnections();
        this->Connect(observer, callable);
    }
#if 0
    explicit Terminus_(typename PromoteControl<Upstream_>::Upstream &upstream)
        :
        observer_(nullptr),
        notifier_{},
        upstreamControl_(upstream)
    {
        PEX_NAME("Terminus_");
        PEX_MEMBER(notifier_);
        PEX_MEMBER(upstreamControl_);

        this->upstreamControl_.ClearConnections();
        PEX_LOG("Terminus upstream ctor: ", this);
    }

    Terminus_(
        Observer *observer,
        typename PromoteControl<Upstream_>::Upstream &upstream,
        Callable callable)
        :
        observer_(nullptr),
        notifier_{},
        upstreamControl_(upstream)
    {
        PEX_NAME("Terminus_");
        PEX_MEMBER(notifier_);
        PEX_MEMBER(upstreamControl_);

        this->upstreamControl_.ClearConnections();
        this->Connect(observer, callable);
    }
#endif

    // It does not make sense to copy callbacks to the same observer.
    // Every copy would mean extra meaningless notifications.
    Terminus_(const Terminus_ &) = delete;
    Terminus_ & operator=(const Terminus_ &) = delete;

private:

    Terminus_(Terminus_ &&other) noexcept
        :
        observer_(nullptr),
        notifier_(),
        upstreamControl_(std::move(other.upstreamControl_))
    {
        assert(this != &other);

        PEX_NAME("Terminus_");
        PEX_MEMBER(notifier_);
        PEX_MEMBER(upstreamControl_);

        this->upstreamControl_.ClearConnections();

        if (other.notifier_.HasConnection())
        {
            assert(other.observer_);
            this->Connect(other.observer_, other.notifier_.GetCallable());
            other.notifier_.ClearConnections();
            assert(!other.upstreamControl_.HasObserver(&other.notifier_));
            other.observer_ = nullptr;
        }
    }

    Terminus_ & operator=(Terminus_ &&other) noexcept
    {
        assert(this != &other);

        this->Disconnect();

        this->upstreamControl_ = std::move(other.upstreamControl_);
        this->upstreamControl_.ClearConnections();

        if (other.notifier_.HasConnection())
        {
            assert(other.observer_);
            this->Connect(other.observer_, other.notifier_.GetCallable());
            other.notifier_.ClearConnections();
            assert(!other.upstreamControl_.HasObserver(&other.notifier_));
            other.observer_ = nullptr;
        }
    }

public:
    // Copy construct
    Terminus_(Observer *observer, const Terminus_ &other)
        :
        Terminus_(other.upstreamControl_)
    {
        assert(this != &other);

        if (other.notifier_.HasConnection())
        {
            this->Connect(observer, other.notifier_.GetCallable());
        }
    }

    // Move construct
    Terminus_(Observer *observer, Terminus_ &&other)
        :
        Terminus_(std::move(other.upstreamControl_))
    {
        assert(this != &other);

        if (other.notifier_.HasConnection())
        {
            assert(observer != nullptr);
            this->Connect(observer, other.notifier_.GetCallable());
            other.notifier_.ClearConnections();
            assert(!other.upstreamControl_.HasObserver(&other.notifier_));
            other.observer_ = nullptr;
        }
    }

    // Copy construct from other observer
    template<typename O>
    Terminus_(
        Observer *observer,
        const Terminus_<O, Upstream_> &other)
        :
        observer_(nullptr),
        notifier_{},
        upstreamControl_(other.upstreamControl_)
    {
        PEX_NAME("Terminus_");
        PEX_MEMBER(notifier_);
        PEX_MEMBER(upstreamControl_);

        PEX_LOG(
            "Terminus copy ctor: ",
            this,
            " with ",
            LookupPexName(observer));

        assert(observer);

        this->upstreamControl_.ClearConnections();

        if constexpr (std::is_same_v<Observer, O>)
        {
            if (other.notifier_.HasConnection())
            {
                this->Connect(observer, other.notifier_.GetCallable());
            }
        }
        // else
        // There is no way to copy the callable from a different observer.
    }

    // Move construct from other observer
    template<typename O>
    Terminus_(
        Observer *observer,
        Terminus_<O, Upstream_> &&other)
        :
        Terminus_(std::move(other.upstreamControl_))
    {
        if constexpr (std::is_same_v<Observer, O>)
        {
            if (other.notifier_.HasConnection())
            {
                this->Connect(observer, other.notifier_.GetCallable());
                other.notifier_.ClearConnections();
                assert(!other.upstreamControl_.HasObserver(&other.notifier_));
                other.observer_ = nullptr;
            }
        }
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
        this->upstreamControl_ = other.upstreamControl_;
        this->upstreamControl_.ClearConnections();

        if constexpr (std::is_same_v<Observer, O>)
        {
            if (other.notifier_.HasConnection())
            {
                this->Connect(observer, other.notifier_.GetCallable());
            }
        }
        // else
        // There is no way to copy the callable from a different observer.

        return *this;
    }

    Terminus_ & RequireAssign(
        Observer *observer,
        const Terminus_<Observer, Upstream_> &other)
    {
        return this->Assign(observer, other);
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
        this->upstreamControl_ = std::move(other.upstreamControl_);
        this->upstreamControl_.ClearConnections();

        if constexpr (std::is_same_v<Observer, O>)
        {
            if (other.notifier_.HasConnection())
            {
                this->Connect(observer, other.notifier_.GetCallable());
                other.notifier_.ClearConnections();
                assert(!other.upstreamControl_.HasObserver(&other.notifier_));
                other.observer_ = nullptr;
            }
        }
        // else
        // There is no way to copy the callable from a different observer.

        return *this;
    }

    Terminus_ & RequireAssign(
        Observer *observer,
        Terminus_<Observer, Upstream_> &&other)
    {
        return this->Assign(observer, std::move(other));
    }

    ~Terminus_()
    {
        this->Disconnect();
        PEX_CLEAR_NAME(this);
        PEX_CLEAR_NAME(&this->notifier_);
        PEX_CLEAR_NAME(&this->upstreamControl_);
    }

    void Disconnect() noexcept
    {
        if (!this->observer_)
        {
            assert(!this->notifier_.HasConnection());
            assert(!this->upstreamControl_.HasObserver(&this->notifier_));

            return;
        }

        assert(this->upstreamControl_.HasObserver(&this->notifier_));
        this->upstreamControl_.Disconnect(&this->notifier_);
        this->notifier_.ClearConnections();
        this->observer_ = nullptr;
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

    const ControlType & GetControl() const
    {
        return this->upstreamControl_;
    }

    ControlType & GetControl()
    {
        return this->upstreamControl_;
    }

    Observer * GetObserver()
    {
        return this->observer_;
    }

    void Connect(Observer *observer, Callable callable)
    {
        assert(observer != nullptr);

        if (this->observer_)
        {
            // Already connected.
            assert(this->notifier_.HasObserver(this->observer_));
            this->notifier_.Disconnect(this->observer_);
        }

        this->observer_ = observer;

        if (!this->upstreamControl_.HasObserver(&this->notifier_))
        {
            // Connect ourselves to the upstream.

            this->upstreamControl_.Connect(
                &this->notifier_,
                &Notifier::OnUpstream);
        }

        this->notifier_.Connect(this->observer_, callable);

        PEX_NAME(fmt::format("Terminus_({})", static_cast<void *>(observer)));
        PEX_LINK_OBSERVER(this, observer);
    }

    bool HasConnection() const
    {
        return this->notifier_.HasConnection();
    }

    std::vector<size_t> GetNotificationOrderChain()
    {
        return this->upstreamControl_
            .GetNotificationOrderChain(&this->notifier_);
    }

private:
    Observer *observer_;
    Notifier notifier_;

protected:
    ControlType upstreamControl_;
};


template<typename T, typename Enable = void>
struct ExtractModel_
{
    using Type = typename T::Model;
};


template<typename T>
struct ExtractModel_
<
    T,
    std::enable_if_t<IsModel<T>>
>
{
    using Type = T;
};


template<typename T>
using ExtractModel = typename ExtractModel_<T>::Type;


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
    // using Upstream = typename PromoteControl<Upstream_>::Type;
    using Upstream = Upstream_;
    using Type = typename Upstream::Type;
    using Filter = typename Upstream::Filter;
    using Access = typename Upstream::Access;

    using Model = ExtractModel<Upstream>;

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

    void Notify()
    {
        static_cast<Derived *>(this)->upstreamControl_.Notify();
    }

protected:
    void SetWithoutNotify_(Argument<Type> value)
    {
        detail::AccessReference(
            static_cast<Derived *>(this)->upstreamControl_)
                .SetWithoutNotify(value);
    }

private:
    const Model & GetModel_() const
    {
        assert(this->HasModel());
        return this->upstreamControl_.GetModel_();
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
    // using Upstream = typename PromoteControl<Upstream_>::Type;
    using Upstream = Upstream_;
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

    // using UpstreamControl = typename PromoteControl<Upstream_>::Type;
    using UpstreamControl = Upstream_;

    friend class EndpointVector<Terminus>::Wrapper;

private:

    Terminus(Terminus &&other) noexcept
        :
        Base(std::move(other))
    {

    }

    Terminus & operator=(Terminus &&other) noexcept
    {
        this->Base::operator=(std::move(other));

        return *this;
    }
};


template<typename ...T>
struct IsTerminus_: std::false_type {};

template<typename ...T>
struct IsTerminus_<Terminus<T...>>: std::true_type {};

template<typename ...T>
inline constexpr bool IsTerminus = IsTerminus_<T...>::value;


} // end namespace pex
