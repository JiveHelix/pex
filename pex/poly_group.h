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
    typename Custom = void
>
struct PolyGroup
{
    using ControlBase =
        detail::MakeControlBase<Custom, PolyValue_>;

    using ModelBase =
        detail::MakeModelBase<Custom, PolyValue_>;

    using ValueBase = typename PolyValue_::ValueBase;
    using Derived = PolyDerived<typename PolyValue_::ValueBase, Template_>;
    using DerivedBase = typename Derived::TemplateBase;

    struct GroupTemplates_
    {
        using Plain = Derived;

        template<typename GroupBase>
        class Model
            :
            public ModelBase,
            public GroupBase
        {
        public:
            using GroupPlain = Derived;

            using GroupBase::GroupBase;

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

            std::shared_ptr<ValueBase> GetValueBase() const override
            {
                return std::make_shared<Derived>(this->Get());
            }

            void SetValueWithoutNotify(const PolyValue_ &value) override
            {
                this->SetWithoutNotify_(
                    value.template RequireDerived<Derived>());
            }

            void DoValueNotify() override
            {
                this->DoNotify_();
            }
        };

        template<typename GroupBase>
        class Control
            :
            public ControlBase,
            public GroupBase
        {
        public:
            using GroupBase::GroupBase;
            using Upstream = typename GroupBase::Upstream;
            using Aggregate = typename GroupBase::Aggregate;

            Control(::pex::poly::Model<PolyValue_, Custom> &model);

            Control(const ::pex::poly::Control<PolyValue_, Custom> &control);

            Control(const Control &other)
                :
                GroupBase(other),
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
                this->GroupBase::operator=(other);
                this->baseNotifier_ = other.baseNotifier_;

                if (this->baseNotifier_.HasConnections())
                {
                    this->aggregate_.AssignUpstream(*this);
                    this->aggregate_.Connect(this, &Control::OnAggregate_);
                }

                return *this;
            }

            std::shared_ptr<ValueBase> GetValueBase() const override
            {
                return std::make_shared<Derived>(this->Get());
            }

