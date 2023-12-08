#pragma once


namespace pex
{


namespace detail
{


template<typename T, typename enable = void>
struct IsMakeSignal_: std::false_type {};

template<typename T>
struct IsMakeSignal_
<
    T,
    std::enable_if_t
    <
        std::is_same_v<T, MakeSignal>
    >
>: std::true_type {};


template<typename T, typename Enable = void>
struct DefinesMakeCustom_: std::false_type {};

template<typename T>
struct DefinesMakeCustom_
<
    T,
    std::enable_if_t<T::isMakeCustom>
>: std::true_type {};

template<typename T>
inline constexpr bool DefinesMakeCustom = DefinesMakeCustom_<T>::value;


template<typename T, typename Enable = void>
struct IsMakeCustom_: std::false_type {};

template<typename T>
struct IsMakeCustom_
<
    T,
    std::enable_if_t<DefinesMakeCustom<T>>
>: std::true_type {};


template<typename ...T>
struct IsMakeGroup_: std::false_type {};

template
<
    typename G,
    typename M,
    typename C
>
struct IsMakeGroup_<MakeGroup<G, M, C>>: std::true_type {};


template<typename ...T>
struct IsFiltered_: std::false_type {};

template<typename ...T>
struct IsFiltered_<Filtered<T...>>: std::true_type {};


template<typename ...T> struct IsMakeRange_: std::false_type {};

template
<
    typename T,
    typename U,
    typename V,
    template<typename, typename> typename W
>
struct IsMakeRange_<MakeRange<T, U, V, W>>: std::true_type {};

template<typename ...T>
struct IsMakeRange_<MakeRange<T...>>: std::true_type {};


template<typename ...T> struct IsMakeSelect_: std::false_type {};

template<typename ...T>
struct IsMakeSelect_<MakeSelect<T...>>: std::true_type {};


template<typename ...T> struct IsMakeList_: std::false_type {};

template<typename T, size_t S>
struct IsMakeList_<MakeList<T, S>>: std::true_type {};


template<typename ...T> struct IsMakePolyList_: std::false_type {};

template<typename T, size_t S>
struct IsMakePolyList_<MakePolyList<T, S>>: std::true_type {};


} // end namespace detail


} // end namespace pex
