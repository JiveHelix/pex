#pragma once


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


template<HasValueBase Supers>
class Control;


template<HasValueBase Supers>
class Model
{
public:
    using Access = GetAccess<Supers>;
    using ValueBase = typename Supers::ValueBase;
    using Value = ::pex::poly::Value<ValueBase>;
    using Type = Value;
    using ModelBase = typename Value::ModelBase;
    using SuperModel = MakeModelSuper<Supers>;

    static constexpr bool isPolyModel = true;
    static_assert(std::is_base_of_v<ModelBase, SuperModel>);

    using ControlType = Control<Supers>;

    template<HasValueBase>
    friend class Control;

    Value Get() const
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

    void Set(const Value &value)
    {
        if (!value.CheckModel(this->base_.get()))
        {
            // Create the right kind of ModelBase for this value.
            this->base_ = value.CreateModel();

            this->superModel_ = dynamic_cast<SuperModel *>(this->base_.get());

            if (!this->superModel_)
            {
                throw std::logic_error(
                    "SuperModel must be derived from ModelBase");
            }

            this->superModel_->SetValue(value);
            this->baseCreated_.Trigger();
        }
        else
        {
            this->superModel_->SetValue(value);
        }
    }

// TODO: Add this to pex::Reference
// protected:
    void SetWithoutNotify_(const Value &value)
    {
        if (!value.CheckModel(this->base_.get()))
        {
            // Create the right kind of ModelBase for this value.
            this->base_ = value.CreateModel();
            this->superModel_ = dynamic_cast<SuperModel *>(this->base_.get());

            if (!this->superModel_)
            {
                throw std::logic_error(
                    "SuperModel must be derived from ModelBase");
            }

            this->superModel_->SetValueWithoutNotify(value);
            this->baseCreated_.Trigger();
        }
        else
        {
            this->superModel_->SetValueWithoutNotify(value);
        }
    }

    void DoNotify_()
    {
        this->superModel_->DoValueNotify();
    }


private:
    std::unique_ptr<ModelBase> base_;
    SuperModel *superModel_;
    pex::model::Signal baseCreated_;
};


} // end namespace poly


} // end namespace pex
