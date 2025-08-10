
#include <catch2/catch.hpp>
#include <pex/value.h>
#include <pex/range.h>
#include <pex/linked_ranges.h>


using Range = pex::model::Range<int>;
using Control = pex::control::Range<Range>;


TEST_CASE("Limits keep value within range.", "[range]")
{
    Range range(18);
    range.SetLimits(0, 20);

    Control control(range);

    REQUIRE(range.Get() == 18);
    REQUIRE(control.minimum.Get() == 0);
    REQUIRE(control.maximum.Get() == 20);

    control.value.Set(23);
    REQUIRE(range.Get() == 20);

    control.value.Set(-3);
    REQUIRE(range.Get() == 0);

    range.SetMinimum(5);
    REQUIRE(range.Get() == 5);
}


TEST_CASE("Limits keep optional value within range.", "[range]")
{
    using OptionalRange = pex::model::Range<std::optional<int>>;
    using OptionalControl = pex::control::Range<OptionalRange>;

    OptionalRange range;
    REQUIRE(!range.Get());

    range.SetLimits(0, 20);

    OptionalControl control(range);

    REQUIRE(!range.Get());

    REQUIRE(control.minimum.Get() == 0);
    REQUIRE(control.maximum.Get() == 20);

    control.value.Set(23);
    REQUIRE(*range.Get() == 20);

    control.value.Set(-3);
    REQUIRE(*range.Get() == 0);

    range.SetMinimum(5);
    REQUIRE(*range.Get() == 5);

    control.value.Set({});
    REQUIRE(!range.Get());
}


TEST_CASE("Limits filter propagates to controls.", "[range]")
{
    Range range(18);
    range.SetLimits(0, 20);

    Control control(range);

    REQUIRE(range.Get() == 18);

    range.SetLimits(0, 30);
    REQUIRE(control.minimum.Get() == 0);
    REQUIRE(control.maximum.Get() == 30);

    control.value.Set(23);
    REQUIRE(range.Get() == 23);

    control.value.Set(-3);
    REQUIRE(range.Get() == 0);

    range.SetMinimum(5);
    REQUIRE(range.Get() == 5);
}


struct Filter
{
    static float Get(int value)
    {
        return static_cast<float>(value) / 100.0f;
    }

    static int Set(float value)
    {
        return static_cast<int>(value * 100.0f);
    }
};


using FilteredRange = pex::control::Range<Control, Filter>;


TEST_CASE("Chaining ranges together to add a filter.", "[range]")
{
    Range range(18);
    range.SetLimits(0, 20);

    Control control(range);
    FilteredRange filtered(control);

    REQUIRE(filtered.minimum.Get() == 0.0f);
    REQUIRE(filtered.maximum.Get() == Approx(0.2f));
    REQUIRE(filtered.value.Get() == Approx(0.18f));

    filtered.value.Set(1.0f);
    REQUIRE(filtered.value.Get() == Approx(0.2f));
}


template<typename Control>
struct Observer
{
    static constexpr auto observerName = "range_tests::Observer";

    Observer(Control control)
        :
        terminus_(
            PEX_THIS("range_tests::Observer"),
            control,
            &Observer::OnValue_)
    {

    }

    void OnValue_(int value)
    {
        this->observed = value;
    }

    pex::Terminus<Observer, Control> terminus_;

    std::optional<int> observed;
};


TEST_CASE("Range value is echoed to observers.", "[range]")
{
    Range range(18);
    range.SetLimits(0, 20);

    Control control(range);
    Observer observer(control.value);
    control.value.Set(13);
    REQUIRE(!!observer.observed);
    REQUIRE(*observer.observed == 13);
}


TEST_CASE("Model value is echoed to observers.", "[range]")
{
    Range range(18);
    range.SetLimits(0, 20);

    Control control(range);
    Observer observer(control.value);
    range.Set(20);
    REQUIRE(!!observer.observed);
    REQUIRE(*observer.observed == 20);
}


TEST_CASE("Limited value is echoed to observers.", "[range]")
{
    Range range(18);
    range.SetLimits(0, 20);

    Control control(range);
    Observer observer(control.value);
    control.value.Set(24);
    REQUIRE(!!observer.observed);
    REQUIRE(*observer.observed == 20);
}


TEST_CASE("Linked ranges has comparison operators", "[range]")
{
    using TestRanges =
        pex::LinkedRanges
        <
            double,
            pex::Limit<0>,
            pex::Limit<0>,
            pex::Limit<10>,
            pex::Limit<10>
        >;

    using TestSettings = typename TestRanges::Settings;

    STATIC_REQUIRE(jive::HasEqualTo<TestSettings>);
}


TEST_CASE("Range limits are never optional.", "[range]")
{
    using MakeRange =
        pex::MakeRange<std::optional<double>, pex::Limit<-45>, pex::Limit<45>>;

    using RangeControl = pex::ControlSelector<MakeRange>;

    STATIC_REQUIRE(
        !jive::IsOptional
        <
            decltype(std::declval<RangeControl>().minimum.Get())
        >);

    STATIC_REQUIRE(
        !jive::IsOptional
        <
            decltype(std::declval<RangeControl>().maximum.Get())
        >);

    using LinearRange =
        pex::control::LinearRange
        <
            typename RangeControl::Upstream,
            typename RangeControl::Access
        >;

    STATIC_REQUIRE(
        !jive::IsOptional<decltype(std::declval<LinearRange>().minimum.Get())>);

    STATIC_REQUIRE(
        !jive::IsOptional<decltype(std::declval<LinearRange>().maximum.Get())>);
}


namespace linked
{


using ColorRanges =
    pex::LinkedRanges
    <
        int,
        pex::Limit<0>,
        pex::Limit<0>,
        pex::Limit<255>,
        pex::Limit<255>
    >;



template<typename T>
struct ColorFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::red, "red"),
        fields::Field(&T::green, "green"),
        fields::Field(&T::blue, "blue"));
};


template<template<typename> typename T>
struct ColorTemplate
{
    T<typename ColorRanges::Group> red;
    T<typename ColorRanges::Group> green;
    T<typename ColorRanges::Group> blue;

    static constexpr auto fields = ColorFields<ColorTemplate>::fields;
    static constexpr auto fieldsTypeName = "Color";
};


using ColorGroup = pex::Group<ColorFields, ColorTemplate>;
using ColorModel = typename ColorGroup::Model;


template<typename T>
struct PixelFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::color, "color"),
        fields::Field(&T::x, "x"),
        fields::Field(&T::y, "y"));
};


template<template<typename> typename T>
struct PixelTemplate
{
    T<ColorGroup> color;
    T<int> x;
    T<int> y;

    static constexpr auto fields = PixelFields<PixelTemplate>::fields;
    static constexpr auto fieldsTypeName = "Pixel";
};


struct PixelCustom
{
    template<typename Base>
    struct Plain: public Base
    {
        Plain()
            :
            Base{}
        {
            this->color.red.high = 1023;
        }
    };

    template<typename Base>
    struct Model: public Base
    {
        Model()
            :
            Base{}
        {

        }
    };
};


using PixelGroup = pex::Group<PixelFields, PixelTemplate, PixelCustom>;
using PixelModel = typename PixelGroup::Model;


} // end namespace linked


TEST_CASE("Linked ranges can be overridden in owner's initializer.", "[range]")
{
    linked::ColorModel color;
    linked::PixelModel pixel;

    REQUIRE(color.red.high.Get() == 255);
    REQUIRE(pixel.color.red.high.Get() == 1023);
}
