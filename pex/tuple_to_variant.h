#pragma once


#include <variant>
#include <tuple>
#include <type_traits>


namespace pex
{


// UniqueTuple takes in variadic type arguments, and returns a tuple without
// any repeated types. Every element of the tuple is unique.
template <typename T, typename... Ts>
struct UniqueTuple_ : std::type_identity<T> {};

template <typename ...Ts, typename U, typename ...Us>
struct UniqueTuple_<std::tuple<Ts...>, U, Us...>
    :
    std::conditional_t
    <
        std::disjunction_v<std::is_same<U, Ts>...>,

        // U is the same as one of the T's.
        // It will not be included a second time.
        // Continue checking the remaining U's.
        UniqueTuple_<std::tuple<Ts...>, Us...>,

        // U is not already in the set of T's.
        // Add it to the unique tuple.
        // Continue checking the remaining U's.
        UniqueTuple_<std::tuple<Ts..., U>, Us...>
    > {};

template <typename ...Ts>
using UniqueTuple = typename UniqueTuple_<std::tuple<>, Ts...>::type;


namespace detail
{


    template<typename>
    struct ConvertToVariant_
    {

    };


    template<typename ...Ts>
    struct ConvertToVariant_<std::tuple<Ts...>>
    {
        using Type = std::variant<Ts...>;
    };


    template<typename T>
    using ConvertToVariant = typename ConvertToVariant_<T>::Type;


} // end namespace detail


template<typename>
struct TupleToVariant_
{

};


template<typename ...Ts>
struct TupleToVariant_<std::tuple<Ts...>>
{
    using Type = detail::ConvertToVariant<UniqueTuple<Ts...>>;
};


template<typename T>
using TupleToVariant = typename TupleToVariant_<T>::Type;


template<typename T>
using Fields = std::decay_t<decltype(T::fields)>;


template<typename T>
struct FieldsVariant_
{
    using Type = TupleToVariant<Fields<T>>;
};


template<typename T>
using FieldsVariant = typename FieldsVariant_<T>::Type;


template<typename Field>
using FieldType = typename Field::Type;

template<typename>
struct FieldsElements_ {};

template<typename ...Ts>
struct FieldsElements_<std::tuple<Ts...>>
{
    using Type = std::tuple<FieldType<Ts>...>;
};

template<typename T>
using FieldsElements = typename FieldsElements_<Fields<T>>::Type;


template<typename T>
using FieldsElementsVariant = TupleToVariant<FieldsElements<T>>;


} // end namespace pex
