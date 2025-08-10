#pragma once


#include <optional>
#include <fields/core.h>
#include "pex/detail/group_connect.h"
#include "pex/terminus.h"
#include "pex/range.h"
#include "pex/select.h"
#include "pex/list.h"
#include "pex/detail/list_connect.h"


namespace pex
{


namespace detail
{


template<typename T, typename Enable = void>
struct MakeConnector_
{
    template<typename Observer>
    using Type = Terminus<Observer, T>;
};


template<typename T>
struct MakeConnector_<T, std::enable_if_t<IsGroupNode<T>>>
{
    template<typename Observer>
    using Type = detail::GroupConnect<Observer, T>;
};


template<typename T>
struct MakeConnector_<T, std::enable_if_t<IsRange<T>>>
{
    template<typename Observer>
    using Type = RangeTerminus<Observer, T>;
};


template<typename T>
struct MakeConnector_<T, std::enable_if_t<IsSelect<T>>>
{
    template<typename Observer>
    using Type = SelectTerminus<Observer, T>;
};


template<typename T>
struct MakeConnector_<T, std::enable_if_t<::pex::IsListNode<T>>>
{
    template<typename Observer>
    using Type = ListConnect<Observer, T>;
};


} // end namespace detail


template<typename Observer, typename Upstream>
struct MakeConnector
    : public detail::MakeConnector_<Upstream>::template Type<Observer>
{
    using Base =
        typename detail::MakeConnector_<Upstream>::template Type<Observer>;

    using Base::Base;

    // Defining constructors here so that Observer and Upstream types can be
    // deduced.
    MakeConnector(Upstream &object)
        :
        Base(object)
    {

    }

    MakeConnector(
        Observer *observer,
        Upstream &object,
        typename Base::Callable callable)
        :
        Base(observer, object, callable)
    {

    }
};


template<typename Observer, typename Upstream>
class Endpoint_
{
    static_assert(
        !std::is_same_v<Observer, void>,
        "Designed for typed observers");

public:
    using Connector = MakeConnector<Observer, Upstream>;
    using UpstreamControl = typename Connector::UpstreamControl;
    using Callable = typename Connector::Callable;

    Endpoint_()
        :
        observer_(nullptr),
        connector()
    {
        PEX_NAME("Endpoint");
        PEX_MEMBER(connector);
    }

    Endpoint_(Observer *observer)
        :
        observer_(observer),
        connector()
    {
        PEX_NAME(fmt::format("Endpoint ({})", pex::LookupPexName(observer)));
        PEX_MEMBER(connector);
    }

    Endpoint_(Observer *observer, UpstreamControl upstream)
        :
        observer_(observer),
        connector(upstream)
    {
        PEX_NAME(fmt::format("Endpoint ({})", pex::LookupPexName(observer)));
        PEX_MEMBER(connector);
    }

    Endpoint_(Observer *observer, UpstreamControl upstream, Callable callable)
        :
        observer_(observer),
        connector(observer, upstream, callable)
    {
        PEX_NAME(fmt::format("Endpoint ({})", pex::LookupPexName(observer)));
        PEX_MEMBER(connector);
    }

    Endpoint_(Observer *observer, typename UpstreamControl::Upstream &model)
        :
        observer_(observer),
        connector(UpstreamControl(model))
    {
        PEX_NAME(fmt::format("Endpoint ({})", pex::LookupPexName(observer)));
        PEX_MEMBER(connector);
    }

    Endpoint_(
        Observer *observer,
        typename UpstreamControl::Upstream &model,
        Callable callable)
        :
        observer_(observer),
        connector(observer, UpstreamControl(model), callable)
    {
        PEX_NAME(fmt::format("Endpoint ({})", pex::LookupPexName(observer)));
        PEX_MEMBER(connector);
    }

    Endpoint_(Observer *observer, const Endpoint_ &other)
        :
        observer_(observer),
        connector(observer, other.connector)
    {
        PEX_NAME(fmt::format("Endpoint ({})", pex::LookupPexName(observer)));
        PEX_MEMBER(connector);
    }

    Endpoint_ & Assign(Observer *observer, const Endpoint_ &other)
    {
        this->observer_ = observer;
        this->connector.Assign(observer, other.connector);

        return *this;
    }

