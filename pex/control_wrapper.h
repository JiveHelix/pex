#pragma once


#include <fmt/core.h>
#include "pex/value_wrapper.h"
#include "pex/model_wrapper.h"
#include "pex/traits.h"
#include "pex/terminus.h"


namespace pex
{


namespace poly
{


template
<
    typename Upstream_,
    HasValueBase Supers,
    typename BaseSignal = pex::control::Signal<::pex::model::Signal>
>
class ControlWrapperTemplate
{
public:
    using Access = GetAccess<Supers>;
    using ValueBase = typename Supers::ValueBase;
    using ValueWrapper = ::pex::poly::ValueWrapperTemplate<ValueBase>;
    using Type = ValueWrapper;
    using Plain = Type;

    using SuperControl = MakeControlSuper<Supers>;

    using Callable = typename SuperControl::Callable;
    using Upstream = Upstream_;

    static constexpr bool isPexCopyable = true;
    static constexpr bool isControlWrapper = true;
    static constexpr auto observerName = "ControlWrapper";

    ControlWrapperTemplate()
        :
        upstream_(),
        base_(),
        baseWillDelete(),
        baseCreated(),
        baseWillDeleteTerminus_(),
        baseCreatedTerminus_()
    {
        PEX_NAME(fmt::format("PolyControl<{}>", jive::GetTypeName<Supers>()));

        PEX_MEMBER(baseWillDelete);
        PEX_MEMBER(baseCreated);
        PEX_MEMBER(baseWillDeleteTerminus_);
        PEX_MEMBER(baseCreatedTerminus_);
    }

    ControlWrapperTemplate(Upstream &upstream)
        :
        upstream_(&upstream),
        base_(),
        baseWillDelete(this->upstream_->baseWillDelete_),
        baseCreated(this->upstream_->baseCreated_),

        baseWillDeleteTerminus_(
            PEX_THIS(
                fmt::format("PolyControl<{}>", jive::GetTypeName<Supers>())),
            this->upstream_->internalBaseWillDelete_,
            &ControlWrapperTemplate::OnBaseWillDelete_),

