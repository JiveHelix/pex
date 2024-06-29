#pragma once


#include "pex/model_value.h"
#include "pex/control_value.h"
#include "pex/range.h"
#include "pex/select.h"
#include "pex/signal.h"
#include "pex/interface.h"
#include "pex/list.h"
#include "pex/poly.h"


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

    using Control = pex::control::Select<Model>;
};


template<typename ListMaker, typename Model>
struct ListModel_
{
    using Type = ::pex::model::List<Model, ListMaker::initialCount>;
};


template<typename ListMaker, typename Model>
using ListModel = typename ListModel_<ListMaker, Model>::Type;


template<typename ListMaker, typename Model, typename Control>
struct ListControl_
{
    using Type =
        ::pex::control::List
        <
            ListModel<ListMaker, Model>,
            Control
        >;
};


template<typename ListMaker, typename Model, typename Control>
using ListControl = typename ListControl_<ListMaker, Model, Control>::Type;


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
struct ModelSelector_<T, std::enable_if_t<IsMakeCustom<T>>>
{
    using Type = typename T::Custom;
};

template<typename T>
struct ModelSelector_<T, std::enable_if_t<(IsGroup<T>)>>
{
    using Type = typename T::Model;
};

template<typename T>
struct ModelSelector_<T, std::enable_if_t<IsMakeList<T>>>
{
    using Type =
        ListModel
        <
            T,
            typename ModelSelector_<typename T::MemberType>::Type
        >;
};


template<typename T>
struct ModelSelector_<T, std::enable_if_t<IsMakePolyList<T>>>
{
    using Type =
        ListModel<T, poly::Model<typename T::Supers>>;
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
        typename T::Access
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
struct ControlSelector_<T, std::enable_if_t<(IsGroup<T>)>>
{
    using Type = typename T::Control;
};

template<typename T>
struct ControlSelector_<T, std::enable_if_t<IsMakeList<T>>>
{
    using Type =
        ListControl
        <
            T,
            typename ModelSelector_<typename T::MemberType>::Type,
            typename ControlSelector_<typename T::MemberType>::Type
        >;
};


template<typename T>
struct ControlSelector_<T, std::enable_if_t<IsMakePolyList<T>>>
{
    using Type =
        ListControl
        <
            T,
            poly::Model<typename T::Supers>,
            poly::Control<typename T::Supers>
        >;
};


} // end namespace detail


template<typename T>
using ModelSelector = typename detail::ModelSelector_<T>::Type;


template<typename T>
using ControlSelector = typename detail::ControlSelector_<T>::Type;


} // end namespace pex
