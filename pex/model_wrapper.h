#pragma once


#include <fmt/core.h>
#include "pex/detail/log.h"
#include "pex/poly_supers.h"
#include "pex/traits.h"


namespace pex
{


namespace poly
{


template<HasValueBase Supers>
struct MakeControlSuper_
{
    using Type =
        ControlSuper
        <
            typename Supers::ValueBase,
            detail::MakeControlUserBase<Supers>
        >;
};

template<HasValueBase Supers>
using MakeControlSuper = typename MakeControlSuper_<Supers>::Type;


template <HasValueBase Supers>
struct MakeModelSuper_
{
    using Type =
        ModelSuper
        <
            typename Supers::ValueBase,
            detail::MakeModelUserBase<Supers>,
            MakeControlSuper<Supers>
        >;
};


template <HasValueBase Supers>
using MakeModelSuper = typename MakeModelSuper_<Supers>::Type;


template<typename Upstream, HasValueBase, typename BaseSignal>
class Control;


// This Model is the same wrapper type for every item in a polymorphic list.
// It manages a virtual object.
// TODO: This extra layer may not be necessary. Why not have a list of
// unique_ptr?
//
template<HasValueBase Supers>
class ModelWrapperTemplate
{
public:
    using Access = GetAccess<Supers>;
    using ValueBase = typename Supers::ValueBase;
    using ValueWrapper = ::pex::poly::ValueWrapperTemplate<ValueBase>;
    using Type = ValueWrapper;
    using ModelBase = typename ValueWrapper::ModelBase;
    using SuperModel = MakeModelSuper<Supers>;

    static constexpr bool isModelWrapper = true;
    static_assert(std::is_base_of_v<ModelBase, SuperModel>);

    using Signal = ::pex::control::Signal<::pex::model::Signal>;

    template<typename Upstream, HasValueBase, typename>
    friend class ControlWrapperTemplate;

    ModelWrapperTemplate()
        :
        base_{},
        superModel_{},
        baseWillDelete_{},
        baseCreated_{},
        internalBaseCreated_{},
        internalBaseWillDelete_{}
    {
        PEX_NAME(
            fmt::format(
                "ModelWrapperTemplate<{}>",
                jive::GetTypeName<Supers>()));

        PEX_MEMBER(baseWillDelete_);
        PEX_MEMBER(baseCreated_);
        PEX_MEMBER(internalBaseCreated_);
        PEX_MEMBER(internalBaseWillDelete_);
    };

    ~ModelWrapperTemplate()
    {
        PEX_CLEAR_NAME(this);
        PEX_CLEAR_NAME(&baseWillDelete_);
        PEX_CLEAR_NAME(&baseCreated_);
        PEX_CLEAR_NAME(&internalBaseCreated_);
        PEX_CLEAR_NAME(&internalBaseWillDelete_);
    }

    ValueWrapper Get() const
    {
        return this->superModel_->GetValue();
    }

    std::string_view GetTypeName() const
    {
        return this->base_->GetTypeName();
    }

    SuperModel * GetVirtual()
    {
        return this->superModel_;
    }

    template<typename DerivedModel>
    DerivedModel & RequireDerived()
    {
        auto result = dynamic_cast<DerivedModel *>(this->base_.get());

        if (!result)
        {
            throw PolyError("Mismatched polymorphic value");
        }

        return *result;
    }

    void Set(const ValueWrapper &value)
    {
        this->SetWithoutNotify_(value);
        this->Notify();
    }

    void Notify()
    {
        this->superModel_->DoValueNotify();
    }

// TODO: Add this to pex::Reference
// protected:
    void SetWithoutNotify_(const ValueWrapper &value)
    {
        if (!value.CheckModel(this->base_.get()))
        {
            if (this->base_)
            {
                // Notify that the base_ will be replaced.
                this->baseWillDelete_.Trigger();

                this->internalBaseWillDelete_.TriggerMayModify();
            }

            // Create the right kind of ModelBase for this value.
            this->base_ = value.CreateModel();
            this->superModel_ = dynamic_cast<SuperModel *>(this->base_.get());

            if (!this->superModel_)
            {
                throw std::logic_error(
                    "SuperModel must be derived from ModelBase");
            }

            this->superModel_->SetValueWithoutNotify(value);

            // Create the new control before signaling the rest of the library.
            // Use the slower TriggerMayModify to allow the new ControlWrapper
            // to connect itself to this signal.
            this->internalBaseCreated_.TriggerMayModify();

            this->baseCreated_.Trigger();
        }
        else
        {
            this->superModel_->SetValueWithoutNotify(value);
        }
    }

    Signal GetBaseWillDelete()
    {
        return {this->baseWillDelete_};
    }

    Signal GetBaseCreated()
    {
        return {this->baseCreated_};
    }

private:
    std::unique_ptr<ModelBase> base_;
    SuperModel *superModel_;
    pex::model::Signal baseWillDelete_;
    pex::model::Signal baseCreated_;
    pex::model::Signal internalBaseCreated_;
    pex::model::Signal internalBaseWillDelete_;
};


} // end namespace poly


} // end namespace pex
