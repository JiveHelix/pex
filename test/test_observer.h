#pragma once

#include "pex/value.h"


template<typename Upstream>
struct Defaults
{
    using Control = pex::control::Value<void, Upstream>;

    template<typename Observer>
    using Terminus = pex::Terminus<Observer, Control>;
};


template
<
    typename Upstream,
    template<typename> typename Terminus = Defaults<Upstream>::template Terminus
>
class TestObserver
{
public:
    static constexpr auto observerName = "TestObserver";

    using Type = typename Upstream::Type;

    TestObserver(Upstream &upstream)
        :
        terminus_(this, upstream),
        count_(0),
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

    size_t GetCount() const
    {
        return this->count_;
    }

private:
    void Observe_(pex::Argument<Type> value)
    {
        this->observedValue = value;
        ++this->count_;
    }

    Terminus<TestObserver> terminus_;
    size_t count_;

public:
    Type observedValue;
};
