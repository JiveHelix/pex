#pragma once

#include "pex/value.h"


template<typename Model>
class TestObserver
{
public:
    using Type = typename Model::Type;

    using Control =
        pex::control::Value<void, Model>;

    using Terminus = pex::Terminus<TestObserver, Control>;

    TestObserver(Model &model)
        :
        terminus_(this, model),
        observedValue{this->terminus_.Get()}
    {
        this->terminus_.Connect(&TestObserver::Observe_);
    }

    void Set(pex::Argument<Type> value)
    {
        this->terminus_.Set(value);
    }

    TestObserver(TestObserver &&other)
        :
        terminus_(this, std::move(other.terminus_)),
        observedValue(std::move(other.observedValue))
    {
        this->terminus_.Connect(&TestObserver::Observe_);
    }

    TestObserver & operator=(TestObserver &&other)
    {
        this->terminus_.Assign(this, std::move(other.terminus_));
        this->observedValue = std::move(other.observedValue);
        this->terminus_.Connect(&TestObserver::Observe_);

        return *this;
    }

private:
    void Observe_(pex::Argument<Type> value)
    {
        this->observedValue = value;
    }

    Terminus terminus_;

public:
    Type observedValue;
};
