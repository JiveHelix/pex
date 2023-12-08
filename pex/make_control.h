#pragma once

#include "pex/traits.h"
#include "pex/signal.h"
#include "pex/control_value.h"


namespace pex
{


/** MakeControl
 **
 ** Converts model values/signals into controls.
 ** Preserves control values/signals.
 **
 **/
template<typename Pex, typename enable = void>
struct MakeControl
{
    using Control = control::Value_<Pex>;
    using Upstream = Pex;
};


template<typename Pex>
struct MakeControl
<
    Pex,
    std::enable_if_t<IsControl<Pex>>
>
{
    using Control = Pex;
    using Upstream = typename Pex::Upstream;
};


template<typename Pex>
struct MakeControl<Pex, std::enable_if_t<IsControlSignal<Pex>>>
{
    using Control = Pex; // control::Signal<>;
    using Upstream = typename Pex::Upstream;
};


template<typename Pex>
struct MakeControl<Pex, std::enable_if_t<IsModelSignal<Pex>>>
{
    using Control = control::Signal<>;
    using Upstream = Pex;
};


} // end namespace pex
