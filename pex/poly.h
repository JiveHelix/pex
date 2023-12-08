#pragma once


#include <memory>
#include <fields/fields.h>
#include "pex/signal.h"
#include "pex/terminus.h"
#include "pex/identity.h"
#include "pex/detail/poly_detail.h"


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


template<typename Json_>
class PolyBase
{
public:
    using Json = Json_;
    using Base = PolyBase<Json_>;

    virtual ~PolyBase() {}

    virtual std::ostream & Describe(
        std::ostream &outputStream,
        const fields::Style &style,
        int indent) const = 0;

    virtual Json Unstructure() const = 0;
    virtual bool operator==(const PolyBase &) const = 0;
    virtual std::string_view GetName() const { return polyTypeName; }

    static constexpr auto polyTypeName = "PolyBase";
};


template
<
    typename PolyBase_,
    template<template<typename> typename> typename Template_
>
class PolyDerived: public PolyBase_, public Template_<pex::Identity>
{
public:
    static_assert(
        detail::IsCompatibleBase<PolyBase_>,
        "Expected virtual functions to be overloaded in this class");

    using Base = PolyBase_;
    using VirtualBase = typename detail::VirtualBase_<Base>::Type;
    using Json = typename Base::Json;
    using TemplateBase = Template_<pex::Identity>;

    PolyDerived()
        :
        Base(),
        TemplateBase()
    {

    }

    PolyDerived(const TemplateBase &other)
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
            PolyDerived::fields,
            style,
            indent);
    }

    Json Unstructure() const override
    {
        return pex::poly::PolyUnstructure<Json>(*this);
    }

    bool operator==(const VirtualBase &other) const override
    {
        auto otherPolyBase = dynamic_cast<const PolyDerived *>(&other);

        if (!otherPolyBase)
        {
            return false;
        }

        return (fields::ComparisonTuple(*this, PolyDerived::fields)
            == fields::ComparisonTuple(*otherPolyBase, PolyDerived::fields));
    }

    std::string_view GetTypeName() const override
    {
        return PolyDerived::fieldsTypeName;
    }
};


template
<
    typename Base_,
    template<template<typename> typename> typename ...DerivedTemplates
>
class Value
{
public:
    using Base = Base_;
    using Creator = detail::Creator_<PolyDerived<Base, DerivedTemplates>...>;

    static constexpr auto fieldsTypeName = Base::polyTypeName;

    Value()
        :
        value_{}
    {

    }

    Value(const std::shared_ptr<Base> &value)
        :
        value_(value)
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
            std::is_same_v<Json, typename Base::Json>,
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

    Value & operator=(std::shared_ptr<Base> value)
    {
        this->value_ = value;
        return *this;
    }

    operator bool () const
    {
        return !!this->value_;
    }

    const Base & Get() const
    {
        return *this->value_.get();
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
        const auto &base = this->Get();
        return dynamic_cast<const Derived *>(&base);
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
    std::shared_ptr<Base> value_;
};


TEMPLATE_OUTPUT_STREAM(Value)
TEMPLATE_COMPARISON_OPERATORS(Value)




template<typename T>
struct Control;


template<typename Value_>
struct Model
{
public:
    using Value = Value_;
    using Type = Value;
    using Base = detail::ModelBase_<Value_>;

    using ControlType = Control<Value_>;

    template<typename T>
    friend struct Control;

    Value Get() const
    {
        return this->base_->GetValue();
    }

    template<typename Derived>
    void Set(const Derived &derived)
    {
        if (!this->base_)
        {
            // Create the right kind of Base for this value.
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

    Base * GetBase()
    {
        return this->base_.get();
    }

private:
    std::unique_ptr<Base> base_;
    pex::model::Signal baseCreated_;
};


template<typename Value_>
struct Control
{
public:
    using Value = Value_;
    using Type = Value;
    using Plain = Type;
    using Base = detail::ControlBase_<Value>;
    using Callable = typename Base::Callable;
    using Upstream = Model<Value>;

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
        auto modelBase = upstream.GetBase();

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
        auto modelBase = upstream.GetBase();

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

    const Base * GetBase() const
    {
        assert(this->base_);
        return this->base_.get();
    }

    void Set(Value &value)
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

private:
    void OnBaseCreated_()
    {
        auto modelBase = this->upstream_->GetBase();

        if (modelBase)
        {
            this->base_ = modelBase->MakeControl();
        }
    }

private:
    using BaseCreatedTerminus =
        ::pex::Terminus<Control, pex::control::Signal<>>;

    Upstream *upstream_;
    std::shared_ptr<Base> base_;
    BaseCreatedTerminus baseCreated_;
};


template<typename ...T>
struct IsPolyControl_: std::false_type {};

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
