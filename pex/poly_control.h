#pragma once


#include "pex/poly_value.h"
#include "pex/poly_model.h"


namespace pex
{


namespace poly
{


template<HasValueBase Supers>
class Control
{
public:
    using ValueBase = typename Supers::ValueBase;
    using Value = ::pex::poly::Value<ValueBase>;
    using Type = Value;
    using Plain = Type;

    using ControlBase = MakeControlBase<Supers>;

    using Callable = typename ControlBase::Callable;
    using Upstream = Model<Supers>;

    static constexpr bool isPexCopyable = true;
    static constexpr auto observerName = "pex::poly::Control";

    Control()
        :
        upstream_(),
        base_(),
        baseCreated_()
    {

    }

    Control(Upstream &upstream)
        :
        upstream_(&upstream),
        base_(),
        baseCreated_(
            this,
            this->upstream_->baseCreated_,
            &Control::OnBaseCreated_)
    {
        auto modelBase = upstream.GetVirtual();

        if (modelBase)
        {
            this->base_ = modelBase->CreateControl();
        }
    }

    Control(const Control &other)
        :
        upstream_(other.upstream_),
        base_(other.base_),
        baseCreated_(
            this,
            this->upstream_->baseCreated_,
            &Control::OnBaseCreated_)
    {

    }

    Control(void *observer, Upstream &upstream, Callable callable)
        :
        upstream_(upstream),
        base_(),
        baseCreated_(
            this,
            this->upstream_->baseCreated_,
            &Control::OnBaseCreated_)
    {
        auto modelBase = upstream.GetVirtual();

        if (modelBase)
        {
            this->base_ = modelBase->CreateControl();
            this->Connect(observer, callable);
        }
    }

    Control(void *observer, const Control &other, Callable callable)
        :
        upstream_(other.upstream_),
        base_(other.base_),
        baseCreated_(
            this,
            this->upstream_->baseCreated_,
            &Control::OnBaseCreated_)
    {
        if (!this->base_)
        {
            throw std::logic_error("Cannot connect without a valid object.");
        }

        this->Connect(observer, callable);
    }

    Control & operator=(const Control &other)
    {
        this->upstream_ = other.upstream_;
        this->base_ = other.base_;
        this->baseCreated_.Assign(this, other.baseCreated_);

        return *this;
    }

    Value Get() const
    {
        assert(this->base_);
        return this->base_->GetValue();
    }

    std::string_view GetTypeName() const
    {
        assert(this->base_);
        return this->base_->GetTypeName();
    }

    const ControlBase * GetVirtual() const
    {
        assert(this->base_);
        return this->base_.get();
    }

    ControlBase * GetVirtual()
    {
        assert(this->base_);
        return this->base_.get();
    }

    void Set(const Value &value)
    {
        assert(this->base_);
        this->base_->SetValue(value);
    }

    operator bool () const
    {
        return (this->base_);
    }

    void Connect(void *observer, Callable callable)
    {
        assert(this->base_);
        this->base_->Connect(observer, callable);
    }

    void Disconnect(void *observer)
    {
        assert(this->base_);
        this->base_->Disconnect(observer);
    }

    bool HasModel() const
    {
        if (!this->upstream_)
        {
            return false;
        }

        return (this->upstream_->GetVirtual() != nullptr);
    }

// TODO: Add this to pex::Reference
// protected:
    void SetWithoutNotify_(const Value &value)
    {
        this->base_->SetValueWithoutNotify(value);
    }

    void DoNotify_()
    {
        this->base_->DoValueNotify();
    }

private:
    void OnBaseCreated_()
    {
        auto modelBase = this->upstream_->GetVirtual();

        if (modelBase)
        {
            this->base_ = modelBase->CreateControl();
        }
    }

private:
    using BaseCreatedTerminus =
        ::pex::Terminus<Control, pex::control::Signal<>>;

    Upstream *upstream_;
    std::shared_ptr<ControlBase> base_;
    BaseCreatedTerminus baseCreated_;
};


template<typename Supers>
struct IsPolyControl_: std::false_type {};

template<typename Supers>
struct IsPolyControl_<Control<Supers>>: std::true_type {};

template<typename Supers>
inline constexpr bool IsPolyControl = IsPolyControl_<Supers>::value;


} // end namespace poly


template<typename T>
struct IsControl_
<
    T,
    std::enable_if_t<poly::IsPolyControl<T>>
>: std::true_type {};


} // end namespace pex
