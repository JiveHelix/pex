#pragma once


#include "pex/model_value.h"
#include "pex/control_value.h"
#include "pex/range.h"
#include "pex/select.h"
#include "pex/signal.h"
#include "pex/interface.h"
#include "pex/model_wrapper.h"
#include "pex/control_wrapper.h"


namespace pex
{


namespace detail
{


template<typename RangeMaker>
struct RangeTypes
{
    using Type = typename RangeMaker::Type;

    using Model =
        ::pex::model::Range
        <
            Type,
            typename RangeMaker::Minimum,
            typename RangeMaker::Maximum,
            RangeMaker::template Value
        >;

    template<typename Upstream>
    using Control = ::pex::control::Range<Upstream>;

    using Mux = ::pex::control::RangeMux<Model>;
    using Follow = ::pex::control::RangeFollow<Mux>;
};


template<typename SelectMaker>
struct SelectTypes
{
    using SelectType = typename SelectMaker::Type;
    using Type = typename SelectType::Type;

    using Model =
        pex::model::Select
        <
            Type,
            SelectType,
            typename SelectMaker::Access
        >;

    template<typename Upstream>
    using Control = ::pex::control::Select<Upstream>;

    using Mux = ::pex::control::SelectMux<Model>;
    using Follow = ::pex::control::SelectFollow<Mux>;
};


/***** ModelSelector *****/
template<typename T, typename = void>
struct ModelSelector_
{
    using Type = model::Value<T>;
};


template<typename T>
struct ModelSelector_
<
    T,
    std::enable_if_t<jive::IsValueContainer<T>::value>
>
{
    using Type = model::ValueContainer<T>;
};

template<typename T>
struct ModelSelector_
<
    T,
    std::enable_if_t<jive::IsKeyValueContainer<T>::value>
>
{
    using Type = model::KeyValueContainer<T>;
};

template<typename T>
struct ModelSelector_<T, std::enable_if_t<IsMakeSignal<T>>>
{
    using Type = model::Signal;
};

template<typename T>
struct ModelSelector_<T, std::enable_if_t<IsFiltered<T>>>
{
    using Type =
        model::Value_
        <
            typename T::Type,
            typename T::ModelFilter,
            typename T::Access
        >;
};

template<typename T>
struct ModelSelector_<T, std::enable_if_t<IsMakeRange<T>>>
{
    using Type = typename RangeTypes<T>::Model;
};

template<typename T>
struct ModelSelector_<T, std::enable_if_t<IsMakeSelect<T>>>
{
    using Type = typename SelectTypes<T>::Model;
};

template<typename T>
struct ModelSelector_<T, std::enable_if_t<IsDefineNodes<T>>>
{
    using Type = typename T::Model;
};

template<typename T>
struct ModelSelector_<T, std::enable_if_t<(IsGroup<T>)>>
{
    using Type = typename T::Model;
};

template<typename T>
struct ModelSelector_<T, std::enable_if_t<(IsDerivedGroup<T>)>>
{
    using Type = typename T::Model;
};

template<typename T>
struct ModelSelector_<T, std::enable_if_t<IsList<T>>>
{
    using Type = typename T::Model;
};


template<typename T>
struct ModelSelector_<T, std::enable_if_t<IsMakePoly<T>>>
{
    using Type = poly::ModelWrapperTemplate<typename T::Supers>;
};

/***** ControlSelector *****/
template<typename T, typename = void>
struct ControlSelector_
{
    using Type = control::Value<typename ModelSelector_<T>::Type>;
};


template<typename T>
struct ControlSelector_
<
    T,
    std::enable_if_t<jive::IsValueContainer<T>::value>
>
{
    using Type = control::ValueContainer<typename ModelSelector_<T>::Type>;
};

template<typename T>
struct ControlSelector_
<
    T,
    std::enable_if_t<jive::IsKeyValueContainer<T>::value>
>
{
    using Type = control::KeyValueContainer<typename ModelSelector_<T>::Type>;
};

template<typename T>
struct ControlSelector_<T, std::enable_if_t<IsMakeSignal<T>>>
{
    using Type = control::Signal<typename ModelSelector_<T>::Type>;
};

template<typename T>
struct ControlSelector_<T, std::enable_if_t<IsFiltered<T>>>
{
    using Type = control::Value_
    <
        typename ModelSelector_<T>::Type,
        NoFilter,
        typename T::Access
    >;
};

template<typename T>
struct ControlSelector_<T, std::enable_if_t<IsMakeRange<T>>>
{
    using Type = typename RangeTypes<T>
        ::template Control<typename ModelSelector_<T>::Type>;
};

template<typename T>
struct ControlSelector_<T, std::enable_if_t<IsMakeSelect<T>>>
{
    using Type = typename SelectTypes<T>
        ::template Control<typename ModelSelector_<T>::Type>;
};

template<typename T>
struct ControlSelector_<T, std::enable_if_t<IsDefineNodes<T>>>
{
    using Type = typename T::template Control<typename ModelSelector_<T>::Type>;
};

template<typename T>
struct ControlSelector_<T, std::enable_if_t<(IsGroup<T>)>>
{
    using Type =
        typename T::template Control<typename ModelSelector_<T>::Type>;
};

template<typename T>
struct ControlSelector_<T, std::enable_if_t<(IsDerivedGroup<T>)>>
{
    using Type =
        typename T::Control;
};

template<typename T>
struct ControlSelector_<T, std::enable_if_t<IsList<T>>>
{
    using Type =
        typename T::template Control<typename ModelSelector_<T>::Type>;
};

template<typename T>
struct ControlSelector_<T, std::enable_if_t<IsMakePoly<T>>>
{
    using Type =
        poly::ControlWrapperTemplate
        <
            typename ModelSelector_<T>::Type,
            typename T::Supers
        >;
};


/***** MuxSelector *****/
template<typename T, typename = void>
struct MuxSelector_
{
    using Type = control::Mux<typename ModelSelector_<T>::Type>;
};


template<typename T>
struct MuxSelector_
<
    T,
    std::enable_if_t<jive::IsValueContainer<T>::value>
>
{
    using Type = control::ValueContainerMux<typename ModelSelector_<T>::Type>;
};

template<typename T>
struct MuxSelector_
<
    T,
    std::enable_if_t<jive::IsKeyValueContainer<T>::value>
>
{
    using Type = control::KeyValueContainerMux
        <
            typename ModelSelector_<T>::Type
        >;
};

template<typename T>
struct MuxSelector_<T, std::enable_if_t<IsMakeSignal<T>>>
{
    using Type = control::SignalMux;
};

template<typename T>
struct MuxSelector_<T, std::enable_if_t<IsFiltered<T>>>
{
    using Type = control::Value_
    <
        typename ControlSelector_<T>::Type,
        NoFilter,
        typename T::Access
    >;
};

template<typename T>
struct MuxSelector_<T, std::enable_if_t<IsMakeRange<T>>>
{
    using Type = typename RangeTypes<T>::Mux;
};

template<typename T>
struct MuxSelector_<T, std::enable_if_t<IsMakeSelect<T>>>
{
    using Type = typename SelectTypes<T>::Mux;
};

template<typename T>
struct MuxSelector_<T, std::enable_if_t<IsDefineNodes<T>>>
{
    using Type = typename T::Mux;
};

template<typename T>
struct MuxSelector_<T, std::enable_if_t<(IsGroup<T>)>>
{
    using Type = typename T::Mux;
};

template<typename T>
struct MuxSelector_<T, std::enable_if_t<(IsDerivedGroup<T>)>>
{
    using Type = typename T::Mux;
};

template<typename T>
struct MuxSelector_<T, std::enable_if_t<IsList<T>>>
{
    using Type = typename T::Mux;
};

template<typename T>
struct MuxSelector_<T, std::enable_if_t<IsMakePoly<T>>>
{
    using Type = typename ControlSelector_<T>::Type;
};


/***** FollowSelector *****/
template<typename T, typename = void>
struct FollowSelector_
{
    using Type = control::Value<typename MuxSelector_<T>::Type>;
};


template<typename T>
struct FollowSelector_
<
    T,
    std::enable_if_t<jive::IsValueContainer<T>::value>
>
{
    using Type = control::ValueContainer<typename MuxSelector_<T>::Type>;
};

template<typename T>
struct FollowSelector_
<
    T,
    std::enable_if_t<jive::IsKeyValueContainer<T>::value>
>
{
    using Type = control::KeyValueContainer<typename MuxSelector_<T>::Type>;
};

template<typename T>
struct FollowSelector_<T, std::enable_if_t<IsMakeSignal<T>>>
{
    using Type = control::Signal<typename MuxSelector_<T>::Type>;
};

template<typename T>
struct FollowSelector_<T, std::enable_if_t<IsFiltered<T>>>
{
    using Type = control::Value_
    <
        typename MuxSelector_<T>::Type,
        NoFilter,
        typename T::Access
    >;
};

template<typename T>
struct FollowSelector_<T, std::enable_if_t<IsMakeRange<T>>>
{
    using Type = typename RangeTypes<T>::Follow;
};

template<typename T>
struct FollowSelector_<T, std::enable_if_t<IsMakeSelect<T>>>
{
    using Type = typename SelectTypes<T>::Follow;
};

template<typename T>
struct FollowSelector_<T, std::enable_if_t<IsDefineNodes<T>>>
{
    using Type = typename T::Follow;
};

template<typename T>
struct FollowSelector_<T, std::enable_if_t<(IsGroup<T>)>>
{
    using Type = typename T::Follow;
};

template<typename T>
struct FollowSelector_<T, std::enable_if_t<(IsDerivedGroup<T>)>>
{
    using Type = typename T::Follow;
};

template<typename T>
struct FollowSelector_<T, std::enable_if_t<IsList<T>>>
{
    using Type = typename T::Follow;
};

template<typename T>
struct FollowSelector_<T, std::enable_if_t<IsMakePoly<T>>>
{
    using Type = typename ControlSelector_<T>::Type;
};


} // end namespace detail


template<typename T>
using ModelSelector = typename detail::ModelSelector_<T>::Type;


template<typename T>
using ControlSelector = typename detail::ControlSelector_<T>::Type;


template<typename T>
using MuxSelector = typename detail::MuxSelector_<T>::Type;


template<typename T>
using FollowSelector = typename detail::FollowSelector_<T>::Type;


} // end namespace pex
