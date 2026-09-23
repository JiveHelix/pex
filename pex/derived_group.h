#pragma once


#include "pex/identity.h"
#include "pex/derived_value.h"
#include "pex/model_wrapper.h"
#include "pex/control_wrapper.h"
#include "pex/group.h"
#include "pex/interface.h"


namespace pex
{


namespace poly
{


template
<
    ::pex::HasMinimalSupers Templates
>
struct DerivedGroup
{
    using Supers = typename Templates::Supers;
    using ValueBase = typename Supers::ValueBase;
    using ValueWrapper = ValueWrapperTemplate<ValueBase>;

    using ControlBase = MakeSuperControl<Supers>;

    using SuperModel = MakeSuperModel<Supers>;
    using ModelWrapper = ::pex::poly::ModelWrapperTemplate<Supers>;

    using DerivedValue = DerivedValueTemplate<Templates>;
    using SchemaBase = typename DerivedValue::SchemaBase;

    static constexpr bool isDerivedGroup = true;

    struct GroupFinisher_
    {
        using Plain = DerivedValue;

        template<typename GroupBase>
        class Model
            :
            public SuperModel,
            public GroupBase
        {
        public:
            using ModelWrapper = DerivedGroup::ModelWrapper;
            using GroupBase::GroupBase;
            using typename GroupBase::Defer;

            Model()
                :
                GroupBase()
            {
                PEX_NAME_UNIQUE("poly::DerivedGroup::Model");
            }

            virtual ~Model()
            {
                PEX_CLEAR_NAME(this);
            }

            ValueWrapper GetValue() const override
            {
                return ValueWrapper(std::make_shared<DerivedValue>(this->Get()));
            }

            void SetValue(const ValueWrapper &value) override
            {
                this->Set(value.template RequireDerived<DerivedValue>());
            }

            std::string_view GetTypeName() const override
            {
                return ::pex::poly::GetTypeName<Templates>();
            }

            std::unique_ptr<MakeSuperControl<Supers>> CreateControl() override;

            void SetValueWithoutNotify(const ValueWrapper &value) override
            {
                this->SetWithoutNotify_(
                    value.template RequireDerived<DerivedValue>());
            }

            void DoValueNotify() override
            {
                this->Notify();
            }
        };

