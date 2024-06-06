#pragma once


#include <memory>
#include <fields/fields.h>
#include "pex/signal.h"
#include "pex/terminus.h"
#include "pex/identity.h"
#include "pex/detail/choose_not_void.h"
#include "pex/detail/poly_detail.h"
#include "pex/detail/traits.h"


namespace pex
{


namespace poly
{


CREATE_EXCEPTION(PolyError, PexError);


template<typename Json, typename T>
Json PolyUnstructure(const T &object)
{
    auto jsonValues = fields::Unstructure<Json>(object);
    jsonValues["type"] = T::fieldsTypeName;

    return jsonValues;
}


template<typename Json_, typename Base_ = void>
class PolyBase
{
public:
    using Json = Json_;

    using Base =
        typename ::pex::detail::ChooseNotVoid<Base_, PolyBase<Json_>>;

    virtual ~PolyBase() {}

    virtual std::ostream & Describe(
        std::ostream &outputStream,
        const fields::Style &style,
        int indent) const = 0;

    virtual Json Unstructure() const = 0;
    virtual bool operator==(const Base &) const = 0;
    virtual std::string_view GetTypeName() const = 0;
    virtual std::shared_ptr<Base> Copy() const = 0;

    static constexpr auto polyTypeName = "PolyBase";
};


template
<
    typename PolyBase_,
    template<template<typename> typename> typename Template_
>
class PolyDerived_: public PolyBase_, public Template_<pex::Identity>
{
public:
    static_assert(
        detail::IsCompatibleBase<PolyBase_>,
        "Expected virtual functions to be overloaded in this class");

    using Base = PolyBase_;
    using VirtualBase = typename detail::VirtualBase_<Base>::Type;
    using Json = typename Base::Json;
    using TemplateBase = Template_<pex::Identity>;

    PolyDerived_()
        :
        Base(),
        TemplateBase()
    {

    }

    PolyDerived_(const TemplateBase &other)
        :
        Base(),
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

