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
        Float,
        TesterLowerBound,
        TesterLow,
        TesterUpperBound,
        TesterHigh
    >;


template<typename T>
struct TesterFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::range, "range"));

    static constexpr auto fieldsTypeName = "Tester";
};


template<template<typename> typename T>
struct TesterTemplate
{
    T<typename TesterRanges<double>::Group> range;

    static constexpr auto fields = TesterFields<TesterTemplate>::fields;
    static constexpr auto fieldsTypeName = "Tester";
};


struct TesterSettings:
    public TesterTemplate<pex::Identity>
{
    static TesterSettings Default()
    {
        TesterSettings settings{{
            TesterRanges<double>::Settings::Default()}};

        return settings;
    }
};


using TesterGroup = pex::Group<TesterFields, TesterTemplate, pex::PlainT<TesterSettings>>;


} // end namespace traits_tests


TEST_CASE("Test HasPlain, HasModel, HasControl", "[traits]")
{
    STATIC_REQUIRE(
        pex::detail::HasModelTemplate
        <
            typename traits_tests::TesterRanges<double>::GroupTypes_,
            traits_tests::TesterTemplate<pex::Identity>
        >);

    STATIC_REQUIRE(
        pex::detail::HasPlain
        <
            typename traits_tests::TesterRanges<double>::GroupTypes_
        >);

    using Model = traits_tests::TesterGroup::Model;
    Model model;
    auto plain = model.Get();
    REQUIRE(plain.range.low <= plain.range.high);

}