        template<typename GroupBase>
        class Control
            :
            public ControlBase,
            public GroupBase
        {
        public:
            using Upstream = typename GroupBase::Upstream;
            using typename GroupBase::Defer;

            template<typename BaseSignal>
            using ControlWrapper =
                ::pex::poly::ControlWrapperTemplate
                <
                    DerivedGroup::ModelWrapper,
                    Supers,
                    BaseSignal
                >;

            using Ensemble = typename GroupBase::Ensemble;

            virtual ~Control()
            {
                PEX_CLEAR_NAME(this);
                PEX_CLEAR_NAME(&this->ensemble_);
                PEX_CLEAR_NAME(&this->baseNotifier_);
            }

            Control()
                :
                GroupBase(),
                ensemble_(),
                baseNotifier_()
            {
                PEX_CONCISE_LOG(this);

                PEX_NAME(
                    fmt::format(
                        "DerivedGroup<{}>::Control<{}>",
                        jive::GetTypeName<Templates>(),
                        jive::GetTypeName<GroupBase>()));

                PEX_MEMBER(ensemble_);
                PEX_MEMBER(baseNotifier_);
            }

            Control(ModelWrapper &wrapper)
                :
                Control(*wrapper.GetVirtual())
            {

            }

            Control(SuperModel &model);

            template<typename BaseSignal>
            Control(const ControlWrapper<BaseSignal> &control);

            explicit Control(const Control &other)
                :
                GroupBase(other),
                ensemble_(),
                baseNotifier_(other.baseNotifier_)
            {
                PEX_CONCISE_LOG(this);

                PEX_NAME(
                    fmt::format(
                        "DerivedGroup<{}>::Control<{}>",
                        jive::GetTypeName<Templates>(),
                        jive::GetTypeName<GroupBase>()));

                PEX_MEMBER(ensemble_);
                PEX_MEMBER(baseNotifier_);

                if (this->baseNotifier_.HasConnections())
                {
                    this->ensemble_.AssignUpstream(*this);
                    this->ensemble_.Connect(this, &Control::OnEnsemble_);
                }
            }

            Control & operator=(const Control &other)
            {
                this->GroupBase::operator=(other);
                this->baseNotifier_ = other.baseNotifier_;

                if (this->baseNotifier_.HasConnections())
                {
                    this->ensemble_.AssignUpstream(*this);
                    this->ensemble_.Connect(this, &Control::OnEnsemble_);
                }

                return *this;
            }

            void StandardEmplace_(const Control &other)
            {
                this->GroupBase::StandardEmplace_(other);
                this->baseNotifier_ = other.baseNotifier_;

                if (this->baseNotifier_.HasConnections())
                {
                    this->ensemble_.AssignUpstream(*this);
                    this->ensemble_.Connect(this, &Control::OnEnsemble_);
                }
            }

            void StandardEmplace_(Upstream &upstream)
            {
                if (this->baseNotifier_.HasConnections())
                {
                    this->ensemble_.Disconnect(this);
                }

                this->GroupBase::StandardEmplace_(upstream);

                if (this->baseNotifier_.HasConnections())
                {
                    this->ensemble_.AssignUpstream(*this);
                    this->ensemble_.Connect(this, &Control::OnEnsemble_);
                }
            }

            ValueWrapper GetValue() const override
            {
                return ValueWrapper(
                    std::make_shared<DerivedValue>(this->Get()));
            }

            void SetValue(const ValueWrapper &value) override
            {
                this->Set(value.template RequireDerived<DerivedValue>());
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
                    this->ensemble_.AssignUpstream(*this);
                    this->ensemble_.Connect(this, &Control::OnEnsemble_);
                }

                this->baseNotifier_.ConnectOnce(observer, callable);

                PEX_LINK_OBSERVER(&this->ensemble_, observer);
            }

            void Disconnect(void *observer) override
            {
                this->baseNotifier_.Disconnect(observer);

                if (!this->baseNotifier_.HasConnections())
                {
                    this->ensemble_.Disconnect(this);
                    assert(!this->ensemble_.HasConnection());
                }
            }

            std::unique_ptr<MakeSuperControl<Supers>> Copy() const override;

            void SetValueWithoutNotify(const ValueWrapper &value) override
            {
                this->SetWithoutNotify_(
                    value.template RequireDerived<DerivedValue>());
            }

            void DoValueNotify() override
            {
                this->Notify();
            }

        private:
            static void OnEnsemble_(
                void * context,
                const DerivedValue &derived)
            {
                auto self = static_cast<Control *>(context);

                if (self->baseNotifier_.HasConnections())
                {
                    self->baseNotifier_.Notify(
                        ValueWrapper(std::make_shared<DerivedValue>(derived)));
                }
            }

        private:
            class BaseNotifier
                : public ::pex::detail::NotifyMany
                <
                    ::pex::detail::ValueConnection<void, ValueWrapper>,
                    ::pex::GetAndSetTag
                >
            {
            public:
                void Notify(const ValueWrapper &value)
                {
                    this->Notify_(value);
                }
            };

            Ensemble ensemble_;
            BaseNotifier baseNotifier_;
        };
    };

private:
    // The private group creates
    // GroupFinisher_::Model<pex::Group::Model_>
    // and GroupFinisher_::Control<pex::Group::Control_>
    using Group_ =
        ::pex::Group<Templates::template Schema, GroupFinisher_>;

public:
    // Allow the Customized types to inherit from Group::Model and Control.
    using Model =
        typename ::pex::detail::FinishModel
        <
            Templates,
            typename Group_::Model
        >;

    using Control =
        typename ::pex::detail::FinishControl
        <
            Templates,
            typename Group_::template Control<typename Group_::Model>
        >;

private:
    // Register the DerivedValue type so that it can be structured from json.
    static const inline bool once =
        []()
        {
            ValueBase::template RegisterDerived<DerivedValue>(
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


#include "pex/detail/derived_group_impl.h"
