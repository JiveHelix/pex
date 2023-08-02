#pragma once


#include "pex/model_value.h"
#include "pex/control_value.h"
#include "pex/range.h"
#include "pex/select.h"
#include "pex/signal.h"
#include "pex/interface.h"


namespace pex
{

namespace detail
{


template<typename RangeMaker>
struct RangeTypes
{
    using Type = typename RangeMaker::Type;

    using Model =
        pex::model::Range
        <
            Type,
            typename RangeMaker::Minimum,
            typename RangeMaker::Maximum,
            RangeMaker::template Value
        >;

    using Control = pex::control::Range<Model>;

    template<typename Observer>
    using Terminus = RangeTerminus<Observer, Model>;
};


template<typename SelectMaker>
struct SelectTypes
{
    using Type = typename SelectMaker::Type;

    using Model =
        pex::model::Select
        <
            Type,
            typename SelectMaker::Access
        >;

    using Control = pex::control::Select<Model>;

    template<typename Observer>
    using Terminus = SelectTerminus<Observer, Model>;
};


/***** ModelSelector *****/
template<typename T, typename = void>
struct ModelSelector_
{
    using Type = model::Value<T>;
};

template<typename T>
struct ModelSelector_<T, std::enable_if_t<IsMakeSignal<T>>>
{
    using Type = model::Signal;
};

template<typename T>
struct ModelSelector_<T, std::enable_if_t<IsFiltered<T>>>
{
    using Type = model::Value_<typename T::Type, typename T::ModelFilter>;
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
struct ModelSelector_<T, std::enable_if_t<IsMakeCustom<T>>>
{
    using Type = typename T::Custom;
};

template<typename T>
struct ModelSelector_<T, std::enable_if_t<IsMakeGroup<T>>>
{
    using Type = typename T::Model;
};


/***** ControlSelector *****/
template<typename T, typename = void>
struct ControlSelector_
{
    using Type = control::Value<typename ModelSelector_<T>::Type>;
};

template<typename T>
struct ControlSelector_<T, std::enable_if_t<IsMakeSignal<T>>>
{
    using Type = control::Signal<>;
};

template<typename T>
struct ControlSelector_<T, std::enable_if_t<IsFiltered<T>>>
{
    using Type = control::Value_
    <
        typename ModelSelector_<T>::Type,
        NoFilter,
        typename T::ControlAccess
    >;
};

template<typename T>
struct ControlSelector_<T, std::enable_if_t<IsMakeRange<T>>>
{
    using Type = typename RangeTypes<T>::Control;
};

template<typename T>
struct ControlSelector_<T, std::enable_if_t<IsMakeSelect<T>>>
{
    using Type = typename SelectTypes<T>::Control;
};

template<typename T>
struct ControlSelector_<T, std::enable_if_t<IsMakeCustom<T>>>
{
    using Type = typename T::Control;
};

template<typename T>
struct ControlSelector_<T, std::enable_if_t<IsMakeGroup<T>>>
{
    using Type = typename T::Control;
};

#if 0
/***** TerminusSelector *****/
template<typename T, typename = void>
struct TerminusSelector_
{
    template<typename Observer>
    using Type = Terminus
    <
        Observer,
        typename ControlSelector_<T>::Type
    >;
};


template<typename T>
struct TerminusSelector_<T, std::enable_if_t<IsMakeRange<T>>>
{
    template<typename Observer>
    using Type = typename RangeTypes<T>::template Terminus<Observer>;
};

template<typename T>
struct TerminusSelector_<T, std::enable_if_t<IsMakeSelect<T>>>
{
    template<typename Observer>
    using Type = typename SelectTypes<T>::template Terminus<Observer>;
};

template<typename T>
struct TerminusSelector_<T, std::enable_if_t<IsMakeCustom<T>>>
{
    template<typename Observer>
    using Type = typename T::template Terminus<Observer>;
};

template<typename T>
struct TerminusSelector_<T, std::enable_if_t<IsMakeGroup<T>>>
{
    template<typename Observer>
    using Type = typename T::template Terminus<Observer>;
};
#endif


/***** Identity *****/
template<typename T, typename = void>
struct Identity_
{
    using Type = T;
};

template<typename T>
struct Identity_
<
    T,
    std::enable_if_t
    <
        IsFiltered<T>
        || IsMakeCustom<T>
        || IsMakeGroup<T>
        || IsMakeRange<T>
        || IsMakeSelect<T>
    >
>
{
    using Type = typename T::Type;
};

template<typename T>
struct Identity_<T, std::enable_if_t<IsMakeSignal<T>>>
{
    using Type = pex::DescribeSignal;
};


} // end namespace detail


template<typename T>
using ModelSelector = typename detail::ModelSelector_<T>::Type;


template<typename T>
using ControlSelector = typename detail::ControlSelector_<T>::Type;

#if 0
template<typename Observer>
struct TerminusSelector
{
    template<typename T>
    using Type = typename detail::TerminusSelector_<T>::template Type<Observer>;
};
#endif


template<typename T>
using Identity = typename detail::Identity_<T>::Type;


} // end namespace pex