    Endpoint_(Endpoint_ &&other)
        :
        observer_(other.observer_),
        connector(other.connector)
    {
        PEX_NAME(fmt::format("Endpoint ({})", pex::LookupPexName(observer_)));
        PEX_MEMBER(connector);
    }

    ~Endpoint_()
    {
        PEX_CLEAR_NAME(this);
        PEX_CLEAR_NAME(&this->connector);
    }

    Endpoint_ & operator=(Endpoint_ &&other)
    {
        this->observer_ = other.observer_;
        this->connector.Assign(this->observer_, other.connector);

        return *this;
    }

    void ConnectUpstream(UpstreamControl upstream, Callable callable)
    {
        this->connector.Emplace(this->observer_, upstream, callable);
    }

    void Connect(Callable callable)
    {
        this->connector.Connect(this->observer_, callable);
    }

    void Disconnect(Observer *)
    {
        this->connector.Disconnect();
    }

    void Disconnect()
    {
        this->connector.Disconnect();
    }

    explicit operator UpstreamControl () const
    {
        return static_cast<UpstreamControl>(this->connector);
    }

protected:
    Observer *observer_;

public:
    Connector connector;
};


template<typename Observer, typename Upstream>
class ValueEndpoint_: public Endpoint_<Observer, Upstream>
{
public:
    using Base = Endpoint_<Observer, Upstream>;
    using UpstreamControl = typename Base::UpstreamControl;
    using Type = typename UpstreamControl::Type;

    using Base::Base;

    Type Get() const
    {
        return this->connector.Get();
    }

    void Set(Argument<Type> value)
    {
        this->connector.Set(value);
    }
};


template<typename Observer, typename Upstream, typename = void>
struct ChooseEndpoint_
{
    using Type = ValueEndpoint_<Observer, Upstream>;
};


template<typename Observer, typename Upstream>
struct ChooseEndpoint_
<
    Observer,
    Upstream,
    std::enable_if_t<IsSignal<Upstream>>
>
{
    using Type = Endpoint_<Observer, Upstream>;
};


template<typename Observer, typename Upstream>
using Endpoint = typename ChooseEndpoint_<Observer, Upstream>::Type;


/***** EndpointSelector *****/
template<typename T>
struct EndpointSelector_
{
    template<typename Observer>
    using Type = Endpoint
    <
        Observer,
        typename pex::detail::ControlSelector_<T>::Type
    >;
};


template<typename Observer>
struct EndpointSelector
{
    template<typename T>
    using Type = typename EndpointSelector_<T>::template Type<Observer>;
};


template<typename Observer, typename EndpointMember, typename MemberControl>
void AssignEndpoints(
    Observer *observer,
    EndpointMember &endpoint,
    MemberControl &control)
{
    endpoint = EndpointMember(observer, control);
}


template
<
    typename Observer,
    typename EndpointGroup,
    typename Control
>
void InitializeEndpoints(
    Observer *observer,
    EndpointGroup &endpointGroup,
    const Control &control)
{
    static_assert(
        fields::HasFields<EndpointGroup>,
        "EndpointGroup is expected to have member 'fields'");

    static_assert(
        fields::HasFields<Control>,
        "Control is expected to have member 'fields'");

    auto initializer = [observer, &endpointGroup, &control](
        auto endpointField,
        auto controlField)
    {
        AssignEndpoints(
            observer,
            endpointGroup.*(endpointField.member),
            control.*(controlField.member));
    };

    jive::ZipApply(
        initializer,
        EndpointGroup::fields,
        Control::fields);
}


/*
 * EndpointGroup is not recursive.
 * It only creates endpoints for the top-level members.
 */
template
<
    typename Observer,
    typename Control
>
class EndpointGroup
    :
    public Control::
        template GroupTemplate<EndpointSelector<Observer>::template Type>
{
public:
    using Callable = typename Endpoint<Observer, Control>::Callable;

    EndpointGroup() = delete;

    EndpointGroup(Observer *observer, const Control &control_)
        :
        control(control_),
        group(observer, control_)
    {
        InitializeEndpoints(observer, *this, control);
    }

    EndpointGroup(
        Observer *observer,
        const Control &control_,
        Callable callable)
        :
        control(control_),
        group(observer, control_, callable)
    {
        InitializeEndpoints(observer, *this, control);
    }

    Control control;
    Endpoint<Observer, Control> group;
};


namespace detail
{


template<typename First_, typename ... TheRest_>
struct ArgsHelper
{
    using First = First_;
    using TheRest = std::tuple<TheRest_...>;
};


template<typename T>
struct SignatureHelper {};


template<typename T, typename U, typename ...Args_>
struct SignatureHelper<T (U::*)(Args_...)>
{
    using Return = T;
    using Class = U;
    using Args = std::tuple<std::decay_t<Args_>...>;
    using First = std::decay_t<std::tuple_element_t<0, Args>>;
    using TheRest = typename ArgsHelper<std::decay_t<Args_>...>::TheRest;
};


template<typename T>
struct Signature: SignatureHelper<std::remove_cv_t<T>> {};


template<typename Signature, typename Upstream, typename Enable = void>
struct BoundArgs_
{
    using Type = typename Signature::TheRest;
};

template<typename Signature, typename Upstream>
struct BoundArgs_
<
    Signature,
    Upstream,
    std::enable_if_t<IsSignal<Upstream>>
>
{
    // Signals have no args of their own.
    // All of the function args are the bound args.
    using Type = typename Signature::Args;
};


template<typename Signature, typename Upstream>
using BoundArgs = typename BoundArgs_<Signature, Upstream>::Type;


template<typename Upstream, typename Enable = void>
struct InternalType_
{
    using Type = int;
};

template<typename Upstream>
struct InternalType_
<
    Upstream,
    std::enable_if_t<!IsSignal<Upstream>>
>
{
    using Type = pex::Argument<typename Upstream::Type>;
};

template<typename Upstream>
using InternalType = typename InternalType_<Upstream>::Type;


} // end namespace detail


template<typename Upstream, typename MemberFunction>
class BoundEndpoint: Separator
{
public:
    using Signature = detail::Signature<MemberFunction>;
    using Observer = typename Signature::Class;
    using Args = detail::BoundArgs<Signature, Upstream>;