            PolyValue_ GetValue() const override
            {
                return PolyValue_(this->GetValueBase());
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

            std::shared_ptr<ControlBase> Copy() const override;

            void SetValueWithoutNotify(const PolyValue_ &value) override
            {
                this->SetWithoutNotify_(
                    value.template RequireDerived<Derived>());
            }

            void DoValueNotify() override
            {
                this->DoNotify_();
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

private:
    using Group_ = ::pex::Group<Fields_, Template_, GroupTemplates_>;

public:
    // Allow the Customized types to inherit from Group::Model and Control.
    using Model =
        typename ::pex::detail::CustomizeModel
        <
            Custom,
            typename Group_::Model
        >;

    using Control =
        typename ::pex::detail::CustomizeControl
        <
            Custom,
            typename Group_::Control
        >;

    // Inherit the ability to CreateModel().
    class PolyValue: public PolyValue_
    {
    public:
        using PolyValue_::PolyValue_;
        using DerivedControl = Control;

        std::unique_ptr<ModelBase> CreateModel() const
        {
            auto result = std::make_unique<Model>();
            result->Set(this->template RequireDerived<Derived>());

            return result;
        }

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

        PolyValue(Derived &&derived)
            :
            PolyValue_{std::make_shared<Derived>(std::move(derived))}
        {

        }

        static PolyValue Default()
        {
            if constexpr (HasDefault<Derived>)
            {
                return PolyValue(Derived::Default());
            }
            else
            {
                return PolyValue(Derived{});
            }
        }
    };

private:
    // Register the Derived type so that it can be structured from json.
    static const inline bool once =
        []()
        {
            ValueBase::template RegisterDerived<Derived>();
            return true;
        }();

    template<const bool &> struct ForceStaticInitialization {};
    using Force = ForceStaticInitialization<once>;
};


template
<
    template<typename> typename Fields_,
    template<template<typename> typename> typename Template_,
    typename PolyValue_,
    typename Custom
>
template<typename GroupBase>
std::shared_ptr<detail::MakeControlBase<Custom, PolyValue_>>
PolyGroup<Fields_, Template_, PolyValue_, Custom>::GroupTemplates_
    ::Model<GroupBase>::MakeControl()
{
    using This = std::remove_cvref_t<decltype(*this)>;

    using DerivedModel =
        typename PolyGroup<Fields_, Template_, PolyValue_, Custom>::Model;

    static_assert(std::is_base_of_v<This, DerivedModel>);

    using Control =
        typename PolyGroup<Fields_, Template_, PolyValue_, Custom>::Control;

    auto derivedModel = dynamic_cast<DerivedModel *>(this);

    if (!derivedModel)
    {
        throw std::logic_error("Expected this class to be a base");
    }

    return std::make_shared<Control>(*derivedModel);
}

#if defined(__GNUG__) && !defined(__clang__) && !defined(_WIN32)
// Avoid bogus -Wpedantic
#ifndef DO_PRAGMA
#define DO_PRAGMA_(arg) _Pragma (#arg)
#define DO_PRAGMA(arg) DO_PRAGMA_(arg)
#endif

#define GNU_NO_PEDANTIC_PUSH \
    DO_PRAGMA(GCC diagnostic push) \
    DO_PRAGMA(GCC diagnostic ignored "-Wpedantic")

#define GNU_NO_PEDANTIC_POP \
    DO_PRAGMA(GCC diagnostic pop)

// GNU compiler needs the template keyword to parse these definitions
// correctly.
#define TEMPLATE template

#else

#define GNU_NO_PEDANTIC_PUSH
#define GNU_NO_PEDANTIC_POP
#define TEMPLATE

#endif // defined __GNUG__


GNU_NO_PEDANTIC_PUSH

template
<
    template<typename> typename Fields_,
    template<template<typename> typename> typename Template_,
    typename PolyValue_,
    typename Custom
>
template<typename GroupBase>
std::shared_ptr<detail::MakeControlBase<Custom, PolyValue_>>
PolyGroup<Fields_, Template_, PolyValue_, Custom>::GroupTemplates_
    ::TEMPLATE Control<GroupBase>::Copy() const
{
    using DerivedControl =
        typename PolyGroup<Fields_, Template_, PolyValue_, Custom>::Control;

    auto derivedControl = dynamic_cast<const DerivedControl *>(this);

    if (!derivedControl)
    {
        throw std::logic_error("Expected this class to be a base");
    }

    return std::make_shared<DerivedControl>(*derivedControl);
}


template
<
    template<typename> typename Fields_,
    template<template<typename> typename> typename Template_,
    typename PolyValue_,
    typename Custom
>
template<typename GroupBase>
PolyGroup<Fields_, Template_, PolyValue_, Custom>::GroupTemplates_
    ::TEMPLATE Control<GroupBase>::Control(
        ::pex::poly::Model<PolyValue_, Custom> &model)
    :
    GroupBase()
{
    using DerivedControl =
        typename PolyGroup<Fields_, Template_, PolyValue_, Custom>::Control;

    auto base = model.GetVirtual();
    auto upcast = dynamic_cast<Upstream *>(base);

    if (!upcast)
    {
        throw PolyError("Mismatched polymorphic value");
    }

    *this = DerivedControl(*upcast);
}


template
<
    template<typename> typename Fields_,
    template<template<typename> typename> typename Template_,
    typename PolyValue_,
    typename Custom
>
template<typename GroupBase>
PolyGroup<Fields_, Template_, PolyValue_, Custom>::GroupTemplates_
    ::TEMPLATE Control<GroupBase>::Control(
        const ::pex::poly::Control<PolyValue_, Custom> &control)
    :
    GroupBase()
{
    using DerivedControl =
        typename PolyGroup<Fields_, Template_, PolyValue_, Custom>::Control;

    auto base = control.GetVirtual();
    auto upcast = dynamic_cast<const DerivedControl *>(base);

    if (!upcast)
    {
        throw PolyError("Mismatched polymorphic value");
    }

    *this = *upcast;
}

#if defined(__GNUG__) && !defined(__clang__) && !defined(_WIN32)
GNU_NO_PEDANTIC_POP
#undef TEMPLATE
#endif


} // end namespace poly


} // end namespace pex
