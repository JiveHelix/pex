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


namespace pex
{

// Helpful templates for generating interfaces

template<typename T>
using Identity = T;

template<typename T>
using Model = model::Value<T>;


#if 0
template<typename T>
using Control = control::Value<void, Model<T>>;
#endif


template<typename Observer, template<typename> typename Upstream = pex::Model>
struct Control
{
    template<typename T>
    using Type = control::Value<Observer, Upstream<T>>;
};


} // end namespace pex