    using InternalEndpoint = Endpoint<BoundEndpoint, Upstream>;
    using Connector = typename InternalEndpoint::Connector;
    using UpstreamControl = typename InternalEndpoint::UpstreamControl;

    static constexpr auto observerName = "BoundEndpoint";

    BoundEndpoint()
        :
        endpoint(PEX_THIS("BoundEndpoint")),
        observer_(nullptr),
        memberFunction_(),
        args_()
    {
        PEX_MEMBER(endpoint);
    }

    BoundEndpoint(Observer *observer)
        :
        endpoint(PEX_THIS("BoundEndpoint")),
        observer_(observer),
        memberFunction_(),
        args_()
    {
        PEX_NAME(
            fmt::format(
                "BoundEndpoint ({})",
                pex::LookupPexName(observer_)));

        PEX_MEMBER(endpoint);
    }

    BoundEndpoint(Observer *observer, UpstreamControl upstream)
        :
        endpoint(PEX_THIS("BoundEndpoint"), upstream),
        observer_(observer),
        memberFunction_(),
        args_()
    {
        PEX_NAME(
            fmt::format(
                "BoundEndpoint ({})",
                pex::LookupPexName(observer_)));

        PEX_MEMBER(endpoint);
    }

    // Uses helper type ...T to allow forwarding the bound arguments.
    template<typename ...T>
    BoundEndpoint(
        Observer *observer,
        UpstreamControl upstream,
        MemberFunction memberFunction,
        T &&...args)
        :
        endpoint(PEX_THIS("BoundEndpoint"), upstream),
        observer_(observer),
        memberFunction_(memberFunction),
        args_(std::make_tuple(std::forward<T>(args)...))
    {
        this->ConnectInternal_();

        PEX_NAME(
            fmt::format(
                "BoundEndpoint ({})",
                pex::LookupPexName(observer_)));

        PEX_MEMBER(endpoint);
    }

    BoundEndpoint(Observer *observer, typename UpstreamControl::Upstream &model)
        :
        endpoint(
            PEX_THIS("BoundEndpoint"),
            UpstreamControl(model)),
        observer_(observer),
        memberFunction_(),
        args_()
    {
        PEX_NAME(
            fmt::format(
                "BoundEndpoint ({})",
                pex::LookupPexName(observer_)));

        PEX_MEMBER(endpoint);
    }

    ~BoundEndpoint()
    {
        PEX_CLEAR_NAME(this);
        PEX_CLEAR_NAME(&this->endpoint);
    }

