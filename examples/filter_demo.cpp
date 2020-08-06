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
#include "pex/value.h"
#include "jive/angles.h"
#include <iostream>

/**
 ** Limit the range of angles to [-pi, pi]
 **/
struct ModelFilter
{
    static double Set(double value)
    {
        static constexpr auto maximumAngle = jive::Angles<double>::pi;
        static constexpr auto minimumAngle = -jive::Angles<double>::pi;
        return std::max(minimumAngle, std::min(maximumAngle, value));
    }
};


/** The interface uses degrees, while the model uses radians. **/
struct DegreesFilter
{
    /** Convert to degrees on retrieval **/
    static double Get(double value)
    {
        return jive::ToDegrees(value);
    }

    /** Convert back to radians on assignment **/
    static double Set(double value)
    {
        return jive::ToRadians(value);
    }
};


using Angle_radians = pex::model::FilteredValue<double, ModelFilter>;

Angle_radians f(0.0);

class Foo
{
public:
    Foo(Angle_radians * const angle_radians)
        :
        angle_degrees_(angle_radians)
    {
        this->angle_degrees_.Connect(this, &Foo::OnAngleChanged_);
    }

    ~Foo()
    {
        this->angle_degrees_.Disconnect(this);
    }

    void OnAngleChanged_(double angle_degrees)
    {
        std::cout << "Foo::OnAngleChanged_: " << angle_degrees << std::endl;
    }
    
    void Set(double angle_degrees)
    {
        this->angle_degrees_.Set(angle_degrees);
    }

    pex::interface::Value<Foo, Angle_radians> angle_degrees_;
};


int main()
{
    Foo p(&f);
    p.Set(250.0);
    p.Set(-181.0);
    p.Set(45.0);
    f.Set(jive::Angles<double>::pi / 3.0);
    std::cout << f.Get() << std::endl;
}
