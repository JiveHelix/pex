#pragma once


#include <variant>
#include <tuple>
#include <type_traits>
#include <jive/unique_tuple.h>


namespace pex
{


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
    using Type = detail::ConvertToVariant<jive::UniqueTuple<Ts...>>;
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
