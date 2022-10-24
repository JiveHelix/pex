/**
  * @file pex.h
  *
  * @brief Imports pex features.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 11 Mar 2022
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/
#pragma once


#include <fields/fields.h>
#include "pex/value.h"
#include "pex/signal.h"
#include "pex/range.h"

#include "pex/chooser.h"
#include "pex/reference.h"
#include "pex/transaction.h"
#include "pex/interface.h"
#include "pex/group.h"


namespace pex
{


template<typename T>
using Model = model::Value<T>;


template<typename Observer, template<typename> typename Upstream = pex::Model>
struct Control
{
    template<typename T>
    using Type = control::Value<Observer, Upstream<T>>;
};


} // end namespace pex
