#pragma once


#include <memory>
#include <nlohmann/json.hpp>
#include <fields/fields.h>
#include "pex/signal.h"
#include "pex/terminus.h"


namespace pex
{


namespace poly
{


CREATE_EXCEPTION(PolyError, PexError);


template<typename Value_>
struct ControlBase_
{
    using Value = Value_;

    virtual ~ControlBase_() {}
    virtual Value GetValue() const = 0;
    virtual void SetValue(const Value &) = 0;
    virtual std::string_view GetTypeName() const = 0;
};


template<typename Value_>
struct ModelBase_
{
    using Value = Value_;
    using ControlPtr = std::shared_ptr<ControlBase_<Value>>;

    virtual ~ModelBase_() {}
    virtual Value GetValue() const = 0;
    virtual void SetValue(const Value &) = 0;
    virtual std::string_view GetTypeName() const = 0;
    virtual ControlPtr MakeControl() = 0;
};


template<typename Creator>
class Value
{
public:
    using Base = typename Creator::Base;

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
            std::is_same_v<Json, nlohmann::json>,
            "Only works with nlohmann:json for now");

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

    template<typename Derived, typename ...Args>
    static Value Create(Args &&...args)
    {
        return {std::make_shared<Derived>(std::forward<Args>(args)...)};
    }

    Value & operator=(std::shared_ptr<Base> value)
    {
        this->value_ = value;
        return *this;
    }

    operator bool () const
    {
        return (this->value_);
    }

    const Base & Get() const
    {
        return *this->value_.get();
    }

    bool operator==(const Value &other) const
    {
        if (!this->value_ && other.value_)
        {
            return false;
        }

        return this->value_->operator==(*other.value_);
    }

private:
    std::shared_ptr<Base> value_;
};


TEMPLATE_OUTPUT_STREAM(Value)
TEMPLATE_COMPARISON_OPERATORS(Value)


template<typename Json, typename T>
Json PolyUnstructure(const T &object)
{
    auto jsonValues = fields::Unstructure<Json>(object);
    jsonValues["type"] = T::fieldsTypeName;

    return jsonValues;
}


template<typename Base_, typename ...Derived>
struct Creator
{
    using Base = Base_;

private:
    template<typename T, typename Json>
    static bool MakeDerived(std::shared_ptr<Base> &result, const Json &json)
    {
        if (json["type"] == T::fieldsTypeName)
        {
            result = std::make_shared<T>(fields::Structure<T>(json));
            return true;
        }

        return false;
    }

    template<typename Json>
    static std::shared_ptr<Base> MakeDerived(const Json &json)
    {
        std::shared_ptr<Base> result{};

        // Call MakeDerived with each derived type until a match is found.
        static_cast<void>(
            ((MakeDerived<Derived>(result, json) ? false : true) && ...));

        return result;
    }

public:
    template<typename Json>
    static std::shared_ptr<Base> Structure(const Json &jsonValues)
    {
        if (!jsonValues.contains("type"))
        {
            throw std::runtime_error(
                "Cannot structure an Aircraft without type information.");
        }

        auto result = MakeDerived<Json>(jsonValues);

        if (!result)
        {
            throw std::runtime_error("Unknown type");
        }

        return result;
    }
};


template<typename T>
struct Control;


template<typename Value_>
struct Model
{
public:
    using Value = Value_;
    using Type = Value;
    using Base = ModelBase_<Value_>;

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
    using Base = ControlBase_<Value>;
    using Upstream = Model<Value>;

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


} // end namespace poly


} // end namespace pex
