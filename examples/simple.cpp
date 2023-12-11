#include <iostream>
#include <pex/model_value.h>
#include <pex/control_value.h>


void Observer(void *, double value)
{
    std::cout << "observed: " << value << std::endl;
}


int main()
{
    using Model = pex::model::Value<double>;
    using Control = pex::control::Value<Model>;
    using Follower = pex::control::Value<Control>;
    Model model(42.0);
    Control control(model);
    Follower follower(control);

    follower.Connect(nullptr, &Observer);

    follower.Set(3.14);
    std::cout << model.Get() << std::endl;

    follower.Disconnect(nullptr);

    return 0;
}
