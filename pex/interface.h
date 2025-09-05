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

#include <pex/default_value_node.h>
#include "pex/no_filter.h"
#include "pex/access_tag.h"
#include "pex/model_value.h"
#include "pex/traits.h"

namespace pex
{

// Helpful templates for generating interfaces

struct MakeSignal {};

struct MakeMute {};


template<typename Nodes>
struct DefineNodes
{
    static constexpr bool isDefineNodes = true;

    using Type = typename Nodes::Type;
    using Model = typename Nodes::Model;

    template
    <
        typename Upstream,
        typename ControlFilter = NoFilter,
        typename Access = GetAndSetTag
    >
    using Control =
        typename Nodes::template Control<Upstream, ControlFilter, Access>;

    using DefaultControl = Control<Model>;

    using Mux = typename Nodes::Mux;

    template
    <
        typename ControlFilter = NoFilter,
        typename Access = GetAndSetTag
    >
    using Follow = typename Nodes::template Follow<ControlFilter, Access>;
};


template
<
    typename T,
    typename Minimum_ = void,
    typename Maximum_ = void,
    template<typename, typename, typename>
    typename ValueNode_ = DefaultValueNode
>
struct MakeRange
{
    using Type = T;
    using Minimum = Minimum_;
    using Maximum = Maximum_;

    template<typename U, typename V, typename W>
    using ValueNode = ValueNode_<U, V, W>;
};


template<typename T, typename = void>
struct HasGetChoices_: std::false_type {};

template<typename T>
struct HasGetChoices_
<
    T,
    std::enable_if_t
    <
        std::is_same_v
        <
            std::vector<typename T::Type>,
            decltype(T::GetChoices())
        >
    >
>
: std::true_type {};


template<typename T>
inline constexpr bool HasGetChoices = HasGetChoices_<T>::value;


template<typename T, typename = void>
struct SelectType
{
    using Type = T;

    static std::vector<Type> GetChoices()
    {
        return {Type{}};
    }
};


template<typename T>
struct SelectType
<
    T,
    std::enable_if_t<HasGetChoices<T>>
>
{
    using Type = typename T::Type;

    static std::vector<Type> GetChoices()
    {
        return T::GetChoices();
    }
};


template
<
    typename T,
    typename Access_ = pex::GetAndSetTag
>
struct MakeSelect
{
    using Type = SelectType<std::decay_t<T>>;
    using Access = Access_;
};


template<HasValueBase Supers_>
struct MakePoly
{
    using Supers = Supers_;
};


template<
    typename T,
    typename ModelFilter_ = NoFilter,
    typename Access_ = GetAndSetTag>
struct Filtered
{
    using Type = T;
    using ModelFilter = ModelFilter_;
    using Access = Access_;
};


template<typename T>
using ReadOnly = Filtered<T, NoFilter, GetTag>;


} // end namespace pex


#include "pex/detail/interface.h"


namespace pex
{


template<typename T>
inline constexpr bool IsMakeSignal = detail::IsMakeSignal_<T>::value;

template<typename T>
inline constexpr bool IsMakeMute = detail::IsMakeMute_<T>::value;

template<typename ...T>
inline constexpr bool IsDefineNodes = detail::IsDefineNodes_<T...>::value;

template<typename ...T>
inline constexpr bool IsFiltered = detail::IsFiltered_<T...>::value;

template<typename ...T>
inline constexpr bool IsMakeRange = detail::IsMakeRange_<T...>::value;

template<typename ...T>
inline constexpr bool IsMakeSelect = detail::IsMakeSelect_<T...>::value;

template<typename ...T>
inline constexpr bool IsMakePoly = detail::IsMakePoly_<T...>::value;


} // end namespace pex
