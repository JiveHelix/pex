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
    typename PolyValue_
>
struct PolyGroup
{
    using ModelBase = detail::ModelBase_<PolyValue_>;
    using ControlBase = detail::ControlBase_<PolyValue_>;
    using Derived = PolyDerived<typename PolyValue_::Base, Template_>;
    using DerivedBase = typename Derived::TemplateBase;

    struct PolyValue: public PolyValue_
    {
        using PolyValue_::PolyValue_;

        std::unique_ptr<ModelBase> CreateModel() const;

        PolyValue()
            :
            PolyValue_()
        {

        }

        PolyValue(DerivedBase &&base)
            :
            PolyValue_{std::make_shared<Derived>(std::move(base))}
        {

        }
    };

    using Group = ::pex::Group<Fields_, Template_, Derived>;


    struct Control;


    struct Model
        :
        public ModelBase,
        public Group::Model
    {
        using Group::Model::Model;
        using ControlType = Control;

        PolyValue_ GetValue() const override
        {
            return PolyValue_(std::make_shared<Derived>(this->Get()));
        }

        void SetValue(const PolyValue_ &value) override
        {
            this->Set(value.template RequireDerived<Derived>());
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
        using Upstream = Model;

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

        Control(const Control &other)
            :
            aggregate_(),
            baseNotifier_(other.baseNotifier_)
        {
            if (this->baseNotifier_.HasConnections())
            {
                this->aggregate_.AssignUpstream(*this);
                this->aggregate_.Connect(this, &Control::OnAggregate_);
            }
        }

        Control & operator=(const Control &other)
        {
            this->Group::Control::operator=(other);
            this->baseNotifier_ = other.baseNotifier_;

            if (this->baseNotifier_.HasConnections())
            {
                this->aggregate_.AssignUpstream(*this);
                this->aggregate_.Connect(this, &Control::OnAggregate_);
            }

            return *this;
        }

        PolyValue_ GetValue() const override
        {
            return PolyValue_(std::make_shared<Derived>(this->Get()));
        }

        void SetValue(const PolyValue_ &value) override
        {
            this->Set(value.template RequireDerived<Derived>());
        }

        std::string_view GetTypeName() const override
        {
            return Derived::fieldsTypeName;
        }

        using BaseCallable = typename ControlBase::Callable;

        void Connect(void *observer, BaseCallable callable) override
        {
            if (!this->baseNotifier_.HasConnections())
            {
                this->aggregate_.AssignUpstream(*this);
                this->aggregate_.Connect(this, &Control::OnAggregate_);
            }

            this->baseNotifier_.ConnectOnce(observer, callable);
        }

        void Disconnect(void *observer) override
        {
            this->baseNotifier_.Disconnect(observer);

            if (!this->baseNotifier_.HasConnections())
            {
                this->aggregate_.Disconnect(this);
            }
        }

    private:
        static void OnAggregate_(void * context, const Derived &derived)
        {
            auto self = static_cast<Control *>(context);

            if (self->baseNotifier_.HasConnections())
            {
                self->baseNotifier_.Notify(
                    PolyValue_(std::make_shared<Derived>(derived)));
            }
        }

    private:
        using Aggregate = typename Group::Aggregate;

        class BaseNotifier
            : public ::pex::detail::NotifyMany
            <
                ::pex::detail::ValueConnection<void, PolyValue_>,
                ::pex::GetAndSetTag
            >
        {
        public:
            void Notify(const PolyValue_ &value)
            {
                this->Notify_(value);
            }
        };

        Aggregate aggregate_;
        BaseNotifier baseNotifier_;
    };
};


template
<
    template<typename> typename Fields_,
    template<template<typename> typename> typename Template_,
    typename PolyValue_
>
std::shared_ptr<detail::ControlBase_<PolyValue_>>
PolyGroup<Fields_, Template_, PolyValue_>::Model::MakeControl()
{
    using ThisGroup = PolyGroup<Fields_, Template_, PolyValue_>;
    using Control = typename ThisGroup::Control;

    return std::make_shared<Control>(*this);
}


template
<
    template<typename> typename Fields_,
    template<template<typename> typename> typename Template_,
    typename PolyValue_
>
std::unique_ptr<detail::ModelBase_<PolyValue_>>
PolyGroup<Fields_, Template_, PolyValue_>::PolyValue
    ::CreateModel() const
{
    return std::make_unique<Model>(this->template RequireDerived<Derived>());
}


} // end namespace poly


} // end namespace pex
