#pragma once


namespace pex
{


namespace detail
{


struct TraitsTest {};


template<typename T, typename = void>
struct HasImpl_: std::false_type {};

template<typename T>
struct HasImpl_
<
    T,
    std::void_t<typename T::template Impl<TraitsTest>>
>
: std::true_type {};

template<typename T>
inline constexpr bool HasImpl = HasImpl_<T>::value;


template<typename T, typename = void>
struct HasModelTemplate_: std::false_type {};

template<typename T>
struct HasModelTemplate_
<
    T,
    std::void_t<typename T::template Model<TraitsTest>>
>
: std::true_type {};

template<typename T>
inline constexpr bool HasModelTemplate = HasModelTemplate_<T>::value;


template<typename T, typename = void>
struct HasControlTemplate_: std::false_type {};

template<typename T>
struct HasControlTemplate_
<
    T,
    std::void_t<typename T::template Control<TraitsTest>>
>
: std::true_type {};

template<typename T>
inline constexpr bool HasControlTemplate = HasControlTemplate_<T>::value;


template<typename T, typename = void>
struct HasModelUserBaseTemplate_: std::false_type {};

template<typename T>
struct HasModelUserBaseTemplate_
<
    T,
    std::void_t<typename T::template ModelUserBase<TraitsTest>>
>
: std::true_type {};

template<typename T>
inline constexpr bool HasModelUserBaseTemplate =
    HasModelUserBaseTemplate_<T>::value;


template<typename T, typename = void>
struct HasControlUserBaseTemplate_: std::false_type {};

template<typename T>
struct HasControlUserBaseTemplate_
<
    T,
    std::void_t<typename T::template ControlUserBase<TraitsTest>>
>
: std::true_type {};

template<typename T>
inline constexpr bool HasControlUserBaseTemplate =
    HasControlUserBaseTemplate_<T>::value;


template<typename T, typename = void>
struct HasPlainTemplate_: std::false_type {};

template<typename T>
struct HasPlainTemplate_
<
    T,
    std::void_t<typename T::template Plain<TraitsTest>>
>
: std::true_type {};

template<typename T>
inline constexpr bool HasPlainTemplate = HasPlainTemplate_<T>::value;


template<typename T, typename = void>
struct HasPlain_: std::false_type {};

template<typename T>
struct HasPlain_
<
    T,
    std::void_t<typename T::Plain>
>
: std::true_type {};

template<typename T>
inline constexpr bool HasPlain = HasPlain_<T>::value;


} // end namespace detail


} // end namespace pex