        baseCreatedTerminus_(
            this,
            this->upstream_->internalBaseCreated_,
            &ControlWrapperTemplate::OnBaseCreated_)
    {
        PEX_MEMBER(baseWillDelete);
        PEX_MEMBER(baseCreated);
        PEX_MEMBER(baseWillDeleteTerminus_);
        PEX_MEMBER(baseCreatedTerminus_);

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

    ControlWrapperTemplate(const ControlWrapperTemplate &other)
        :
        upstream_(other.upstream_),
        base_(),
        baseWillDelete(this->upstream_->baseWillDelete_),
        baseCreated(this->upstream_->baseCreated_),

        baseWillDeleteTerminus_(
            PEX_THIS(
                fmt::format("PolyControl<{}>", jive::GetTypeName<Supers>())),

            this->upstream_->internalBaseWillDelete_,
            &ControlWrapperTemplate::OnBaseWillDelete_),

        baseCreatedTerminus_(
            this,
            this->upstream_->internalBaseCreated_,
            &ControlWrapperTemplate::OnBaseCreated_)
    {
        if (other.base_)
        {
            this->base_ = other.base_->Copy();
        }

        PEX_MEMBER(baseWillDelete);
        PEX_MEMBER(baseCreated);
        PEX_MEMBER(baseWillDeleteTerminus_);
        PEX_MEMBER(baseCreatedTerminus_);

        PEX_LOG(
            " Copy ",
            LookupPexName(this),
            " from ",
            LookupPexName(&other));
    }

    ControlWrapperTemplate(ControlWrapperTemplate &&other) noexcept
        :
        upstream_(std::move(other.upstream_)),
        base_(std::move(other.base_)),
        baseWillDelete(this->upstream_->baseWillDelete_),
        baseCreated(this->upstream_->baseCreated_),

        baseWillDeleteTerminus_(
            PEX_THIS(
                fmt::format("PolyControl<{}>", jive::GetTypeName<Supers>())),
            this->upstream_->internalBaseWillDelete_,
            &ControlWrapperTemplate::OnBaseWillDelete_),

        baseCreatedTerminus_(
            this,
            this->upstream_->internalBaseCreated_,
            &ControlWrapperTemplate::OnBaseCreated_)
    {
        PEX_MEMBER(baseWillDelete);
        PEX_MEMBER(baseCreated);
        PEX_MEMBER(baseWillDeleteTerminus_);
        PEX_MEMBER(baseCreatedTerminus_);

        PEX_LOG(
            " Copy ",
            LookupPexName(this),
            " from ",
            LookupPexName(&other));
    }

    ControlWrapperTemplate(
        void *observer,
        Upstream &upstream,
        Callable callable)
        :
        upstream_(upstream),
        base_(),
        baseWillDelete(this->upstream_->baseWillDelete_),
        baseCreated(this->upstream_->baseCreated_),

        baseWillDeleteTerminus_(
            PEX_THIS(
                fmt::format("PolyControl<{}>", jive::GetTypeName<Supers>())),
            this->upstream_->internalBaseWillDelete_,
            &ControlWrapperTemplate::OnBaseWillDelete_),

        baseCreatedTerminus_(
            this,
            this->upstream_->internalBaseCreated_,
            &ControlWrapperTemplate::OnBaseCreated_)
    {
        PEX_MEMBER(baseWillDelete);
        PEX_MEMBER(baseCreated);
        PEX_MEMBER(baseWillDeleteTerminus_);
        PEX_MEMBER(baseCreatedTerminus_);

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

    ControlWrapperTemplate(
        void *observer,
        const ControlWrapperTemplate &other,
        Callable callable)
        :
        upstream_(other.upstream_),
        base_(),
        baseWillDelete(this->upstream_->baseWillDelete_),
        baseCreated(this->upstream_->baseCreated_),

        baseWillDeleteTerminus_(
            PEX_THIS(
                fmt::format("PolyControl<{}>", jive::GetTypeName<Supers>())),
            this->upstream_->internalBaseWillDelete_,
            &ControlWrapperTemplate::OnBaseWillDelete_),

        baseCreatedTerminus_(
            this,
            this->upstream_->internalBaseCreated_,
            &ControlWrapperTemplate::OnBaseCreated_)
    {
        PEX_MEMBER(baseWillDelete);
        PEX_MEMBER(baseCreated);
        PEX_MEMBER(baseWillDeleteTerminus_);
        PEX_MEMBER(baseCreatedTerminus_);

        if (!other.base_)
        {
            throw std::logic_error("Cannot connect without a valid object.");
        }

        this->base_ = other.base_->Copy();

        PEX_LOG(
            " Copy with callable ",
            LookupPexName(this),
            " from ",
            LookupPexName(&other));

        this->Connect(observer, callable);
    }

    ControlWrapperTemplate & operator=(const ControlWrapperTemplate &other)
    {
        PEX_CONCISE_LOG(
            " operator= copy ",
            LookupPexName(this),
            " from ",
            LookupPexName(&other));

        this->upstream_ = other.upstream_;

        if (other.base_)
        {
            this->base_ = other.base_->Copy();
        }
        else
        {
            this->base_.reset();
        }

        this->baseWillDelete = other.baseWillDelete;
        this->baseCreated = other.baseCreated;

        this->baseWillDeleteTerminus_.RequireAssign(
            this,
            other.baseWillDeleteTerminus_);

        this->baseCreatedTerminus_.RequireAssign(
            this,
            other.baseCreatedTerminus_);

        return *this;
    }

    ControlWrapperTemplate & operator=(ControlWrapperTemplate &&other)
    {
        PEX_CONCISE_LOG(
            " operator= move ",
            LookupPexName(this),
            " from ",
            LookupPexName(&other));

        this->upstream_ = std::move(other.upstream_);
        this->base_ = std::move(other.base_);

        if (this->base_)
        {
            PEX_CONCISE_LOG(
                "moved base_: ",
                LookupPexName(this->base_.get()));
        }

        assert(!other.base_);

        this->baseWillDelete = other.baseWillDelete;
        this->baseCreated = std::move(other.baseCreated);

        this->baseWillDeleteTerminus_.RequireAssign(
            this,
            other.baseWillDeleteTerminus_);

        this->baseCreatedTerminus_.RequireAssign(
            this,
            other.baseCreatedTerminus_);

        return *this;
    }

    ~ControlWrapperTemplate()
    {
        PEX_CLEAR_NAME(this);
        PEX_CLEAR_NAME(&baseCreated);
        PEX_CLEAR_NAME(&baseCreatedTerminus_);
        PEX_CLEAR_NAME(&baseWillDeleteTerminus_);

    }

    ValueWrapper Get() const
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

    void Set(const ValueWrapper &value)
    {
        assert(this->base_);
        this->base_->SetValue(value);
    }

    operator bool () const
    {
        return this->base_.get() != NULL;
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

    void Notify()
    {
        this->base_->DoValueNotify();
    }

// TODO: Add this to pex::Reference
// protected:
    void SetWithoutNotify_(const ValueWrapper &value)
    {
        this->base_->SetValueWithoutNotify(value);
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

    void OnBaseWillDelete_()
    {
        this->base_.reset();
    }

private:
    using BaseTerminus =
        ::pex::Terminus<ControlWrapperTemplate, ::pex::control::Signal<::pex::model::Signal>>;

    Upstream *upstream_;
    std::unique_ptr<SuperControl> base_;

public:
    BaseSignal baseWillDelete;
    BaseSignal baseCreated;

private:
    BaseTerminus baseWillDeleteTerminus_;
    BaseTerminus baseCreatedTerminus_;
};


#if 0
template<typename Supers>
using MuxBase =
    ControlWrapperTemplate
    <
        ModelWrapperTemplate<Supers>,
        Supers,
        pex::control::SignalMux
    >;


template<typename Supers>
class Mux : public MuxBase<Supers>
{
public:
    using Base = MuxBase<Supers>;
    using Upstream = typename Base::Upstream;
    static constexpr bool isPexCopyable = false;

    Mux(const Mux &) = delete;
    Mux(Mux &&) = delete;
    Mux & operator=(const Mux &) = delete;
    Mux & operator=(Mux &&) = delete;

    using Base::Base;

    void ChangeUpstream(Upstream &upstream)
    {
        if (this->base_)
        {
            ::pex::model::Signal temporaryBaseWillDelete;
            this->baseWillDelete.ChangeUpstream(temporaryBaseWillDelete);
            temporaryBaseWillDelete.Trigger();
        }

        this->upstream_ = upstream;

        auto modelBase = this->upstream_->GetVirtual();

        if (modelBase)
        {
            this->base_ = modelBase->CreateControl();
            ::pex::model::Signal temporaryBaseCreated;
            this->baseCreated.ChangeUpstream(temporaryBaseCreated);
            temporaryBaseCreated.Trigger();
        }

        this->baseCreated.ChangeUpstream(upstream.baseCreated);
        this->baseWillDelete.ChangeUpstream(upstream.baseWillDelete);

        this->baseCreatedTerminus_.Emplace(
            this,
            upstream.internalBaseCreated_,
            &Base::OnBaseCreated_);

        this->baseWillDeleteTerminus_.Emplace(
            this,
            upstream.internalBaseWillDelete_,
            &Base::OnBaseWillDelete_);
    }
};
#endif


} // end namespace poly


} // end namespace pex
