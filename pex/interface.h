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
#include "pex/model_value.h"

namespace pex
{

// Helpful templates for generating interfaces

struct MakeSignal {};


template<typename Custom_>
struct MakeCustom
{
    static constexpr bool isMakeCustom = true;

    using Custom = Custom_;
    using Type = typename Custom::Type;

    using Control = typename Custom::Control;
};


template
<
    typename T,
    typename Minimum_ = void,
    typename Maximum_ = void,
    template<typename, typename> typename Value_ = pex::model::Value_
>
struct MakeRange
{
    using Type = T;
    using Minimum = Minimum_;
    using Maximum = Maximum_;

    template<typename U, typename V>
    using Value = Value_<U, V>;
};


template
<
    typename T,
    typename Access_ = pex::GetAndSetTag
>
struct MakeSelect
{
    using Type = T;
    using Access = Access_;
};


template
<
    typename T,
    size_t initialCount_
>
struct MakeList
{
    using MemberType = T;
    inline static constexpr size_t initialCount = initialCount_;
};


template
<
    typename Group_,
    typename Model_ = typename Group_::Model,
    typename Control_ = typename Group_::Control
>
struct MakeGroup
{
    using Group = Group_;
    using Model = Model_;
    using Control = Control_;
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


#include "pex/detail/interface.h"


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

template<typename ...T>
inline constexpr bool IsMakeSelect = detail::IsMakeSelect_<T...>::value;

template<typename ...T>
inline constexpr bool IsMakeList = detail::IsMakeList_<T...>::value;


} // end namespace pex
