#pragma once


#include <optional>
#include <fields/core.h>
#include "pex/detail/group_connect.h"
#include "pex/terminus.h"
#include "pex/range.h"
#include "pex/select.h"


namespace pex
{


namespace detail
{


template<typename T, typename Enable = void>
struct MakeConnector_
{
    template<typename Observer, typename Upstream>
    using Type = Terminus<Observer, Upstream>;
};


template<typename T>
struct MakeConnector_<T, std::enable_if_t<IsAccessor<T>>>
{
    template<typename Observer, typename GroupControl>
    using Type = detail::GroupConnect<Observer, GroupControl>;
};


template<typename T>
struct MakeConnector_<T, std::enable_if_t<IsRange<T>>>
{
    template<typename Observer, typename Upstream>
    using Type = RangeTerminus<Observer, Upstream>;
};


template<typename T>
struct MakeConnector_<T, std::enable_if_t<IsSelect<T>>>
{
    template<typename Observer, typename Upstream>
    using Type = SelectTerminus<Observer, Upstream>;
};

#if 0
template<typename T>
struct MakeConnector_<T, std::enable_if_t<!IsAccessor<T>>>
{
    template<typename Observer, typename Upstream>
    using Type = Terminus<Observer, Upstream>;
};
#endif


} // end namespace detail


template<typename Observer, typename Upstream>
struct MakeConnector
    : public detail::MakeConnector_<Upstream>::template Type<Observer, Upstream>
{
    using Base = typename detail::MakeConnector_<Upstream>
        ::template Type<Observer, Upstream>;

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
struct Endpoint
{
    static_assert(
        !std::is_same_v<Observer, void>,
        "Designed for typed observers");

public:
    using Connector = MakeConnector<Observer, Upstream>;
    using Callable = typename Connector::Callable;

    Endpoint()
        :
        observer_(nullptr),
        connector_()
    {

    }

    Endpoint(Observer *observer)
        :
        observer_(observer),
        connector_()
    {

    }

    Endpoint(Observer *observer, Upstream upstream)
        :
        observer_(observer),
        connector_(observer, upstream)
    {

    }

    Endpoint(Observer *observer, Upstream upstream, Callable callable)
        :
        observer_(observer),
        connector_(observer, upstream, callable)
    {

    }

    Endpoint(Observer *observer, typename Upstream::Upstream &model)
        :
        observer_(observer),
        connector_(observer, Upstream(model))
    {

    }

    Endpoint(Observer *observer, typename Upstream::Upstream &model, Callable callable)
        :
        observer_(observer),
        connector_(observer, Upstream(model), callable)
    {

    }

    Endpoint(const Endpoint &other)
        :
        observer_(other.observer_),
        connector_(other.observer_, other.connector_)
    {

    }

    Endpoint & operator=(const Endpoint &other)
    {
        this->observer_ = other.observer_;
        this->connector_.Assign(other.observer_, other.connector_);
        return *this;
    }

    void ConnectUpstream(Upstream upstream, Callable callable)
    {
        this->connector_.Assign(
            this->observer_,
            Connector(this->observer_, upstream, callable));
    }

    void Connect(Callable callable)
    {
        this->connector_.Connect(callable);
    }

    const Upstream & GetUpstream() const
    {
        return this->connector_.GetUpstream();
    }

    explicit operator Upstream () const
    {
        return static_cast<Upstream>(this->connector_);
    }

    Observer *observer_;
    Connector connector_;
};


template<typename Observer, typename Upstream>
struct EndpointControl: public Endpoint<Observer, Upstream>
{
public:
    using Base = Endpoint<Observer, Upstream>;
    using Base::Base;

    using Callable = typename Base::Callable;

    EndpointControl(Observer *observer, Upstream upstream)
        :
        Base(observer, upstream),
        control(upstream)
    {

    }

    EndpointControl(Observer *observer, Upstream upstream, Callable callable)
        :
        Base(observer, upstream, callable),
        control(upstream)
    {

    }

    EndpointControl(
        Observer *observer,
        typename Upstream::Upstream &model)
        :
        Base(observer, model),
        control(model)
    {

    }

    EndpointControl(
        Observer *observer,
        typename Upstream::Upstream &model,
        Callable callable)
        :
        Base(observer, model, callable),
        control(model)
    {

    }

    void ConnectUpstream(Upstream upstream, Callable callable)
    {
        this->Base::ConnectUpstream(upstream, callable);
        this->control = upstream;
    }

    Upstream control;
};



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
    EndpointGroup() = delete;

    EndpointGroup(Observer *observer, const Control &control_)
        :
        control(control_),
        group(observer, control_)
    {
        InitializeEndpoints(observer, *this, control);
    }

    Control control;
    Endpoint<Observer, Control> group;
};



} // end namespace pex
