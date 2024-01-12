#pragma once


namespace pex
{


namespace detail
{


template<typename MaybeVoid, typename Fallback, typename = void>
struct ChooseNotVoid_
{
    using Type = Fallback;
};

template<typename MaybeVoid, typename Fallback>
struct ChooseNotVoid_
<
    MaybeVoid,
    Fallback,
    std::enable_if_t<!std::is_same_v<MaybeVoid, void>>
>
{
    using Type = MaybeVoid;
};


template<typename MaybeVoid, typename Fallback>
using ChooseNotVoid = typename ChooseNotVoid_<MaybeVoid, Fallback>::Type;


} // end namespace detail


} // end namespace pex
