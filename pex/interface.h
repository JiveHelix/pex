/**
  * @file interface.h
  * 
  * @brief Declares templated helpers for generating interfaces.
  * 
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 11 Mar 2022
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/


#pragma once


#include "pex/value.h"
#include "pex/signal.h"


namespace pex
{

// Helpful templates for generating interfaces

template<typename T>
using Model = model::Value<T>;


template<typename Observer, template<typename> typename Upstream = pex::Model>
struct Control
{
    template<typename T>
    using Type = control::Value<Observer, Upstream<T>>;
};


struct MakeSignal {};


template<typename Custom_>
struct MakeCustom
{
    using Custom = Custom_;
    using Type = typename Custom::Type;

    template<typename Observer>
    using Control = typename Custom::template Control<Observer>;
};


template<typename Group_>
struct MakeGroup
{
    using Group = Group_;
    using Type = typename Group_::Plain;
};


template<
    typename T,
    typename ModelFilter_ = NoFilter,
    typename ControlAccess_ = GetAndSetTag>
struct Member
{
    using Type = T;
    using ModelFilter = ModelFilter_;
    using ControlAccess = ControlAccess_;
};


} // end namespace pex


#include "pex/detail/interface_detail.h"


namespace pex
{


template<typename T>
using ModelSelector = typename detail::ModelSelector_<T>::Type;


template<typename Observer>
struct ControlSelector
{
    template<typename T>
    using Type = typename detail::ControlSelector_<T>::template Type<Observer>;
};


template<typename Observer>
struct TerminusSelector
{
    template<typename T>
    using Type = typename detail::TerminusSelector_<T>::template Type<Observer>;
};


template<typename T>
using Identity = typename detail::Identity_<T>::Type;


} // end namespace pex
