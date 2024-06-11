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
struct MakeConnector_<T, std::enable_if_t<::pex::IsList<T>>>
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
    MakeConnector(
        Observer *observer,
        Upstream &object)
        :
        Base(observer, object)
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

    }

    Endpoint_(Observer *observer)
        :
        observer_(observer),
        connector()
    {

    }

    Endpoint_(Observer *observer, UpstreamControl upstream)
        :
        observer_(observer),
        connector(observer, upstream)
    {

    }

    Endpoint_(Observer *observer, UpstreamControl upstream, Callable callable)
        :
        observer_(observer),
        connector(observer, upstream, callable)
    {

    }

    Endpoint_(Observer *observer, typename UpstreamControl::Upstream &model)
        :
        observer_(observer),
        connector(observer, UpstreamControl(model))
    {

    }

    Endpoint_(
        Observer *observer,
        typename UpstreamControl::Upstream &model,
        Callable callable)
        :
        observer_(observer),
        connector(observer, UpstreamControl(model), callable)
    {

    }

    Endpoint_(Observer *observer, const Endpoint_ &other)
        :
        observer_(observer),
        connector(observer, other.connector)
    {

    }

    Endpoint_ & Assign(Observer *observer, const Endpoint_ &other)
    {
        this->observer_ = observer;
        this->connector.Assign(observer, other.connector);

        return *this;
    }

    Endpoint_ & operator=(Endpoint_ &&other)
    {
        this->observer_ = other.observer_;
        this->connector.Assign(this->observer_, other.connector);

        return *this;
    }

    void ConnectUpstream(UpstreamControl upstream, Callable callable)
    {
        this->connector.Assign(
            this->observer_,
            Connector(this->observer_, upstream, callable));
    }

    void Connect(Callable callable)
    {
        this->connector.Connect(callable);
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



} // end namespace pex
