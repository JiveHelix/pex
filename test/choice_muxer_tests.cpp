#include <catch2/catch.hpp>

#include <pex/choice_muxer.h>


struct TestChoiceMaker
{
    using Key = std::string;
    using Type = int;
    using ChoiceMap = std::map<Key, std::vector<Type>>;

    static ChoiceMap GetChoiceMap()
    {
        return {
            {"foo", {1, 2, 3, 4}},
            {"bar", {5, 6, 7}}};
    }
};


TEST_CASE("Choices change with key", "[choice_muxer]")
{
    using Model = pex::ChoiceMuxerModel<TestChoiceMaker>;
    using Control = pex::ChoiceMuxerControl<TestChoiceMaker>;

    Model model;
    Control control(model);

    // Default keys are sorted alphabetically
    REQUIRE(model.key.Get() == "bar");
    REQUIRE(control.select.choices.Get().size() == 3);
    REQUIRE(control.select.choices.Get()[0] == 5);
    REQUIRE(*control.select.value.Get() == 5);

    model.key.Set("foo");

    REQUIRE(model.key.Get() == "foo");
    REQUIRE(control.select.choices.Get().size() == 4);
    REQUIRE(control.select.choices.Get()[0] == 1);
    REQUIRE(*control.select.value.Get() == 1);
}


TEST_CASE("Update choice map doesn't change selection", "[choice_muxer]")
{
    using Model = pex::ChoiceMuxerModel<TestChoiceMaker>;
    using Control = pex::ChoiceMuxerControl<TestChoiceMaker>;

    Model model;
    Control control(model);

    model.key.Set("foo");
    model.select.SetSelection(2);

    REQUIRE(control.key.Get() == "foo");
    REQUIRE(*control.select.value.Get() == 3);

    auto choiceMap = model.GetChoiceMap();
    choiceMap["car"] = std::vector{{10, 11, 12, 13, 14}};
    model.SetChoiceMap(choiceMap);

    REQUIRE(control.key.Get() == "foo");
    REQUIRE(*control.select.value.Get() == 3);

    model.key.Set("car");

    REQUIRE(control.key.Get() == "car");
    REQUIRE(*control.select.value.Get() == 12);
}
