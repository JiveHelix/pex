#pragma once

#include "pex/value.h"


template<typename Model>
struct Defaults
{
    using Control = pex::control::Value<void, Model>;

    template<typename Observer>
    using Terminus = pex::Terminus<Observer, Control>;
};


template
<
    typename Model,
    template<typename> typename Terminus = Defaults<Model>::template Terminus
>
class TestObserver
{
public:
    using Type = typename Model::Type;

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
        REQUIRE(this->terminus_.GetObserver() == this);
        this->terminus_.Connect(&TestObserver::Observe_);
    }

    TestObserver & operator=(TestObserver &&other)
    {
        this->terminus_.Assign(this, std::move(other.terminus_));
        this->observedValue = std::move(other.observedValue);
        this->terminus_.Connect(&TestObserver::Observe_);

        REQUIRE(this->terminus_.GetObserver() == this);

        return *this;
    }

    TestObserver(const TestObserver &other)
        :
        terminus_(this, other.terminus_),
        observedValue(std::move(other.observedValue))
    {
        this->terminus_.Connect(&TestObserver::Observe_);
        REQUIRE(this->terminus_.GetObserver() == this);
    }

    TestObserver & operator=(const TestObserver &other)
    {
        this->terminus_.Assign(this, other.terminus_);
        this->observedValue = other.observedValue;
        this->terminus_.Connect(&TestObserver::Observe_);
        REQUIRE(this->terminus_.GetObserver() == this);

        return *this;
    }

private:
    void Observe_(pex::Argument<Type> value)
    {
        this->observedValue = value;
    }

    Terminus<TestObserver> terminus_;

public:
    Type observedValue;
};
