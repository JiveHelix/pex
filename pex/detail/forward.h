#pragma once


namespace pex
{


namespace detail
{


template
<
    typename Observer,
    typename Upstream_,
    template<typename> typename Selector
>
class GroupConnect;


template
<
    typename Observer,
    typename Upstream_,
    template<typename> typename Selector
>
class ListConnect;


} // end namespace detail


} // end namespace pex
