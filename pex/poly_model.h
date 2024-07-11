#pragma once


#include "pex/poly_value.h"
#include "pex/detail/poly_detail.h"


namespace pex
{


namespace poly
{


template<typename ValueBase, typename ControlUserBase>
using ControlBase =
    detail::ControlBase_<::pex::poly::Value<ValueBase>, ControlUserBase>;


template<typename Supers>
struct MakeControlBase_
{
    using Type =
        ControlBase
        <
            typename Supers::ValueBase,
            detail::MakeControlUserBase<Supers>
        >;
};

template<typename Supers>
using MakeControlBase = typename MakeControlBase_<Supers>::Type;


template
<
    typename ValueBase,
    typename ModelUserBase,
    typename ControlUserBase
>
using ModelBase =
    detail::ModelBase_
    <
        ::pex::poly::Value<ValueBase>,
        ModelUserBase,
        ControlBase<ValueBase, ControlUserBase>
    >;


template <typename Supers>
struct MakeModelBase_
{
    using Type =
        detail::ModelBase_
        <
            ::pex::poly::Value<typename Supers::ValueBase>,
            detail::MakeModelUserBase<Supers>,
            MakeControlBase<Supers>
        >;
};


template <typename Supers>
using MakeModelBase = typename MakeModelBase_<Supers>::Type;


template<HasValueBase Supers>
class Control;


template<HasValueBase Supers>
class Model
{
public:
    using ValueBase = typename Supers::ValueBase;
    using Value = ::pex::poly::Value<ValueBase>;
    using Type = Value;
    using ModelBase = MakeModelBase<Supers>;
    using ControlType = Control<Supers>;

    template<HasValueBase>
    friend class Control;

    Value Get() const
    {
        return this->base_->GetValue();
    }

    template<typename Derived>
    void Set(const Derived &derived)
    {
        if (!this->base_)
        {
            // Create the right kind of ModelBase for this value.
            this->base_ = derived.CreateModel();
            this->base_->SetValue(derived);
            this->baseCreated_.Trigger();
        }
        else
        {
            this->base_->SetValue(derived);
        }
    }

    std::string_view GetTypeName() const
    {
        return this->base_->GetTypeName();
    }

    ModelBase * GetVirtual()
    {
        return this->base_.get();
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
    std::unique_ptr<ModelBase> base_;
    pex::model::Signal baseCreated_;
};





} // end namespace poly


} // end namespace pex
