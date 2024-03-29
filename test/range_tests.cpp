
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
        terminus_(this, control)
    {
        PEX_LOG("Connect");
        this->terminus_.Connect(&Observer::OnValue_);
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
