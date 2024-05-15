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


template<typename T, typename Base, typename = void>
struct HasModelTemplate_: std::false_type {};

template<typename T, typename Base>
struct HasModelTemplate_
<
    T,
    Base,
    std::void_t<typename T::template Model<Base>>
>
: std::true_type {};

template<typename T, typename Base>
inline constexpr bool HasModelTemplate = HasModelTemplate_<T, Base>::value;


template<typename T, typename Base, typename = void>
struct HasControlTemplate_: std::false_type {};

template<typename T, typename Base>
struct HasControlTemplate_
<
    T,
    Base,
    std::void_t<typename T::template Control<Base>>
>
: std::true_type {};

template<typename T, typename Base>
inline constexpr bool HasControlTemplate = HasControlTemplate_<T, Base>::value;


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


template<typename T, typename Base, typename = void>
struct HasPlainTemplate_: std::false_type {};

template<typename T, typename Base>
struct HasPlainTemplate_
<
    T,
    Base,
    std::void_t<typename T::template Plain<Base>>
>
: std::true_type {};

template<typename T, typename Base>
inline constexpr bool HasPlainTemplate = HasPlainTemplate_<T, Base>::value;


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
