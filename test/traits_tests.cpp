#include <catch2/catch.hpp>

#include <vector>
#include <pex/detail/traits.h>
#include <pex/group.h>
#include <pex/range.h>
#include <pex/linked_ranges.h>


namespace traits_tests
{


using TesterLowerBound = pex::Limit<0>;
using TesterUpperBound = pex::Limit<1>;
using TesterLow = pex::Limit<0, 1, 10>;
using TesterHigh = pex::Limit<0, 25, 100>;


template<typename Float>
using TesterRanges =
    pex::LinkedRanges
    <
        pex::RangeOptions<Float, TesterLowerBound, TesterUpperBound>
    >;


template<template<typename> typename T>
struct TesterSchema
{
    T<typename TesterRanges<double>::Group> range;

    static constexpr auto fieldsTypeName = "Tester";
};


using TesterGroup = pex::Group<TesterSchema>;


} // end namespace traits_tests


TEST_CASE("Test HasPlain, HasModel, HasControl", "[traits]")
{
    STATIC_REQUIRE(
        pex::detail::HasModelTemplate
        <
            typename traits_tests::TesterRanges<double>::Finisher,
            traits_tests::TesterSchema<pex::Identity>
        >);

    STATIC_REQUIRE(
        pex::detail::HasPlain
        <
            typename traits_tests::TesterRanges<double>::Finisher
        >);

    using Model = traits_tests::TesterGroup::Model;
    Model model;
    auto plain = model.Get();
    REQUIRE(plain.range.low <= plain.range.high);

}
