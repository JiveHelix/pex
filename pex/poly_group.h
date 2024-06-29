#pragma once

#include "pex/identity.h"
#include "pex/poly.h"
#include "pex/group.h"
#include "pex/interface.h"


namespace pex
{


namespace poly
{


template
<
    typename Templates
>
requires ::pex::HasMinimalSupers<Templates>
class PolyDerived_
    :
    public Templates::Supers::ValueBase,
    public Templates::template Template<pex::Identity>
{
public:
    using ValueBase = typename Templates::Supers::ValueBase;

    static_assert(
        detail::IsCompatibleBase<ValueBase>,
        "Expected virtual functions to be overloaded in this class");

    using VirtualBase = typename detail::VirtualBase_<ValueBase>::Type;
    using Json = typename ValueBase::Json;
    using TemplateBase = typename Templates::template Template<pex::Identity>;

    PolyDerived_()
        :
        ValueBase(),
        TemplateBase()
    {

    }

    PolyDerived_(const TemplateBase &other)
        :
        ValueBase(),
        TemplateBase(other)
    {

    }

    std::ostream & Describe(
        std::ostream &outputStream,
        const fields::Style &style,
        int indent) const override
    {
        return fields::DescribeFields(
            outputStream,
            *this,
            PolyDerived_::fields,
            style,
            indent);
    }

    Json Unstructure() const override
    {
        return pex::poly::PolyUnstructure<Json>(*this);
    }

    bool operator==(const VirtualBase &other) const override
    {
        auto otherPolyBase = dynamic_cast<const PolyDerived_ *>(&other);

        if (!otherPolyBase)
        {
            return false;
        }

        return (fields::ComparisonTuple(*this)
            == fields::ComparisonTuple(*otherPolyBase));
    }

    std::string_view GetTypeName() const override
    {
        return PolyDerived_::fieldsTypeName;
    }

    std::shared_ptr<ValueBase> Copy() const override
    {
        if constexpr (::pex::detail::HasImpl<Templates>)
        {
            using Type = Templates::template Impl<PolyDerived_>;
            auto self = dynamic_cast<const Type *>(this);

            if (!self)
            {
                throw std::logic_error(
                    "This is not the class you are looking for.");
            }

            return std::make_shared<Type>(*self);
        }
        else
        {
            return std::make_shared<PolyDerived_>(*this);
        }
    }
};


template
<
    typename Templates,
    typename = void
>
struct MakePolyDerived
{
    using Type = PolyDerived_<Templates>;
};


template<typename Templates>
struct MakePolyDerived
<
    Templates,
    std::enable_if_t<::pex::detail::HasImpl<Templates>>
>
{
    using Type =
        Templates::template Impl<PolyDerived_<Templates>>;
};


template<typename Templates>
using PolyDerived = typename MakePolyDerived<Templates>::Type;


template
<
    template<typename> typename Fields,
    ::pex::HasMinimalSupers Templates
>
struct PolyGroup
{
    using Supers = typename Templates::Supers;
    using ValueBase = typename Supers::ValueBase;
    using PolyValue_ = Value<ValueBase>;

    using ControlBase = detail::MakeControlBase<Supers>;

    using ModelBase =
        detail::MakeModelBase<Supers>;

    using Derived = PolyDerived<Templates>;
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

            Control(::pex::poly::Model<Supers> &model);

            Control(const ::pex::poly::Control<Supers> &control);

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
    template<typename> typename Fields,
    ::pex::HasMinimalSupers Templates
>
template<typename GroupBase>
std::shared_ptr<detail::MakeControlBase<typename Templates::Supers>>
PolyGroup<Fields, Templates>::GroupTemplates_
    ::Model<GroupBase>::MakeControl()
{
    using This = std::remove_cvref_t<decltype(*this)>;

    using DerivedModel =
        typename PolyGroup<Fields, Templates>::Model;

    static_assert(std::is_base_of_v<This, DerivedModel>);

    using Control =
        typename PolyGroup<Fields, Templates>::Control;

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
    template<typename> typename Fields,
    ::pex::HasMinimalSupers Templates
>
template<typename GroupBase>
std::shared_ptr<detail::MakeControlBase<typename Templates::Supers>>
PolyGroup<Fields, Templates>::GroupTemplates_
    ::TEMPLATE Control<GroupBase>::Copy() const
{
    using DerivedControl = typename PolyGroup<Fields, Templates>::Control;

    auto derivedControl = dynamic_cast<const DerivedControl *>(this);

    if (!derivedControl)
    {
        throw std::logic_error("Expected this class to be a base");
    }

    return std::make_shared<DerivedControl>(*derivedControl);
}


template
<
    template<typename> typename Fields,
    ::pex::HasMinimalSupers Templates
>
template<typename GroupBase>
PolyGroup<Fields, Templates>::GroupTemplates_
    ::TEMPLATE Control<GroupBase>::Control(
        ::pex::poly::Model<typename Templates::Supers> &model)
    :
    GroupBase()
{
    using DerivedControl = typename PolyGroup<Fields, Templates>::Control;

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
    template<typename> typename Fields,
    ::pex::HasMinimalSupers Templates
>
template<typename GroupBase>
PolyGroup<Fields, Templates>::GroupTemplates_
    ::TEMPLATE Control<GroupBase>::Control(
        const ::pex::poly::Control<typename Templates::Supers> &control)
    :
    GroupBase()
{
    using DerivedControl = typename PolyGroup<Fields, Templates>::Control;

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