    std::shared_ptr<Base> Copy() const override
    {
        if constexpr (::pex::detail::HasImpl<Template_<pex::Identity>>)
        {
            using Type = Template_<pex::Identity>::template Impl<PolyDerived_>;
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
    typename Base,
    template<template<typename> typename> typename Template_,
    typename = void
>
struct MakePolyDerived
{
    using Type = PolyDerived_<Base, Template_>;
};


template
<
    typename Base,
    template<template<typename> typename> typename Template_
>
struct MakePolyDerived
<
    Base,
    Template_,
    std::enable_if_t<::pex::detail::HasImpl<Template_<pex::Identity>>>
>
{
    using Type =
        Template_<pex::Identity>::template Impl<PolyDerived_<Base, Template_>>;
};


template
<
    typename Base,
    template<template<typename> typename> typename Template_
>
using PolyDerived = typename MakePolyDerived<Base, Template_>::Type;


template
<
    typename ValueBase_,
    template<template<typename> typename> typename ...DerivedTemplates
>
class Value
{
public:
    using ValueBase = ValueBase_;

    using Creator =
        detail::Creator_<PolyDerived<ValueBase, DerivedTemplates>...>;

    static constexpr auto fieldsTypeName = ValueBase::polyTypeName;

    Value()
        :
        value_{}
    {

    }

    Value(const std::shared_ptr<ValueBase> &value)
        :
        value_(value)
    {

    }

    Value(const ValueBase &value)
        :
        value_(value.Copy())
    {

    }

    std::ostream & Describe(
        std::ostream &outputStream,
        const fields::Style &style,
        int indent) const
    {
        if (!this->value_)
        {
            throw std::logic_error("Unitialized member");
        }

        return this->value_->Describe(outputStream, style, indent);
    }

    template<typename Json>
    Json Unstructure() const
    {
        static_assert(
            std::is_same_v<Json, typename ValueBase::Json>,
            "Must match PolyBase Json type");

        if (!this->value_)
        {
            throw std::logic_error("Unitialized member");
        }

        return this->value_->Unstructure();
    }

    template<typename Json>
    static Value Structure(const Json &jsonValues)
    {
        return {Creator::Structure(jsonValues)};
    }

    template<typename Derived, typename ...Args>
    void Initialize(Args &&...args)
    {
        this->value_ = std::make_shared<Derived>(std::forward<Args>(args)...);
    }

    Value & operator=(std::shared_ptr<ValueBase> value)
    {
        this->value_ = value;
        return *this;
    }

    operator bool () const
    {
        return !!this->value_;
    }

    std::shared_ptr<const ValueBase> GetValueBase() const
    {
        return this->value_;
    }

    std::shared_ptr<ValueBase> GetValueBase()
    {
        return this->value_;
    }

    bool operator==(const Value &other) const
    {
        if (!(this->value_ && other.value_))
        {
            return false;
        }

        return this->value_->operator==(*other.value_);
    }

    template<typename Derived>
    const Derived * GetDerived() const
    {
        const auto base = this->value_.get();
        return dynamic_cast<const Derived *>(base);
    }

    template<typename Derived>
    const Derived & RequireDerived() const
    {
        auto derived = this->template GetDerived<Derived>();

        if (!derived)
        {
            throw PolyError("Mismatched polymorphic value");
        }

        return *derived;
    }

private:
    std::shared_ptr<ValueBase> value_;
};


TEMPLATE_OUTPUT_STREAM(Value)
TEMPLATE_COMPARISON_OPERATORS(Value)

template<typename T, typename Custom = void>
class Control;


template<typename Value_, typename Custom = void>
class Model
{
public:
    using Value = Value_;
    using Type = Value;
    using ModelBase = detail::MakeModelBase<Custom, Value>;
    using ControlType = Control<Value_, Custom>;

    template<typename, typename>
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


template<typename Value_, typename Custom>
class Control
{
public:
    using Value = Value_;
    using Type = Value;
    using Plain = Type;

    using ControlBase = detail::MakeControlBase<Custom, Value>;

    using Callable = typename ControlBase::Callable;
    using Upstream = Model<Value, Custom>;

    static constexpr bool isPexCopyable = true;

    Control()
        :
        upstream_(),
        base_(),
        baseCreated_()
    {

    }

    Control(Upstream &upstream)
        :
        upstream_(&upstream),
        base_(),
        baseCreated_(
            this,
            this->upstream_->baseCreated_,
            &Control::OnBaseCreated_)
    {
        auto modelBase = upstream.GetVirtual();

        if (modelBase)
        {
            this->base_ = modelBase->MakeControl();
        }
    }

    Control(const Control &other)
        :
        upstream_(other.upstream_),
        base_(other.base_),
        baseCreated_(
            this,
            this->upstream_->baseCreated_,
            &Control::OnBaseCreated_)
    {

    }

    Control(void *observer, Upstream &upstream, Callable callable)
        :
        upstream_(upstream),
        base_(),
        baseCreated_(
            this,
            this->upstream_->baseCreated_,
            &Control::OnBaseCreated_)
    {
        auto modelBase = upstream.GetVirtual();

        if (modelBase)
        {
            this->base_ = modelBase->MakeControl();
            this->Connect(observer, callable);
        }
    }

    Control(void *observer, const Control &other, Callable callable)
        :
        upstream_(other.upstream_),
        base_(other.base_),
        baseCreated_(
            this,
            this->upstream_->baseCreated_,
            &Control::OnBaseCreated_)
    {
        if (!this->base_)
        {
            throw std::logic_error("Cannot connect without a valid object.");
        }

        this->Connect(observer, callable);
    }

    Control & operator=(const Control &other)
    {
        this->upstream_ = other.upstream_;
        this->base_ = other.base_;
        this->baseCreated_.Assign(this, other.baseCreated_);

        return *this;
    }

    Value Get() const
    {
        assert(this->base_);
        return this->base_->GetValue();
    }

    std::string_view GetTypeName() const
    {
        assert(this->base_);
        return this->base_->GetTypeName();
    }

    const ControlBase * GetVirtual() const
    {
        assert(this->base_);
        return this->base_.get();
    }

    ControlBase * GetVirtual()
    {
        assert(this->base_);
        return this->base_.get();
    }

    void Set(const Value &value)
    {
        assert(this->base_);
        this->base_->SetValue(value);
    }

    operator bool () const
    {
        return (this->base_);
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
    void OnBaseCreated_()
    {
        auto modelBase = this->upstream_->GetVirtual();

        if (modelBase)
        {
            this->base_ = modelBase->MakeControl();
        }
    }

private:
    using BaseCreatedTerminus =
        ::pex::Terminus<Control, pex::control::Signal<>>;

    Upstream *upstream_;
    std::shared_ptr<ControlBase> base_;
    BaseCreatedTerminus baseCreated_;
};


template<typename ...T>
struct IsPolyControl_: std::false_type {};

template<typename T, typename Custom>
struct IsPolyControl_<Control<T, Custom>>: std::true_type {};

template<typename T>
struct IsPolyControl_<Control<T>>: std::true_type {};

template<typename T>
inline constexpr bool IsPolyControl = IsPolyControl_<T>::value;


} // end namespace poly


template<typename T>
struct IsControl_
<
    T,
    std::enable_if_t<poly::IsPolyControl<T>>
>: std::true_type {};


} // end namespace pex
