/**
  * @file value.cpp
  *
  * @brief A brief demonstration of filtering.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 22 Jul 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif

#include <cmath>
#include <iostream>


#include "pex/value.h"


/**
 ** Limit the range of angles to [-pi, pi]
 **/
struct ModelFilter
{
    static double Set(double value)
    {
        static constexpr auto maximumAngle = M_PI;
        static constexpr auto minimumAngle = -M_PI;
        return std::max(minimumAngle, std::min(maximumAngle, value));
    }
};


/** The control uses degrees, while the model uses radians. **/
struct DegreesFilter
{
    /** Convert to degrees on retrieval **/
    static double Get(double value)
    {
        return 180 * value / M_PI;
    }

    /** Convert back to radians on assignment **/
    static double Set(double value)
    {
        return M_PI * value / 180.0;
    }
};


using Angle_radians = pex::model::FilteredValue<double, ModelFilter>;

static_assert(pex::IsModel<Angle_radians>);
static_assert(!pex::IsCopyable<Angle_radians>);
static_assert(pex::IsDirect<pex::UpstreamT<Angle_radians>>);

Angle_radians f(0.0);

class Foo
{
public:
    Foo(Angle_radians &angle_radians)
        :
        angle_degrees(angle_radians)
    {
        this->angle_degrees.Connect(this, &Foo::OnAngleChanged_);
    }

    ~Foo()
    {
        this->angle_degrees.Disconnect(this);
    }

    void OnAngleChanged_(double value)
    {
        std::cout << "Foo::OnAngleChanged_: " << value << std::endl;
    }

    pex::control::FilteredValue<Foo, Angle_radians, DegreesFilter> angle_degrees;
};


int main()
{
    Foo p(f);
    p.angle_degrees.Set(250.0);
    p.angle_degrees.Set(-181.0);
    p.angle_degrees.Set(45.0);
    f.Set(M_PI / 3.0);
    std::cout << f.Get() << std::endl;
}
