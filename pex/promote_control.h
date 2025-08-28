#pragma once

#include "pex/traits.h"
#include "pex/signal.h"
#include "pex/control_value.h"
#include <pex/range.h>
#include <pex/list.h>
#include <pex/select.h>


namespace pex
{


/** PromoteControl
 **
 ** Converts model values/signals to controls.
 ** Preserves control values/signals.
 **
 ** Converts mux values/signals to follows
 ** Preserves Follow values/signals.
 **
 **/
template<typename Pex, typename enable = void>
struct PromoteControl
{
    using Type = control::Value_<Pex>;
    using Upstream = Pex;
};


template<typename Pex>
struct PromoteControl
<
    Pex,
    std::enable_if_t<IsControl<Pex>>
>
{
    using Type = Pex;
    using Upstream = typename Pex::Upstream;
};


template<typename Pex>
struct PromoteControl<Pex, std::enable_if_t<IsSignalControl<Pex>>>
{
    using Type = Pex;
    using Upstream = typename Pex::Upstream;
};


template<typename Pex>
struct PromoteControl<Pex, std::enable_if_t<IsSignalModel<Pex>>>
{
    using Type = control::Signal<Pex>;
    using Upstream = Pex;
};


template<typename P>
struct PromoteControl<P, std::enable_if_t<IsRangeModel<P>>>
{
    using Type = control::Range<P>;
    using Upstream = P;
};


template<typename P>
struct PromoteControl<P, std::enable_if_t<IsRangeControl<P>>>
{
    using Type = P;
    using Upstream = typename P::Upstream;
};


template<typename P>
struct PromoteControl<P, std::enable_if_t<IsRangeMux<P>>>
{
    using Type = control::RangeFollow<P>;
    using Upstream = P;
};


template<typename P>
struct PromoteControl<P, std::enable_if_t<IsGroupModel<P>>>
{
    using Type = typename P::GroupType::template Control<P>;
    using Upstream = P;
};


template<typename P>
struct PromoteControl<P, std::enable_if_t<IsGroupControl<P>>>
{
    using Type = P;
    using Upstream = typename P::Upstream;
};

template<typename P>
struct PromoteControl<P, std::enable_if_t<IsGroupMux<P>>>
{
    using Type = typename P::GroupType::Follow;
    using Upstream = P;
};

#if 0
template<typename P>
struct PromoteControl<P, std::enable_if_t<IsGroupFollow<P>>>
{
    using Type = P;
    using Upstream = typename P::Upstream;
};
#endif


template<typename P>
struct PromoteControl<P, std::enable_if_t<IsListModel<P>>>
{
    using Type = typename P::ListType::template Control<P>;
    using Upstream = P;
};


template<typename P>
struct PromoteControl<P, std::enable_if_t<IsListControl<P>>>
{
    using Type = P;
    using Upstream = typename P::Upstream;
};

template<typename P>
struct PromoteControl<P, std::enable_if_t<IsListMux<P>>>
{
    using Type = typename P::ListType::Follow;
    using Upstream = P;
};


template<typename P>
struct PromoteControl<P, std::enable_if_t<IsSelectModel<P>>>
{
    using Type = control::Select<P>;
    using Upstream = P;
};


template<typename P>
struct PromoteControl<P, std::enable_if_t<IsSelectControl<P>>>
{
    using Type = P;
    using Upstream = typename P::Upstream;
};


template<typename P>
struct PromoteControl<P, std::enable_if_t<IsSelectMux<P>>>
{
    using Type = control::SelectFollow<P>;
    using Upstream = P;
};


} // end namespace pex
