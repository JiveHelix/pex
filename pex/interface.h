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

#include "pex/no_filter.h"
#include "pex/access_tag.h"

namespace pex
{

// Helpful templates for generating interfaces

struct MakeSignal {};


template<typename Custom_>
struct MakeCustom
{
    using Custom = Custom_;
    using Type = typename Custom::Type;

    template<typename Observer>
    using Control = typename Custom::template Control<Observer>;
};


template
<
    typename T,
    typename Minimum_ = void,
    typename Maximum_ = void
>
struct MakeRange
{
    using Type = T;
    using Minimum = Minimum_;
    using Maximum = Maximum_;
};


template
<
    typename Group_,
    typename Model_ = typename Group_::Model,
    template<typename> typename Control_ = Group_::template Control,
    template<typename> typename Terminus_ = Group_::template Terminus
>
struct MakeGroup
{
    using Group = Group_;
    using Model = Model_;

    template<typename Observer>
    using Control = Control_<Observer>;

    template<typename Observer>
    using Terminus = Terminus_<Observer>;

    using Type = typename Group_::Plain;
};


template<
    typename T,
    typename ModelFilter_ = NoFilter,
    typename ControlAccess_ = GetAndSetTag>
struct Filtered
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
inline constexpr bool IsMakeSignal = detail::IsMakeSignal_<T>::value;

template<typename ...T>
inline constexpr bool IsMakeCustom = detail::IsMakeCustom_<T...>::value;

template<typename ...T>
inline constexpr bool IsMakeGroup = detail::IsMakeGroup_<T...>::value;

template<typename ...T>
inline constexpr bool IsFiltered = detail::IsFiltered_<T...>::value;

template<typename ...T>
inline constexpr bool IsMakeRange = detail::IsMakeRange_<T...>::value;


} // end namespace pex
