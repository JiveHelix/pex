#pragma once


#include "pex/endpoint.h"
#include "pex/tuple_to_variant.h"


namespace pex
{


template
<
    template<typename> typename Fields,
    typename Observer,
    typename Source,
    typename Target,
    size_t index
>
struct BoundField
{
    static constexpr auto sourceField = std::get<index>(Fields<Source>::fields);
    using SourceField = std::decay_t<decltype(sourceField)>;

    static constexpr auto targetField = std::get<index>(Fields<Target>::fields);
    using TargetField = std::decay_t<decltype(targetField)>;

    using SourceType = typename SourceField::Type;
    using Value = typename SourceType::Type;

    using Endpoint = pex::BoundEndpoint
        <
            SourceType,
            void (Observer::*)(pex::Argument<Value>, const TargetField &),
            TargetField
        >;
};


template
<
    template<typename> typename Fields,
    typename Observer,
    typename Source,
    typename Target
>
using BoundFields =
    decltype(
        []<size_t ...I>(std::index_sequence<I...>)
        {
            return std::tuple(
                std::declval
                <
                    BoundField<Fields, Observer, Source, Target, I>
                >()...);
        }(std::make_index_sequence<std::tuple_size_v<pex::Fields<Source>>>{}));


template<typename T>
using EndpointType = typename T::Endpoint;


#if 0

template<typename T>
using EndpointsTuple =
    decltype(
        std::apply(
            []<typename ...Ts>(Ts &&...)
            {
                return std::make_tuple(std::declval<EndpointType<Ts>>()...);
            },
            std::declval<T>()));

#else

template<typename>
struct EndpointsTuple_ {};

template<typename ...Ts>
struct EndpointsTuple_<std::tuple<Ts...>>
{
    using Type = std::tuple<EndpointType<Ts>...>;
};

template<typename T>
using EndpointsTuple = typename EndpointsTuple_<T>::Type;

#endif

template<typename T>
using EndpointsVariant = TupleToVariant<EndpointsTuple<T>>;


} // end namespace pex
