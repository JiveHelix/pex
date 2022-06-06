#pragma once

#include <string>
#include "pex/value.h"


enum class UnitSystem: uint8_t
{
    MKS,
    CGS,
    FPS,
    FFF
};


struct ShortConverter
{
    static std::string ToString(UnitSystem unitSystem)
    {
        switch (unitSystem)
        {
            case (UnitSystem::MKS):
                return "MKS";

            case (UnitSystem::CGS):
                return "CGS";

            case (UnitSystem::FPS):
                return "FPS";

            case (UnitSystem::FFF):
                return "FFF";

            default:
                throw std::logic_error("Unknown unit system");
        }
    }
};


struct LongConverter
{
    static std::string ToString(UnitSystem unitSystem)
    {
        switch (unitSystem)
        {
            case (UnitSystem::MKS):
                return "meter-kilogram-second";

            case (UnitSystem::CGS):
                return "centimeter-gram-second";

            case (UnitSystem::FPS):
                return "foot-pound-second";

            case (UnitSystem::FFF):
                return "furlong-firkin-fortnight";

            default:
                throw std::logic_error("Unknown unit system");
        }
    }
};


using UnitsModel = pex::model::Value<UnitSystem>;
using UnitsControl = pex::control::Value<void, UnitsModel>;
