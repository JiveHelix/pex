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


template<typename T, typename enable = void>
struct IsMakeMute_: std::false_type {};

template<typename T>
struct IsMakeMute_
<
    T,
    std::enable_if_t
    <
        std::is_same_v<T, MakeMute>
    >
>: std::true_type {};


template<typename T, typename Enable = void>
struct DefinesIsDefineNodes_: std::false_type {};

template<typename T>
struct DefinesIsDefineNodes_
<
    T,
    std::enable_if_t<T::isDefineNodes>
>: std::true_type {};

template<typename T>
inline constexpr bool DefinesIsDefineNodes = DefinesIsDefineNodes_<T>::value;


template<typename T, typename Enable = void>
struct IsDefineNodes_: std::false_type {};

template<typename T>
struct IsDefineNodes_
<
    T,
    std::enable_if_t<DefinesIsDefineNodes<T>>
>: std::true_type {};


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
    template<typename, typename, typename> typename W
>
struct IsMakeRange_<MakeRange<T, U, V, W>>: std::true_type {};

template<typename ...T>
struct IsMakeRange_<MakeRange<T...>>: std::true_type {};


template<typename ...T> struct IsMakeSelect_: std::false_type {};

template<typename ...T>
struct IsMakeSelect_<MakeSelect<T...>>: std::true_type {};

template<typename ...T> struct IsMakePoly_: std::false_type {};

template<typename Supers>
struct IsMakePoly_<MakePoly<Supers>>: std::true_type {};


} // end namespace detail


} // end namespace pex
