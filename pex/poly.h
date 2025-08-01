#pragma once


#include "pex/identity.h"
#include "pex/poly_derived.h"
#include "pex/poly_model.h"
#include "pex/group.h"
#include "pex/interface.h"


namespace pex
{


namespace poly
{


template
<
    template<typename> typename Fields,
    ::pex::HasMinimalSupers Templates
>
struct Poly
{
    using Supers = typename Templates::Supers;
    using ValueBase = typename Supers::ValueBase;
    using PolyValue = Value<ValueBase>;

    using ControlBase = MakeControlSuper<Supers>;

    using SuperModel = MakeModelSuper<Supers>;

    using Derived = PolyDerived<Templates>;
    using TemplateBase = typename Derived::TemplateBase;

    static constexpr bool isPolyGroup = true;

    struct GroupTemplates_
    {
        using Plain = Derived;

        template<typename GroupBase>
        class Model
            :
            public SuperModel,
            public GroupBase
        {
        public:
            using GroupBase::GroupBase;

            virtual ~Model() {}

            PolyValue GetValue() const override
            {
                return PolyValue(std::make_shared<Derived>(this->Get()));
            }

            void SetValue(const PolyValue &value) override
            {
                this->Set(value.template RequireDerived<Derived>());
            }

            std::string_view GetTypeName() const override
            {
                return ::pex::poly::GetTypeName<Templates>();
            }

            std::shared_ptr<MakeControlSuper<Supers>> CreateControl() override;

            void SetValueWithoutNotify(const PolyValue &value) override
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
            // using GroupBase::GroupBase;
            using Upstream = typename GroupBase::Upstream;
            using Aggregate = typename GroupBase::Aggregate;

            virtual ~Control() {}

            Control()
                :
                GroupBase(),
                aggregate_(),
                baseNotifier_()
            {
                PEX_NAME(
                    fmt::format(
                        "Poly<Fields, {}>::Control<{}>",
                        jive::GetTypeName<Templates>(),
                        jive::GetTypeName<GroupBase>()));

                PEX_MEMBER(aggregate_);
                PEX_MEMBER(baseNotifier_);
            }

            Control(SuperModel &model);

            Control(const ::pex::poly::Control<Supers> &control);

            Control(const Control &other)
                :
                GroupBase(other),
                aggregate_(),
                baseNotifier_(other.baseNotifier_)
            {
                PEX_NAME(
                    fmt::format(
                        "Poly<Fields, {}>::Control<{}>",
                        jive::GetTypeName<Templates>(),
                        jive::GetTypeName<GroupBase>()));

                PEX_MEMBER(aggregate_);
                PEX_MEMBER(baseNotifier_);

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

            PolyValue GetValue() const override
            {
                return PolyValue(std::make_shared<Derived>(this->Get()));
            }

            void SetValue(const PolyValue &value) override
            {
                this->Set(value.template RequireDerived<Derived>());
            }

            std::string_view GetTypeName() const override
            {
                return ::pex::poly::GetTypeName<Templates>();
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

            std::shared_ptr<MakeControlSuper<Supers>> Copy() const override;

            void SetValueWithoutNotify(const PolyValue &value) override
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
                        PolyValue(std::make_shared<Derived>(derived)));
                }
            }

        private:
            class BaseNotifier
                : public ::pex::detail::NotifyMany
                <
                    ::pex::detail::ValueConnection<void, PolyValue>,
                    ::pex::GetAndSetTag
                >
            {
            public:
                void Notify(const PolyValue &value)
                {
                    this->Notify_(value);
                }
            };

            Aggregate aggregate_;
            BaseNotifier baseNotifier_;
        };
    };

private:
    using Group_ =
        ::pex::Group<Fields, Templates::template Template, GroupTemplates_>;

public:
    // Allow the Customized types to inherit from Group::Model and Control.
    using Model =
        typename ::pex::detail::CustomizeModel
        <
            Templates,
            typename Group_::Model
        >;

    using Control =
        typename ::pex::detail::CustomizeControl
        <
            Templates,
            typename Group_::Control
        >;

private:
    // Register the Derived type so that it can be structured from json.
    static const inline bool once =
        []()
        {
            ValueBase::template RegisterDerived<Derived>(
                std::string(::pex::poly::GetTypeName<Templates>()));

            ValueBase::template RegisterModel<Model>(
                std::string(::pex::poly::GetTypeName<Templates>()));

            return true;
        }();

    template<const bool &> struct ForceStaticInitialization {};
    using Force = ForceStaticInitialization<once>;
};


} // end namespace poly


} // end namespace pex


#include "pex/detail/poly_impl.h"
