#pragma once


#include <pex/promote_control.h>


namespace pex
{


template<typename Observer, typename Upstream_>
class RangeTerminus
{
public:
    using UpstreamControl = typename PromoteControl<Upstream_>::Type;

    using Upstream = typename PromoteControl<Upstream_>::Upstream;
    using Value = pex::Terminus<Observer, typename UpstreamControl::Value>;
    using Limit = pex::Terminus<Observer, typename UpstreamControl::Limit>;
    using Type = typename UpstreamControl::Type;
    using Callable = typename Value::Callable;

    Value value;
    Limit minimum;
    Limit maximum;

    static constexpr bool isPexCopyable = UpstreamControl::isPexCopyable;

    RangeTerminus()
        :
        value{},
        minimum{},
        maximum{}
    {

    }

    explicit RangeTerminus(const UpstreamControl &pex)
        :
        value(pex.value),
        minimum(pex.minimum),
        maximum(pex.maximum)
    {

    }

    RangeTerminus(
        Observer *observer,
        const UpstreamControl &pex,
        Callable callable)
        :
        value(observer, pex.value, callable),
        minimum(pex.minimum),
        maximum(pex.maximum)
    {

    }

    RangeTerminus(Observer *observer, UpstreamControl &&pex)
        :
        value(observer, std::move(pex.value)),
        minimum(observer, std::move(pex.minimum)),
        maximum(observer, std::move(pex.maximum))
    {

    }

    RangeTerminus(
        Observer *observer,
        typename PromoteControl<Upstream_>::Upstream &upstream)
        :
        value(observer, upstream.value_),
        minimum(observer, upstream.minimum_),
        maximum(observer, upstream.maximum_)
    {

    }

    void SwapUpstream(typename PromoteControl<Upstream_>::Upstream &upstream)
    {
        this->value.SwapUpstream(upstream.value_);
        this->minimum.SwapUpstream(upstream.minimum_);
        this->maximum.SwapUpstream(upstream.maximum_);
    }

    RangeTerminus(const RangeTerminus &other) = delete;
    RangeTerminus(RangeTerminus &&other) = delete;
    RangeTerminus & operator=(const RangeTerminus &) = delete;
    RangeTerminus & operator=(RangeTerminus &&) = delete;

    // Copy construct
    RangeTerminus(Observer *observer, const RangeTerminus &other)
        :
        value(observer, other.value),
        minimum(observer, other.minimum),
        maximum(observer, other.maximum)
    {

    }

    // Copy construct from other observer
    template<typename O>
    RangeTerminus(
        Observer *observer,
        const RangeTerminus<O, Upstream_> &other)
        :
        value(observer, other.value),
        minimum(observer, other.minimum),
        maximum(observer, other.maximum)
    {

    }

    // Move construct
    RangeTerminus(Observer *observer, RangeTerminus &&other)
        :
        value(observer, std::move(other.value)),
        minimum(observer, std::move(other.minimum)),
        maximum(observer, std::move(other.maximum))
    {

    }

    // Move construct from other observer
    template<typename O>
    RangeTerminus(
        Observer *observer,
        RangeTerminus<O, Upstream_> &&other)
        :
        value(observer, std::move(other.value)),
        minimum(observer, std::move(other.minimum)),
        maximum(observer, std::move(other.maximum))
    {

    }

    void Emplace(const UpstreamControl &control)
    {
        this->Disconnect();

        this->value.Emplace(control.value);
        this->minimum.Emplace(control.minimum);
        this->maximum.Emplace(control.maximum);
    }

    void Emplace(
        Observer *observer,
        const UpstreamControl &control,
        Callable callable)
    {
        this->Disconnect();

        this->value.Emplace(observer, control.value, callable);
        this->minimum.Emplace(control.minimum);
        this->maximum.Emplace(control.maximum);

        this->Connect(observer, callable);
    }

    // Copy assign
    RangeTerminus & Assign(
        Observer *observer,
        const RangeTerminus<Observer, Upstream_> &other)
    {
        this->value.Assign(observer, other.value);
        this->minimum.Assign(observer, other.minimum);
        this->maximum.Assign(observer, other.maximum);

        return *this;
    }

    // Move assign
    RangeTerminus & Assign(
        Observer *observer,
        RangeTerminus<Observer, Upstream_> &&other)
    {
        this->value.Assign(observer, std::move(other.value));
        this->minimum.Assign(observer, std::move(other.minimum));
        this->maximum.Assign(observer, std::move(other.maximum));

        return *this;
    }

    void Connect(Observer *observer, Callable callable)
    {
        this->value.Connect(observer, callable);
    }

    void Disconnect(Observer *)
    {
        this->Disconnect();
    }

    void Disconnect()
    {
        this->value.Disconnect();
        this->minimum.Disconnect();
        this->maximum.Disconnect();
    }

    explicit operator Type () const
    {
        return this->value.Get();
    }

    explicit operator UpstreamControl () const
    {
        UpstreamControl result;
        result.value = this->value;
        result.minimum = this->minimum;
        result.maximum = this->maximum;

        return result;
    }

    Type Get() const
    {
        return this->value.Get();
    }

    void Set(Argument<Type> value_)
    {
        this->value.Set(value_);
    }

    bool HasModel() const
    {
        return this->value.HasModel()
            && this->minimum.HasModel()
            && this->maximum.HasModel();
    }

    template<typename>
    friend class ::pex::Reference;

    void Notify()
    {
        this->value.Notify();
    }

private:
    void SetWithoutNotify_(Argument<Type> value_)
    {
        detail::AccessReference(this->value).SetWithoutNotify(value_);
    }
};


} // end namespace pex
