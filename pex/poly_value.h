#pragma once


#include <memory>
#include "pex/error.h"
#include "pex/get_type_name.h"


namespace pex
{


namespace poly
{


CREATE_EXCEPTION(PolyError, PexError);


// Manages polymorphic member value_ (ValueBase_).
template<typename ValueBase_>
class Value
{
public:
    using ValueBase = ValueBase_;
    using ModelBase = typename ValueBase::ModelBase;

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

    Value(const Value &other)
        :
        value_(other.value_->Copy())
    {

    }

    Value & operator=(const Value &other)
    {
        this->value_ = other.value_->Copy();

        return *this;
    }

    Value(Value &&other)
        :
        value_(std::move(other.value_))
    {

    }

    Value & operator=(Value &&other)
    {
        this->value_ = std::move(other.value_);

        return *this;
    }

    template<typename Derived, typename ...Args>
    static Value Create(Args && ...args)
    {
        using TemplateBase = typename Derived::TemplateBase;

        return Value(
            std::make_shared<Derived>(
                TemplateBase{std::forward<Args>(args)...}));
    }

    template<typename Derived>
    static Value Default()
    {
        return Value(std::make_shared<Derived>());
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

    std::string_view GetTypeName() const
    {
        if (!this->value_)
        {
            return "NULL";
        }

        return this->value_->GetTypeName();
    }

    template<typename Json>
    Json Unstructure() const
    {
        static_assert(
            std::is_same_v<Json, typename ValueBase::Json>,
            "Must match ValueBase Json type");

        if (!this->value_)
        {
            throw std::logic_error("Unitialized member");
        }

        return this->value_->Unstructure();
    }

    template<typename Json>
    static Value Structure(const Json &jsonValues)
    {
        return {ValueBase::Structure(jsonValues)};
    }

    bool CheckModel(ModelBase *modelBase) const
    {
        if (!this->value_)
        {
            throw std::logic_error("Unitialized member");
        }

        return this->value_->CheckModel(modelBase);
    }

    std::unique_ptr<ModelBase> CreateModel() const
    {
        if (!this->value_)
        {
            throw std::logic_error("Unitialized member");
        }

        return this->value_->CreateModel();
    }

#if 0
    template<typename Derived, typename ...Args>
    void Initialize(Args &&...args)
    {
        this->value_ = std::make_shared<Derived>(std::forward<Args>(args)...);
    }
#endif

    Value & operator=(std::shared_ptr<ValueBase> value)
    {
        this->value_ = value.Copy();
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
            if (!this->value_)
            {
                throw PolyError("Unitialized");
            }

            std::cout << "dynamic cast failed from "
                << this->value_->GetTypeName()
                << " to "
                << Derived::DoGetTypeName() << std::endl;

            this->value_->ReportAddress("Failed dynamic_cast");

            throw PolyError("Mismatched polymorphic value");
        }

        return *derived;
    }

    template<typename Derived>
    Derived * GetDerived()
    {
        auto base = this->value_.get();
        return dynamic_cast<Derived *>(base);
    }

    template<typename Derived>
    Derived & RequireDerived()
    {
        auto derived = this->template GetDerived<Derived>();

        if (!derived)
        {
            if (!this->value_)
            {
                throw PolyError("Unitialized");
            }

            std::cout << "dynamic cast failed from "
                << this->value_->GetTypeName()
                << " to "
                << Derived::DoGetTypeName() << std::endl;

            this->value_->ReportAddress("Failed dynamic_cast");

            throw PolyError("Mismatched polymorphic value");
        }

        return *derived;
    }

private:
    std::shared_ptr<ValueBase> value_;
};


} // end namespace poly


} // end namespace pex
