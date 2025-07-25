#pragma once


#include <fmt/core.h>
#include "pex/poly_value.h"
#include "pex/poly_model.h"
#include "pex/traits.h"


namespace pex
{


namespace poly
{


template<HasValueBase Supers>
class Control
{
public:
    using Access = GetAccess<Supers>;
    using ValueBase = typename Supers::ValueBase;
    using Value = ::pex::poly::Value<ValueBase>;
    using Type = Value;
    using Plain = Type;

    using SuperControl = MakeControlSuper<Supers>;

    using Callable = typename SuperControl::Callable;
    using Upstream = Model<Supers>;

    static constexpr bool isPexCopyable = true;
    static constexpr bool isPolyControl = true;
    static constexpr auto observerName = "pex::poly::Control";

    Control()
        :
        upstream_(),
        base_(),
        baseCreated(),
        baseCreatedTerminus_()
    {
        REGISTER_PEX_NAME(
            this,
            fmt::format("PolyControl<{}>", jive::GetTypeName<Supers>()));

        REGISTER_PEX_PARENT(baseCreated);
        REGISTER_PEX_PARENT(baseCreatedTerminus_);
    }

    Control(Upstream &upstream)
        :
        upstream_(&upstream),
        base_(),
        baseCreated(this->upstream_->baseCreated_),

        baseCreatedTerminus_(
            USE_REGISTER_PEX_NAME(
                this,
                fmt::format("PolyControl<{}>", jive::GetTypeName<Supers>())),
            this->upstream_->baseCreated_,
            &Control::OnBaseCreated_)
    {
        REGISTER_PEX_PARENT(baseCreated);
        REGISTER_PEX_PARENT(baseCreatedTerminus_);

        auto modelBase = upstream.GetVirtual();

        if (modelBase)
        {
            this->base_ = modelBase->CreateControl();
        }

        PEX_LOG(
            " Construct from upstream ",
            LookupPexName(this),
            " from ",
            LookupPexName(&upstream));
    }

    Control(const Control &other)
        :
        upstream_(other.upstream_),
        base_(other.base_),
        baseCreated(this->upstream_->baseCreated_),

        baseCreatedTerminus_(
            USE_REGISTER_PEX_NAME(
                this,
                fmt::format("PolyControl<{}>", jive::GetTypeName<Supers>())),
            this->upstream_->baseCreated_,
            &Control::OnBaseCreated_)
    {
        REGISTER_PEX_PARENT(baseCreated);
        REGISTER_PEX_PARENT(baseCreatedTerminus_);

        PEX_LOG(
            " Copy ",
            LookupPexName(this),
            " from ",
            LookupPexName(&other));
    }

    Control(Control &&other) noexcept
        :
        upstream_(std::move(other.upstream_)),
        base_(std::move(other.base_)),
        baseCreated(this->upstream_->baseCreated_),
        baseCreatedTerminus_(
            USE_REGISTER_PEX_NAME(
                this,
                fmt::format("PolyControl<{}>", jive::GetTypeName<Supers>())),
            this->upstream_->baseCreated_,
            &Control::OnBaseCreated_)
    {
        REGISTER_PEX_PARENT(baseCreated);
        REGISTER_PEX_PARENT(baseCreatedTerminus_);

        PEX_LOG(
            " Copy ",
            LookupPexName(this),
            " from ",
            LookupPexName(&other));
    }

    Control(void *observer, Upstream &upstream, Callable callable)
        :
        upstream_(upstream),
        base_(),
        baseCreated(this->upstream_->baseCreated_),
        baseCreatedTerminus_(
            USE_REGISTER_PEX_NAME(
                this,
                fmt::format("PolyControl<{}>", jive::GetTypeName<Supers>())),
            this->upstream_->baseCreated_,
            &Control::OnBaseCreated_)
    {
        REGISTER_PEX_PARENT(baseCreated);
        REGISTER_PEX_PARENT(baseCreatedTerminus_);

        auto modelBase = upstream.GetVirtual();

        if (modelBase)
        {
            this->base_ = modelBase->CreateControl();
            this->Connect(observer, callable);
        }

        PEX_LOG(
            " Construct from upstream with callable ",
            LookupPexName(this),
            " from ",
            LookupPexName(&upstream));
    }

    Control(void *observer, const Control &other, Callable callable)
        :
        upstream_(other.upstream_),
        base_(other.base_),
        baseCreated(this->upstream_->baseCreated_),
        baseCreatedTerminus_(
            USE_REGISTER_PEX_NAME(
                this,
                fmt::format("PolyControl<{}>", jive::GetTypeName<Supers>())),
            this->upstream_->baseCreated_,
            &Control::OnBaseCreated_)
    {
        REGISTER_PEX_PARENT(baseCreated);
        REGISTER_PEX_PARENT(baseCreatedTerminus_);

        if (!this->base_)
        {
            throw std::logic_error("Cannot connect without a valid object.");
        }

        PEX_LOG(
            " Copy with callable ",
            LookupPexName(this),
            " from ",
            LookupPexName(&other));

        this->Connect(observer, callable);
    }

    Control & operator=(const Control &other)
    {
        PEX_LOG(
            " operator= ",
            LookupPexName(this),
            " from ",
            LookupPexName(&other));

        this->upstream_ = other.upstream_;
        this->base_ = other.base_;
        this->baseCreated = other.baseCreated;

        this->baseCreatedTerminus_.RequireAssign(
            this,
            other.baseCreatedTerminus_);

        return *this;
    }

    ~Control()
    {
        PEX_LOG("Destruct ", LookupPexName(this));
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

    // GetVirtual may return nullptr
    const SuperControl * GetVirtual() const
    {
        return this->base_.get();
    }

    SuperControl * GetVirtual()
    {
        return this->base_.get();
    }

    template<typename DerivedControl>
    DerivedControl & RequireDerived()
    {
        auto result = dynamic_cast<DerivedControl *>(this->base_.get());

        if (!result)
        {
            throw PolyError("Mismatched polymorphic value");
        }

        return *result;
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
    std::shared_ptr<SuperControl> base_;

public:
    using BaseCreatedControl = pex::control::Signal<>;
    BaseCreatedControl baseCreated;

private:
    BaseCreatedTerminus baseCreatedTerminus_;
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