    void ConnectInternal_()
    {
        if constexpr (IsSignal<Upstream>)
        {
            this->endpoint.Connect(&BoundEndpoint::OnInternalSignal_);
        }
        else
        {
            this->endpoint.Connect(&BoundEndpoint::OnInternal_);
        }

        PEX_NAME(
            fmt::format(
                "BoundEndpoint ({})",
                pex::LookupPexName(observer_)));
    }

    // Uses helper type ...T to allow forwarding the bound arguments.
    template<typename ...T>
    BoundEndpoint(
        Observer *observer,
        typename UpstreamControl::Upstream &model,
        MemberFunction memberFunction,
        T &&...args)
        :
        endpoint(
            PEX_THIS("BoundEndpoint"),
            UpstreamControl(model)),
        observer_(observer),
        memberFunction_(memberFunction),
        args_(std::make_tuple(std::forward<T>(args)...))
    {
        PEX_MEMBER(endpoint);
        this->ConnectInternal_();
    }

    BoundEndpoint(Observer *observer, const BoundEndpoint &other)
        :
        endpoint(
            PEX_THIS("BoundEndpoint"),
            other.endpoint),
        observer_(observer),
        memberFunction_(other.memberFunction_),
        args_(other.args_)
    {
        PEX_NAME(
            fmt::format(
                "BoundEndpoint ({})",
                pex::LookupPexName(observer_)));

        PEX_MEMBER(endpoint);
    }

    BoundEndpoint & Assign(Observer *observer, const BoundEndpoint &other)
    {
        this->endpoint.Assign(this, other.endpoint);
        this->observer_ = observer;
        this->memberFunction_ = other.memberFunction_;
        this->args_ = other.args_;

        return *this;
    }

    BoundEndpoint(BoundEndpoint &&other)
        :
        endpoint(
            PEX_THIS("BoundEndpoint"),
            other.endpoint),
        observer_(other.observer_),
        memberFunction_(other.memberFunction_),
        args_(other.args_)
    {
        PEX_NAME(
            fmt::format(
                "BoundEndpoint ({})",
                pex::LookupPexName(observer_)));

        PEX_MEMBER(endpoint);
    }

    BoundEndpoint & operator=(BoundEndpoint &&other)
    {
        this->endpoint.Assign(this, other.endpoint);
        this->observer_ = other.observer_;
        this->memberFunction_ = other.memberFunction_;
        this->args_ = other.args_;

        return *this;
    }

    // Uses helper type ...T to allow forwarding the bound arguments.
    template<typename ...T>
    void ConnectUpstream(
        UpstreamControl upstream,
        MemberFunction memberFunction,
        T &&...args)
    {
        if constexpr (IsSignal<Upstream>)
        {
            this->endpoint.ConnectUpstream(
                upstream,
                &BoundEndpoint::OnInternalSignal_);
        }
        else
        {
            this->endpoint.ConnectUpstream(
                upstream,
                &BoundEndpoint::OnInternal_);
        }

        this->memberFunction_ = memberFunction;
        this->args_ = std::make_tuple(std::forward<T>(args)...);
    }

    // Uses helper type ...T to allow forwarding the bound arguments.
    template<typename ...T>
    void Connect(MemberFunction memberFunction, T &&...args)
    {
        this->ConnectInternal_();
        this->memberFunction_ = memberFunction;
        this->args_ = std::make_tuple(std::forward<T>(args)...);
    }

    void Disconnect(Observer *)
    {
        this->endpoint.Disconnect();
    }

    void Disconnect()
    {
        this->endpoint.Disconnect();
    }

    explicit operator UpstreamControl () const
    {
        return static_cast<UpstreamControl>(this->endpoint);
    }

private:
    void OnInternalSignal_()
    {
        std::apply(
            [this](auto &...args)
            {
                (this->observer_->*this->memberFunction_)((args)...);
            },
            this->args_);
    }

    void OnInternal_(detail::InternalType<Upstream> value)
    {
        std::apply(
            [this, &value](auto &...args)
            {
                (this->observer_->*this->memberFunction_)(value, (args)...);
            },
            this->args_);
    }

private:
    InternalEndpoint endpoint;
    Observer *observer_;
    MemberFunction memberFunction_;
    Args args_;
};


} // end namespace pex
