/**
  * @author Jive Helix (jivehelix@gmail.com)
  * @copyright 2020 Jive Helix
  * Licensed under the MIT license. See LICENSE file.
  */

#include <catch2/catch.hpp>
#include <string>
#include <jive/testing/generator_limits.h>
#include <jive/testing/gettys_words.h>
#include "test_observer.h"


TEMPLATE_TEST_CASE(
    "Numeric value propagation",
    "[value]",
    int8_t,
    uint8_t,
    int16_t,
    uint16_t,
    int32_t,
    uint32_t,
    int64_t,
    uint64_t,
    float,
    double,
    long double)
{
    auto original = static_cast<TestType>(
        GENERATE(
            take(
                3,
                random(
                    GeneratorLimits<TestType>::Lowest(),
                    GeneratorLimits<TestType>::Max()))));

    auto propagated = static_cast<TestType>(
        GENERATE(
            take(
                10,
                random(
                    GeneratorLimits<TestType>::Lowest(),
                    GeneratorLimits<TestType>::Max()))));

    using Model = pex::model::Value<TestType>;
    Model model{original};

    TerminusObserver<Model> observer(model);

    if constexpr (std::is_floating_point_v<TestType>)
    {
        REQUIRE(observer.observedValue == original);
    }
    else
    {
        REQUIRE(observer.observedValue == Approx(original));
    }

    model.Set(propagated);

    if constexpr (std::is_floating_point_v<TestType>)
    {
        REQUIRE(observer.observedValue == propagated);
    }
    else
    {
        REQUIRE(observer.observedValue == Approx(propagated));
    }
}


TEMPLATE_TEST_CASE(
    "Numeric value propagation using assignment operator",
    "[value]",
    int8_t,
    uint8_t,
    int16_t,
    uint16_t,
    int32_t,
    uint32_t,
    int64_t,
    uint64_t,
    float,
    double,
    long double)
{
    auto original = static_cast<TestType>(
        GENERATE(
            take(
            3,
            random(
                GeneratorLimits<TestType>::Lowest(),
                GeneratorLimits<TestType>::Max()))));

    auto propagated = static_cast<TestType>(
        GENERATE(
            take(
            10,
            random(
                GeneratorLimits<TestType>::Lowest(),
                GeneratorLimits<TestType>::Max()))));

    using Model = pex::model::Value<TestType>;
    Model model{original};
    TerminusObserver<Model> observer(model);

    if constexpr (std::is_floating_point_v<TestType>)
    {
        REQUIRE(observer.observedValue == original);
    }
    else
    {
        REQUIRE(observer.observedValue == Approx(original));
    }

    model = propagated;

    if constexpr (std::is_floating_point_v<TestType>)
    {
        REQUIRE(observer.observedValue == propagated);
    }
    else
    {
        REQUIRE(observer.observedValue == Approx(propagated));
    }
}




TEST_CASE("std::string propagation", "[value]")
{
    auto wordCount = GENERATE(take(10, random(1u, 10u)));

    std::string original = RandomGettysWords().MakeWords(wordCount);
    std::string propagated = RandomGettysWords().MakeWords(wordCount);

    using Model = pex::model::Value<std::string>;
    Model model(original);
    TerminusObserver<Model> observer(model);

    REQUIRE(observer.observedValue == original);
    model.Set(propagated);
    REQUIRE(observer.observedValue == propagated);
}


TEMPLATE_TEST_CASE(
    "Numeric value fan-out propagation",
    "[value]",
    int8_t,
    uint8_t,
    int16_t,
    uint16_t,
    int32_t,
    uint32_t,
    int64_t,
    uint64_t,
    float,
    double,
    long double)
{
    auto original = static_cast<TestType>(
        GENERATE(
            take(
            3,
            random(
                GeneratorLimits<TestType>::Lowest(),
                GeneratorLimits<TestType>::Max()))));

    auto propagated = static_cast<TestType>(
        GENERATE(
            take(
            10,
            random(
                GeneratorLimits<TestType>::Lowest(),
                GeneratorLimits<TestType>::Max()))));

    using Model = pex::model::Value<TestType>;
    Model model{original};
    TerminusObserver<Model> observer1(model);
    TerminusObserver<Model> observer2(model);
    TerminusObserver<Model> observer3(model);
    TerminusObserver<Model> observer4(model);

    if constexpr (std::is_floating_point_v<TestType>)
    {
        REQUIRE(observer1.observedValue == original);
        REQUIRE(observer2.observedValue == original);
        REQUIRE(observer3.observedValue == original);
        REQUIRE(observer4.observedValue == original);
    }
    else
    {
        REQUIRE(observer1.observedValue == Approx(original));
        REQUIRE(observer2.observedValue == Approx(original));
        REQUIRE(observer3.observedValue == Approx(original));
        REQUIRE(observer4.observedValue == Approx(original));
    }

    // Control values echo back to us, and fan out to all other observers.
    observer1.Set(propagated);

    if constexpr (std::is_floating_point_v<TestType>)
    {
        REQUIRE(observer1.observedValue == propagated);
        REQUIRE(observer2.observedValue == propagated);
        REQUIRE(observer3.observedValue == propagated);
        REQUIRE(observer4.observedValue == propagated);
    }
    else
    {
        REQUIRE(observer1.observedValue == Approx(propagated));
        REQUIRE(observer2.observedValue == Approx(propagated));
        REQUIRE(observer3.observedValue == Approx(propagated));
        REQUIRE(observer4.observedValue == Approx(propagated));
    }
}
