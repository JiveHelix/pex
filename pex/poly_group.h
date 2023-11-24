#pragma once

#include "pex/poly.h"
#include "pex/group.h"


namespace pex
{


namespace poly
{


template
<
    template<typename> typename Fields_,
    template<template<typename> typename> typename Template_,
    typename PolyValue_,
    typename Derived_
>
struct PolyGroup
{
    using ModelBase = ModelBase_<PolyValue_>;
    using ControlBase = ControlBase_<PolyValue_>;
    using Derived = Derived_;

    struct PolyValue: public PolyValue_
    {
        using PolyValue_::PolyValue_;

        std::unique_ptr<ModelBase> CreateModel() const;

        template<typename ...Args>
        static PolyValue Create(Args &&...args)
        {
            return {std::make_shared<Derived>(std::forward<Args>(args)...)};
        }
    };

    using Group = ::pex::Group<Fields_, Template_, Derived>;

    static const Derived * GetDerived(const PolyValue_ &value)
    {
        const auto &base = value.Get();
        return dynamic_cast<const Derived *>(&base);
    }

    static const Derived & RequireDerived(const PolyValue_ &value)
    {
        auto derived = GetDerived(value);

        if (!derived)
        {
            throw PolyError("Mismatched polymorphic value");
        }

        return *derived;
    }

    struct Model
        :
        public ModelBase,
        public Group::Model
    {
        using Group::Model::Model;

        PolyValue_ GetValue() const override
        {
            return PolyValue_(std::make_shared<Derived>(this->Get()));
        }

        void SetValue(const PolyValue_ &value) override
        {
            this->Set(RequireDerived(value));
        }

        std::string_view GetTypeName() const override
        {
            return Derived::fieldsTypeName;
        }

        std::shared_ptr<ControlBase> MakeControl() override;
    };

    struct Control
        :
        public ControlBase,
        public Group::Control
    {
        Control()
            :
            Group::Control()
        {

        }

        Control(Model &model)
            :
            Group::Control(model)
        {

        }

        Control(::pex::poly::Model<PolyValue_> &model)
        {
            auto base = model.GetBase();

            auto upcast = dynamic_cast<Model *>(base);

            if (!upcast)
            {
                throw PolyError("Mismatched polymorphic value");
            }

            *this = Control(*upcast);
        }

        Control(const ::pex::poly::Control<PolyValue_> &control)
        {
            auto base = control.GetBase();

            auto upcast = dynamic_cast<const Control *>(base);

            if (!upcast)
            {
                throw PolyError("Mismatched polymorphic value");
            }

            *this = *upcast;
        }

        PolyValue_ GetValue() const override
        {
            return PolyValue_(std::make_shared<Derived>(this->Get()));
        }

        void SetValue(const PolyValue_ &value) override
        {
            this->Set(RequireDerived(value));
        }

        std::string_view GetTypeName() const override
        {
            return Derived::fieldsTypeName;
        }
    };
};


template
<
    template<typename> typename Fields_,
    template<template<typename> typename> typename Template_,
    typename PolyValue_,
    typename Derived_
>
std::shared_ptr<ControlBase_<PolyValue_>>
PolyGroup<Fields_, Template_, PolyValue_, Derived_>::Model::MakeControl()
{
    using ThisGroup = PolyGroup<Fields_, Template_, PolyValue_, Derived_>;
    using Control = typename ThisGroup::Control;

    return std::make_shared<Control>(*this);
}


template
<
    template<typename> typename Fields_,
    template<template<typename> typename> typename Template_,
    typename PolyValue_,
    typename Derived_
>
std::unique_ptr<ModelBase_<PolyValue_>>
PolyGroup<Fields_, Template_, PolyValue_, Derived_>::PolyValue
    ::CreateModel() const
{
    return std::make_unique<Model>(RequireDerived(*this));
}


} // end namespace poly


} // end namespace pex
