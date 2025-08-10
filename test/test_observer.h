#pragma once

#include "pex/accessors.h"
#include "pex/endpoint.h"


template<typename Upstream>
struct Defaults
{
    using Control = pex::control::Value<Upstream>;

    template<typename Observer>
    using Terminus = pex::Terminus<Observer, Control>;
};


template
<
    typename Upstream,
    template<typename> typename Terminus = Defaults<Upstream>::template Terminus
>
class TerminusObserver
{
public:
    static constexpr auto observerName = "TerminusObserver";

    using Type = typename Upstream::Type;

    TerminusObserver(Upstream &upstream)
        :
        terminus_(
            PEX_THIS("TerminusObserver"),
            upstream,
            &TerminusObserver::Observe_),

        count_(0),
        observedValue{this->terminus_.Get()}
    {

    }

    void Set(pex::Argument<Type> value)
    {
        this->terminus_.Set(value);
    }

    TerminusObserver(TerminusObserver &&other)
        :
        terminus_(this, std::move(other.terminus_)),
        count_(other.count_),
        observedValue(std::move(other.observedValue))
    {
        REQUIRE(this->terminus_.GetObserver() == this);
    }

    TerminusObserver & operator=(TerminusObserver &&other)
    {
        this->terminus_.Assign(this, std::move(other.terminus_));
        this->count_ = other.count_;
        this->observedValue = std::move(other.observedValue);

        REQUIRE(this->terminus_.GetObserver() == this);

        return *this;
    }

    TerminusObserver(const TerminusObserver &other)
        :
        terminus_(this, other.terminus_),
        count_(other.count_),
        observedValue(std::move(other.observedValue))
    {
        REQUIRE(this->terminus_.GetObserver() == this);
    }

    TerminusObserver & operator=(const TerminusObserver &other)
    {
        this->terminus_.Assign(this, other.terminus_);
        this->count_ = other.count_;
        this->observedValue = other.observedValue;
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

    Terminus<TerminusObserver> terminus_;
    size_t count_;

public:
    Type observedValue;
};


template<typename Object>
class TestObserver
{
public:
    static constexpr auto observerName = "TestObserver";

    using Type = typename Object::Type;

    TestObserver(Object &object)
        :
        connect_(this, object, &TestObserver::Observe_),
        count_(0),
        observedValue{object.Get()}
    {

    }

    void Set(pex::Argument<Type> value)
    {
        this->connect_.Set(value);
    }

    TestObserver(TestObserver &&) = delete;
    TestObserver & operator=(TestObserver &&) = delete;
    TestObserver(const TestObserver &) = delete;
    TestObserver & operator=(const TestObserver &) = delete;

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

    pex::MakeConnector<TestObserver<Object>, Object> connect_;
    size_t count_;

public:
    Type observedValue;
};
