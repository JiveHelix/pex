#pragma once


#include <pex/promote_control.h>


namespace pex
{


template<typename Observer, typename Upstream_>
class BoundedTerminus
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

    BoundedTerminus()
        :
        value{},
        minimum{},
        maximum{}
    {

    }

    explicit BoundedTerminus(const UpstreamControl &pex)
        :
        value(pex.value),
        minimum(pex.minimum),
        maximum(pex.maximum)
    {

    }

    BoundedTerminus(
        Observer *observer,
        const UpstreamControl &pex,
        Callable callable)
        :
        value(observer, pex.value, callable),
        minimum(pex.minimum),
        maximum(pex.maximum)
    {

    }

    BoundedTerminus(Observer *observer, UpstreamControl &&pex)
        :
        value(observer, std::move(pex.value)),
        minimum(observer, std::move(pex.minimum)),
        maximum(observer, std::move(pex.maximum))
    {

    }

    BoundedTerminus(
        Observer *observer,
        typename PromoteControl<Upstream_>::Upstream &upstream)
        :
        value(observer, upstream.value_),
        minimum(observer, upstream.minimum_),
        maximum(observer, upstream.maximum_)
    {

    }

    void ChangeUpstream(typename PromoteControl<Upstream_>::Upstream &upstream)
    {
        this->value.ChangeUpstream(upstream.value_);
        this->minimum.ChangeUpstream(upstream.minimum_);
        this->maximum.ChangeUpstream(upstream.maximum_);
    }

    BoundedTerminus(const BoundedTerminus &other) = delete;
    BoundedTerminus(BoundedTerminus &&other) = delete;
    BoundedTerminus & operator=(const BoundedTerminus &) = delete;
    BoundedTerminus & operator=(BoundedTerminus &&) = delete;

    // Copy construct
    BoundedTerminus(Observer *observer, const BoundedTerminus &other)
        :
        value(observer, other.value),
        minimum(observer, other.minimum),
        maximum(observer, other.maximum)
    {

    }

    // Copy construct from other observer
    template<typename O>
    BoundedTerminus(
        Observer *observer,
        const BoundedTerminus<O, Upstream_> &other)
        :
        value(observer, other.value),
        minimum(observer, other.minimum),
        maximum(observer, other.maximum)
    {

    }

    // Move construct
    BoundedTerminus(Observer *observer, BoundedTerminus &&other)
        :
        value(observer, std::move(other.value)),
        minimum(observer, std::move(other.minimum)),
        maximum(observer, std::move(other.maximum))
    {

    }

    // Move construct from other observer
    template<typename O>
    BoundedTerminus(
        Observer *observer,
        BoundedTerminus<O, Upstream_> &&other)
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
    BoundedTerminus & Assign(
        Observer *observer,
        const BoundedTerminus<Observer, Upstream_> &other)
    {
        this->value.Assign(observer, other.value);
        this->minimum.Assign(observer, other.minimum);
        this->maximum.Assign(observer, other.maximum);

        return *this;
    }

    // Move assign
    BoundedTerminus & Assign(
        Observer *observer,
        BoundedTerminus<Observer, Upstream_> &&other)
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
