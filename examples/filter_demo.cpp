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

#include <iostream>
#include "pex/value.h"
#include "tau/angles.h"

/**
 ** Limit the range of angles to [-pi, pi]
 **/
struct ModelFilter
{
    static double Set(double value)
    {
        static constexpr auto maximumAngle = tau::Angles<double>::pi;
        static constexpr auto minimumAngle = -tau::Angles<double>::pi;
        return std::max(minimumAngle, std::min(maximumAngle, value));
    }
};


/** The interface uses degrees, while the model uses radians. **/
struct DegreesFilter
{
    /** Convert to degrees on retrieval **/
    static double Get(double value)
    {
        return tau::ToDegrees(value);
    }

    /** Convert back to radians on assignment **/
    static double Set(double value)
    {
        return tau::ToRadians(value);
    }
};


using Angle_radians = pex::model::FilteredValue<double, ModelFilter>;

Angle_radians f(0.0);

class Foo
{
public:
    Foo(Angle_radians * const angle_radians)
        :
        angle_degrees(angle_radians)
    {
        this->angle_degrees.Connect(this, &Foo::OnAngleChanged_);
    }

    ~Foo()
    {
        this->angle_degrees.Disconnect();
    }

    void OnAngleChanged_(double value)
    {
        std::cout << "Foo::OnAngleChanged_: " << value << std::endl;
    }

    pex::interface::FilteredValue<Foo, Angle_radians, DegreesFilter> angle_degrees;
};


int main()
{
    Foo p(&f);
    p.angle_degrees.Set(250.0);
    p.angle_degrees.Set(-181.0);
    p.angle_degrees.Set(45.0);
    f.Set(tau::Angles<double>::pi / 3.0);
    std::cout << f.Get() << std::endl;
}
