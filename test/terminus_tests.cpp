#include <catch2/catch.hpp>
#include "test_observer.h"
#include "pex/terminus.h"


TEST_CASE("Terminus uses new observer after move.", "[terminus]")
{
    using Model = pex::model::Value<int>;
    using Observer = TestObserver<Model>;

    Model value(42);
    Observer first(value);

    REQUIRE(first.observedValue == 42);

    first.Set(43);

    REQUIRE(first.observedValue == 43);
    
    Observer second(std::move(first));

    REQUIRE(second.observedValue == 43);

    second.Set(44);

    REQUIRE(second.observedValue == 44);
}
